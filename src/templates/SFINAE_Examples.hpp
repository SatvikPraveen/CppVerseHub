// File: src/templates/SFINAE_Examples.hpp
// SFINAE (Substitution Failure Is Not An Error) demonstrations and techniques

#ifndef SFINAE_EXAMPLES_HPP
#define SFINAE_EXAMPLES_HPP

#include <type_traits>
#include <utility>
#include <vector>
#include <iterator>
#include <iostream>
#include <string>
#include <concepts>
#include <functional>
#include <memory>

namespace CppVerseHub {
namespace Templates {
namespace SFINAE {

// ===== Basic SFINAE Examples =====

/**
 * @brief Check if a type has a member function 'size()'
 */
template<typename T>
class has_size_method {
private:
    template<typename U>
    static auto test(int) -> decltype(std::declval<U>().size(), std::true_type{});
    
    template<typename>
    static std::false_type test(...);
    
public:
    static constexpr bool value = decltype(test<T>(0))::value;
};

template<typename T>
constexpr bool has_size_method_v = has_size_method<T>::value;

/**
 * @brief Check if a type has a member function 'begin()'
 */
template<typename T>
class has_begin_method {
private:
    template<typename U>
    static auto test(int) -> decltype(std::declval<U>().begin(), std::true_type{});
    
    template<typename>
    static std::false_type test(...);
    
public:
    static constexpr bool value = decltype(test<T>(0))::value;
};

template<typename T>
constexpr bool has_begin_method_v = has_begin_method<T>::value;

/**
 * @brief Check if a type has a member function 'end()'
 */
template<typename T>
class has_end_method {
private:
    template<typename U>
    static auto test(int) -> decltype(std::declval<U>().end(), std::true_type{});
    
    template<typename>
    static std::false_type test(...);
    
public:
    static constexpr bool value = decltype(test<T>(0))::value;
};

template<typename T>
constexpr bool has_end_method_v = has_end_method<T>::value;

/**
 * @brief Check if a type is iterable (has begin() and end())
 */
template<typename T>
constexpr bool is_iterable_v = has_begin_method_v<T> && has_end_method_v<T>;

/**
 * @brief Check if a type has a specific member variable
 */
#define DEFINE_HAS_MEMBER(member_name) \
    template<typename T> \
    class has_member_##member_name { \
    private: \
        template<typename U> \
        static auto test(int) -> decltype(std::declval<U>().member_name, std::true_type{}); \
        template<typename> \
        static std::false_type test(...); \
    public: \
        static constexpr bool value = decltype(test<T>(0))::value; \
    }; \
    template<typename T> \
    constexpr bool has_member_##member_name##_v = has_member_##member_name<T>::value;

DEFINE_HAS_MEMBER(data)
DEFINE_HAS_MEMBER(value)
DEFINE_HAS_MEMBER(first)
DEFINE_HAS_MEMBER(second)

/**
 * @brief Check if a type has a specific member function with signature
 */
template<typename T, typename Signature>
class has_member_function;

template<typename T, typename R, typename... Args>
class has_member_function<T, R(Args...)> {
private:
    template<typename U>
    static auto test(int) -> decltype(
        std::is_same_v<R, decltype(std::declval<U>()(std::declval<Args>()...))>,
        std::true_type{}
    );
    
    template<typename>
    static std::false_type test(...);
    
public:
    static constexpr bool value = decltype(test<T>(0))::value;
};

template<typename T, typename Signature>
constexpr bool has_member_function_v = has_member_function<T, Signature>::value;

/**
 * @brief Detect if a type supports arithmetic operations
 */
template<typename T>
class supports_arithmetic {
private:
    template<typename U>
    static auto test_add(int) -> decltype(std::declval<U>() + std::declval<U>(), std::true_type{});
    template<typename>
    static std::false_type test_add(...);
    
    template<typename U>
    static auto test_sub(int) -> decltype(std::declval<U>() - std::declval<U>(), std::true_type{});
    template<typename>
    static std::false_type test_sub(...);
    
    template<typename U>
    static auto test_mul(int) -> decltype(std::declval<U>() * std::declval<U>(), std::true_type{});
    template<typename>
    static std::false_type test_mul(...);
    
