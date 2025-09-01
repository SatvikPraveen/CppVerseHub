// File: examples/advanced_usage/plugin_systems.cpp
// CppVerseHub Plugin System Example
// Advanced demonstration of extensible plugin architecture for system customization

#include <iostream>
#include <memory>
#include <vector>
#include <map>
#include <string>
#include <functional>
#include <typeinfo>
#include <typeindex>
#include <any>
#include <algorithm>

// Core includes
#include "Entity.hpp"
#include "Planet.hpp"
#include "Fleet.hpp"
#include "Mission.hpp"
#include "Logger.hpp"
#include "Factory.hpp"

using namespace std;

/**
 * @brief Base Plugin Interface
 * All plugins must implement this interface
 */
class IPlugin {
public:
    virtual ~IPlugin() = default;
    virtual string getName() const = 0;
    virtual string getVersion() const = 0;
    virtual bool initialize() = 0;
    virtual void shutdown() = 0;
    virtual bool isEnabled() const = 0;
};

/**
 * @brief Plugin Manager - Central registry for all plugins
 */
class PluginManager {
private:
    map<string, unique_ptr<IPlugin>> plugins;
    map<type_index, vector<function<void(const any&)>>> eventHandlers;
    static unique_ptr<PluginManager> instance;
    
public:
    static PluginManager& getInstance() {
        if (!instance) {
            instance = make_unique<PluginManager>();
        }
        return *instance;
    }
    
    bool registerPlugin(unique_ptr<IPlugin> plugin) {
        string name = plugin->getName();
        
        if (plugins.find(name) != plugins.end()) {
            LOG_WARN("Plugin {} already registered", name);
            return false;
        }
        
        if (!plugin->initialize()) {
            LOG_ERROR("Failed to initialize plugin {}", name);
            return false;
        }
        
        plugins[name] = move(plugin);
        LOG_INFO("Successfully registered plugin: {} v{}", name, plugins[name]->getVersion());
        return true;
    }
    
    bool unregisterPlugin(const string& name) {
        auto it = plugins.find(name);
        if (it == plugins.end()) {
            LOG_WARN("Plugin {} not found for unregistration", name);
            return false;
        }
        
        it->second->shutdown();
        plugins.erase(it);
        LOG_INFO("Unregistered plugin: {}", name);
        return true;
    }
    
    IPlugin* getPlugin(const string& name) {
        auto it = plugins.find(name);
        return (it != plugins.end()) ? it->second.get() : nullptr;
    }
    
    vector<string> getPluginNames() const {
        vector<string> names;
        for (const auto& [name, plugin] : plugins) {
            names.push_back(name);
        }
        return names;
    }
    
    // Event system for plugin communication
    template<typename EventType>
    void subscribeToEvent(function<void(const EventType&)> handler) {
        auto wrapper = [handler](const any& event) {
            try {
                const EventType& typedEvent = any_cast<const EventType&>(event);
                handler(typedEvent);
            } catch (const bad_any_cast& e) {
                LOG_ERROR("Bad event cast: {}", e.what());
            }
        };
        
        eventHandlers[type_index(typeid(EventType))].push_back(wrapper);
        LOG_DEBUG("Subscribed to event type: {}", typeid(EventType).name());
    }
    
    template<typename EventType>
    void publishEvent(const EventType& event) {
        auto it = eventHandlers.find(type_index(typeid(EventType)));
        if (it != eventHandlers.end()) {
            for (const auto& handler : it->second) {
                handler(any(event));
            }
        }
    }
    
    void shutdown() {
        for (auto& [name, plugin] : plugins) {
            plugin->shutdown();
        }
        plugins.clear();
        eventHandlers.clear();
        LOG_INFO("Plugin manager shutdown complete");
    }
};

unique_ptr<PluginManager> PluginManager::instance = nullptr;

/**
 * @brief Event types for plugin communication
 */
struct EntityCreatedEvent {
    Entity* entity;
    string entityType;
    chrono::system_clock::time_point timestamp;
};

struct MissionCompletedEvent {
    string missionId;
    bool success;
    string missionType;
    chrono::system_clock::time_point timestamp;
};

struct ResourceChangedEvent {
    Entity* entity;
    string resourceType;
    int oldAmount;
    int newAmount;
    chrono::system_clock::time_point timestamp;
};

/**
 * @brief Statistics Plugin - Tracks system-wide statistics
 */
class StatisticsPlugin : public IPlugin {
private:
    bool enabled = false;
    map<string, int> entityCounts;
    map<string, int> missionOutcomes;
    map<string, int> resourceChanges;
    chrono::system_clock::time_point startTime;
    
public:
    string getName() const override { return "StatisticsPlugin"; }
    string getVersion() const override { return "1.0.0"; }
    bool isEnabled() const override { return enabled; }
    
