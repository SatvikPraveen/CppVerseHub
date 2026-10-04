/**
 * @file ConceptsDemo.hpp
 * @brief C++20 concepts: defining, composing and constraining with named requirements.
 *
 * Demonstrates
 *  - building a concept hierarchy by conjunction so that the compiler can apply *subsumption*
 *    (the more constrained overload wins, see `classify`);
 *  - requires-expressions checking member functions, nested types and expression validity;
 *  - abbreviated function templates (`Unsigned auto exponent`);
 *  - constrained class templates and constrained member functions (`ContainerAdapter`, `MathVector`);
 *  - concept-constrained wrappers around `std::ranges` algorithms.
 *
 * Concepts replace most uses of SFINAE (compare with SFINAE_Examples.hpp): diagnostics name the
 * unsatisfied requirement instead of a failed substitution deep inside an overload set.
 */

#ifndef CPPVERSEHUB_TEMPLATES_CONCEPTS_DEMO_HPP
#define CPPVERSEHUB_TEMPLATES_CONCEPTS_DEMO_HPP

#include <algorithm>
#include <cmath>
#include <concepts>
#include <cstddef>
#include <functional>
#include <initializer_list>
#include <iostream>
#include <iterator>
#include <memory>
#include <numeric>
#include <ranges>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

