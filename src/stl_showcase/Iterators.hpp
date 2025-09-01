// File: src/stl_showcase/Iterators.hpp
// CppVerseHub - STL Iterators Demonstrations (Custom & STL iterator usage)

#pragma once

#include <iterator>
#include <vector>
#include <list>
#include <map>
#include <set>
#include <iostream>
#include <algorithm>
#include <memory>
#include <string>
#include <sstream>

namespace CppVerseHub::STL {

/**
 * @brief StarSystem data structure for iterator demonstrations
 */
struct StarSystem {
    std::string name;
    int planet_count;
    double distance_ly;
    std::string classification;
    
    StarSystem(const std::string& n = "", int pc = 0, double dist = 0.0, const std::string& cls = "")
        : name(n), planet_count(pc), distance_ly(dist), classification(cls) {}
    
    friend std::ostream& operator<<(std::ostream& os, const StarSystem& sys) {
        os << sys.name << " (" << sys.classification << ") - " 
           << sys.planet_count << " planets, " << sys.distance_ly << " ly";
        return os;
    }
};

/**
 * @brief Iterator Category Demonstrations
 * 
 * Demonstrates the five iterator categories and their capabilities:
 * input, output, forward, bidirectional, and random access iterators.
 */
class IteratorCategories {
public:
    /**
     * @brief Demonstrate input iterators
     */
    static void demonstrateInputIterators() {
        std::cout << "\n=== Input Iterator Demonstration ===\n";
        
        // Input iterators allow reading from a sequence
        // std::istream_iterator is a classic example
        std::string sensor_data = "10.5 20.3 15.7 8.9 25.1";
        std::istringstream iss(sensor_data);
        
        std::cout << "Reading sensor data from stream: " << sensor_data << "\n";
        std::cout << "Parsed values: ";
        
        // Create input iterator for double values from stream
        std::istream_iterator<double> input_iter(iss);
        std::istream_iterator<double> end_iter; // Default constructor creates end iterator
        
        while (input_iter != end_iter) {
            std::cout << *input_iter << " ";
            ++input_iter; // Input iterators only support increment
        }
        std::cout << "\n";
        
        // Input iterators are single-pass - once advanced, previous elements can't be accessed
        std::cout << "Input iterators are single-pass only!\n";
        
        // Using input iterator with algorithm
        std::string numbers_str = "1 2 3 4 5 6 7 8 9 10";
        std::istringstream number_stream(numbers_str);
        
        std::vector<int> numbers;
        std::copy(std::istream_iterator<int>(number_stream),
                 std::istream_iterator<int>(),
                 std::back_inserter(numbers));
        
        std::cout << "Numbers copied from stream: ";
        for (int n : numbers) std::cout << n << " ";
        std::cout << "\n";
    }
    
    /**
     * @brief Demonstrate output iterators
     */
    static void demonstrateOutputIterators() {
        std::cout << "\n=== Output Iterator Demonstration ===\n";
        
        std::vector<std::string> ship_names{"Enterprise", "Voyager", "Defiant", "Discovery"};
        
        std::cout << "Ship names: ";
        for (const auto& name : ship_names) std::cout << name << " ";
        std::cout << "\n";
        
        // std::ostream_iterator - output to stream
        std::cout << "Using ostream_iterator: ";
        std::copy(ship_names.begin(), ship_names.end(),
                 std::ostream_iterator<std::string>(std::cout, " "));
        std::cout << "\n";
        
        // std::back_inserter - output iterator that calls push_back
        std::vector<std::string> more_ships;
        std::cout << "Using back_inserter to copy to new vector:\n";
        
        std::copy(ship_names.begin(), ship_names.end(),
                 std::back_inserter(more_ships));
        
        std::cout << "Copied ships: ";
        for (const auto& name : more_ships) std::cout << name << " ";
        std::cout << "\n";
        
        // std::front_inserter - calls push_front (requires front insertion capability)
        std::list<int> mission_ids{300, 400, 500};
        std::vector<int> new_ids{100, 200};
        
        std::cout << "Original mission IDs: ";
        for (int id : mission_ids) std::cout << id << " ";
        std::cout << "\n";
        
        std::copy(new_ids.begin(), new_ids.end(),
                 std::front_inserter(mission_ids));
        
        std::cout << "After front insertion: ";
        for (int id : mission_ids) std::cout << id << " ";
        std::cout << "\n";
        
        // std::inserter - insert at specific position
        std::vector<int> coordinates{10, 30, 50};
        std::vector<int> fill_coordinates{20, 40};
        
        std::cout << "Original coordinates: ";
        for (int coord : coordinates) std::cout << coord << " ";
        std::cout << "\n";
        
        auto insert_pos = coordinates.begin() + 1;
        std::copy(fill_coordinates.begin(), fill_coordinates.end(),
                 std::inserter(coordinates, insert_pos));
        
        std::cout << "After insertion at position 1: ";
        for (int coord : coordinates) std::cout << coord << " ";
        std::cout << "\n";
    }
    
