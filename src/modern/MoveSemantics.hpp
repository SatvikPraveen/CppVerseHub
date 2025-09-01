// File: src/modern/MoveSemantics.hpp
// Perfect Forwarding & Move Optimization Demonstration

#pragma once

#include <string>
#include <vector>
#include <memory>
#include <iostream>
#include <utility>
#include <chrono>
#include <algorithm>
#include <type_traits>

namespace CppVerseHub::Modern::MoveSemantics {

// ===== PERFORMANCE MEASUREMENT UTILITY =====

class PerformanceTimer {
private:
    std::chrono::high_resolution_clock::time_point start_time_;
    
public:
    PerformanceTimer() : start_time_(std::chrono::high_resolution_clock::now()) {}
    
    ~PerformanceTimer() {
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time_);
        std::cout << "Operation took: " << duration.count() << " microseconds" << std::endl;
    }
};

// ===== SPACE GAME ENTITIES WITH MOVE SEMANTICS =====

class ResourceContainer {
private:
    std::string name_;
    std::vector<std::string> resources_;
    std::unique_ptr<std::vector<int>> resource_amounts_;
    static int construction_count_;
    static int copy_count_;
    static int move_count_;
    
public:
    // Constructor
    ResourceContainer(std::string name) : name_(std::move(name)), resource_amounts_(std::make_unique<std::vector<int>>()) {
        ++construction_count_;
        std::cout << "ResourceContainer '" << name_ << "' constructed (Total constructions: " << construction_count_ << ")" << std::endl;
    }
    
    // Copy Constructor
    ResourceContainer(const ResourceContainer& other) 
        : name_(other.name_), resources_(other.resources_), 
          resource_amounts_(std::make_unique<std::vector<int>>(*other.resource_amounts_)) {
        ++copy_count_;
        std::cout << "ResourceContainer '" << name_ << "' COPIED (Total copies: " << copy_count_ << ")" << std::endl;
    }
    
    // Move Constructor
    ResourceContainer(ResourceContainer&& other) noexcept 
        : name_(std::move(other.name_)), resources_(std::move(other.resources_)), 
          resource_amounts_(std::move(other.resource_amounts_)) {
        ++move_count_;
        std::cout << "ResourceContainer '" << name_ << "' MOVED (Total moves: " << move_count_ << ")" << std::endl;
    }
    
    // Copy Assignment
    ResourceContainer& operator=(const ResourceContainer& other) {
        if (this != &other) {
            ++copy_count_;
            name_ = other.name_;
            resources_ = other.resources_;
            resource_amounts_ = std::make_unique<std::vector<int>>(*other.resource_amounts_);
            std::cout << "ResourceContainer '" << name_ << "' copy ASSIGNED (Total copies: " << copy_count_ << ")" << std::endl;
        }
        return *this;
    }
    
    // Move Assignment
    ResourceContainer& operator=(ResourceContainer&& other) noexcept {
        if (this != &other) {
            ++move_count_;
            name_ = std::move(other.name_);
            resources_ = std::move(other.resources_);
            resource_amounts_ = std::move(other.resource_amounts_);
            std::cout << "ResourceContainer '" << name_ << "' move ASSIGNED (Total moves: " << move_count_ << ")" << std::endl;
        }
        return *this;
    }
    
    // Destructor
    ~ResourceContainer() {
        if (!name_.empty()) {  // Only print if not moved-from
            std::cout << "ResourceContainer '" << name_ << "' destroyed" << std::endl;
        }
    }
    
    // Add resource methods
    void add_resource(const std::string& resource, int amount) {
        resources_.push_back(resource);
        resource_amounts_->push_back(amount);
    }
    
    void add_resource(std::string&& resource, int amount) {
        resources_.push_back(std::move(resource));
        resource_amounts_->push_back(amount);
    }
    
    // Getters
    const std::string& get_name() const { return name_; }
    const std::vector<std::string>& get_resources() const { return resources_; }
    size_t resource_count() const { return resources_.size(); }
    
    // Static methods for statistics
    static int get_construction_count() { return construction_count_; }
    static int get_copy_count() { return copy_count_; }
    static int get_move_count() { return move_count_; }
    static void reset_counters() { construction_count_ = copy_count_ = move_count_ = 0; }
    
    friend std::ostream& operator<<(std::ostream& os, const ResourceContainer& container) {
        os << "ResourceContainer{name='" << container.name_ << "', resources=" << container.resources_.size() << "}";
        return os;
    }
};

