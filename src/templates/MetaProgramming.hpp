// File: src/templates/MetaProgramming.hpp
// Compile-time computations and template metaprogramming demonstrations

#ifndef META_PROGRAMMING_HPP
#define META_PROGRAMMING_HPP

#include <type_traits>
#include <utility>
#include <array>
#include <string_view>
#include <concepts>
#include <tuple>
#include <functional>
#include <cstdint>

namespace CppVerseHub {
namespace Templates {
namespace Meta {

// ===== Type Traits and Type Manipulation =====

/**
 * @brief Check if a type is a template instantiation of a given template
 */
template<template<typename...> class Template, typename T>
struct is_instantiation_of : std::false_type {};

template<template<typename...> class Template, typename... Args>
struct is_instantiation_of<Template, Template<Args...>> : std::true_type {};

template<template<typename...> class Template, typename T>
constexpr bool is_instantiation_of_v = is_instantiation_of<Template, T>::value;

/**
 * @brief Remove all cv-qualifiers and references
 */
template<typename T>
using remove_cvref_t = std::remove_cv_t<std::remove_reference_t<T>>;

/**
 * @brief Check if a type is any of the given types
 */
template<typename T, typename... Types>
struct is_any_of : std::disjunction<std::is_same<T, Types>...> {};

template<typename T, typename... Types>
constexpr bool is_any_of_v = is_any_of<T, Types...>::value;

/**
 * @brief Type list for compile-time type collections
 */
template<typename... Types>
struct type_list {
    static constexpr std::size_t size = sizeof...(Types);
};

/**
 * @brief Get the Nth type from a type list
 */
template<std::size_t N, typename... Types>
struct type_at;

template<std::size_t N, typename Head, typename... Tail>
struct type_at<N, Head, Tail...> {
    using type = typename type_at<N - 1, Tail...>::type;
};

template<typename Head, typename... Tail>
struct type_at<0, Head, Tail...> {
    using type = Head;
};

template<std::size_t N, typename... Types>
using type_at_t = typename type_at<N, Types...>::type;

/**
 * @brief Find the index of a type in a type list
 */
template<typename T, typename... Types>
struct type_index;

template<typename T, typename Head, typename... Tail>
struct type_index<T, Head, Tail...> {
    static constexpr std::size_t value = std::is_same_v<T, Head> ? 0 : 1 + type_index<T, Tail...>::value;
};

template<typename T>
struct type_index<T> {
    static constexpr std::size_t value = static_cast<std::size_t>(-1); // Not found
};

template<typename T, typename... Types>
constexpr std::size_t type_index_v = type_index<T, Types...>::value;

/**
 * @brief Check if a type exists in a type list
 */
template<typename T, typename... Types>
constexpr bool type_exists_v = (std::is_same_v<T, Types> || ...);

// ===== Compile-time Arithmetic =====

/**
 * @brief Compile-time factorial calculation
 */
template<std::size_t N>
struct factorial {
    static constexpr std::size_t value = N * factorial<N - 1>::value;
};

template<>
struct factorial<0> {
    static constexpr std::size_t value = 1;
};

template<std::size_t N>
constexpr std::size_t factorial_v = factorial<N>::value;

/**
 * @brief Constexpr factorial function (C++14 style)
 */
constexpr std::size_t factorial_func(std::size_t n) {
    return n <= 1 ? 1 : n * factorial_func(n - 1);
}

/**
 * @brief Compile-time Fibonacci sequence
 */
template<std::size_t N>
struct fibonacci {
    static constexpr std::size_t value = fibonacci<N - 1>::value + fibonacci<N - 2>::value;
};

template<>
struct fibonacci<0> {
    static constexpr std::size_t value = 0;
};

template<>
struct fibonacci<1> {
    static constexpr std::size_t value = 1;
};

template<std::size_t N>
constexpr std::size_t fibonacci_v = fibonacci<N>::value;

/**
 * @brief Constexpr Fibonacci with memoization
 */
constexpr std::size_t fibonacci_func(std::size_t n) {
    if (n <= 1) return n;
    
    std::size_t a = 0, b = 1;
    for (std::size_t i = 2; i <= n; ++i) {
        std::size_t temp = a + b;
        a = b;
        b = temp;
    }
    return b;
}

/**
 * @brief Compile-time power calculation
 */
template<std::size_t Base, std::size_t Exponent>
struct power {
    static constexpr std::size_t value = Base * power<Base, Exponent - 1>::value;
};

template<std::size_t Base>
struct power<Base, 0> {
    static constexpr std::size_t value = 1;
};

template<std::size_t Base, std::size_t Exponent>
constexpr std::size_t power_v = power<Base, Exponent>::value;

/**
 * @brief Constexpr power function with fast exponentiation
 */
constexpr std::size_t power_func(std::size_t base, std::size_t exp) {
    if (exp == 0) return 1;
    if (exp == 1) return base;
    
    std::size_t result = 1;
    while (exp > 0) {
        if (exp & 1) result *= base;
        base *= base;
        exp >>= 1;
    }
    return result;
}

/**
 * @brief Check if a number is prime at compile time
 */
constexpr bool is_prime(std::size_t n) {
    if (n < 2) return false;
    if (n == 2) return true;
    if (n % 2 == 0) return false;
    
    for (std::size_t i = 3; i * i <= n; i += 2) {
        if (n % i == 0) return false;
    }
    return true;
}

/**
 * @brief Generate prime numbers up to N at compile time
 */
template<std::size_t N>
constexpr std::array<std::size_t, []() {
    std::size_t count = 0;
    for (std::size_t i = 2; i <= N; ++i) {
        if (is_prime(i)) ++count;
    }
    return count;
}()> generate_primes() {
    constexpr std::size_t count = []() {
        std::size_t c = 0;
        for (std::size_t i = 2; i <= N; ++i) {
            if (is_prime(i)) ++c;
        }
        return c;
    }();
    
    std::array<std::size_t, count> primes{};
    std::size_t index = 0;
    
    for (std::size_t i = 2; i <= N && index < count; ++i) {
        if (is_prime(i)) {
            primes[index++] = i;
        }
    }
    
    return primes;
}

// ===== Compile-time String Processing =====

/**
 * @brief Compile-time string class
 */
template<std::size_t N>
struct compile_time_string {
    char data[N];
    std::size_t length;
    
