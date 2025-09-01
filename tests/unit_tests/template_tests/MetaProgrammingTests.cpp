// File: tests/unit_tests/template_tests/MetaProgrammingTests.cpp
// Compile-time tests for CppVerseHub template metaprogramming showcase

#include <catch2/catch.hpp>
#include <type_traits>
#include <string>
#include <vector>
#include <array>
#include <tuple>
#include <chrono>

// Include the metaprogramming headers
#include "MetaProgramming.hpp"
#include "Planet.hpp"
#include "Fleet.hpp"
#include "Entity.hpp"

using namespace CppVerseHub::Templates;
using namespace CppVerseHub::Core;

// File: tests/unit_tests/template_tests/MetaProgrammingTests.cpp
// Compile-time tests for CppVerseHub template metaprogramming showcase

#include <catch2/catch.hpp>
#include <type_traits>
#include <string>
#include <vector>
#include <array>
#include <tuple>
#include <chrono>

// Include the metaprogramming headers
#include "MetaProgramming.hpp"
#include "Planet.hpp"
#include "Fleet.hpp"
#include "Entity.hpp"

using namespace CppVerseHub::Templates;
using namespace CppVerseHub::Core;

TEST_CASE("Compile-time Arithmetic", "[metaprogramming][compile-time][arithmetic]") {
    
    SECTION("Template recursion for factorial") {
        template<int N>
        struct Factorial {
            static constexpr int value = N * Factorial<N - 1>::value;
        };
        
        template<>
        struct Factorial<0> {
            static constexpr int value = 1;
        };
        
        // Compile-time tests
        static_assert(Factorial<0>::value == 1);
        static_assert(Factorial<1>::value == 1);
        static_assert(Factorial<5>::value == 120);
        static_assert(Factorial<10>::value == 3628800);
        
        // Runtime verification
        REQUIRE(Factorial<5>::value == 120);
        REQUIRE(Factorial<10>::value == 3628800);
    }
    
    SECTION("Fibonacci sequence at compile-time") {
        template<int N>
        struct Fibonacci {
            static constexpr int value = Fibonacci<N - 1>::value + Fibonacci<N - 2>::value;
        };
        
        template<>
        struct Fibonacci<0> {
            static constexpr int value = 0;
        };
        
        template<>
        struct Fibonacci<1> {
            static constexpr int value = 1;
        };
        
        static_assert(Fibonacci<0>::value == 0);
        static_assert(Fibonacci<1>::value == 1);
        static_assert(Fibonacci<2>::value == 1);
        static_assert(Fibonacci<3>::value == 2);
        static_assert(Fibonacci<10>::value == 55);
        
        REQUIRE(Fibonacci<10>::value == 55);
        REQUIRE(Fibonacci<15>::value == 610);
    }
    
    SECTION("Power calculation at compile-time") {
        template<int Base, int Exp>
        struct Power {
            static constexpr int value = Base * Power<Base, Exp - 1>::value;
        };
        
        template<int Base>
        struct Power<Base, 0> {
            static constexpr int value = 1;
        };
        
        static_assert(Power<2, 0>::value == 1);
        static_assert(Power<2, 3>::value == 8);
        static_assert(Power<3, 4>::value == 81);
        static_assert(Power<10, 3>::value == 1000);
        
        REQUIRE(Power<2, 10>::value == 1024);
        REQUIRE(Power<5, 3>::value == 125);
    }
    
    SECTION("Compile-time GCD calculation") {
        template<int A, int B>
        struct GCD {
            static constexpr int value = GCD<B, A % B>::value;
        };
        
        template<int A>
        struct GCD<A, 0> {
            static constexpr int value = A;
        };
        
        static_assert(GCD<48, 18>::value == 6);
        static_assert(GCD<100, 25>::value == 25);
        static_assert(GCD<17, 13>::value == 1);
        
        REQUIRE(GCD<48, 18>::value == 6);
        REQUIRE(GCD<100, 25>::value == 25);
    }
}

