/**
 * @file MetaProgramming.hpp
 * @brief Template metaprogramming: computing with types and values at compile time.
 *
 * Demonstrates
 *  - type traits and type lists (indexing, searching, transforming, filtering);
 *  - recursive class-template computation vs. constexpr functions (factorial, Fibonacci, power);
 *  - constexpr containers/algorithms (prime sieve, sorting, a constexpr lookup map);
 *  - fixed strings usable as non-type template parameters, constexpr FNV-1a hashing;
 *  - compile-time loops (`static_for`) and tuple manipulation via index sequences;
 *  - CRTP (static polymorphism, operator mixins) and a Meyers singleton;
 *  - compile-time rational arithmetic and dimensional analysis (units checked by the type system);
 *  - expression templates that fuse vector arithmetic into one loop without temporaries;
 *  - a std::variant-based state machine driven by detected member functions.
 *
 * Every pure compile-time facility here is verified with static_assert in the tests.
 */

#ifndef CPPVERSEHUB_TEMPLATES_META_PROGRAMMING_HPP
#define CPPVERSEHUB_TEMPLATES_META_PROGRAMMING_HPP

/// Portable `[[no_unique_address]]`: MSVC ignores the standard spelling and needs its own attribute.
#ifndef CPPVERSEHUB_NO_UNIQUE_ADDRESS
#if defined(_MSC_VER) && !defined(__clang__)
#define CPPVERSEHUB_NO_UNIQUE_ADDRESS [[msvc::no_unique_address]]
#else
#define CPPVERSEHUB_NO_UNIQUE_ADDRESS [[no_unique_address]]
#endif
#endif

#include <array>
#include <cassert>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <initializer_list>
#include <iostream>
#include <memory>
#include <numeric>
#include <optional>
#include <stdexcept>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