    constexpr compile_time_string() : data{}, length(0) {}
    
    constexpr compile_time_string(const char (&str)[N]) : length(N - 1) {
        for (std::size_t i = 0; i < N; ++i) {
            data[i] = str[i];
        }
    }
    
    constexpr char operator[](std::size_t i) const { return data[i]; }
    constexpr std::size_t size() const { return length; }
    constexpr const char* c_str() const { return data; }
    
    constexpr std::string_view to_string_view() const {
        return std::string_view(data, length);
    }
};

// Deduction guide
template<std::size_t N>
compile_time_string(const char (&)[N]) -> compile_time_string<N>;

/**
 * @brief Compile-time string concatenation
 */
template<std::size_t N1, std::size_t N2>
constexpr auto operator+(const compile_time_string<N1>& lhs, const compile_time_string<N2>& rhs) {
    compile_time_string<N1 + N2 - 1> result;
    result.length = lhs.length + rhs.length;
    
    for (std::size_t i = 0; i < lhs.length; ++i) {
        result.data[i] = lhs.data[i];
    }
    
    for (std::size_t i = 0; i < rhs.length; ++i) {
        result.data[lhs.length + i] = rhs.data[i];
    }
    
    result.data[result.length] = '\0';
    return result;
}

/**
 * @brief Compile-time string hashing (FNV-1a)
 */
constexpr std::uint64_t hash_string(std::string_view str) {
    constexpr std::uint64_t FNV_OFFSET_BASIS = 14695981039346656037ULL;
    constexpr std::uint64_t FNV_PRIME = 1099511628211ULL;
    
    std::uint64_t hash = FNV_OFFSET_BASIS;
    for (char c : str) {
        hash ^= static_cast<std::uint64_t>(c);
        hash *= FNV_PRIME;
    }
    return hash;
}

// ===== Template Metaprogramming Utilities =====

/**
 * @brief Conditional type selection based on compile-time condition
 */
template<bool Condition, typename TrueType, typename FalseType>
using conditional_t = typename std::conditional<Condition, TrueType, FalseType>::type;

/**
 * @brief Enable if with void default
 */
template<bool Condition, typename T = void>
using enable_if_t = typename std::enable_if<Condition, T>::type;

/**
 * @brief Compile-time loop unrolling
 */
template<std::size_t N, typename Func>
constexpr void static_for(Func&& func) {
    if constexpr (N > 0) {
        func(std::integral_constant<std::size_t, N - 1>{});
        static_for<N - 1>(std::forward<Func>(func));
    }
}

/**
 * @brief Compile-time array initialization with function
 */
template<typename T, std::size_t N, typename Func>
constexpr std::array<T, N> make_array(Func&& func) {
    std::array<T, N> arr{};
    static_for<N>([&](auto i) {
        arr[i] = func(i);
    });
    return arr;
}

/**
 * @brief Compile-time tuple manipulation
 */
template<typename Tuple, std::size_t... Indices>
constexpr auto tuple_reverse_impl(Tuple&& t, std::index_sequence<Indices...>) {
    constexpr auto size = std::tuple_size_v<std::remove_reference_t<Tuple>>;
    return std::make_tuple(std::get<size - 1 - Indices>(std::forward<Tuple>(t))...);
}

template<typename Tuple>
constexpr auto tuple_reverse(Tuple&& t) {
    constexpr auto size = std::tuple_size_v<std::remove_reference_t<Tuple>>;
    return tuple_reverse_impl(std::forward<Tuple>(t), std::make_index_sequence<size>{});
}

/**
 * @brief Compile-time tuple filtering
 */
template<typename Predicate, typename Tuple>
constexpr auto tuple_filter(Predicate&& pred, Tuple&& t) {
    return std::apply([&](auto&&... elements) {
        return std::tuple_cat(
            std::conditional_t<
                std::invoke_result_v<Predicate, decltype(elements)>,
                std::tuple<std::remove_reference_t<decltype(elements)>>,
                std::tuple<>
            >(elements)...
        );
    }, std::forward<Tuple>(t));
}

// ===== Advanced Template Techniques =====

/**
 * @brief CRTP (Curiously Recurring Template Pattern) base
 */
template<typename Derived>
class CRTP_Base {
public:
    Derived& derived() { return static_cast<Derived&>(*this); }
    const Derived& derived() const { return static_cast<const Derived&>(*this); }
    
protected:
    CRTP_Base() = default;
    ~CRTP_Base() = default;
    CRTP_Base(const CRTP_Base&) = default;
    CRTP_Base& operator=(const CRTP_Base&) = default;
    CRTP_Base(CRTP_Base&&) = default;
    CRTP_Base& operator=(CRTP_Base&&) = default;
};

/**
 * @brief Example CRTP usage - Comparable mixin
 */
template<typename Derived>
class Comparable : public CRTP_Base<Derived> {
public:
    friend bool operator!=(const Derived& lhs, const Derived& rhs) {
        return !(lhs == rhs);
    }
    
