/**
 * @file TemplateSpecialization.hpp
 * @brief Full and partial template specialisation of class and variable templates.
 *
 * Demonstrates
 *  - full (explicit) specialisation (`Serializer<int>`, `Serializer<bool>`, ...);
 *  - partial specialisation on type shape: pointers, references, arrays, cv-qualifiers
 *    (including how to avoid ambiguity between overlapping patterns such as `const T` vs `T[N]`);
 *  - partial specialisation on template-ids (std::vector<T>, std::array<T, N>, smart pointers,
 *    std::tuple, std::pair, std::optional, std::variant);
 *  - dispatching on iterator categories and on SFINAE conditions in specialisations;
 *  - decomposing function types (pointer-to-function / pointer-to-member, noexcept);
 *  - variable template specialisation;
 *  - why alias templates *cannot* be specialised, and the class-template workaround.
 */

#ifndef CPPVERSEHUB_TEMPLATES_TEMPLATE_SPECIALIZATION_HPP
#define CPPVERSEHUB_TEMPLATES_TEMPLATE_SPECIALIZATION_HPP

#include <algorithm>
#include <array>
#include <cstddef>
#include <functional>
#include <iostream>
#include <iterator>
#include <locale>
#include <memory>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

namespace CppVerseHub::Templates::Specialization {

// ===== Full specialisation =====

/**
 * @brief Primary template: types without a dedicated serializer are reported as opaque.
 * @tparam T value type
 */
template <typename T>
struct Serializer {
    /** @brief Whether a real serializer exists for T. */
    static constexpr bool is_specialized = false;
    /** @return placeholder text */
    [[nodiscard]] static std::string serialize(const T&) { return "<opaque>"; }
};

/** @brief Full specialisation for int. */
template <>
struct Serializer<int> {
    static constexpr bool is_specialized = true; ///< real serializer
    /** @param value number @return decimal text */
    [[nodiscard]] static std::string serialize(const int& value) { return std::to_string(value); }
    /** @param data decimal text @return parsed number @throws std::invalid_argument / std::out_of_range */
    [[nodiscard]] static int deserialize(const std::string& data) { return std::stoi(data); }
};

/** @brief Full specialisation for double (round-trippable, locale-independent "C" formatting). */
template <>
struct Serializer<double> {
    static constexpr bool is_specialized = true; ///< real serializer
    /** @param value number @return text with 17 significant digits */
    [[nodiscard]] static std::string serialize(const double& value) {
        std::ostringstream oss;
        oss.imbue(std::locale::classic());
        oss.precision(17);
        oss << value;
        return oss.str();
    }
    /** @param data text @return parsed number @throws std::invalid_argument */
    [[nodiscard]] static double deserialize(const std::string& data) {
        std::istringstream iss(data);
        iss.imbue(std::locale::classic());
        double value = 0.0;
        if (!(iss >> value)) {
            throw std::invalid_argument("Serializer<double>: not a number: " + data);
        }
        return value;
    }
};

/** @brief Full specialisation for std::string (quoted). */
template <>
struct Serializer<std::string> {
    static constexpr bool is_specialized = true; ///< real serializer
    /** @param value text @return quoted text */
    [[nodiscard]] static std::string serialize(const std::string& value) { return "\"" + value + "\""; }
    /** @param data possibly quoted text @return unquoted text */
    [[nodiscard]] static std::string deserialize(const std::string& data) {
        if (data.size() >= 2 && data.front() == '"' && data.back() == '"') {
            return data.substr(1, data.size() - 2);
        }
        return data;
    }
};

/** @brief Full specialisation for bool. */
template <>
struct Serializer<bool> {
    static constexpr bool is_specialized = true; ///< real serializer
    /** @param value flag @return "true" or "false" */
    [[nodiscard]] static std::string serialize(const bool& value) { return value ? "true" : "false"; }
    /** @param data text @return true iff data == "true" */
    [[nodiscard]] static bool deserialize(const std::string& data) { return data == "true"; }
};

/** @brief Partial specialisation composing element serializers for std::vector. */
template <typename T>
struct Serializer<std::vector<T>> {
    static constexpr bool is_specialized = Serializer<T>::is_specialized; ///< real if elements are
    /** @param values elements @return "[a,b,c]" */
    [[nodiscard]] static std::string serialize(const std::vector<T>& values) {
        std::string result = "[";
        for (std::size_t i = 0; i < values.size(); ++i) {
            if (i > 0) {
                result += ',';
            }
            result += Serializer<T>::serialize(values[i]);
        }
        return result + "]";
    }
};

// ===== Partial specialisation on type shape =====

/**
 * @brief Primary template describing a type's shape. cv-qualification is reported via the
 *        standard traits rather than with `const T` specialisations, because `const T` and `T[N]`
 *        would both match `const int[3]` and be ambiguous.
 */
template <typename T>
struct TypeInfo {
    static constexpr bool is_pointer = false;               ///< T is U*
    static constexpr bool is_reference = false;             ///< T is U& or U&&
    static constexpr bool is_array = false;                 ///< T is U[N] or U[]
    static constexpr bool is_const = std::is_const_v<T>;    ///< top-level const
    static constexpr bool is_volatile = std::is_volatile_v<T>; ///< top-level volatile
    static constexpr const char* name = "value";            ///< shape name
};

/** @brief Pointers (cv-unqualified pointer object). */
template <typename T>
struct TypeInfo<T*> : TypeInfo<void> {
    static constexpr bool is_pointer = true;          ///< T is U*
    static constexpr bool is_const = false;           ///< top-level const
    static constexpr bool is_volatile = false;        ///< top-level volatile
    static constexpr const char* name = "pointer";    ///< shape name
    using pointed_type = T;                           ///< U
};

/** @brief Const pointers (`U* const`) — more specialised than both `T*` and the primary. */
template <typename T>
struct TypeInfo<T* const> : TypeInfo<T*> {
    static constexpr bool is_const = true; ///< top-level const
};

/** @brief Lvalue references. */
template <typename T>
struct TypeInfo<T&> : TypeInfo<void> {
    static constexpr bool is_reference = true;                ///< reference
    static constexpr const char* name = "lvalue_reference";   ///< shape name
    using referenced_type = T;                                ///< U
};

/** @brief Rvalue references. */
template <typename T>
struct TypeInfo<T&&> : TypeInfo<void> {
    static constexpr bool is_reference = true;                ///< reference
    static constexpr const char* name = "rvalue_reference";   ///< shape name
    using referenced_type = T;                                ///< U
};

/** @brief Arrays of known bound. */
template <typename T, std::size_t N>
struct TypeInfo<T[N]> : TypeInfo<void> {
    static constexpr bool is_array = true;              ///< array
    static constexpr bool is_const = std::is_const_v<T>; ///< element constness (arrays inherit it)
    static constexpr const char* name = "array";        ///< shape name
    static constexpr std::size_t size = N;              ///< extent
    using element_type = T;                             ///< element type
};

/** @brief Arrays of unknown bound. */
template <typename T>
struct TypeInfo<T[]> : TypeInfo<void> {
    static constexpr bool is_array = true;                    ///< array
    static constexpr const char* name = "unbounded_array";    ///< shape name
    using element_type = T;                                   ///< element type
};

/**
 * @brief cv-qualification classified with three partial specialisations; `const volatile T`
 *        is more specialised than either `const T` or `volatile T`, so it resolves the overlap.
 */
template <typename T>
struct Constness {
    static constexpr const char* name = "unqualified"; ///< qualification name
    using unqualified = T;                             ///< T without cv
};
/** @brief const T. */
template <typename T>
struct Constness<const T> {
    static constexpr const char* name = "const"; ///< qualification name
    using unqualified = T;                       ///< T without cv
};
/** @brief volatile T. */
template <typename T>
struct Constness<volatile T> {
    static constexpr const char* name = "volatile"; ///< qualification name
    using unqualified = T;                          ///< T without cv
};
/** @brief const volatile T. */
template <typename T>
struct Constness<const volatile T> {
    static constexpr const char* name = "const volatile"; ///< qualification name
    using unqualified = T;                                ///< T without cv
};

// ===== Partial specialisation on template-ids =====

/** @brief Primary: values printed via operator<< if possible, otherwise as opaque. */
template <typename T>
struct ContainerPrinter {
    /** @param out stream @param value value */
    static void print(std::ostream& out, const T& value) {
        if constexpr (requires { out << value; }) {
            out << "value: " << value;
        } else {
            out << "<opaque object>";
        }
    }
};

/** @brief std::vector<T>. */
template <typename T, typename Alloc>
struct ContainerPrinter<std::vector<T, Alloc>> {
    /** @param out stream @param vec vector */
    static void print(std::ostream& out, const std::vector<T, Alloc>& vec) {
        out << "vector[" << vec.size() << "]: [";
        for (std::size_t i = 0; i < vec.size(); ++i) {
            out << (i > 0 ? ", " : "") << vec[i];
        }
        out << ']';
    }
};

/** @brief std::array<T, N>: N is deduced in the specialisation. */
template <typename T, std::size_t N>
struct ContainerPrinter<std::array<T, N>> {
    /** @param out stream @param arr array */
    static void print(std::ostream& out, const std::array<T, N>& arr) {
        out << "array[" << N << "]: [";
        for (std::size_t i = 0; i < N; ++i) {
            out << (i > 0 ? ", " : "") << arr[i];
        }
        out << ']';
    }
};

/** @brief std::unique_ptr. */
template <typename T, typename D>
struct ContainerPrinter<std::unique_ptr<T, D>> {
    /** @param out stream @param ptr pointer */
    static void print(std::ostream& out, const std::unique_ptr<T, D>& ptr) {
        if (ptr) {
            out << "unique_ptr -> " << *ptr;
        } else {
            out << "unique_ptr -> null";
        }
    }
};

/** @brief std::shared_ptr. */
template <typename T>
struct ContainerPrinter<std::shared_ptr<T>> {
    /** @param out stream @param ptr pointer */
    static void print(std::ostream& out, const std::shared_ptr<T>& ptr) {
        if (ptr) {
            out << "shared_ptr[" << ptr.use_count() << "] -> " << *ptr;
        } else {
            out << "shared_ptr -> null";
        }
    }
};

/**
 * @brief Convenience wrapper returning ContainerPrinter's output as a string.
 * @param value value to print
 * @return printed text
 */
template <typename T>
[[nodiscard]] std::string print_to_string(const T& value) {
    std::ostringstream oss;
    ContainerPrinter<T>::print(oss, value);
    return oss.str();
}

// ===== Iterator category dispatch =====

/** @brief Select an implementation by iterator category. */
template <typename Iterator, typename Category = typename std::iterator_traits<Iterator>::iterator_category>
struct IteratorHelper;

/** @brief Input iterators: single pass, linear distance. */
template <typename Iterator>
struct IteratorHelper<Iterator, std::input_iterator_tag> {
    static constexpr const char* category_name = "input_iterator"; ///< category
    /** @param first begin @param last end @return number of steps */
    static std::size_t distance(Iterator first, Iterator last) {
        std::size_t count = 0;
        for (; first != last; ++first) {
            ++count;
        }
        return count;
    }
    /** @param it iterator @param n steps forward */
    static void advance(Iterator& it, std::size_t n) {
        for (; n > 0; --n) {
            ++it;
        }
    }
};

/** @brief Forward iterators reuse the input implementation (inheritance between specialisations). */
template <typename Iterator>
struct IteratorHelper<Iterator, std::forward_iterator_tag> : IteratorHelper<Iterator, std::input_iterator_tag> {
    static constexpr const char* category_name = "forward_iterator"; ///< category
};

/** @brief Bidirectional iterators add retreat(). */
template <typename Iterator>
struct IteratorHelper<Iterator, std::bidirectional_iterator_tag>
    : IteratorHelper<Iterator, std::forward_iterator_tag> {
    static constexpr const char* category_name = "bidirectional_iterator"; ///< category
    /** @param it iterator @param n steps backward */
    static void retreat(Iterator& it, std::size_t n) {
        for (; n > 0; --n) {
            --it;
        }
    }
};

/** @brief Random-access iterators: O(1) everything. */
template <typename Iterator>
struct IteratorHelper<Iterator, std::random_access_iterator_tag> {
    using difference_type = typename std::iterator_traits<Iterator>::difference_type; ///< distance type
    static constexpr const char* category_name = "random_access_iterator";            ///< category
    /** @param first begin @param last end @return last - first */
    static std::size_t distance(Iterator first, Iterator last) { return static_cast<std::size_t>(last - first); }
    /** @param it iterator @param n steps forward */
    static void advance(Iterator& it, std::size_t n) { it += static_cast<difference_type>(n); }
    /** @param it iterator @param n steps backward */
    static void retreat(Iterator& it, std::size_t n) { it -= static_cast<difference_type>(n); }
};

// ===== Decomposing function types =====

/** @brief Primary template left undefined: only function-like types are supported. */
template <typename F>
struct FunctionWrapper;

/** @brief Free function pointers. */
template <typename R, typename... Args>
struct FunctionWrapper<R (*)(Args...)> {
    using function_type = R (*)(Args...);                      ///< wrapped type
    using return_type = R;                                     ///< result
    using argument_types = std::tuple<Args...>;                ///< parameters
    static constexpr std::size_t arity = sizeof...(Args);      ///< parameter count
    static constexpr bool is_noexcept = false;                 ///< noexcept-qualified
    static constexpr const char* type_name = "function_pointer"; ///< description

