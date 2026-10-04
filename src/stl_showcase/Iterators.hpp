/**
 * @file Iterators.hpp
 * @brief Iterator showcase: the C++20 iterator concept hierarchy, standard iterator adapters,
 *        and three hand-written iterators that model the standard concepts precisely.
 *
 *  - SimpleVector<T>::iterator models std::contiguous_iterator (pointer-like, the strongest level).
 *  - FilterView<V, Pred>::iterator models std::forward_iterator; FilterView is a forward_range
 *    usable with std::ranges algorithms and range adaptors.
 *  - FibonacciRange's iterator is a C++20 std::forward_iterator whose operator* returns a prvalue:
 *    legal for the C++20 concept but not for the legacy Cpp17ForwardIterator requirements, so it
 *    advertises iterator_category = input_iterator_tag and iterator_concept = forward_iterator_tag.
 *
 * Every claim is verified by static_assert at the bottom of the file, so a regression is a
 * compile error rather than a silent pessimisation.
 */
#pragma once

#include <algorithm>
#include <compare>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <initializer_list>
#include <iostream>
#include <iterator>
#include <list>
#include <memory>
#include <ranges>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

namespace CppVerseHub::STL {

/**
 * @brief A star system record used by the iterator demonstrations.
 */
struct StarSystem {
    std::string name;       ///< System name.
    std::string star_type;  ///< Spectral class.
    int planet_count{0};    ///< Known planets.
    double distance_ly{0.0};///< Distance from Sol in light years.

    /// @brief Member-wise equality.
    friend bool operator==(const StarSystem&, const StarSystem&) = default;
};

/**
 * @brief Deterministic sample star systems sorted by distance.
 * @return Five systems.
 */
[[nodiscard]] std::vector<StarSystem> sampleStarSystems();

// ------------------------------------------------------------------- concept-based utilities

/**
 * @brief Name of the strongest C++20 iterator concept modelled by @p It.
 * @tparam It Iterator type.
 * @return "contiguous", "random_access", "bidirectional", "forward", "input", "output" or "none".
 */
template <typename It>
[[nodiscard]] constexpr std::string_view iteratorCategoryName() noexcept {
    if constexpr (std::contiguous_iterator<It>) {
        return "contiguous";
    } else if constexpr (std::random_access_iterator<It>) {
        return "random_access";
    } else if constexpr (std::bidirectional_iterator<It>) {
        return "bidirectional";
    } else if constexpr (std::forward_iterator<It>) {
        return "forward";
    } else if constexpr (std::input_iterator<It>) {
        return "input";
    } else if constexpr (std::input_or_output_iterator<It>) {
        return "output";
    } else {
        return "none";
    }
}

/**
 * @brief Concept-dispatched advance: O(1) for random access, O(|n|) otherwise.
 * @tparam It Iterator type.
 * @param it Iterator to move.
 * @param n Steps (negative only allowed for bidirectional iterators).
 * @throws std::invalid_argument if n < 0 for a non-bidirectional iterator.
 */
template <std::input_or_output_iterator It>
constexpr void advanceBy(It& it, std::iter_difference_t<It> n) {
    if constexpr (std::random_access_iterator<It>) {
        it += n;
    } else {
        if constexpr (std::bidirectional_iterator<It>) {
            for (; n < 0; ++n) {
                --it;
            }
        } else if (n < 0) {
            throw std::invalid_argument("advanceBy: negative step on a non-bidirectional iterator");
        }
        for (; n > 0; --n) {
            ++it;
        }
    }
}

/**
 * @brief Concept-dispatched distance: O(1) for sized sentinels, O(n) otherwise.
 * @tparam It Iterator type.
 * @tparam S Sentinel type.
 * @param first Start.
 * @param last End (must be reachable from first).
 * @return Number of increments from first to last.
 */
template <std::input_or_output_iterator It, std::sentinel_for<It> S>
[[nodiscard]] constexpr std::iter_difference_t<It> distanceBetween(It first, S last) {
    if constexpr (std::sized_sentinel_for<S, It>) {
        return last - first;
    } else {
        std::iter_difference_t<It> n = 0;
        for (; first != last; ++first) {
            ++n;
        }
        return n;
    }
}

// ------------------------------------------------------------------------------ SimpleVector

/**
 * @brief Minimal growable array whose iterator models std::contiguous_iterator.
 *
 * Storage is a std::unique_ptr<T[]>, so T must be default-initialisable; elements beyond size()
 * are value-initialised placeholders. Growth doubles capacity and moves elements with
 * std::move_if_noexcept, giving the strong exception guarantee for nothrow-movable T.
 *
 * @tparam T Element type.
 */
template <typename T>
    requires std::default_initializable<T> && std::movable<T>
class SimpleVector {
    template <bool Const>
    class Iterator;

public:
    using value_type = T;                    ///< Element type.
    using size_type = std::size_t;           ///< Size type.
    using difference_type = std::ptrdiff_t;  ///< Difference type.
    using reference = T&;                    ///< Reference type.
    using const_reference = const T&;        ///< Const reference type.
    using iterator = Iterator<false>;        ///< Mutable contiguous iterator.
    using const_iterator = Iterator<true>;   ///< Const contiguous iterator.