    /**
     * @brief Demonstrate forward iterators
     */
    static void demonstrateForwardIterators() {
        std::cout << "\n=== Forward Iterator Demonstration ===\n";
        
        // Forward iterators can be incremented and support multi-pass algorithms
        std::forward_list<StarSystem> star_systems;
        star_systems.push_front({"Proxima Centauri", 2, 4.24, "Red Dwarf"});
        star_systems.push_front({"Alpha Centauri", 3, 4.37, "Binary"});
        star_systems.push_front({"Barnard's Star", 0, 5.96, "Red Dwarf"});
        star_systems.push_front({"Wolf 359", 0, 7.86, "Red Dwarf"});
        
        std::cout << "Star systems (using forward iterator):\n";
        for (auto it = star_systems.begin(); it != star_systems.end(); ++it) {
            std::cout << "- " << *it << "\n";
        }
        
        // Forward iterators support multi-pass - can iterate multiple times
        std::cout << "\nCounting systems with planets (first pass):\n";
        int systems_with_planets = 0;
        for (auto it = star_systems.begin(); it != star_systems.end(); ++it) {
            if (it->planet_count > 0) {
                systems_with_planets++;
            }
        }
        std::cout << "Systems with planets: " << systems_with_planets << "\n";
        
        std::cout << "\nFinding closest system (second pass):\n";
        auto closest_it = star_systems.begin();
        for (auto it = star_systems.begin(); it != star_systems.end(); ++it) {
            if (it->distance_ly < closest_it->distance_ly) {
                closest_it = it;
            }
        }
        std::cout << "Closest system: " << *closest_it << "\n";
        
        // Forward iterators can be copied and compared
        auto saved_position = star_systems.begin();
        ++saved_position; // Move to second element
        
        std::cout << "\nSaved position points to: " << *saved_position << "\n";
        
        // Algorithm that requires forward iterator (can't use input/output iterators)
        auto red_dwarf_count = std::count_if(star_systems.begin(), star_systems.end(),
            [](const StarSystem& sys) { return sys.classification == "Red Dwarf"; });
        
        std::cout << "Red dwarf systems: " << red_dwarf_count << "\n";
    }
    
    /**
     * @brief Demonstrate bidirectional iterators
     */
    static void demonstrateBidirectionalIterators() {
        std::cout << "\n=== Bidirectional Iterator Demonstration ===\n";
        
        // Bidirectional iterators can move both forward and backward
        std::list<std::string> patrol_route{"Earth", "Mars", "Jupiter", "Saturn", "Neptune"};
        
        std::cout << "Patrol route (forward): ";
        for (auto it = patrol_route.begin(); it != patrol_route.end(); ++it) {
            std::cout << *it << " -> ";
        }
        std::cout << "Return\n";
        
        // Moving backward using bidirectional iterator
        std::cout << "Return journey (backward): ";
        for (auto it = patrol_route.rbegin(); it != patrol_route.rend(); ++it) {
            std::cout << *it << " -> ";
        }
        std::cout << "Earth\n";
        
        // Direct backward iteration
        std::cout << "Manual backward iteration: ";
        auto it = patrol_route.end();
        while (it != patrol_route.begin()) {
            --it; // Bidirectional iterators support decrement
            std::cout << *it;
            if (it != patrol_route.begin()) std::cout << " <- ";
        }
        std::cout << "\n";
        
        // Bidirectional iterators with std::map
        std::map<std::string, int> fleet_sizes{
            {"Alpha Squadron", 12},
            {"Beta Fleet", 8}, 
            {"Gamma Wing", 6},
            {"Delta Force", 15}
        };
        
        std::cout << "\nFleet sizes (forward):\n";
        for (auto it = fleet_sizes.begin(); it != fleet_sizes.end(); ++it) {
            std::cout << "- " << it->first << ": " << it->second << " ships\n";
        }
        
        std::cout << "\nFleet sizes (reverse):\n";
        for (auto it = fleet_sizes.rbegin(); it != fleet_sizes.rend(); ++it) {
            std::cout << "- " << it->first << ": " << it->second << " ships\n";
        }
        
        // Algorithm that benefits from bidirectional iteration
        std::list<int> mission_priorities{5, 2, 8, 1, 9, 3, 7};
        std::cout << "\nOriginal priorities: ";
        for (int p : mission_priorities) std::cout << p << " ";
        std::cout << "\n";
        
        // std::reverse requires bidirectional iterators
        std::reverse(mission_priorities.begin(), mission_priorities.end());
        
        std::cout << "Reversed priorities: ";
        for (int p : mission_priorities) std::cout << p << " ";
        std::cout << "\n";
    }
    
