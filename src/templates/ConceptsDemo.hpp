// File: src/templates/ConceptsDemo.hpp
// C++20 concepts usage and demonstrations

#ifndef CONCEPTS_DEMO_HPP
#define CONCEPTS_DEMO_HPP

#include <concepts>
#include <type_traits>
#include <iterator>
#include <ranges>
#include <functional>
#include <iostream>
#include <vector>
#include <string>
#include <memory>
#include <numeric>
#include <algorithm>

namespace CppVerseHub {
namespace Templates {
namespace Concepts {

// ===== Basic Concepts =====

/**
 * @brief Concept for arithmetic types
 */
template<typename T>
concept Arithmetic = std::is_arithmetic_v<T>;

/**
 * @brief Concept for integral types
 */
template<typename T>
concept Integral = std::integral<T>;

/**
 * @brief Concept for floating point types
 */
template<typename T>
concept FloatingPoint = std::floating_point<T>;

/**
 * @brief Concept for signed types
 */
template<typename T>
concept Signed = std::signed_integral<T> || std::floating_point<T>;

/**
 * @brief Concept for unsigned types
 */
template<typename T>
concept Unsigned = std::unsigned_integral<T>;

// ===== Custom Concepts =====

/**
 * @brief Concept for types with size() method
 */
template<typename T>
concept HasSize = requires(T t) {
    { t.size() } -> std::convertible_to<std::size_t>;
};

/**
 * @brief Concept for types with begin() and end() methods
 */
template<typename T>
concept Iterable = requires(T t) {
    { t.begin() } -> std::input_or_output_iterator;
    { t.end() } -> std::input_or_output_iterator;
};

/**
 * @brief Concept for container types
 */
template<typename T>
concept Container = Iterable<T> && HasSize<T> && requires(T t) {
    typename T::value_type;
    typename T::iterator;
    typename T::const_iterator;
    { t.empty() } -> std::convertible_to<bool>;
};

/**
 * @brief Concept for types with push_back method
 */
template<typename T>
concept PushBackable = requires(T t, typename T::value_type v) {
    t.push_back(v);
};

/**
 * @brief Concept for sequence containers
 */
template<typename T>
concept SequenceContainer = Container<T> && PushBackable<T> && requires(T t) {
    { t.front() } -> std::convertible_to<typename T::value_type&>;
    { t.back() } -> std::convertible_to<typename T::value_type&>;
};

/**
 * @brief Concept for associative containers
 */
template<typename T>
concept AssociativeContainer = Container<T> && requires(T t) {
    typename T::key_type;
    typename T::mapped_type;
    { t.find(typename T::key_type{}) } -> std::convertible_to<typename T::iterator>;
};

/**
 * @brief Concept for printable types
 */
template<typename T>
concept Printable = requires(T t, std::ostream& os) {
    { os << t } -> std::convertible_to<std::ostream&>;
};

/**
 * @brief Concept for comparable types
 */
template<typename T>
concept Comparable = requires(T a, T b) {
    { a == b } -> std::convertible_to<bool>;
    { a != b } -> std::convertible_to<bool>;
    { a < b } -> std::convertible_to<bool>;
    { a <= b } -> std::convertible_to<bool>;
    { a > b } -> std::convertible_to<bool>;
    { a >= b } -> std::convertible_to<bool>;
};

/**
 * @brief Concept for hashable types
 */
template<typename T>
concept Hashable = requires(T t) {
    { std::hash<T>{}(t) } -> std::convertible_to<std::size_t>;
};

/**
 * @brief Concept for copyable types
 */
template<typename T>
concept Copyable = std::copy_constructible<T> && std::assignable_from<T&, const T&>;

/**
 * @brief Concept for movable types
 */
template<typename T>
concept Movable = std::move_constructible<T> && std::assignable_from<T&, T>;

/**
 * @brief Concept for smart pointer types
 */
template<typename T>
concept SmartPointer = requires(T t) {
    { *t };
    { t.get() } -> std::convertible_to<typename T::element_type*>;
    { t.reset() } -> std::convertible_to<void>;
    { static_cast<bool>(t) } -> std::convertible_to<bool>;
};

// ===== Advanced Concepts =====

/**
 * @brief Concept for function objects
 */
template<typename F, typename... Args>
concept Invocable = std::invocable<F, Args...>;

/**
 * @brief Concept for predicates
 */
template<typename F, typename... Args>
concept Predicate = Invocable<F, Args...> && 
                   std::convertible_to<std::invoke_result_t<F, Args...>, bool>;

/**
 * @brief Concept for unary predicates
 */
template<typename F, typename T>
concept UnaryPredicate = Predicate<F, T>;

/**
 * @brief Concept for binary predicates
 */
template<typename F, typename T>
concept BinaryPredicate = Predicate<F, T, T>;

/**
 * @brief Concept for ranges
 */
template<typename R>
concept Range = std::ranges::range<R>;

/**
 * @brief Concept for random access ranges
 */
template<typename R>
concept RandomAccessRange = std::ranges::random_access_range<R>;

/**
 * @brief Concept for numeric types
 */
template<typename T>
concept Numeric = Arithmetic<T> && requires(T a, T b) {
    { a + b } -> std::convertible_to<T>;
    { a - b } -> std::convertible_to<T>;
    { a * b } -> std::convertible_to<T>;
    { a / b } -> std::convertible_to<T>;
};

/**
 * @brief Concept for additive types
 */
template<typename T>
concept Additive = requires(T a, T b) {
    { a + b } -> std::convertible_to<T>;
    { a += b } -> std::convertible_to<T&>;
};

/**
 * @brief Concept for multiplicative types
 */
template<typename T>
concept Multiplicative = requires(T a, T b) {
    { a * b } -> std::convertible_to<T>;
    { a *= b } -> std::convertible_to<T&>;
};

/**
 * @brief Concept for ring-like types (additive + multiplicative)
 */
template<typename T>
concept Ring = Additive<T> && Multiplicative<T> && requires(T a) {
    { -a } -> std::convertible_to<T>;
    T{0}; // Zero element
    T{1}; // Unity element
};

/**
 * @brief Concept for field-like types
 */
template<typename T>
concept Field = Ring<T> && requires(T a, T b) {
    { a / b } -> std::convertible_to<T>;
    { a /= b } -> std::convertible_to<T&>;
};

/**
 * @brief Concept for serializable types
 */
template<typename T>
concept Serializable = requires(T t, std::ostream& os, std::istream& is) {
    { os << t } -> std::convertible_to<std::ostream&>;
    { is >> t } -> std::convertible_to<std::istream&>;
};

// ===== Concept-based Function Templates =====

/**
 * @brief Print function for printable types
 */
template<Printable T>
void print(const T& value) {
    std::cout << value << std::endl;
}

/**
 * @brief Print function for containers
 */
template<Container T>
    requires Printable<typename T::value_type>
void print(const T& container) {
    std::cout << "[";
    bool first = true;
    for (const auto& item : container) {
        if (!first) std::cout << ", ";
        std::cout << item;
        first = false;
    }
    std::cout << "]" << std::endl;
}

/**
 * @brief Generic algorithm for numeric operations
 */
template<Numeric T>
constexpr T power(T base, Unsigned auto exponent) {
    if (exponent == 0) return T{1};
    if (exponent == 1) return base;
    
    T result = T{1};
    while (exponent > 0) {
        if (exponent & 1) result *= base;
        base *= base;
        exponent >>= 1;
    }
    return result;
}

/**
 * @brief Sum function for additive types
 */
template<Range R>
    requires Additive<std::ranges::range_value_t<R>>
constexpr auto sum_range(const R& range) {
    using T = std::ranges::range_value_t<R>;
    return std::accumulate(std::ranges::begin(range), std::ranges::end(range), T{});
}

/**
 * @brief Product function for multiplicative types
 */
template<Range R>
    requires Multiplicative<std::ranges::range_value_t<R>>
constexpr auto product_range(const R& range) {
    using T = std::ranges::range_value_t<R>;
    return std::accumulate(std::ranges::begin(range), std::ranges::end(range), T{1}, std::multiplies<T>{});
}

/**
 * @brief Generic sort function
 */
template<std::ranges::random_access_range R>
    requires std::sortable<std::ranges::iterator_t<R>>
void sort_range(R&& range) {
    std::ranges::sort(range);
}

/**
 * @brief Generic sort with comparator
 */
template<std::ranges::random_access_range R, typename Compare>
    requires std::sortable<std::ranges::iterator_t<R>, Compare>
void sort_range(R&& range, Compare comp) {
    std::ranges::sort(range, comp);
}

/**
 * @brief Generic find function
 */
template<std::ranges::input_range R, typename T>
    requires std::equality_comparable_with<std::ranges::range_value_t<R>, T>
constexpr std::ranges::iterator_t<R> find_in_range(R&& range, const T& value) {
    return std::ranges::find(range, value);
}

/**
 * @brief Generic find_if function
 */
template<std::ranges::input_range R, UnaryPredicate<std::ranges::range_value_t<R>> Pred>
constexpr std::ranges::iterator_t<R> find_if_in_range(R&& range, Pred pred) {
    return std::ranges::find_if(range, pred);
}

/**
 * @brief Generic copy function
 */
template<std::ranges::input_range InputRange, std::weakly_incrementable OutputIt>
    requires std::indirectly_copyable<std::ranges::iterator_t<InputRange>, OutputIt>
constexpr OutputIt copy_range(InputRange&& input, OutputIt output) {
    return std::ranges::copy(input, output).out;
}

/**
 * @brief Generic transform function
 */
template<std::ranges::input_range InputRange, 
         std::weakly_incrementable OutputIt, 
         typename UnaryOp>
    requires std::indirectly_writable<OutputIt, std::invoke_result_t<UnaryOp, std::ranges::range_reference_t<InputRange>>>
constexpr OutputIt transform_range(InputRange&& input, OutputIt output, UnaryOp op) {
    return std::ranges::transform(input, output, op).out;
}

// ===== Concept-based Class Templates =====

/**
 * @brief Generic container adapter
 */
template<Container C>
class ContainerAdapter {
private:
    C container_;
    
public:
    using container_type = C;
    using value_type = typename C::value_type;
    using size_type = typename C::size_type;
    
