/**
 * @file AsyncComms.hpp
 * @brief Asynchronous communication patterns and message passing systems
 * @details File location: src/concurrency/AsyncComms.hpp
 * 
 * This file demonstrates various asynchronous communication patterns including
 * message queues, publish-subscribe systems, and actor-like communication.
 */

#ifndef ASYNCCOMMS_HPP
#define ASYNCCOMMS_HPP

#include <thread>
#include <future>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <functional>
#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <iostream>
#include <chrono>
#include <any>
#include <variant>
#include <optional>

namespace CppVerseHub::Concurrency {

    /**
     * @class Message
     * @brief Generic message structure for async communication
     */
    struct Message {
        std::string type;
        std::any payload;
        std::string sender_id;
        std::chrono::steady_clock::time_point timestamp;
        std::optional<std::string> correlation_id;

        template<typename T>
        Message(const std::string& msg_type, T&& data, const std::string& sender = "")
            : type(msg_type), payload(std::forward<T>(data)), sender_id(sender),
              timestamp(std::chrono::steady_clock::now()) {}

        template<typename T>
        T get_payload() const {
            try {
                return std::any_cast<T>(payload);
            } catch (const std::bad_any_cast& e) {
                throw std::runtime_error("Invalid payload type cast: " + std::string(e.what()));
            }
        }

        bool has_correlation_id() const { return correlation_id.has_value(); }
        void set_correlation_id(const std::string& id) { correlation_id = id; }
    };

    /**
     * @class MessageQueue
     * @brief Thread-safe message queue with capacity management
     */
    template<typename T>
    class MessageQueue {
    public:
        explicit MessageQueue(size_t max_capacity = 1000) : max_capacity_(max_capacity) {}

        bool send(T message, std::chrono::milliseconds timeout = std::chrono::milliseconds::zero()) {
            std::unique_lock<std::mutex> lock(mutex_);
            
            if (timeout == std::chrono::milliseconds::zero()) {
                not_full_.wait(lock, [this] { return queue_.size() < max_capacity_ || closed_; });
            } else {
                if (!not_full_.wait_for(lock, timeout, [this] { return queue_.size() < max_capacity_ || closed_; })) {
                    return false; // Timeout
                }
            }
            
            if (closed_) return false;
            
            queue_.push(std::move(message));
            not_empty_.notify_one();
            return true;
        }

        std::optional<T> receive(std::chrono::milliseconds timeout = std::chrono::milliseconds::zero()) {
            std::unique_lock<std::mutex> lock(mutex_);
            
            if (timeout == std::chrono::milliseconds::zero()) {
                not_empty_.wait(lock, [this] { return !queue_.empty() || closed_; });
            } else {
                if (!not_empty_.wait_for(lock, timeout, [this] { return !queue_.empty() || closed_; })) {
                    return std::nullopt; // Timeout
                }
            }
            
            if (queue_.empty() && closed_) return std::nullopt;
            
            T message = std::move(queue_.front());
            queue_.pop();
            not_full_.notify_one();
            return message;
        }

        void close() {
            std::lock_guard<std::mutex> lock(mutex_);
            closed_ = true;
            not_empty_.notify_all();
            not_full_.notify_all();
        }

        size_t size() const {
            std::lock_guard<std::mutex> lock(mutex_);
            return queue_.size();
        }

        bool is_closed() const {
            std::lock_guard<std::mutex> lock(mutex_);
            return closed_;
        }

    private:
        std::queue<T> queue_;
        mutable std::mutex mutex_;
        std::condition_variable not_empty_;
        std::condition_variable not_full_;
        size_t max_capacity_;
        bool closed_{false};
    };

    /**
     * @class PubSubSystem
     * @brief Publish-Subscribe messaging system
     */
    class PubSubSystem {
    public:
        using MessageHandler = std::function<void(const Message&)>;
        using SubscriptionId = size_t;

        PubSubSystem() = default;
        ~PubSubSystem() { shutdown(); }

        SubscriptionId subscribe(const std::string& topic, MessageHandler handler);
        bool unsubscribe(const std::string& topic, SubscriptionId sub_id);
        void publish(const std::string& topic, const Message& message);
        
        void start_processing();
        void shutdown();
        
        size_t subscriber_count(const std::string& topic) const;
        std::vector<std::string> get_topics() const;

    private:
        struct Subscription {
            SubscriptionId id;
            MessageHandler handler;
        };

        std::unordered_map<std::string, std::vector<Subscription>> subscriptions_;
        MessageQueue<std::pair<std::string, Message>> message_queue_;
        std::thread processing_thread_;
        std::atomic<SubscriptionId> next_sub_id_{1};
        mutable std::mutex subscriptions_mutex_;
        std::atomic<bool> running_{false};