    friend bool operator>(const Derived& lhs, const Derived& rhs) {
        return rhs < lhs;
    }
    
    friend bool operator<=(const Derived& lhs, const Derived& rhs) {
        return !(lhs > rhs);
    }
    
    friend bool operator>=(const Derived& lhs, const Derived& rhs) {
        return !(lhs < rhs);
    }
};

/**
 * @brief Template specialization detector
 */
template<typename T, template<typename...> typename Template>
struct is_specialization : std::false_type {};

template<template<typename...> typename Template, typename... Args>
struct is_specialization<Template<Args...>, Template> : std::true_type {};

template<typename T, template<typename...> typename Template>
constexpr bool is_specialization_v = is_specialization<T, Template>::value;

/**
 * @brief Compile-time rational number arithmetic
 */
template<std::intmax_t Num, std::intmax_t Den = 1>
class ratio {
private:
    static constexpr std::intmax_t gcd(std::intmax_t a, std::intmax_t b) {
        return b == 0 ? a : gcd(b, a % b);
    }
    
    static constexpr std::intmax_t abs(std::intmax_t x) {
        return x < 0 ? -x : x;
    }
    
    static constexpr std::intmax_t g = gcd(abs(Num), abs(Den));
    
public:
    static constexpr std::intmax_t num = (Den < 0 ? -Num : Num) / g;
    static constexpr std::intmax_t den = abs(Den) / g;
    
