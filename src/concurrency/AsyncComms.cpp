/**
 * @file AsyncComms.cpp
 * @brief Implementation of asynchronous communication patterns
 * @details File location: src/concurrency/AsyncComms.cpp
 */

#include "AsyncComms.hpp"
#include <random>
#include <sstream>
#include <iomanip>

namespace CppVerseHub::Concurrency {

    // PubSubSystem Implementation
    PubSubSystem::SubscriptionId PubSubSystem::subscribe(const std::string& topic, MessageHandler handler) {
        std::lock_guard<std::mutex> lock(subscriptions_mutex_);
        SubscriptionId id = next_sub_id_.fetch_add(1);
        subscriptions_[topic].push_back({id, std::move(handler)});
        std::cout << "PubSub: Subscribed to topic '" << topic << "' with ID " << id << "\n";
        return id;
    }

    bool PubSubSystem::unsubscribe(const std::string& topic, SubscriptionId sub_id) {
        std::lock_guard<std::mutex> lock(subscriptions_mutex_);
        auto it = subscriptions_.find(topic);
        if (it != subscriptions_.end()) {
            auto& subs = it->second;
            auto sub_it = std::find_if(subs.begin(), subs.end(),
                [sub_id](const Subscription& sub) { return sub.id == sub_id; });
            if (sub_it != subs.end()) {
                subs.erase(sub_it);
                std::cout << "PubSub: Unsubscribed from topic '" << topic << "' ID " << sub_id << "\n";
                if (subs.empty()) {
                    subscriptions_.erase(it);
                }
                return true;
            }
        }
        return false;
    }

    void PubSubSystem::publish(const std::string& topic, const Message& message) {
        if (!running_.load()) {
            std::cout << "PubSub: Cannot publish - system not running\n";
            return;
        }
        message_queue_.send({topic, message});
    }

    void PubSubSystem::start_processing() {
        if (!running_.exchange(true)) {
            processing_thread_ = std::thread(&PubSubSystem::process_messages, this);
            std::cout << "PubSub: Started message processing\n";
        }
    }

    void PubSubSystem::shutdown() {
        if (running_.exchange(false)) {
            message_queue_.close();
            if (processing_thread_.joinable()) {
                processing_thread_.join();
            }
            std::cout << "PubSub: Shutdown complete\n";
        }
    }

    size_t PubSubSystem::subscriber_count(const std::string& topic) const {
        std::lock_guard<std::mutex> lock(subscriptions_mutex_);
        auto it = subscriptions_.find(topic);
        return (it != subscriptions_.end()) ? it->second.size() : 0;
    }

    std::vector<std::string> PubSubSystem::get_topics() const {
        std::lock_guard<std::mutex> lock(subscriptions_mutex_);
        std::vector<std::string> topics;
        for (const auto& [topic, subs] : subscriptions_) {
            topics.push_back(topic);
        }
        return topics;
    }

    void PubSubSystem::process_messages() {
        while (running_.load() || message_queue_.size() > 0) {
            auto msg_pair = message_queue_.receive(std::chrono::milliseconds(100));
            if (!msg_pair.has_value()) continue;

            const auto& [topic, message] = msg_pair.value();
            
            std::lock_guard<std::mutex> lock(subscriptions_mutex_);
            auto it = subscriptions_.find(topic);
            if (it != subscriptions_.end()) {
                for (const auto& subscription : it->second) {
                    try {
                        subscription.handler(message);
                    } catch (const std::exception& e) {
                        std::cout << "PubSub: Handler exception for topic '" << topic 
                                  << "': " << e.what() << "\n";
                    }
                }
            }
        }
    }

    // ActorSystem::Actor Implementation
    void ActorSystem::Actor::start() {
        if (!running_.exchange(true)) {
            actor_thread_ = std::thread(&Actor::actor_loop, this);
            on_start();
            std::cout << "Actor '" << name_ << "' started\n";
        }
    }

    void ActorSystem::Actor::stop() {
        if (running_.exchange(false)) {
            mailbox_.close();
            if (actor_thread_.joinable()) {
                actor_thread_.join();
            }
            on_stop();
            std::cout << "Actor '" << name_ << "' stopped\n";
        }
    }

    void ActorSystem::Actor::send_message(const Message& message) {
        mailbox_.send(message);
    }