namespace CppVerseHub::Templates::Concepts {

// ===== Basic concept hierarchy (built by conjunction so subsumption works) =====

/** @brief Satisfied by built-in arithmetic types (integral and floating point, including bool/char). */
template <typename T>
concept Arithmetic = std::is_arithmetic_v<T>;

/** @brief Integral arithmetic types; refines (subsumes) Arithmetic. */
template <typename T>
concept Integral = Arithmetic<T> && std::integral<T>;

/** @brief Signed integral types; refines (subsumes) Integral. */
template <typename T>
concept SignedIntegral = Integral<T> && std::signed_integral<T>;

/** @brief Unsigned integral types; refines (subsumes) Integral. */
template <typename T>
concept Unsigned = Integral<T> && std::unsigned_integral<T>;

/** @brief Floating point types; refines (subsumes) Arithmetic. */
template <typename T>
concept FloatingPoint = Arithmetic<T> && std::floating_point<T>;

/** @brief Types that can represent negative values (signed integers or floating point). */
template <typename T>
concept Signed = SignedIntegral<T> || FloatingPoint<T>;

// ===== Structural concepts (requires-expressions) =====

/** @brief Types exposing a `size()` convertible to std::size_t. */
template <typename T>
concept HasSize = requires(const T& t) {
    { t.size() } -> std::convertible_to<std::size_t>;
};

/** @brief Types with member `begin()`/`end()` returning iterators. */
template <typename T>
concept Iterable = requires(T& t) {
    { t.begin() } -> std::input_or_output_iterator;
    { t.end() } -> std::input_or_output_iterator;
};

/** @brief A minimal "Container" named requirement: iterable, sized, with the usual nested types. */
template <typename T>
concept Container = Iterable<T> && HasSize<T> && requires(const T& t) {
    typename T::value_type;
    typename T::iterator;
    typename T::const_iterator;
    { t.empty() } -> std::convertible_to<bool>;
};

/** @brief Containers that accept `push_back(value_type)`. */
template <typename T>
concept PushBackable = requires(T& t, const typename T::value_type& v) { t.push_back(v); };

/** @brief Sequence containers: Container + push_back + front/back access. */
template <typename T>
concept SequenceContainer = Container<T> && PushBackable<T> && requires(T& t) {
    { t.front() } -> std::same_as<typename T::value_type&>;
    { t.back() } -> std::same_as<typename T::value_type&>;
};

/** @brief Associative containers with key/mapped types and `find(key)`. */
template <typename T>
concept AssociativeContainer = Container<T> && requires(T& t, const typename T::key_type& k) {
    typename T::key_type;
    typename T::mapped_type;
    { t.find(k) } -> std::same_as<typename T::iterator>;
};

/** @brief Types that can be written to a std::ostream. */
template <typename T>
concept Printable = requires(const T& t, std::ostream& os) {
    { os << t } -> std::convertible_to<std::ostream&>;
};

/** @brief Types supporting all six relational operators with bool-convertible results. */
template <typename T>
concept Comparable = requires(const T& a, const T& b) {
    { a == b } -> std::convertible_to<bool>;
    { a != b } -> std::convertible_to<bool>;
    { a < b } -> std::convertible_to<bool>;
    { a <= b } -> std::convertible_to<bool>;
    { a > b } -> std::convertible_to<bool>;
    { a >= b } -> std::convertible_to<bool>;
};

/** @brief Types for which `std::hash<T>` is enabled. */
template <typename T>
concept Hashable = requires(const T& t) {
    { std::hash<T>{}(t) } -> std::convertible_to<std::size_t>;
};

/** @brief Copy-constructible and copy-assignable types. */
template <typename T>
concept Copyable = std::copy_constructible<T> && std::assignable_from<T&, const T&>;

/** @brief Move-constructible and move-assignable types. */
template <typename T>
concept Movable = std::move_constructible<T> && std::assignable_from<T&, T>;

/** @brief Smart-pointer-like types: dereferenceable, `get()`, `reset()`, and contextual bool. */
template <typename T>
concept SmartPointer = requires(T t) {
    typename T::element_type;
    { *t } -> std::same_as<typename T::element_type&>;
    { t.get() } -> std::convertible_to<typename T::element_type*>;
    t.reset();
    { static_cast<bool>(t) } -> std::same_as<bool>;
};

// ===== Callable concepts =====

/** @brief Callables invocable with Args (thin alias over std::invocable). */
template <typename F, typename... Args>
concept Invocable = std::invocable<F, Args...>;

/** @brief Callables invocable with Args whose result converts to bool. */
template <typename F, typename... Args>
concept Predicate = Invocable<F, Args...> && std::convertible_to<std::invoke_result_t<F, Args...>, bool>;

/** @brief Predicate over one T. */
template <typename F, typename T>
concept UnaryPredicate = Predicate<F, T>;

/** @brief Predicate over two T. */
template <typename F, typename T>
concept BinaryPredicate = Predicate<F, T, T>;

/** @brief Any std::ranges::range. */
template <typename R>
concept Range = std::ranges::range<R>;

/** @brief Any std::ranges::random_access_range. */
template <typename R>
concept RandomAccessRange = std::ranges::random_access_range<R>;

// ===== Algebraic concepts =====

/** @brief Arithmetic types closed under + - * /. */
template <typename T>
concept Numeric = Arithmetic<T> && requires(T a, T b) {
    { a + b } -> std::convertible_to<T>;
    { a - b } -> std::convertible_to<T>;
    { a * b } -> std::convertible_to<T>;
    { a / b } -> std::convertible_to<T>;
};

/** @brief Types supporting `+` and `+=`. */
template <typename T>
concept Additive = requires(T a, T b) {
    { a + b } -> std::convertible_to<T>;
    { a += b } -> std::convertible_to<T&>;
};

/** @brief Types supporting `*` and `*=`. */
template <typename T>
concept Multiplicative = requires(T a, T b) {
    { a * b } -> std::convertible_to<T>;
    { a *= b } -> std::convertible_to<T&>;
};

/** @brief Ring-like types: additive, multiplicative, negatable, with 0 and 1 constructible. */
template <typename T>
concept Ring = Additive<T> && Multiplicative<T> && requires(T a) {
    { -a } -> std::convertible_to<T>;
    T{0};
    T{1};
};

/**
 * @brief Field-like types: a Ring with division. Purely syntactic, so `int` also satisfies it;
 *        concepts check syntax, the semantic axioms remain the programmer's responsibility.
 */
template <typename T>
concept Field = Ring<T> && requires(T a, T b) {
    { a / b } -> std::convertible_to<T>;
    { a /= b } -> std::convertible_to<T&>;
};

/** @brief Types that can be both written to and read from streams. */
template <typename T>
concept Serializable = requires(T t, std::ostream& os, std::istream& is) {
    { os << t } -> std::convertible_to<std::ostream&>;
    { is >> t } -> std::convertible_to<std::istream&>;
};

// ===== Subsumption-based overloading =====

/** @brief Fallback for non-arithmetic types. @return "non-arithmetic". */
template <typename T>
[[nodiscard]] constexpr std::string_view classify(const T&) noexcept {
    return "non-arithmetic";
}

/** @brief Overload for arithmetic types. @return "arithmetic". */
template <Arithmetic T>
[[nodiscard]] constexpr std::string_view classify(const T&) noexcept {
    return "arithmetic";
}

/** @brief Integral subsumes Arithmetic, so this wins for integers. @return "integral". */
template <Integral T>
[[nodiscard]] constexpr std::string_view classify(const T&) noexcept {
    return "integral";
}

/** @brief SignedIntegral subsumes Integral. @return "signed integral". */
template <SignedIntegral T>
[[nodiscard]] constexpr std::string_view classify(const T&) noexcept {
    return "signed integral";
}

/** @brief FloatingPoint subsumes Arithmetic. @return "floating point". */
template <FloatingPoint T>
[[nodiscard]] constexpr std::string_view classify(const T&) noexcept {
    return "floating point";
}

// ===== Concept-constrained function templates =====

/**
 * @brief Format a Printable value as a string.
 * @param value value to format
 * @return the streamed representation
 */
template <Printable T>
[[nodiscard]] std::string format_value(const T& value) {
    std::ostringstream oss;
    oss << value;
    return oss.str();
}

/**
 * @brief Format a container of printable elements as "[a, b, c]".
 *
 * The `!Printable<C>` clause disambiguates types that are both containers and printable
 * (e.g. std::string), which would otherwise make the two overloads ambiguous.
 * @param container container to format
 * @return bracketed, comma separated representation
 */
template <Container C>
    requires(Printable<typename C::value_type> && !Printable<C>)
[[nodiscard]] std::string format_value(const C& container) {
    std::ostringstream oss;
    oss << '[';
    bool first = true;
    for (const auto& item : container) {
        if (!first) {
            oss << ", ";
        }
        oss << item;
        first = false;
    }
    oss << ']';
    return oss.str();
}

/**
 * @brief Write a value (or container) followed by a newline to a stream.
 * @param out destination stream
 * @param value value accepted by one of the format_value overloads
 */
template <typename T>
    requires requires(const T& v) { format_value(v); }
void print(std::ostream& out, const T& value) {
    out << format_value(value) << '\n';
}

/**
 * @brief Exponentiation by squaring for any Numeric base and unsigned exponent.
 * @param base base value
 * @param exponent non-negative exponent (abbreviated template parameter)
 * @return base raised to exponent
 */
template <Numeric T>
[[nodiscard]] constexpr T power(T base, Unsigned auto exponent) noexcept {
    T result = T{1};
    while (exponent > 0) {
        if ((exponent & 1U) != 0U) {
            result *= base;
        }
        exponent >>= 1U;
        if (exponent > 0) {
            base *= base;
        }
    }
    return result;
}

/**
 * @brief Sum the elements of a range whose value type is Additive.
 * @param range input range
 * @return sum, starting from a value-initialised element
 */
template <Range R>
    requires Additive<std::ranges::range_value_t<R>>
[[nodiscard]] constexpr auto sum_range(const R& range) {
    using T = std::ranges::range_value_t<R>;
    return std::accumulate(std::ranges::begin(range), std::ranges::end(range), T{});
}

/**
 * @brief Multiply the elements of a range whose value type is Multiplicative.
 * @param range input range
 * @return product, starting from T{1}
 */
template <Range R>
    requires Multiplicative<std::ranges::range_value_t<R>>
[[nodiscard]] constexpr auto product_range(const R& range) {
    using T = std::ranges::range_value_t<R>;
    return std::accumulate(std::ranges::begin(range), std::ranges::end(range), T{1}, std::multiplies<T>{});
}

/**
 * @brief Sort a random-access range in place.
 * @param range range to sort
 */
template <std::ranges::random_access_range R>
    requires std::sortable<std::ranges::iterator_t<R>>
constexpr void sort_range(R&& range) {
    std::ranges::sort(range);
}

/**
 * @brief Sort a random-access range in place using a comparator.
 * @param range range to sort
 * @param comp strict weak ordering
 */
template <std::ranges::random_access_range R, typename Compare>
    requires std::sortable<std::ranges::iterator_t<R>, Compare>
constexpr void sort_range(R&& range, Compare comp) {
    std::ranges::sort(range, comp);
}

/**
 * @brief Find the first element equal to value.
 * @param range input range
 * @param value value to look for
 * @return iterator to the element or end
 */
template <std::ranges::input_range R, typename T>
    requires std::equality_comparable_with<std::ranges::range_value_t<R>, T>
[[nodiscard]] constexpr std::ranges::borrowed_iterator_t<R> find_in_range(R&& range, const T& value) {
    return std::ranges::find(std::forward<R>(range), value);
}

/**
 * @brief Find the first element satisfying pred.
 * @param range input range
 * @param pred unary predicate
 * @return iterator to the element or end
 */
template <std::ranges::input_range R, UnaryPredicate<std::ranges::range_reference_t<R>> Pred>
[[nodiscard]] constexpr std::ranges::borrowed_iterator_t<R> find_if_in_range(R&& range, Pred pred) {
    return std::ranges::find_if(std::forward<R>(range), pred);
}

/**
 * @brief Copy a range to an output iterator.
 * @param input source range
 * @param output destination iterator
 * @return iterator past the last written element
 */
template <std::ranges::input_range InputRange, std::weakly_incrementable OutputIt>
    requires std::indirectly_copyable<std::ranges::iterator_t<InputRange>, OutputIt>
constexpr OutputIt copy_range(InputRange&& input, OutputIt output) {
    return std::ranges::copy(input, output).out;
}

/**
 * @brief Transform a range into an output iterator.
 * @param input source range
 * @param output destination iterator
 * @param op unary transformation
 * @return iterator past the last written element
 */
template <std::ranges::input_range InputRange, std::weakly_incrementable OutputIt, typename UnaryOp>
    requires std::indirectly_writable<OutputIt,
                                      std::invoke_result_t<UnaryOp&, std::ranges::range_reference_t<InputRange>>>
constexpr OutputIt transform_range(InputRange&& input, OutputIt output, UnaryOp op) {
    return std::ranges::transform(input, output, op).out;
}

// ===== Concept-constrained class templates =====

/**
 * @brief Wraps any Container; sequence-only operations are enabled through member constraints.
 * @tparam C wrapped container type
 */
template <Container C>
class ContainerAdapter {
public:
    using container_type = C;
    using value_type = typename C::value_type;
    using size_type = typename C::size_type;