        void process_messages();
    };

    /**
     * @class AsyncChannel
     * @brief Bidirectional async communication channel
     */
    template<typename SendType, typename ReceiveType = SendType>
    class AsyncChannel {
    public:
        explicit AsyncChannel(size_t buffer_size = 100)
            : send_queue_(buffer_size), receive_queue_(buffer_size) {}

        std::future<bool> async_send(SendType message) {
            return std::async(std::launch::async, [this, msg = std::move(message)]() mutable {
                return send_queue_.send(std::move(msg), std::chrono::milliseconds(1000));
            });
        }

        std::future<std::optional<ReceiveType>> async_receive() {
            return std::async(std::launch::async, [this]() {
                return receive_queue_.receive(std::chrono::milliseconds(1000));
            });
        }

        bool send(SendType message, std::chrono::milliseconds timeout = std::chrono::milliseconds(100)) {
            return send_queue_.send(std::move(message), timeout);
        }

        std::optional<ReceiveType> receive(std::chrono::milliseconds timeout = std::chrono::milliseconds(100)) {
            return receive_queue_.receive(timeout);
        }

        void close() {
            send_queue_.close();
            receive_queue_.close();
        }

        // For bidirectional communication
        MessageQueue<SendType>& get_send_queue() { return send_queue_; }
        MessageQueue<ReceiveType>& get_receive_queue() { return receive_queue_; }

    private:
        MessageQueue<SendType> send_queue_;
        MessageQueue<ReceiveType> receive_queue_;
    };

    /**
     * @class ActorSystem
     * @brief Simple actor-based communication system
     */
    class ActorSystem {
    public:
        class Actor {
        public:
            Actor(const std::string& name, ActorSystem* system)
                : name_(name), system_(system), mailbox_(1000) {}
            
            virtual ~Actor() { stop(); }

            void start();
            void stop();
            void send_message(const Message& message);
            const std::string& name() const { return name_; }

        protected:
            virtual void handle_message(const Message& message) = 0;
            virtual void on_start() {}
            virtual void on_stop() {}
            
            void send_to_actor(const std::string& actor_name, const Message& message);

        private:
            std::string name_;
            ActorSystem* system_;
            MessageQueue<Message> mailbox_;
            std::thread actor_thread_;
            std::atomic<bool> running_{false};

            void actor_loop();
        };

        void register_actor(std::shared_ptr<Actor> actor);
        void unregister_actor(const std::string& name);
        void send_message(const std::string& actor_name, const Message& message);
        void shutdown();

        size_t actor_count() const;
        std::vector<std::string> get_actor_names() const;

    private:
        std::unordered_map<std::string, std::shared_ptr<Actor>> actors_;
        mutable std::mutex actors_mutex_;
    };

    /**
     * @class RequestResponseSystem
     * @brief Async request-response communication pattern
     */
    class RequestResponseSystem {
    public:
        using RequestHandler = std::function<Message(const Message&)>;
        using ResponseCallback = std::function<void(const Message&)>;

        void register_handler(const std::string& request_type, RequestHandler handler);
        void unregister_handler(const std::string& request_type);

        std::future<Message> send_request(const Message& request);
        void send_request_async(const Message& request, ResponseCallback callback);

        void start_processing();
        void shutdown();

    private:
        std::unordered_map<std::string, RequestHandler> handlers_;
        std::unordered_map<std::string, std::promise<Message>> pending_requests_;
        std::unordered_map<std::string, ResponseCallback> pending_callbacks_;
        
        MessageQueue<Message> request_queue_;
        std::thread processing_thread_;
        std::atomic<bool> running_{false};
        mutable std::mutex handlers_mutex_;
        mutable std::mutex pending_mutex_;

        void process_requests();
        std::string generate_correlation_id();
    };

    /**
     * @class AsyncCommDemo
     * @brief Comprehensive demonstration of async communication patterns
     */
    class AsyncCommDemo {
    public:
        static void demonstrate_message_queue();
        static void demonstrate_pubsub_system();
        static void demonstrate_async_channel();
        static void demonstrate_actor_system();
        static void demonstrate_request_response();
        static void demonstrate_space_communication_network();
        static void run_all_demonstrations();

    private:
        // Space-themed demonstration classes
        class SpaceStationActor;
        class MissionControlActor;
        class SatelliteActor;
        
        struct SpaceMessage {
            std::string mission_id;
            std::string data;
            double coordinates[3];
        };

        static void simulate_network_delay();
    };

} // namespace CppVerseHub::Concurrency

#endif // ASYNCCOMMS_HPP