/**
 * @file SFINAE_Examples.hpp
 * @brief SFINAE (Substitution Failure Is Not An Error) and its modern replacements.
 *
 * Demonstrates, roughly in historical order:
 *  - classic detection with overloaded `test(int)` / `test(...)` and trailing decltype;
 *  - `std::void_t` partial specialisation and the generic *detection idiom* (`is_detected`);
 *  - `std::enable_if_t` in return types to select overloads by type properties;
 *  - tag dispatch, and SFINAE in class-template partial specialisations;
 *  - the C++20 equivalent written with concepts (`modern_describe`), for comparison.
 *
 * All functions are pure (they return strings or values) so they are easy to test; nothing here
 * writes to a global stream.
 */

#ifndef CPPVERSEHUB_TEMPLATES_SFINAE_EXAMPLES_HPP
#define CPPVERSEHUB_TEMPLATES_SFINAE_EXAMPLES_HPP

#include <algorithm>
#include <concepts>
#include <cstddef>
#include <cstring>
#include <iostream>
#include <iterator>
#include <memory>
#include <sstream>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include "templates/MetaProgramming.hpp"

namespace CppVerseHub::Templates::SFINAE {

// ===== Classic detection: overloaded test functions =====

/**
 * @brief Detect a callable member `size()`. If `decltype(std::declval<U>().size())` is ill-formed,
 *        the first overload silently drops out and the variadic fallback is chosen.
 */
template <typename T>
class has_size_method {
    template <typename U>
    static auto test(int) -> decltype(std::declval<U&>().size(), std::true_type{});
    template <typename>
    static std::false_type test(...);

public:
    /** @brief true if T has a callable size() */
    static constexpr bool value = decltype(test<T>(0))::value;
};

/** @brief Convenience variable. */
template <typename T>
inline constexpr bool has_size_method_v = has_size_method<T>::value;

/** @brief Detect a callable member `begin()`. */
template <typename T>
class has_begin_method {
    template <typename U>
    static auto test(int) -> decltype(std::declval<U&>().begin(), std::true_type{});
    template <typename>
    static std::false_type test(...);

public:
    /** @brief true if T has a callable begin() */
    static constexpr bool value = decltype(test<T>(0))::value;
};

/** @brief Convenience variable. */
template <typename T>
inline constexpr bool has_begin_method_v = has_begin_method<T>::value;

/** @brief Detect a callable member `end()`. */
template <typename T>
class has_end_method {
    template <typename U>
    static auto test(int) -> decltype(std::declval<U&>().end(), std::true_type{});
    template <typename>
    static std::false_type test(...);

public:
    /** @brief true if T has a callable end() */
    static constexpr bool value = decltype(test<T>(0))::value;
};

/** @brief Convenience variable. */
template <typename T>
inline constexpr bool has_end_method_v = has_end_method<T>::value;

/** @brief True if T has member begin() and end(). */
template <typename T>
inline constexpr bool is_iterable_v = has_begin_method_v<T> && has_end_method_v<T>;

/**
 * @brief Generate a `has_member_<name>` trait detecting a data member or member function name.
 *        Macros are the only way to parameterise over an identifier.
 */
#define CPPVERSEHUB_DEFINE_HAS_MEMBER(member_name)                                                   \
    template <typename T, typename = void>                                                          \
    struct has_member_##member_name : std::false_type {};                                           \
    template <typename T>                                                                           \
    struct has_member_##member_name<T, std::void_t<decltype(&T::member_name)>> : std::true_type {}; \
    template <typename T>                                                                           \
    inline constexpr bool has_member_##member_name##_v = has_member_##member_name<T>::value

CPPVERSEHUB_DEFINE_HAS_MEMBER(data);
CPPVERSEHUB_DEFINE_HAS_MEMBER(value);
CPPVERSEHUB_DEFINE_HAS_MEMBER(first);
CPPVERSEHUB_DEFINE_HAS_MEMBER(second);

/** @brief Per-operator arithmetic support detection. */
template <typename T>
class supports_arithmetic {
    template <typename U>
    static auto test_add(int) -> decltype(std::declval<U>() + std::declval<U>(), std::true_type{});
    template <typename>
    static std::false_type test_add(...);
    template <typename U>
    static auto test_sub(int) -> decltype(std::declval<U>() - std::declval<U>(), std::true_type{});
    template <typename>
    static std::false_type test_sub(...);
    template <typename U>
    static auto test_mul(int) -> decltype(std::declval<U>() * std::declval<U>(), std::true_type{});
    template <typename>
    static std::false_type test_mul(...);
    template <typename U>
    static auto test_div(int) -> decltype(std::declval<U>() / std::declval<U>(), std::true_type{});
    template <typename>
    static std::false_type test_div(...);

public:
    static constexpr bool has_add = decltype(test_add<T>(0))::value; ///< supports a + b
    static constexpr bool has_sub = decltype(test_sub<T>(0))::value; ///< supports a - b
    static constexpr bool has_mul = decltype(test_mul<T>(0))::value; ///< supports a * b
    static constexpr bool has_div = decltype(test_div<T>(0))::value; ///< supports a / b
    /** @brief all four operators supported */
    static constexpr bool value = has_add && has_sub && has_mul && has_div;
};

/** @brief Convenience variable. */
template <typename T>
inline constexpr bool supports_arithmetic_v = supports_arithmetic<T>::value;

// ===== void_t and the detection idiom =====

/** @brief void_t-based detection of `os << value`. */
template <typename T, typename = void>
struct has_insertion_operator : std::false_type {};

/** @brief Selected when the stream expression is well-formed. */
template <typename T>
struct has_insertion_operator<T, std::void_t<decltype(std::declval<std::ostream&>() << std::declval<const T&>())>>
    : std::true_type {};

/** @brief Convenience variable. */
template <typename T>
inline constexpr bool has_insertion_operator_v = has_insertion_operator<T>::value;

namespace detail {
template <typename Default, typename AlwaysVoid, template <typename...> class Op, typename... Args>
struct detector {
    using value_t = std::false_type;
    using type = Default;
};

template <typename Default, template <typename...> class Op, typename... Args>
struct detector<Default, std::void_t<Op<Args...>>, Op, Args...> {
    using value_t = std::true_type;
    using type = Op<Args...>;
};
} // namespace detail

/** @brief Placeholder type returned by detected_t when detection fails. */
struct nonesuch {
    nonesuch() = delete;
    ~nonesuch() = delete;
    nonesuch(const nonesuch&) = delete;
    nonesuch& operator=(const nonesuch&) = delete;
};

/** @brief std::true_type if Op<Args...> is well-formed (Library Fundamentals TS v2 detection idiom). */
template <template <typename...> class Op, typename... Args>
using is_detected = typename detail::detector<nonesuch, void, Op, Args...>::value_t;

/** @brief Convenience variable for is_detected. */
template <template <typename...> class Op, typename... Args>
inline constexpr bool is_detected_v = is_detected<Op, Args...>::value;

/** @brief Op<Args...> if well-formed, otherwise nonesuch. */
template <template <typename...> class Op, typename... Args>
using detected_t = typename detail::detector<nonesuch, void, Op, Args...>::type;

/** @brief Op<Args...> if well-formed, otherwise Default. */
template <typename Default, template <typename...> class Op, typename... Args>
using detected_or_t = typename detail::detector<Default, void, Op, Args...>::type;

/** @brief Archetype: type of `t.size()`. */
template <typename T>
using size_expression_t = decltype(std::declval<T&>().size());

/** @brief Archetype: type of `t.push_back(v)`. */
template <typename T, typename V>
using push_back_expression_t = decltype(std::declval<T&>().push_back(std::declval<V>()));

/** @brief Archetype: nested `value_type`. */
template <typename T>
using value_type_t = typename T::value_type;

// ===== enable_if overload selection =====

/** @brief Describe an arithmetic value. @param value number @return description */
template <typename T>
[[nodiscard]] std::enable_if_t<std::is_arithmetic_v<T>, std::string> describe_value(const T& value) {
    std::ostringstream oss;
    oss << "arithmetic: " << value;
    return oss.str();
}

/** @brief Describe a string. @param value string @return description */
template <typename T>
[[nodiscard]] std::enable_if_t<std::is_same_v<T, std::string>, std::string> describe_value(const T& value) {
    return "string: \"" + value + "\"";
}

/** @brief Describe an iterable container. @param container container @return description */
template <typename T>
[[nodiscard]] std::enable_if_t<is_iterable_v<T> && !std::is_same_v<T, std::string>, std::string>
describe_value(const T& container) {
    std::ostringstream oss;
    oss << "container: [";
    bool first = true;
    for (const auto& item : container) {
        oss << (first ? "" : ", ") << item;
        first = false;
    }
    oss << ']';
    return oss.str();
}

/** @brief Fallback for anything else. @return description */
template <typename T>
[[nodiscard]] std::enable_if_t<!std::is_arithmetic_v<T> && !std::is_same_v<T, std::string> && !is_iterable_v<T>,
                               std::string>
describe_value(const T&) {
    return "opaque object";
}

/** @brief Size via member size(). @param container container @return element count */
template <typename T>
[[nodiscard]] std::enable_if_t<has_size_method_v<T>, std::size_t> get_size(const T& container) {
    return static_cast<std::size_t>(container.size());
}

/** @brief Size by walking begin()..end() when size() is unavailable. @param container range @return count */
template <typename T>
[[nodiscard]] std::enable_if_t<!has_size_method_v<T> && is_iterable_v<T>, std::size_t>
get_size(const T& container) {
    return static_cast<std::size_t>(std::distance(container.begin(), container.end()));
}

/** @brief Size of a built-in array, deduced from its type. @return N */
template <typename T, std::size_t N>
[[nodiscard]] constexpr std::size_t get_size(const T (&)[N]) noexcept {
    return N;
}

/** @brief Serialise a bool. @param value flag @return "true"/"false" */
[[nodiscard]] inline std::string serialize(bool value) {
    return value ? "true" : "false";
}

/** @brief Serialise a number. @param value number @return decimal text */
template <typename T>
[[nodiscard]] std::enable_if_t<std::is_arithmetic_v<T> && !std::is_same_v<T, bool>, std::string>
serialize(const T& value) {
    std::ostringstream oss;
    oss << value;
    return oss.str();
}

/** @brief Serialise a string with quotes. @param value text @return quoted text */
template <typename T>
[[nodiscard]] std::enable_if_t<std::is_same_v<T, std::string>, std::string> serialize(const T& value) {
    return "\"" + value + "\"";
}

/** @brief Serialise any iterable recursively as a JSON-like array. @param container range @return text */
template <typename T>
[[nodiscard]] std::enable_if_t<is_iterable_v<T> && !std::is_same_v<T, std::string>, std::string>
serialize(const T& container) {
    std::string result = "[";
    bool first = true;
    for (const auto& item : container) {
        if (!first) {
            result += ',';
        }
        result += serialize(item);
        first = false;
    }
    result += ']';
    return result;
}

/** @brief Print value if streamable. @param out stream @param value value */
template <typename T>
std::enable_if_t<has_insertion_operator_v<T>> safe_print(std::ostream& out, const T& value) {
    out << value << '\n';
}

/** @brief Placeholder for non-streamable values. @param out stream */
template <typename T>
std::enable_if_t<!has_insertion_operator_v<T>> safe_print(std::ostream& out, const T&) {
    out << "[non-printable object]\n";
}

// ===== Smart pointer detection =====

/** @brief Primary: not a standard smart pointer. */
template <typename T>
struct is_smart_pointer : std::false_type {};
/** @brief std::unique_ptr with any deleter. */
template <typename T, typename D>
struct is_smart_pointer<std::unique_ptr<T, D>> : std::true_type {};
/** @brief std::shared_ptr. */
template <typename T>
struct is_smart_pointer<std::shared_ptr<T>> : std::true_type {};

/** @brief Convenience variable. */
template <typename T>
inline constexpr bool is_smart_pointer_v = is_smart_pointer<std::remove_cv_t<T>>::value;

/** @brief Describe the pointee of a raw pointer. @param ptr pointer @return description */
template <typename Ptr>
[[nodiscard]] std::enable_if_t<std::is_pointer_v<Ptr>, std::string> safe_dereference(Ptr ptr) {
    if (ptr == nullptr) {
        return "null raw pointer";
    }
    std::ostringstream oss;
    oss << "raw pointer -> " << *ptr;
    return oss.str();
}

/** @brief Describe the pointee of a smart pointer. @param ptr pointer @return description */
template <typename Ptr>
[[nodiscard]] std::enable_if_t<is_smart_pointer_v<Ptr>, std::string> safe_dereference(const Ptr& ptr) {
    if (!ptr) {
        return "null smart pointer";
    }
    std::ostringstream oss;
    oss << "smart pointer -> " << *ptr;
    return oss.str();
}

// ===== Tag dispatch =====

struct container_tag {};  ///< dispatch tag for iterable types
struct arithmetic_tag {}; ///< dispatch tag for numbers
struct string_tag {};     ///< dispatch tag for std::string
struct generic_tag {};    ///< dispatch tag for everything else

/** @brief Compute the dispatch tag type for T. */
template <typename T>
using type_tag_t = std::conditional_t<
    std::is_arithmetic_v<T>, arithmetic_tag,
    std::conditional_t<std::is_same_v<T, std::string>, string_tag,
                       std::conditional_t<is_iterable_v<T>, container_tag, generic_tag>>>;

/** @brief Container overload. @param value container @return description */
template <typename T>
[[nodiscard]] std::string process_value(const T& value, container_tag) {
    return "container with " + std::to_string(get_size(value)) + " elements";
}
/** @brief Arithmetic overload. @param value number @return description */
template <typename T>
[[nodiscard]] std::string process_value(const T& value, arithmetic_tag) {
    return "arithmetic " + serialize(value);
}
/** @brief String overload. @param value text @return description */
template <typename T>
[[nodiscard]] std::string process_value(const T& value, string_tag) {
    return "string of length " + std::to_string(value.size());
}
/** @brief Generic overload. @return description */
template <typename T>
[[nodiscard]] std::string process_value(const T&, generic_tag) {
    return "generic object";
}
/** @brief Entry point that computes the tag and dispatches. @param value value @return description */
template <typename T>
[[nodiscard]] std::string process_value(const T& value) {
    return process_value(value, type_tag_t<T>{});
}

// ===== SFINAE in partial specialisations =====

/** @brief Generic advance: linear walk via std::advance. */
template <typename Iterator, typename = void>
struct algorithm_selector {
    static constexpr bool is_constant_time = false; ///< whether advance is O(1)
    /** @param it iterator @param n steps */
    static void advance(Iterator& it, typename std::iterator_traits<Iterator>::difference_type n) {
        std::advance(it, n);
    }
};

/** @brief Random-access iterators: O(1) `it += n`. */
template <typename Iterator>
struct algorithm_selector<Iterator,
                          std::enable_if_t<std::is_base_of_v<std::random_access_iterator_tag,
                                                             typename std::iterator_traits<Iterator>::iterator_category>>> {
    static constexpr bool is_constant_time = true; ///< whether advance is O(1)
    /** @param it iterator @param n steps */
    static void advance(Iterator& it, typename std::iterator_traits<Iterator>::difference_type n) { it += n; }
};

/**
 * @brief memcpy fast path for trivially copyable elements between raw pointers.
 * @param first source begin
 * @param last source end
 * @param dest destination (must not overlap)
 * @return dest + (last - first)
 */
template <typename InputIt, typename OutputIt>
std::enable_if_t<std::is_pointer_v<InputIt> && std::is_pointer_v<OutputIt> &&
                     std::is_trivially_copyable_v<std::remove_pointer_t<InputIt>> &&
                     std::is_same_v<std::remove_cv_t<std::remove_pointer_t<InputIt>>, std::remove_pointer_t<OutputIt>>,
                 OutputIt>
optimized_copy(InputIt first, InputIt last, OutputIt dest) {
    const auto count = static_cast<std::size_t>(last - first);
    if (count != 0) {
        std::memcpy(dest, first, count * sizeof(*first));
    }
    return dest + count;
}

/** @brief Generic fallback using std::copy. @param first begin @param last end @param dest out @return end of output */
template <typename InputIt, typename OutputIt>
std::enable_if_t<!(std::is_pointer_v<InputIt> && std::is_pointer_v<OutputIt> &&
                   std::is_trivially_copyable_v<std::remove_pointer_t<InputIt>> &&
                   std::is_same_v<std::remove_cv_t<std::remove_pointer_t<InputIt>>, std::remove_pointer_t<OutputIt>>),
                 OutputIt>
optimized_copy(InputIt first, InputIt last, OutputIt dest) {
    return std::copy(first, last, dest);
}

/** @brief Move if the move constructor cannot throw (re-implementation of std::move_if_noexcept). */
template <typename T>
[[nodiscard]] constexpr std::enable_if_t<std::is_nothrow_move_constructible_v<T> || !std::is_copy_constructible_v<T>,
                                         T&&>
conditional_move(T& value) noexcept {
    return std::move(value);
}

/** @brief Copy instead of a potentially throwing move. @param value source @return const lvalue reference */
template <typename T>
[[nodiscard]] constexpr std::enable_if_t<!std::is_nothrow_move_constructible_v<T> && std::is_copy_constructible_v<T>,
                                         const T&>
conditional_move(T& value) noexcept {
    return value;
}

// ===== SFINAE-friendly invocation =====

/** @brief Hand-rolled std::is_invocable (callable objects and function pointers). */
template <typename F, typename... Args>
class is_callable {
    template <typename Fn, typename... As>
    static auto test(int) -> decltype(std::declval<Fn>()(std::declval<As>()...), std::true_type{});
    template <typename, typename...>
    static std::false_type test(...);

public:
    /** @brief true if F(Args...) is well-formed */
    static constexpr bool value = decltype(test<F, Args...>(0))::value;
};

/** @brief Convenience variable. */
template <typename F, typename... Args>
inline constexpr bool is_callable_v = is_callable<F, Args...>::value;

/**
 * @brief Call func(args...) — only participates if the call is well-formed.
 * @param func callable
 * @param args arguments
 * @return func's result
 */
template <typename F, typename... Args>
auto safe_invoke(F&& func, Args&&... args)
    -> std::enable_if_t<is_callable_v<F, Args...>, decltype(std::declval<F>()(std::declval<Args>()...))> {
    return std::forward<F>(func)(std::forward<Args>(args)...);
}

// ===== Class template that adapts its interface =====

/** @brief Wrapper whose interface depends on whether T has size(). */
template <typename T, bool HasSize = has_size_method_v<T>>
class AdaptiveContainer;

/** @brief Sized flavour: size() is O(1) via the member. */
template <typename T>
class AdaptiveContainer<T, true> {
public:
    /** @param container wrapped container */
    explicit AdaptiveContainer(T container) : container_(std::move(container)) {}
    /** @return element count */
    [[nodiscard]] std::size_t size() const { return static_cast<std::size_t>(container_.size()); }
    /** @return true if empty */
    [[nodiscard]] bool empty() const { return size() == 0; }
    /** @return true: this flavour has constant-time size */
    [[nodiscard]] static constexpr bool has_constant_time_size() noexcept { return true; }
    /** @return iterator */
    auto begin() { return container_.begin(); }
    /** @return iterator */
    auto end() { return container_.end(); }

private:
    T container_;
};

/** @brief Unsized flavour (e.g. std::forward_list): size() walks the range. */
template <typename T>
class AdaptiveContainer<T, false> {
public:
    /** @param container wrapped container */
    explicit AdaptiveContainer(T container) : container_(std::move(container)) {}
    /** @return element count (linear) */
    [[nodiscard]] std::size_t size() const {
        return static_cast<std::size_t>(std::distance(container_.begin(), container_.end()));
    }
    /** @return true if empty */
    [[nodiscard]] bool empty() const { return container_.begin() == container_.end(); }
    /** @return false: this flavour computes size by iteration */
    [[nodiscard]] static constexpr bool has_constant_time_size() noexcept { return false; }
    /** @return iterator */
    auto begin() { return container_.begin(); }
    /** @return iterator */
    auto end() { return container_.end(); }

private:
    T container_;
};

/** @return std::make_unique<T>() — only for default-constructible T */
template <typename T>
[[nodiscard]] std::enable_if_t<std::is_default_constructible_v<T>, std::unique_ptr<T>> make_unique_default() {
    return std::make_unique<T>();
}

/** @param args constructor arguments @return std::make_unique<T>(args...) — only if T is constructible */
template <typename T, typename... Args>
[[nodiscard]] std::enable_if_t<std::is_constructible_v<T, Args...>, std::unique_ptr<T>>
make_unique_with_args(Args&&... args) {
    return std::make_unique<T>(std::forward<Args>(args)...);
}

/** @brief Element type of a std::vector; substitution fails for anything else. */
template <typename T>
using vector_element_t = std::enable_if_t<Meta::is_instantiation_of_v<std::vector, T>, typename T::value_type>;

// ===== The C++20 way: concepts instead of enable_if =====

/** @brief Arithmetic types. */
template <typename T>
concept ArithmeticType = std::is_arithmetic_v<T>;

/** @brief Types with begin(), end() and size(). */
template <typename T>
concept SizedContainer = requires(const T& t) {
    t.begin();
    t.end();
    t.size();
};

/** @brief Streamable types. */
template <typename T>
concept Streamable = requires(std::ostream& os, const T& t) { os << t; };

/** @brief Concept overload for numbers. @param value number @return description */
template <ArithmeticType T>
[[nodiscard]] std::string modern_describe(const T& value) {
    return "arithmetic " + serialize(value);
}

/** @brief Concept overload for sized containers (strings excluded via !Streamable). @return description */
template <SizedContainer T>
    requires(!Streamable<T>)
[[nodiscard]] std::string modern_describe(const T& container) {
    return "container with " + std::to_string(container.size()) + " elements";
}

/** @brief Concept overload for other streamable types. @param value value @return description */
template <typename T>
    requires(!ArithmeticType<T> && Streamable<T>)
[[nodiscard]] std::string modern_describe(const T& value) {
    std::ostringstream oss;
    oss << "printable " << value;
    return oss.str();
}

/** @brief Fallback. @return description */
template <typename T>
    requires(!ArithmeticType<T> && !Streamable<T> && !SizedContainer<T>)
[[nodiscard]] std::string modern_describe(const T&) {
    return "generic object";
}

// ===== Showcase =====

/**
 * @brief Show detection results and SFINAE-selected overloads.
 * @param out destination stream
 */
inline void demonstrate_sfinae(std::ostream& out = std::cout) {
    out << "--- SFINAE ---\n" << std::boolalpha;
    out << "has_size<vector>    = " << has_size_method_v<std::vector<int>> << '\n';
    out << "has_size<int>       = " << has_size_method_v<int> << '\n';
    out << "is_detected<size>   = " << is_detected_v<size_expression_t, std::string> << '\n';
    out << "has_member_first<pair> = " << has_member_first_v<std::pair<int, int>> << '\n';
    out << std::noboolalpha;
    out << describe_value(42) << '\n';
    out << describe_value(std::string("hello")) << '\n';
    out << describe_value(std::vector<int>{1, 2, 3}) << '\n';
    out << "serialize nested    = " << serialize(std::vector<std::vector<int>>{{1, 2}, {3}}) << '\n';
    out << "tag dispatch        = " << process_value(std::vector<double>{1.0, 2.0}) << '\n';
    out << "modern_describe     = " << modern_describe(std::string("text")) << '\n';
    const auto owned = std::make_unique<int>(7);
    out << safe_dereference(owned) << '\n';
    safe_print(out, 3.5);
}

} // namespace CppVerseHub::Templates::SFINAE

#endif // CPPVERSEHUB_TEMPLATES_SFINAE_EXAMPLES_HPP