    /** @brief Default-construct an empty adapter. */
    ContainerAdapter() = default;

    /** @brief Adopt a container. @param container container moved into the adapter */
    explicit ContainerAdapter(C container) : container_(std::move(container)) {}

    /** @return number of elements */
    [[nodiscard]] size_type size() const noexcept { return container_.size(); }
    /** @return true if empty */
    [[nodiscard]] bool empty() const noexcept { return container_.empty(); }

    /** @return iterator to first element */
    [[nodiscard]] auto begin() { return container_.begin(); }
    /** @return iterator past last element */
    [[nodiscard]] auto end() { return container_.end(); }
    /** @return const iterator to first element */
    [[nodiscard]] auto begin() const { return container_.begin(); }
    /** @return const iterator past last element */
    [[nodiscard]] auto end() const { return container_.end(); }

    /** @return first element (sequence containers only) */
    [[nodiscard]] value_type& front()
        requires SequenceContainer<C>
    {
        return container_.front();
    }

    /** @return last element (sequence containers only) */
    [[nodiscard]] value_type& back()
        requires SequenceContainer<C>
    {
        return container_.back();
    }

    /** @brief Append a value (sequence containers only). @param value element to append */
    void push_back(value_type value)
        requires SequenceContainer<C>
    {
        container_.push_back(std::move(value));
    }

