// File: src/patterns/Observer.hpp
// CppVerseHub - Observer Pattern Implementation for Planet Monitoring System

#pragma once

#include <vector>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <functional>
#include <chrono>
#include <mutex>
#include <algorithm>
#include <any>

namespace CppVerseHub::Patterns {

/**
 * @brief Event data structure for observer notifications
 */
struct EventData {
    std::string event_type;
    std::string source_id;
    std::chrono::system_clock::time_point timestamp;
    std::unordered_map<std::string, std::any> properties;
    
    EventData(const std::string& type, const std::string& source)
        : event_type(type), source_id(source), timestamp(std::chrono::system_clock::now()) {}
    
    /**
     * @brief Set a property value
     * @tparam T Type of the property value
     * @param key Property key
     * @param value Property value
     */
    template<typename T>
    void setProperty(const std::string& key, const T& value) {
        properties[key] = value;
    }
    
    /**
     * @brief Get a property value
     * @tparam T Expected type of the property value
     * @param key Property key
     * @return Optional property value
     */
    template<typename T>
    std::optional<T> getProperty(const std::string& key) const {
        auto it = properties.find(key);
        if (it != properties.end()) {
            try {
                return std::any_cast<T>(it->second);
            } catch (const std::bad_any_cast&) {
                return std::nullopt;
            }
        }
        return std::nullopt;
    }
    
    /**
     * @brief Check if property exists
     * @param key Property key
     * @return true if property exists
     */
    bool hasProperty(const std::string& key) const {
        return properties.find(key) != properties.end();
    }
};

/**
 * @brief Forward declarations
 */
class IObserver;
class ISubject;

/**
 * @brief Observer interface for receiving notifications
 * 
 * Objects implementing this interface can register with subjects
 * to receive notifications when events occur.
 */
class IObserver {
public:
    virtual ~IObserver() = default;
    
    /**
     * @brief Called when an observed event occurs
     * @param event_data Event information
     * @param subject Pointer to the subject that triggered the event
     */
    virtual void onNotify(const EventData& event_data, ISubject* subject) = 0;
    
    /**
     * @brief Get observer identifier
     * @return Unique identifier for this observer
     */
    virtual std::string getObserverId() const = 0;
    
    /**
     * @brief Get observer priority (higher values = higher priority)
     * @return Priority value (default: 0)
     */
    virtual int getPriority() const { return 0; }
    
    /**
     * @brief Check if observer is interested in specific event types
     * @param event_type Event type to check
     * @return true if interested, false otherwise
     */
    virtual bool isInterestedInEvent(const std::string& event_type) const { 
        return true; // By default, interested in all events
    }
};

/**
 * @brief Subject interface for objects that can be observed
 * 
 * Objects implementing this interface can have observers attached
 * and will notify them when events occur.
 */
class ISubject {
public:
    virtual ~ISubject() = default;
    
    /**
     * @brief Attach an observer
     * @param observer Observer to attach
     * @return true if successfully attached, false otherwise
     */
    virtual bool attachObserver(std::shared_ptr<IObserver> observer) = 0;
    
    /**
     * @brief Detach an observer
     * @param observer Observer to detach
     * @return true if successfully detached, false otherwise
     */
    virtual bool detachObserver(std::shared_ptr<IObserver> observer) = 0;
    
    /**
     * @brief Detach observer by ID
     * @param observer_id ID of observer to detach
     * @return true if successfully detached, false otherwise
     */
    virtual bool detachObserver(const std::string& observer_id) = 0;
    
    /**
     * @brief Notify all attached observers
     * @param event_data Event data to send to observers
     */
    virtual void notifyObservers(const EventData& event_data) = 0;
    
    /**
     * @brief Get number of attached observers
     * @return Number of observers
     */
    virtual size_t getObserverCount() const = 0;
    