    template<typename U>
    static auto test_div(int) -> decltype(std::declval<U>() / std::declval<U>(), std::true_type{});
    template<typename>
    static std::false_type test_div(...);
    
public:
    static constexpr bool has_add = decltype(test_add<T>(0))::value;
    static constexpr bool has_sub = decltype(test_sub<T>(0))::value;
    static constexpr bool has_mul = decltype(test_mul<T>(0))::value;
    static constexpr bool has_div = decltype(test_div<T>(0))::value;
    
    static constexpr bool value = has_add && has_sub && has_mul && has_div;
};

template<typename T>
constexpr bool supports_arithmetic_v = supports_arithmetic<T>::value;

// ===== SFINAE-based Function Overloading =====

/**
 * @brief Generic print function that adapts to different types
 */
template<typename T>
std::enable_if_t<std::is_arithmetic_v<T>>
print_value(const T& value) {
    std::cout << "Arithmetic value: " << value << std::endl;
}

template<typename T>
std::enable_if_t<std::is_same_v<T, std::string>>
print_value(const T& value) {
    std::cout << "String value: \"" << value << "\"" << std::endl;
}

template<typename T>
std::enable_if_t<is_iterable_v<T> && !std::is_same_v<T, std::string>>
print_value(const T& container) {
    std::cout << "Container: [";
    bool first = true;
    for (const auto& item : container) {
        if (!first) std::cout << ", ";
        std::cout << item;
        first = false;
    }
    std::cout << "]" << std::endl;
}

template<typename T>
std::enable_if_t<!std::is_arithmetic_v<T> && !std::is_same_v<T, std::string> && !is_iterable_v<T>>
print_value(const T& value) {
    std::cout << "Generic value (address: " << &value << ")" << std::endl;
}

/**
 * @brief Size function that works with different container types
 */
template<typename T>
std::enable_if_t<has_size_method_v<T>, std::size_t>
get_size(const T& container) {
    return container.size();
}

template<typename T>
std::enable_if_t<!has_size_method_v<T> && is_iterable_v<T>, std::size_t>
get_size(const T& container) {
    return std::distance(container.begin(), container.end());
}

template<typename T, std::size_t N>
constexpr std::size_t get_size(const T(&)[N]) {
    return N;
}

/**
 * @brief Conditional serialization based on type capabilities
 */
template<typename T>
std::enable_if_t<std::is_arithmetic_v<T>, std::string>
serialize(const T& value) {
    return std::to_string(value);
}

template<typename T>
std::enable_if_t<std::is_same_v<T, std::string>, std::string>
serialize(const T& value) {
    return "\"" + value + "\"";
}

template<typename T>
std::enable_if_t<is_iterable_v<T> && !std::is_same_v<T, std::string>, std::string>
serialize(const T& container) {
    std::string result = "[";
    bool first = true;
    for (const auto& item : container) {
        if (!first) result += ",";
        result += serialize(item);
        first = false;
    }
    result += "]";
    return result;
}

// ===== Advanced SFINAE Techniques =====

/**
 * @brief Expression SFINAE - detect valid expressions
 */
template<typename T, typename = void>
struct has_insertion_operator : std::false_type {};

template<typename T>
struct has_insertion_operator<T, std::void_t<decltype(std::declval<std::ostream&>() << std::declval<T>())>>
    : std::true_type {};

template<typename T>
constexpr bool has_insertion_operator_v = has_insertion_operator<T>::value;

/**
 * @brief Template that works only with types that can be output
 */
template<typename T>
std::enable_if_t<has_insertion_operator_v<T>>
safe_print(const T& value) {
    std::cout << value << std::endl;
}

template<typename T>
std::enable_if_t<!has_insertion_operator_v<T>>
safe_print(const T&) {
    std::cout << "[Non-printable object]" << std::endl;
}

/**
 * @brief Detect if a type is a smart pointer
 */
template<typename T>
struct is_smart_pointer : std::false_type {};

template<typename T>
struct is_smart_pointer<std::unique_ptr<T>> : std::true_type {};

template<typename T>
struct is_smart_pointer<std::shared_ptr<T>> : std::true_type {};

template<typename T, typename D>
struct is_smart_pointer<std::unique_ptr<T, D>> : std::true_type {};

template<typename T>
constexpr bool is_smart_pointer_v = is_smart_pointer<T>::value;

/**
 * @brief Function that works with both raw pointers and smart pointers
 */
template<typename PtrType>
std::enable_if_t<std::is_pointer_v<PtrType>>
safe_dereference(PtrType ptr) {
    if (ptr) {
        std::cout << "Raw pointer value: " << *ptr << std::endl;
    } else {
        std::cout << "Null raw pointer" << std::endl;
    }
}

template<typename PtrType>
std::enable_if_t<is_smart_pointer_v<PtrType>>
safe_dereference(const PtrType& ptr) {
    if (ptr) {
        std::cout << "Smart pointer value: " << *ptr << std::endl;
    } else {
        std::cout << "Null smart pointer" << std::endl;
    }
}

/**
 * @brief Tag dispatch technique combined with SFINAE
 */
struct container_tag {};
struct arithmetic_tag {};
struct string_tag {};
struct generic_tag {};

template<typename T>
constexpr auto get_type_tag() {
    if constexpr (std::is_arithmetic_v<T>) {
        return arithmetic_tag{};
    } else if constexpr (std::is_same_v<T, std::string>) {
        return string_tag{};
    } else if constexpr (is_iterable_v<T>) {
        return container_tag{};
    } else {
        return generic_tag{};
    }
}

template<typename T>
void process_value(const T& value, container_tag) {
    std::cout << "Processing container with " << get_size(value) << " elements" << std::endl;
}

template<typename T>
void process_value(const T& value, arithmetic_tag) {
    std::cout << "Processing arithmetic value: " << value << std::endl;
}

template<typename T>
void process_value(const T& value, string_tag) {
    std::cout << "Processing string: \"" << value << "\"" << std::endl;
}

template<typename T>
void process_value(const T& value, generic_tag) {
    std::cout << "Processing generic object at " << &value << std::endl;
}

template<typename T>
void process_value(const T& value) {
    process_value(value, get_type_tag<T>());
}

// ===== SFINAE with Template Specialization =====

/**
 * @brief Generic algorithm selector based on iterator category
 */
template<typename Iterator, typename = void>
struct algorithm_selector {
    static void advance(Iterator& it, typename std::iterator_traits<Iterator>::difference_type n) {
        std::advance(it, n); // Fallback to std::advance
    }
};

template<typename Iterator>
struct algorithm_selector<Iterator, 
                         std::enable_if_t<std::is_same_v<
                             typename std::iterator_traits<Iterator>::iterator_category,
                             std::random_access_iterator_tag>>> {
    static void advance(Iterator& it, typename std::iterator_traits<Iterator>::difference_type n) {
        it += n; // Optimized for random access iterators
    }
};

/**
 * @brief Optimized copy based on type properties
 */
template<typename InputIt, typename OutputIt>
std::enable_if_t<
    std::is_trivially_copyable_v<typename std::iterator_traits<InputIt>::value_type> &&
    std::is_pointer_v<InputIt> &&
    std::is_pointer_v<OutputIt>,
    OutputIt>
optimized_copy(InputIt first, InputIt last, OutputIt dest) {
    // Use memcpy for trivially copyable types with pointer iterators
    const auto count = std::distance(first, last);
    std::memcpy(dest, first, count * sizeof(typename std::iterator_traits<InputIt>::value_type));
    return dest + count;
}

template<typename InputIt, typename OutputIt>
std::enable_if_t<
    !(std::is_trivially_copyable_v<typename std::iterator_traits<InputIt>::value_type> &&
      std::is_pointer_v<InputIt> &&
      std::is_pointer_v<OutputIt>),
    OutputIt>
optimized_copy(InputIt first, InputIt last, OutputIt dest) {
    // Fallback to standard copy
    return std::copy(first, last, dest);
}

/**
 * @brief Conditional move semantics
 */
template<typename T>
std::enable_if_t<std::is_nothrow_move_constructible_v<T>, T&&>
conditional_move(T& value) noexcept {
    return std::move(value);
}

template<typename T>
std::enable_if_t<!std::is_nothrow_move_constructible_v<T>, const T&>
conditional_move(T& value) noexcept {
    return value; // Copy instead of potentially throwing move
}

// ===== SFINAE with Concepts (C++20 style) =====

#if __cpp_concepts >= 201907L

/**
 * @brief Concepts-based version of arithmetic detection
 */
template<typename T>
concept Arithmetic = std::is_arithmetic_v<T>;

template<typename T>
concept Container = requires(T t) {
    t.begin();
    t.end();
    t.size();
};

template<typename T>
concept Printable = requires(T t) {
    std::cout << t;
};

/**
 * @brief Function overloads using concepts instead of SFINAE
 */
template<Arithmetic T>
void modern_print(const T& value) {
    std::cout << "Arithmetic: " << value << std::endl;
}

template<Container T>
void modern_print(const T& container) {
    std::cout << "Container with " << container.size() << " elements" << std::endl;
}

template<typename T>
    requires (!Arithmetic<T> && !Container<T> && Printable<T>)
void modern_print(const T& value) {
    std::cout << "Printable: " << value << std::endl;
}

template<typename T>
    requires (!Arithmetic<T> && !Container<T> && !Printable<T>)
void modern_print(const T& value) {
    std::cout << "Generic object at " << &value << std::endl;
}

#endif

// ===== SFINAE Utility Classes =====

/**
 * @brief Type selector based on boolean condition
 */
template<bool Condition, typename TrueType, typename FalseType>
using conditional_t = typename std::conditional<Condition, TrueType, FalseType>::type;

/**
 * @brief Multi-conditional type selector
 */
template<typename... Conditions>
struct first_valid_type;

template<typename Head, typename... Tail>
struct first_valid_type<Head, Tail...> {
    using type = Head;
};

template<>
struct first_valid_type<> {
    // Empty case - could be an error or void
    using type = void;
};

template<typename... Conditions>
using first_valid_type_t = typename first_valid_type<Conditions...>::type;

/**
 * @brief SFINAE-friendly invoke result
 */
template<typename F, typename... Args>
class is_invocable {
private:
    template<typename Fn, typename... As>
    static auto test(int) -> decltype(std::declval<Fn>()(std::declval<As>()...), std::true_type{});
    