TEST_CASE("Type Traits and Detection", "[metaprogramming][type-traits][detection]") {
    
    SECTION("Custom type traits") {
        // Detect if type has getName method
        template<typename T>
        class has_getName {
            template<typename U>
            static auto test(int) -> decltype(std::declval<U>().getName(), std::true_type{});
            
            template<typename>
            static std::false_type test(...);
            
        public:
            static constexpr bool value = decltype(test<T>(0))::value;
        };
        
        // Detect if type has getId method
        template<typename T>
        class has_getId {
            template<typename U>
            static auto test(int) -> decltype(std::declval<U>().getId(), std::true_type{});
            
            template<typename>
            static std::false_type test(...);
            
        public:
            static constexpr bool value = decltype(test<T>(0))::value;
        };
        
        // Detect if type has update method
        template<typename T>
        class has_update {
            template<typename U>
            static auto test(int) -> decltype(std::declval<U>().update(std::declval<double>()), std::true_type{});
            
            template<typename>
            static std::false_type test(...);
            
        public:
            static constexpr bool value = decltype(test<T>(0))::value;
        };
        
        // Test with game objects
        static_assert(has_getName<Planet>::value);
        static_assert(has_getName<Fleet>::value);
        static_assert(!has_getName<int>::value);
        static_assert(!has_getName<std::string>::value);
        
        static_assert(has_getId<Planet>::value);
        static_assert(has_getId<Fleet>::value);
        static_assert(!has_getId<double>::value);
        
        static_assert(has_update<Planet>::value);
        static_assert(has_update<Fleet>::value);
        static_assert(!has_update<int>::value);
        
        REQUIRE(has_getName<Planet>::value);
        REQUIRE(has_getId<Fleet>::value);
        REQUIRE(has_update<Planet>::value);
        REQUIRE(!has_getName<int>::value);
    }
    
    SECTION("Concept-like type checking") {
        // Define a concept-like trait for Entity types
        template<typename T>
        struct is_entity {
            static constexpr bool value = has_getName<T>::value && 
                                        has_getId<T>::value && 
                                        has_update<T>::value;
        };
        
        template<typename T>
        constexpr bool is_entity_v = is_entity<T>::value;
        
        static_assert(is_entity_v<Planet>);
        static_assert(is_entity_v<Fleet>);
        static_assert(!is_entity_v<int>);
        static_assert(!is_entity_v<std::string>);
        
        REQUIRE(is_entity_v<Planet>);
        REQUIRE(is_entity_v<Fleet>);
        REQUIRE(!is_entity_v<int>);
    }
    
    SECTION("Container detection") {
        template<typename T>
        class is_container {
            template<typename U>
            static auto test(int) -> decltype(
                std::declval<U>().begin(),
                std::declval<U>().end(),
                std::declval<U>().size(),
                std::true_type{}
            );
            
            template<typename>
            static std::false_type test(...);
            
        public:
            static constexpr bool value = decltype(test<T>(0))::value;
        };
        
        template<typename T>
        constexpr bool is_container_v = is_container<T>::value;
        
        static_assert(is_container_v<std::vector<int>>);
        static_assert(is_container_v<std::string>);
        static_assert(is_container_v<std::array<int, 5>>);
        static_assert(!is_container_v<int>);
        static_assert(!is_container_v<Planet>);
        
        REQUIRE(is_container_v<std::vector<int>>);
        REQUIRE(!is_container_v<Planet>);
    }
    
    SECTION("Iterator category detection") {
        template<typename Iter>
        struct iterator_category_name {
            using category = typename std::iterator_traits<Iter>::iterator_category;
            
            static constexpr const char* name() {
                if constexpr (std::is_same_v<category, std::input_iterator_tag>) {
                    return "input";
                } else if constexpr (std::is_same_v<category, std::output_iterator_tag>) {
                    return "output";
                } else if constexpr (std::is_same_v<category, std::forward_iterator_tag>) {
                    return "forward";
                } else if constexpr (std::is_same_v<category, std::bidirectional_iterator_tag>) {
                    return "bidirectional";
                } else if constexpr (std::is_same_v<category, std::random_access_iterator_tag>) {
                    return "random_access";
                } else {
                    return "unknown";
                }
            }
        };
        
        REQUIRE(std::string(iterator_category_name<std::vector<int>::iterator>::name()) == "random_access");
        REQUIRE(std::string(iterator_category_name<std::list<int>::iterator>::name()) == "bidirectional");
        REQUIRE(std::string(iterator_category_name<std::forward_list<int>::iterator>::name()) == "forward");
    }
}