    /**
     * @brief Demonstrate random access iterators
     */
    static void demonstrateRandomAccessIterators() {
        std::cout << "\n=== Random Access Iterator Demonstration ===\n";
        
        // Random access iterators support constant-time jumps to arbitrary positions
        std::vector<StarSystem> galaxy{
            {"Sol", 8, 0.0, "G-class"},
            {"Alpha Centauri", 3, 4.37, "Binary"},
            {"Sirius", 0, 8.6, "Binary"},
            {"Vega", 1, 25.0, "A-class"},
            {"Altair", 0, 16.7, "A-class"},
            {"Arcturus", 0, 36.7, "K-class"},
            {"Capella", 0, 42.9, "G-class"},
            {"Rigel", 0, 860.0, "Blue Supergiant"}
        };
        
        std::cout << "Galaxy database (" << galaxy.size() << " systems):\n";
        
        // Random access - jump directly to any position
        std::cout << "System at index 3: " << galaxy[3] << "\n";
        std::cout << "System at index 6: " << galaxy[6] << "\n";
        
        // Iterator arithmetic (only available with random access iterators)
        auto it = galaxy.begin();
        std::cout << "First system: " << *it << "\n";
        
        it += 3; // Jump forward by 3
        std::cout << "System at begin+3: " << *it << "\n";
        
        it -= 1; // Jump backward by 1
        std::cout << "System at (begin+3)-1: " << *it << "\n";
        
        // Distance calculation
        auto first_it = galaxy.begin();
        auto last_it = galaxy.end() - 1; // Last element
        auto distance = last_it - first_it;
        std::cout << "Distance from first to last: " << distance << "\n";
        
        // Comparison operators (beyond == and !=)
        auto mid_it = galaxy.begin() + galaxy.size() / 2;
        std::cout << "Middle system: " << *mid_it << "\n";
        
        std::cout << "first_it < mid_it: " << (first_it < mid_it) << "\n";
        std::cout << "mid_it > first_it: " << (mid_it > first_it) << "\n";
        
        // Random access enables efficient algorithms
        std::cout << "\nSorting by distance (requires random access):\n";
        std::sort(galaxy.begin(), galaxy.end(),
                 [](const StarSystem& a, const StarSystem& b) {
                     return a.distance_ly < b.distance_ly;
                 });
        
        for (size_t i = 0; i < galaxy.size(); ++i) {
            std::cout << i + 1 << ". " << galaxy[i] << "\n";
        }
        
        // Binary search (requires random access)
        std::cout << "\nSearching for systems within 10 light years:\n";
        auto close_systems_end = std::upper_bound(galaxy.begin(), galaxy.end(), 10.0,
            [](double distance, const StarSystem& sys) {
                return distance < sys.distance_ly;
            });
        
        std::cout << "Systems within 10 ly:\n";
        for (auto iter = galaxy.begin(); iter != close_systems_end; ++iter) {
            std::cout << "- " << *iter << "\n";
        }
    }
};

/**
 * @brief Iterator Adapter Demonstrations
 * 
 * Demonstrates iterator adapters that modify iterator behavior:
 * reverse_iterator, insert_iterator, stream_iterator.
 */
class IteratorAdapters {
public:
    /**
     * @brief Demonstrate reverse iterators
     */
    static void demonstrateReverseIterators() {
        std::cout << "\n=== Reverse Iterator Demonstration ===\n";
        
        std::vector<std::string> launch_sequence{
            "Engine Ignition", "Fuel Flow Check", "Navigation Lock", 
            "Communication Test", "Final Countdown", "Launch"
        };
        
        std::cout << "Launch sequence (normal order):\n";
        for (size_t i = 0; i < launch_sequence.size(); ++i) {
            std::cout << i + 1 << ". " << launch_sequence[i] << "\n";
        }
        
        // Using reverse iterators
        std::cout << "\nAbort sequence (reverse order):\n";
        int step = 1;
        for (auto rit = launch_sequence.rbegin(); rit != launch_sequence.rend(); ++rit) {
            std::cout << step++ << ". Abort " << *rit << "\n";
        }
        
        // Converting between normal and reverse iterators
        auto normal_it = launch_sequence.begin() + 3;
        std::cout << "\nNormal iterator at position 3: " << *normal_it << "\n";
        
        // Convert to reverse iterator (points to previous element)
        auto reverse_it = std::make_reverse_iterator(normal_it);
        std::cout << "Corresponding reverse iterator: " << *reverse_it << "\n";
        
        // Convert back to normal iterator
        auto back_to_normal = reverse_it.base();
        std::cout << "Back to normal iterator: " << *back_to_normal << "\n";
        
        // Reverse iterators with algorithms
        std::vector<int> numbers{1, 2, 3, 4, 5, 6, 7, 8, 9};
        
        std::cout << "\nOriginal numbers: ";
        for (int n : numbers) std::cout << n << " ";
        std::cout << "\n";
        
        // Copy last 5 elements in reverse order
        std::vector<int> last_five_reversed;
        std::copy(numbers.rbegin(), numbers.rbegin() + 5,
                 std::back_inserter(last_five_reversed));
        
        std::cout << "Last 5 elements (reversed): ";
        for (int n : last_five_reversed) std::cout << n << " ";
        std::cout << "\n";
    }
    