    SimpleVector() = default;

    /**
     * @brief Construct @p count copies of @p value.
     * @param count Number of elements.
     * @param value Element value.
     */
    explicit SimpleVector(size_type count, const T& value = T{}) : SimpleVector() {
        reserve(count);
        for (size_type i = 0; i < count; ++i) {
            data_[i] = value;
        }
        size_ = count;
    }

    /**
     * @brief Construct from an initializer list.
     * @param init Elements.
     */
    SimpleVector(std::initializer_list<T> init) : SimpleVector() {
        reserve(init.size());
        std::ranges::copy(init, data_.get());
        size_ = init.size();
    }

    /// @brief Deep copy. @param other Source.
    SimpleVector(const SimpleVector& other) : SimpleVector() {
        reserve(other.size_);
        std::copy_n(other.data_.get(), other.size_, data_.get());
        size_ = other.size_;
    }

    /// @brief Move constructor; leaves @p other empty. @param other Source.
    SimpleVector(SimpleVector&& other) noexcept
        : data_(std::move(other.data_)),
          size_(std::exchange(other.size_, 0)),
          capacity_(std::exchange(other.capacity_, 0)) {}

    /// @brief Copy-and-swap assignment. @param other Source. @return *this.
    SimpleVector& operator=(const SimpleVector& other) {
        if (this != &other) {
            SimpleVector copy(other);
            swap(copy);
        }
        return *this;
    }

    /// @brief Move assignment. @param other Source. @return *this.
    SimpleVector& operator=(SimpleVector&& other) noexcept {
        SimpleVector moved(std::move(other));
        swap(moved);
        return *this;
    }

    ~SimpleVector() = default;

    /**
     * @brief Swap contents.
     * @param other Vector to swap with.
     */
    void swap(SimpleVector& other) noexcept {
        using std::swap;
        swap(data_, other.data_);
        swap(size_, other.size_);
        swap(capacity_, other.capacity_);
    }

    /**
     * @brief Ensure capacity for at least @p new_capacity elements.
     * @param new_capacity Requested capacity.
     */
    void reserve(size_type new_capacity) {
        if (new_capacity <= capacity_) {
            return;
        }
        auto fresh = std::make_unique<T[]>(new_capacity);
        for (size_type i = 0; i < size_; ++i) {
            fresh[i] = std::move_if_noexcept(data_[i]);
        }
        data_ = std::move(fresh);
        capacity_ = new_capacity;
    }

    /**
     * @brief Append a copy (safe even if @p value aliases an element).
     * @param value Value to append.
     */
    void push_back(const T& value) { emplace_back(value); }

    /**
     * @brief Append by move.
     * @param value Value to append.
     */
    void push_back(T&& value) { emplace_back(std::move(value)); }

    /**
     * @brief Construct an element at the end.
     * @tparam Args Constructor argument types.
     * @param args Constructor arguments.
     * @return Reference to the new element.
     */
    template <typename... Args>
    T& emplace_back(Args&&... args) {
        T element(std::forward<Args>(args)...);  // built before growth so aliasing args stay valid
        if (size_ == capacity_) {
            reserve(capacity_ == 0 ? 4 : capacity_ * 2);
        }
        data_[size_] = std::move(element);
        return data_[size_++];
    }

    /**
     * @brief Remove the last element (resets the slot so resources are released).
     * @throws std::out_of_range if empty.
     */
    void pop_back() {
        if (size_ == 0) {
            throw std::out_of_range("SimpleVector::pop_back on empty vector");
        }
        data_[--size_] = T{};
    }

    /// @brief Remove all elements (capacity is kept).
    void clear() noexcept(std::is_nothrow_default_constructible_v<T> && std::is_nothrow_move_assignable_v<T>) {
        while (size_ > 0) {
            data_[--size_] = T{};
        }
    }