    /**
     * @brief Get subject identifier
     * @return Unique identifier for this subject
     */
    virtual std::string getSubjectId() const = 0;
};

/**
 * @brief Concrete implementation of Subject interface
 * 
 * Thread-safe implementation that manages a collection of observers
 * and provides notification functionality with priority ordering.
 */
class Subject : public ISubject {
public:
    explicit Subject(const std::string& subject_id) : subject_id_(subject_id) {}
    
    virtual ~Subject() = default;
    
    bool attachObserver(std::shared_ptr<IObserver> observer) override {
        if (!observer) return false;
        
        std::lock_guard<std::mutex> lock(observers_mutex_);
        
        // Check if already attached
        auto it = std::find_if(observers_.begin(), observers_.end(),
            [&observer](const std::weak_ptr<IObserver>& weak_obs) {
                auto obs = weak_obs.lock();
                return obs && obs->getObserverId() == observer->getObserverId();
            });
        
        if (it != observers_.end()) {
            return false; // Already attached
        }
        
        observers_.push_back(observer);
        sortObserversByPriority();
        return true;
    }
    
    bool detachObserver(std::shared_ptr<IObserver> observer) override {
        if (!observer) return false;
        
        std::lock_guard<std::mutex> lock(observers_mutex_);
        
        auto it = std::remove_if(observers_.begin(), observers_.end(),
            [&observer](const std::weak_ptr<IObserver>& weak_obs) {
                auto obs = weak_obs.lock();
                return !obs || obs->getObserverId() == observer->getObserverId();
            });
        
        bool removed = it != observers_.end();
        observers_.erase(it, observers_.end());
        return removed;
    }
    
    bool detachObserver(const std::string& observer_id) override {
        std::lock_guard<std::mutex> lock(observers_mutex_);
        
        auto it = std::remove_if(observers_.begin(), observers_.end(),
            [&observer_id](const std::weak_ptr<IObserver>& weak_obs) {
                auto obs = weak_obs.lock();
                return !obs || obs->getObserverId() == observer_id;
            });
        
        bool removed = it != observers_.end();
        observers_.erase(it, observers_.end());
        return removed;
    }
    
    void notifyObservers(const EventData& event_data) override {
        std::vector<std::shared_ptr<IObserver>> current_observers;
        
        // Copy observers to avoid holding lock during notifications
        {
            std::lock_guard<std::mutex> lock(observers_mutex_);
            current_observers.reserve(observers_.size());
            
            // Clean up expired observers and collect valid ones
            auto it = std::remove_if(observers_.begin(), observers_.end(),
                [&current_observers, &event_data](const std::weak_ptr<IObserver>& weak_obs) {
                    auto obs = weak_obs.lock();
                    if (obs && obs->isInterestedInEvent(event_data.event_type)) {
                        current_observers.push_back(obs);
                        return false; // Keep in observers_ list
                    }
                    return !obs; // Remove expired observers
                });
            observers_.erase(it, observers_.end());
        }
        
        // Notify observers (outside of lock to prevent deadlocks)
        for (auto& observer : current_observers) {
            try {
                observer->onNotify(event_data, this);
            } catch (...) {
                // Log error but continue notifying other observers
                // In a real implementation, you might want to log this
            }
        }
    }
    
    size_t getObserverCount() const override {
        std::lock_guard<std::mutex> lock(observers_mutex_);
        return observers_.size();
    }
    
    std::string getSubjectId() const override {
        return subject_id_;
    }

protected:
    /**
     * @brief Sort observers by priority (higher priority first)
     */
    void sortObserversByPriority() {
        std::sort(observers_.begin(), observers_.end(),
            [](const std::weak_ptr<IObserver>& a, const std::weak_ptr<IObserver>& b) {
                auto obs_a = a.lock();
                auto obs_b = b.lock();
                if (!obs_a) return false;
                if (!obs_b) return true;
                return obs_a->getPriority() > obs_b->getPriority();
            });
    }

private:
    std::string subject_id_;
    mutable std::mutex observers_mutex_;
    std::vector<std::weak_ptr<IObserver>> observers_;
};

/**
 * @brief Planet monitoring data structures
 */
struct ResourceLevel {
    std::string resource_type;
    double current_amount;
    double maximum_capacity;
    double extraction_rate;
    