    bool initialize() override {
        startTime = chrono::system_clock::now();
        
        auto& pm = PluginManager::getInstance();
        
        // Subscribe to events
        pm.subscribeToEvent<EntityCreatedEvent>([this](const EntityCreatedEvent& event) {
            entityCounts[event.entityType]++;
            LOG_DEBUG("Statistics: Entity created - {} (total: {})", 
                     event.entityType, entityCounts[event.entityType]);
        });
        
        pm.subscribeToEvent<MissionCompletedEvent>([this](const MissionCompletedEvent& event) {
            string outcome = event.success ? "success" : "failure";
            missionOutcomes[outcome]++;
            LOG_DEBUG("Statistics: Mission {} - {}", event.missionId, outcome);
        });
        
        pm.subscribeToEvent<ResourceChangedEvent>([this](const ResourceChangedEvent& event) {
            resourceChanges[event.resourceType]++;
        });
        
        enabled = true;
        LOG_INFO("Statistics plugin initialized");
        return true;
    }
    
    void shutdown() override {
        enabled = false;
        LOG_INFO("Statistics plugin shutdown");
    }
    
    void printStatistics() const {
        if (!enabled) return;
        
        auto now = chrono::system_clock::now();
        auto runtime = chrono::duration_cast<chrono::seconds>(now - startTime).count();
        
        cout << "\n=== System Statistics ===" << endl;
        cout << "Runtime: " << runtime << " seconds" << endl;
        
        cout << "\nEntity Counts:" << endl;
        for (const auto& [type, count] : entityCounts) {
            cout << "  " << type << ": " << count << endl;
        }
        
        cout << "\nMission Outcomes:" << endl;
        for (const auto& [outcome, count] : missionOutcomes) {
            cout << "  " << outcome << ": " << count << endl;
        }
        
        cout << "\nResource Operations:" << endl;
        for (const auto& [resource, count] : resourceChanges) {
            cout << "  " << resource << " changes: " << count << endl;
        }
    }
};

/**
 * @brief AI Behavior Plugin - Adds intelligent fleet behavior
 */
class AIBehaviorPlugin : public IPlugin {
private:
    bool enabled = false;
    vector<Fleet*> managedFleets;
    
public:
    string getName() const override { return "AIBehaviorPlugin"; }
    string getVersion() const override { return "1.2.0"; }
    bool isEnabled() const override { return enabled; }
    
    bool initialize() override {
        auto& pm = PluginManager::getInstance();
        
        // Subscribe to entity creation to manage new fleets
        pm.subscribeToEvent<EntityCreatedEvent>([this](const EntityCreatedEvent& event) {
            if (event.entityType == "Fleet") {
                if (auto* fleet = dynamic_cast<Fleet*>(event.entity)) {
                    managedFleets.push_back(fleet);
                    LOG_INFO("AI Plugin now managing fleet: {}", fleet->getName());
                }
            }
        });
        
        enabled = true;
        LOG_INFO("AI Behavior plugin initialized");
        return true;
    }
    
    void shutdown() override {
        managedFleets.clear();
        enabled = false;
        LOG_INFO("AI Behavior plugin shutdown");
    }
    
    void updateAI() {
        if (!enabled) return;
        
        for (Fleet* fleet : managedFleets) {
            executeAIBehavior(fleet);
        }
    }
    
private:
    void executeAIBehavior(Fleet* fleet) {
        // Simple AI: Move fleets towards nearest planet if idle
        if (fleet->getCurrentMission() == nullptr) {
            LOG_DEBUG("AI controlling fleet: {}", fleet->getName());
            
            // In a real implementation, this would involve pathfinding,
            // threat assessment, resource optimization, etc.
        }
    }
};

/**
 * @brief Custom Resource Plugin - Adds new resource types dynamically
 */
class CustomResourcePlugin : public IPlugin {
private:
    bool enabled = false;
    vector<string> customResources = {
        "dark_matter", "antimatter", "quantum_crystals", 
        "exotic_matter", "temporal_fragments"
    };
    
public:
    string getName() const override { return "CustomResourcePlugin"; }
    string getVersion() const override { return "2.0.1"; }
    bool isEnabled() const override { return enabled; }
    