    template<typename, typename...>
    static std::false_type test(...);
    
public:
    static constexpr bool value = decltype(test<F, Args...>(0))::value;
};

template<typename F, typename... Args>
constexpr bool is_invocable_v = is_invocable<F, Args...>::value;

/**
 * @brief Safe invoke that only compiles if callable
 */
template<typename F, typename... Args>
std::enable_if_t<is_invocable_v<F, Args...>, decltype(std::declval<F>()(std::declval<Args>()...))>
safe_invoke(F&& func, Args&&... args) {
    return std::forward<F>(func)(std::forward<Args>(args)...);
}

/**
 * @brief Template class that provides different interfaces based on type properties
 */
template<typename T, bool HasSize = has_size_method_v<T>>
class AdaptiveContainer;

template<typename T>
class AdaptiveContainer<T, true> {
private:
    T container_;
    
public:
    explicit AdaptiveContainer(T container) : container_(std::move(container)) {}
    
    auto size() const { return container_.size(); }
    bool empty() const { return container_.size() == 0; }
    
    template<typename U = T>
    std::enable_if_t<has_begin_method_v<U>, decltype(std::declval<U>().begin())>
    begin() { return container_.begin(); }
    
    template<typename U = T>
    std::enable_if_t<has_end_method_v<U>, decltype(std::declval<U>().end())>
    end() { return container_.end(); }
};

template<typename T>
class AdaptiveContainer<T, false> {
private:
    T container_;
    
public:
    explicit AdaptiveContainer(T container) : container_(std::move(container)) {}
    
