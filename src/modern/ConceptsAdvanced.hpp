// File: src/modern/ConceptsAdvanced.hpp
// Advanced C++20 Concepts Demonstrations

#pragma once

#include <concepts>
#include <type_traits>
#include <iterator>
#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <iostream>

namespace CppVerseHub::Modern::Concepts {

// ===== BASIC CONCEPTS =====

template<typename T>
concept Numeric = std::integral<T> || std::floating_point<T>;

template<typename T>
concept Printable = requires(T t, std::ostream& os) {
    os << t;
};

template<typename T>
concept Comparable = requires(T a, T b) {
    { a == b } -> std::convertible_to<bool>;
    { a != b } -> std::convertible_to<bool>;
    { a < b } -> std::convertible_to<bool>;
    { a > b } -> std::convertible_to<bool>;
    { a <= b } -> std::convertible_to<bool>;
    { a >= b } -> std::convertible_to<bool>;
};

// ===== CONTAINER CONCEPTS =====

template<typename T>
concept Container = requires(T t) {
    typename T::value_type;
    typename T::iterator;
    typename T::const_iterator;
    typename T::size_type;
    
    { t.begin() } -> std::convertible_to<typename T::iterator>;
    { t.end() } -> std::convertible_to<typename T::iterator>;
    { t.size() } -> std::convertible_to<typename T::size_type>;
    { t.empty() } -> std::convertible_to<bool>;
};

template<typename T>
concept Iterable = requires(T t) {
    { t.begin() } -> std::input_or_output_iterator;
    { t.end() } -> std::sentinel_for<decltype(t.begin())>;
};

template<typename T>
concept RandomAccessContainer = Container<T> && requires(T t, typename T::size_type n) {
    { t[n] } -> std::convertible_to<typename T::value_type&>;
    { t.at(n) } -> std::convertible_to<typename T::value_type&>;
};

// ===== CALLABLE CONCEPTS =====

template<typename F, typename... Args>
concept Invocable = std::invocable<F, Args...>;

template<typename F, typename R, typename... Args>
concept InvocableWithReturn = std::invocable<F, Args...> && 
    std::convertible_to<std::invoke_result_t<F, Args...>, R>;

template<typename Pred, typename T>
concept Predicate = std::predicate<Pred, T>;

template<typename F, typename T>
concept UnaryFunction = requires(F f, T t) {
    { f(t) } -> std::convertible_to<T>;
};

template<typename F, typename T>
concept BinaryFunction = requires(F f, T a, T b) {
    { f(a, b) } -> std::convertible_to<T>;
};

// ===== SPACE GAME SPECIFIC CONCEPTS =====

template<typename T>
concept Entity = requires(T t) {
    typename T::EntityType;
    { t.getId() } -> std::convertible_to<int>;
    { t.getName() } -> std::convertible_to<std::string>;
    { t.isActive() } -> std::convertible_to<bool>;
};

template<typename T>
concept Positionable = requires(T t) {
    { t.getX() } -> std::floating_point;
    { t.getY() } -> std::floating_point;
    { t.getZ() } -> std::floating_point;
    { t.setPosition(0.0, 0.0, 0.0) } -> std::same_as<void>;
};

template<typename T>
concept Movable = Positionable<T> && requires(T t) {
    { t.getVelocityX() } -> std::floating_point;
    { t.getVelocityY() } -> std::floating_point;
    { t.getVelocityZ() } -> std::floating_point;
    { t.move(0.0) } -> std::same_as<void>;  // delta time
};

template<typename T>
concept Resource = requires(T t) {
    { t.getAmount() } -> Numeric;
    { t.getType() } -> std::convertible_to<std::string>;
    { t.isRenewable() } -> std::convertible_to<bool>;
};

template<typename T>
concept Mission = Entity<T> && requires(T t) {
    { t.execute() } -> std::same_as<void>;
    { t.getProgress() } -> std::floating_point;
    { t.isComplete() } -> std::convertible_to<bool>;
    { t.getDuration() } -> Numeric;
};

// ===== ADVANCED COMPOSITE CONCEPTS =====

template<typename T>
concept SpaceEntity = Entity<T> && Positionable<T>;

template<typename T>
concept MoveableSpaceEntity = SpaceEntity<T> && Movable<T>;

template<typename T>
concept ResourceContainer = Container<T> && requires(T t) {
    requires Resource<typename T::value_type>;
};

// ===== CONCEPT UTILITIES =====

template<typename T>
struct is_container : std::false_type {};

template<Container T>
struct is_container<T> : std::true_type {};

template<typename T>
constexpr bool is_container_v = is_container<T>::value;

// ===== CONSTRAINED TEMPLATE FUNCTIONS =====

template<Numeric T>
constexpr T add(T a, T b) {
    return a + b;
}

template<Comparable T>
constexpr T max(T a, T b) {
    return (a > b) ? a : b;
}

template<Container C>
void print_container(const C& container) {
    std::cout << "Container contents: ";
    for (const auto& item : container) {
        std::cout << item << " ";
    }
    std::cout << std::endl;
}

template<Iterable Range, UnaryFunction<typename Range::value_type> Func>
void transform_range(Range& range, Func func) {
    for (auto& item : range) {
        item = func(item);
    }
}

// ===== ADVANCED CONSTRAINT COMBINATIONS =====

template<typename T>
concept NumericContainer = Container<T> && Numeric<typename T::value_type>;

template<NumericContainer C>
auto sum_container(const C& container) {
    using ValueType = typename C::value_type;
    ValueType sum{};
    for (const auto& item : container) {
        sum += item;
    }
    return sum;
}

template<typename T>
concept SerializableEntity = Entity<T> && requires(T t) {
    { t.serialize() } -> std::convertible_to<std::string>;
    { T::deserialize(std::string{}) } -> std::convertible_to<T>;
};

// ===== CONCEPT DEBUGGING UTILITIES =====

template<typename T>
void concept_info() {
    std::cout << "Type analysis for: " << typeid(T).name() << std::endl;
    std::cout << "Is Numeric: " << Numeric<T> << std::endl;
    std::cout << "Is Printable: " << Printable<T> << std::endl;
    std::cout << "Is Comparable: " << Comparable<T> << std::endl;
    std::cout << "Is Container: " << Container<T> << std::endl;
    std::cout << "Is Iterable: " << Iterable<T> << std::endl;
}

// ===== CONSTRAINED ALIASES =====

template<Numeric T>
using NumericVector = std::vector<T>;

template<Entity T>
using EntityPtr = std::unique_ptr<T>;

template<Mission T>
using MissionContainer = std::vector<std::unique_ptr<T>>;

// ===== CONCEPT-BASED FACTORY =====

template<typename T>
concept Constructible = std::default_constructible<T>;

template<Constructible T>
class ConceptFactory {
public:
    template<typename... Args>
    static T create(Args&&... args) requires std::constructible_from<T, Args...> {
        return T{std::forward<Args>(args)...};
    }
    