    using type = ratio<num, den>;
};

/**
 * @brief Ratio arithmetic operations
 */
template<typename R1, typename R2>
using ratio_add = ratio<R1::num * R2::den + R2::num * R1::den, R1::den * R2::den>;

template<typename R1, typename R2>
using ratio_subtract = ratio<R1::num * R2::den - R2::num * R1::den, R1::den * R2::den>;

template<typename R1, typename R2>
using ratio_multiply = ratio<R1::num * R2::num, R1::den * R2::den>;

template<typename R1, typename R2>
using ratio_divide = ratio<R1::num * R2::den, R1::den * R2::num>;

/**
 * @brief Compile-time dimensional analysis
 */
template<int Mass, int Length, int Time, int Current = 0, int Temperature = 0, int Amount = 0, int Luminosity = 0>
struct dimension {
    static constexpr int mass = Mass;
    static constexpr int length = Length;
    static constexpr int time = Time;
    static constexpr int current = Current;
    static constexpr int temperature = Temperature;
    static constexpr int amount = Amount;
    static constexpr int luminosity = Luminosity;
};

// Common dimensions
using dimensionless = dimension<0, 0, 0>;
using mass_dimension = dimension<1, 0, 0>;
using length_dimension = dimension<0, 1, 0>;
using time_dimension = dimension<0, 0, 1>;
using velocity_dimension = dimension<0, 1, -1>;
using acceleration_dimension = dimension<0, 1, -2>;
using force_dimension = dimension<1, 1, -2>;
using energy_dimension = dimension<1, 2, -2>;

/**
 * @brief Compile-time unit system with dimensional analysis
 */
template<typename Rep, typename Dimension = dimensionless>
class quantity {
private:
    Rep value_;
    
public:
    using rep = Rep;
    using dimension = Dimension;
    
    constexpr quantity() : value_(Rep{}) {}
    constexpr explicit quantity(const Rep& val) : value_(val) {}
    
    template<typename Rep2>
        requires std::convertible_to<Rep2, Rep>
    constexpr explicit quantity(const Rep2& val) : value_(static_cast<Rep>(val)) {}
    
    constexpr Rep count() const { return value_; }
    constexpr operator Rep() const { return value_; }
    
    // Arithmetic operations with dimensional analysis
    constexpr quantity operator+() const { return *this; }
    constexpr quantity operator-() const { return quantity(-value_); }
    
    template<typename Rep2>
    constexpr quantity& operator+=(const quantity<Rep2, Dimension>& rhs) {
        value_ += rhs.count();
        return *this;
    }
    
    template<typename Rep2>
    constexpr quantity& operator-=(const quantity<Rep2, Dimension>& rhs) {
        value_ -= rhs.count();
        return *this;
    }
    
    template<typename Rep2>
    constexpr quantity& operator*=(const Rep2& rhs) {
        value_ *= rhs;
        return *this;
    }
    
    template<typename Rep2>
    constexpr quantity& operator/=(const Rep2& rhs) {
        value_ /= rhs;
        return *this;
    }
};

// Addition and subtraction (same dimensions only)
template<typename Rep1, typename Rep2, typename Dim>
constexpr auto operator+(const quantity<Rep1, Dim>& lhs, const quantity<Rep2, Dim>& rhs) {
    using CommonRep = std::common_type_t<Rep1, Rep2>;
    return quantity<CommonRep, Dim>(lhs.count() + rhs.count());
}

template<typename Rep1, typename Rep2, typename Dim>
constexpr auto operator-(const quantity<Rep1, Dim>& lhs, const quantity<Rep2, Dim>& rhs) {
    using CommonRep = std::common_type_t<Rep1, Rep2>;
    return quantity<CommonRep, Dim>(lhs.count() - rhs.count());
}

// Multiplication (dimensions add)
template<typename Rep1, typename Rep2, int M1, int L1, int T1, int M2, int L2, int T2>
constexpr auto operator*(const quantity<Rep1, dimension<M1, L1, T1>>& lhs,
                        const quantity<Rep2, dimension<M2, L2, T2>>& rhs) {
    using CommonRep = std::common_type_t<Rep1, Rep2>;
    using ResultDim = dimension<M1 + M2, L1 + L2, T1 + T2>;
    return quantity<CommonRep, ResultDim>(lhs.count() * rhs.count());
}

// Division (dimensions subtract)
template<typename Rep1, typename Rep2, int M1, int L1, int T1, int M2, int L2, int T2>
constexpr auto operator/(const quantity<Rep1, dimension<M1, L1, T1>>& lhs,
                        const quantity<Rep2, dimension<M2, L2, T2>>& rhs) {
    using CommonRep = std::common_type_t<Rep1, Rep2>;
    using ResultDim = dimension<M1 - M2, L1 - L2, T1 - T2>;
    return quantity<CommonRep, ResultDim>(lhs.count() / rhs.count());
}

// Scalar multiplication/division
template<typename Rep1, typename Rep2, typename Dim>
constexpr auto operator*(const quantity<Rep1, Dim>& lhs, const Rep2& rhs) {
    using CommonRep = std::common_type_t<Rep1, Rep2>;
    return quantity<CommonRep, Dim>(lhs.count() * rhs);
}

template<typename Rep1, typename Rep2, typename Dim>
constexpr auto operator*(const Rep1& lhs, const quantity<Rep2, Dim>& rhs) {
    using CommonRep = std::common_type_t<Rep1, Rep2>;
    return quantity<CommonRep, Dim>(lhs * rhs.count());
}

template<typename Rep1, typename Rep2, typename Dim>
constexpr auto operator/(const quantity<Rep1, Dim>& lhs, const Rep2& rhs) {
    using CommonRep = std::common_type_t<Rep1, Rep2>;
    return quantity<CommonRep, Dim>(lhs.count() / rhs);
}

// Common quantity types
using mass = quantity<double, mass_dimension>;
using length = quantity<double, length_dimension>;
using time_duration = quantity<double, time_dimension>;
using velocity = quantity<double, velocity_dimension>;
using acceleration = quantity<double, acceleration_dimension>;
using force = quantity<double, force_dimension>;
using energy = quantity<double, energy_dimension>;

/**
 * @brief Template-based state machine
 */
template<typename... States>
class StateMachine {
private:
    std::variant<States...> current_state_;
    
public:
    template<typename InitialState>
        requires (std::is_same_v<InitialState, States> || ...)
    explicit StateMachine(InitialState&& initial) : current_state_(std::forward<InitialState>(initial)) {}
    