    void ActorSystem::Actor::send_to_actor(const std::string& actor_name, const Message& message) {
        if (system_) {
            system_->send_message(actor_name, message);
        }
    }

    void ActorSystem::Actor::actor_loop() {
        while (running_.load() || mailbox_.size() > 0) {
            auto message = mailbox_.receive(std::chrono::milliseconds(100));
            if (message.has_value()) {
                try {
                    handle_message(message.value());
                } catch (const std::exception& e) {
                    std::cout << "Actor '" << name_ << "' message handling exception: " 
                              << e.what() << "\n";
                }
            }
        }
    }

    // ActorSystem Implementation
    void ActorSystem::register_actor(std::shared_ptr<Actor> actor) {
        std::lock_guard<std::mutex> lock(actors_mutex_);
        actors_[actor->name()] = actor;
        std::cout << "ActorSystem: Registered actor '" << actor->name() << "'\n";
    }

    void ActorSystem::unregister_actor(const std::string& name) {
        std::lock_guard<std::mutex> lock(actors_mutex_);
        auto it = actors_.find(name);
        if (it != actors_.end()) {
            it->second->stop();
            actors_.erase(it);
            std::cout << "ActorSystem: Unregistered actor '" << name << "'\n";
        }
    }

    void ActorSystem::send_message(const std::string& actor_name, const Message& message) {
        std::lock_guard<std::mutex> lock(actors_mutex_);
        auto it = actors_.find(actor_name);
        if (it != actors_.end()) {
            it->second->send_message(message);
        } else {
            std::cout << "ActorSystem: Actor '" << actor_name << "' not found\n";
        }
    }

    void ActorSystem::shutdown() {
        std::lock_guard<std::mutex> lock(actors_mutex_);
        for (auto& [name, actor] : actors_) {
            actor->stop();
        }
        actors_.clear();
        std::cout << "ActorSystem: Shutdown complete\n";
    }

    size_t ActorSystem::actor_count() const {
        std::lock_guard<std::mutex> lock(actors_mutex_);
        return actors_.size();
    }

    std::vector<std::string> ActorSystem::get_actor_names() const {
        std::lock_guard<std::mutex> lock(actors_mutex_);
        std::vector<std::string> names;
        for (const auto& [name, actor] : actors_) {
            names.push_back(name);
        }
        return names;
    }

    // RequestResponseSystem Implementation
    void RequestResponseSystem::register_handler(const std::string& request_type, RequestHandler handler) {
        std::lock_guard<std::mutex> lock(handlers_mutex_);
        handlers_[request_type] = std::move(handler);
        std::cout << "ReqResp: Registered handler for '" << request_type << "'\n";
    }

    void RequestResponseSystem::unregister_handler(const std::string& request_type) {
        std::lock_guard<std::mutex> lock(handlers_mutex_);
        handlers_.erase(request_type);
        std::cout << "ReqResp: Unregistered handler for '" << request_type << "'\n";
    }

    std::future<Message> RequestResponseSystem::send_request(const Message& request) {
        std::string corr_id = generate_correlation_id();
        Message req_copy = request;
        req_copy.set_correlation_id(corr_id);

        std::promise<Message> promise;
        auto future = promise.get_future();

        {
            std::lock_guard<std::mutex> lock(pending_mutex_);
            pending_requests_[corr_id] = std::move(promise);
        }

        request_queue_.send(req_copy);
        return future;
    }

    void RequestResponseSystem::send_request_async(const Message& request, ResponseCallback callback) {
        std::string corr_id = generate_correlation_id();
        Message req_copy = request;
        req_copy.set_correlation_id(corr_id);

        {
            std::lock_guard<std::mutex> lock(pending_mutex_);
            pending_callbacks_[corr_id] = std::move(callback);
        }

        request_queue_.send(req_copy);
    }

    void RequestResponseSystem::start_processing() {
        if (!running_.exchange(true)) {
            processing_thread_ = std::thread(&RequestResponseSystem::process_requests, this);
            std::cout << "ReqResp: Started processing\n";
        }
    }

    void RequestResponseSystem::shutdown() {
        if (running_.exchange(false)) {
            request_queue_.close();
            if (processing_thread_.joinable()) {
                processing_thread_.join();
            }
            std::cout << "ReqResp: Shutdown complete\n";
        }
    }