namespace CppVerseHub::Templates::Meta {

// ===== Type traits =====

/**
 * @brief True if T is a specialization of the class template Template (type parameters only).
 * @tparam Template class template to test against
 * @tparam T type to inspect
 */
template <template <typename...> class Template, typename T>
struct is_instantiation_of : std::false_type {};

/** @brief Matching partial specialization. */
template <template <typename...> class Template, typename... Args>
struct is_instantiation_of<Template, Template<Args...>> : std::true_type {};

/** @brief Convenience variable for is_instantiation_of. */
template <template <typename...> class Template, typename T>
inline constexpr bool is_instantiation_of_v = is_instantiation_of<Template, std::remove_cv_t<T>>::value;

/** @brief True if T is the same as any of Types. */
template <typename T, typename... Types>
struct is_any_of : std::disjunction<std::is_same<T, Types>...> {};

/** @brief Convenience variable for is_any_of. */
template <typename T, typename... Types>
inline constexpr bool is_any_of_v = is_any_of<T, Types...>::value;

// ===== Type lists =====

/** @brief Sentinel index returned when a type is not found. */
inline constexpr std::size_t npos = static_cast<std::size_t>(-1);

/** @brief A compile-time sequence of types. */
template <typename... Types>
struct type_list {
    /** @brief Number of types in the list. */
    static constexpr std::size_t size = sizeof...(Types);
};

/** @brief The N-th type of a pack (recursive peeling). */
template <std::size_t N, typename... Types>
struct type_at;

/** @brief Recursive case: drop the head. */
template <std::size_t N, typename Head, typename... Tail>
struct type_at<N, Head, Tail...> {
    using type = typename type_at<N - 1, Tail...>::type;
};

/** @brief Base case: N == 0 selects the head. */
template <typename Head, typename... Tail>
struct type_at<0, Head, Tail...> {
    using type = Head;
};

/** @brief Alias for type_at. */
template <std::size_t N, typename... Types>
using type_at_t = typename type_at<N, Types...>::type;

/**
 * @brief Index of T in Types (first match) or npos, computed by a short-circuiting fold instead of recursion.
 *
 * The `||` fold stops at the first match; `index` counts the non-matching types before it. An empty pack
 * folds to `false`, so no loop bound is ever compared against zero.
 */
template <typename T, typename... Types>
inline constexpr std::size_t type_index_v = [] {
    std::size_t index = 0;
    const bool found = ((std::is_same_v<T, Types> ? true : (++index, false)) || ...);
    return found ? index : npos;
}();

/** @brief True if T occurs in Types. */
template <typename T, typename... Types>
inline constexpr bool type_exists_v = (std::is_same_v<T, Types> || ...);

/** @brief Element access on a type_list. */
template <std::size_t N, typename List>
struct list_at;

/** @brief Unpack the list into type_at. */
template <std::size_t N, typename... Types>
struct list_at<N, type_list<Types...>> : type_at<N, Types...> {};

/** @brief Alias for list_at. */
template <std::size_t N, typename List>
using list_at_t = typename list_at<N, List>::type;

/** @brief Prepend T to a type_list. */
template <typename T, typename List>
struct push_front;

/** @brief Implementation. */
template <typename T, typename... Types>
struct push_front<T, type_list<Types...>> {
    using type = type_list<T, Types...>;
};

/** @brief Alias for push_front. */
template <typename T, typename List>
using push_front_t = typename push_front<T, List>::type;

/** @brief Append T to a type_list. */
template <typename T, typename List>
struct push_back;

/** @brief Implementation. */
template <typename T, typename... Types>
struct push_back<T, type_list<Types...>> {
    using type = type_list<Types..., T>;
};

/** @brief Alias for push_back. */
template <typename T, typename List>
using push_back_t = typename push_back<T, List>::type;

/** @brief Apply a metafunction F<T>::type to every element. */
template <typename List, template <typename> class F>
struct transform;

/** @brief Implementation. */
template <template <typename> class F, typename... Types>
struct transform<type_list<Types...>, F> {
    using type = type_list<typename F<Types>::type...>;
};

/** @brief Alias for transform. */
template <typename List, template <typename> class F>
using transform_t = typename transform<List, F>::type;

/** @brief Concatenate two type_lists. */
template <typename L1, typename L2>
struct concat;

/** @brief Implementation. */
template <typename... A, typename... B>
struct concat<type_list<A...>, type_list<B...>> {
    using type = type_list<A..., B...>;
};

/** @brief Alias for concat. */
template <typename L1, typename L2>
using concat_t = typename concat<L1, L2>::type;

/** @brief Keep the elements for which Pred<T>::value is true. */
template <typename List, template <typename> class Pred>
struct filter;

/** @brief Base case. */
template <template <typename> class Pred>
struct filter<type_list<>, Pred> {
    using type = type_list<>;
};

/** @brief Recursive case. */
template <template <typename> class Pred, typename Head, typename... Tail>
struct filter<type_list<Head, Tail...>, Pred> {
    using rest = typename filter<type_list<Tail...>, Pred>::type;
    using type = std::conditional_t<Pred<Head>::value, push_front_t<Head, rest>, rest>;
};

/** @brief Alias for filter. */
template <typename List, template <typename> class Pred>
using filter_t = typename filter<List, Pred>::type;

// ===== Compile-time arithmetic =====

/** @brief Factorial by recursive template instantiation (C++98 style). */
template <std::size_t N>
struct factorial {
    /** @brief N! */
    static constexpr std::size_t value = N * factorial<N - 1>::value;
};

/** @brief Base case 0! = 1. */
template <>
struct factorial<0> {
    /** @brief 0! */
    static constexpr std::size_t value = 1;
};

/** @brief Convenience variable for factorial. */
template <std::size_t N>
inline constexpr std::size_t factorial_v = factorial<N>::value;

/** @brief Factorial as a constexpr function. @param n argument @return n! */
[[nodiscard]] constexpr std::size_t factorial_func(std::size_t n) noexcept {
    std::size_t result = 1;
    for (std::size_t i = 2; i <= n; ++i) {
        result *= i;
    }
    return result;
}

/** @brief Fibonacci by recursive instantiation; memoised for free by the compiler. */
template <std::size_t N>
struct fibonacci {
    /** @brief F(N) */
    static constexpr std::size_t value = fibonacci<N - 1>::value + fibonacci<N - 2>::value;
};

/** @brief F(0) = 0. */
template <>
struct fibonacci<0> {
    /** @brief F(0) */
    static constexpr std::size_t value = 0;
};

/** @brief F(1) = 1. */
template <>
struct fibonacci<1> {
    /** @brief F(1) */
    static constexpr std::size_t value = 1;
};

/** @brief Convenience variable for fibonacci. */
template <std::size_t N>
inline constexpr std::size_t fibonacci_v = fibonacci<N>::value;

/** @brief Iterative Fibonacci. @param n index @return F(n) */
[[nodiscard]] constexpr std::size_t fibonacci_func(std::size_t n) noexcept {
    std::size_t a = 0;
    std::size_t b = 1;
    for (std::size_t i = 0; i < n; ++i) {
        const std::size_t next = a + b;
        a = b;
        b = next;
    }
    return a;
}

/** @brief Base^Exponent by recursive instantiation. */
template <std::size_t Base, std::size_t Exponent>
struct power {
    /** @brief Base^Exponent */
    static constexpr std::size_t value = Base * power<Base, Exponent - 1>::value;
};

/** @brief Partial specialization: x^0 = 1. */
template <std::size_t Base>
struct power<Base, 0> {
    /** @brief 1 */
    static constexpr std::size_t value = 1;
};

/** @brief Convenience variable for power. */
template <std::size_t Base, std::size_t Exponent>
inline constexpr std::size_t power_v = power<Base, Exponent>::value;

/** @brief Fast exponentiation. @param base base @param exp exponent @return base^exp (mod 2^64) */
[[nodiscard]] constexpr std::size_t power_func(std::size_t base, std::size_t exp) noexcept {
    std::size_t result = 1;
    while (exp > 0) {
        if ((exp & 1U) != 0U) {
            result *= base;
        }
        exp >>= 1U;
        if (exp > 0) {
            base *= base;
        }
    }
    return result;
}

/** @brief Greatest common divisor by Euclid's algorithm, as a template. */
template <std::size_t A, std::size_t B>
struct gcd {
    /** @brief gcd(A, B) */
    static constexpr std::size_t value = gcd<B, A % B>::value;
};

/** @brief Base case gcd(A, 0) = A. */
template <std::size_t A>
struct gcd<A, 0> {
    /** @brief A */
    static constexpr std::size_t value = A;
};

/** @brief Convenience variable for gcd. */
template <std::size_t A, std::size_t B>
inline constexpr std::size_t gcd_v = gcd<A, B>::value;

/** @brief Trial-division primality. @param n candidate @return true if n is prime */
[[nodiscard]] constexpr bool is_prime(std::size_t n) noexcept {
    if (n < 2) {
        return false;
    }
    if (n % 2 == 0) {
        return n == 2;
    }
    for (std::size_t i = 3; i <= n / i; i += 2) {
        if (n % i == 0) {
            return false;
        }
    }
    return true;
}

/** @brief Number of primes <= n. @param n upper bound @return pi(n) */
[[nodiscard]] constexpr std::size_t prime_count(std::size_t n) noexcept {
    std::size_t count = 0;
    for (std::size_t i = 2; i <= n; ++i) {
        count += is_prime(i) ? 1U : 0U;
    }
    return count;
}

/** @brief All primes <= N in an array whose size is itself computed at compile time. @return primes */
template <std::size_t N>
[[nodiscard]] constexpr std::array<std::size_t, prime_count(N)> generate_primes() noexcept {
    std::array<std::size_t, prime_count(N)> primes{};
    std::size_t index = 0;
    for (std::size_t i = 2; i <= N; ++i) {
        if (is_prime(i)) {
            primes[index++] = i;
        }
    }
    return primes;
}

// ===== Compile-time strings =====

/**
 * @brief Fixed-capacity string that is a *structural type*, so it can be a template argument.
 * @tparam N buffer size including the terminating NUL
 */
template <std::size_t N>
struct compile_time_string {
    char data[N]{};          ///< NUL-terminated character buffer
    std::size_t length = 0;  ///< number of characters (excluding NUL)

