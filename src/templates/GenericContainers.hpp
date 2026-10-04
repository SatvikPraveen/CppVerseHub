/**
 * @file GenericContainers.hpp
 * @brief Hand-written generic containers and smart pointers built on class templates.
 *
 * Demonstrates how the standard library's vocabulary types are put together:
 *  - `DynamicArray<T, Allocator>`: an allocator-aware, exception-safe growable array with a custom
 *    contiguous iterator template (`ArrayIterator<IsConst>`), allocator propagation traits,
 *    `operator<=>` and strong/basic exception guarantees documented per operation;
 *  - `UniquePtr<T, Deleter>`: exclusive ownership with `[[no_unique_address]]` empty-deleter
 *    optimisation and an array partial specialisation;
 *  - `SharedPtr<T>` / `WeakPtr<T>`: atomically reference-counted shared ownership with a
 *    type-erased control block, single-allocation `make_shared_ptr`, and the aliasing constructor;
 *  - `Optional<T>`: a constexpr-friendly optional implemented with a union (no reinterpret_cast).
 *
 * These are teaching implementations: in production prefer std::vector, std::unique_ptr,
 * std::shared_ptr and std::optional.
 */

#ifndef CPPVERSEHUB_TEMPLATES_GENERIC_CONTAINERS_HPP
#define CPPVERSEHUB_TEMPLATES_GENERIC_CONTAINERS_HPP

/// Portable `[[no_unique_address]]`: MSVC ignores the standard spelling and needs its own attribute.
#ifndef CPPVERSEHUB_NO_UNIQUE_ADDRESS
#if defined(_MSC_VER) && !defined(__clang__)
#define CPPVERSEHUB_NO_UNIQUE_ADDRESS [[msvc::no_unique_address]]
#else
#define CPPVERSEHUB_NO_UNIQUE_ADDRESS [[no_unique_address]]
#endif
#endif

#include <algorithm>
#include <atomic>
#include <cassert>
#include <compare>
#include <concepts>
#include <cstddef>
#include <functional>
#include <initializer_list>
#include <iostream>
#include <iterator>
#include <limits>
#include <memory>
#include <new>
#include <optional>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>

namespace CppVerseHub::Templates {

// =====================================================================================================
// DynamicArray
// =====================================================================================================

/**
 * @brief Allocator-aware growable array (a simplified std::vector).
 *
 * Exception guarantees: push_back/emplace_back/reserve/copy-assignment give the strong guarantee
 * when T's move constructor is noexcept (or T is copyable); insert/erase give the basic guarantee.
 *
 * @tparam T element type
 * @tparam Allocator allocator whose pointer type is T*
 */
template <typename T, typename Allocator = std::allocator<T>>
class DynamicArray {
    using alloc_traits = std::allocator_traits<Allocator>;
    static_assert(std::is_same_v<typename alloc_traits::value_type, T>, "Allocator::value_type must be T");
    static_assert(std::is_same_v<typename alloc_traits::pointer, T*>, "Allocator must use raw pointers");

public:
    /**
     * @brief Contiguous iterator over DynamicArray; IsConst selects the const flavour.
     * @tparam IsConst true for const_iterator
     */
    template <bool IsConst>
    class ArrayIterator {
    public:
        using iterator_concept = std::contiguous_iterator_tag;
        using iterator_category = std::random_access_iterator_tag;
        using value_type = std::remove_cv_t<T>;
        using element_type = std::conditional_t<IsConst, const T, T>;
        using difference_type = std::ptrdiff_t;
        using pointer = element_type*;
        using reference = element_type&;

        /** @brief Singular (null) iterator. */
        constexpr ArrayIterator() noexcept = default;
        /** @brief Wrap a raw pointer. @param ptr element pointer */
        constexpr explicit ArrayIterator(pointer ptr) noexcept : ptr_(ptr) {}
        /** @brief Implicit conversion iterator -> const_iterator. @param other mutable iterator */
        template <bool OtherConst>
            requires(IsConst && !OtherConst)
        // NOLINTNEXTLINE(google-explicit-constructor): iterator -> const_iterator must be implicit
        constexpr ArrayIterator(const ArrayIterator<OtherConst>& other) noexcept : ptr_(other.operator->()) {}

        /** @return referenced element */
        [[nodiscard]] constexpr reference operator*() const noexcept { return *ptr_; }
        /** @return pointer to element */
        [[nodiscard]] constexpr pointer operator->() const noexcept { return ptr_; }
        /** @param n offset @return element at offset n */
        [[nodiscard]] constexpr reference operator[](difference_type n) const noexcept { return ptr_[n]; }

        /** @return *this after increment */
        constexpr ArrayIterator& operator++() noexcept {
            ++ptr_;
            return *this;
        }
        /** @return copy before increment */
        constexpr ArrayIterator operator++(int) noexcept {
            ArrayIterator tmp = *this;
            ++ptr_;
            return tmp;
        }
        /** @return *this after decrement */
        constexpr ArrayIterator& operator--() noexcept {
            --ptr_;
            return *this;
        }
        /** @return copy before decrement */
        constexpr ArrayIterator operator--(int) noexcept {
            ArrayIterator tmp = *this;
            --ptr_;
            return tmp;
        }
        /** @param n offset @return *this advanced by n */
        constexpr ArrayIterator& operator+=(difference_type n) noexcept {
            ptr_ += n;
            return *this;
        }
        /** @param n offset @return *this moved back by n */
        constexpr ArrayIterator& operator-=(difference_type n) noexcept {
            ptr_ -= n;
            return *this;
        }

        /** @param it iterator @param n offset @return it + n */
        [[nodiscard]] friend constexpr ArrayIterator operator+(ArrayIterator it, difference_type n) noexcept {
            return it += n;
        }
        /** @param n offset @param it iterator @return it + n */
        [[nodiscard]] friend constexpr ArrayIterator operator+(difference_type n, ArrayIterator it) noexcept {
            return it += n;
        }
        /** @param it iterator @param n offset @return it - n */
        [[nodiscard]] friend constexpr ArrayIterator operator-(ArrayIterator it, difference_type n) noexcept {
            return it -= n;
        }
        /** @param a lhs @param b rhs @return distance a - b */
        [[nodiscard]] friend constexpr difference_type operator-(const ArrayIterator& a,
                                                                 const ArrayIterator& b) noexcept {
            return a.ptr_ - b.ptr_;
        }
        /** @return true if both point to the same element */
        [[nodiscard]] constexpr bool operator==(const ArrayIterator&) const noexcept = default;
        /** @return ordering of the underlying pointers */
        [[nodiscard]] constexpr auto operator<=>(const ArrayIterator&) const noexcept = default;

    private:
        pointer ptr_ = nullptr;
    };

    using value_type = T;
    using allocator_type = Allocator;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;
    using reference = T&;
    using const_reference = const T&;
    using pointer = T*;
    using const_pointer = const T*;
    using iterator = ArrayIterator<false>;
    using const_iterator = ArrayIterator<true>;
    using reverse_iterator = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;

    // ----- construction / destruction -----