    /** @param f function pointer */
    constexpr explicit FunctionWrapper(function_type f) noexcept : func(f) {}
    /** @param args arguments @return f(args...) */
    R operator()(Args... args) const { return func(std::forward<Args>(args)...); }

    function_type func; ///< wrapped pointer
};

/** @brief noexcept free function pointers: noexcept is part of the type since C++17. */
template <typename R, typename... Args>
struct FunctionWrapper<R (*)(Args...) noexcept> {
    using function_type = R (*)(Args...) noexcept;                      ///< wrapped type
    using return_type = R;                                              ///< result
    using argument_types = std::tuple<Args...>;                         ///< parameters
    static constexpr std::size_t arity = sizeof...(Args);               ///< parameter count
    static constexpr bool is_noexcept = true;                           ///< noexcept-qualified
    static constexpr const char* type_name = "noexcept_function_pointer"; ///< description

    /** @param f function pointer */
    constexpr explicit FunctionWrapper(function_type f) noexcept : func(f) {}
    /** @param args arguments @return f(args...) */
    R operator()(Args... args) const noexcept { return func(std::forward<Args>(args)...); }

    function_type func; ///< wrapped pointer
};

/** @brief Non-const member function pointers. */
template <typename R, typename C, typename... Args>
struct FunctionWrapper<R (C::*)(Args...)> {
    using function_type = R (C::*)(Args...);                          ///< wrapped type
    using return_type = R;                                            ///< result
    using class_type = C;                                             ///< owning class
    using argument_types = std::tuple<Args...>;                       ///< parameters
    static constexpr std::size_t arity = sizeof...(Args);             ///< parameter count
    static constexpr bool is_noexcept = false;                        ///< noexcept-qualified
    static constexpr const char* type_name = "member_function_pointer"; ///< description