    /** @brief Empty string. */
    constexpr compile_time_string() noexcept = default;

    /** @brief From a string literal. @param str literal */
    constexpr compile_time_string(const char (&str)[N]) noexcept : length(N - 1) { // NOLINT
        for (std::size_t i = 0; i < N; ++i) {
            data[i] = str[i];
        }
    }

    /** @param i index @return character i */
    [[nodiscard]] constexpr char operator[](std::size_t i) const noexcept { return data[i]; }
    /** @return number of characters */
    [[nodiscard]] constexpr std::size_t size() const noexcept { return length; }
    /** @return NUL-terminated buffer */
    [[nodiscard]] constexpr const char* c_str() const noexcept { return data; }
    /** @return view of the characters */
    [[nodiscard]] constexpr std::string_view view() const noexcept { return {data, length}; }

    /** @return true if the character sequences are equal */
    template <std::size_t M>
    [[nodiscard]] constexpr bool operator==(const compile_time_string<M>& other) const noexcept {
        return view() == other.view();
    }
};

/** @brief Deduce capacity from a literal. */
template <std::size_t N>
compile_time_string(const char (&)[N]) -> compile_time_string<N>;

/**
 * @brief Concatenate two compile-time strings.
 * @param lhs first
 * @param rhs second
 * @return string of capacity N1 + N2 - 1
 */
template <std::size_t N1, std::size_t N2>
[[nodiscard]] constexpr auto operator+(const compile_time_string<N1>& lhs,
                                       const compile_time_string<N2>& rhs) noexcept {
    compile_time_string<N1 + N2 - 1> result;
    for (std::size_t i = 0; i < lhs.length; ++i) {
        result.data[i] = lhs.data[i];
    }
    for (std::size_t i = 0; i < rhs.length; ++i) {
        result.data[lhs.length + i] = rhs.data[i];
    }
    result.length = lhs.length + rhs.length;
    result.data[result.length] = '\0';
    return result;
}

/**
 * @brief A type tagged with a string; the string is part of the type (class-type NTTP, C++20).
 * @tparam Name compile-time name
 */
template <compile_time_string Name>
struct named_tag {
    /** @brief The name carried by the type. */
    static constexpr std::string_view name = Name.view();
};

/** @brief 64-bit FNV-1a hash, usable in constant expressions (e.g. switch on strings). @param str input @return hash */
[[nodiscard]] constexpr std::uint64_t hash_string(std::string_view str) noexcept {
    constexpr std::uint64_t fnv_offset_basis = 14695981039346656037ULL;
    constexpr std::uint64_t fnv_prime = 1099511628211ULL;
    std::uint64_t hash = fnv_offset_basis;
    for (const char c : str) {
        hash ^= static_cast<std::uint64_t>(static_cast<unsigned char>(c));
        hash *= fnv_prime;
    }
    return hash;
}

// ===== Compile-time loops and tuples =====

/**
 * @brief Invoke func with std::integral_constant<size_t, I> for I = 0..N-1 (in order).
 * @param func generic callable
 */
template <std::size_t N, typename Func>
constexpr void static_for(Func&& func) {
    [&]<std::size_t... I>(std::index_sequence<I...>) {
        (func(std::integral_constant<std::size_t, I>{}), ...);
    }(std::make_index_sequence<N>{});
}

/**
 * @brief Build a std::array whose i-th element is func(i), at compile time if func is constexpr.
 * @param func generator taking an integral_constant index
 * @return generated array
 */
template <typename T, std::size_t N, typename Func>
[[nodiscard]] constexpr std::array<T, N> generate_array(Func&& func) {
    std::array<T, N> arr{};
    static_for<N>([&](auto i) { arr[i] = static_cast<T>(func(i)); });
    return arr;
}

/**
 * @brief Reverse the element order of a tuple.
 * @param t tuple-like object
 * @return new tuple with reversed elements
 */
template <typename Tuple>
[[nodiscard]] constexpr auto tuple_reverse(Tuple&& t) {
    constexpr std::size_t size = std::tuple_size_v<std::remove_cvref_t<Tuple>>;
    return [&]<std::size_t... I>(std::index_sequence<I...>) {
        return std::make_tuple(std::get<size - 1 - I>(std::forward<Tuple>(t))...);
    }(std::make_index_sequence<size>{});
}

namespace detail {
template <template <typename> class Pred, typename E>
constexpr auto filter_one(const E& element) {
    if constexpr (Pred<E>::value) {
        return std::tuple<E>(element);
    } else {
        return std::tuple<>{};
    }
}
} // namespace detail

/**
 * @brief Keep the tuple elements whose type satisfies the trait Pred (e.g. std::is_integral).
 * @param t tuple-like object
 * @return tuple of the selected elements (copies)
 */
template <template <typename> class Pred, typename Tuple>
[[nodiscard]] constexpr auto tuple_filter(const Tuple& t) {
    return std::apply([](const auto&... elements) { return std::tuple_cat(detail::filter_one<Pred>(elements)...); },
                      t);
}

/**
 * @brief Apply func to every tuple element.
 * @param t tuple-like object
 * @param func generic callable
 * @return tuple of results
 */
template <typename Tuple, typename Func>
[[nodiscard]] constexpr auto tuple_transform(const Tuple& t, Func&& func) {
    return std::apply([&](const auto&... elements) { return std::make_tuple(func(elements)...); }, t);
}

// ===== CRTP =====

/**
 * @brief CRTP helper giving static access to the derived class.
 * @tparam Derived the class inheriting from CRTP_Base<Derived>
 */
template <typename Derived>
class CRTP_Base {
public:
    /** @return *this as Derived& */
    [[nodiscard]] constexpr Derived& derived() noexcept { return static_cast<Derived&>(*this); }
    /** @return *this as const Derived& */
    [[nodiscard]] constexpr const Derived& derived() const noexcept { return static_cast<const Derived&>(*this); }

protected:
    constexpr CRTP_Base() noexcept = default;
    constexpr ~CRTP_Base() = default;
    constexpr CRTP_Base(const CRTP_Base&) noexcept = default;
    constexpr CRTP_Base& operator=(const CRTP_Base&) noexcept = default;
    constexpr CRTP_Base(CRTP_Base&&) noexcept = default;
    constexpr CRTP_Base& operator=(CRTP_Base&&) noexcept = default;
};

/**
 * @brief Operator mixin: Derived provides `==` and `<`; this injects `!=`, `>`, `<=`, `>=` as
 *        hidden friends (the pre-C++20 alternative to a defaulted operator<=>).
 * @tparam Derived the class being completed
 */
template <typename Derived>
class TotallyOrdered : public CRTP_Base<Derived> {
public:
    /** @return !(lhs == rhs) */
    [[nodiscard]] friend constexpr bool operator!=(const Derived& lhs, const Derived& rhs) { return !(lhs == rhs); }
    /** @return rhs < lhs */
    [[nodiscard]] friend constexpr bool operator>(const Derived& lhs, const Derived& rhs) { return rhs < lhs; }
    /** @return !(rhs < lhs) */
    [[nodiscard]] friend constexpr bool operator<=(const Derived& lhs, const Derived& rhs) { return !(rhs < lhs); }
    /** @return !(lhs < rhs) */
    [[nodiscard]] friend constexpr bool operator>=(const Derived& lhs, const Derived& rhs) { return !(lhs < rhs); }
};

/**
 * @brief Static polymorphism: shared interface without virtual dispatch.
 *        Derived must implement `area_impl()` and `name_impl()`.
 * @tparam Derived concrete shape
 */
template <typename Derived>
class ShapeBase : public CRTP_Base<Derived> {
public:
    /** @return area computed by the derived class */
    [[nodiscard]] constexpr double area() const { return this->derived().area_impl(); }
    /** @return human-readable name from the derived class */
    [[nodiscard]] constexpr std::string_view name() const { return this->derived().name_impl(); }
};

/** @brief CRTP shape example: a circle. */
class CircleShape : public ShapeBase<CircleShape> {
public:
    /** @param radius circle radius */
    constexpr explicit CircleShape(double radius) noexcept : radius_(radius) {}
    /** @return pi * r^2 */
    [[nodiscard]] constexpr double area_impl() const noexcept { return 3.14159265358979323846 * radius_ * radius_; }
    /** @return "circle" */
    [[nodiscard]] constexpr std::string_view name_impl() const noexcept { return "circle"; }

private:
    double radius_;
};

/** @brief CRTP shape example: an axis-aligned rectangle. */
class RectangleShape : public ShapeBase<RectangleShape> {
public:
    /** @param width width @param height height */
    constexpr RectangleShape(double width, double height) noexcept : width_(width), height_(height) {}
    /** @return width * height */
    [[nodiscard]] constexpr double area_impl() const noexcept { return width_ * height_; }
    /** @return "rectangle" */
    [[nodiscard]] constexpr std::string_view name_impl() const noexcept { return "rectangle"; }

private:
    double width_;
    double height_;
};

/**
 * @brief Sum the areas of heterogeneous CRTP shapes with a fold expression (no virtual calls).
 * @param shapes shapes deriving from ShapeBase
 * @return total area
 */
template <typename... Shapes>
[[nodiscard]] constexpr double total_area(const ShapeBase<Shapes>&... shapes) {
    return (0.0 + ... + shapes.area());
}

// ===== Compile-time rational arithmetic =====

/**
 * @brief Rational number Num/Den normalised at compile time (sign in the numerator, lowest terms).
 *        Overflow of intermediate products is not checked.
 */
template <std::intmax_t Num, std::intmax_t Den = 1>
struct Ratio {
    static_assert(Den != 0, "Ratio denominator must be non-zero");

private:
    static constexpr std::intmax_t abs_value(std::intmax_t x) noexcept { return x < 0 ? -x : x; }
    static constexpr std::intmax_t divisor = std::gcd(abs_value(Num), abs_value(Den));

public:
    /** @brief Normalised numerator. */
    static constexpr std::intmax_t num = (Den < 0 ? -Num : Num) / divisor;
    /** @brief Normalised (positive) denominator. */
    static constexpr std::intmax_t den = abs_value(Den) / divisor;
    /** @brief The canonical type for this value. */
    using type = Ratio<num, den>;
    /** @return value as double */
    [[nodiscard]] static constexpr double to_double() noexcept {
        return static_cast<double>(num) / static_cast<double>(den);
    }
};

/** @brief R1 + R2 (canonical). */
template <typename R1, typename R2>
using ratio_add = typename Ratio<R1::num * R2::den + R2::num * R1::den, R1::den * R2::den>::type;
/** @brief R1 - R2 (canonical). */
template <typename R1, typename R2>
using ratio_subtract = typename Ratio<R1::num * R2::den - R2::num * R1::den, R1::den * R2::den>::type;
/** @brief R1 * R2 (canonical). */
template <typename R1, typename R2>
using ratio_multiply = typename Ratio<R1::num * R2::num, R1::den * R2::den>::type;
/** @brief R1 / R2 (canonical). */
template <typename R1, typename R2>
using ratio_divide = typename Ratio<R1::num * R2::den, R1::den * R2::num>::type;
/** @brief True if R1 and R2 denote the same value. */
template <typename R1, typename R2>
inline constexpr bool ratio_equal_v = R1::num == R2::num && R1::den == R2::den;

// ===== Dimensional analysis =====

/** @brief Physical dimension as exponents of mass, length and time. */
template <int Mass, int Length, int Time>
struct dimension {
    static constexpr int mass = Mass;     ///< exponent of mass
    static constexpr int length = Length; ///< exponent of length
    static constexpr int time = Time;     ///< exponent of time
};

/** @brief Dimension of a product. */
template <typename D1, typename D2>
using dimension_multiply = dimension<D1::mass + D2::mass, D1::length + D2::length, D1::time + D2::time>;
/** @brief Dimension of a quotient. */
template <typename D1, typename D2>
using dimension_divide = dimension<D1::mass - D2::mass, D1::length - D2::length, D1::time - D2::time>;

using dimensionless = dimension<0, 0, 0>;          ///< pure number
using mass_dimension = dimension<1, 0, 0>;         ///< kg
using length_dimension = dimension<0, 1, 0>;       ///< m
using time_dimension = dimension<0, 0, 1>;         ///< s
using velocity_dimension = dimension<0, 1, -1>;    ///< m/s
using acceleration_dimension = dimension<0, 1, -2>; ///< m/s^2
using force_dimension = dimension<1, 1, -2>;       ///< N
using energy_dimension = dimension<1, 2, -2>;      ///< J

/**
 * @brief A value tagged with a physical dimension; mixing incompatible units fails to compile.
 * @tparam Rep numeric representation
 * @tparam Dim dimension<...>
 */
template <typename Rep, typename Dim = dimensionless>
class quantity {
public:
    using rep = Rep;
    using dimension_type = Dim;