    // No size method - would need to iterate to count
    bool empty() const {
        return container_.begin() == container_.end();
    }
    
    template<typename U = T>
    std::enable_if_t<has_begin_method_v<U>, decltype(std::declval<U>().begin())>
    begin() { return container_.begin(); }
    
    template<typename U = T>
    std::enable_if_t<has_end_method_v<U>, decltype(std::declval<U>().end())>
    end() { return container_.end(); }
};

/**
 * @brief Factory function with SFINAE-based type deduction
 */
template<typename T>
std::enable_if_t<std::is_default_constructible_v<T>, std::unique_ptr<T>>
make_unique_default() {
    return std::make_unique<T>();
}

template<typename T, typename... Args>
std::enable_if_t<std::is_constructible_v<T, Args...>, std::unique_ptr<T>>
make_unique_with_args(Args&&... args) {
    return std::make_unique<T>(std::forward<Args>(args)...);
}

/**
 * @brief Example of SFINAE in template aliases
 */
template<typename T>
using vector_element_t = std::enable_if_t<
    is_instantiation_of_v<std::vector, T>,
    typename T::value_type
>;

// Usage: vector_element_t<std::vector<int>> -> int
//        vector_element_t<int> -> compilation error

} // namespace SFINAE
} // namespace Templates
} // namespace CppVerseHub

#endif // SFINAE_EXAMPLES_HPP