    ContainerAdapter() = default;
    explicit ContainerAdapter(const C& container) : container_(container) {}
    explicit ContainerAdapter(C&& container) : container_(std::move(container)) {}
    
    size_type size() const { return container_.size(); }
    bool empty() const { return container_.empty(); }
    
    auto begin() -> decltype(container_.begin()) { return container_.begin(); }
    auto end() -> decltype(container_.end()) { return container_.end(); }
    auto begin() const -> decltype(container_.begin()) { return container_.begin(); }
    auto end() const -> decltype(container_.end()) { return container_.end(); }
    
    // Only available for sequence containers
    template<typename Self = C>
        requires SequenceContainer<Self>
    value_type& front() { return container_.front(); }
    
    template<typename Self = C>
        requires SequenceContainer<Self>
    const value_type& front() const { return container_.front(); }
    
    template<typename Self = C>
        requires SequenceContainer<Self>
    value_type& back() { return container_.back(); }
    
    template<typename Self = C>
        requires SequenceContainer<Self>
    const value_type& back() const { return container_.back(); }
    
    template<typename Self = C>
        requires SequenceContainer<Self>
    void push_back(const value_type& value) { container_.push_back(value); }
    
    template<typename Self = C>
        requires SequenceContainer<Self>
    void push_back(value_type&& value) { container_.push_back(std::move(value)); }
};

/**
 * @brief Generic mathematical vector
 */
template<Field T>
class Vector {
private:
    std::vector<T> data_;
    
public:
    using value_type = T;
    using size_type = std::size_t;
    using iterator = typename std::vector<T>::iterator;
    using const_iterator = typename std::vector<T>::const_iterator;
    