    /** @brief Zero. */
    constexpr quantity() noexcept = default;
    /** @brief From a raw value. @param value magnitude in SI units */
    template <typename Rep2>
        requires std::convertible_to<Rep2, Rep>
    constexpr explicit quantity(const Rep2& value) noexcept : value_(static_cast<Rep>(value)) {}

    /** @return magnitude */
    [[nodiscard]] constexpr Rep count() const noexcept { return value_; }
    /** @return negated quantity */
    [[nodiscard]] constexpr quantity operator-() const noexcept { return quantity(-value_); }
    /** @param rhs same-dimension quantity @return *this */
    constexpr quantity& operator+=(const quantity& rhs) noexcept {
        value_ += rhs.value_;
        return *this;
    }
    /** @param rhs same-dimension quantity @return *this */
    constexpr quantity& operator-=(const quantity& rhs) noexcept {
        value_ -= rhs.value_;
        return *this;
    }
    /** @return comparison of magnitudes (same dimension only) */
    [[nodiscard]] constexpr auto operator<=>(const quantity&) const = default;

private:
    Rep value_{};
};

/** @return lhs + rhs (same dimension) */
template <typename R1, typename R2, typename D>
[[nodiscard]] constexpr auto operator+(const quantity<R1, D>& lhs, const quantity<R2, D>& rhs) noexcept {
    return quantity<std::common_type_t<R1, R2>, D>(lhs.count() + rhs.count());
}
/** @return lhs - rhs (same dimension) */
template <typename R1, typename R2, typename D>
[[nodiscard]] constexpr auto operator-(const quantity<R1, D>& lhs, const quantity<R2, D>& rhs) noexcept {
    return quantity<std::common_type_t<R1, R2>, D>(lhs.count() - rhs.count());
}
/** @return lhs * rhs with dimensions added */
template <typename R1, typename R2, typename D1, typename D2>
[[nodiscard]] constexpr auto operator*(const quantity<R1, D1>& lhs, const quantity<R2, D2>& rhs) noexcept {
    return quantity<std::common_type_t<R1, R2>, dimension_multiply<D1, D2>>(lhs.count() * rhs.count());
}
/** @return lhs / rhs with dimensions subtracted */
template <typename R1, typename R2, typename D1, typename D2>
[[nodiscard]] constexpr auto operator/(const quantity<R1, D1>& lhs, const quantity<R2, D2>& rhs) noexcept {
    return quantity<std::common_type_t<R1, R2>, dimension_divide<D1, D2>>(lhs.count() / rhs.count());
}
/** @return quantity scaled by an arithmetic scalar */
template <typename R1, typename S, typename D>
    requires std::is_arithmetic_v<S>
[[nodiscard]] constexpr auto operator*(const quantity<R1, D>& lhs, const S& scalar) noexcept {
    return quantity<std::common_type_t<R1, S>, D>(lhs.count() * scalar);
}
/** @return quantity scaled by an arithmetic scalar */
template <typename S, typename R2, typename D>
    requires std::is_arithmetic_v<S>
[[nodiscard]] constexpr auto operator*(const S& scalar, const quantity<R2, D>& rhs) noexcept {
    return quantity<std::common_type_t<S, R2>, D>(scalar * rhs.count());
}
/** @return quantity divided by an arithmetic scalar */
template <typename R1, typename S, typename D>
    requires std::is_arithmetic_v<S>
[[nodiscard]] constexpr auto operator/(const quantity<R1, D>& lhs, const S& scalar) noexcept {
    return quantity<std::common_type_t<R1, S>, D>(lhs.count() / scalar);
}

using mass = quantity<double, mass_dimension>;                 ///< kilograms
using length = quantity<double, length_dimension>;             ///< metres
using time_duration = quantity<double, time_dimension>;        ///< seconds
using velocity = quantity<double, velocity_dimension>;         ///< metres per second
using acceleration = quantity<double, acceleration_dimension>; ///< metres per second squared
using force = quantity<double, force_dimension>;               ///< newtons
using energy = quantity<double, energy_dimension>;             ///< joules

// ===== Variant-based state machine =====

/**
 * @brief Finite state machine whose states are types. A state reacts to an event if it has a member
 *        `on_event(const Event&)`; the returned value (a state, or a std::variant of states)
 *        becomes the new state. Unhandled events are ignored.
 * @tparam States the state types
 */
/// @brief A state type S reacts to event E if it has a callable `on_event(const E&)`.
template <typename S, typename E>
concept HandlesEvent = requires(S& state, const E& event) { state.on_event(event); };

template <typename... States>
class StateMachine {
public:
    using state_variant = std::variant<States...>;