TEST_CASE("Template Metaprogramming Lists", "[metaprogramming][typelist][operations]") {
    
    SECTION("Type list implementation") {
        template<typename... Types>
        struct TypeList {
            static constexpr std::size_t size = sizeof...(Types);
        };
        
        // Type list operations
        template<typename List>
        struct Length;
        
        template<typename... Types>
        struct Length<TypeList<Types...>> {
            static constexpr std::size_t value = sizeof...(Types);
        };
        
        template<typename List, std::size_t Index>
        struct At;
        
        template<typename Head, typename... Tail>
        struct At<TypeList<Head, Tail...>, 0> {
            using type = Head;
        };
        
        template<typename Head, typename... Tail, std::size_t Index>
        struct At<TypeList<Head, Tail...>, Index> {
            using type = typename At<TypeList<Tail...>, Index - 1>::type;
        };
        
        // Test type list
        using GameTypes = TypeList<Planet, Fleet, int, std::string, double>;
        
        static_assert(Length<GameTypes>::value == 5);
        static_assert(std::is_same_v<typename At<GameTypes, 0>::type, Planet>);
        static_assert(std::is_same_v<typename At<GameTypes, 1>::type, Fleet>);
        static_assert(std::is_same_v<typename At<GameTypes, 2>::type, int>);
        
        REQUIRE(Length<GameTypes>::value == 5);
    }
    
    SECTION("Type list searching") {
        template<typename... Types>
        struct TypeList {};
        
        template<typename List, typename T>
        struct Contains;
        
        template<typename T>
        struct Contains<TypeList<>, T> {
            static constexpr bool value = false;
        };
        
        template<typename T, typename... Rest>
        struct Contains<TypeList<T, Rest...>, T> {
            static constexpr bool value = true;
        };
        
        template<typename Head, typename... Rest, typename T>
        struct Contains<TypeList<Head, Rest...>, T> {
            static constexpr bool value = Contains<TypeList<Rest...>, T>::value;
        };
        
        template<typename List, typename T>
        struct IndexOf;
        
        template<typename T, typename... Rest>
        struct IndexOf<TypeList<T, Rest...>, T> {
            static constexpr std::size_t value = 0;
        };
        
        template<typename Head, typename... Rest, typename T>
        struct IndexOf<TypeList<Head, Rest...>, T> {
            static constexpr std::size_t value = 1 + IndexOf<TypeList<Rest...>, T>::value;
        };
        
        using TestTypes = TypeList<int, Planet, Fleet, std::string>;
        
        static_assert(Contains<TestTypes, Planet>::value);
        static_assert(Contains<TestTypes, Fleet>::value);
        static_assert(!Contains<TestTypes, double>::value);
        
        static_assert(IndexOf<TestTypes, Planet>::value == 1);
        static_assert(IndexOf<TestTypes, Fleet>::value == 2);
        static_assert(IndexOf<TestTypes, std::string>::value == 3);
        
        REQUIRE(Contains<TestTypes, Planet>::value);
        REQUIRE(!Contains<TestTypes, double>::value);
    }
    
    SECTION("Type list transformations") {
        template<typename... Types>
        struct TypeList {};
        
        template<typename List, template<typename> class Transform>
        struct Map;
        
        template<typename... Types, template<typename> class Transform>
        struct Map<TypeList<Types...>, Transform> {
            using type = TypeList<typename Transform<Types>::type...>;
        };
        
        template<typename List, template<typename> class Predicate>
        struct Filter;
        
        template<template<typename> class Predicate>
        struct Filter<TypeList<>, Predicate> {
            using type = TypeList<>;
        };
        
        template<typename Head, typename... Tail, template<typename> class Predicate>
        struct Filter<TypeList<Head, Tail...>, Predicate> {
            using filtered_tail = typename Filter<TypeList<Tail...>, Predicate>::type;
            
            using type = std::conditional_t<
                Predicate<Head>::value,
                /* if true */ typename PrependType<Head, filtered_tail>::type,
                /* if false */ filtered_tail
            >;
        };
        
        template<typename T, typename List>
        struct PrependType;
        
        template<typename T, typename... Types>
        struct PrependType<T, TypeList<Types...>> {
            using type = TypeList<T, Types...>;
        };
        
        // Transform to pointers
        template<typename T>
        struct AddPointer {
            using type = T*;
        };
        
        // Filter for arithmetic types
        template<typename T>
        struct IsArithmetic {
            static constexpr bool value = std::is_arithmetic_v<T>;
        };
        
        using OriginalTypes = TypeList<int, Planet, double, Fleet, char>;
        using PointerTypes = typename Map<OriginalTypes, AddPointer>::type;
        using ArithmeticTypes = typename Filter<OriginalTypes, IsArithmetic>::type;
        
        // Verify transformations
        static_assert(std::is_same_v<PointerTypes, TypeList<int*, Planet*, double*, Fleet*, char*>>);
        static_assert(std::is_same_v<ArithmeticTypes, TypeList<int, double, char>>);
    }
}

