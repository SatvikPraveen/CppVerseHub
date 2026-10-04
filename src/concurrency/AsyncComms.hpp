/**
 * @file AsyncComms.hpp
 * @brief Asynchronous communication patterns: publish/subscribe, actors, request/response.
 * @details File location: src/concurrency/AsyncComms.hpp
 *
 * Message passing replaces shared mutable state with ownership transfer: each piece of
 * state is touched by exactly one thread, and threads communicate by sending values
 * through queues. This header shows three canonical shapes:
 *
 *  - `MessageBus`: topic-based publish/subscribe with one dispatcher thread, which gives
 *    a total delivery order; handlers run without any bus lock held, so a handler may
 *    itself publish or (un)subscribe without deadlocking.
 *  - `Actor<Msg>`: a private mailbox drained by a private thread; the behaviour owns its
 *    state, so it needs no locks. Combined with `std::variant` messages and a
 *    `std::promise` reply slot this gives the "ask" pattern.
 *  - `RequestResponseServer`: named request handlers served by a `ThreadPool`, each request
 *    answered through a `std::future` (unknown request types fail the future).
 */

#ifndef CPPVERSEHUB_CONCURRENCY_ASYNCCOMMS_HPP
#define CPPVERSEHUB_CONCURRENCY_ASYNCCOMMS_HPP

#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <functional>
#include <future>
#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <shared_mutex>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include "concurrency/ConditionalVariables.hpp"
#include "concurrency/ThreadPool.hpp"

namespace CppVerseHub::Concurrency {

    /** @brief A message routed by topic. */
    struct Message {
        std::string topic;                           ///< Routing key.
        std::string payload;                         ///< Opaque body.
        std::uint64_t sender_id = 0;                 ///< Logical sender.
        std::optional<std::uint64_t> correlation_id; ///< Links a reply to its request.
    };

    /**
     * @brief Topic-based publish/subscribe bus with an internal dispatcher thread.
     *
     * Guarantees: messages are delivered in publication order (global FIFO); a handler
     * subscribed when a message is *dispatched* receives it; `flush()` returns once the bus
     * is idle, i.e. every message published before (or during) the call has been delivered. Exceptions thrown by a
     * handler are caught and counted (`handler_failures()`).
     *
     * Re-entrancy: a handler may call `publish`, `subscribe` and `unsubscribe`. Publications
     * made from the dispatcher thread bypass the bounded queue (into a dispatcher-private
     * backlog delivered before the next queued message), so a handler can never block on
     * a full queue that only it could drain. A handler must not call `flush()` or `shutdown()`.
     */
    class MessageBus {
    public:
        using Handler = std::function<void(const Message&)>;  ///< Subscriber callback.
        using SubscriptionId = std::uint64_t;                 ///< Handle for `unsubscribe`.

        /**
         * @brief Starts the dispatcher.
         * @param queue_capacity Maximum number of undelivered messages before `publish` blocks.
         */
        explicit MessageBus(std::size_t queue_capacity = 1024);
        MessageBus(const MessageBus&) = delete;
        MessageBus& operator=(const MessageBus&) = delete;
        MessageBus(MessageBus&&) = delete;
        MessageBus& operator=(MessageBus&&) = delete;
        /** @brief Delivers everything already published, then stops. */
        ~MessageBus();

        /**
         * @brief Registers a handler for a topic.
         * @param topic Topic to listen on.
         * @param handler Callback, invoked on the dispatcher thread.
         * @return Subscription identifier.
         */
        SubscriptionId subscribe(const std::string& topic, Handler handler);

        /**
         * @brief Removes a subscription. A delivery already in progress may still complete.
         * @param id Identifier returned by `subscribe`.
         * @return true if it existed.
         */
        bool unsubscribe(SubscriptionId id);

        /**
         * @brief Queues a message for asynchronous delivery.
         * @param message Message (its `topic` selects the subscribers).
         * @return false if the bus has been shut down.
         */
        bool publish(Message message);

        /**
         * @brief Blocks until the bus is idle (all published messages, including those published
         *        by handlers in the meantime, delivered). May wait indefinitely under a continuous
         *        stream of publications.
         */
        void flush();

        /** @brief Stops accepting messages, delivers the backlog and joins the dispatcher. */
        void shutdown();

        /** @brief @param topic Topic. @return Number of subscribers on `topic`. */
        [[nodiscard]] std::size_t subscriber_count(const std::string& topic) const;
        /** @brief @return Topics with at least one subscriber, sorted. */
        [[nodiscard]] std::vector<std::string> topics() const;
        /** @brief @return Number of (message, handler) deliveries performed. */
        [[nodiscard]] std::uint64_t deliveries() const noexcept { return deliveries_.load(); }
        /** @brief @return Number of handler invocations that threw. */
        [[nodiscard]] std::uint64_t handler_failures() const noexcept { return failures_.load(); }

    private:
        struct Subscription {
            SubscriptionId id;
            std::string topic;
            std::shared_ptr<const Handler> handler;
        };

        void dispatch_loop();

        mutable std::shared_mutex subs_mutex_;
        std::map<SubscriptionId, Subscription> subscriptions_;
        SubscriptionId next_id_ = 1;

        BoundedQueue<Message> queue_;
        std::deque<Message> reentrant_;  // only touched by the dispatcher thread
        std::mutex progress_mutex_;
        std::condition_variable progress_cv_;
        std::uint64_t published_ = 0;  // guarded by progress_mutex_
        std::uint64_t delivered_ = 0;  // guarded by progress_mutex_