    /** @param initial initial state */
    template <typename Initial>
        requires(std::is_same_v<std::remove_cvref_t<Initial>, States> || ...)
    explicit StateMachine(Initial&& initial) : state_(std::forward<Initial>(initial)) {}

    /**
     * @brief Dispatch an event to the current state.
     * @param event event object
     * @return true if the current state handled the event
     */
    template <typename Event>
    bool process_event(const Event& event) {
        std::optional<state_variant> next;
        const bool handled = std::visit(
            [&](auto& state) -> bool {
                // A named concept rather than an inline requires-expression: MSVC mis-evaluates the
                // latter inside a generic lambda and treats every event as unhandled.
                if constexpr (HandlesEvent<std::remove_reference_t<decltype(state)>, Event>) {
                    next.emplace(state.on_event(event));
                    return true;
                } else {
                    return false;
                }
            },
            state_);
        if (next) {
            state_ = std::move(*next); // assign after visit: never destroy the visited alternative mid-call
        }
        return handled;
    }

    /** @return true if the current state is State */
    template <typename State>
    [[nodiscard]] bool is_state() const noexcept {
        return std::holds_alternative<State>(state_);
    }

    /** @return pointer to the current state if it is State, else nullptr */
    template <typename State>
    [[nodiscard]] const State* get_state() const noexcept {
        return std::get_if<State>(&state_);
    }