    /**
     * @brief Demonstrate move iterators
     */
    static void demonstrateMoveIterators() {
        std::cout << "\n=== Move Iterator Demonstration ===\n";
        
        // Move iterators for efficient transfer of resources
        std::vector<std::unique_ptr<std::string>> source_data;
        source_data.push_back(std::make_unique<std::string>("Ship Alpha"));
        source_data.push_back(std::make_unique<std::string>("Ship Beta"));
        source_data.push_back(std::make_unique<std::string>("Ship Gamma"));
        source_data.push_back(std::make_unique<std::string>("Ship Delta"));
        
        std::cout << "Source data before move:\n";
        for (size_t i = 0; i < source_data.size(); ++i) {
            if (source_data[i]) {
                std::cout << "- " << *source_data[i] << "\n";
            } else {
                std::cout << "- [moved]\n";
            }
        }
        
        // Move elements to destination using move iterators
        std::vector<std::unique_ptr<std::string>> destination_data;
        
        // std::make_move_iterator creates move iterators
        std::copy(std::make_move_iterator(source_data.begin()),
                 std::make_move_iterator(source_data.end()),
                 std::back_inserter(destination_data));
        
        std::cout << "\nAfter move operation:\n";
        std::cout << "Source data:\n";
        for (size_t i = 0; i < source_data.size(); ++i) {
            if (source_data[i]) {
                std::cout << "- " << *source_data[i] << "\n";
            } else {
                std::cout << "- [moved]\n";
            }
        }
        
        std::cout << "Destination data:\n";
        for (size_t i = 0; i < destination_data.size(); ++i) {
            if (destination_data[i]) {
                std::cout << "- " << *destination_data[i] << "\n";
            } else {
                std::cout << "- [null]\n";
            }
        }
        
        // Move semantics with regular objects
        std::vector<std::string> ship_names{"Enterprise", "Voyager", "Defiant"};
        std::vector<std::string> transferred_names;
        
        std::cout << "\nMoving ship names:\n";
        std::cout << "Original names: ";
        for (const auto& name : ship_names) std::cout << name << " ";
        std::cout << "\n";
        
        // Move using move iterators
        std::move(std::make_move_iterator(ship_names.begin()),
                 std::make_move_iterator(ship_names.end()),
                 std::back_inserter(transferred_names));
        
        std::cout << "After move:\n";
        std::cout << "Original (moved-from): ";
        for (const auto& name : ship_names) std::cout << "'" << name << "' ";
        std::cout << "\nTransferred: ";
        for (const auto& name : transferred_names) std::cout << name << " ";
        std::cout << "\n";
    }
};

/**
 * @brief Custom Iterator Implementation
 * 
 * Demonstrates how to create custom iterators that work with STL algorithms.
 */
template<typename T>
class SimpleVector {
private:
    std::unique_ptr<T[]> data_;
    size_t size_;
    size_t capacity_;

public:
    explicit SimpleVector(size_t initial_capacity = 10) 
        : data_(std::make_unique<T[]>(initial_capacity)), size_(0), capacity_(initial_capacity) {}
    
    void push_back(const T& value) {
        if (size_ >= capacity_) {
            resize();
        }
        data_[size_++] = value;
    }
    
    size_t size() const { return size_; }
    
    T& operator[](size_t index) { return data_[index]; }
    const T& operator[](size_t index) const { return data_[index]; }
    
private:
    void resize() {
        capacity_ *= 2;
        auto new_data = std::make_unique<T[]>(capacity_);
        for (size_t i = 0; i < size_; ++i) {
            new_data[i] = std::move(data_[i]);
        }
        data_ = std::move(new_data);
    }

public:
    /**
     * @brief Custom iterator class
     */
    class iterator {
    public:
        // Iterator traits (required for STL compatibility)
        using iterator_category = std::random_access_iterator_tag;
        using value_type = T;
        using difference_type = std::ptrdiff_t;
        using pointer = T*;
        using reference = T&;
        
        // Constructors
        iterator() : ptr_(nullptr) {}
        explicit iterator(pointer ptr) : ptr_(ptr) {}
        
        // Dereference operators
        reference operator*() const { return *ptr_; }
        pointer operator->() const { return ptr_; }
        
        // Increment operators
        iterator& operator++() { ++ptr_; return *this; }
        iterator operator++(int) { iterator tmp = *this; ++ptr_; return tmp; }
        
        // Decrement operators (bidirectional)
        iterator& operator--() { --ptr_; return *this; }
        iterator operator--(int) { iterator tmp = *this; --ptr_; return tmp; }
        
        // Arithmetic operators (random access)
        iterator& operator+=(difference_type n) { ptr_ += n; return *this; }
        iterator& operator-=(difference_type n) { ptr_ -= n; return *this; }
        
        iterator operator+(difference_type n) const { return iterator(ptr_ + n); }
        iterator operator-(difference_type n) const { return iterator(ptr_ - n); }
        
        difference_type operator-(const iterator& other) const { return ptr_ - other.ptr_; }
        
        // Subscript operator
        reference operator[](difference_type n) const { return ptr_[n]; }
        
