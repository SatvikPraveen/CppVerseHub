// File: src/templates/TemplateSpecialization.hpp
// Full and partial template specialization demonstrations

#ifndef TEMPLATE_SPECIALIZATION_HPP
#define TEMPLATE_SPECIALIZATION_HPP

#include <type_traits>
#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include <utility>
#include <functional>
#include <array>
#include <tuple>
#include <variant>
#include <optional>

namespace CppVerseHub {
namespace Templates {
namespace Specialization {

// ===== Full Template Specialization =====

/**
 * @brief Generic serializer template
 */
template<typename T>
struct Serializer {
    static std::string serialize(const T& value) {
        return "Generic object";
    }
    
    static T deserialize(const std::string& data) {
        return T{};
    }
};

/**
 * @brief Full specialization for int
 */
template<>
struct Serializer<int> {
    static std::string serialize(const int& value) {
        return std::to_string(value);
    }
    
    static int deserialize(const std::string& data) {
        return std::stoi(data);
    }
};

/**
 * @brief Full specialization for double
 */
template<>
struct Serializer<double> {
    static std::string serialize(const double& value) {
        return std::to_string(value);
    }
    
    static double deserialize(const std::string& data) {
        return std::stod(data);
    }
};

/**
 * @brief Full specialization for std::string
 */
template<>
struct Serializer<std::string> {
    static std::string serialize(const std::string& value) {
        return "\"" + value + "\"";
    }
    
    static std::string deserialize(const std::string& data) {
        if (data.size() >= 2 && data.front() == '"' && data.back() == '"') {
            return data.substr(1, data.size() - 2);
        }
        return data;
    }
};

/**
 * @brief Full specialization for bool
 */
template<>
struct Serializer<bool> {
    static std::string serialize(const bool& value) {
        return value ? "true" : "false";
    }
    
    static bool deserialize(const std::string& data) {
        return data == "true";
    }
};

// ===== Partial Template Specialization =====

/**
 * @brief Generic type traits template
 */
template<typename T>
struct TypeInfo {
    static constexpr bool is_pointer = false;
    static constexpr bool is_reference = false;
    static constexpr bool is_const = false;
    static constexpr bool is_volatile = false;
    static constexpr const char* name = "unknown";
};

/**
 * @brief Partial specialization for pointer types
 */
template<typename T>
struct TypeInfo<T*> {
    static constexpr bool is_pointer = true;
    static constexpr bool is_reference = false;
    static constexpr bool is_const = std::is_const_v<T>;
    static constexpr bool is_volatile = std::is_volatile_v<T>;
    static constexpr const char* name = "pointer";
    
    using pointed_type = T;
};

/**
 * @brief Partial specialization for reference types
 */
template<typename T>
struct TypeInfo<T&> {
    static constexpr bool is_pointer = false;
    static constexpr bool is_reference = true;
    static constexpr bool is_const = std::is_const_v<T>;
    static constexpr bool is_volatile = std::is_volatile_v<T>;
    static constexpr const char* name = "lvalue_reference";
    
    using referenced_type = T;
};

/**
 * @brief Partial specialization for rvalue reference types
 */
template<typename T>
struct TypeInfo<T&&> {
    static constexpr bool is_pointer = false;
    static constexpr bool is_reference = true;
    static constexpr bool is_const = std::is_const_v<T>;
    static constexpr bool is_volatile = std::is_volatile_v<T>;
    static constexpr const char* name = "rvalue_reference";
    
    using referenced_type = T;
};

/**
 * @brief Partial specialization for const types
 */
template<typename T>
struct TypeInfo<const T> {
    static constexpr bool is_pointer = TypeInfo<T>::is_pointer;
    static constexpr bool is_reference = TypeInfo<T>::is_reference;
    static constexpr bool is_const = true;
    static constexpr bool is_volatile = TypeInfo<T>::is_volatile;
    static constexpr const char* name = "const_qualified";
    
    using unqualified_type = T;
};

/**
 * @brief Partial specialization for volatile types
 */
template<typename T>
struct TypeInfo<volatile T> {
    static constexpr bool is_pointer = TypeInfo<T>::is_pointer;
    static constexpr bool is_reference = TypeInfo<T>::is_reference;
    static constexpr bool is_const = TypeInfo<T>::is_const;
    static constexpr bool is_volatile = true;
    static constexpr const char* name = "volatile_qualified";
    
    using unqualified_type = T;
};

/**
 * @brief Partial specialization for arrays
 */
template<typename T, std::size_t N>
struct TypeInfo<T[N]> {
    static constexpr bool is_pointer = false;
    static constexpr bool is_reference = false;
    static constexpr bool is_const = std::is_const_v<T>;
    static constexpr bool is_volatile = std::is_volatile_v<T>;
    static constexpr const char* name = "array";
    static constexpr std::size_t size = N;
    