    /** @param visitor callable over all states @return visitor's result */
    template <typename Visitor>
    decltype(auto) visit(Visitor&& visitor) const {
        return std::visit(std::forward<Visitor>(visitor), state_);
    }

private:
    state_variant state_;
};

// ===== Expression templates =====

template <typename T>
class ExprVector;

/** @brief Leaves (ExprVector) are held by reference, intermediate nodes by value (no dangling). */
template <typename E>
struct expression_storage {
    using type = E;
};

/** @brief Leaf storage. */
template <typename T>
struct expression_storage<ExprVector<T>> {
    using type = const ExprVector<T>&;
};

/**
 * @brief CRTP base of all vector expressions.
 * @tparam E concrete expression
 */
template <typename E>
class VectorExpression {
public:
    /** @return the concrete expression */
    [[nodiscard]] constexpr const E& self() const noexcept { return static_cast<const E&>(*this); }
    /** @return number of elements */
    [[nodiscard]] constexpr std::size_t size() const noexcept { return self().size(); }
    /** @param i index @return element i (computed lazily) */
    [[nodiscard]] constexpr auto operator[](std::size_t i) const { return self()[i]; }

protected:
    constexpr VectorExpression() noexcept = default;
};

/**
 * @brief Lazy element-wise binary operation node.
 * @tparam E1 left expression
 * @tparam E2 right expression
 * @tparam Op element-wise binary functor
 */
template <typename E1, typename E2, typename Op>
class VectorBinaryOp : public VectorExpression<VectorBinaryOp<E1, E2, Op>> {
public:
    /** @param lhs left operand @param rhs right operand @param op functor */
    constexpr VectorBinaryOp(const E1& lhs, const E2& rhs, Op op = Op{}) : lhs_(lhs), rhs_(rhs), op_(op) {
        assert(lhs.size() == rhs.size());
    }
    /** @return element count */
    [[nodiscard]] constexpr std::size_t size() const noexcept { return lhs_.size(); }
    /** @param i index @return op(lhs[i], rhs[i]) */
    [[nodiscard]] constexpr auto operator[](std::size_t i) const { return op_(lhs_[i], rhs_[i]); }

private:
    typename expression_storage<E1>::type lhs_;
    typename expression_storage<E2>::type rhs_;
    CPPVERSEHUB_NO_UNIQUE_ADDRESS Op op_;
};

/**
 * @brief Lazy scalar * expression node.
 * @tparam S scalar type
 * @tparam E expression
 */
template <typename S, typename E>
class VectorScaleOp : public VectorExpression<VectorScaleOp<S, E>> {
public:
    /** @param scalar factor @param expr expression */
    constexpr VectorScaleOp(S scalar, const E& expr) : scalar_(scalar), expr_(expr) {}
    /** @return element count */
    [[nodiscard]] constexpr std::size_t size() const noexcept { return expr_.size(); }
    /** @param i index @return scalar * expr[i] */
    [[nodiscard]] constexpr auto operator[](std::size_t i) const { return scalar_ * expr_[i]; }

private:
    S scalar_;
    typename expression_storage<E>::type expr_;
};

/**
 * @brief Concrete vector; evaluating an expression into it runs a single fused loop.
 * @tparam T element type
 */
template <typename T>
class ExprVector : public VectorExpression<ExprVector<T>> {
public:
    /** @param size number of zero elements */
    explicit ExprVector(std::size_t size) : data_(size) {}
    /** @param init elements */
    ExprVector(std::initializer_list<T> init) : data_(init) {}
    /** @brief Evaluate an expression. @param expr expression to materialise */
    template <typename E>
    ExprVector(const VectorExpression<E>& expr) : data_(expr.size()) { // NOLINT(google-explicit-constructor)
        assign(expr);
    }
    /** @brief Evaluate an expression into *this. @param expr expression @return *this */
    template <typename E>
    ExprVector& operator=(const VectorExpression<E>& expr) {
        data_.resize(expr.size());
        assign(expr);
        return *this;
    }