    bool initialize() override {
        // Register custom resource types
        for (const string& resource : customResources) {
            registerCustomResource(resource);
        }
        
        auto& pm = PluginManager::getInstance();
        
        // Add custom resources to new planets
        pm.subscribeToEvent<EntityCreatedEvent>([this](const EntityCreatedEvent& event) {
            if (event.entityType == "Planet") {
                if (auto* planet = dynamic_cast<Planet*>(event.entity)) {
                    addCustomResourcesToPlanet(planet);
                }
            }
        });
        
        enabled = true;
        LOG_INFO("Custom Resource plugin initialized with {} custom resources", 
                customResources.size());
        return true;
    }
    
    void shutdown() override {
        enabled = false;
        LOG_INFO("Custom Resource plugin shutdown");
    }
    
private:
    void registerCustomResource(const string& resource) {
        // In a real implementation, this would register with ResourceManager
        LOG_DEBUG("Registered custom resource: {}", resource);
    }
    
    void addCustomResourcesToPlanet(Planet* planet) {
        mt19937 rng{random_device{}()};
        uniform_int_distribution<int> amountDist(0, 100);
        
        for (const string& resource : customResources) {
            if (uniform_real_distribution<double>(0.0, 1.0)(rng) < 0.3) {  // 30% chance
                int amount = amountDist(rng);
                planet->addResources(resource, amount);
                LOG_DEBUG("Added {} {} to planet {}", amount, resource, planet->getName());
                
                // Publish resource change event
                ResourceChangedEvent event{planet, resource, 0, amount, chrono::system_clock::now()};
                PluginManager::getInstance().publishEvent(event);
            }
        }
    }
};

/**
 * @brief Performance Monitoring Plugin
 */
class PerformancePlugin : public IPlugin {
private:
    bool enabled = false;
    map<string, chrono::high_resolution_clock::time_point> operationStartTimes;
    map<string, vector<double>> operationTimes;  // in milliseconds
    
public:
    string getName() const override { return "PerformancePlugin"; }
    string getVersion() const override { return "1.1.0"; }
    bool isEnabled() const override { return enabled; }
    
    bool initialize() override {
        enabled = true;
        LOG_INFO("Performance monitoring plugin initialized");
        return true;
    }
    
    void shutdown() override {
        enabled = false;
        LOG_INFO("Performance plugin shutdown");
    }
    
    void startOperation(const string& operationName) {
        if (!enabled) return;
        operationStartTimes[operationName] = chrono::high_resolution_clock::now();
    }
    
    void endOperation(const string& operationName) {
        if (!enabled) return;
        
        auto it = operationStartTimes.find(operationName);
        if (it != operationStartTimes.end()) {
            auto endTime = chrono::high_resolution_clock::now();
            auto duration = chrono::duration_cast<chrono::microseconds>(endTime - it->second);
            double milliseconds = duration.count() / 1000.0;
            
            operationTimes[operationName].push_back(milliseconds);
            operationStartTimes.erase(it);
        }
    }
    
    void printPerformanceReport() const {
        if (!enabled) return;
        
        cout << "\n=== Performance Report ===" << endl;
        
        for (const auto& [operation, times] : operationTimes) {
            if (times.empty()) continue;
            
            double total = 0.0;
            double min_time = times[0];
            double max_time = times[0];
            
            for (double time : times) {
                total += time;
                min_time = min(min_time, time);
                max_time = max(max_time, time);
            }
            
            double average = total / times.size();
            
            cout << operation << ":" << endl;
            cout << "  Calls: " << times.size() << endl;
            cout << "  Average: " << fixed << setprecision(3) << average << "ms" << endl;
            cout << "  Min: " << min_time << "ms" << endl;
            cout << "  Max: " << max_time << "ms" << endl;
            cout << "  Total: " << total << "ms" << endl;
        }
    }
};

/**
 * @brief Demo application showing plugin system usage
 */