    void RequestResponseSystem::process_requests() {
        while (running_.load() || request_queue_.size() > 0) {
            auto request = request_queue_.receive(std::chrono::milliseconds(100));
            if (!request.has_value()) continue;

            const Message& req = request.value();
            Message response("response", std::string("No handler found"));

            {
                std::lock_guard<std::mutex> lock(handlers_mutex_);
                auto it = handlers_.find(req.type);
                if (it != handlers_.end()) {
                    try {
                        response = it->second(req);
                    } catch (const std::exception& e) {
                        response = Message("error", std::string(e.what()));
                    }
                }
            }

            if (req.has_correlation_id()) {
                response.set_correlation_id(req.correlation_id.value());

                std::lock_guard<std::mutex> lock(pending_mutex_);
                
                // Check for promise-based request
                auto promise_it = pending_requests_.find(req.correlation_id.value());
                if (promise_it != pending_requests_.end()) {
                    promise_it->second.set_value(response);
                    pending_requests_.erase(promise_it);
                    continue;
                }

                // Check for callback-based request
                auto callback_it = pending_callbacks_.find(req.correlation_id.value());
                if (callback_it != pending_callbacks_.end()) {
                    callback_it->second(response);
                    pending_callbacks_.erase(callback_it);
                }
            }
        }
    }