    /** @brief Empty array. */
    DynamicArray() noexcept(std::is_nothrow_default_constructible_v<Allocator>) = default;

    /** @brief Empty array using alloc. @param alloc allocator */
    explicit DynamicArray(const Allocator& alloc) noexcept : alloc_(alloc) {}

    /** @brief count value-initialised elements. @param count size @param alloc allocator */
    explicit DynamicArray(size_type count, const Allocator& alloc = Allocator()) : alloc_(alloc) {
        guarded([&] {
            reserve(count);
            for (size_type i = 0; i < count; ++i) {
                emplace_back_unchecked();
            }
        });
    }

    /** @brief count copies of value. @param count size @param value fill @param alloc allocator */
    DynamicArray(size_type count, const T& value, const Allocator& alloc = Allocator()) : alloc_(alloc) {
        guarded([&] {
            reserve(count);
            for (size_type i = 0; i < count; ++i) {
                emplace_back_unchecked(value);
            }
        });
    }

    /** @brief Copy [first, last). @param first begin @param last end @param alloc allocator */
    template <std::input_iterator InputIt, std::sentinel_for<InputIt> Sentinel>
    DynamicArray(InputIt first, Sentinel last, const Allocator& alloc = Allocator()) : alloc_(alloc) {
        guarded([&] {
            if constexpr (std::sized_sentinel_for<Sentinel, InputIt>) {
                reserve(static_cast<size_type>(last - first));
            }
            for (; first != last; ++first) {
                emplace_back(*first);
            }
        });
    }

    /** @brief From an initializer list. @param init elements @param alloc allocator */
    DynamicArray(std::initializer_list<T> init, const Allocator& alloc = Allocator())
        : DynamicArray(init.begin(), init.end(), alloc) {}

    /** @brief Deep copy (allocator via select_on_container_copy_construction). @param other source */
    DynamicArray(const DynamicArray& other)
        : alloc_(alloc_traits::select_on_container_copy_construction(other.alloc_)) {
        guarded([&] {
            reserve(other.size_);
            for (const auto& element : other) {
                emplace_back_unchecked(element);
            }
        });
    }

    /** @brief Steal storage. @param other source, left empty */
    DynamicArray(DynamicArray&& other) noexcept
        : data_(std::exchange(other.data_, nullptr))
        , size_(std::exchange(other.size_, 0))
        , capacity_(std::exchange(other.capacity_, 0))
        , alloc_(std::move(other.alloc_)) {}

    /** @brief Destroy elements and free storage. */
    ~DynamicArray() { release_storage(); }

    /**
     * @brief Copy assignment with the strong guarantee (copy then commit).
     * @param other source
     * @return *this
     */
    DynamicArray& operator=(const DynamicArray& other) {
        if (this == &other) {
            return *this;
        }
        constexpr bool propagate = alloc_traits::propagate_on_container_copy_assignment::value;
        DynamicArray tmp(other.begin(), other.end(), propagate ? other.alloc_ : alloc_);
        release_storage();
        if constexpr (propagate) {
            alloc_ = tmp.alloc_;
        }
        steal(tmp);
        return *this;
    }

    /**
     * @brief Move assignment honouring allocator propagation; falls back to element-wise moves when
     *        allocators differ and do not propagate.
     * @param other source
     * @return *this
     */
    DynamicArray& operator=(DynamicArray&& other) noexcept(
        alloc_traits::propagate_on_container_move_assignment::value || alloc_traits::is_always_equal::value) {
        if (this == &other) {
            return *this;
        }
        if constexpr (alloc_traits::propagate_on_container_move_assignment::value) {
            release_storage();
            alloc_ = std::move(other.alloc_);
            steal(other);
        } else {
            if (alloc_traits::is_always_equal::value || alloc_ == other.alloc_) {
                release_storage();
                steal(other);
            } else {
                DynamicArray tmp(alloc_);
                tmp.reserve(other.size_);
                for (auto& element : other) {
                    tmp.emplace_back_unchecked(std::move(element));
                }
                release_storage();
                steal(tmp);
                other.clear();
            }
        }
        return *this;
    }

    /** @brief Replace contents with init. @param init elements @return *this */
    DynamicArray& operator=(std::initializer_list<T> init) {
        DynamicArray tmp(init, alloc_);
        release_storage();
        steal(tmp);
        return *this;
    }

    // ----- element access -----

    /** @param pos index @return element @throws std::out_of_range */
    [[nodiscard]] reference at(size_type pos) {
        check_index(pos);
        return data_[pos];
    }
    /** @param pos index @return element @throws std::out_of_range */
    [[nodiscard]] const_reference at(size_type pos) const {
        check_index(pos);
        return data_[pos];
    }
    /** @param pos index (unchecked) @return element */
    [[nodiscard]] reference operator[](size_type pos) noexcept {
        assert(pos < size_);
        return data_[pos];
    }
    /** @param pos index (unchecked) @return element */
    [[nodiscard]] const_reference operator[](size_type pos) const noexcept {
        assert(pos < size_);
        return data_[pos];
    }
    /** @return first element (precondition: !empty()) */
    [[nodiscard]] reference front() noexcept { return (*this)[0]; }
    /** @return first element (precondition: !empty()) */
    [[nodiscard]] const_reference front() const noexcept { return (*this)[0]; }
    /** @return last element (precondition: !empty()) */
    [[nodiscard]] reference back() noexcept { return (*this)[size_ - 1]; }
    /** @return last element (precondition: !empty()) */
    [[nodiscard]] const_reference back() const noexcept { return (*this)[size_ - 1]; }
    /** @return pointer to contiguous storage (may be null when empty) */
    [[nodiscard]] T* data() noexcept { return data_; }
    /** @return pointer to contiguous storage (may be null when empty) */
    [[nodiscard]] const T* data() const noexcept { return data_; }

    // ----- iterators -----

    /** @return iterator to first element */
    [[nodiscard]] iterator begin() noexcept { return iterator(data_); }
    /** @return const iterator to first element */
    [[nodiscard]] const_iterator begin() const noexcept { return const_iterator(data_); }
    /** @return const iterator to first element */
    [[nodiscard]] const_iterator cbegin() const noexcept { return begin(); }
    /** @return iterator past last element */
    [[nodiscard]] iterator end() noexcept { return iterator(data_ + size_); }
    /** @return const iterator past last element */
    [[nodiscard]] const_iterator end() const noexcept { return const_iterator(data_ + size_); }
    /** @return const iterator past last element */
    [[nodiscard]] const_iterator cend() const noexcept { return end(); }
    /** @return reverse iterator to last element */
    [[nodiscard]] reverse_iterator rbegin() noexcept { return reverse_iterator(end()); }
    /** @return const reverse iterator to last element */
    [[nodiscard]] const_reverse_iterator rbegin() const noexcept { return const_reverse_iterator(end()); }
    /** @return reverse iterator before first element */
    [[nodiscard]] reverse_iterator rend() noexcept { return reverse_iterator(begin()); }
    /** @return const reverse iterator before first element */
    [[nodiscard]] const_reverse_iterator rend() const noexcept { return const_reverse_iterator(begin()); }

