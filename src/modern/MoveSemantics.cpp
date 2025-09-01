// File: src/modern/MoveSemantics.cpp
// Perfect Forwarding & Move Optimization Implementation

#include "MoveSemantics.hpp"
#include <iomanip>
#include <sstream>
#include <thread>

namespace CppVerseHub::Modern::MoveSemantics {

// ===== ADVANCED MOVE SEMANTICS EXAMPLES =====

class AdvancedSpaceStation {
private:
    std::string name_;
    std::vector<std::unique_ptr<ResourceContainer>> modules_;
    std::unique_ptr<std::map<std::string, double>> sensor_data_;
    std::shared_ptr<std::vector<std::string>> communication_log_;
    
public:
    // Constructor with perfect forwarding
    template<typename StringType>
    AdvancedSpaceStation(StringType&& name) 
        : name_(std::forward<StringType>(name)),
          sensor_data_(std::make_unique<std::map<std::string, double>>()),
          communication_log_(std::make_shared<std::vector<std::string>>()) {
        
        if constexpr (std::is_rvalue_reference_v<StringType&&>) {
            std::cout << "AdvancedSpaceStation: Moving name '" << name_ << "'" << std::endl;
        } else {
            std::cout << "AdvancedSpaceStation: Copying name '" << name_ << "'" << std::endl;
        }
    }
    
    // Move-only semantics
    AdvancedSpaceStation(const AdvancedSpaceStation&) = delete;
    AdvancedSpaceStation& operator=(const AdvancedSpaceStation&) = delete;
    
    AdvancedSpaceStation(AdvancedSpaceStation&& other) noexcept 
        : name_(std::move(other.name_)),
          modules_(std::move(other.modules_)),
          sensor_data_(std::move(other.sensor_data_)),
          communication_log_(std::move(other.communication_log_)) {
        std::cout << "AdvancedSpaceStation '" << name_ << "' moved" << std::endl;
    }
    
    AdvancedSpaceStation& operator=(AdvancedSpaceStation&& other) noexcept {
        if (this != &other) {
            name_ = std::move(other.name_);
            modules_ = std::move(other.modules_);
            sensor_data_ = std::move(other.sensor_data_);
            communication_log_ = std::move(other.communication_log_);
            std::cout << "AdvancedSpaceStation '" << name_ << "' move assigned" << std::endl;
        }
        return *this;
    }
    
    ~AdvancedSpaceStation() {
        if (!name_.empty()) {
            std::cout << "AdvancedSpaceStation '" << name_ << "' destroyed" << std::endl;
        }
    }
    
    // Perfect forwarding for module addition
    template<typename... Args>
    void add_module(Args&&... args) {
        std::cout << "Adding module with perfect forwarding" << std::endl;
        modules_.push_back(std::make_unique<ResourceContainer>(std::forward<Args>(args)...));
    }
    
    // Sensor data with universal references
    template<typename Key, typename Value>
    void record_sensor_data(Key&& key, Value&& value) {
        if constexpr (std::is_rvalue_reference_v<Key&&>) {
            std::cout << "Moving sensor key: " << key << std::endl;
        } else {
            std::cout << "Copying sensor key: " << key << std::endl;
        }
        (*sensor_data_)[std::forward<Key>(key)] = std::forward<Value>(value);
    }
    
    // Communication log with perfect forwarding
    template<typename Message>
    void log_communication(Message&& message) {
        std::ostringstream oss;
        oss << "[" << std::chrono::duration_cast<std::chrono::seconds>(
                       std::chrono::system_clock::now().time_since_epoch()).count()
            << "] " << std::forward<Message>(message);
        
        communication_log_->push_back(oss.str());
        
        if constexpr (std::is_rvalue_reference_v<Message&&>) {
            std::cout << "Logged communication (moved): " << message << std::endl;
        } else {
            std::cout << "Logged communication (copied): " << message << std::endl;
        }
    }
    
    // Getters
    const std::string& get_name() const { return name_; }
    size_t module_count() const { return modules_.size(); }
    size_t sensor_data_count() const { return sensor_data_->size(); }
    size_t communication_log_size() const { return communication_log_->size(); }
    
    // Get communication log (shared ownership)
    std::shared_ptr<std::vector<std::string>> get_communication_log() const {
        return communication_log_;
    }
};

// ===== MOVE-SEMANTICS AWARE ALGORITHMS =====

namespace MoveAlgorithms {
    