    /** @return true if key is present (associative containers only) @param key key to search */
    [[nodiscard]] bool contains_key(const auto& key) const
        requires AssociativeContainer<C>
    {
        return container_.find(key) != container_.end();
    }

    /** @return the wrapped container */
    [[nodiscard]] const C& underlying() const noexcept { return container_; }

private:
    C container_{};
};

/**
 * @brief Dense mathematical vector over a Field-like scalar type.
 * @tparam T scalar type satisfying Field
 */
template <Field T>
class MathVector {
public:
    using value_type = T;
    using size_type = std::size_t;
    using iterator = typename std::vector<T>::iterator;
    using const_iterator = typename std::vector<T>::const_iterator;

    /** @brief Empty vector. */
    MathVector() = default;
    /** @brief Zero vector of given dimension. @param size dimension */
    explicit MathVector(size_type size) : data_(size) {}
    /** @brief Vector with every component set to value. @param size dimension @param value fill */
    MathVector(size_type size, const T& value) : data_(size, value) {}
    /** @brief Vector from components. @param init components */
    MathVector(std::initializer_list<T> init) : data_(init) {}

    /** @return dimension */
    [[nodiscard]] size_type size() const noexcept { return data_.size(); }
    /** @return true if dimension is zero */
    [[nodiscard]] bool empty() const noexcept { return data_.empty(); }

