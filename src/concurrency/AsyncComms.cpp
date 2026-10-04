/**
 * @file AsyncComms.cpp
 * @brief Message bus and request/response server implementations, plus the showcase.
 * @details File location: src/concurrency/AsyncComms.cpp
 */

#include "concurrency/AsyncComms.hpp"

#include <set>
#include <variant>

namespace CppVerseHub::Concurrency {

// ---------------------------------------------------------------- MessageBus

namespace {
// The bus whose dispatcher runs on the current thread (nullptr elsewhere).
thread_local const MessageBus* tls_dispatching_bus = nullptr;
} // namespace

MessageBus::MessageBus(std::size_t queue_capacity) : queue_(queue_capacity) {
    dispatcher_ = std::thread([this] { dispatch_loop(); });
}

MessageBus::~MessageBus() {
    shutdown();
}

MessageBus::SubscriptionId MessageBus::subscribe(const std::string& topic, Handler handler) {
    std::unique_lock lock(subs_mutex_);
    const SubscriptionId id = next_id_++;
    subscriptions_.emplace(id, Subscription{id, topic, std::make_shared<const Handler>(std::move(handler))});
    return id;
}

bool MessageBus::unsubscribe(SubscriptionId id) {
    std::unique_lock lock(subs_mutex_);
    return subscriptions_.erase(id) != 0;
}

bool MessageBus::publish(Message message) {
    if (tls_dispatching_bus == this) {
        if (queue_.is_closed()) {
            return false;
        }
        {
            std::lock_guard lock(progress_mutex_);
            ++published_;
        }
        reentrant_.push_back(std::move(message));
        return true;
    }
    {
        std::lock_guard lock(progress_mutex_);
        ++published_;
    }
    if (!queue_.push(std::move(message))) {
        {
            std::lock_guard lock(progress_mutex_);
            --published_;
        }
        progress_cv_.notify_all();
        return false;
    }
    return true;
}

void MessageBus::flush() {
    // Re-entrant publications are delivered out of queue order, so "the first N deliveries"
    // is not "the first N messages"; waiting for quiescence is the robust criterion.
    std::unique_lock lock(progress_mutex_);
    progress_cv_.wait(lock, [this] { return delivered_ == published_; });
}

void MessageBus::shutdown() {
    queue_.close();
    std::lock_guard lock(join_mutex_);
    if (dispatcher_.joinable()) {
        dispatcher_.join();
    }
}

std::size_t MessageBus::subscriber_count(const std::string& topic) const {
    std::shared_lock lock(subs_mutex_);
    std::size_t n = 0;
    for (const auto& entry : subscriptions_) {
        const Subscription& sub = entry.second;
        n += (sub.topic == topic) ? 1 : 0;
    }
    return n;
}

std::vector<std::string> MessageBus::topics() const {
    std::shared_lock lock(subs_mutex_);
    std::set<std::string> unique;
    for (const auto& entry : subscriptions_) {
        const Subscription& sub = entry.second;
        unique.insert(sub.topic);
    }
    return {unique.begin(), unique.end()};
}

void MessageBus::dispatch_loop() {
    tls_dispatching_bus = this;
    std::vector<std::shared_ptr<const Handler>> targets;
    for (;;) {
        std::optional<Message> message;
        if (!reentrant_.empty()) {
            message.emplace(std::move(reentrant_.front()));
            reentrant_.pop_front();
        } else {
            message = queue_.pop();
            if (!message) {
                break; // closed and drained
            }
        }
        targets.clear();
        {
            std::shared_lock lock(subs_mutex_);
            for (const auto& entry : subscriptions_) {
                const Subscription& sub = entry.second;
                if (sub.topic == message->topic) {
                    targets.push_back(sub.handler);
                }
            }
        }
        // Invoke without holding any bus lock: handlers may publish or (un)subscribe.
        for (const auto& handler : targets) {
            try {
                (*handler)(*message);
            } catch (...) {
                failures_.fetch_add(1);
            }
            deliveries_.fetch_add(1);
        }
        {
            std::lock_guard lock(progress_mutex_);
            ++delivered_;
        }
        progress_cv_.notify_all();
    }
    tls_dispatching_bus = nullptr;
}

// ---------------------------------------------------------------- RequestResponseServer

void RequestResponseServer::register_handler(const std::string& type, Handler handler) {
    std::unique_lock lock(handlers_mutex_);
    handlers_[type] = std::make_shared<const Handler>(std::move(handler));
}

bool RequestResponseServer::unregister_handler(const std::string& type) {
    std::unique_lock lock(handlers_mutex_);
    return handlers_.erase(type) != 0;
}

RequestResponseServer::Ticket RequestResponseServer::request(const std::string& type, std::string payload) {
    const std::uint64_t id = next_correlation_.fetch_add(1, std::memory_order_relaxed);
    auto future = pool_.submit([this, type, body = std::move(payload), id]() -> Response {
        std::shared_ptr<const Handler> handler;
        {
            std::shared_lock lock(handlers_mutex_);
            const auto it = handlers_.find(type);
            if (it != handlers_.end()) {
                handler = it->second;
            }
        }
        if (!handler) {
            throw UnknownRequestError(type);
        }
        return Response{id, (*handler)(body)};
    });
    return Ticket{id, std::move(future)};
}

// ---------------------------------------------------------------- showcase

namespace {

struct Deposit {
    int amount;
};
struct QueryBalance {
    std::promise<int> reply;
};
using AccountCommand = std::variant<Deposit, QueryBalance>;

template <typename... Fs>
struct Overloaded : Fs... {
    using Fs::operator()...;
};
template <typename... Fs>
Overloaded(Fs...) -> Overloaded<Fs...>;

} // namespace

void demonstrate_async_comms(std::ostream& out) {
    out << "=== Asynchronous communication ===\n";

    {
        // Declared before the bus so they outlive the dispatcher thread.
        std::vector<std::string> station_log; // touched only on the dispatcher thread
        std::vector<std::string> rover_log;   // ditto
        MessageBus bus;
        static_cast<void>(
            bus.subscribe("telemetry", [&](const Message& m) { station_log.push_back(m.payload); }));
        static_cast<void>(
            bus.subscribe("telemetry", [&](const Message& m) { rover_log.push_back(m.payload); }));
        static_cast<void>(
            bus.subscribe("alerts", [&](const Message& m) { station_log.push_back("ALERT " + m.payload); }));
        for (int i = 0; i < 3; ++i) {
            static_cast<void>(
                bus.publish(Message{"telemetry", "frame-" + std::to_string(i), 7, std::nullopt}));
        }
        static_cast<void>(bus.publish(Message{"alerts", "solar-flare", 7, std::nullopt}));
        bus.flush(); // establishes happens-before with the dispatcher's writes
        out << "MessageBus topics:";
        for (const auto& t : bus.topics()) {
            out << ' ' << t;
        }
        out << "\n  station received:";
        for (const auto& s : station_log) {
            out << " [" << s << ']';
        }
        out << "\n  rover received " << rover_log.size() << " telemetry frames; total deliveries "
            << bus.deliveries() << '\n';
    }

    {
        int balance = 0; // owned by the actor thread
        Actor<AccountCommand> account([&balance](AccountCommand& command) {
            std::visit(Overloaded{[&](Deposit& d) { balance += d.amount; },
                                  [&](QueryBalance& q) { q.reply.set_value(balance); }},
                       command);
        });
        std::vector<std::thread> clients;
        for (int c = 0; c < 4; ++c) {
            clients.emplace_back([&account] {
                for (int i = 0; i < 25; ++i) {
                    static_cast<void>(account.tell(AccountCommand{Deposit{10}}));
                }
            });
        }
        for (auto& t : clients) {
            t.join();
        }
        QueryBalance query;
        std::future<int> answer = query.reply.get_future();
        static_cast<void>(account.tell(AccountCommand{std::move(query)}));
        out << "Actor (ask pattern): balance after 100 deposits of 10 = " << answer.get() << '\n';
    }

    {
        RequestResponseServer server(2);
        server.register_handler("echo", [](const std::string& s) { return s; });
        server.register_handler("upper", [](const std::string& s) {
            std::string r = s;
            for (char& c : r) {
                if (c >= 'a' && c <= 'z') {
                    c = static_cast<char>(c - 'a' + 'A');
                }
            }
            return r;
        });
        auto t1 = server.request("upper", "dock at bay 3");
        auto t2 = server.request("echo", "ping");
        auto t3 = server.request("warp", "engage");
        const auto r1 = t1.response.get();
        out << "Request #" << r1.correlation_id << " upper -> \"" << r1.payload << "\"\n";
        out << "Request #" << t2.correlation_id << " echo -> \"" << t2.response.get().payload << "\"\n";
        try {
            static_cast<void>(t3.response.get());
        } catch (const UnknownRequestError& e) {
            out << "Request #" << t3.correlation_id << " failed: " << e.what() << '\n';
        }
    }
    out << '\n';
}

} // namespace CppVerseHub::Concurrency