    using element_type = T;
};

// ===== Container Specializations =====

/**
 * @brief Generic container printer
 */
template<typename T>
struct ContainerPrinter {
    static void print(const T& value) {
        std::cout << "Generic value: " << &value << std::endl;
    }
};

/**
 * @brief Partial specialization for std::vector
 */
template<typename T>
struct ContainerPrinter<std::vector<T>> {
    static void print(const std::vector<T>& vec) {
        std::cout << "Vector[" << vec.size() << "]: [";
        for (std::size_t i = 0; i < vec.size(); ++i) {
            if (i > 0) std::cout << ", ";
            std::cout << vec[i];
        }
        std::cout << "]" << std::endl;
    }
};

/**
 * @brief Partial specialization for std::array
 */
template<typename T, std::size_t N>
struct ContainerPrinter<std::array<T, N>> {
    static void print(const std::array<T, N>& arr) {
        std::cout << "Array[" << N << "]: [";
        for (std::size_t i = 0; i < N; ++i) {
            if (i > 0) std::cout << ", ";
            std::cout << arr[i];
        }
        std::cout << "]" << std::endl;
    }
};

/**
 * @brief Partial specialization for smart pointers
 */
template<typename T>
struct ContainerPrinter<std::unique_ptr<T>> {
    static void print(const std::unique_ptr<T>& ptr) {
        if (ptr) {
            std::cout << "unique_ptr -> " << *ptr << std::endl;
        } else {
            std::cout << "unique_ptr -> null" << std::endl;
        }
    }
};

template<typename T>
struct ContainerPrinter<std::shared_ptr<T>> {
    static void print(const std::shared_ptr<T>& ptr) {
        if (ptr) {
            std::cout << "shared_ptr[" << ptr.use_count() << "] -> " << *ptr << std::endl;
        } else {
            std::cout << "shared_ptr -> null" << std::endl;
        }
    }
};

// ===== Advanced Specialization Patterns =====

/**
 * @brief Template for handling different iterator categories
 */
template<typename Iterator, typename Category = typename std::iterator_traits<Iterator>::iterator_category>
struct IteratorHelper;

/**
 * @brief Specialization for input iterators
 */
template<typename Iterator>
struct IteratorHelper<Iterator, std::input_iterator_tag> {
    static constexpr const char* category_name = "input_iterator";
    
    static std::size_t distance(Iterator first, Iterator last) {
        std::size_t count = 0;
        while (first != last) {
            ++first;
            ++count;
        }
        return count;
    }
    
    static void advance(Iterator& it, std::size_t n) {
        while (n-- > 0) {
            ++it;
        }
    }
};

/**
 * @brief Specialization for forward iterators
 */
template<typename Iterator>
struct IteratorHelper<Iterator, std::forward_iterator_tag> {
    static constexpr const char* category_name = "forward_iterator";
    
    static std::size_t distance(Iterator first, Iterator last) {
        std::size_t count = 0;
        while (first != last) {
            ++first;
            ++count;
        }
        return count;
    }
    
    static void advance(Iterator& it, std::size_t n) {
        while (n-- > 0) {
            ++it;
        }
    }
};

/**
 * @brief Specialization for bidirectional iterators
 */
template<typename Iterator>
struct IteratorHelper<Iterator, std::bidirectional_iterator_tag> {
    static constexpr const char* category_name = "bidirectional_iterator";
    
    static std::size_t distance(Iterator first, Iterator last) {
        std::size_t count = 0;
        while (first != last) {
            ++first;
            ++count;
        }
        return count;
    }
    
    static void advance(Iterator& it, std::size_t n) {
        while (n-- > 0) {
            ++it;
        }
    }
    
    static void retreat(Iterator& it, std::size_t n) {
        while (n-- > 0) {
            --it;
        }
    }
};

/**
 * @brief Specialization for random access iterators
 */
template<typename Iterator>
struct IteratorHelper<Iterator, std::random_access_iterator_tag> {
    static constexpr const char* category_name = "random_access_iterator";
    
    static std::size_t distance(Iterator first, Iterator last) {
        return static_cast<std::size_t>(last - first);
    }
    
    static void advance(Iterator& it, std::size_t n) {
        it += static_cast<typename std::iterator_traits<Iterator>::difference_type>(n);
    }
    
    static void retreat(Iterator& it, std::size_t n) {
        it -= static_cast<typename std::iterator_traits<Iterator>::difference_type>(n);
    }
};

// ===== Template Specialization for Function Objects =====

/**
 * @brief Generic function wrapper
 */
template<typename F>
struct FunctionWrapper;

/**
 * @brief Specialization for function pointers
 */
template<typename R, typename... Args>
struct FunctionWrapper<R(*)(Args...)> {
    using function_type = R(*)(Args...);
    using return_type = R;
    using argument_types = std::tuple<Args...>;
    