    // ----- capacity -----

    /** @return true if there are no elements */
    [[nodiscard]] bool empty() const noexcept { return size_ == 0; }
    /** @return number of elements */
    [[nodiscard]] size_type size() const noexcept { return size_; }
    /** @return number of elements storable without reallocation */
    [[nodiscard]] size_type capacity() const noexcept { return capacity_; }
    /** @return theoretical maximum size */
    [[nodiscard]] size_type max_size() const noexcept { return alloc_traits::max_size(alloc_); }

    /**
     * @brief Ensure capacity >= new_cap (strong guarantee).
     * @param new_cap requested capacity
     * @throws std::length_error if new_cap > max_size()
     */
    void reserve(size_type new_cap) {
        if (new_cap > max_size()) {
            throw std::length_error("DynamicArray::reserve: capacity exceeds max_size");
        }
        if (new_cap > capacity_) {
            reallocate(new_cap);
        }
    }

    /** @brief Release unused capacity. */
    void shrink_to_fit() {
        if (size_ < capacity_) {
            reallocate(size_);
        }
    }

    // ----- modifiers -----

    /** @brief Destroy all elements, keeping capacity. */
    void clear() noexcept {
        destroy_range(0, size_);
        size_ = 0;
    }

    /**
     * @brief Construct an element at the end (strong guarantee; safe if args alias an element).
     * @param args constructor arguments
     * @return reference to the new element
     */
    template <typename... Args>
    reference emplace_back(Args&&... args) {
        if (size_ == capacity_) {
            reallocate_insert(size_, std::forward<Args>(args)...);
        } else {
            emplace_back_unchecked(std::forward<Args>(args)...);
        }
        return back();
    }

    /** @brief Append a copy. @param value element */
    void push_back(const T& value) { emplace_back(value); }
    /** @brief Append by move. @param value element */
    void push_back(T&& value) { emplace_back(std::move(value)); }

    /** @brief Remove the last element (precondition: !empty()). */
    void pop_back() noexcept {
        assert(size_ > 0);
        --size_;
        alloc_traits::destroy(alloc_, data_ + size_);
    }

    /**
     * @brief Construct an element before pos.
     * @param pos insertion point
     * @param args constructor arguments
     * @return iterator to the new element
     */
    template <typename... Args>
    iterator emplace(const_iterator pos, Args&&... args) {
        const size_type offset = offset_of(pos);
        if (size_ == capacity_) {
            reallocate_insert(offset, std::forward<Args>(args)...);
        } else if (offset == size_) {
            emplace_back_unchecked(std::forward<Args>(args)...);
        } else {
            T value(std::forward<Args>(args)...); // materialise first: args may alias an element
            emplace_back_unchecked(std::move(data_[size_ - 1]));
            std::move_backward(data_ + offset, data_ + size_ - 2, data_ + size_ - 1);
            data_[offset] = std::move(value);
        }
        return begin() + static_cast<difference_type>(offset);
    }

    /** @param pos insertion point @param value element @return iterator to inserted element */
    iterator insert(const_iterator pos, const T& value) { return emplace(pos, value); }
    /** @param pos insertion point @param value element @return iterator to inserted element */
    iterator insert(const_iterator pos, T&& value) { return emplace(pos, std::move(value)); }

    /**
     * @brief Insert count copies of value before pos.
     * @param pos insertion point
     * @param count number of copies
     * @param value element to copy
     * @return iterator to the first inserted element
     */
    iterator insert(const_iterator pos, size_type count, const T& value) {
        const size_type offset = offset_of(pos);
        const T copy(value); // value may refer into *this
        append_then_rotate(offset, [&] {
            for (size_type i = 0; i < count; ++i) {
                emplace_back(copy);
            }
        });
        return begin() + static_cast<difference_type>(offset);
    }

    /**
     * @brief Insert [first, last) before pos (the range must not refer into *this).
     * @param pos insertion point
     * @param first range begin
     * @param last range end
     * @return iterator to the first inserted element
     */
    template <std::input_iterator InputIt>
    iterator insert(const_iterator pos, InputIt first, InputIt last) {
        const size_type offset = offset_of(pos);
        append_then_rotate(offset, [&] {
            for (; first != last; ++first) {
                emplace_back(*first);
            }
        });
        return begin() + static_cast<difference_type>(offset);
    }

    /** @param pos element to erase @return iterator following the erased element */
    iterator erase(const_iterator pos) { return erase(pos, pos + 1); }

    /**
     * @brief Erase [first, last).
     * @param first range begin
     * @param last range end
     * @return iterator following the last erased element
     */
    iterator erase(const_iterator first, const_iterator last) {
        const size_type from = offset_of(first);
        const size_type to = offset_of(last);
        assert(from <= to && to <= size_);
        if (from != to) {
            std::move(data_ + to, data_ + size_, data_ + from);
            const size_type new_size = size_ - (to - from);
            destroy_range(new_size, size_);
            size_ = new_size;
        }
        return begin() + static_cast<difference_type>(from);
    }

    /** @brief Resize, value-initialising new elements. @param count new size */
    void resize(size_type count) {
        if (count < size_) {
            destroy_range(count, size_);
            size_ = count;
            return;
        }
        reserve(count);
        while (size_ < count) {
            emplace_back_unchecked();
        }
    }

    /** @brief Resize, copying value into new elements. @param count new size @param value fill */
    void resize(size_type count, const T& value) {
        if (count < size_) {
            destroy_range(count, size_);
            size_ = count;
            return;
        }
        const T copy(value);
        reserve(count);
        while (size_ < count) {
            emplace_back_unchecked(copy);
        }
    }

    /**
     * @brief Swap contents. Allocators are swapped if they propagate on swap; otherwise they must
     *        compare equal (same precondition as the standard containers).
     * @param other array to swap with
     */
    void swap(DynamicArray& other) noexcept {
        if constexpr (alloc_traits::propagate_on_container_swap::value) {
            using std::swap;
            swap(alloc_, other.alloc_);
        } else {
            assert(alloc_ == other.alloc_);
        }
        std::swap(data_, other.data_);
        std::swap(size_, other.size_);
        std::swap(capacity_, other.capacity_);
    }

    /** @return copy of the allocator */
    [[nodiscard]] allocator_type get_allocator() const noexcept { return alloc_; }