TEST_CASE("Constexpr Programming", "[metaprogramming][constexpr][compile-time]") {
    
    SECTION("Constexpr functions") {
        constexpr int constexpr_factorial(int n) {
            if (n <= 1) return 1;
            return n * constexpr_factorial(n - 1);
        }
        
        constexpr int constexpr_fibonacci(int n) {
            if (n <= 1) return n;
            return constexpr_fibonacci(n - 1) + constexpr_fibonacci(n - 2);
        }
        
        constexpr bool is_prime(int n) {
            if (n <= 1) return false;
            if (n <= 3) return true;
            if (n % 2 == 0 || n % 3 == 0) return false;
            
            for (int i = 5; i * i <= n; i += 6) {
                if (n % i == 0 || n % (i + 2) == 0) {
                    return false;
                }
            }
            return true;
        }
        
        // Compile-time evaluation
        static_assert(constexpr_factorial(5) == 120);
        static_assert(constexpr_fibonacci(10) == 55);
        static_assert(is_prime(17));
        static_assert(!is_prime(15));
        
        // Runtime verification
        REQUIRE(constexpr_factorial(6) == 720);
        REQUIRE(constexpr_fibonacci(12) == 144);
        REQUIRE(is_prime(23));
        REQUIRE(!is_prime(21));
    }
    
    SECTION("Constexpr containers") {
        constexpr auto make_array() {
            std::array<int, 5> arr{};
            for (std::size_t i = 0; i < arr.size(); ++i) {
                arr[i] = static_cast<int>(i * i);
            }
            return arr;
        }
        
        constexpr auto squares = make_array();
        
        static_assert(squares[0] == 0);
        static_assert(squares[1] == 1);
        static_assert(squares[2] == 4);
        static_assert(squares[3] == 9);
        static_assert(squares[4] == 16);
        
        REQUIRE(squares[3] == 9);
        REQUIRE(squares[4] == 16);
    }
    
    SECTION("Constexpr string operations") {
        constexpr std::size_t constexpr_strlen(const char* str) {
            std::size_t len = 0;
            while (str[len] != '\0') {
                ++len;
            }
            return len;
        }
        
        constexpr bool constexpr_equal(const char* a, const char* b) {
            while (*a && *b && *a == *b) {
                ++a;
                ++b;
            }
            return *a == *b;
        }
        
        static_assert(constexpr_strlen("Hello") == 5);
        static_assert(constexpr_strlen("CppVerseHub") == 11);
        static_assert(constexpr_equal("test", "test"));
        static_assert(!constexpr_equal("test", "Test"));
        
        REQUIRE(constexpr_strlen("World") == 5);
        REQUIRE(constexpr_equal("same", "same"));
    }
    
    SECTION("Constexpr complex calculations") {
        struct Point {
            double x, y;
            
            constexpr Point(double x_val, double y_val) : x(x_val), y(y_val) {}
            
            constexpr double distance_to(const Point& other) const {
                double dx = x - other.x;
                double dy = y - other.y;
                return std::sqrt(dx * dx + dy * dy);
            }
            
            constexpr Point midpoint_with(const Point& other) const {
                return Point((x + other.x) / 2, (y + other.y) / 2);
            }
        };
        
        constexpr Point p1{0.0, 0.0};
        constexpr Point p2{3.0, 4.0};
        constexpr Point midpoint = p1.midpoint_with(p2);
        constexpr double distance = p1.distance_to(p2);
        
        static_assert(midpoint.x == 1.5);
        static_assert(midpoint.y == 2.0);
        static_assert(distance == 5.0);
        
        REQUIRE(midpoint.x == 1.5);
        REQUIRE(distance == 5.0);
    }
}