    double getUtilizationPercentage() const {
        return maximum_capacity > 0.0 ? (current_amount / maximum_capacity) * 100.0 : 0.0;
    }
    
    bool isLow(double threshold = 20.0) const {
        return getUtilizationPercentage() < threshold;
    }
    
    bool isCritical(double threshold = 5.0) const {
        return getUtilizationPercentage() < threshold;
    }
};

struct PlanetStatus {
    std::string planet_id;
    std::string planet_name;
    double population;
    double defense_strength;
    std::vector<ResourceLevel> resources;
    std::string status; // "stable", "under_attack", "resource_depleted", etc.
    std::chrono::system_clock::time_point last_update;
    
    PlanetStatus(const std::string& id, const std::string& name)
        : planet_id(id), planet_name(name), population(0.0), defense_strength(0.0),
          status("stable"), last_update(std::chrono::system_clock::now()) {}
};

/**
 * @brief Observable Planet class
 * 
 * Represents a planet that can be monitored for various events
 * such as resource changes, population changes, and attacks.
 */
class ObservablePlanet : public Subject {
public:
    explicit ObservablePlanet(const std::string& planet_id, const std::string& planet_name)
        : Subject(planet_id), status_(planet_id, planet_name) {}
    
    /**
     * @brief Get current planet status
     * @return Current planet status
     */
    const PlanetStatus& getStatus() const {
        std::lock_guard<std::mutex> lock(status_mutex_);
        return status_;
    }
    
    /**
     * @brief Update population
     * @param new_population New population value
     */
    void updatePopulation(double new_population) {
        double old_population;
        {
            std::lock_guard<std::mutex> lock(status_mutex_);
            old_population = status_.population;
            status_.population = new_population;
            status_.last_update = std::chrono::system_clock::now();
        }
        
        if (std::abs(new_population - old_population) > 0.01) {
            EventData event("population_changed", getSubjectId());
            event.setProperty("old_population", old_population);
            event.setProperty("new_population", new_population);
            event.setProperty("planet_name", status_.planet_name);
            notifyObservers(event);
        }
    }
    
    /**
     * @brief Update defense strength
     * @param new_defense Defense strength value
     */
    void updateDefenseStrength(double new_defense) {
        double old_defense;
        {
            std::lock_guard<std::mutex> lock(status_mutex_);
            old_defense = status_.defense_strength;
            status_.defense_strength = new_defense;
            status_.last_update = std::chrono::system_clock::now();
        }
        
        EventData event("defense_changed", getSubjectId());
        event.setProperty("old_defense", old_defense);
        event.setProperty("new_defense", new_defense);
        event.setProperty("planet_name", status_.planet_name);
        notifyObservers(event);
    }
    
    /**
     * @brief Update resource level
     * @param resource_type Type of resource
     * @param new_amount New resource amount
     * @param extraction_rate Extraction rate (optional)
     */
    void updateResource(const std::string& resource_type, double new_amount, double extraction_rate = 0.0) {
        double old_amount = 0.0;
        bool resource_exists = false;
        
        {
            std::lock_guard<std::mutex> lock(status_mutex_);
            auto it = std::find_if(status_.resources.begin(), status_.resources.end(),
                [&resource_type](const ResourceLevel& res) {
                    return res.resource_type == resource_type;
                });
            
            if (it != status_.resources.end()) {
                old_amount = it->current_amount;
                it->current_amount = new_amount;
                if (extraction_rate > 0.0) {
                    it->extraction_rate = extraction_rate;
                }
                resource_exists = true;
            } else {
                // Add new resource
                ResourceLevel new_resource;
                new_resource.resource_type = resource_type;
                new_resource.current_amount = new_amount;
                new_resource.maximum_capacity = new_amount * 2.0; // Default capacity
                new_resource.extraction_rate = extraction_rate;
                status_.resources.push_back(new_resource);
            }
            status_.last_update = std::chrono::system_clock::now();
        }
        
        EventData event("resource_changed", getSubjectId());
        event.setProperty("resource_type", resource_type);
        event.setProperty("old_amount", old_amount);
        event.setProperty("new_amount", new_amount);
        event.setProperty("planet_name", status_.planet_name);
        
        // Check for critical resource levels
        if (resource_exists) {
            auto resource_level = getResourceLevel(resource_type);
            if (resource_level && resource_level->isCritical()) {
                event.setProperty("is_critical", true);
            } else if (resource_level && resource_level->isLow()) {
                event.setProperty("is_low", true);
            }
        }
        
        notifyObservers(event);
    }
    