    // Move-aware filter
    template<typename Container, typename Predicate>
    Container move_filter(Container&& container, Predicate pred) {
        Container result;
        
        for (auto& item : container) {
            if (pred(item)) {
                if constexpr (std::is_rvalue_reference_v<Container&&>) {
                    result.push_back(std::move(item));
                } else {
                    result.push_back(item);
                }
            }
        }
        
        return result;
    }
    
    // Move-aware transform
    template<typename InputContainer, typename OutputContainer, typename Transform>
    OutputContainer move_transform(InputContainer&& input, Transform transform) {
        OutputContainer output;
        output.reserve(input.size());
        
        for (auto& item : input) {
            if constexpr (std::is_rvalue_reference_v<InputContainer&&>) {
                output.push_back(transform(std::move(item)));
            } else {
                output.push_back(transform(item));
            }
        }
        
        return output;
    }
    
    // Partition with move semantics
    template<typename Container, typename Predicate>
    std::pair<Container, Container> move_partition(Container&& container, Predicate pred) {
        Container true_partition, false_partition;
        
        for (auto& item : container) {
            if (pred(item)) {
                if constexpr (std::is_rvalue_reference_v<Container&&>) {
                    true_partition.push_back(std::move(item));
                } else {
                    true_partition.push_back(item);
                }
            } else {
                if constexpr (std::is_rvalue_reference_v<Container&&>) {
                    false_partition.push_back(std::move(item));
                } else {
                    false_partition.push_back(item);
                }
            }
        }
        
        return std::make_pair(std::move(true_partition), std::move(false_partition));
    }
    
} // namespace MoveAlgorithms

void demonstrate_advanced_move_patterns() {
    std::cout << "\n=== Advanced Move Patterns ===" << std::endl;
    
    // Advanced space station with perfect forwarding
    std::cout << "Creating advanced space stations:" << std::endl;
    
    std::string station_name = "Deep Space Nine";
    AdvancedSpaceStation station1(station_name);  // Copy
    AdvancedSpaceStation station2(std::string("Babylon 5"));  // Move
    AdvancedSpaceStation station3("Enterprise");  // Move (string literal)
    
    // Add modules with perfect forwarding
    std::cout << "\nAdding modules:" << std::endl;
    station1.add_module("Command Center");
    station1.add_module(std::string("Engineering Deck"));
    
    std::string module_name = "Medical Bay";
    station1.add_module(module_name);  // Copy
    station1.add_module(std::move(module_name));  // Move
    
    // Record sensor data
    std::cout << "\nRecording sensor data:" << std::endl;
    std::string sensor_key = "temperature";
    station1.record_sensor_data(sensor_key, 23.5);  // Copy key
    station1.record_sensor_data(std::string("humidity"), 45.2);  // Move key
    
    // Communication logging
    std::cout << "\nLogging communications:" << std::endl;
    std::string message = "All systems operational";
    station1.log_communication(message);  // Copy
    station1.log_communication("Incoming transmission from Earth");  // Move
    
    // Move station into container
    std::cout << "\nMoving stations:" << std::endl;
    std::vector<AdvancedSpaceStation> station_network;
    station_network.push_back(std::move(station1));
    station_network.push_back(std::move(station2));
    
    std::cout << "Station network size: " << station_network.size() << std::endl;
    std::cout << "First station modules: " << station_network[0].module_count() << std::endl;
}

void demonstrate_move_algorithms() {
    std::cout << "\n=== Move-Aware Algorithms ===" << std::endl;
    
    // Create test containers
    std::vector<ResourceContainer> resources;
    resources.emplace_back("Gold Mining Facility");
    resources.emplace_back("Platinum Refinery");
    resources.emplace_back("Iron Foundry");
    resources.emplace_back("Rare Earth Processing");
    resources.emplace_back("Water Treatment Plant");
    
    std::cout << "Original resources: " << resources.size() << std::endl;
    ResourceContainer::reset_counters();
    
    // Move filter - filter containers with long names
    std::cout << "\nFiltering with move semantics:" << std::endl;
    auto long_named = MoveAlgorithms::move_filter(
        std::move(resources),  // Move the entire container
        [](const ResourceContainer& container) {
            return container.get_name().length() > 15;
        });
    
    std::cout << "Filtered resources: " << long_named.size() << std::endl;
    std::cout << "Moves performed: " << ResourceContainer::get_move_count() << std::endl;
    
    // Create new test data for transform
    std::vector<std::string> facility_names = {
        "Alpha Station", "Beta Outpost", "Gamma Base", "Delta Platform"
    };
    
    std::cout << "\nTransforming with move semantics:" << std::endl;
    ResourceContainer::reset_counters();
    
    auto transformed = MoveAlgorithms::move_transform<
        std::vector<std::string>, 
        std::vector<ResourceContainer>
    >(
        std::move(facility_names),
        [](std::string&& name) -> ResourceContainer {
            std::cout << "  Creating ResourceContainer from: " << name << std::endl;
            return ResourceContainer(std::move(name));
        }
    );
    
    std::cout << "Transformed containers: " << transformed.size() << std::endl;
    std::cout << "Moves performed: " << ResourceContainer::get_move_count() << std::endl;
    
    // Partition example
    std::cout << "\nPartitioning with move semantics:" << std::endl;
    ResourceContainer::reset_counters();
    
    auto [short_named, long_named_partition] = MoveAlgorithms::move_partition(
        std::move(transformed),
        [](const ResourceContainer& container) {
            return container.get_name().length() <= 12;
        });
    
    std::cout << "Short named containers: " << short_named.size() << std::endl;
    std::cout << "Long named containers: " << long_named_partition.size() << std::endl;
    std::cout << "Moves performed: " << ResourceContainer::get_move_count() << std::endl;
}

// ===== EXCEPTION SAFETY WITH MOVE SEMANTICS =====

class ExceptionSafeContainer {
private:
    std::vector<ResourceContainer> data_;
    
public:
    ExceptionSafeContainer() = default;
    