TEST_CASE("Template Specialization Patterns", "[metaprogramming][specialization][patterns]") {
    
    SECTION("Full template specialization") {
        template<typename T>
        struct TypeName {
            static constexpr const char* name = "unknown";
        };
        
        template<>
        struct TypeName<int> {
            static constexpr const char* name = "integer";
        };
        
        template<>
        struct TypeName<double> {
            static constexpr const char* name = "double";
        };
        
        template<>
        struct TypeName<std::string> {
            static constexpr const char* name = "string";
        };
        
        template<>
        struct TypeName<Planet> {
            static constexpr const char* name = "planet";
        };
        
        REQUIRE(std::string(TypeName<int>::name) == "integer");
        REQUIRE(std::string(TypeName<double>::name) == "double");
        REQUIRE(std::string(TypeName<std::string>::name) == "string");
        REQUIRE(std::string(TypeName<Planet>::name) == "planet");
        REQUIRE(std::string(TypeName<float>::name) == "unknown");
    }
    
    SECTION("Partial template specialization") {
        template<typename T, typename U>
        struct Relationship {
            static constexpr const char* description = "unrelated";
        };
        
        template<typename T>
        struct Relationship<T, T> {
            static constexpr const char* description = "same type";
        };
        
        template<typename T>
        struct Relationship<T, T*> {
            static constexpr const char* description = "pointer to type";
        };
        
        template<typename T>
        struct Relationship<T*, T> {
            static constexpr const char* description = "type to pointer";
        };
        
        template<typename T, typename U>
        struct Relationship<T*, U*> {
            static constexpr const char* description = "both pointers";
        };
        
        REQUIRE(std::string(Relationship<int, int>::description) == "same type");
        REQUIRE(std::string(Relationship<int, int*>::description) == "pointer to type");
        REQUIRE(std::string(Relationship<int*, int>::description) == "type to pointer");
        REQUIRE(std::string(Relationship<int*, double*>::description) == "both pointers");
        REQUIRE(std::string(Relationship<int, double>::description) == "unrelated");
    }
    
    SECTION("SFINAE-based specialization") {
        template<typename T, typename Enable = void>
        struct Processor {
            static std::string process(const T&) {
                return "generic processing";
            }
        };
        
        template<typename T>
        struct Processor<T, typename std::enable_if_t<std::is_arithmetic_v<T>>> {
            static std::string process(const T& value) {
                return "arithmetic: " + std::to_string(value);
            }
        };
        
        template<typename T>
        struct Processor<T, typename std::enable_if_t<std::is_same_v<T, std::string>>> {
            static std::string process(const T& value) {
                return "string: " + value;
            }
        };
        
        // Test with different types
        REQUIRE(Processor<int>::process(42) == "arithmetic: 42");
        REQUIRE(Processor<double>::process(3.14) == "arithmetic: 3.140000");
        REQUIRE(Processor<std::string>::process("hello") == "string: hello");
        
        // Create a Planet and test generic processing
        Planet testPlanet("TestPlanet", Vector3D{0, 0, 0});
        REQUIRE(Processor<Planet>::process(testPlanet) == "generic processing");
    }
}