    /**
     * @brief Simulate an attack on the planet
     * @param attacker_strength Strength of the attacking force
     * @param attack_type Type of attack (e.g., "orbital_bombardment", "ground_assault")
     */
    void simulateAttack(double attacker_strength, const std::string& attack_type = "unknown") {
        {
            std::lock_guard<std::mutex> lock(status_mutex_);
            status_.status = "under_attack";
            status_.last_update = std::chrono::system_clock::now();
        }
        
        EventData event("planet_under_attack", getSubjectId());
        event.setProperty("attacker_strength", attacker_strength);
        event.setProperty("defense_strength", status_.defense_strength);
        event.setProperty("attack_type", attack_type);
        event.setProperty("planet_name", status_.planet_name);
        event.setProperty("is_critical", attacker_strength > status_.defense_strength);
        notifyObservers(event);
        
        // Simulate damage to population and resources
        if (attacker_strength > status_.defense_strength) {
            double damage_factor = (attacker_strength - status_.defense_strength) / attacker_strength;
            updatePopulation(status_.population * (1.0 - damage_factor * 0.1)); // 10% max population loss
            
            // Damage random resources
            for (auto& resource : status_.resources) {
                if (resource.current_amount > 0) {
                    double resource_damage = resource.current_amount * damage_factor * 0.05; // 5% max resource loss
                    updateResource(resource.resource_type, resource.current_amount - resource_damage);
                }
            }
        }
        
        // After attack, set status back to stable (simplified)
        {
            std::lock_guard<std::mutex> lock(status_mutex_);
            status_.status = "stable";
        }
    }
    
    /**
     * @brief Get resource level information
     * @param resource_type Type of resource
     * @return Pointer to resource level or nullptr if not found
     */
    const ResourceLevel* getResourceLevel(const std::string& resource_type) const {
        std::lock_guard<std::mutex> lock(status_mutex_);
        auto it = std::find_if(status_.resources.begin(), status_.resources.end(),
            [&resource_type](const ResourceLevel& res) {
                return res.resource_type == resource_type;
            });
        return (it != status_.resources.end()) ? &(*it) : nullptr;
    }
    
    /**
     * @brief Add a new resource type to the planet
     * @param resource_type Type of resource
     * @param initial_amount Initial amount
     * @param max_capacity Maximum capacity
     * @param extraction_rate Extraction rate
     */
    void addResource(const std::string& resource_type, double initial_amount, 
                    double max_capacity, double extraction_rate = 0.0) {
        std::lock_guard<std::mutex> lock(status_mutex_);
        
        // Check if resource already exists
        auto it = std::find_if(status_.resources.begin(), status_.resources.end(),
            [&resource_type](const ResourceLevel& res) {
                return res.resource_type == resource_type;
            });
        
        if (it == status_.resources.end()) {
            ResourceLevel new_resource;
            new_resource.resource_type = resource_type;
            new_resource.current_amount = initial_amount;
            new_resource.maximum_capacity = max_capacity;
            new_resource.extraction_rate = extraction_rate;
            status_.resources.push_back(new_resource);
            
            status_.last_update = std::chrono::system_clock::now();
        }
    }

private:
    mutable std::mutex status_mutex_;
    PlanetStatus status_;
};

/**
 * @brief Resource Monitor Observer
 * 
 * Monitors resource levels across planets and triggers alerts
 * when resources become critically low.
 */
class ResourceMonitor : public IObserver {
public:
    explicit ResourceMonitor(const std::string& monitor_id, double critical_threshold = 5.0, double low_threshold = 20.0)
        : monitor_id_(monitor_id), critical_threshold_(critical_threshold), low_threshold_(low_threshold) {}
    