    /** @return true if sizes and elements are equal */
    [[nodiscard]] friend bool operator==(const DynamicArray& lhs, const DynamicArray& rhs)
        requires std::equality_comparable<T>
    {
        return std::equal(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
    }

    /** @return lexicographic three-way comparison */
    [[nodiscard]] friend auto operator<=>(const DynamicArray& lhs, const DynamicArray& rhs)
        requires std::three_way_comparable<T>
    {
        return std::lexicographical_compare_three_way(lhs.begin(), lhs.end(), rhs.begin(), rhs.end());
    }

    /** @brief ADL swap. @param lhs first @param rhs second */
    friend void swap(DynamicArray& lhs, DynamicArray& rhs) noexcept { lhs.swap(rhs); }

private:
    template <typename Fn>
    void guarded(Fn&& fn) {
        try {
            fn();
        } catch (...) {
            release_storage();
            throw;
        }
    }

    void check_index(size_type pos) const {
        if (pos >= size_) {
            throw std::out_of_range("DynamicArray::at: index " + std::to_string(pos) + " out of range");
        }
    }

    [[nodiscard]] size_type offset_of(const_iterator pos) const noexcept {
        return static_cast<size_type>(pos - cbegin());
    }

    [[nodiscard]] size_type next_capacity(size_type minimum) const {
        if (minimum > max_size()) {
            throw std::length_error("DynamicArray: maximum size exceeded");
        }
        const size_type doubled = capacity_ > max_size() / 2 ? max_size() : capacity_ * 2;
        return std::max<size_type>({minimum, doubled, 1});
    }

    template <typename... Args>
    void emplace_back_unchecked(Args&&... args) {
        assert(size_ < capacity_);
        alloc_traits::construct(alloc_, data_ + size_, std::forward<Args>(args)...);
        ++size_;
    }

    void destroy_range(size_type from, size_type to) noexcept {
        for (size_type i = from; i < to; ++i) {
            alloc_traits::destroy(alloc_, data_ + i);
        }
    }

    void release_storage() noexcept {
        if (data_ != nullptr) {
            destroy_range(0, size_);
            alloc_traits::deallocate(alloc_, data_, capacity_);
        }
        data_ = nullptr;
        size_ = 0;
        capacity_ = 0;
    }

    void steal(DynamicArray& other) noexcept {
        data_ = std::exchange(other.data_, nullptr);
        size_ = std::exchange(other.size_, 0);
        capacity_ = std::exchange(other.capacity_, 0);
    }

    // Move (or copy, if the move may throw and T is copyable) the elements into a new buffer.
    void reallocate(size_type new_cap) {
        T* new_data = new_cap == 0 ? nullptr : alloc_traits::allocate(alloc_, new_cap);
        size_type built = 0;
        try {
            for (; built < size_; ++built) {
                alloc_traits::construct(alloc_, new_data + built, std::move_if_noexcept(data_[built]));
            }
        } catch (...) {
            for (size_type i = 0; i < built; ++i) {
                alloc_traits::destroy(alloc_, new_data + i);
            }
            alloc_traits::deallocate(alloc_, new_data, new_cap);
            throw;
        }
        const size_type count = size_;
        release_storage();
        data_ = new_data;
        size_ = count;
        capacity_ = new_cap;
    }

    // Grow and construct a new element at offset in one step; the new element is built first so
    // that arguments referring to existing elements stay valid.
    template <typename... Args>
    void reallocate_insert(size_type offset, Args&&... args) {
        const size_type new_cap = next_capacity(size_ + 1);
        T* new_data = alloc_traits::allocate(alloc_, new_cap);
        try {
            alloc_traits::construct(alloc_, new_data + offset, std::forward<Args>(args)...);
        } catch (...) {
            alloc_traits::deallocate(alloc_, new_data, new_cap);
            throw;
        }
        size_type built = 0;
        try {
            for (; built < size_; ++built) {
                const size_type target = built < offset ? built : built + 1;
                alloc_traits::construct(alloc_, new_data + target, std::move_if_noexcept(data_[built]));
            }
        } catch (...) {
            for (size_type i = 0; i < built; ++i) {
                alloc_traits::destroy(alloc_, new_data + (i < offset ? i : i + 1));
            }
            alloc_traits::destroy(alloc_, new_data + offset);
            alloc_traits::deallocate(alloc_, new_data, new_cap);
            throw;
        }
        const size_type count = size_;
        release_storage();
        data_ = new_data;
        size_ = count + 1;
        capacity_ = new_cap;
    }

    // Append via fn, then rotate the appended block into place; rolls back appends on failure.
    template <typename Fn>
    void append_then_rotate(size_type offset, Fn&& fn) {
        const size_type old_size = size_;
        try {
            fn();
        } catch (...) {
            destroy_range(old_size, size_);
            size_ = old_size;
            throw;
        }
        std::rotate(data_ + offset, data_ + old_size, data_ + size_);
    }

    T* data_ = nullptr;
    size_type size_ = 0;
    size_type capacity_ = 0;
    CPPVERSEHUB_NO_UNIQUE_ADDRESS Allocator alloc_{};
};

// =====================================================================================================
// UniquePtr
// =====================================================================================================

/**
 * @brief Exclusive-ownership smart pointer (simplified std::unique_ptr).
 * @tparam T managed object type
 * @tparam Deleter callable invoked with T* on destruction
 */
template <typename T, typename Deleter = std::default_delete<T>>
class UniquePtr {
public:
    using element_type = T;
    using deleter_type = Deleter;
    using pointer = T*;

    /** @brief Null pointer. */
    constexpr UniquePtr() noexcept = default;
    /** @brief Null pointer. */
    constexpr UniquePtr(std::nullptr_t) noexcept {} // NOLINT(google-explicit-constructor)
    /** @brief Adopt p. @param p owned pointer */
    constexpr explicit UniquePtr(pointer p) noexcept : ptr_(p) {}
    /** @brief Adopt p with deleter d. @param p owned pointer @param d deleter */
    constexpr UniquePtr(pointer p, Deleter d) noexcept : ptr_(p), deleter_(std::move(d)) {}

    /** @brief Transfer ownership. @param other source, left null */
    constexpr UniquePtr(UniquePtr&& other) noexcept
        : ptr_(other.release()), deleter_(std::move(other.deleter_)) {}

    /** @brief Converting move (Derived -> Base). @param other source, left null */
    template <typename U, typename E>
        requires(std::convertible_to<U*, T*> && !std::is_array_v<U> && std::convertible_to<E, Deleter>)
    constexpr UniquePtr(UniquePtr<U, E>&& other) noexcept // NOLINT(google-explicit-constructor)
        : ptr_(other.release()), deleter_(std::move(other.get_deleter())) {}

    UniquePtr(const UniquePtr&) = delete;
    UniquePtr& operator=(const UniquePtr&) = delete;

    /** @brief Delete the owned object, if any. */
    constexpr ~UniquePtr() { reset(); }

    /** @brief Transfer ownership. @param other source @return *this */
    constexpr UniquePtr& operator=(UniquePtr&& other) noexcept {
        reset(other.release());
        deleter_ = std::move(other.deleter_);
        return *this;
    }

    /** @brief Converting move-assignment. @param other source @return *this */
    template <typename U, typename E>
        requires(std::convertible_to<U*, T*> && !std::is_array_v<U> && std::assignable_from<Deleter&, E &&>)
    constexpr UniquePtr& operator=(UniquePtr<U, E>&& other) noexcept {
        reset(other.release());
        deleter_ = std::move(other.get_deleter());
        return *this;
    }

    /** @brief Reset to null. @return *this */
    constexpr UniquePtr& operator=(std::nullptr_t) noexcept {
        reset();
        return *this;
    }