        // Comparison operators
        bool operator==(const iterator& other) const { return ptr_ == other.ptr_; }
        bool operator!=(const iterator& other) const { return ptr_ != other.ptr_; }
        bool operator<(const iterator& other) const { return ptr_ < other.ptr_; }
        bool operator>(const iterator& other) const { return ptr_ > other.ptr_; }
        bool operator<=(const iterator& other) const { return ptr_ <= other.ptr_; }
        bool operator>=(const iterator& other) const { return ptr_ >= other.ptr_; }
        
    private:
        pointer ptr_;
    };
    
    /**
     * @brief Const iterator class
     */
    class const_iterator {
    public:
        using iterator_category = std::random_access_iterator_tag;
        using value_type = T;
        using difference_type = std::ptrdiff_t;
        using pointer = const T*;
        using reference = const T&;
        
        const_iterator() : ptr_(nullptr) {}
        explicit const_iterator(pointer ptr) : ptr_(ptr) {}
        const_iterator(const iterator& it) : ptr_(&(*it)) {}
        
        reference operator*() const { return *ptr_; }
        pointer operator->() const { return ptr_; }
        
        const_iterator& operator++() { ++ptr_; return *this; }
        const_iterator operator++(int) { const_iterator tmp = *this; ++ptr_; return tmp; }
        
        const_iterator& operator--() { --ptr_; return *this; }
        const_iterator operator--(int) { const_iterator tmp = *this; --ptr_; return tmp; }
        
        const_iterator& operator+=(difference_type n) { ptr_ += n; return *this; }
        const_iterator& operator-=(difference_type n) { ptr_ -= n; return *this; }
        
        const_iterator operator+(difference_type n) const { return const_iterator(ptr_ + n); }
        const_iterator operator-(difference_type n) const { return const_iterator(ptr_ - n); }
        
        difference_type operator-(const const_iterator& other) const { return ptr_ - other.ptr_; }
        
        reference operator[](difference_type n) const { return ptr_[n]; }
        
        bool operator==(const const_iterator& other) const { return ptr_ == other.ptr_; }
        bool operator!=(const const_iterator& other) const { return ptr_ != other.ptr_; }
        bool operator<(const const_iterator& other) const { return ptr_ < other.ptr_; }
        bool operator>(const const_iterator& other) const { return ptr_ > other.ptr_; }
        bool operator<=(const const_iterator& other) const { return ptr_ <= other.ptr_; }
        bool operator>=(const const_iterator& other) const { return ptr_ >= other.ptr_; }
        
    private:
        pointer ptr_;
    };
    
    // Iterator access methods
    iterator begin() { return iterator(data_.get()); }
    iterator end() { return iterator(data_.get() + size_); }
    
    const_iterator begin() const { return const_iterator(data_.get()); }
    const_iterator end() const { return const_iterator(data_.get() + size_); }
    
    const_iterator cbegin() const { return const_iterator(data_.get()); }
    const_iterator cend() const { return const_iterator(data_.get() + size_); }
};

/**
 * @brief Range-based Custom Iterator
 * 
 * A custom iterator that can filter elements based on a predicate.
 */
template<typename Iterator, typename Predicate>
class FilterIterator {
public:
    using iterator_category = std::forward_iterator_tag;
    using value_type = typename std::iterator_traits<Iterator>::value_type;
    using difference_type = typename std::iterator_traits<Iterator>::difference_type;
    using pointer = typename std::iterator_traits<Iterator>::pointer;
    using reference = typename std::iterator_traits<Iterator>::reference;
    
    FilterIterator(Iterator current, Iterator end, Predicate pred)
        : current_(current), end_(end), predicate_(pred) {
        // Move to first valid element
        while (current_ != end_ && !predicate_(*current_)) {
            ++current_;
        }
    }
    
    FilterIterator(Iterator end) : current_(end), end_(end) {}
    
    reference operator*() const { return *current_; }
    pointer operator->() const { return current_.operator->(); }
    
    FilterIterator& operator++() {
        ++current_;
        while (current_ != end_ && !predicate_(*current_)) {
            ++current_;
        }
        return *this;
    }
    
    FilterIterator operator++(int) {
        FilterIterator tmp = *this;
        ++(*this);
        return tmp;
    }
    
    bool operator==(const FilterIterator& other) const {
        return current_ == other.current_;
    }
    