    void onNotify(const EventData& event_data, ISubject* subject) override {
        if (event_data.event_type == "resource_changed") {
            handleResourceChange(event_data, subject);
        }
    }
    
    std::string getObserverId() const override {
        return monitor_id_;
    }
    
    int getPriority() const override {
        return 5; // High priority for resource monitoring
    }
    
    bool isInterestedInEvent(const std::string& event_type) const override {
        return event_type == "resource_changed";
    }
    
    /**
     * @brief Get alert history
     * @return Vector of alert messages
     */
    const std::vector<std::string>& getAlertHistory() const {
        std::lock_guard<std::mutex> lock(alerts_mutex_);
        return alert_history_;
    }
    
    /**
     * @brief Clear alert history
     */
    void clearAlertHistory() {
        std::lock_guard<std::mutex> lock(alerts_mutex_);
        alert_history_.clear();
    }

private:
    void handleResourceChange(const EventData& event_data, ISubject* subject) {
        auto resource_type = event_data.getProperty<std::string>("resource_type");
        auto new_amount = event_data.getProperty<double>("new_amount");
        auto planet_name = event_data.getProperty<std::string>("planet_name");
        auto is_critical = event_data.getProperty<bool>("is_critical");
        auto is_low = event_data.getProperty<bool>("is_low");
        
        if (!resource_type || !new_amount || !planet_name) return;
        
        std::string alert_message;
        
        if (is_critical && is_critical.value()) {
            alert_message = "CRITICAL: Planet " + planet_name.value() + 
                          " has critically low " + resource_type.value() + 
                          " (" + std::to_string(new_amount.value()) + " units remaining)";
        } else if (is_low && is_low.value()) {
            alert_message = "WARNING: Planet " + planet_name.value() + 
                          " has low " + resource_type.value() + 
                          " (" + std::to_string(new_amount.value()) + " units remaining)";
        }
        
        if (!alert_message.empty()) {
            std::lock_guard<std::mutex> lock(alerts_mutex_);
            alert_history_.push_back(alert_message);
            
            // Limit history size
            if (alert_history_.size() > 100) {
                alert_history_.erase(alert_history_.begin());
            }
        }
    }
    
    std::string monitor_id_;
    double critical_threshold_;
    double low_threshold_;
    mutable std::mutex alerts_mutex_;
    std::vector<std::string> alert_history_;
};

/**
 * @brief Defense Monitor Observer
 * 
 * Monitors planetary defense systems and attack events.
 */
class DefenseMonitor : public IObserver {
public:
    explicit DefenseMonitor(const std::string& monitor_id)
        : monitor_id_(monitor_id) {}
    
    void onNotify(const EventData& event_data, ISubject* subject) override {
        if (event_data.event_type == "planet_under_attack") {
            handleAttackEvent(event_data, subject);
        } else if (event_data.event_type == "defense_changed") {
            handleDefenseChange(event_data, subject);
        }
    }
    
    std::string getObserverId() const override {
        return monitor_id_;
    }
    
    int getPriority() const override {
        return 10; // Highest priority for defense monitoring
    }
    
    bool isInterestedInEvent(const std::string& event_type) const override {
        return event_type == "planet_under_attack" || event_type == "defense_changed";
    }
    