// Static member definitions
int ResourceContainer::construction_count_ = 0;
int ResourceContainer::copy_count_ = 0;
int ResourceContainer::move_count_ = 0;

// ===== SPACECRAFT WITH MOVE SEMANTICS =====

class Spacecraft {
private:
    int id_;
    std::string name_;
    std::string classification_;
    std::vector<ResourceContainer> cargo_;
    std::unique_ptr<std::vector<std::string>> mission_log_;
    
public:
    // Constructor
    Spacecraft(int id, std::string name, std::string classification) 
        : id_(id), name_(std::move(name)), classification_(std::move(classification)),
          mission_log_(std::make_unique<std::vector<std::string>>()) {
        std::cout << "Spacecraft " << name_ << " constructed" << std::endl;
    }
    
    // Move-only class (deleted copy operations)
    Spacecraft(const Spacecraft&) = delete;
    Spacecraft& operator=(const Spacecraft&) = delete;
    
    // Move constructor
    Spacecraft(Spacecraft&& other) noexcept 
        : id_(other.id_), name_(std::move(other.name_)), 
          classification_(std::move(other.classification_)),
          cargo_(std::move(other.cargo_)), 
          mission_log_(std::move(other.mission_log_)) {
        std::cout << "Spacecraft " << name_ << " moved" << std::endl;
        other.id_ = -1; // Mark as moved-from
    }
    
    // Move assignment
    Spacecraft& operator=(Spacecraft&& other) noexcept {
        if (this != &other) {
            id_ = other.id_;
            name_ = std::move(other.name_);
            classification_ = std::move(other.classification_);
            cargo_ = std::move(other.cargo_);
            mission_log_ = std::move(other.mission_log_);
            other.id_ = -1;
            std::cout << "Spacecraft " << name_ << " move assigned" << std::endl;
        }
        return *this;
    }
    
    ~Spacecraft() {
        if (id_ != -1) {  // Only print if not moved-from
            std::cout << "Spacecraft " << name_ << " destroyed" << std::endl;
        }
    }
    
    // Perfect forwarding for adding cargo
    template<typename... Args>
    void add_cargo(Args&&... args) {
        cargo_.emplace_back(std::forward<Args>(args)...);
    }
    
    // Add mission log entry with perfect forwarding
    template<typename T>
    void log_mission(T&& message) {
        if constexpr (std::is_rvalue_reference_v<T&&>) {
            std::cout << "Moving mission log entry: " << message << std::endl;
        } else {
            std::cout << "Copying mission log entry: " << message << std::endl;
        }
        mission_log_->push_back(std::forward<T>(message));
    }
    
    // Getters
    int get_id() const { return id_; }
    const std::string& get_name() const { return name_; }
    const std::string& get_classification() const { return classification_; }
    size_t cargo_count() const { return cargo_.size(); }
    size_t mission_log_size() const { return mission_log_->size(); }
    
    friend std::ostream& operator<<(std::ostream& os, const Spacecraft& ship) {
        os << "Spacecraft{id=" << ship.id_ << ", name='" << ship.name_ 
           << "', class='" << ship.classification_ << "', cargo=" << ship.cargo_.size() << "}";
        return os;
    }
};

// ===== PERFECT FORWARDING UTILITIES =====

namespace ForwardingUtils {
    
    // Universal reference wrapper for perfect forwarding
    template<typename Func, typename... Args>
    decltype(auto) invoke_perfectly(Func&& func, Args&&... args) {
        std::cout << "Perfect forwarding with " << sizeof...(args) << " arguments" << std::endl;
        return std::forward<Func>(func)(std::forward<Args>(args)...);
    }
    
    // Factory function with perfect forwarding
    template<typename T, typename... Args>
    std::unique_ptr<T> make_unique_perfect(Args&&... args) {
        std::cout << "Creating unique_ptr with perfect forwarding" << std::endl;
        return std::make_unique<T>(std::forward<Args>(args)...);
    }
    
    // Forwarding to member functions
    template<typename Object, typename Method, typename... Args>
    decltype(auto) invoke_member(Object&& obj, Method&& method, Args&&... args) {
        std::cout << "Invoking member function with perfect forwarding" << std::endl;
        return (std::forward<Object>(obj).*std::forward<Method>(method))(std::forward<Args>(args)...);
    }
    
    // Conditional move utility
    template<typename T>
    constexpr decltype(auto) move_if_noexcept(T& x) {
        if constexpr (std::is_nothrow_move_constructible_v<T>) {
            return std::move(x);
        } else {
            return x;
        }
    }
    
} // namespace ForwardingUtils

// ===== MOVE-AWARE CONTAINER =====