    bool operator!=(const FilterIterator& other) const {
        return current_ != other.current_;
    }

private:
    Iterator current_;
    Iterator end_;
    Predicate predicate_;
};

/**
 * @brief Helper function to create filter iterators
 */
template<typename Iterator, typename Predicate>
FilterIterator<Iterator, Predicate> make_filter_iterator(Iterator current, Iterator end, Predicate pred) {
    return FilterIterator<Iterator, Predicate>(current, end, pred);
}

/**
 * @brief Custom Iterator Demonstrations
 * 
 * Demonstrates custom iterator implementations and their usage with STL algorithms.
 */
class CustomIterators {
public:
    /**
     * @brief Demonstrate custom vector iterator
     */
    static void demonstrateCustomVector() {
        std::cout << "\n=== Custom Vector Iterator Demonstration ===\n";
        
        SimpleVector<std::string> fleet_registry;
        fleet_registry.push_back("USS Enterprise");
        fleet_registry.push_back("USS Voyager");
        fleet_registry.push_back("USS Defiant");
        fleet_registry.push_back("USS Discovery");
        fleet_registry.push_back("USS Constitution");
        
        std::cout << "Fleet Registry contents:\n";
        
        // Using custom iterator with range-based for loop
        for (const auto& ship : fleet_registry) {
            std::cout << "- " << ship << "\n";
        }
        
        // Using custom iterator with STL algorithms
        std::cout << "\nUsing custom iterator with STL algorithms:\n";
        
        // std::find
        auto it = std::find(fleet_registry.begin(), fleet_registry.end(), "USS Voyager");
        if (it != fleet_registry.end()) {
            std::cout << "Found: " << *it << "\n";
        }
        
        // std::count_if
        auto uss_count = std::count_if(fleet_registry.begin(), fleet_registry.end(),
            [](const std::string& ship) { return ship.find("USS") == 0; });
        std::cout << "Ships with USS prefix: " << uss_count << "\n";
        
        // std::sort
        std::sort(fleet_registry.begin(), fleet_registry.end());
        std::cout << "After sorting:\n";
        for (const auto& ship : fleet_registry) {
            std::cout << "- " << ship << "\n";
        }
        
        // Random access iterator capabilities
        std::cout << "\nRandom access operations:\n";
        auto first_it = fleet_registry.begin();
        auto third_it = first_it + 2;
        std::cout << "Third ship: " << *third_it << "\n";
        
        auto distance = fleet_registry.end() - fleet_registry.begin();
        std::cout << "Total ships: " << distance << "\n";
        
        // Iterator comparison
        std::cout << "first_it < third_it: " << (first_it < third_it) << "\n";
    }
    
    /**
     * @brief Demonstrate filter iterator
     */
    static void demonstrateFilterIterator() {
        std::cout << "\n=== Filter Iterator Demonstration ===\n";
        
        std::vector<StarSystem> star_systems{
            {"Sol", 8, 0.0, "G-class"},
            {"Proxima Centauri", 2, 4.24, "Red Dwarf"},
            {"Alpha Centauri", 3, 4.37, "Binary"},
            {"Barnard's Star", 0, 5.96, "Red Dwarf"},
            {"Wolf 359", 0, 7.86, "Red Dwarf"},
            {"Sirius", 0, 8.6, "Binary"},
            {"Vega", 1, 25.0, "A-class"}
        };
        
        std::cout << "All star systems:\n";
        for (const auto& sys : star_systems) {
            std::cout << "- " << sys << "\n";
        }
        
        // Filter for systems with planets
        auto has_planets = [](const StarSystem& sys) { return sys.planet_count > 0; };
        
        std::cout << "\nSystems with planets (using filter iterator):\n";
        auto filter_begin = make_filter_iterator(star_systems.begin(), star_systems.end(), has_planets);
        auto filter_end = FilterIterator<std::vector<StarSystem>::iterator, decltype(has_planets)>(star_systems.end());
        
        for (auto it = filter_begin; it != filter_end; ++it) {
            std::cout << "- " << *it << "\n";
        }
        
        // Filter for nearby systems (< 10 light years)
        auto is_nearby = [](const StarSystem& sys) { return sys.distance_ly < 10.0; };
        
        std::cout << "\nNearby systems (< 10 ly):\n";
        auto nearby_begin = make_filter_iterator(star_systems.begin(), star_systems.end(), is_nearby);
        auto nearby_end = FilterIterator<std::vector<StarSystem>::iterator, decltype(is_nearby)>(star_systems.end());
        
        for (auto it = nearby_begin; it != nearby_end; ++it) {
            std::cout << "- " << *it << "\n";
        }
        
        // Count filtered elements
        int nearby_count = 0;
        for (auto it = nearby_begin; it != nearby_end; ++it) {
            nearby_count++;
        }
        std::cout << "Total nearby systems: " << nearby_count << "\n";
        
        // Filter for Red Dwarf stars
        auto is_red_dwarf = [](const StarSystem& sys) { return sys.classification == "Red Dwarf"; };
        
        std::cout << "\nRed Dwarf systems:\n";
        auto red_dwarf_begin = make_filter_iterator(star_systems.begin(), star_systems.end(), is_red_dwarf);
        auto red_dwarf_end = FilterIterator<std::vector<StarSystem>::iterator, decltype(is_red_dwarf)>(star_systems.end());
        
        for (auto it = red_dwarf_begin; it != red_dwarf_end; ++it) {
            std::cout << "- " << *it << "\n";
        }
    }
};

/**
 * @brief Iterator Utilities and Helpers
 * 
 * Demonstrates utility functions for working with iterators.
 */
class IteratorUtilities {
public:
    /**
     * @brief Demonstrate std::advance and std::distance
     */
    static void demonstrateAdvanceDistance() {
        std::cout << "\n=== Iterator Advance and Distance ===\n";
        
        std::vector<int> coordinates{10, 20, 30, 40, 50, 60, 70, 80, 90, 100};
        
        std::cout << "Coordinates: ";
        for (int coord : coordinates) std::cout << coord << " ";
        std::cout << "\n";
        
        // std::advance - move iterator by n positions
        auto it = coordinates.begin();
        std::cout << "Iterator starts at: " << *it << "\n";
        
        std::advance(it, 3);
        std::cout << "After advance(3): " << *it << "\n";
        
        std::advance(it, -1); // Can move backward with bidirectional/random access iterators
        std::cout << "After advance(-1): " << *it << "\n";
        
        // std::distance - calculate distance between iterators
        auto first_it = coordinates.begin();
        auto last_it = coordinates.end();
        
        auto total_distance = std::distance(first_it, last_it);
        std::cout << "Distance from begin to end: " << total_distance << "\n";
        
        auto mid_it = coordinates.begin() + coordinates.size() / 2;
        auto first_half_distance = std::distance(first_it, mid_it);
        auto second_half_distance = std::distance(mid_it, last_it);
        
        std::cout << "Distance to middle: " << first_half_distance << "\n";
        std::cout << "Distance from middle to end: " << second_half_distance << "\n";
        
        // Using advance and distance with different iterator types
        std::list<std::string> mission_log{"Start", "Waypoint1", "Waypoint2", "Waypoint3", "End"};
        
        auto log_it = mission_log.begin();
        std::cout << "\nMission log navigation:\n";
        std::cout << "Current: " << *log_it << "\n";
        
        std::advance(log_it, 2);
        std::cout << "After advance(2): " << *log_it << "\n";
        
        auto log_distance = std::distance(mission_log.begin(), log_it);
        std::cout << "Distance from start: " << log_distance << "\n";
    }
    