    Vector() = default;
    explicit Vector(size_type size) : data_(size) {}
    Vector(size_type size, const T& value) : data_(size, value) {}
    Vector(std::initializer_list<T> init) : data_(init) {}
    
    size_type size() const { return data_.size(); }
    bool empty() const { return data_.empty(); }
    
    T& operator[](size_type index) { return data_[index]; }
    const T& operator[](size_type index) const { return data_[index]; }
    
    T& at(size_type index) { return data_.at(index); }
    const T& at(size_type index) const { return data_.at(index); }
    
    iterator begin() { return data_.begin(); }
    iterator end() { return data_.end(); }
    const_iterator begin() const { return data_.begin(); }
    const_iterator end() const { return data_.end(); }
    const_iterator cbegin() const { return data_.cbegin(); }
    const_iterator cend() const { return data_.cend(); }
    
    void push_back(const T& value) { data_.push_back(value); }
    void push_back(T&& value) { data_.push_back(std::move(value)); }
    
    void resize(size_type size) { data_.resize(size); }
    void resize(size_type size, const T& value) { data_.resize(size, value); }
    
    // Vector operations
    Vector& operator+=(const Vector& other) {
        if (size() != other.size()) {
            throw std::invalid_argument("Vector sizes must match for addition");
        }
        for (size_type i = 0; i < size(); ++i) {
            data_[i] += other[i];
        }
        return *this;
    }
    
    Vector& operator-=(const Vector& other) {
        if (size() != other.size()) {
            throw std::invalid_argument("Vector sizes must match for subtraction");
        }
        for (size_type i = 0; i < size(); ++i) {
            data_[i] -= other[i];
        }
        return *this;
    }
    
    Vector& operator*=(const T& scalar) {
        for (auto& element : data_) {
            element *= scalar;
        }
        return *this;
    }
    
    Vector& operator/=(const T& scalar) {
        for (auto& element : data_) {
            element /= scalar;
        }
        return *this;
    }
    
    Vector operator+(const Vector& other) const {
        Vector result = *this;
        result += other;
        return result;
    }
    
    Vector operator-(const Vector& other) const {
        Vector result = *this;
        result -= other;
        return result;
    }
    
    Vector operator*(const T& scalar) const {
        Vector result = *this;
        result *= scalar;
        return result;
    }
    
    Vector operator/(const T& scalar) const {
        Vector result = *this;
        result /= scalar;
        return result;
    }
    