    static std::unique_ptr<T> create_unique() {
        return std::make_unique<T>();
    }
    
    static std::shared_ptr<T> create_shared() {
        return std::make_shared<T>();
    }
};

// ===== DEMO CLASSES FOR TESTING =====

class DemoEntity {
public:
    using EntityType = int;
    
    int getId() const { return id_; }
    std::string getName() const { return name_; }
    bool isActive() const { return active_; }
    
    double getX() const { return x_; }
    double getY() const { return y_; }
    double getZ() const { return z_; }
    void setPosition(double x, double y, double z) { 
        x_ = x; y_ = y; z_ = z; 
    }
    
private:
    int id_ = 1;
    std::string name_ = "DemoEntity";
    bool active_ = true;
    double x_ = 0.0, y_ = 0.0, z_ = 0.0;
};

class DemoResource {
public:
    int getAmount() const { return amount_; }
    std::string getType() const { return type_; }
    bool isRenewable() const { return renewable_; }
    
private:
    int amount_ = 100;
    std::string type_ = "Energy";
    bool renewable_ = true;
};

// ===== CONCEPT USAGE EXAMPLES =====

void demonstrate_concepts() {
    std::cout << "\n=== C++20 Concepts Demonstration ===" << std::endl;
    
    // Basic concept usage
    std::cout << "\nTesting basic concepts:" << std::endl;
    concept_info<int>();
    concept_info<std::string>();
    concept_info<std::vector<int>>();
    
    // Constrained function usage
    std::cout << "\nConstrained functions:" << std::endl;
    std::cout << "add(5, 3) = " << add(5, 3) << std::endl;
    std::cout << "max(10.5, 7.2) = " << max(10.5, 7.2) << std::endl;
    
    // Container concepts
    std::vector<int> numbers = {1, 2, 3, 4, 5};
    print_container(numbers);
    
    auto total = sum_container(numbers);
    std::cout << "Sum of container: " << total << std::endl;
    
    // Entity concepts
    DemoEntity entity;
    std::cout << "\nEntity ID: " << entity.getId() << std::endl;
    std::cout << "Entity Name: " << entity.getName() << std::endl;
    std::cout << "Entity Active: " << entity.isActive() << std::endl;
    
    // Concept-based factory
    auto factory_entity = ConceptFactory<DemoEntity>::create();
    auto unique_entity = ConceptFactory<DemoEntity>::create_unique();
    
    std::cout << "Factory-created entity ID: " << factory_entity.getId() << std::endl;
}

} // namespace CppVerseHub::Modern::Concepts