    /// @brief Unchecked access. @param i Index (< size()). @return Element reference.
    [[nodiscard]] T& operator[](size_type i) noexcept { return data_[i]; }
    /// @brief Unchecked access. @param i Index (< size()). @return Element reference.
    [[nodiscard]] const T& operator[](size_type i) const noexcept { return data_[i]; }

    /**
     * @brief Checked access.
     * @param i Index.
     * @return Element reference.
     * @throws std::out_of_range if i >= size().
     */
    [[nodiscard]] T& at(size_type i) {
        if (i >= size_) {
            throw std::out_of_range("SimpleVector::at index out of range");
        }
        return data_[i];
    }

    /// @brief Checked access. @param i Index. @return Element reference. @throws std::out_of_range.
    [[nodiscard]] const T& at(size_type i) const {
        if (i >= size_) {
            throw std::out_of_range("SimpleVector::at index out of range");
        }
        return data_[i];
    }

    /// @brief Pointer to the first element. @return Data pointer (may be null when empty).
    [[nodiscard]] T* data() noexcept { return data_.get(); }
    /// @brief Pointer to the first element. @return Data pointer (may be null when empty).
    [[nodiscard]] const T* data() const noexcept { return data_.get(); }
    /// @brief Element count. @return Size.
    [[nodiscard]] size_type size() const noexcept { return size_; }
    /// @brief Allocated slots. @return Capacity.
    [[nodiscard]] size_type capacity() const noexcept { return capacity_; }
    /// @brief Whether empty. @return true when size() == 0.
    [[nodiscard]] bool empty() const noexcept { return size_ == 0; }

    /// @brief Begin iterator. @return Iterator to the first element.
    [[nodiscard]] iterator begin() noexcept { return iterator{data_.get()}; }
    /// @brief End iterator. @return Iterator past the last element.
    [[nodiscard]] iterator end() noexcept { return iterator{data_.get() + size_}; }
    /// @brief Const begin. @return Const iterator to the first element.
    [[nodiscard]] const_iterator begin() const noexcept { return const_iterator{data_.get()}; }
    /// @brief Const end. @return Const iterator past the last element.
    [[nodiscard]] const_iterator end() const noexcept { return const_iterator{data_.get() + size_}; }
    /// @brief Const begin. @return Const iterator to the first element.
    [[nodiscard]] const_iterator cbegin() const noexcept { return begin(); }
    /// @brief Const end. @return Const iterator past the last element.
    [[nodiscard]] const_iterator cend() const noexcept { return end(); }

    /**
     * @brief Element-wise equality.
     * @param lhs First vector.
     * @param rhs Second vector.
     * @return true if sizes and elements match.
     */
    friend bool operator==(const SimpleVector& lhs, const SimpleVector& rhs)
        requires std::equality_comparable<T>
    {
        return std::ranges::equal(lhs, rhs);
    }

private:
    /**
     * @brief Pointer-wrapping iterator; Const selects the const_iterator flavour.
     */
    template <bool Const>
    class Iterator {
    public:
        using iterator_concept = std::contiguous_iterator_tag;
        using iterator_category = std::random_access_iterator_tag;
        using value_type = T;
        using difference_type = std::ptrdiff_t;
        using element_type = std::conditional_t<Const, const T, T>;
        using pointer = element_type*;
        using reference = element_type&;

        Iterator() = default;
        explicit Iterator(pointer ptr) noexcept : ptr_(ptr) {}

        /// Implicit conversion iterator -> const_iterator.
        template <bool OtherConst>
            requires(Const && !OtherConst)
        Iterator(const Iterator<OtherConst>& other) noexcept : ptr_(other.ptr_) {}  // NOLINT(google-explicit-constructor)

        [[nodiscard]] reference operator*() const noexcept { return *ptr_; }
        [[nodiscard]] pointer operator->() const noexcept { return ptr_; }
        [[nodiscard]] reference operator[](difference_type n) const noexcept { return ptr_[n]; }

        Iterator& operator++() noexcept {
            ++ptr_;
            return *this;
        }
        Iterator operator++(int) noexcept {
            Iterator copy = *this;
            ++ptr_;
            return copy;
        }
        Iterator& operator--() noexcept {
            --ptr_;
            return *this;
        }
        Iterator operator--(int) noexcept {
            Iterator copy = *this;
            --ptr_;
            return copy;
        }
        Iterator& operator+=(difference_type n) noexcept {
            ptr_ += n;
            return *this;
        }
        Iterator& operator-=(difference_type n) noexcept {
            ptr_ -= n;
            return *this;
        }