    /**
     * @brief Demonstrate std::next and std::prev
     */
    static void demonstrateNextPrev() {
        std::cout << "\n=== Iterator Next and Prev ===\n";
        
        std::vector<std::string> fleet_formation{"Alpha", "Beta", "Gamma", "Delta", "Epsilon"};
        
        std::cout << "Fleet formation: ";
        for (const auto& ship : fleet_formation) std::cout << ship << " ";
        std::cout << "\n";
        
        auto current_it = fleet_formation.begin() + 2; // Point to "Gamma"
        std::cout << "Current position: " << *current_it << "\n";
        
        // std::next - get iterator n positions ahead (doesn't modify original)
        auto next_it = std::next(current_it);
        std::cout << "Next ship: " << *next_it << "\n";
        std::cout << "Current still points to: " << *current_it << "\n";
        
        auto next2_it = std::next(current_it, 2);
        std::cout << "Ship 2 positions ahead: " << *next2_it << "\n";
        
        // std::prev - get iterator n positions back (doesn't modify original)
        auto prev_it = std::prev(current_it);
        std::cout << "Previous ship: " << *prev_it << "\n";
        
        auto prev2_it = std::prev(current_it, 2);
        std::cout << "Ship 2 positions back: " << *prev2_it << "\n";
        
        // Safe bounds checking
        if (std::next(current_it, 3) != fleet_formation.end()) {
            std::cout << "Ship 3 positions ahead: " << *std::next(current_it, 3) << "\n";
        } else {
            std::cout << "No ship 3 positions ahead (would be past end)\n";
        }
        
        // Working with different container types
        std::list<int> priority_queue{5, 10, 15, 20, 25};
        auto pq_it = priority_queue.begin();
        std::advance(pq_it, 2); // Move to middle
        
        std::cout << "\nPriority queue navigation:\n";
        std::cout << "Current priority: " << *pq_it << "\n";
        std::cout << "Next priority: " << *std::next(pq_it) << "\n";
        std::cout << "Previous priority: " << *std::prev(pq_it) << "\n";
    }
    