    static constexpr std::size_t arity = sizeof...(Args);
    static constexpr bool is_noexcept = false;
    static constexpr const char* type_name = "function_pointer";
    
    function_type func;
    
    explicit FunctionWrapper(function_type f) : func(f) {}
    
    R operator()(Args... args) const {
        return func(args...);
    }
};

/**
 * @brief Specialization for member function pointers
 */
template<typename R, typename C, typename... Args>
struct FunctionWrapper<R(C::*)(Args...)> {
    using function_type = R(C::*)(Args...);
    using return_type = R;
    using class_type = C;
    using argument_types = std::tuple<Args...>;
    
    static constexpr std::size_t arity = sizeof...(Args);
    static constexpr bool is_noexcept = false;
    static constexpr const char* type_name = "member_function_pointer";
    
    function_type func;
    
    explicit FunctionWrapper(function_type f) : func(f) {}
    
    R operator()(C& obj, Args... args) const {
        return (obj.*func)(args...);
    }
    
    R operator()(C* obj, Args... args) const {
        return (obj->*func)(args...);
    }
};

/**
 * @brief Specialization for const member function pointers
 */
template<typename R, typename C, typename... Args>
struct FunctionWrapper<R(C::*)(Args...) const> {
    using function_type = R(C::*)(Args...) const;
    using return_type = R;
    using class_type = C;
    using argument_types = std::tuple<Args...>;
    
    static constexpr std::size_t arity = sizeof...(Args);
    static constexpr bool is_noexcept = false;
    static constexpr const char* type_name = "const_member_function_pointer";
    
    function_type func;
    
    explicit FunctionWrapper(function_type f) : func(f) {}
    
    R operator()(const C& obj, Args... args) const {
        return (obj.*func)(args...);
    }
    