    std::string RequestResponseSystem::generate_correlation_id() {
        static std::atomic<size_t> counter{0};
        return "req_" + std::to_string(counter.fetch_add(1)) + "_" + 
               std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());
    }

    // AsyncCommDemo Implementation
    void AsyncCommDemo::demonstrate_message_queue() {
        std::cout << "\n=== Message Queue Demonstration ===\n";
        
        MessageQueue<std::string> queue(5);
        
        // Producer thread
        std::thread producer([&queue]() {
            for (int i = 0; i < 10; ++i) {
                std::string message = "Message " + std::to_string(i);
                bool sent = queue.send(message, std::chrono::milliseconds(100));
                std::cout << "Producer: " << (sent ? "Sent" : "Failed to send") 
                          << " '" << message << "'\n";
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
            }
            std::cout << "Producer: Finished sending\n";
        });
        
        // Consumer thread
        std::thread consumer([&queue]() {
            for (int i = 0; i < 10; ++i) {
                auto message = queue.receive(std::chrono::milliseconds(200));
                if (message.has_value()) {
                    std::cout << "Consumer: Received '" << message.value() << "'\n";
                } else {
                    std::cout << "Consumer: Timeout waiting for message\n";
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(80));
            }
            std::cout << "Consumer: Finished receiving\n";
        });
        
        producer.join();
        consumer.join();
        
        std::cout << "Queue size after demo: " << queue.size() << "\n";
    }

    void AsyncCommDemo::demonstrate_pubsub_system() {
        std::cout << "\n=== Publish-Subscribe System Demonstration ===\n";
        
        PubSubSystem pubsub;
        pubsub.start_processing();
        
        // Subscribe to different topics
        auto sub1 = pubsub.subscribe("space.missions", [](const Message& msg) {
            std::cout << "Mission Subscriber: " << msg.get_payload<std::string>() << "\n";
        });
        
        auto sub2 = pubsub.subscribe("space.alerts", [](const Message& msg) {
            std::cout << "Alert Subscriber: " << msg.get_payload<std::string>() << "\n";
        });
        
        auto sub3 = pubsub.subscribe("space.missions", [](const Message& msg) {
            std::cout << "Mission Monitor: " << msg.get_payload<std::string>() << "\n";
        });
        
        // Publish messages
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        pubsub.publish("space.missions", Message("mission.start", std::string("Mars exploration mission initiated")));
        pubsub.publish("space.alerts", Message("alert.critical", std::string("Asteroid detected on collision course")));
        pubsub.publish("space.missions", Message("mission.update", std::string("Rover deployed successfully")));
        pubsub.publish("space.unknown", Message("test", std::string("This should have no subscribers")));
        
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        
        std::cout << "Subscribers for 'space.missions': " << pubsub.subscriber_count("space.missions") << "\n";
        std::cout << "Subscribers for 'space.alerts': " << pubsub.subscriber_count("space.alerts") << "\n";
        
        // Unsubscribe and test
        pubsub.unsubscribe("space.missions", sub3);
        pubsub.publish("space.missions", Message("mission.final", std::string("Mission completed")));
        
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        pubsub.shutdown();
    }

    void AsyncCommDemo::demonstrate_async_channel() {
        std::cout << "\n=== Async Channel Demonstration ===\n";
        
        AsyncChannel<std::string> channel(3);
        
        // Async sending
        auto send_future1 = channel.async_send("Command: Initialize systems");
        auto send_future2 = channel.async_send("Command: Deploy solar panels");
        auto send_future3 = channel.async_send("Command: Begin data collection");
        
        // Async receiving
        auto recv_future1 = channel.async_receive();
        auto recv_future2 = channel.async_receive();
        
        // Check send results
        std::cout << "Send 1 result: " << (send_future1.get() ? "Success" : "Failed") << "\n";
        std::cout << "Send 2 result: " << (send_future2.get() ? "Success" : "Failed") << "\n";
        std::cout << "Send 3 result: " << (send_future3.get() ? "Success" : "Failed") << "\n";
        
        // Get received messages
        auto msg1 = recv_future1.get();
        auto msg2 = recv_future2.get();
        
        if (msg1.has_value()) std::cout << "Received: " << msg1.value() << "\n";
        if (msg2.has_value()) std::cout << "Received: " << msg2.value() << "\n";
        
        // Synchronous operations
        bool sent = channel.send("Sync message", std::chrono::milliseconds(50));
        std::cout << "Sync send result: " << (sent ? "Success" : "Failed") << "\n";
        
        auto sync_msg = channel.receive(std::chrono::milliseconds(50));
        if (sync_msg.has_value()) {
            std::cout << "Sync received: " << sync_msg.value() << "\n";
        }
    }

    void AsyncCommDemo::demonstrate_actor_system() {
        std::cout << "\n=== Actor System Demonstration ===\n";
        
        // Define space-themed actors
        class SpaceStationActor : public ActorSystem::Actor {
        public:
            SpaceStationActor(const std::string& name, ActorSystem* system) 
                : Actor(name, system) {}
                
        protected:
            void handle_message(const Message& message) override {
                std::cout << "SpaceStation[" << name() << "] received: " << message.type;
                if (message.type == "supply.request") {
                    std::cout << " - Processing supply request\n";
                    simulate_network_delay();
                    send_to_actor("mission_control", Message("supply.response", 
                        std::string("Supplies dispatched from " + name())));
                } else if (message.type == "status.query") {
                    std::cout << " - Reporting operational status\n";
                    send_to_actor(message.sender_id, Message("status.report", 
                        std::string("Station " + name() + " operational")));
                }
                std::cout << "\n";
            }
            
            void on_start() override {
                std::cout << "SpaceStation[" << name() << "] coming online\n";
            }
        };
        
        class MissionControlActor : public ActorSystem::Actor {
        public:
            MissionControlActor(const std::string& name, ActorSystem* system) 
                : Actor(name, system) {}
                
        protected:
            void handle_message(const Message& message) override {
                std::cout << "MissionControl[" << name() << "] received: " << message.type;
                if (message.type == "supply.response") {
                    std::cout << " - " << message.get_payload<std::string>() << "\n";
                } else if (message.type == "status.report") {
                    std::cout << " - " << message.get_payload<std::string>() << "\n";
                }
            }
        };
        
        ActorSystem actor_system;
        
        // Create and register actors
        auto station1 = std::make_shared<SpaceStationActor>("alpha_station", &actor_system);
        auto station2 = std::make_shared<SpaceStationActor>("beta_station", &actor_system);
        auto mission_control = std::make_shared<MissionControlActor>("mission_control", &actor_system);
        
        actor_system.register_actor(station1);
        actor_system.register_actor(station2);
        actor_system.register_actor(mission_control);
        
        // Start actors
        station1->start();
        station2->start();
        mission_control->start();
        
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        // Send messages between actors
        actor_system.send_message("alpha_station", Message("supply.request", std::string("Need water and oxygen")));
        actor_system.send_message("beta_station", Message("status.query", std::string(""), "mission_control"));
        
        std::this_thread::sleep_for(std::chrono::milliseconds(300));
        
        std::cout << "Actor system has " << actor_system.actor_count() << " active actors\n";
        actor_system.shutdown();
    }

    void AsyncCommDemo::demonstrate_request_response() {
        std::cout << "\n=== Request-Response System Demonstration ===\n";
        
        RequestResponseSystem req_resp;
        req_resp.start_processing();
        
        // Register handlers
        req_resp.register_handler("calculate.distance", [](const Message& request) {
            auto data = request.get_payload<std::string>();
            simulate_network_delay();
            return Message("calculation.result", std::string("Distance calculated: 42.7 AU for " + data));
        });
        
        req_resp.register_handler("system.status", [](const Message& request) {
            return Message("status.response", std::string("All systems nominal"));
        });
        
        // Synchronous request
        std::cout << "Sending synchronous request...\n";
        auto future = req_resp.send_request(Message("calculate.distance", std::string("Earth to Mars")));
        auto response = future.get();
        std::cout << "Sync response: " << response.get_payload<std::string>() << "\n";
        
        // Asynchronous request
        std::cout << "Sending asynchronous request...\n";
        req_resp.send_request_async(Message("system.status", std::string("")), 
            [](const Message& response) {
                std::cout << "Async response: " << response.get_payload<std::string>() << "\n";
            });
        
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        req_resp.shutdown();
    }

    void AsyncCommDemo::demonstrate_space_communication_network() {
        std::cout << "\n=== Space Communication Network Simulation ===\n";
        
        PubSubSystem network;
        network.start_processing();
        
        // Create communication channels for different spacecraft
        std::vector<std::string> spacecraft = {"ISS", "Hubble", "Voyager1", "Perseverance"};
        std::atomic<int> message_count{0};
        
        // Subscribe each spacecraft to network channels
        for (const auto& craft : spacecraft) {
            network.subscribe("network.broadcast", [craft, &message_count](const Message& msg) {
                message_count.fetch_add(1);
                std::cout << craft << " received broadcast: " << msg.get_payload<std::string>() << "\n";
            });
            
            network.subscribe("telemetry." + craft, [craft](const Message& msg) {
                std::cout << craft << " telemetry update: " << msg.get_payload<std::string>() << "\n";
            });
        }
        
        // Simulate mission control sending broadcasts
        std::thread mission_control([&network]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            network.publish("network.broadcast", 
                Message("broadcast", std::string("Solar storm warning - all craft take precautions")));
            
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            network.publish("network.broadcast", 
                Message("broadcast", std::string("Communication window opening in 5 minutes")));
        });
        
        // Simulate spacecraft sending telemetry
        std::thread telemetry_sender([&network]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(150));
            network.publish("telemetry.ISS", 
                Message("telemetry", std::string("Altitude: 408km, Speed: 7.66km/s")));
            
            network.publish("telemetry.Perseverance", 
                Message("telemetry", std::string("Sol 1000: Collecting rock samples at Jezero Crater")));
            
            network.publish("telemetry.Voyager1", 
                Message("telemetry", std::string("Distance: 23.5 billion km from Earth")));
        });
        
        mission_control.join();
        telemetry_sender.join();
        
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        
        std::cout << "Total broadcast messages received: " << message_count.load() << "\n";
        std::cout << "Network topics: ";
        for (const auto& topic : network.get_topics()) {
            std::cout << topic << " ";
        }
        std::cout << "\n";
        
        network.shutdown();
    }

    void AsyncCommDemo::run_all_demonstrations() {
        std::cout << "\n========== ASYNC COMMUNICATION COMPREHENSIVE DEMONSTRATION ==========\n";
        
        demonstrate_message_queue();
        demonstrate_pubsub_system();
        demonstrate_async_channel();
        demonstrate_actor_system();
        demonstrate_request_response();
        demonstrate_space_communication_network();
        
        std::cout << "\n========== ASYNC COMMUNICATION DEMONSTRATION COMPLETE ==========\n";
    }

    void AsyncCommDemo::simulate_network_delay() {
        // Simulate variable network delay between 10-100ms
        static std::random_device rd;
        static std::mt19937 gen(rd());
        static std::uniform_int_distribution<> delay_dist(10, 100);
        
        std::this_thread::sleep_for(std::chrono::milliseconds(delay_dist(gen)));
    }

} // namespace CppVerseHub::Concurrency