    /** @return owned object (precondition: non-null) */
    [[nodiscard]] constexpr T& operator*() const noexcept { return *ptr_; }
    /** @return owned pointer */
    [[nodiscard]] constexpr pointer operator->() const noexcept { return ptr_; }
    /** @return owned pointer */
    [[nodiscard]] constexpr pointer get() const noexcept { return ptr_; }
    /** @return the deleter */
    [[nodiscard]] constexpr Deleter& get_deleter() noexcept { return deleter_; }
    /** @return the deleter */
    [[nodiscard]] constexpr const Deleter& get_deleter() const noexcept { return deleter_; }
    /** @return true if non-null */
    [[nodiscard]] constexpr explicit operator bool() const noexcept { return ptr_ != nullptr; }

    /** @brief Give up ownership without deleting. @return previously owned pointer */
    [[nodiscard]] constexpr pointer release() noexcept { return std::exchange(ptr_, nullptr); }

    /** @brief Replace the owned pointer, deleting the old one. @param p new pointer */
    constexpr void reset(pointer p = nullptr) noexcept {
        pointer old = std::exchange(ptr_, p);
        if (old != nullptr) {
            deleter_(old);
        }
    }

    /** @brief Swap with other. @param other pointer to swap with */
    constexpr void swap(UniquePtr& other) noexcept {
        using std::swap;
        swap(ptr_, other.ptr_);
        swap(deleter_, other.deleter_);
    }

    /** @return true if both hold the same pointer */
    [[nodiscard]] friend constexpr bool operator==(const UniquePtr& a, const UniquePtr& b) noexcept {
        return a.ptr_ == b.ptr_;
    }
    /** @return true if null */
    [[nodiscard]] friend constexpr bool operator==(const UniquePtr& a, std::nullptr_t) noexcept {
        return a.ptr_ == nullptr;
    }

private:
    pointer ptr_ = nullptr;
    CPPVERSEHUB_NO_UNIQUE_ADDRESS Deleter deleter_{};
};

/**
 * @brief Partial specialisation for arrays of unknown bound: uses delete[] and offers operator[].
 * @tparam T element type
 * @tparam Deleter callable invoked with T* on destruction (default: std::default_delete<T[]>)
 */
template <typename T, typename Deleter>
class UniquePtr<T[], Deleter> {
public:
    using element_type = T;
    using deleter_type = Deleter;
    using pointer = T*;

    /** @brief Null pointer. */
    constexpr UniquePtr() noexcept = default;
    /** @brief Null pointer. */
    constexpr UniquePtr(std::nullptr_t) noexcept {} // NOLINT(google-explicit-constructor)
    /** @brief Adopt an array allocated with new[]. @param p owned array */
    constexpr explicit UniquePtr(pointer p) noexcept : ptr_(p) {}
    /** @brief Transfer ownership. @param other source */
    constexpr UniquePtr(UniquePtr&& other) noexcept
        : ptr_(other.release()), deleter_(std::move(other.deleter_)) {}
    UniquePtr(const UniquePtr&) = delete;
    UniquePtr& operator=(const UniquePtr&) = delete;
    /** @brief Delete the owned array. */
    constexpr ~UniquePtr() { reset(); }

    /** @brief Transfer ownership. @param other source @return *this */
    constexpr UniquePtr& operator=(UniquePtr&& other) noexcept {
        reset(other.release());
        deleter_ = std::move(other.deleter_);
        return *this;
    }

    /** @param i index @return element i (unchecked) */
    [[nodiscard]] constexpr T& operator[](std::size_t i) const noexcept { return ptr_[i]; }
    /** @return owned pointer */
    [[nodiscard]] constexpr pointer get() const noexcept { return ptr_; }
    /** @return true if non-null */
    [[nodiscard]] constexpr explicit operator bool() const noexcept { return ptr_ != nullptr; }
    /** @return previously owned pointer, ownership released */
    [[nodiscard]] constexpr pointer release() noexcept { return std::exchange(ptr_, nullptr); }
    /** @brief Replace the owned array. @param p new array */
    constexpr void reset(pointer p = nullptr) noexcept {
        pointer old = std::exchange(ptr_, p);
        if (old != nullptr) {
            deleter_(old);
        }
    }

private:
    pointer ptr_ = nullptr;
    CPPVERSEHUB_NO_UNIQUE_ADDRESS Deleter deleter_{};
};

/**
 * @brief Create a UniquePtr owning a new T.
 * @param args constructor arguments
 * @return owning pointer
 */
template <typename T, typename... Args>
    requires(!std::is_array_v<T>)
[[nodiscard]] UniquePtr<T> make_unique_ptr(Args&&... args) {
    return UniquePtr<T>(new T(std::forward<Args>(args)...));
}

/**
 * @brief Create a UniquePtr owning a value-initialised array of n elements.
 * @param n element count
 * @return owning pointer
 */
template <typename T>
    requires(std::is_unbounded_array_v<T>)
[[nodiscard]] UniquePtr<T> make_unique_ptr(std::size_t n) {
    return UniquePtr<T>(new std::remove_extent_t<T>[n]());
}

// =====================================================================================================
// SharedPtr / WeakPtr
// =====================================================================================================

namespace detail {

/**
 * @brief Type-erased control block: strong count, weak count (+1 while any strong ref exists),
 *        and virtual disposal of the managed object.
 */
class ControlBlockBase {
public:
    ControlBlockBase() noexcept = default;
    ControlBlockBase(const ControlBlockBase&) = delete;
    ControlBlockBase& operator=(const ControlBlockBase&) = delete;
    ControlBlockBase(ControlBlockBase&&) = delete;
    ControlBlockBase& operator=(ControlBlockBase&&) = delete;
    /** @brief Virtual destructor (blocks are deleted through the base). */
    virtual ~ControlBlockBase() = default;

    /** @brief Add a strong reference (caller already holds one). */
    void add_strong() noexcept { strong_.fetch_add(1, std::memory_order_relaxed); }

    /** @brief Add a strong reference only if the object is still alive. @return success */
    [[nodiscard]] bool try_add_strong() noexcept {
        std::size_t count = strong_.load(std::memory_order_relaxed);
        while (count != 0) {
            if (strong_.compare_exchange_weak(count, count + 1, std::memory_order_acq_rel,
                                              std::memory_order_relaxed)) {
                return true;
            }
        }
        return false;
    }

    /** @brief Drop a strong reference; disposes the object when it was the last. */
    void release_strong() noexcept {
        if (strong_.fetch_sub(1, std::memory_order_acq_rel) == 1) {
            dispose();
            release_weak();
        }
    }

    /** @brief Add a weak reference. */
    void add_weak() noexcept { weak_.fetch_add(1, std::memory_order_relaxed); }

    /** @brief Drop a weak reference; deletes the block when it was the last. */
    void release_weak() noexcept {
        if (weak_.fetch_sub(1, std::memory_order_acq_rel) == 1) {
            delete this;
        }
    }