TEST_CASE("Advanced Metaprogramming Techniques", "[metaprogramming][advanced][techniques]") {
    
    SECTION("Expression templates") {
        template<typename E>
        class VecExpression {
        public:
            double operator[](std::size_t i) const {
                return static_cast<const E&>(*this)[i];
            }
            
            std::size_t size() const {
                return static_cast<const E&>(*this).size();
            }
        };
        
        class Vec : public VecExpression<Vec> {
        private:
            std::vector<double> data_;
            
        public:
            Vec(std::size_t size) : data_(size) {}
            Vec(std::initializer_list<double> init) : data_(init) {}
            
            double& operator[](std::size_t i) { return data_[i]; }
            double operator[](std::size_t i) const { return data_[i]; }
            std::size_t size() const { return data_.size(); }
            
            template<typename E>
            Vec& operator=(const VecExpression<E>& expr) {
                for (std::size_t i = 0; i < size(); ++i) {
                    data_[i] = expr[i];
                }
                return *this;
            }
        };
        
        template<typename E1, typename E2>
        class VecSum : public VecExpression<VecSum<E1, E2>> {
        private:
            const E1& e1_;
            const E2& e2_;
            
        public:
            VecSum(const E1& e1, const E2& e2) : e1_(e1), e2_(e2) {}
            
            double operator[](std::size_t i) const {
                return e1_[i] + e2_[i];
            }
            
            std::size_t size() const { return e1_.size(); }
        };
        
        template<typename E1, typename E2>
        VecSum<E1, E2> operator+(const VecExpression<E1>& e1, const VecExpression<E2>& e2) {
            return VecSum<E1, E2>(static_cast<const E1&>(e1), static_cast<const E2&>(e2));
        }
        
        // Test expression templates
        Vec a{1.0, 2.0, 3.0};
        Vec b{4.0, 5.0, 6.0};
        Vec c{7.0, 8.0, 9.0};
        
        Vec result(3);
        result = a + b + c; // This creates expression template chain
        
        REQUIRE(result[0] == Approx(12.0));
        REQUIRE(result[1] == Approx(15.0));
        REQUIRE(result[2] == Approx(18.0));
    }
    
    SECTION("Policy-based design") {
        // Storage policy
        template<typename T>
        class VectorStorage {
        protected:
            std::vector<T> data_;
            
        public:
            using iterator = typename std::vector<T>::iterator;
            using const_iterator = typename std::vector<T>::const_iterator;
            
            void resize(std::size_t size) { data_.resize(size); }
            std::size_t size() const { return data_.size(); }
            T& operator[](std::size_t i) { return data_[i]; }
            const T& operator[](std::size_t i) const { return data_[i]; }
            iterator begin() { return data_.begin(); }
            iterator end() { return data_.end(); }
        };
        
        template<typename T, std::size_t N>
        class ArrayStorage {
        protected:
            std::array<T, N> data_;
            
        public:
            using iterator = typename std::array<T, N>::iterator;
            using const_iterator = typename std::array<T, N>::const_iterator;
            
            constexpr std::size_t size() const { return N; }
            T& operator[](std::size_t i) { return data_[i]; }
            const T& operator[](std::size_t i) const { return data_[i]; }
            iterator begin() { return data_.begin(); }
            iterator end() { return data_.end(); }
        };
        
        // Checking policy
        class BoundsChecking {
        protected:
            void checkBounds(std::size_t index, std::size_t size) const {
                if (index >= size) {
                    throw std::out_of_range("Index out of bounds");
                }
            }
        };
        
        class NoBoundsChecking {
        protected:
            void checkBounds(std::size_t, std::size_t) const {
                // No checking
            }
        };
        
        // Policy-based container
        template<typename T, 
                 template<typename> class StoragePolicy,
                 class CheckingPolicy>
        class PolicyContainer : public StoragePolicy<T>, public CheckingPolicy {
        public:
            T& at(std::size_t index) {
                this->checkBounds(index, this->size());
                return (*this)[index];
            }
            
            const T& at(std::size_t index) const {
                this->checkBounds(index, this->size());
                return (*this)[index];
            }
        };
        
        // Test different policy combinations
        PolicyContainer<int, VectorStorage, BoundsChecking> safeVector;
        PolicyContainer<int, VectorStorage, NoBoundsChecking> fastVector;
        
        safeVector.resize(5);
        fastVector.resize(5);
        
        for (int i = 0; i < 5; ++i) {
            safeVector[i] = i;
            fastVector[i] = i * 2;
        }
        
        REQUIRE(safeVector.at(2) == 2);
        REQUIRE(fastVector.at(2) == 4);
        
        // Test bounds checking
        REQUIRE_THROWS_AS(safeVector.at(10), std::out_of_range);
        // fastVector.at(10) would not throw, but would be undefined behavior
    }
    
    SECTION("Template metaprogram for compile-time sorting") {
        template<int... Values>
        struct IntList {};
        
        template<int Value, typename List>
        struct Insert;
        
        template<int Value>
        struct Insert<Value, IntList<>> {
            using type = IntList<Value>;
        };
        
        template<int Value, int Head, int... Tail>
        struct Insert<Value, IntList<Head, Tail...>> {
            using type = std::conditional_t<
                (Value <= Head),
                IntList<Value, Head, Tail...>,
                typename PrependInt<Head, typename Insert<Value, IntList<Tail...>>::type>::type
            >;
        };
        
        template<int Value, typename List>
        struct PrependInt;
        
        template<int Value, int... Values>
        struct PrependInt<Value, IntList<Values...>> {
            using type = IntList<Value, Values...>;
        };
        
        template<typename List>
        struct Sort;
        
        template<>
        struct Sort<IntList<>> {
            using type = IntList<>;
        };
        
        template<int Head, int... Tail>
        struct Sort<IntList<Head, Tail...>> {
            using sorted_tail = typename Sort<IntList<Tail...>>::type;
            using type = typename Insert<Head, sorted_tail>::type;
        };
        
        // Test compile-time sorting
        using UnsortedList = IntList<5, 2, 8, 1, 9, 3>;
        using SortedList = typename Sort<UnsortedList>::type;
        
        static_assert(std::is_same_v<SortedList, IntList<1, 2, 3, 5, 8, 9>>);
    }
}