template<typename T>
class MoveAwareVector {
private:
    std::vector<T> data_;
    mutable size_t access_count_ = 0;
    
public:
    MoveAwareVector() = default;
    
    // Copy constructor
    MoveAwareVector(const MoveAwareVector& other) : data_(other.data_) {
        std::cout << "MoveAwareVector copied with " << data_.size() << " elements" << std::endl;
    }
    
    // Move constructor
    MoveAwareVector(MoveAwareVector&& other) noexcept : data_(std::move(other.data_)) {
        std::cout << "MoveAwareVector moved with " << data_.size() << " elements" << std::endl;
    }
    
    // Copy assignment
    MoveAwareVector& operator=(const MoveAwareVector& other) {
        if (this != &other) {
            data_ = other.data_;
            std::cout << "MoveAwareVector copy assigned with " << data_.size() << " elements" << std::endl;
        }
        return *this;
    }
    
    // Move assignment
    MoveAwareVector& operator=(MoveAwareVector&& other) noexcept {
        if (this != &other) {
            data_ = std::move(other.data_);
            std::cout << "MoveAwareVector move assigned with " << data_.size() << " elements" << std::endl;
        }
        return *this;
    }
    
    // Perfect forwarding for emplacement
    template<typename... Args>
    void emplace_back(Args&&... args) {
        std::cout << "Emplacing element with perfect forwarding" << std::endl;
        data_.emplace_back(std::forward<Args>(args)...);
    }
    
    // Move-aware push_back
    void push_back(const T& value) {
        std::cout << "Pushing back by copy" << std::endl;
        data_.push_back(value);
    }
    
    void push_back(T&& value) {
        std::cout << "Pushing back by move" << std::endl;
        data_.push_back(std::move(value));
    }
    
    // Access methods
    const T& at(size_t index) const {
        ++access_count_;
        return data_.at(index);
    }
    
    T& at(size_t index) {
        ++access_count_;
        return data_.at(index);
    }
    
    size_t size() const { return data_.size(); }
    size_t access_count() const { return access_count_; }
    
    // Iterator support
    auto begin() { return data_.begin(); }
    auto end() { return data_.end(); }
    auto begin() const { return data_.begin(); }
    auto end() const { return data_.end(); }
};

// ===== RVALUE REFERENCE DEMONSTRATIONS =====

void demonstrate_basic_move_semantics() {
    std::cout << "\n=== Basic Move Semantics ===" << std::endl;
    
    ResourceContainer::reset_counters();
    
    // Create original container
    ResourceContainer original("Earth Station");
    original.add_resource("Water", 1000);
    original.add_resource("Oxygen", 500);
    
    std::cout << "\nCopy vs Move comparison:" << std::endl;
    
    // Copy operation
    {
        std::cout << "Creating copy:" << std::endl;
        PerformanceTimer timer;
        ResourceContainer copy = original;  // Copy constructor
    }
    
    // Move operation
    {
        std::cout << "Creating temporary for move:" << std::endl;
        ResourceContainer temp("Mars Station");
        temp.add_resource("Iron", 750);
        temp.add_resource("Silicon", 300);
        
        std::cout << "Moving temporary:" << std::endl;
        PerformanceTimer timer;
        ResourceContainer moved = std::move(temp);  // Move constructor
    }
    
    std::cout << "\nFinal statistics:" << std::endl;
    std::cout << "Constructions: " << ResourceContainer::get_construction_count() << std::endl;
    std::cout << "Copies: " << ResourceContainer::get_copy_count() << std::endl;
    std::cout << "Moves: " << ResourceContainer::get_move_count() << std::endl;
}

void demonstrate_perfect_forwarding() {
    std::cout << "\n=== Perfect Forwarding ===" << std::endl;
    
    // Factory function demonstration
    std::cout << "Creating ResourceContainer with perfect forwarding:" << std::endl;
    auto container = ForwardingUtils::make_unique_perfect<ResourceContainer>("Jupiter Station");
    
    // Function call with perfect forwarding
    auto add_resource_func = [](ResourceContainer& container, const std::string& name, int amount) {
        container.add_resource(name, amount);
        std::cout << "Added resource: " << name << " (amount: " << amount << ")" << std::endl;
    };
    
    std::cout << "\nForwarding to lambda function:" << std::endl;
    ForwardingUtils::invoke_perfectly(add_resource_func, *container, std::string("Helium"), 200);
    
    // Member function forwarding
    std::cout << "\nForwarding to member function:" << std::endl;
    auto add_method = &ResourceContainer::add_resource;
    ForwardingUtils::invoke_member(*container, add_method, std::string("Hydrogen"), 150);
}