    /**
     * @brief Get attack history
     * @return Vector of attack records
     */
    const std::vector<std::string>& getAttackHistory() const {
        std::lock_guard<std::mutex> lock(attacks_mutex_);
        return attack_history_;
    }

private:
    void handleAttackEvent(const EventData& event_data, ISubject* subject) {
        auto planet_name = event_data.getProperty<std::string>("planet_name");
        auto attacker_strength = event_data.getProperty<double>("attacker_strength");
        auto defense_strength = event_data.getProperty<double>("defense_strength");
        auto attack_type = event_data.getProperty<std::string>("attack_type");
        auto is_critical = event_data.getProperty<bool>("is_critical");
        
        if (!planet_name || !attacker_strength || !defense_strength) return;
        
        std::string attack_record = "ATTACK on " + planet_name.value() + 
                                   ": Attacker strength " + std::to_string(attacker_strength.value()) +
                                   " vs Defense " + std::to_string(defense_strength.value());
        
        if (attack_type) {
            attack_record += " (Type: " + attack_type.value() + ")";
        }
        
        if (is_critical && is_critical.value()) {
            attack_record += " - DEFENSE BREACHED!";
        } else {
            attack_record += " - Attack repelled";
        }
        
        std::lock_guard<std::mutex> lock(attacks_mutex_);
        attack_history_.push_back(attack_record);
        
        // Limit history size
        if (attack_history_.size() > 50) {
            attack_history_.erase(attack_history_.begin());
        }
    }
    
    void handleDefenseChange(const EventData& event_data, ISubject* subject) {
        auto planet_name = event_data.getProperty<std::string>("planet_name");
        auto old_defense = event_data.getProperty<double>("old_defense");
        auto new_defense = event_data.getProperty<double>("new_defense");
        
        if (!planet_name || !old_defense || !new_defense) return;
        
        if (std::abs(new_defense.value() - old_defense.value()) > 0.01) {
            std::string change_type = (new_defense.value() > old_defense.value()) ? "UPGRADED" : "DOWNGRADED";
            std::string defense_record = "DEFENSE " + change_type + " on " + planet_name.value() +
                                       ": " + std::to_string(old_defense.value()) + 
                                       " -> " + std::to_string(new_defense.value());
            
            std::lock_guard<std::mutex> lock(attacks_mutex_);
            attack_history_.push_back(defense_record);
        }
    }
    
    std::string monitor_id_;
    mutable std::mutex attacks_mutex_;
    std::vector<std::string> attack_history_;
};

/**
 * @brief Population Monitor Observer
 * 
 * Monitors population changes across planets.
 */
class PopulationMonitor : public IObserver {
public:
    explicit PopulationMonitor(const std::string& monitor_id, double growth_threshold = 5.0)
        : monitor_id_(monitor_id), growth_threshold_(growth_threshold) {}
    
    void onNotify(const EventData& event_data, ISubject* subject) override {
        if (event_data.event_type == "population_changed") {
            handlePopulationChange(event_data, subject);
        }
    }
    
    std::string getObserverId() const override {
        return monitor_id_;
    }
    
    int getPriority() const override {
        return 3; // Medium priority
    }
    
    bool isInterestedInEvent(const std::string& event_type) const override {
        return event_type == "population_changed";
    }
    
    /**
     * @brief Get population statistics
     * @return Map of planet names to population data
     */
    const std::unordered_map<std::string, std::pair<double, double>>& getPopulationStats() const {
        std::lock_guard<std::mutex> lock(stats_mutex_);
        return population_stats_; // pair: <current_population, growth_rate>
    }

private:
    void handlePopulationChange(const EventData& event_data, ISubject* subject) {
        auto planet_name = event_data.getProperty<std::string>("planet_name");
        auto old_population = event_data.getProperty<double>("old_population");
        auto new_population = event_data.getProperty<double>("new_population");
        
        if (!planet_name || !old_population || !new_population) return;
        
        double growth_rate = 0.0;
        if (old_population.value() > 0.0) {
            growth_rate = ((new_population.value() - old_population.value()) / old_population.value()) * 100.0;
        }
        
        {
            std::lock_guard<std::mutex> lock(stats_mutex_);
            population_stats_[planet_name.value()] = {new_population.value(), growth_rate};
        }
        
        // Log significant population changes
        if (std::abs(growth_rate) > growth_threshold_) {
            std::string change_type = (growth_rate > 0) ? "GROWTH" : "DECLINE";
            // In a real system, you might log this to a file or database
        }
    }
    