    /**
     * @brief Demonstrate iterator invalidation scenarios
     */
    static void demonstrateIteratorInvalidation() {
        std::cout << "\n=== Iterator Invalidation Scenarios ===\n";
        
        std::vector<int> mission_priorities{1, 2, 3, 4, 5};
        
        std::cout << "Original priorities: ";
        for (int p : mission_priorities) std::cout << p << " ";
        std::cout << "\n";
        
        // Safe iterator usage
        auto it = mission_priorities.begin() + 2;
        std::cout << "Iterator points to: " << *it << "\n";
        
        // Modifying element is safe
        *it = 99;
        std::cout << "After modification: " << *it << "\n";
        
        std::cout << "Modified vector: ";
        for (int p : mission_priorities) std::cout << p << " ";
        std::cout << "\n";
        
        // WARNING: These operations can invalidate iterators
        std::cout << "\nDemonstrating iterator invalidation scenarios:\n";
        
        // 1. Vector reallocation can invalidate all iterators
        std::vector<int> growing_vector{1, 2, 3};
        auto growing_it = growing_vector.begin() + 1;
        std::cout << "Before reallocation: " << *growing_it << "\n";
        
        // This might cause reallocation and invalidate growing_it
        for (int i = 4; i <= 10; ++i) {
            growing_vector.push_back(i);
        }
        
        // DANGER: growing_it might be invalid now!
        std::cout << "After reallocation, iterator may be invalid\n";
        std::cout << "Safe approach: Use indices or reacquire iterators\n";
        
        // 2. Insertion can invalidate iterators
        std::vector<std::string> ship_names{"Alpha", "Beta", "Gamma"};
        auto name_it = ship_names.begin() + 1;
        std::cout << "Iterator before insertion: " << *name_it << "\n";
        
        ship_names.insert(ship_names.begin(), "Zulu"); // Insert at beginning
        // name_it is now potentially invalid or points to wrong element
        
        std::cout << "After insertion, reacquire iterator:\n";
        name_it = ship_names.begin() + 2; // Reacquire - now points to "Beta" again
        std::cout << "Reacquired iterator: " << *name_it << "\n";
        
        // 3. Erasure invalidates iterators
        std::list<int> numbers{10, 20, 30, 40, 50};
        auto num_it = numbers.begin();
        std::advance(num_it, 2); // Point to 30
        
        std::cout << "Before erasure: " << *num_it << "\n";
        
        auto next_valid_it = numbers.erase(num_it); // Erase 30, get iterator to next element
        std::cout << "After erasure, next valid element: " << *next_valid_it << "\n";
        
        // Best practices
        std::cout << "\nBest practices for iterator safety:\n";
        std::cout << "1. Reacquire iterators after container modifications\n";
        std::cout << "2. Use indices when possible for random access containers\n";
        std::cout << "3. Use iterator return values from erase operations\n";
        std::cout << "4. Prefer algorithms over manual iterator manipulation\n";
    }
};

/**
 * @brief Performance comparison of different iterator types
 */
class IteratorPerformanceComparison {
public:
    /**
     * @brief Compare iteration performance across container types
     */
    static void compareIterationPerformance() {
        std::cout << "\n=== Iterator Performance Comparison ===\n";
        
        const size_t test_size = 1000000;
        
        // Vector with random access iterator
        std::vector<int> vec_data;
        vec_data.reserve(test_size);
        for (size_t i = 0; i < test_size; ++i) {
            vec_data.push_back(static_cast<int>(i));
        }
        
        // List with bidirectional iterator
        std::list<int> list_data;
        for (size_t i = 0; i < test_size; ++i) {
            list_data.push_back(static_cast<int>(i));
        }
        
        // Test sequential access
        auto start = std::chrono::high_resolution_clock::now();
        long long sum = 0;
        for (auto it = vec_data.begin(); it != vec_data.end(); ++it) {
            sum += *it;
        }
        auto end = std::chrono::high_resolution_clock::now();
        auto vector_seq_time = std::chrono::duration<double, std::milli>(end - start).count();
        
        start = std::chrono::high_resolution_clock::now();
        sum = 0;
        for (auto it = list_data.begin(); it != list_data.end(); ++it) {
            sum += *it;
        }
        end = std::chrono::high_resolution_clock::now();
        auto list_seq_time = std::chrono::duration<double, std::milli>(end - start).count();
        
        std::cout << "Sequential iteration (" << test_size << " elements):\n";
        std::cout << "Vector (random access iterator): " << vector_seq_time << " ms\n";
        std::cout << "List (bidirectional iterator): " << list_seq_time << " ms\n";
        
        // Test random access (only possible with vector)
        start = std::chrono::high_resolution_clock::now();
        sum = 0;
        for (size_t i = 0; i < test_size; i += 100) { // Every 100th element
            sum += vec_data[i];
        }
        end = std::chrono::high_resolution_clock::now();
        auto vector_random_time = std::chrono::duration<double, std::milli>(end - start).count();
        
        // Simulate random access on list (very inefficient)
        start = std::chrono::high_resolution_clock::now();
        sum = 0;
        for (size_t i = 0; i < test_size; i += 100) {
            auto it = list_data.begin();
            std::advance(it, i);
            sum += *it;
        }
        end = std::chrono::high_resolution_clock::now();
        auto list_random_time = std::chrono::duration<double, std::milli>(end - start).count();
        
        std::cout << "\nRandom access (every 100th element):\n";
        std::cout << "Vector (direct indexing): " << vector_random_time << " ms\n";
        std::cout << "List (advance iterator): " << list_random_time << " ms\n";
        std::cout << "List random access penalty: " << (list_random_time / vector_random_time) << "x slower\n";
        
        std::cout << "\nKey takeaways:\n";
        std::cout << "- Use vector for frequent random access\n";
        std::cout << "- Use list for frequent insertion/deletion in middle\n";
        std::cout << "- Iterator category affects algorithm performance\n";
        std::cout << "- Choose container based on access patterns\n";
    }
};

} // namespace CppVerseHub::STL