TEST_CASE("Metaprogramming Performance", "[metaprogramming][performance][compile-time]") {
    
    SECTION("Compile-time vs runtime performance") {
        // Compile-time computation
        template<int N>
        struct CompileTimePower {
            static constexpr long long value = N * CompileTimePower<N - 1>::value;
        };
        
        template<>
        struct CompileTimePower<0> {
            static constexpr long long value = 1;
        };
        
        // Runtime computation
        constexpr long long runtimePower(int n) {
            long long result = 1;
            for (int i = 0; i < n; ++i) {
                result *= 2;
            }
            return result;
        }
        
        // Compare results
        constexpr auto compileTimeResult = CompileTimePower<20>::value;
        const auto runtimeResult = runtimePower(20);
        
        // Both should be computed, but compile-time version is computed during compilation
        REQUIRE(compileTimeResult == runtimeResult);
        REQUIRE(compileTimeResult == 1048576); // 2^20
        
        // Measure runtime computation (compile-time version has zero runtime cost)
        auto duration = measurePerformance([&]() {
            volatile auto result = runtimePower(20);
            (void)result;
        }, "Runtime power calculation");
        
        INFO("Runtime computation took: " << duration.count() << " microseconds");
        REQUIRE(duration.count() >= 0); // Some time should pass
    }
    
    SECTION("Template instantiation depth") {
        // Test deep template recursion
        template<int Depth>
        struct DeepRecursion {
            static constexpr int value = 1 + DeepRecursion<Depth - 1>::value;
        };
        
        template<>
        struct DeepRecursion<0> {
            static constexpr int value = 0;
        };
        
        // Test various depths
        static_assert(DeepRecursion<10>::value == 10);
        static_assert(DeepRecursion<100>::value == 100);
        static_assert(DeepRecursion<500>::value == 500);
        
        REQUIRE(DeepRecursion<100>::value == 100);
        REQUIRE(DeepRecursion<500>::value == 500);
    }
    
    SECTION("Memory usage of template metaprogramming") {
        // Test compile-time memory efficiency
        template<std::size_t N>
        struct LargeArray {
            static constexpr std::array<int, N> generate() {
                std::array<int, N> arr{};
                for (std::size_t i = 0; i < N; ++i) {
                    arr[i] = static_cast<int>(i * i);
                }
                return arr;
            }
            
            static constexpr auto value = generate();
        };
        
        // These are computed at compile time, so no runtime memory overhead
        constexpr auto smallArray = LargeArray<100>::value;
        constexpr auto largeArray = LargeArray<1000>::value;
        
        REQUIRE(smallArray[10] == 100);
        REQUIRE(largeArray[10] == 100);
        REQUIRE(smallArray.size() == 100);
        REQUIRE(largeArray.size() == 1000);
        
        // Verify the arrays are actually computed correctly
        for (std::size_t i = 0; i < 10; ++i) {
            REQUIRE(smallArray[i] == static_cast<int>(i * i));
            REQUIRE(largeArray[i] == static_cast<int>(i * i));
        }
    }
}