    std::string monitor_id_;
    double growth_threshold_;
    mutable std::mutex stats_mutex_;
    std::unordered_map<std::string, std::pair<double, double>> population_stats_;
};

/**
 * @brief Event Logger Observer
 * 
 * Logs all events to a centralized log for auditing and analysis.
 */
class EventLogger : public IObserver {
public:
    explicit EventLogger(const std::string& logger_id, const std::string& log_file_path = "")
        : logger_id_(logger_id), log_file_path_(log_file_path) {}
    
    void onNotify(const EventData& event_data, ISubject* subject) override {
        logEvent(event_data, subject);
    }
    
    std::string getObserverId() const override {
        return logger_id_;
    }
    
    int getPriority() const override {
        return 1; // Low priority - logging should happen last
    }
    
    bool isInterestedInEvent(const std::string& event_type) const override {
        return true; // Log all events
    }
    
    /**
     * @brief Get event log entries
     * @return Vector of log entries
     */
    const std::vector<std::string>& getEventLog() const {
        std::lock_guard<std::mutex> lock(log_mutex_);
        return event_log_;
    }
    
    /**
     * @brief Clear event log
     */
    void clearEventLog() {
        std::lock_guard<std::mutex> lock(log_mutex_);
        event_log_.clear();
    }

private:
    void logEvent(const EventData& event_data, ISubject* subject) {
        auto timestamp = std::chrono::system_clock::to_time_t(event_data.timestamp);
        
        std::ostringstream log_entry;
        log_entry << "[" << std::put_time(std::localtime(&timestamp), "%Y-%m-%d %H:%M:%S") << "] "
                 << "Event: " << event_data.event_type 
                 << " | Source: " << event_data.source_id;
        
        // Add relevant properties based on event type
        if (event_data.hasProperty("planet_name")) {
            auto planet_name = event_data.getProperty<std::string>("planet_name");
            if (planet_name) {
                log_entry << " | Planet: " << planet_name.value();
            }
        }
        
        std::lock_guard<std::mutex> lock(log_mutex_);
        event_log_.push_back(log_entry.str());
        
        // Limit log size
        if (event_log_.size() > 1000) {
            event_log_.erase(event_log_.begin());
        }
        
        // If log file is specified, write to file (simplified implementation)
        if (!log_file_path_.empty()) {
            // In a real implementation, you would write to file here
        }
    }
    
    std::string logger_id_;
    std::string log_file_path_;
    mutable std::mutex log_mutex_;
    std::vector<std::string> event_log_;
};

/**
 * @brief Observer Manager - Utility class for managing observers
 * 
 * Provides convenience methods for creating and managing different
 * types of observers across multiple subjects.
 */
class ObserverManager {
public:
    /**
     * @brief Create and attach a resource monitor to a subject
     * @param subject Subject to attach monitor to
     * @param monitor_id Unique ID for the monitor
     * @param critical_threshold Critical resource threshold
     * @param low_threshold Low resource threshold
     * @return Shared pointer to created monitor
     */
    std::shared_ptr<ResourceMonitor> createResourceMonitor(
        std::shared_ptr<ISubject> subject,
        const std::string& monitor_id,
        double critical_threshold = 5.0,
        double low_threshold = 20.0
    ) {
        auto monitor = std::make_shared<ResourceMonitor>(monitor_id, critical_threshold, low_threshold);
        if (subject) {
            subject->attachObserver(monitor);
            resource_monitors_[monitor_id] = monitor;
        }
        return monitor;
    }
    