    /** @return current strong count (racy snapshot) */
    [[nodiscard]] std::size_t use_count() const noexcept { return strong_.load(std::memory_order_relaxed); }

protected:
    /** @brief Destroy the managed object. */
    virtual void dispose() noexcept = 0;

private:
    std::atomic<std::size_t> strong_{1};
    std::atomic<std::size_t> weak_{1};
};

/** @brief Control block for an externally allocated pointer plus deleter. */
template <typename U, typename Deleter>
class PointerControlBlock final : public ControlBlockBase {
public:
    /** @param ptr managed pointer @param deleter disposal callable */
    PointerControlBlock(U* ptr, Deleter deleter) noexcept : ptr_(ptr), deleter_(std::move(deleter)) {}

protected:
    void dispose() noexcept override { deleter_(ptr_); }

private:
    U* ptr_;
    CPPVERSEHUB_NO_UNIQUE_ADDRESS Deleter deleter_;
};

/** @brief Control block that stores the object inline (single allocation for make_shared_ptr). */
template <typename T>
class InplaceControlBlock final : public ControlBlockBase {
public:
    /** @param args constructor arguments for the inline object */
    template <typename... Args>
    explicit InplaceControlBlock(Args&&... args) {
        std::construct_at(std::addressof(value_), std::forward<Args>(args)...);
    }
    InplaceControlBlock(const InplaceControlBlock&) = delete;
    InplaceControlBlock& operator=(const InplaceControlBlock&) = delete;
    InplaceControlBlock(InplaceControlBlock&&) = delete;
    InplaceControlBlock& operator=(InplaceControlBlock&&) = delete;
    /** @brief The object was already destroyed by dispose(). */
    ~InplaceControlBlock() override {}

    /** @return pointer to the inline object */
    [[nodiscard]] T* get() noexcept { return std::addressof(value_); }

protected:
    void dispose() noexcept override { std::destroy_at(std::addressof(value_)); }

private:
    union {
        T value_;
    };
};

} // namespace detail

template <typename T>
class WeakPtr;

/**
 * @brief Shared-ownership smart pointer with thread-safe reference counting.
 * @tparam T managed object type
 */
template <typename T>
class SharedPtr {
public:
    using element_type = T;
    using weak_type = WeakPtr<T>;

    /** @brief Empty pointer. */
    constexpr SharedPtr() noexcept = default;
    /** @brief Empty pointer. */
    constexpr SharedPtr(std::nullptr_t) noexcept {} // NOLINT(google-explicit-constructor)

    /** @brief Adopt p (deleted with delete). @param p owned pointer */
    template <typename U>
        requires std::convertible_to<U*, T*>
    explicit SharedPtr(U* p) : SharedPtr(p, std::default_delete<U>{}) {}

    /** @brief Adopt p with a custom deleter. @param p owned pointer @param d deleter */
    template <typename U, typename D>
        requires(std::convertible_to<U*, T*> && std::invocable<D&, U*>)
    SharedPtr(U* p, D d) : ptr_(p) {
        try {
            block_ = new detail::PointerControlBlock<U, D>(p, d);
        } catch (...) {
            d(p);
            throw;
        }
    }

    /** @brief Adopt ownership from a UniquePtr. @param owner source, left null */
    template <typename U, typename D>
        requires std::convertible_to<U*, T*>
    SharedPtr(UniquePtr<U, D>&& owner) // NOLINT(google-explicit-constructor)
        : SharedPtr() {
        if (owner) {
            block_ = new detail::PointerControlBlock<U, D>(owner.get(), owner.get_deleter());
            ptr_ = owner.release();
        }
    }

    /**
     * @brief Aliasing constructor: shares ownership with other but points at p (e.g. a member).
     * @param other owner to share with
     * @param p pointer exposed by get()
     */
    template <typename U>
    SharedPtr(const SharedPtr<U>& other, T* p) noexcept : ptr_(p), block_(other.block_) {
        if (block_ != nullptr) {
            block_->add_strong();
        }
    }

    /** @brief Share ownership. @param other source */
    SharedPtr(const SharedPtr& other) noexcept : SharedPtr(other, other.ptr_) {}

    /** @brief Converting copy (Derived -> Base). @param other source */
    template <typename U>
        requires std::convertible_to<U*, T*>
    SharedPtr(const SharedPtr<U>& other) noexcept // NOLINT(google-explicit-constructor)
        : SharedPtr(other, other.ptr_) {}

    /** @brief Transfer ownership. @param other source, left empty */
    SharedPtr(SharedPtr&& other) noexcept
        : ptr_(std::exchange(other.ptr_, nullptr)), block_(std::exchange(other.block_, nullptr)) {}

    /** @brief Converting move. @param other source, left empty */
    template <typename U>
        requires std::convertible_to<U*, T*>
    SharedPtr(SharedPtr<U>&& other) noexcept // NOLINT(google-explicit-constructor)
        : ptr_(std::exchange(other.ptr_, nullptr)), block_(std::exchange(other.block_, nullptr)) {}

    /** @brief Drop this reference. */
    ~SharedPtr() {
        if (block_ != nullptr) {
            block_->release_strong();
        }
    }

    /** @param other source @return *this */
    SharedPtr& operator=(const SharedPtr& other) noexcept {
        SharedPtr(other).swap(*this);
        return *this;
    }
    /** @param other source @return *this */
    SharedPtr& operator=(SharedPtr&& other) noexcept {
        SharedPtr(std::move(other)).swap(*this);
        return *this;
    }

    /** @return managed pointer */
    [[nodiscard]] T* get() const noexcept { return ptr_; }
    /** @return managed object (precondition: non-null) */
    [[nodiscard]] T& operator*() const noexcept { return *ptr_; }
    /** @return managed pointer */
    [[nodiscard]] T* operator->() const noexcept { return ptr_; }
    /** @return number of SharedPtr instances sharing ownership (0 if empty) */
    [[nodiscard]] std::size_t use_count() const noexcept {
        return block_ != nullptr ? block_->use_count() : 0;
    }
    /** @return true if get() != nullptr */
    [[nodiscard]] explicit operator bool() const noexcept { return ptr_ != nullptr; }

    /** @brief Become empty. */
    void reset() noexcept { SharedPtr().swap(*this); }
    /** @brief Replace with a newly adopted pointer. @param p owned pointer */
    template <typename U>
        requires std::convertible_to<U*, T*>
    void reset(U* p) {
        SharedPtr(p).swap(*this);
    }
    /** @brief Swap with other. @param other pointer to swap with */
    void swap(SharedPtr& other) noexcept {
        std::swap(ptr_, other.ptr_);
        std::swap(block_, other.block_);
    }

    /** @return true if both expose the same pointer */
    template <typename U>
    [[nodiscard]] friend bool operator==(const SharedPtr& a, const SharedPtr<U>& b) noexcept {
        return a.get() == b.get();
    }
    /** @return true if empty */
    [[nodiscard]] friend bool operator==(const SharedPtr& a, std::nullptr_t) noexcept {
        return a.ptr_ == nullptr;
    }

private:
    template <typename>
    friend class SharedPtr;
    template <typename>
    friend class WeakPtr;
    template <typename U, typename... Args>
    friend SharedPtr<U> make_shared_ptr(Args&&... args);