        [[nodiscard]] friend Iterator operator+(Iterator it, difference_type n) noexcept { return it += n; }
        [[nodiscard]] friend Iterator operator+(difference_type n, Iterator it) noexcept { return it += n; }
        [[nodiscard]] friend Iterator operator-(Iterator it, difference_type n) noexcept { return it -= n; }
        [[nodiscard]] friend difference_type operator-(const Iterator& lhs, const Iterator& rhs) noexcept {
            return lhs.ptr_ - rhs.ptr_;
        }
        [[nodiscard]] friend bool operator==(const Iterator&, const Iterator&) = default;
        [[nodiscard]] friend auto operator<=>(const Iterator&, const Iterator&) = default;

    private:
        template <bool>
        friend class Iterator;
        pointer ptr_ = nullptr;
    };

    std::unique_ptr<T[]> data_;
    size_type size_{0};
    size_type capacity_{0};
};

// -------------------------------------------------------------------------------- FilterView

/**
 * @brief Lazy filtering view whose iterator models std::forward_iterator.
 *
 * Like std::ranges::filter_view, the iterator stores the underlying iterator plus a pointer to
 * its parent view (where the predicate lives), so iterators stay cheap to copy and the predicate
 * is never duplicated. The underlying range must be a common forward range. As with the standard
 * filter_view, iteration requires a non-const view; begin() is O(n) and deliberately not cached so
 * that copying the view can never leave a stale cached iterator behind.
 *
 * @tparam V Underlying view (typically std::ranges::ref_view or owning_view).
 * @tparam Pred Predicate over the underlying elements.
 */
template <std::ranges::view V, typename Pred>
    requires std::ranges::forward_range<V> && std::ranges::common_range<V> && std::is_object_v<Pred> &&
             std::indirect_unary_predicate<const Pred, std::ranges::iterator_t<V>>
class FilterView : public std::ranges::view_interface<FilterView<V, Pred>> {
public:
    /**
     * @brief Forward iterator over the elements that satisfy the predicate.
     */
    class iterator {
    public:
        using iterator_concept = std::forward_iterator_tag;
        using iterator_category = std::forward_iterator_tag;
        using value_type = std::ranges::range_value_t<V>;
        using difference_type = std::ranges::range_difference_t<V>;
        using reference = std::ranges::range_reference_t<V>;

        iterator() = default;

        /**
         * @brief Construct from a parent and a position that already satisfies the predicate.
         * @param parent Owning view.
         * @param current Underlying position.
         */
        iterator(FilterView* parent, std::ranges::iterator_t<V> current)
            : current_(std::move(current)), parent_(parent) {}

        /// @brief Dereference. @return Underlying reference.
        [[nodiscard]] reference operator*() const { return *current_; }

        /// @brief Advance to the next matching element. @return *this.
        iterator& operator++() {
            current_ = std::ranges::find_if(std::next(current_), std::ranges::end(parent_->base_),
                                            std::cref(parent_->pred_));
            return *this;
        }

        /// @brief Post-increment. @return Previous position.
        iterator operator++(int) {
            iterator copy = *this;
            ++*this;
            return copy;
        }

        /// @brief Underlying position. @return Iterator into the base range.
        [[nodiscard]] const std::ranges::iterator_t<V>& base() const& noexcept { return current_; }

        /**
         * @brief Position equality.
         * @param lhs First iterator.
         * @param rhs Second iterator.
         * @return true if both refer to the same underlying position.
         */
        [[nodiscard]] friend bool operator==(const iterator& lhs, const iterator& rhs) {
            return lhs.current_ == rhs.current_;
        }

    private:
        std::ranges::iterator_t<V> current_{};
        FilterView* parent_ = nullptr;
    };

    /**
     * @brief Construct the view.
     * @param base Underlying view.
     * @param pred Predicate.
     */
    FilterView(V base, Pred predicate) : base_(std::move(base)), pred_(std::move(predicate)) {}