    /**
     * @brief Create and attach a defense monitor to a subject
     */
    std::shared_ptr<DefenseMonitor> createDefenseMonitor(
        std::shared_ptr<ISubject> subject,
        const std::string& monitor_id
    ) {
        auto monitor = std::make_shared<DefenseMonitor>(monitor_id);
        if (subject) {
            subject->attachObserver(monitor);
            defense_monitors_[monitor_id] = monitor;
        }
        return monitor;
    }
    
    /**
     * @brief Create and attach a population monitor to a subject
     */
    std::shared_ptr<PopulationMonitor> createPopulationMonitor(
        std::shared_ptr<ISubject> subject,
        const std::string& monitor_id,
        double growth_threshold = 5.0
    ) {
        auto monitor = std::make_shared<PopulationMonitor>(monitor_id, growth_threshold);
        if (subject) {
            subject->attachObserver(monitor);
            population_monitors_[monitor_id] = monitor;
        }
        return monitor;
    }
    
    /**
     * @brief Create and attach an event logger to a subject
     */
    std::shared_ptr<EventLogger> createEventLogger(
        std::shared_ptr<ISubject> subject,
        const std::string& logger_id,
        const std::string& log_file_path = ""
    ) {
        auto logger = std::make_shared<EventLogger>(logger_id, log_file_path);
        if (subject) {
            subject->attachObserver(logger);
            event_loggers_[logger_id] = logger;
        }
        return logger;
    }
    
    /**
     * @brief Get resource monitor by ID
     */
    std::shared_ptr<ResourceMonitor> getResourceMonitor(const std::string& monitor_id) {
        auto it = resource_monitors_.find(monitor_id);
        return (it != resource_monitors_.end()) ? it->second.lock() : nullptr;
    }
    
    /**
     * @brief Get defense monitor by ID
     */
    std::shared_ptr<DefenseMonitor> getDefenseMonitor(const std::string& monitor_id) {
        auto it = defense_monitors_.find(monitor_id);
        return (it != defense_monitors_.end()) ? it->second.lock() : nullptr;
    }
    
    /**
     * @brief Get population monitor by ID
     */
    std::shared_ptr<PopulationMonitor> getPopulationMonitor(const std::string& monitor_id) {
        auto it = population_monitors_.find(monitor_id);
        return (it != population_monitors_.end()) ? it->second.lock() : nullptr;
    }
    
    /**
     * @brief Get event logger by ID
     */
    std::shared_ptr<EventLogger> getEventLogger(const std::string& logger_id) {
        auto it = event_loggers_.find(logger_id);
        return (it != event_loggers_.end()) ? it->second.lock() : nullptr;
    }
    
    /**
     * @brief Remove expired observers from internal tracking
     */
    void cleanup() {
        cleanupMap(resource_monitors_);
        cleanupMap(defense_monitors_);
        cleanupMap(population_monitors_);
        cleanupMap(event_loggers_);
    }

private:
    template<typename T>
    void cleanupMap(std::unordered_map<std::string, std::weak_ptr<T>>& monitor_map) {
        auto it = monitor_map.begin();
        while (it != monitor_map.end()) {
            if (it->second.expired()) {
                it = monitor_map.erase(it);
            } else {
                ++it;
            }
        }
    }
    
    std::unordered_map<std::string, std::weak_ptr<ResourceMonitor>> resource_monitors_;
    std::unordered_map<std::string, std::weak_ptr<DefenseMonitor>> defense_monitors_;
    std::unordered_map<std::string, std::weak_ptr<PopulationMonitor>> population_monitors_;
    std::unordered_map<std::string, std::weak_ptr<EventLogger>> event_loggers_;
};

} // namespace CppVerseHub::Patterns