    /** @return element count */
    [[nodiscard]] std::size_t size() const noexcept { return data_.size(); }
    /** @param i index @return element */
    [[nodiscard]] T& operator[](std::size_t i) noexcept { return data_[i]; }
    /** @param i index @return element */
    [[nodiscard]] const T& operator[](std::size_t i) const noexcept { return data_[i]; }
    /** @return underlying storage */
    [[nodiscard]] const std::vector<T>& values() const noexcept { return data_; }

private:
    template <typename E>
    void assign(const VectorExpression<E>& expr) {
        for (std::size_t i = 0; i < data_.size(); ++i) {
            data_[i] = static_cast<T>(expr[i]);
        }
    }

    std::vector<T> data_;
};

/** @return lazy lhs + rhs */
template <typename E1, typename E2>
[[nodiscard]] constexpr auto operator+(const VectorExpression<E1>& lhs, const VectorExpression<E2>& rhs) {
    return VectorBinaryOp<E1, E2, std::plus<>>(lhs.self(), rhs.self());
}
/** @return lazy lhs - rhs */
template <typename E1, typename E2>
[[nodiscard]] constexpr auto operator-(const VectorExpression<E1>& lhs, const VectorExpression<E2>& rhs) {
    return VectorBinaryOp<E1, E2, std::minus<>>(lhs.self(), rhs.self());
}
/** @return lazy element-wise lhs * rhs */
template <typename E1, typename E2>
[[nodiscard]] constexpr auto operator*(const VectorExpression<E1>& lhs, const VectorExpression<E2>& rhs) {
    return VectorBinaryOp<E1, E2, std::multiplies<>>(lhs.self(), rhs.self());
}
/** @return lazy scalar * expr */
template <typename S, typename E>
    requires std::is_arithmetic_v<S>
[[nodiscard]] constexpr auto operator*(S scalar, const VectorExpression<E>& expr) {
    return VectorScaleOp<S, E>(scalar, expr.self());
}

// ===== Constexpr algorithms and containers =====

/** @brief Bubble sort usable in constant expressions. @param arr input (by value) @return sorted copy */
template <typename T, std::size_t N>
[[nodiscard]] constexpr std::array<T, N> bubble_sort(std::array<T, N> arr) {
    for (std::size_t i = 0; i + 1 < N; ++i) {
        for (std::size_t j = 0; j + 1 < N - i; ++j) {
            if (arr[j + 1] < arr[j]) {
                std::swap(arr[j], arr[j + 1]);
            }
        }
    }
    return arr;
}

/** @brief Insertion sort usable in constant expressions. @param arr input (by value) @return sorted copy */
template <typename T, std::size_t N>
[[nodiscard]] constexpr std::array<T, N> insertion_sort(std::array<T, N> arr) {
    for (std::size_t i = 1; i < N; ++i) {
        T key = arr[i];
        std::size_t j = i;
        while (j > 0 && key < arr[j - 1]) {
            arr[j] = arr[j - 1];
            --j;
        }
        arr[j] = key;
    }
    return arr;
}

/**
 * @brief Fixed-size, linear-search key/value map that can live entirely in a constant expression.
 * @tparam Key key type (equality comparable, literal)
 * @tparam Value value type (literal)
 * @tparam N number of entries
 */
template <typename Key, typename Value, std::size_t N>
class ConstexprMap {
public:
    /** @param entries key/value pairs */
    constexpr explicit ConstexprMap(std::array<std::pair<Key, Value>, N> entries) : entries_(entries) {}

    /** @param key key to look up @return the value, or nullopt */
    [[nodiscard]] constexpr std::optional<Value> find(const Key& key) const {
        for (const auto& [k, v] : entries_) {
            if (k == key) {
                return v;
            }
        }
        return std::nullopt;
    }
    /** @param key key to look up @return true if present */
    [[nodiscard]] constexpr bool contains(const Key& key) const { return find(key).has_value(); }
    /** @param key key to look up @return value @throws std::out_of_range if absent */
    [[nodiscard]] constexpr Value at(const Key& key) const {
        const auto found = find(key);
        if (!found) {
            throw std::out_of_range("ConstexprMap::at: key not found");
        }
        return *found;
    }
    /** @return number of entries */
    [[nodiscard]] static constexpr std::size_t size() noexcept { return N; }

private:
    std::array<std::pair<Key, Value>, N> entries_;
};

/**
 * @brief Build a ConstexprMap from pairs, deducing the size.
 * @param first first entry
 * @param rest remaining entries (same pair type)
 * @return the map
 */
template <typename Key, typename Value, typename... Rest>
    requires(std::is_same_v<Rest, std::pair<Key, Value>> && ...)
[[nodiscard]] constexpr auto make_constexpr_map(std::pair<Key, Value> first, Rest... rest) {
    return ConstexprMap<Key, Value, 1 + sizeof...(Rest)>({{first, rest...}});
}

// ===== Template-based design patterns =====

/**
 * @brief CRTP Meyers singleton: thread-safe lazy initialisation via a function-local static.
 *        Derived must befriend Singleton<Derived> and keep its constructor private.
 * @tparam T the singleton class
 */
template <typename T>
class Singleton {
public:
    Singleton(const Singleton&) = delete;
    Singleton& operator=(const Singleton&) = delete;
    Singleton(Singleton&&) = delete;
    Singleton& operator=(Singleton&&) = delete;