    /** @param index component index @return component reference (unchecked) */
    [[nodiscard]] T& operator[](size_type index) noexcept { return data_[index]; }
    /** @param index component index @return component (unchecked) */
    [[nodiscard]] const T& operator[](size_type index) const noexcept { return data_[index]; }
    /** @param index component index @return component reference @throws std::out_of_range */
    [[nodiscard]] T& at(size_type index) { return data_.at(index); }
    /** @param index component index @return component @throws std::out_of_range */
    [[nodiscard]] const T& at(size_type index) const { return data_.at(index); }

    /** @return iterator to first component */
    [[nodiscard]] iterator begin() noexcept { return data_.begin(); }
    /** @return iterator past last component */
    [[nodiscard]] iterator end() noexcept { return data_.end(); }
    /** @return const iterator to first component */
    [[nodiscard]] const_iterator begin() const noexcept { return data_.begin(); }
    /** @return const iterator past last component */
    [[nodiscard]] const_iterator end() const noexcept { return data_.end(); }

    /** @brief Component-wise addition. @param other same-sized vector @return *this @throws std::invalid_argument */
    MathVector& operator+=(const MathVector& other) {
        require_same_size(other, "addition");
        for (size_type i = 0; i < size(); ++i) {
            data_[i] += other[i];
        }
        return *this;
    }

    /** @brief Component-wise subtraction. @param other same-sized vector @return *this @throws std::invalid_argument */
    MathVector& operator-=(const MathVector& other) {
        require_same_size(other, "subtraction");
        for (size_type i = 0; i < size(); ++i) {
            data_[i] -= other[i];
        }
        return *this;
    }

    /** @brief Scale in place. @param scalar factor @return *this */
    MathVector& operator*=(const T& scalar) {
        for (auto& element : data_) {
            element *= scalar;
        }
        return *this;
    }