    template<typename Event>
    void process_event(const Event& event) {
        std::visit([&](auto& state) {
            if constexpr (requires { state.process(event); }) {
                auto new_state = state.process(event);
                if constexpr (!std::is_void_v<decltype(new_state)>) {
                    current_state_ = std::move(new_state);
                }
            }
        }, current_state_);
    }
    
    template<typename State>
    bool is_state() const {
        return std::holds_alternative<State>(current_state_);
    }
    
    template<typename State>
    const State* get_state() const {
        return std::get_if<State>(&current_state_);
    }
    
    template<typename Visitor>
    auto visit(Visitor&& visitor) const {
        return std::visit(std::forward<Visitor>(visitor), current_state_);
    }
};

/**
 * @brief Compile-time expression templates for mathematical operations
 */
template<typename E>
class VectorExpression {
public:
    const E& self() const { return static_cast<const E&>(*this); }
    std::size_t size() const { return self().size(); }
    auto operator[](std::size_t i) const { return self()[i]; }
};

template<typename E1, typename E2, typename Op>
class VectorBinaryOp : public VectorExpression<VectorBinaryOp<E1, E2, Op>> {
private:
    const E1& lhs_;
    const E2& rhs_;
    Op op_;
    
public:
    VectorBinaryOp(const E1& lhs, const E2& rhs, Op op = Op{}) 
        : lhs_(lhs), rhs_(rhs), op_(op) {}
    
    std::size_t size() const { return lhs_.size(); }
    
    auto operator[](std::size_t i) const {
        return op_(lhs_[i], rhs_[i]);
    }
};

template<typename T>
class Vector : public VectorExpression<Vector<T>> {
private:
    std::vector<T> data_;
    
public:
    explicit Vector(std::size_t size) : data_(size) {}
    Vector(std::initializer_list<T> init) : data_(init) {}
    
    template<typename E>
    Vector(const VectorExpression<E>& expr) : data_(expr.size()) {
        for (std::size_t i = 0; i < expr.size(); ++i) {
            data_[i] = expr[i];
        }
    }
    
    std::size_t size() const { return data_.size(); }
    T& operator[](std::size_t i) { return data_[i]; }
    const T& operator[](std::size_t i) const { return data_[i]; }
    