    // Strong exception safety guarantee
    template<typename... Args>
    void safe_emplace_back(Args&&... args) {
        try {
            // Create temporary object first
            ResourceContainer temp(std::forward<Args>(args)...);
            
            // If construction succeeds, move it into container
            // Move operations should be noexcept
            data_.push_back(std::move(temp));
            
            std::cout << "Successfully emplaced container" << std::endl;
        } catch (const std::exception& e) {
            std::cout << "Exception during construction: " << e.what() << std::endl;
            // Object is not added to container, maintaining strong guarantee
            throw;
        }
    }
    
    // Exception-safe swap
    void swap(ExceptionSafeContainer& other) noexcept {
        data_.swap(other.data_);
    }
    
    size_t size() const { return data_.size(); }
    
    // Exception-safe move assignment
    ExceptionSafeContainer& operator=(ExceptionSafeContainer&& other) noexcept {
        if (this != &other) {
            // Use swap for exception safety
            ExceptionSafeContainer temp(std::move(other));
            swap(temp);
        }
        return *this;
    }
};

void demonstrate_exception_safety() {
    std::cout << "\n=== Exception Safety with Move Semantics ===" << std::endl;
    
    ExceptionSafeContainer container;
    
    std::cout << "Adding elements with exception safety:" << std::endl;
    
    try {
        container.safe_emplace_back("Safe Container 1");
        container.safe_emplace_back("Safe Container 2");
        
        std::cout << "Container size after additions: " << container.size() << std::endl;
        
        // Simulate potential exception scenario
        // In a real scenario, this might throw during construction
        container.safe_emplace_back("Safe Container 3");
        
    } catch (const std::exception& e) {
        std::cout << "Caught exception: " << e.what() << std::endl;
        std::cout << "Container size after exception: " << container.size() << std::endl;
    }
    
    // Test exception-safe move assignment
    std::cout << "\nTesting exception-safe move assignment:" << std::endl;
    ExceptionSafeContainer other_container;
    other_container.safe_emplace_back("Other Container 1");
    
    std::cout << "Before move - Original: " << container.size() 
              << ", Other: " << other_container.size() << std::endl;
    
    container = std::move(other_container);
    
    std::cout << "After move - Original: " << container.size() 
              << ", Other: " << other_container.size() << std::endl;
}

// ===== MOVE SEMANTICS WITH THREADING =====

void demonstrate_concurrent_move_operations() {
    std::cout << "\n=== Concurrent Move Operations ===" << std::endl;
    
    const size_t num_threads = 4;
    const size_t items_per_thread = 1000;
    
    ResourceContainer::reset_counters();
    
    // Thread-safe move operations
    std::vector<std::thread> workers;
    std::vector<std::vector<ResourceContainer>> thread_results(num_threads);
    
    std::cout << "Creating " << num_threads << " worker threads..." << std::endl;
    
    for (size_t thread_id = 0; thread_id < num_threads; ++thread_id) {
        workers.emplace_back([thread_id, items_per_thread, &thread_results]() {
            std::vector<ResourceContainer> local_containers;
            local_containers.reserve(items_per_thread);
            
            for (size_t i = 0; i < items_per_thread; ++i) {
                std::string name = "Thread-" + std::to_string(thread_id) + 
                                 "-Container-" + std::to_string(i);
                local_containers.emplace_back(std::move(name));
            }
            
            // Move the entire vector to the results
            thread_results[thread_id] = std::move(local_containers);
        });
    }
    
    // Wait for all threads to complete
    for (auto& worker : workers) {
        worker.join();
    }
    
    // Collect results using move semantics
    std::vector<ResourceContainer> final_results;
    final_results.reserve(num_threads * items_per_thread);
    
    for (auto& thread_result : thread_results) {
        for (auto& container : thread_result) {
            final_results.push_back(std::move(container));
        }
    }
    
    std::cout << "Collected " << final_results.size() << " containers from all threads" << std::endl;
    std::cout << "Total moves performed: " << ResourceContainer::get_move_count() << std::endl;
    std::cout << "Total copies performed: " << ResourceContainer::get_copy_count() << std::endl;
    
    double move_efficiency = static_cast<double>(ResourceContainer::get_move_count()) / 
                            (ResourceContainer::get_move_count() + ResourceContainer::get_copy_count()) * 100.0;
    std::cout << "Move efficiency: " << std::fixed << std::setprecision(1) << move_efficiency << "%" << std::endl;
}

// ===== COMPREHENSIVE MOVE SEMANTICS ANALYSIS =====

void analyze_move_semantics_impact() {
    std::cout << "\n=== Move Semantics Impact Analysis ===" << std::endl;
    
    const size_t test_size = 5000;
    
    // Test 1: Vector reallocations with and without move semantics
    std::cout << "Test 1: Vector reallocations" << std::endl;
    ResourceContainer::reset_counters();
    
    {
        std::vector<ResourceContainer> move_aware_vector;
        // Intentionally not reserving space to force reallocations
        
        for (size_t i = 0; i < test_size; ++i) {
            move_aware_vector.emplace_back("Container-" + std::to_string(i));
        }
        
        std::cout << "  Vector with move semantics:" << std::endl;
        std::cout << "    Constructions: " << ResourceContainer::get_construction_count() << std::endl;
        std::cout << "    Copies: " << ResourceContainer::get_copy_count() << std::endl;
        std::cout << "    Moves: " << ResourceContainer::get_move_count() << std::endl;
    }
    
    // Test 2: Return value optimization with move semantics
    std::cout << "\nTest 2: Function returns" << std::endl;
    ResourceContainer::reset_counters();
    
    auto create_container_vector = [](size_t count) -> std::vector<ResourceContainer> {
        std::vector<ResourceContainer> result;
        result.reserve(count);
        
        for (size_t i = 0; i < count; ++i) {
            result.emplace_back("Returned-" + std::to_string(i));
        }
        
        return result; // RVO + move semantics
    };
    
    {
        auto returned_vector = create_container_vector(1000);
        std::cout << "  Function return with RVO + moves:" << std::endl;
        std::cout << "    Constructions: " << ResourceContainer::get_construction_count() << std::endl;
        std::cout << "    Copies: " << ResourceContainer::get_copy_count() << std::endl;
        std::cout << "    Moves: " << ResourceContainer::get_move_count() << std::endl;
        std::cout << "    Returned vector size: " << returned_vector.size() << std::endl;
    }
    
    // Test 3: Algorithm performance with move semantics
    std::cout << "\nTest 3: Algorithm performance" << std::endl;
    ResourceContainer::reset_counters();
    
    {
        std::vector<ResourceContainer> source;
        source.reserve(2000);
        for (size_t i = 0; i < 2000; ++i) {
            source.emplace_back("Source-" + std::to_string(i));
        }
        
        // Sort using move-aware operations
        std::sort(source.begin(), source.end(), 
                 [](const ResourceContainer& a, const ResourceContainer& b) {
                     return a.get_name() < b.get_name();
                 });
        
        std::cout << "  Sorting with move-aware comparison:" << std::endl;
        std::cout << "    Additional moves during sort: " << ResourceContainer::get_move_count() << std::endl;
        std::cout << "    Additional copies during sort: " << ResourceContainer::get_copy_count() << std::endl;
    }
    
    // Summary
    std::cout << "\n=== Move Semantics Benefits Summary ===" << std::endl;
    std::cout << "✓ Reduced memory allocations and deallocations" << std::endl;
    std::cout << "✓ Eliminated unnecessary deep copies" << std::endl;
    std::cout << "✓ Improved performance for temporary objects" << std::endl;
    std::cout << "✓ Better exception safety with noexcept moves" << std::endl;
    std::cout << "✓ Enabled move-only types for unique ownership" << std::endl;
    std::cout << "✓ Perfect forwarding preserves efficiency" << std::endl;
}

} // namespace CppVerseHub::Modern::MoveSemantics