    /** @brief Divide in place. @param scalar divisor @return *this */
    MathVector& operator/=(const T& scalar) {
        for (auto& element : data_) {
            element /= scalar;
        }
        return *this;
    }

    /** @param lhs left operand @param rhs right operand @return lhs + rhs */
    [[nodiscard]] friend MathVector operator+(MathVector lhs, const MathVector& rhs) {
        lhs += rhs;
        return lhs;
    }
    /** @param lhs left operand @param rhs right operand @return lhs - rhs */
    [[nodiscard]] friend MathVector operator-(MathVector lhs, const MathVector& rhs) {
        lhs -= rhs;
        return lhs;
    }
    /** @param lhs vector @param scalar factor @return lhs * scalar */
    [[nodiscard]] friend MathVector operator*(MathVector lhs, const T& scalar) {
        lhs *= scalar;
        return lhs;
    }
    /** @param lhs vector @param scalar divisor @return lhs / scalar */
    [[nodiscard]] friend MathVector operator/(MathVector lhs, const T& scalar) {
        lhs /= scalar;
        return lhs;
    }
    /** @return true if all components are equal */
    [[nodiscard]] friend bool operator==(const MathVector&, const MathVector&) = default;

    /**
     * @brief Dot product.
     * @param other same-sized vector
     * @return sum of component products
     * @throws std::invalid_argument on size mismatch
     */
    [[nodiscard]] T dot(const MathVector& other) const {
        require_same_size(other, "dot product");
        T result{};
        for (size_type i = 0; i < size(); ++i) {
            result += data_[i] * other[i];
        }
        return result;
    }

    /** @return Euclidean norm (floating point scalars only) */
    [[nodiscard]] T magnitude() const
        requires FloatingPoint<T>
    {
        return std::sqrt(dot(*this));
    }

    /** @return unit vector in the same direction @throws std::invalid_argument for the zero vector */
    [[nodiscard]] MathVector normalized() const
        requires FloatingPoint<T>
    {
        const T mag = magnitude();
        if (mag == T{}) {
            throw std::invalid_argument("Cannot normalize zero vector");
        }
        return *this / mag;
    }

private:
    void require_same_size(const MathVector& other, const char* operation) const {
        if (size() != other.size()) {
            throw std::invalid_argument(std::string("MathVector sizes must match for ") + operation);
        }
    }

    std::vector<T> data_;
};

/**
 * @brief Thin wrapper accepting anything that models SmartPointer.
 * @tparam P smart pointer type (std::unique_ptr, std::shared_ptr, ...)
 */
template <SmartPointer P>
class SmartPtrWrapper {
public:
    using pointer_type = P;
    using element_type = typename P::element_type;

    /** @brief Empty wrapper. */
    SmartPtrWrapper() = default;
    /** @brief Take ownership of a smart pointer. @param ptr pointer to adopt */
    explicit SmartPtrWrapper(P ptr) noexcept : ptr_(std::move(ptr)) {}