    /** @return the unique instance */
    [[nodiscard]] static T& instance() {
        static T inst;
        return inst;
    }

protected:
    Singleton() = default;
    ~Singleton() = default;
};

/**
 * @brief Observer pattern parameterised on the event type.
 * @tparam Event event payload
 */
template <typename Event>
class Observable {
public:
    using handler_type = std::function<void(const Event&)>;
    using subscription_id = std::size_t;

    /** @param observer callback @return id usable with unsubscribe */
    subscription_id subscribe(handler_type observer) {
        const subscription_id id = next_id_++;
        observers_.emplace_back(id, std::move(observer));
        return id;
    }
    /** @param id subscription to remove @return true if it existed */
    bool unsubscribe(subscription_id id) {
        const auto old_size = observers_.size();
        std::erase_if(observers_, [id](const auto& entry) { return entry.first == id; });
        return observers_.size() != old_size;
    }
    /** @brief Deliver event to all observers in subscription order. @param event payload */
    void notify(const Event& event) const {
        for (const auto& entry : observers_) {
            entry.second(event);
        }
    }
    /** @return number of observers */
    [[nodiscard]] std::size_t observer_count() const noexcept { return observers_.size(); }
    /** @brief Remove all observers. */
    void clear_observers() noexcept { observers_.clear(); }

private:
    std::vector<std::pair<subscription_id, handler_type>> observers_;
    subscription_id next_id_ = 0;
};

/**
 * @brief Command pattern interface parameterised on the result type.
 * @tparam Result value produced by execute()
 */
template <typename Result = void>
class Command {
public:
    Command() = default;
    Command(const Command&) = default;
    Command& operator=(const Command&) = default;
    Command(Command&&) noexcept = default;
    Command& operator=(Command&&) noexcept = default;
    /** @brief Virtual destructor for polymorphic deletion. */
    virtual ~Command() = default;
    /** @return result of running the command */
    virtual Result execute() = 0;
    /** @brief Revert the command (default: no-op). */
    virtual void undo() {}
};

/**
 * @brief Adapts any nullary callable into a Command.
 * @tparam Func callable type
 */
template <typename Func, typename Result = std::invoke_result_t<Func&>>
class FunctionCommand final : public Command<Result> {
public:
    /** @param f callable to wrap */
    explicit FunctionCommand(Func f) : func_(std::move(f)) {}
    /** @return f() */
    Result execute() override { return std::invoke(func_); }

private:
    Func func_;
};

/**
 * @brief Wrap a callable in a heap-allocated Command.
 * @param func nullary callable
 * @return owning pointer to the command interface
 */
template <typename Func>
[[nodiscard]] auto make_command(Func&& func) {
    using F = std::decay_t<Func>;
    using R = std::invoke_result_t<F&>;
    return std::unique_ptr<Command<R>>(std::make_unique<FunctionCommand<F, R>>(std::forward<Func>(func)));
}

// ===== Showcase =====

/**
 * @brief Print the results of the compile-time facilities in this header.
 * @param out destination stream
 */
inline void demonstrate_metaprogramming(std::ostream& out = std::cout) {
    out << "--- Metaprogramming ---\n";
    out << "factorial<10>       = " << factorial_v<10> << '\n';
    out << "fibonacci<20>       = " << fibonacci_v<20> << '\n';
    out << "power<2, 16>        = " << power_v<2, 16> << '\n';
    out << "gcd<84, 36>         = " << gcd_v<84, 36> << '\n';

    constexpr auto primes = generate_primes<30>();
    out << "primes <= 30        =";
    for (const auto p : primes) {
        out << ' ' << p;
    }
    out << '\n';

    constexpr auto sorted = insertion_sort(std::array<int, 5>{5, 1, 4, 2, 3});
    out << "constexpr sorted    =";
    for (const auto v : sorted) {
        out << ' ' << v;
    }
    out << '\n';

    constexpr compile_time_string hello("Hello, ");
    constexpr compile_time_string world("World");
    constexpr auto greeting = hello + world;
    out << "concatenated        = " << greeting.view() << '\n';
    out << "named_tag name      = " << named_tag<"meta">::name << '\n';

    using third_ratio = ratio_add<Ratio<1, 6>, Ratio<1, 6>>;
    out << "1/6 + 1/6           = " << third_ratio::num << '/' << third_ratio::den << '\n';

    constexpr mass m{2.0};
    constexpr acceleration a{9.81};
    constexpr force f = m * a;
    out << "F = m * a           = " << f.count() << " N\n";

    const ExprVector<double> x{1.0, 2.0, 3.0};
    const ExprVector<double> y{4.0, 5.0, 6.0};
    const ExprVector<double> z = x + 2.0 * y - x * y;
    out << "x + 2y - x*y        =";
    for (const double v : z.values()) {
        out << ' ' << v;
    }
    out << '\n';

    const CircleShape circle(1.0);
    const RectangleShape rect(2.0, 3.0);
    out << "CRTP total area     = " << total_area(circle, rect) << '\n';

    constexpr auto map = make_constexpr_map(std::pair{std::string_view("one"), 1},
                                            std::pair{std::string_view("two"), 2});
    out << "ConstexprMap[two]   = " << map.at("two") << '\n';
}

} // namespace CppVerseHub::Templates::Meta

#endif // CPPVERSEHUB_TEMPLATES_META_PROGRAMMING_HPP