    R operator()(const C* obj, Args... args) const {
        return (obj->*func)(args...);
    }
};

// ===== Tuple Specialization =====

/**
 * @brief Generic tuple processor
 */
template<typename T>
struct TupleProcessor {
    static void process(const T&) {
        std::cout << "Not a tuple" << std::endl;
    }
};

/**
 * @brief Partial specialization for std::tuple
 */
template<typename... Types>
struct TupleProcessor<std::tuple<Types...>> {
    static void process(const std::tuple<Types...>& t) {
        std::cout << "Tuple with " << sizeof...(Types) << " elements: ";
        process_elements(t, std::index_sequence_for<Types...>{});
        std::cout << std::endl;
    }
    
private:
    template<std::size_t... Indices>
    static void process_elements(const std::tuple<Types...>& t, std::index_sequence<Indices...>) {
        ((std::cout << std::get<Indices>(t) << (Indices < sizeof...(Types) - 1 ? ", " : "")), ...);
    }
};

/**
 * @brief Specialization for std::pair
 */
template<typename T1, typename T2>
struct TupleProcessor<std::pair<T1, T2>> {
    static void process(const std::pair<T1, T2>& p) {
        std::cout << "Pair: (" << p.first << ", " << p.second << ")" << std::endl;
    }
};

// ===== Optional and Variant Specializations =====

/**
 * @brief Generic option handler
 */
template<typename T>
struct OptionHandler {
    static void handle(const T& value) {
        std::cout << "Value: " << value << std::endl;
    }
};

/**
 * @brief Specialization for std::optional
 */
template<typename T>
struct OptionHandler<std::optional<T>> {
    static void handle(const std::optional<T>& opt) {
        if (opt) {
            std::cout << "Optional has value: " << *opt << std::endl;
        } else {
            std::cout << "Optional is empty" << std::endl;
        }
    }
};

/**
 * @brief Partial specialization for std::variant
 */
template<typename... Types>
struct OptionHandler<std::variant<Types...>> {
    static void handle(const std::variant<Types...>& var) {
        std::cout << "Variant holds alternative " << var.index() << ": ";
        std::visit([](const auto& value) {
            std::cout << value;
        }, var);
        std::cout << std::endl;
    }
};

// ===== SFINAE-based Specialization =====

/**
 * @brief Template with SFINAE-based partial specialization
 */
template<typename T, typename Enable = void>
struct AlgorithmSelector {
    static void sort(T& container) {
        std::cout << "Generic sort for unknown container type" << std::endl;
    }
};

/**
 * @brief Specialization for containers with random access iterators
 */
template<typename T>
struct AlgorithmSelector<T, std::enable_if_t<
    std::is_same_v<
        typename std::iterator_traits<typename T::iterator>::iterator_category,
        std::random_access_iterator_tag
    >
>> {
    static void sort(T& container) {
        std::cout << "Quick sort for random access container" << std::endl;
        std::sort(container.begin(), container.end());
    }
};

/**
 * @brief Specialization for containers with bidirectional iterators
 */
template<typename T>
struct AlgorithmSelector<T, std::enable_if_t<
    std::is_same_v<
        typename std::iterator_traits<typename T::iterator>::iterator_category,
        std::bidirectional_iterator_tag
    > && !std::is_same_v<
        typename std::iterator_traits<typename T::iterator>::iterator_category,
        std::random_access_iterator_tag
    >
>> {
    static void sort(T& container) {
        std::cout << "Merge sort for bidirectional container" << std::endl;
        // Would implement merge sort here
    }
};

// ===== Variable Template Specialization =====

/**
 * @brief Generic variable template
 */
template<typename T>
constexpr bool is_numeric_v = std::is_arithmetic_v<T>;

/**
 * @brief Specialization for std::string (not numeric)
 */
template<>
constexpr bool is_numeric_v<std::string> = false;

/**
 * @brief Variable template for type size categories
 */
template<typename T>
constexpr const char* size_category_v = "unknown";

template<>
constexpr const char* size_category_v<char> = "tiny";

template<>
constexpr const char* size_category_v<short> = "small";

template<>
constexpr const char* size_category_v<int> = "medium";

template<>
constexpr const char* size_category_v<long> = "large";

template<>
constexpr const char* size_category_v<long long> = "huge";

// ===== Alias Template Specialization =====

/**
 * @brief Generic container element type extractor
 */
template<typename T>
using element_type_t = typename T::value_type;

/**
 * @brief Specialization for arrays
 */
template<typename T, std::size_t N>
using element_type_t<T[N]> = T;

/**
 * @brief Generic pointer type maker
 */
template<typename T>
using pointer_t = T*;

/**
 * @brief Smart pointer type maker
 */
template<typename T>
using unique_pointer_t = std::unique_ptr<T>;

template<typename T>
using shared_pointer_t = std::shared_ptr<T>;

// ===== Template Specialization Utilities =====

/**
 * @brief Helper to test template specializations
 */
template<typename T>
void test_specializations(const T& value) {
    std::cout << "\n=== Testing Specializations for Type ===\n";
    
    // Test TypeInfo specialization
    std::cout << "TypeInfo:\n";
    std::cout << "  Is pointer: " << TypeInfo<T>::is_pointer << "\n";
    std::cout << "  Is reference: " << TypeInfo<T>::is_reference << "\n";
    std::cout << "  Is const: " << TypeInfo<T>::is_const << "\n";
    std::cout << "  Type name: " << TypeInfo<T>::name << "\n";
    
    // Test ContainerPrinter specialization
    std::cout << "ContainerPrinter:\n  ";
    ContainerPrinter<T>::print(value);
    
    // Test TupleProcessor specialization
    std::cout << "TupleProcessor:\n  ";
    TupleProcessor<T>::process(value);
    
    // Test OptionHandler specialization
    std::cout << "OptionHandler:\n  ";
    OptionHandler<T>::handle(value);
    
    std::cout << "========================================\n";
}

/**
 * @brief Demo function to show all specializations in action
 */
inline void demonstrate_specializations() {
    std::cout << "=== Template Specialization Demonstrations ===\n";
    
    // Test basic type specializations
    int i = 42;
    test_specializations(i);
    
    // Test pointer specialization
    int* pi = &i;
    test_specializations(pi);
    
    // Test reference specialization (would need to be called differently)
    
    // Test container specializations
    std::vector<int> vec = {1, 2, 3, 4, 5};
    test_specializations(vec);
    
    std::array<double, 3> arr = {1.1, 2.2, 3.3};
    test_specializations(arr);
    
    // Test smart pointer specializations
    auto unique_ptr = std::make_unique<int>(100);
    test_specializations(unique_ptr);
    
    auto shared_ptr = std::make_shared<std::string>("Hello");
    test_specializations(shared_ptr);
    
    // Test tuple specializations
    std::tuple<int, double, std::string> tup = {1, 2.5, "test"};
    test_specializations(tup);
    
    std::pair<int, std::string> p = {42, "answer"};
    test_specializations(p);
    
    // Test optional specializations
    std::optional<int> opt_full = 123;
    test_specializations(opt_full);
    
    std::optional<int> opt_empty;
    test_specializations(opt_empty);
    
    // Test variant specializations
    std::variant<int, double, std::string> var = std::string("variant");
    test_specializations(var);
    
    std::cout << "===============================================\n";
}

} // namespace Specialization
} // namespace Templates
} // namespace CppVerseHub

#endif // TEMPLATE_SPECIALIZATION_HPP