    /** @return reference to the pointee (must not be null) */
    [[nodiscard]] element_type& operator*() const { return *ptr_; }
    /** @return raw pointer to the pointee */
    [[nodiscard]] element_type* operator->() const noexcept { return ptr_.get(); }
    /** @return raw pointer to the pointee */
    [[nodiscard]] element_type* get() const noexcept { return ptr_.get(); }
    /** @return true if non-null */
    [[nodiscard]] explicit operator bool() const noexcept { return static_cast<bool>(ptr_); }
    /** @brief Release the pointee. */
    void reset() noexcept { ptr_.reset(); }
    /** @return the underlying smart pointer */
    [[nodiscard]] const P& get_pointer() const noexcept { return ptr_; }

private:
    P ptr_{};
};

// ===== Constrained algorithm wrappers =====

/** @brief Thin, concept-constrained wrappers over std::ranges algorithms. */
namespace algorithms {

/** @param range input range @param pred predicate @return true if pred holds for all elements */
template <std::ranges::input_range R, UnaryPredicate<std::ranges::range_reference_t<R>> Pred>
[[nodiscard]] constexpr bool all_of(R&& range, Pred pred) {
    return std::ranges::all_of(range, pred);
}

/** @param range input range @param pred predicate @return true if pred holds for any element */
template <std::ranges::input_range R, UnaryPredicate<std::ranges::range_reference_t<R>> Pred>
[[nodiscard]] constexpr bool any_of(R&& range, Pred pred) {
    return std::ranges::any_of(range, pred);
}

/** @param range input range @param pred predicate @return true if pred holds for no element */
template <std::ranges::input_range R, UnaryPredicate<std::ranges::range_reference_t<R>> Pred>
[[nodiscard]] constexpr bool none_of(R&& range, Pred pred) {
    return std::ranges::none_of(range, pred);
}

/** @param range input range @param pred predicate @return number of elements satisfying pred */
template <std::ranges::input_range R, UnaryPredicate<std::ranges::range_reference_t<R>> Pred>
[[nodiscard]] constexpr auto count_if(R&& range, Pred pred) {
    return std::ranges::count_if(range, pred);
}

/**
 * @brief Remove consecutive duplicates (std::unique semantics).
 * @param range forward range
 * @return iterator to the new logical end
 */
template <std::ranges::forward_range R>
    requires std::permutable<std::ranges::iterator_t<R>> &&
             std::equality_comparable<std::ranges::range_value_t<R>>
constexpr std::ranges::iterator_t<R> unique(R& range) {
    return std::ranges::unique(range).begin();
}

/** @brief Sort with comparator. @param range range @param comp binary predicate */
template <std::ranges::random_access_range R, BinaryPredicate<std::ranges::range_reference_t<R>> Compare>
    requires std::sortable<std::ranges::iterator_t<R>, Compare>
constexpr void sort(R&& range, Compare comp) {
    std::ranges::sort(range, comp);
}

/** @brief Sort ascending. @param range range */
template <std::ranges::random_access_range R>
    requires std::sortable<std::ranges::iterator_t<R>>
constexpr void sort(R&& range) {
    std::ranges::sort(range);
}

} // namespace algorithms

// ===== Showcase =====

/**
 * @brief Walk through the concept-based utilities of this header.
 * @param out destination stream
 */
inline void demonstrate_concepts(std::ostream& out = std::cout) {
    out << "--- Concepts ---\n";
    out << "classify(42)        = " << classify(42) << '\n';
    out << "classify(42u)       = " << classify(42U) << '\n';
    out << "classify(3.14)      = " << classify(3.14) << '\n';
    out << "classify(string)    = " << classify(std::string("x")) << '\n';
    out << "power(2, 10u)       = " << power(2, 10U) << '\n';

    std::vector<int> values{5, 3, 8, 1, 9, 2};
    out << "values              = " << format_value(values) << '\n';
    sort_range(values);
    out << "sorted              = " << format_value(values) << '\n';
    out << "sum / product       = " << sum_range(values) << " / " << product_range(values) << '\n';
    out << "count_if(even)      = " << algorithms::count_if(values, [](int v) { return v % 2 == 0; }) << '\n';

    MathVector<double> a{3.0, 4.0};
    const MathVector<double> b{1.0, 2.0};
    out << "|(3,4)|             = " << a.magnitude() << '\n';
    out << "(3,4).(1,2)         = " << a.dot(b) << '\n';

    ContainerAdapter<std::vector<int>> adapter;
    adapter.push_back(7);
    adapter.push_back(11);
    out << "adapter front/back  = " << adapter.front() << '/' << adapter.back() << '\n';

    const SmartPtrWrapper<std::unique_ptr<int>> wrapped(std::make_unique<int>(99));
    out << "wrapped unique_ptr  = " << *wrapped << '\n';
}

} // namespace CppVerseHub::Templates::Concepts

#endif // CPPVERSEHUB_TEMPLATES_CONCEPTS_DEMO_HPP