    /// @brief First matching element. @return Iterator (O(n) search).
    [[nodiscard]] iterator begin() { return iterator{this, std::ranges::find_if(base_, std::cref(pred_))}; }
    /// @brief Past-the-end iterator. @return Iterator at the underlying end.
    [[nodiscard]] iterator end() { return iterator{this, std::ranges::end(base_)}; }
    /// @brief Underlying view. @return Copy of the base view.
    [[nodiscard]] V base() const
        requires std::copy_constructible<V>
    {
        return base_;
    }
    /// @brief Predicate. @return Reference to the stored predicate.
    [[nodiscard]] const Pred& pred() const noexcept { return pred_; }

private:
    V base_;
    Pred pred_;
};

/**
 * @brief Deduction guide: FilterView(vec, pred) wraps vec in std::views::all.
 */
template <typename R, typename Pred>
FilterView(R&&, Pred) -> FilterView<std::views::all_t<R>, Pred>;

// ---------------------------------------------------------------------------- FibonacciRange

/**
 * @brief Lazily generated, bounded Fibonacci sequence terminated by std::default_sentinel.
 */
class FibonacciRange {
public:
    /// @brief Largest count whose terms all fit in std::uint64_t (fib(0) .. fib(93)).
    static constexpr std::size_t max_count = 94;

    /**
     * @brief Generator iterator: state is (current, next, index). A C++20 forward iterator with
     *        prvalue dereference; equality compares positions.
     */
    class iterator {
    public:
        using iterator_concept = std::forward_iterator_tag;
        using iterator_category = std::input_iterator_tag;  // prvalue reference: legacy input only
        using value_type = std::uint64_t;
        using difference_type = std::ptrdiff_t;

        iterator() = default;

        /**
         * @brief Construct a generator positioned at index 0.
         * @param limit Number of terms to produce.
         */
        constexpr explicit iterator(std::size_t limit) noexcept : limit_(limit) {}

        /// @brief Current term. @return fib(index).
        [[nodiscard]] constexpr value_type operator*() const noexcept { return current_; }

        /// @brief Advance one term. @return *this.
        constexpr iterator& operator++() noexcept {
            const value_type sum = current_ + next_;  // unsigned: wraps harmlessly past the last term
            current_ = next_;
            next_ = sum;
            ++index_;
            return *this;
        }

        /// @brief Post-increment. @return Previous position.
        constexpr iterator operator++(int) noexcept {
            iterator copy = *this;
            ++*this;
            return copy;
        }

        /// @brief Index of the current term. @return Index.
        [[nodiscard]] constexpr std::size_t index() const noexcept { return index_; }

        /// @brief Position equality. @param lhs First. @param rhs Second. @return Same index.
        [[nodiscard]] friend constexpr bool operator==(const iterator& lhs, const iterator& rhs) noexcept {
            return lhs.index_ == rhs.index_;
        }

        /// @brief End test. @param it Iterator. @return true once the limit is reached.
        [[nodiscard]] friend constexpr bool operator==(const iterator& it, std::default_sentinel_t) noexcept {
            return it.index_ >= it.limit_;
        }

    private:
        value_type current_{0};
        value_type next_{1};
        std::size_t index_{0};
        std::size_t limit_{0};
    };

    /**
     * @brief Construct a range of the first @p count Fibonacci numbers.
     * @param count Number of terms (<= max_count).
     * @throws std::out_of_range if count > max_count.
     */
    constexpr explicit FibonacciRange(std::size_t count) : count_(count) {
        if (count_ > max_count) {
            throw std::out_of_range("FibonacciRange: terms beyond fib(93) overflow 64 bits");
        }
    }