    // Adopts an already-counted strong reference.
    SharedPtr(T* p, detail::ControlBlockBase* block) noexcept : ptr_(p), block_(block) {}

    T* ptr_ = nullptr;
    detail::ControlBlockBase* block_ = nullptr;
};

/**
 * @brief Create a SharedPtr with the object and control block in a single allocation.
 * @param args constructor arguments
 * @return owning pointer with use_count() == 1
 */
template <typename T, typename... Args>
[[nodiscard]] SharedPtr<T> make_shared_ptr(Args&&... args) {
    auto* block = new detail::InplaceControlBlock<T>(std::forward<Args>(args)...);
    return SharedPtr<T>(block->get(), block);
}

/**
 * @brief Non-owning observer of a SharedPtr-managed object; lock() yields a SharedPtr if alive.
 * @tparam T observed object type
 */
template <typename T>
class WeakPtr {
public:
    /** @brief Empty observer. */
    constexpr WeakPtr() noexcept = default;
    /** @brief Observe owner. @param owner shared pointer to observe */
    WeakPtr(const SharedPtr<T>& owner) noexcept // NOLINT(google-explicit-constructor)
        : ptr_(owner.ptr_), block_(owner.block_) {
        if (block_ != nullptr) {
            block_->add_weak();
        }
    }
    /** @param other source */
    WeakPtr(const WeakPtr& other) noexcept : ptr_(other.ptr_), block_(other.block_) {
        if (block_ != nullptr) {
            block_->add_weak();
        }
    }
    /** @param other source, left empty */
    WeakPtr(WeakPtr&& other) noexcept
        : ptr_(std::exchange(other.ptr_, nullptr)), block_(std::exchange(other.block_, nullptr)) {}
    /** @brief Drop the weak reference. */
    ~WeakPtr() {
        if (block_ != nullptr) {
            block_->release_weak();
        }
    }
    /** @param other source @return *this */
    WeakPtr& operator=(const WeakPtr& other) noexcept {
        WeakPtr(other).swap(*this);
        return *this;
    }
    /** @param other source @return *this */
    WeakPtr& operator=(WeakPtr&& other) noexcept {
        WeakPtr(std::move(other)).swap(*this);
        return *this;
    }

    /** @return number of owners of the observed object */
    [[nodiscard]] std::size_t use_count() const noexcept {
        return block_ != nullptr ? block_->use_count() : 0;
    }
    /** @return true if the observed object has been destroyed (or nothing is observed) */
    [[nodiscard]] bool expired() const noexcept { return use_count() == 0; }

    /** @return an owning pointer if the object is alive, otherwise an empty one (race-free) */
    [[nodiscard]] SharedPtr<T> lock() const noexcept {
        if (block_ != nullptr && block_->try_add_strong()) {
            return SharedPtr<T>(ptr_, block_);
        }
        return SharedPtr<T>();
    }

    /** @brief Stop observing. */
    void reset() noexcept { WeakPtr().swap(*this); }
    /** @param other observer to swap with */
    void swap(WeakPtr& other) noexcept {
        std::swap(ptr_, other.ptr_);
        std::swap(block_, other.block_);
    }

private:
    T* ptr_ = nullptr;
    detail::ControlBlockBase* block_ = nullptr;
};

// =====================================================================================================
// Optional
// =====================================================================================================

/**
 * @brief Optional value storage implemented with an anonymous union (usable in constant expressions).
 * @tparam T contained value type (not a reference)
 */
template <typename T>
class Optional {
    static_assert(!std::is_reference_v<T>, "Optional<T&> is not supported");
    static_assert(!std::is_same_v<std::remove_cv_t<T>, std::nullopt_t>, "Optional<nullopt_t> is ill-formed");
    static_assert(!std::is_same_v<std::remove_cv_t<T>, std::in_place_t>,
                  "Optional<in_place_t> is ill-formed");

    template <typename U>
    static constexpr bool is_value_arg = std::constructible_from<T, U&&> &&
                                         !std::is_same_v<std::remove_cvref_t<U>, Optional> &&
                                         !std::is_same_v<std::remove_cvref_t<U>, std::in_place_t> &&
                                         !std::is_same_v<std::remove_cvref_t<U>, std::nullopt_t>;

public:
    using value_type = T;

    /** @brief Disengaged. */
    constexpr Optional() noexcept : empty_{} {}
    /** @brief Disengaged. */
    constexpr Optional(std::nullopt_t) noexcept : empty_{} {} // NOLINT(google-explicit-constructor)

    /** @brief Engaged with a value converted from v. @param v value */
    template <typename U = T>
        requires is_value_arg<U>
    // NOLINTNEXTLINE(google-explicit-constructor): conditionally explicit, mirroring std::optional
    constexpr explicit(!std::is_convertible_v<U&&, T>) Optional(U&& v)
        : value_(std::forward<U>(v)), engaged_(true) {}

    /** @brief Engaged with a value constructed in place. @param args constructor arguments */
    template <typename... Args>
        requires std::constructible_from<T, Args...>
    constexpr explicit Optional(std::in_place_t, Args&&... args)
        : value_(std::forward<Args>(args)...), engaged_(true) {}

    /** @param other source */
    constexpr Optional(const Optional& other)
        requires std::copy_constructible<T>
        : empty_{} {
        if (other.engaged_) {
            std::construct_at(std::addressof(value_), other.value_);
            engaged_ = true;
        }
    }

    /** @param other source (stays engaged, holding a moved-from value) */
    constexpr Optional(Optional&& other) noexcept(std::is_nothrow_move_constructible_v<T>)
        requires std::move_constructible<T>
        : empty_{} {
        if (other.engaged_) {
            std::construct_at(std::addressof(value_), std::move(other.value_));
            engaged_ = true;
        }
    }

    /** @brief Destroy the contained value, if any. */
    constexpr ~Optional() { reset(); }

    /** @param other source @return *this */
    constexpr Optional& operator=(const Optional& other)
        requires(std::copy_constructible<T> && std::is_copy_assignable_v<T>)
    {
        if (engaged_ && other.engaged_) {
            value_ = other.value_;
        } else if (other.engaged_) {
            emplace(other.value_);
        } else {
            reset();
        }
        return *this;
    }

    /** @param other source @return *this */
    constexpr Optional& operator=(Optional&& other) noexcept(std::is_nothrow_move_constructible_v<T> &&
                                                             std::is_nothrow_move_assignable_v<T>)
        requires(std::move_constructible<T> && std::is_move_assignable_v<T>)
    {
        if (engaged_ && other.engaged_) {
            value_ = std::move(other.value_);
        } else if (other.engaged_) {
            emplace(std::move(other.value_));
        } else {
            reset();
        }
        return *this;
    }

    /** @brief Disengage. @return *this */
    constexpr Optional& operator=(std::nullopt_t) noexcept {
        reset();
        return *this;
    }