    /** @param f member pointer */
    constexpr explicit FunctionWrapper(function_type f) noexcept : func(f) {}
    /** @param obj target @param args arguments @return (obj.*f)(args...) */
    R operator()(C& obj, Args... args) const { return (obj.*func)(std::forward<Args>(args)...); }

    function_type func; ///< wrapped pointer
};

/** @brief Const member function pointers. */
template <typename R, typename C, typename... Args>
struct FunctionWrapper<R (C::*)(Args...) const> {
    using function_type = R (C::*)(Args...) const;                          ///< wrapped type
    using return_type = R;                                                  ///< result
    using class_type = C;                                                   ///< owning class
    using argument_types = std::tuple<Args...>;                             ///< parameters
    static constexpr std::size_t arity = sizeof...(Args);                   ///< parameter count
    static constexpr bool is_noexcept = false;                              ///< noexcept-qualified
    static constexpr const char* type_name = "const_member_function_pointer"; ///< description

    /** @param f member pointer */
    constexpr explicit FunctionWrapper(function_type f) noexcept : func(f) {}
    /** @param obj target @param args arguments @return (obj.*f)(args...) */
    R operator()(const C& obj, Args... args) const { return (obj.*func)(std::forward<Args>(args)...); }

    function_type func; ///< wrapped pointer
};

// ===== Tuple-like specialisations =====

/** @brief Primary: not a tuple. */
template <typename T>
struct TupleProcessor {
    static constexpr std::size_t arity = 1; ///< element count
    /** @return description */
    [[nodiscard]] static std::string describe(const T&) { return "not a tuple"; }
};

/** @brief std::tuple of any arity. */
template <typename... Types>
struct TupleProcessor<std::tuple<Types...>> {
    static constexpr std::size_t arity = sizeof...(Types); ///< element count
    /** @param t tuple @return "tuple<N>(a, b, ...)" */
    [[nodiscard]] static std::string describe(const std::tuple<Types...>& t) {
        std::ostringstream oss;
        oss << "tuple<" << arity << ">(";
        std::apply(
            [&oss](const auto&... elements) {
                [[maybe_unused]] std::size_t index = 0;
                ((oss << (index++ == 0 ? "" : ", ") << elements), ...);
            },
            t);
        oss << ')';
        return oss.str();
    }
};

/** @brief std::pair. */
template <typename T1, typename T2>
struct TupleProcessor<std::pair<T1, T2>> {
    static constexpr std::size_t arity = 2; ///< element count
    /** @param p pair @return "pair(a, b)" */
    [[nodiscard]] static std::string describe(const std::pair<T1, T2>& p) {
        std::ostringstream oss;
        oss << "pair(" << p.first << ", " << p.second << ')';
        return oss.str();
    }
};

// ===== Optional / variant specialisations =====

/** @brief Primary: plain values. */
template <typename T>
struct OptionHandler {
    /** @param value value @return description */
    [[nodiscard]] static std::string describe(const T& value) {
        std::ostringstream oss;
        oss << "value " << value;
        return oss.str();
    }
};

/** @brief std::optional<T>. */
template <typename T>
struct OptionHandler<std::optional<T>> {
    /** @param opt optional @return description */
    [[nodiscard]] static std::string describe(const std::optional<T>& opt) {
        if (!opt) {
            return "empty optional";
        }
        std::ostringstream oss;
        oss << "optional " << *opt;
        return oss.str();
    }
};

/** @brief std::variant<Types...>. */
template <typename... Types>
struct OptionHandler<std::variant<Types...>> {
    /** @param var variant @return description including the active index */
    [[nodiscard]] static std::string describe(const std::variant<Types...>& var) {
        std::ostringstream oss;
        oss << "variant#" << var.index() << ' ';
        std::visit([&oss](const auto& value) { oss << value; }, var);
        return oss.str();
    }
};

// ===== SFINAE-controlled specialisation =====

namespace detail {
template <typename T>
using iterator_category_of_t = typename std::iterator_traits<typename T::iterator>::iterator_category;
} // namespace detail

/** @brief Primary: container without usable iterators. */
template <typename T, typename Enable = void>
struct AlgorithmSelector {
    static constexpr const char* strategy = "unsupported"; ///< chosen strategy
};

/** @brief Random-access containers: std::sort in place. */
template <typename T>
struct AlgorithmSelector<
    T, std::enable_if_t<std::is_base_of_v<std::random_access_iterator_tag, detail::iterator_category_of_t<T>>>> {
    static constexpr const char* strategy = "introsort"; ///< chosen strategy
    /** @param container container sorted in place */
    static void sort(T& container) { std::sort(container.begin(), container.end()); }
};

/** @brief Bidirectional-only containers: copy to a vector, sort, copy back. */
template <typename T>
struct AlgorithmSelector<T, std::enable_if_t<std::is_same_v<detail::iterator_category_of_t<T>,
                                                            std::bidirectional_iterator_tag>>> {
    static constexpr const char* strategy = "copy-sort-copy"; ///< chosen strategy
    /** @param container container sorted in place */
    static void sort(T& container) {
        std::vector<typename T::value_type> buffer(std::make_move_iterator(container.begin()),
                                                   std::make_move_iterator(container.end()));
        std::sort(buffer.begin(), buffer.end());
        std::move(buffer.begin(), buffer.end(), container.begin());
    }
};

// ===== Variable template specialisation =====

/** @brief Arithmetic types are numeric ... */
template <typename T>
inline constexpr bool is_numeric_v = std::is_arithmetic_v<T>;
/** @brief ... except bool, which is arithmetic but not a number (full specialisation). */
template <>
inline constexpr bool is_numeric_v<bool> = false;

/** @brief Size category of fundamental integer types. */
template <typename T>
inline constexpr const char* size_category_v = "unknown";
/** @brief char. */
template <>
inline constexpr const char* size_category_v<char> = "tiny";
/** @brief short. */
template <>
inline constexpr const char* size_category_v<short> = "small";
/** @brief int. */
template <>
inline constexpr const char* size_category_v<int> = "medium";
/** @brief long. */
template <>
inline constexpr const char* size_category_v<long> = "large";
/** @brief long long. */
template <>
inline constexpr const char* size_category_v<long long> = "huge";

// ===== Alias templates cannot be specialised: use a class template =====

/**
 * @brief Element type of a container or array. `template<class T, size_t N> using element_type_t<T[N]>`
 *        is ill-formed (alias templates admit no specialisation), so the variation lives in a class
 *        template and the alias merely forwards to it.
 */
template <typename T>
struct element_type {
    using type = typename T::value_type; ///< container element
};
/** @brief Arrays of known bound. */
template <typename T, std::size_t N>
struct element_type<T[N]> {
    using type = T; ///< array element
};
/** @brief Pointers. */
template <typename T>
struct element_type<T*> {
    using type = T; ///< pointee
};
/** @brief Alias forwarding to element_type. */
template <typename T>
using element_type_t = typename element_type<T>::type;

/** @brief Alias templates are still useful as type factories. */
template <typename T>
using pointer_t = T*;
/** @brief std::unique_ptr factory alias. */
template <typename T>
using unique_pointer_t = std::unique_ptr<T>;
/** @brief std::shared_ptr factory alias. */
template <typename T>
using shared_pointer_t = std::shared_ptr<T>;

// ===== Showcase =====

/**
 * @brief Demonstrate which specialisation is selected for a variety of types.
 * @param out destination stream
 */
inline void demonstrate_specializations(std::ostream& out = std::cout) {
    out << "--- Template specialization ---\n";
    out << "Serializer<int>     = " << Serializer<int>::serialize(42) << '\n';
    out << "Serializer<bool>    = " << Serializer<bool>::serialize(true) << '\n';
    out << "Serializer<string>  = " << Serializer<std::string>::serialize("hi") << '\n';
    out << "Serializer<vector>  = " << Serializer<std::vector<int>>::serialize({1, 2, 3}) << '\n';
    out << "TypeInfo<int*>      = " << TypeInfo<int*>::name << '\n';
    out << "TypeInfo<int&&>     = " << TypeInfo<int&&>::name << '\n';
    out << "TypeInfo<int[4]>    = " << TypeInfo<int[4]>::name << " of " << TypeInfo<int[4]>::size << '\n';
    out << "Constness<const volatile int> = " << Constness<const volatile int>::name << '\n';

    out << print_to_string(std::vector<int>{1, 2, 3}) << '\n';
    out << print_to_string(std::array<double, 2>{1.5, 2.5}) << '\n';
    out << print_to_string(std::make_shared<std::string>("shared")) << '\n';

    out << TupleProcessor<std::tuple<int, double, std::string>>::describe({1, 2.5, "x"}) << '\n';
    out << OptionHandler<std::variant<int, std::string>>::describe(std::string("alt")) << '\n';
    out << "size_category<long> = " << size_category_v<long> << '\n';
    out << "AlgorithmSelector<vector> = " << AlgorithmSelector<std::vector<int>>::strategy << '\n';
}

} // namespace CppVerseHub::Templates::Specialization

#endif // CPPVERSEHUB_TEMPLATES_TEMPLATE_SPECIALIZATION_HPP