void demonstrate_move_only_types() {
    std::cout << "\n=== Move-Only Types ===" << std::endl;
    
    // Create spacecraft (move-only type)
    std::cout << "Creating spacecraft:" << std::endl;
    Spacecraft explorer(1, "USS Explorer", "Science Vessel");
    
    // Add cargo using perfect forwarding
    std::cout << "\nAdding cargo with perfect forwarding:" << std::endl;
    explorer.add_cargo("Research Station Alpha");
    explorer.add_cargo(std::string("Mining Platform Beta"));  // Explicit move
    
    // Move spacecraft into vector
    std::cout << "\nMoving spacecraft into container:" << std::endl;
    std::vector<Spacecraft> fleet;
    fleet.push_back(std::move(explorer));
    
    std::cout << "Fleet size: " << fleet.size() << std::endl;
    std::cout << "First spacecraft: " << fleet[0] << std::endl;
    
    // Mission logging with perfect forwarding
    std::cout << "\nLogging missions:" << std::endl;
    std::string mission_report = "Exploration of Alpha Centauri completed";
    fleet[0].log_mission(mission_report);  // Copy
    fleet[0].log_mission("Emergency rescue operation successful");  // Move
}

void demonstrate_move_aware_container() {
    std::cout << "\n=== Move-Aware Container ===" << std::endl;
    
    MoveAwareVector<ResourceContainer> containers;
    
    std::cout << "Adding elements to move-aware container:" << std::endl;
    
    // Emplace back (perfect forwarding)
    containers.emplace_back("Station Alpha");
    containers.emplace_back("Station Beta");
    
    // Push back with copy
    ResourceContainer temp("Station Gamma");
    containers.push_back(temp);
    
    // Push back with move
    containers.push_back(std::move(temp));
    
    std::cout << "\nContainer size: " << containers.size() << std::endl;
    std::cout << "Accessing elements:" << std::endl;
    for (size_t i = 0; i < containers.size(); ++i) {
        std::cout << "  " << containers.at(i) << std::endl;
    }
    
    std::cout << "Total access count: " << containers.access_count() << std::endl;
}

// ===== MOVE SEMANTICS OPTIMIZATION PATTERNS =====

namespace OptimizationPatterns {
    
    // Return value optimization aware function
    std::vector<ResourceContainer> create_resource_fleet(size_t count) {
        std::vector<ResourceContainer> fleet;
        fleet.reserve(count);  // Prevent reallocations
        
        for (size_t i = 0; i < count; ++i) {
            fleet.emplace_back("Station-" + std::to_string(i));
        }
        
        return fleet;  // RVO/NRVO applies here
    }
    
    // Move-aware swap
    template<typename T>
    void efficient_swap(T& a, T& b) noexcept(std::is_nothrow_move_constructible_v<T> && 
                                            std::is_nothrow_move_assignable_v<T>) {
        T temp = std::move(a);
        a = std::move(b);
        b = std::move(temp);
    }
    
    // Conditional move based on exception safety
    template<typename T>
    void safe_move_assign(T& target, T& source) {
        if constexpr (std::is_nothrow_move_assignable_v<T>) {
            std::cout << "Using move assignment (nothrow)" << std::endl;
            target = std::move(source);
        } else {
            std::cout << "Using copy assignment (exception safety)" << std::endl;
            target = source;
        }
    }
    
} // namespace OptimizationPatterns

void demonstrate_optimization_patterns() {
    std::cout << "\n=== Move Optimization Patterns ===" << std::endl;
    
    // RVO demonstration
    std::cout << "Creating fleet with RVO:" << std::endl;
    {
        PerformanceTimer timer;
        auto fleet = OptimizationPatterns::create_resource_fleet(3);
        std::cout << "Created fleet with " << fleet.size() << " stations" << std::endl;
    }
    
    // Efficient swap
    std::cout << "\nEfficient swap demonstration:" << std::endl;
    ResourceContainer station1("Alpha Base");
    ResourceContainer station2("Beta Base");
    
    station1.add_resource("Gold", 100);
    station2.add_resource("Platinum", 200);
    
    std::cout << "Before swap - Station1: " << station1 << ", Station2: " << station2 << std::endl;
    OptimizationPatterns::efficient_swap(station1, station2);
    std::cout << "After swap - Station1: " << station1 << ", Station2: " << station2 << std::endl;
    
    // Exception-safe move
    std::cout << "\nException-safe move assignment:" << std::endl;
    ResourceContainer target("Target");
    ResourceContainer source("Source");
    source.add_resource("Rare Element", 50);
    
    OptimizationPatterns::safe_move_assign(target, source);
}