    /// @brief First term. @return Iterator at fib(0).
    [[nodiscard]] constexpr iterator begin() const noexcept { return iterator{count_}; }
    /// @brief Sentinel. @return std::default_sentinel.
    [[nodiscard]] constexpr std::default_sentinel_t end() const noexcept { return std::default_sentinel; }
    /// @brief Number of terms. @return Count.
    [[nodiscard]] constexpr std::size_t size() const noexcept { return count_; }

private:
    std::size_t count_;
};

// ----------------------------------------------------------------- iterator adapter helpers

/**
 * @brief Parse whitespace-separated integers with std::istream_iterator; stops at the first
 *        non-integer token.
 * @param text Input text.
 * @return Parsed integers.
 */
[[nodiscard]] std::vector<int> parseInts(std::string_view text);

/**
 * @brief Join integers with a delimiter using std::ostream_iterator (no trailing delimiter).
 * @param values Values.
 * @param delimiter Separator.
 * @return Joined text.
 */
[[nodiscard]] std::string joinInts(std::span<const int> values, std::string_view delimiter);

/**
 * @brief Reverse copy through std::reverse_iterator (rbegin/rend).
 * @param values Values.
 * @return Values in reverse order.
 */
[[nodiscard]] std::vector<int> reversedCopy(std::span<const int> values);

/**
 * @brief Move every element out of @p source with std::make_move_iterator.
 * @tparam T Element type (may be move-only, e.g. std::unique_ptr).
 * @param source Vector whose elements are moved from; it is cleared afterwards.
 * @return Vector owning the moved elements.
 */
template <typename T>
[[nodiscard]] std::vector<T> moveAll(std::vector<T>& source) {
    std::vector<T> destination(std::make_move_iterator(source.begin()), std::make_move_iterator(source.end()));
    source.clear();
    return destination;
}

/**
 * @brief Erase matching elements with the classic `it = c.erase(it)` loop, which is correct for
 *        every standard sequence and associative container despite iterator invalidation.
 * @tparam Container Container with erase(iterator) returning the next iterator.
 * @tparam Pred Predicate over elements.
 * @param container Container to modify.
 * @param pred Predicate selecting elements to erase.
 * @return Number of erased elements.
 */
template <typename Container, typename Pred>
std::size_t eraseWhileIterating(Container& container, Pred pred) {
    std::size_t erased = 0;
    for (auto it = container.begin(); it != container.end();) {
        if (std::invoke(pred, *it)) {
            it = container.erase(it);
            ++erased;
        } else {
            ++it;
        }
    }
    return erased;
}

/**
 * @brief Interleave two lists with std::front_inserter / std::inserter / std::back_inserter.
 * @param front Values pushed to the front (ending up reversed).
 * @param middle Values inserted after the front block.
 * @param back Values appended.
 * @return Resulting list contents.
 */
[[nodiscard]] std::list<int> buildWithInserters(std::span<const int> front, std::span<const int> middle,
                                                std::span<const int> back);

// ------------------------------------------------------------------------------ demonstrations

/// @brief Narrate the iterator concept hierarchy on standard containers. @param out Destination stream.
void demonstrateIteratorCategories(std::ostream& out = std::cout);
/// @brief Narrate reverse/insert/move/stream iterators. @param out Destination stream.
void demonstrateIteratorAdapters(std::ostream& out = std::cout);
/// @brief Narrate SimpleVector, FilterView and FibonacciRange with std::ranges. @param out Destination stream.
void demonstrateCustomIterators(std::ostream& out = std::cout);
/// @brief Narrate advance/distance/next/prev and invalidation-safe erasure. @param out Destination stream.
void demonstrateIteratorUtilities(std::ostream& out = std::cout);
/// @brief Run every iterator demonstration. @param out Destination stream.
void runIteratorsDemo(std::ostream& out = std::cout);

// ------------------------------------------------------------------- compile-time guarantees

static_assert(std::contiguous_iterator<SimpleVector<int>::iterator>);
static_assert(std::contiguous_iterator<SimpleVector<int>::const_iterator>);
static_assert(std::ranges::contiguous_range<SimpleVector<int>>);
static_assert(std::ranges::sized_range<SimpleVector<int>>);
static_assert(std::convertible_to<SimpleVector<int>::iterator, SimpleVector<int>::const_iterator>);
static_assert(!std::convertible_to<SimpleVector<int>::const_iterator, SimpleVector<int>::iterator>);

using IntFilterView = FilterView<std::ranges::ref_view<std::vector<int>>, bool (*)(int)>;
static_assert(std::forward_iterator<IntFilterView::iterator>);
static_assert(!std::bidirectional_iterator<IntFilterView::iterator>);
static_assert(std::ranges::forward_range<IntFilterView>);
static_assert(std::ranges::view<IntFilterView>);

static_assert(std::forward_iterator<FibonacciRange::iterator>);
static_assert(std::sentinel_for<std::default_sentinel_t, FibonacciRange::iterator>);
static_assert(std::ranges::forward_range<FibonacciRange>);
static_assert(std::same_as<std::iterator_traits<FibonacciRange::iterator>::iterator_category,
                           std::input_iterator_tag>);

}  // namespace CppVerseHub::STL

/**
 * @brief FibonacciRange iterators carry all their state, so they may outlive the range object:
 *        opting into borrowed_range lets algorithms return real iterators for rvalue ranges
 *        instead of std::ranges::dangling.
 */
namespace std::ranges {
template <>
inline constexpr bool enable_borrowed_range<CppVerseHub::STL::FibonacciRange> = true;
}  // namespace std::ranges

static_assert(std::ranges::borrowed_range<CppVerseHub::STL::FibonacciRange>);