        std::atomic<std::uint64_t> deliveries_{0};
        std::atomic<std::uint64_t> failures_{0};
        std::mutex join_mutex_;
        std::thread dispatcher_;
    };

    /**
     * @brief An actor: a mailbox plus a dedicated thread running the behaviour on each message.
     *
     * The behaviour is only ever invoked from the actor's thread, one message at a time, so
     * state captured by it needs no synchronisation. `stop()` (and the destructor) process
     * every message accepted before the call.
     * @tparam Msg Message type (typically a `std::variant` of commands).
     */
    template <typename Msg>
    class Actor {
    public:
        using Behaviour = std::function<void(Msg&)>;  ///< Message handler.

        /**
         * @brief Starts the actor thread.
         * @param behaviour Handler run for every message (exceptions are caught and counted).
         * @param mailbox_capacity Mailbox bound; `tell` blocks when full.
         */
        explicit Actor(Behaviour behaviour, std::size_t mailbox_capacity = 256)
            : behaviour_(std::move(behaviour)), mailbox_(mailbox_capacity), thread_([this] { run(); }) {}

        Actor(const Actor&) = delete;
        Actor& operator=(const Actor&) = delete;
        Actor(Actor&&) = delete;
        Actor& operator=(Actor&&) = delete;
        /** @brief Drains the mailbox and joins the thread. */
        ~Actor() { stop(); }

        /**
         * @brief Sends a message (fire and forget).
         * @param message Message to enqueue.
         * @return false if the actor has been stopped.
         */
        template <typename U = Msg>
        bool tell(U&& message) {
            return mailbox_.push(std::forward<U>(message));
        }

        /** @brief Closes the mailbox, processes the backlog and joins. Idempotent. */
        void stop() {
            mailbox_.close();
            std::lock_guard lock(join_mutex_);
            if (thread_.joinable()) {
                thread_.join();
            }
        }

        /** @brief @return Messages processed so far. */
        [[nodiscard]] std::uint64_t processed() const noexcept { return processed_.load(); }
        /** @brief @return Messages whose handler threw. */
        [[nodiscard]] std::uint64_t failures() const noexcept { return failures_.load(); }

    private:
        void run() {
            while (auto message = mailbox_.pop()) {
                try {
                    behaviour_(*message);
                } catch (...) {
                    failures_.fetch_add(1);
                }
                processed_.fetch_add(1);
            }
        }

        Behaviour behaviour_;
        BoundedQueue<Msg> mailbox_;
        std::atomic<std::uint64_t> processed_{0};
        std::atomic<std::uint64_t> failures_{0};
        std::mutex join_mutex_;
        std::thread thread_;  // last member: starts after everything it uses is constructed
    };

    /**
     * @brief Thrown (through the future) when no handler is registered for a request type.
     */
    class UnknownRequestError : public std::runtime_error {
    public:
        /** @brief @param type The unrecognised request type. */
        explicit UnknownRequestError(const std::string& type)
            : std::runtime_error("no handler registered for request type '" + type + "'") {}
    };

    /**
     * @brief Request/response server: handlers keyed by request type, executed on a thread pool.
     */
    class RequestResponseServer {
    public:
        using Handler = std::function<std::string(const std::string&)>;  ///< payload -> reply.

        /** @brief Reply to a request. */
        struct Response {
            std::uint64_t correlation_id = 0;  ///< Matches `Ticket::correlation_id`.
            std::string payload;               ///< Handler output.
        };

        /** @brief Handle for an in-flight request. */
        struct Ticket {
            std::uint64_t correlation_id = 0;  ///< Unique id assigned by the server.
            std::future<Response> response;    ///< Becomes ready when the handler finishes.
        };

        /** @brief @param workers Number of worker threads serving requests. */
        explicit RequestResponseServer(std::size_t workers = 2) : pool_(workers) {}

        /**
         * @brief Registers or replaces the handler for a request type.
         * @param type Request type.
         * @param handler Handler.
         */
        void register_handler(const std::string& type, Handler handler);
        /** @brief @param type Request type. @return true if a handler was removed. */
        bool unregister_handler(const std::string& type);

        /**
         * @brief Sends a request.
         * @param type Request type (resolved when the request executes).
         * @param payload Request body.
         * @return Ticket whose future yields the response or `UnknownRequestError`.
         * @throws PoolShutdownError after `shutdown()`.
         */
        [[nodiscard]] Ticket request(const std::string& type, std::string payload);

        /** @brief Completes in-flight requests and stops the workers. */
        void shutdown() { pool_.shutdown(); }

    private:
        mutable std::shared_mutex handlers_mutex_;
        std::map<std::string, std::shared_ptr<const Handler>> handlers_;
        std::atomic<std::uint64_t> next_correlation_{1};
        ThreadPool pool_;  // last: destroyed (and drained) first, while handlers_ is alive
    };

    /**
     * @brief Showcase: pub/sub fan-out, an actor with the ask pattern, request/response.
     * @param out Destination stream.
     */
    void demonstrate_async_comms(std::ostream& out = std::cout);

}  // namespace CppVerseHub::Concurrency

#endif  // CPPVERSEHUB_CONCURRENCY_ASYNCCOMMS_HPP