// ===== MOVE SEMANTICS BENCHMARKING =====

void benchmark_move_vs_copy() {
    std::cout << "\n=== Move vs Copy Performance Benchmark ===" << std::endl;
    
    const size_t element_count = 10000;
    const size_t iterations = 100;
    
    // Create test data
    std::vector<ResourceContainer> source_data;
    source_data.reserve(element_count);
    for (size_t i = 0; i < element_count; ++i) {
        ResourceContainer container("Container-" + std::to_string(i));
        for (size_t j = 0; j < 10; ++j) {
            container.add_resource("Resource-" + std::to_string(j), static_cast<int>(i + j));
        }
        source_data.push_back(std::move(container));
    }
    
    ResourceContainer::reset_counters();
    
    // Copy benchmark
    std::cout << "Copy performance test:" << std::endl;
    auto start_copy = std::chrono::high_resolution_clock::now();
    
    for (size_t iter = 0; iter < iterations; ++iter) {
        std::vector<ResourceContainer> copied_data;
        copied_data.reserve(element_count);
        for (const auto& container : source_data) {
            copied_data.push_back(container);  // Copy
        }
    }
    
    auto end_copy = std::chrono::high_resolution_clock::now();
    auto copy_duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_copy - start_copy);
    
    // Move benchmark (recreate source data)
    source_data.clear();
    for (size_t i = 0; i < element_count; ++i) {
        ResourceContainer container("Container-" + std::to_string(i));
        for (size_t j = 0; j < 10; ++j) {
            container.add_resource("Resource-" + std::to_string(j), static_cast<int>(i + j));
        }
        source_data.push_back(std::move(container));
    }
    
    std::cout << "Move performance test:" << std::endl;
    auto start_move = std::chrono::high_resolution_clock::now();
    
    for (size_t iter = 0; iter < iterations; ++iter) {
        // Recreate source data for each iteration
        std::vector<ResourceContainer> temp_source;
        temp_source.reserve(element_count);
        for (size_t i = 0; i < element_count; ++i) {
            temp_source.emplace_back("TempContainer-" + std::to_string(i));
        }
        
        std::vector<ResourceContainer> moved_data;
        moved_data.reserve(element_count);
        for (auto& container : temp_source) {
            moved_data.push_back(std::move(container));  // Move
        }
    }
    
    auto end_move = std::chrono::high_resolution_clock::now();
    auto move_duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_move - start_move);
    
    // Results
    std::cout << "\nBenchmark Results (" << iterations << " iterations, " << element_count << " elements each):" << std::endl;
    std::cout << "  Copy time: " << copy_duration.count() << " ms" << std::endl;
    std::cout << "  Move time: " << move_duration.count() << " ms" << std::endl;
    
    if (move_duration.count() > 0) {
        double speedup = static_cast<double>(copy_duration.count()) / move_duration.count();
        std::cout << "  Move speedup: " << std::fixed << std::setprecision(2) << speedup << "x faster" << std::endl;
    }
    
    std::cout << "\nOperation counts:" << std::endl;
    std::cout << "  Total copies: " << ResourceContainer::get_copy_count() << std::endl;
    std::cout << "  Total moves: " << ResourceContainer::get_move_count() << std::endl;
}

// ===== MAIN DEMONSTRATION FUNCTION =====

void demonstrate_all_move_semantics() {
    std::cout << "\n🚀 Perfect Forwarding & Move Semantics Demonstration 🚀" << std::endl;
    std::cout << "=======================================================" << std::endl;
    
    demonstrate_basic_move_semantics();
    demonstrate_perfect_forwarding();
    demonstrate_move_only_types();
    demonstrate_move_aware_container();
    demonstrate_optimization_patterns();
    benchmark_move_vs_copy();
    
    std::cout << "\n✨ Move semantics demonstration complete! ✨" << std::endl;
    std::cout << "\nKey Move Semantics Benefits:" << std::endl;
    std::cout << "• Eliminates unnecessary copies" << std::endl;
    std::cout << "• Perfect forwarding preserves value categories" << std::endl;
    std::cout << "• Move-only types enable unique ownership" << std::endl;
    std::cout << "• Significant performance improvements" << std::endl;
    std::cout << "• Exception safety with noexcept moves" << std::endl;
    std::cout << "• Return value optimization compatibility" << std::endl;
}

} // namespace CppVerseHub::Modern::MoveSemantics