    template<typename E>
    Vector& operator=(const VectorExpression<E>& expr) {
        if (data_.size() != expr.size()) {
            data_.resize(expr.size());
        }
        for (std::size_t i = 0; i < expr.size(); ++i) {
            data_[i] = expr[i];
        }
        return *this;
    }
};

// Vector operations using expression templates
template<typename E1, typename E2>
auto operator+(const VectorExpression<E1>& lhs, const VectorExpression<E2>& rhs) {
    return VectorBinaryOp(lhs.self(), rhs.self(), std::plus<>{});
}

template<typename E1, typename E2>
auto operator-(const VectorExpression<E1>& lhs, const VectorExpression<E2>& rhs) {
    return VectorBinaryOp(lhs.self(), rhs.self(), std::minus<>{});
}

template<typename E1, typename E2>
auto operator*(const VectorExpression<E1>& lhs, const VectorExpression<E2>& rhs) {
    return VectorBinaryOp(lhs.self(), rhs.self(), std::multiplies<>{});
}

/**
 * @brief Compile-time sorting algorithms
 */
template<typename T, std::size_t N>
constexpr std::array<T, N> bubble_sort(std::array<T, N> arr) {
    for (std::size_t i = 0; i < N; ++i) {
        for (std::size_t j = 0; j < N - i - 1; ++j) {
            if (arr[j] > arr[j + 1]) {
                T temp = arr[j];
                arr[j] = arr[j + 1];
                arr[j + 1] = temp;
            }
        }
    }
    return arr;
}

template<typename T, std::size_t N>
constexpr std::array<T, N> insertion_sort(std::array<T, N> arr) {
    for (std::size_t i = 1; i < N; ++i) {
        T key = arr[i];
        std::size_t j = i;
        
        while (j > 0 && arr[j - 1] > key) {
            arr[j] = arr[j - 1];
            --j;
        }
        arr[j] = key;
    }
    return arr;
}

/**
 * @brief Template-based compile-time map
 */
template<typename Key, typename Value>
struct pair {
    Key key;
    Value value;
    
    constexpr pair(Key k, Value v) : key(k), value(v) {}
};

template<typename... Pairs>
class compile_time_map {
private:
    std::tuple<Pairs...> data_;
    
    template<typename Key, std::size_t Index>
    constexpr auto find_impl() const {
        if constexpr (Index >= sizeof...(Pairs)) {
            return std::nullopt;
        } else {
            const auto& p = std::get<Index>(data_);
            if constexpr (std::is_same_v<Key, decltype(p.key)>) {
                if (p.key == Key{}) {
                    return std::optional{p.value};
                }
            }
            return find_impl<Key, Index + 1>();
        }
    }
    
public:
    constexpr compile_time_map(Pairs... pairs) : data_(pairs...) {}
    
    template<typename Key>
    constexpr auto find() const {
        return find_impl<Key, 0>();
    }
    
    template<typename Key>
    constexpr bool contains() const {
        return find<Key>().has_value();
    }
    
    static constexpr std::size_t size() { return sizeof...(Pairs); }
};

// Helper function to create compile-time maps
template<typename... Pairs>
constexpr auto make_compile_time_map(Pairs... pairs) {
    return compile_time_map<Pairs...>(pairs...);
}

/**
 * @brief Template-based design patterns
 */

// Singleton template
template<typename T>
class Singleton {
protected:
    Singleton() = default;
    ~Singleton() = default;
    Singleton(const Singleton&) = delete;
    Singleton& operator=(const Singleton&) = delete;
    Singleton(Singleton&&) = delete;
    Singleton& operator=(Singleton&&) = delete;
    
public:
    static T& getInstance() {
        static T instance;
        return instance;
    }
};

// Observer pattern template
template<typename Event>
class Observable {
private:
    std::vector<std::function<void(const Event&)>> observers_;
    
public:
    void subscribe(std::function<void(const Event&)> observer) {
        observers_.push_back(std::move(observer));
    }
    
    void notify(const Event& event) {
        for (const auto& observer : observers_) {
            observer(event);
        }
    }
    
    void clear_observers() {
        observers_.clear();
    }
};

// Command pattern template
template<typename Result = void>
class Command {
public:
    virtual ~Command() = default;
    virtual Result execute() = 0;
    virtual void undo() {}
};

template<typename Func, typename Result = std::invoke_result_t<Func>>
class FunctionCommand : public Command<Result> {
private:
    Func func_;
    
public:
    explicit FunctionCommand(Func f) : func_(std::move(f)) {}
    
    Result execute() override {
        return func_();
    }
};

template<typename Func>
auto make_command(Func&& func) {
    return std::make_unique<FunctionCommand<std::decay_t<Func>>>(std::forward<Func>(func));
}

} // namespace Meta
} // namespace Templates
} // namespace CppVerseHub

#endif // META_PROGRAMMING_HPP