    /** @brief Assign a value. @param v value @return *this */
    template <typename U = T>
        requires(is_value_arg<U> && std::is_assignable_v<T&, U &&>)
    constexpr Optional& operator=(U&& v) {
        if (engaged_) {
            value_ = std::forward<U>(v);
        } else {
            emplace(std::forward<U>(v));
        }
        return *this;
    }

    /** @return true if a value is present */
    [[nodiscard]] constexpr bool has_value() const noexcept { return engaged_; }
    /** @return true if a value is present */
    [[nodiscard]] constexpr explicit operator bool() const noexcept { return engaged_; }

    /** @return contained value @throws std::bad_optional_access if empty */
    [[nodiscard]] constexpr T& value() & {
        check();
        return value_;
    }
    /** @return contained value @throws std::bad_optional_access if empty */
    [[nodiscard]] constexpr const T& value() const& {
        check();
        return value_;
    }
    /** @return contained value moved out @throws std::bad_optional_access if empty */
    [[nodiscard]] constexpr T&& value() && {
        check();
        return std::move(value_);
    }

    /** @param fallback value used when empty @return contained value or fallback */
    template <typename U>
    [[nodiscard]] constexpr T value_or(U&& fallback) const& {
        return engaged_ ? value_ : static_cast<T>(std::forward<U>(fallback));
    }

    /** @return contained value (precondition: has_value()) */
    [[nodiscard]] constexpr T& operator*() & noexcept { return value_; }
    /** @return contained value (precondition: has_value()) */
    [[nodiscard]] constexpr const T& operator*() const& noexcept { return value_; }
    /** @return pointer to contained value (precondition: has_value()) */
    [[nodiscard]] constexpr T* operator->() noexcept { return std::addressof(value_); }
    /** @return pointer to contained value (precondition: has_value()) */
    [[nodiscard]] constexpr const T* operator->() const noexcept { return std::addressof(value_); }

    /** @brief Destroy the contained value, if any. */
    constexpr void reset() noexcept {
        if (engaged_) {
            std::destroy_at(std::addressof(value_));
            engaged_ = false;
        }
    }

    /** @brief Replace contents with a value constructed from args. @param args ctor args @return value */
    template <typename... Args>
    constexpr T& emplace(Args&&... args) {
        reset();
        std::construct_at(std::addressof(value_), std::forward<Args>(args)...);
        engaged_ = true;
        return value_;
    }

    /**
     * @brief Monadic map: apply fn to the value if present.
     * @param fn callable taking const T&
     * @return Optional of fn's result, empty if *this is empty
     */
    template <typename F>
    [[nodiscard]] constexpr auto transform(F&& fn) const& {
        using U = std::remove_cvref_t<std::invoke_result_t<F, const T&>>;
        return engaged_ ? Optional<U>(std::invoke(std::forward<F>(fn), value_)) : Optional<U>();
    }

    /**
     * @brief Monadic bind: fn returns an Optional; empty propagates.
     * @param fn callable taking const T& and returning Optional<U>
     * @return fn's result or an empty Optional
     */
    template <typename F>
    [[nodiscard]] constexpr auto and_then(F&& fn) const& {
        using R = std::remove_cvref_t<std::invoke_result_t<F, const T&>>;
        return engaged_ ? std::invoke(std::forward<F>(fn), value_) : R();
    }

    /** @brief Swap contents. @param other optional to swap with */
    constexpr void swap(Optional& other) noexcept(std::is_nothrow_move_constructible_v<T> &&
                                                  std::is_nothrow_swappable_v<T>) {
        if (engaged_ && other.engaged_) {
            using std::swap;
            swap(value_, other.value_);
        } else if (engaged_) {
            other.emplace(std::move(value_));
            reset();
        } else if (other.engaged_) {
            emplace(std::move(other.value_));
            other.reset();
        }
    }

    /** @return true if both empty or both engaged with equal values */
    [[nodiscard]] friend constexpr bool operator==(const Optional& a, const Optional& b)
        requires std::equality_comparable<T>
    {
        if (a.engaged_ != b.engaged_) {
            return false;
        }
        return !a.engaged_ || a.value_ == b.value_;
    }
    /** @return true if empty */
    [[nodiscard]] friend constexpr bool operator==(const Optional& a, std::nullopt_t) noexcept {
        return !a.engaged_;
    }
    /**
     * @return true if engaged and equal to v. The Optional parameter is deduced (Self) so that no
     *         implicit conversion to Optional is considered; otherwise checking `T == U` could
     *         recurse into this very overload through Optional's converting constructor.
     */
    template <typename Self, typename U>
        requires(std::is_same_v<Self, Optional> && !std::is_same_v<std::remove_cvref_t<U>, Optional> &&
                 !std::is_same_v<std::remove_cvref_t<U>, std::nullopt_t> &&
                 requires(const T& t, const U& u) {
                     { t == u } -> std::convertible_to<bool>;
                 })
    [[nodiscard]] friend constexpr bool operator==(const Self& a, const U& v) {
        return a.engaged_ && a.value_ == v;
    }

private:
    constexpr void check() const {
        if (!engaged_) {
            throw std::bad_optional_access();
        }
    }

    union {
        char empty_;
        T value_;
    };
    bool engaged_ = false;
};

/** @brief Deduce Optional<T> from a value. */
template <typename T>
Optional(T) -> Optional<T>;

// =====================================================================================================
// Showcase
// =====================================================================================================

/**
 * @brief Exercise the hand-written containers and smart pointers.
 * @param out destination stream
 */
inline void demonstrate_generic_containers(std::ostream& out = std::cout) {
    out << "--- Generic containers ---\n";
    DynamicArray<int> numbers{1, 2, 3};
    numbers.push_back(4);
    numbers.insert(numbers.begin(), 0);
    numbers.erase(numbers.begin() + 2);
    out << "DynamicArray        =";
    for (int n : numbers) {
        out << ' ' << n;
    }
    out << " (size " << numbers.size() << ", capacity " << numbers.capacity() << ")\n";

    DynamicArray<std::string> words(2, std::string("hi"));
    words.emplace_back(3, 'x');
    out << "DynamicArray<string> back = " << words.back() << '\n';

    auto unique = make_unique_ptr<std::string>("owned");
    out << "UniquePtr           = " << *unique << '\n';
    auto array = make_unique_ptr<int[]>(3);
    array[1] = 42;
    out << "UniquePtr<int[]>[1] = " << array[1] << '\n';

    auto shared = make_shared_ptr<std::string>("shared");
    WeakPtr<std::string> observer = shared;
    { out << "SharedPtr use_count = " << shared.use_count() << '\n'; }
    out << "after scope         = " << shared.use_count() << ", weak expired: " << std::boolalpha
        << observer.expired() << '\n';
    shared.reset();
    out << "after reset expired = " << observer.expired() << std::noboolalpha << '\n';

    Optional<int> maybe;
    out << "Optional empty      = " << maybe.value_or(-1) << '\n';
    maybe = 21;
    out << "Optional doubled    = " << maybe.transform([](int v) { return v * 2; }).value() << '\n';
}

} // namespace CppVerseHub::Templates

#endif // CPPVERSEHUB_TEMPLATES_GENERIC_CONTAINERS_HPP