class PluginDemo {
public:
    void runDemo() {
        cout << "=== Plugin System Demonstration ===" << endl;
        
        auto& pluginManager = PluginManager::getInstance();
        
        // Register plugins
        cout << "\n--- Registering Plugins ---" << endl;
        
        pluginManager.registerPlugin(make_unique<StatisticsPlugin>());
        pluginManager.registerPlugin(make_unique<AIBehaviorPlugin>());
        pluginManager.registerPlugin(make_unique<CustomResourcePlugin>());
        
        auto perfPlugin = make_unique<PerformancePlugin>();
        auto* perfPtr = perfPlugin.get();  // Keep reference for direct calls
        pluginManager.registerPlugin(move(perfPlugin));
        
        // Show registered plugins
        cout << "\nRegistered plugins:" << endl;
        for (const string& name : pluginManager.getPluginNames()) {
            auto* plugin = pluginManager.getPlugin(name);
            cout << "  - " << name << " v" << plugin->getVersion() 
                 << " (enabled: " << (plugin->isEnabled() ? "yes" : "no") << ")" << endl;
        }
        
        // Create entities and trigger events
        cout << "\n--- Creating Entities ---" << endl;
        
        perfPtr->startOperation("entity_creation");
        
        // Create planets
        for (int i = 0; i < 5; ++i) {
            auto planet = make_unique<Planet>(
                "Planet-" + to_string(i),
                Vector3D{static_cast<double>(i * 50), 0, 0},
                PlanetType::TERRESTRIAL
            );
            
            // Publish entity creation event
            EntityCreatedEvent event{planet.get(), "Planet", chrono::system_clock::now()};
            pluginManager.publishEvent(event);
            
            cout << "Created: " << planet->getName() << endl;
        }
        
        // Create fleets
        for (int i = 0; i < 3; ++i) {
            vector<Ship> ships = {
                {"Ship-" + to_string(i) + "-A", ShipType::SCOUT, 100},
                {"Ship-" + to_string(i) + "-B", ShipType::COMBAT, 200}
            };
            
            auto fleet = make_unique<Fleet>(
                "Fleet-" + to_string(i),
                Vector3D{static_cast<double>(i * 30), 20, 10},
                ships
            );
            
            // Publish entity creation event
            EntityCreatedEvent event{fleet.get(), "Fleet", chrono::system_clock::now()};
            pluginManager.publishEvent(event);
            
            cout << "Created: " << fleet->getName() << endl;
        }
        
        perfPtr->endOperation("entity_creation");
        
        // Simulate missions
        cout << "\n--- Simulating Mission Completions ---" << endl;
        
        for (int i = 0; i < 8; ++i) {
            bool success = (i % 3) != 0;  // 2/3 success rate
            
            MissionCompletedEvent event{
                "Mission-" + to_string(i),
                success,
                "Exploration",
                chrono::system_clock::now()
            };
            
            pluginManager.publishEvent(event);
            cout << "Mission-" << i << ": " << (success ? "SUCCESS" : "FAILURE") << endl;
        }
        
        // Let AI plugin do some work
        cout << "\n--- Running AI Updates ---" << endl;
        if (auto* aiPlugin = dynamic_cast<AIBehaviorPlugin*>(pluginManager.getPlugin("AIBehaviorPlugin"))) {
            for (int i = 0; i < 3; ++i) {
                aiPlugin->updateAI();
                this_thread::sleep_for(chrono::milliseconds(100));
            }
        }
        
        // Display plugin reports
        cout << "\n--- Plugin Reports ---" << endl;
        
        if (auto* statsPlugin = dynamic_cast<StatisticsPlugin*>(pluginManager.getPlugin("StatisticsPlugin"))) {
            statsPlugin->printStatistics();
        }
        
        perfPtr->printPerformanceReport();
        
        // Demonstrate plugin management
        cout << "\n--- Plugin Management ---" << endl;
        
        // Unregister a plugin
        cout << "Unregistering AIBehaviorPlugin..." << endl;
        pluginManager.unregisterPlugin("AIBehaviorPlugin");
        
        cout << "Remaining plugins:" << endl;
        for (const string& name : pluginManager.getPluginNames()) {
            cout << "  - " << name << endl;
        }
        
        // Cleanup
        pluginManager.shutdown();
        
        cout << "\n=== Plugin Demo Complete ===" << endl;
    }
};

/**
 * @brief Main function
 */
int main() {
    cout << "CppVerseHub - Plugin System Example" << endl;
    cout << "===================================" << endl;
    
    // Initialize logging
    auto& logger = Logger::getInstance();
    logger.setLevel(Logger::Level::INFO);
    logger.enableConsoleLogging(true);
    
    try {
        PluginDemo demo;
        demo.runDemo();
        
        cout << "\nPlugin system example completed successfully!" << endl;
        cout << "\nThis example demonstrated:" << endl;
        cout << "  - Plugin registration and management" << endl;
        cout << "  - Event-driven plugin communication" << endl;
        cout << "  - Statistics collection plugin" << endl;
        cout << "  - AI behavior plugin" << endl;
        cout << "  - Custom resource plugin" << endl;
        cout << "  - Performance monitoring plugin" << endl;
        cout << "  - Dynamic plugin loading/unloading" << endl;
        cout << "  - Type-safe event system" << endl;
        
        return 0;
        
    } catch (const exception& e) {
        LOG_ERROR("Error in plugin systems example: {}", e.what());
        cerr << "Error: " << e.what() << endl;
        return 1;
    }
}