    // Dot product
    T dot(const Vector& other) const {
        if (size() != other.size()) {
            throw std::invalid_argument("Vector sizes must match for dot product");
        }
        T result = T{};
        for (size_type i = 0; i < size(); ++i) {
            result += data_[i] * other[i];
        }
        return result;
    }
    
    // Magnitude
    T magnitude() const 
        requires requires(T t) { { std::sqrt(t) } -> std::convertible_to<T>; }
    {
        return std::sqrt(dot(*this));
    }
    
    // Normalize
    Vector normalized() const 
        requires requires(T t) { { std::sqrt(t) } -> std::convertible_to<T>; }
    {
        T mag = magnitude();
        if (mag == T{}) {
            throw std::invalid_argument("Cannot normalize zero vector");
        }
        return *this / mag;
    }
};

/**
 * @brief Generic smart pointer wrapper
 */
template<SmartPointer P>
class SmartPtrWrapper {
private:
    P ptr_;
    
public:
    using pointer_type = P;
    using element_type = typename P::element_type;
    
    SmartPtrWrapper() = default;
    explicit SmartPtrWrapper(P ptr) : ptr_(std::move(ptr)) {}
    
    element_type& operator*() { return *ptr_; }
    const element_type& operator*() const { return *ptr_; }
    
    element_type* operator->() { return ptr_.get(); }
    const element_type* operator->() const { return ptr_.get(); }
    
    element_type* get() { return ptr_.get(); }
    const element_type* get() const { return ptr_.get(); }
    
    explicit operator bool() const { return static_cast<bool>(ptr_); }
    
    void reset() { ptr_.reset(); }
    
    template<typename... Args>
        requires requires(P p, Args... args) { p.reset(args...); }
    void reset(Args&&... args) { 
        ptr_.reset(std::forward<Args>(args)...); 
    }
    
    P& get_pointer() { return ptr_; }
    const P& get_pointer() const { return ptr_; }
};

// ===== Constrained Algorithms =====

/**
 * @brief Generic algorithms with concept constraints
 */
namespace algorithms {

template<std::ranges::input_range R, UnaryPredicate<std::ranges::range_value_t<R>> Pred>
constexpr bool all_of(R&& range, Pred pred) {
    return std::ranges::all_of(range, pred);
}

template<std::ranges::input_range R, UnaryPredicate<std::ranges::range_value_t<R>> Pred>
constexpr bool any_of(R&& range, Pred pred) {
    return std::ranges::any_of(range, pred);
}

template<std::ranges::input_range R, UnaryPredicate<std::ranges::range_value_t<R>> Pred>
constexpr bool none_of(R&& range, Pred pred) {
    return std::ranges::none_of(range, pred);
}

template<std::ranges::input_range R, UnaryPredicate<std::ranges::range_value_t<R>> Pred>
constexpr auto count_if(R&& range, Pred pred) {
    return std::ranges::count_if(range, pred);
}

template<std::ranges::forward_range R>
    requires std::equality_comparable<std::ranges::range_value_t<R>>
constexpr std::ranges::iterator_t<R> unique(R&& range) {
    return std::ranges::unique(range).begin();
}

template<std::ranges::random_access_range R, BinaryPredicate<std::ranges::range_value_t<R>> Compare>
constexpr void sort(R&& range, Compare comp) {
    std::ranges::sort(range, comp);
}

template<std::ranges::random_access_range R>
    requires std::sortable<std::ranges::iterator_t<R>>
constexpr void sort(R&& range) {
    std::ranges::sort(range);
}

} // namespace algorithms

// ===== Concept Testing Utilities =====

/**
 * @brief Test if a type satisfies a concept
 */
#define TEST_CONCEPT(ConceptName, Type) \
    static_assert(ConceptName<Type>, #Type " should satisfy " #ConceptName); \
    constexpr bool test_##ConceptName##_##Type = ConceptName<Type>

/**
 * @brief Concept validation at compile time
 */
template<typename T>
constexpr void validate_concept_requirements() {
    // Test basic concepts
    constexpr bool is_arithmetic = Arithmetic<T>;
    constexpr bool is_printable = Printable<T>;
    constexpr bool is_comparable = Comparable<T>;
    constexpr bool is_hashable = Hashable<T>;
    
    // Print results (will be optimized away)
    if constexpr (is_arithmetic) {
        // T satisfies Arithmetic concept
    }
    if constexpr (is_printable) {
        // T satisfies Printable concept
    }
    if constexpr (is_comparable) {
        // T satisfies Comparable concept
    }
    if constexpr (is_hashable) {
        // T satisfies Hashable concept
    }
}

} // namespace Concepts
} // namespace Templates
} // namespace CppVerseHub

#endif // CONCEPTS_DEMO_HPP