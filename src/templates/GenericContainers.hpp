// File: src/templates/GenericContainers.hpp
// Custom template containers demonstrating advanced template programming

#ifndef GENERIC_CONTAINERS_HPP
#define GENERIC_CONTAINERS_HPP

#include <memory>
#include <utility>
#include <iterator>
#include <functional>
#include <type_traits>
#include <initializer_list>
#include <algorithm>
#include <concepts>
#include <ranges>
#include <iostream>

namespace CppVerseHub {
namespace Templates {

/**
 * @brief Custom dynamic array implementation with template programming features
 * @tparam T Element type
 * @tparam Allocator Custom allocator type (default: std::allocator<T>)
 */
template<typename T, typename Allocator = std::allocator<T>>
class DynamicArray {
public:
    // Type aliases for STL compatibility
    using value_type = T;
    using allocator_type = Allocator;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;
    using reference = T&;
    using const_reference = const T&;
    using pointer = typename std::allocator_traits<Allocator>::pointer;
    using const_pointer = typename std::allocator_traits<Allocator>::const_pointer;

    // Iterator implementation
    class iterator {
    public:
        using iterator_category = std::random_access_iterator_tag;
        using value_type = T;
        using difference_type = std::ptrdiff_t;
        using pointer = T*;
        using reference = T&;

        iterator() : ptr_(nullptr) {}
        explicit iterator(pointer ptr) : ptr_(ptr) {}

        reference operator*() const { return *ptr_; }
        pointer operator->() const { return ptr_; }
        reference operator[](difference_type n) const { return ptr_[n]; }

        iterator& operator++() { ++ptr_; return *this; }
        iterator operator++(int) { iterator tmp = *this; ++ptr_; return tmp; }
        iterator& operator--() { --ptr_; return *this; }
        iterator operator--(int) { iterator tmp = *this; --ptr_; return tmp; }

        iterator& operator+=(difference_type n) { ptr_ += n; return *this; }
        iterator& operator-=(difference_type n) { ptr_ -= n; return *this; }
        
        iterator operator+(difference_type n) const { return iterator(ptr_ + n); }
        iterator operator-(difference_type n) const { return iterator(ptr_ - n); }
        difference_type operator-(const iterator& other) const { return ptr_ - other.ptr_; }

        bool operator==(const iterator& other) const { return ptr_ == other.ptr_; }
        bool operator!=(const iterator& other) const { return ptr_ != other.ptr_; }
        bool operator<(const iterator& other) const { return ptr_ < other.ptr_; }
        bool operator<=(const iterator& other) const { return ptr_ <= other.ptr_; }
        bool operator>(const iterator& other) const { return ptr_ > other.ptr_; }
        bool operator>=(const iterator& other) const { return ptr_ >= other.ptr_; }

    private:
        pointer ptr_;
    };

    class const_iterator {
    public:
        using iterator_category = std::random_access_iterator_tag;
        using value_type = T;
        using difference_type = std::ptrdiff_t;
        using pointer = const T*;
        using reference = const T&;

        const_iterator() : ptr_(nullptr) {}
        explicit const_iterator(pointer ptr) : ptr_(ptr) {}
        const_iterator(const iterator& it) : ptr_(&(*it)) {}

        reference operator*() const { return *ptr_; }
        pointer operator->() const { return ptr_; }
        reference operator[](difference_type n) const { return ptr_[n]; }

        const_iterator& operator++() { ++ptr_; return *this; }
        const_iterator operator++(int) { const_iterator tmp = *this; ++ptr_; return tmp; }
        const_iterator& operator--() { --ptr_; return *this; }
        const_iterator operator--(int) { const_iterator tmp = *this; --ptr_; return tmp; }

        const_iterator& operator+=(difference_type n) { ptr_ += n; return *this; }
        const_iterator& operator-=(difference_type n) { ptr_ -= n; return *this; }
        
        const_iterator operator+(difference_type n) const { return const_iterator(ptr_ + n); }
        const_iterator operator-(difference_type n) const { return const_iterator(ptr_ - n); }
        difference_type operator-(const const_iterator& other) const { return ptr_ - other.ptr_; }

        bool operator==(const const_iterator& other) const { return ptr_ == other.ptr_; }
        bool operator!=(const const_iterator& other) const { return ptr_ != other.ptr_; }
        bool operator<(const const_iterator& other) const { return ptr_ < other.ptr_; }
        bool operator<=(const const_iterator& other) const { return ptr_ <= other.ptr_; }
        bool operator>(const const_iterator& other) const { return ptr_ > other.ptr_; }
        bool operator>=(const const_iterator& other) const { return ptr_ >= other.ptr_; }

    private:
        pointer ptr_;
    };

    using reverse_iterator = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;

private:
    pointer data_;
    size_type size_;
    size_type capacity_;
    [[no_unique_address]] allocator_type allocator_;

    void reallocate(size_type new_capacity) {
        pointer new_data = std::allocator_traits<Allocator>::allocate(allocator_, new_capacity);
        
        if constexpr (std::is_nothrow_move_constructible_v<T>) {
            std::uninitialized_move(data_, data_ + size_, new_data);
        } else {
            try {
                std::uninitialized_copy(data_, data_ + size_, new_data);
            } catch (...) {
                std::allocator_traits<Allocator>::deallocate(allocator_, new_data, new_capacity);
                throw;
            }
        }
        
        destroy_all();
        std::allocator_traits<Allocator>::deallocate(allocator_, data_, capacity_);
        
        data_ = new_data;
        capacity_ = new_capacity;
    }

    void destroy_all() noexcept {
        for (size_type i = 0; i < size_; ++i) {
            std::allocator_traits<Allocator>::destroy(allocator_, data_ + i);
        }
        size_ = 0;
    }

public:
    // Constructors
    DynamicArray() : data_(nullptr), size_(0), capacity_(0), allocator_() {}
    
    explicit DynamicArray(const Allocator& alloc) 
        : data_(nullptr), size_(0), capacity_(0), allocator_(alloc) {}
    
    explicit DynamicArray(size_type count, const T& value = T{}, const Allocator& alloc = Allocator{})
        : size_(count), capacity_(count), allocator_(alloc) {
        data_ = std::allocator_traits<Allocator>::allocate(allocator_, capacity_);
        std::uninitialized_fill_n(data_, size_, value);
    }

    template<typename InputIt>
    DynamicArray(InputIt first, InputIt last, const Allocator& alloc = Allocator{})
        : allocator_(alloc) {
        if constexpr (std::random_access_iterator<InputIt>) {
            size_ = std::distance(first, last);
            capacity_ = size_;
            data_ = std::allocator_traits<Allocator>::allocate(allocator_, capacity_);
            std::uninitialized_copy(first, last, data_);
        } else {
            data_ = nullptr;
            size_ = 0;
            capacity_ = 0;
            for (auto it = first; it != last; ++it) {
                push_back(*it);
            }
        }
    }

    DynamicArray(std::initializer_list<T> init, const Allocator& alloc = Allocator{})
        : DynamicArray(init.begin(), init.end(), alloc) {}

    // Copy constructor
    DynamicArray(const DynamicArray& other)
        : size_(other.size_), capacity_(other.capacity_), 
          allocator_(std::allocator_traits<Allocator>::select_on_container_copy_construction(other.allocator_)) {
        data_ = std::allocator_traits<Allocator>::allocate(allocator_, capacity_);
        std::uninitialized_copy(other.data_, other.data_ + size_, data_);
    }

    // Move constructor
    DynamicArray(DynamicArray&& other) noexcept
        : data_(std::exchange(other.data_, nullptr)),
          size_(std::exchange(other.size_, 0)),
          capacity_(std::exchange(other.capacity_, 0)),
          allocator_(std::move(other.allocator_)) {}

    // Destructor
    ~DynamicArray() {
        if (data_) {
            destroy_all();
            std::allocator_traits<Allocator>::deallocate(allocator_, data_, capacity_);
        }
    }

    // Assignment operators
    DynamicArray& operator=(const DynamicArray& other) {
        if (this != &other) {
            DynamicArray tmp(other);
            swap(tmp);
        }
        return *this;
    }

    DynamicArray& operator=(DynamicArray&& other) noexcept {
        if (this != &other) {
            DynamicArray tmp(std::move(other));
            swap(tmp);
        }
        return *this;
    }

    DynamicArray& operator=(std::initializer_list<T> init) {
        DynamicArray tmp(init, allocator_);
        swap(tmp);
        return *this;
    }

    // Element access
    reference at(size_type pos) {
        if (pos >= size_) {
            throw std::out_of_range("DynamicArray::at: index out of range");
        }
        return data_[pos];
    }

    const_reference at(size_type pos) const {
        if (pos >= size_) {
            throw std::out_of_range("DynamicArray::at: index out of range");
        }
        return data_[pos];
    }

    reference operator[](size_type pos) noexcept { return data_[pos]; }
    const_reference operator[](size_type pos) const noexcept { return data_[pos]; }

    reference front() noexcept { return data_[0]; }
    const_reference front() const noexcept { return data_[0]; }

    reference back() noexcept { return data_[size_ - 1]; }
    const_reference back() const noexcept { return data_[size_ - 1]; }

    T* data() noexcept { return data_; }
    const T* data() const noexcept { return data_; }

    // Iterators
    iterator begin() noexcept { return iterator(data_); }
    const_iterator begin() const noexcept { return const_iterator(data_); }
    const_iterator cbegin() const noexcept { return const_iterator(data_); }

    iterator end() noexcept { return iterator(data_ + size_); }
    const_iterator end() const noexcept { return const_iterator(data_ + size_); }
    const_iterator cend() const noexcept { return const_iterator(data_ + size_); }

    reverse_iterator rbegin() noexcept { return reverse_iterator(end()); }
    const_reverse_iterator rbegin() const noexcept { return const_reverse_iterator(end()); }
    const_reverse_iterator crbegin() const noexcept { return const_reverse_iterator(end()); }

    reverse_iterator rend() noexcept { return reverse_iterator(begin()); }
    const_reverse_iterator rend() const noexcept { return const_reverse_iterator(begin()); }
    const_reverse_iterator crend() const noexcept { return const_reverse_iterator(begin()); }

    // Capacity
    bool empty() const noexcept { return size_ == 0; }
    size_type size() const noexcept { return size_; }
    size_type max_size() const noexcept { return std::allocator_traits<Allocator>::max_size(allocator_); }
    size_type capacity() const noexcept { return capacity_; }

    void reserve(size_type new_cap) {
        if (new_cap > capacity_) {
            reallocate(new_cap);
        }
    }

    void shrink_to_fit() {
        if (size_ < capacity_) {
            DynamicArray tmp(*this);
            swap(tmp);
        }
    }

    // Modifiers
    void clear() noexcept {
        destroy_all();
    }

    iterator insert(const_iterator pos, const T& value) {
        return emplace(pos, value);
    }

    iterator insert(const_iterator pos, T&& value) {
        return emplace(pos, std::move(value));
    }

    iterator insert(const_iterator pos, size_type count, const T& value) {
        auto offset = pos - cbegin();
        if (size_ + count > capacity_) {
            reallocate(std::max(capacity_ * 2, size_ + count));
        }
        
        auto insert_pos = begin() + offset;
        std::move_backward(insert_pos, end(), end() + count);
        std::uninitialized_fill_n(insert_pos, count, value);
        size_ += count;
        
        return insert_pos;
    }

    template<typename InputIt>
    iterator insert(const_iterator pos, InputIt first, InputIt last) {
        auto offset = pos - cbegin();
        size_type count = std::distance(first, last);
        
        if (size_ + count > capacity_) {
            reallocate(std::max(capacity_ * 2, size_ + count));
        }
        
        auto insert_pos = begin() + offset;
        std::move_backward(insert_pos, end(), end() + count);
        std::uninitialized_copy(first, last, insert_pos);
        size_ += count;
        
        return insert_pos;
    }

    template<typename... Args>
    iterator emplace(const_iterator pos, Args&&... args) {
        auto offset = pos - cbegin();
        
        if (size_ >= capacity_) {
            reallocate(capacity_ == 0 ? 1 : capacity_ * 2);
        }
        
        auto emplace_pos = begin() + offset;
        if (emplace_pos != end()) {
            std::move_backward(emplace_pos, end(), end() + 1);
        }
        
        std::allocator_traits<Allocator>::construct(allocator_, &(*emplace_pos), std::forward<Args>(args)...);
        ++size_;
        
        return emplace_pos;
    }

    iterator erase(const_iterator pos) {
        return erase(pos, pos + 1);
    }

    iterator erase(const_iterator first, const_iterator last) {
        auto start_offset = first - cbegin();
        auto end_offset = last - cbegin();
        auto count = last - first;
        
        auto erase_start = begin() + start_offset;
        auto erase_end = begin() + end_offset;
        
        // Destroy elements in range
        for (auto it = erase_start; it != erase_end; ++it) {
            std::allocator_traits<Allocator>::destroy(allocator_, &(*it));
        }
        
        // Move remaining elements
        std::move(erase_end, end(), erase_start);
        size_ -= count;
        
        return erase_start;
    }

    void push_back(const T& value) {
        emplace_back(value);
    }

    void push_back(T&& value) {
        emplace_back(std::move(value));
    }

    template<typename... Args>
    reference emplace_back(Args&&... args) {
        if (size_ >= capacity_) {
            reallocate(capacity_ == 0 ? 1 : capacity_ * 2);
        }
        
        std::allocator_traits<Allocator>::construct(allocator_, data_ + size_, std::forward<Args>(args)...);
        ++size_;
        
        return back();
    }

    void pop_back() noexcept {
        if (!empty()) {
            --size_;
            std::allocator_traits<Allocator>::destroy(allocator_, data_ + size_);
        }
    }

    void resize(size_type count) {
        resize(count, T{});
    }

    void resize(size_type count, const T& value) {
        if (count < size_) {
            // Shrink
            for (size_type i = count; i < size_; ++i) {
                std::allocator_traits<Allocator>::destroy(allocator_, data_ + i);
            }
            size_ = count;
        } else if (count > size_) {
            // Grow
            if (count > capacity_) {
                reallocate(count);
            }
            std::uninitialized_fill(data_ + size_, data_ + count, value);
            size_ = count;
        }
    }

    void swap(DynamicArray& other) noexcept {
        using std::swap;
        swap(data_, other.data_);
        swap(size_, other.size_);
        swap(capacity_, other.capacity_);
        
        if constexpr (std::allocator_traits<Allocator>::propagate_on_container_swap::value) {
            swap(allocator_, other.allocator_);
        }
    }

    // Allocator access
    allocator_type get_allocator() const noexcept {
        return allocator_;
    }

    // Comparison operators
    bool operator==(const DynamicArray& other) const {
        return size_ == other.size_ && std::equal(begin(), end(), other.begin());
    }

    bool operator!=(const DynamicArray& other) const {
        return !(*this == other);
    }

    bool operator<(const DynamicArray& other) const {
        return std::lexicographical_compare(begin(), end(), other.begin(), other.end());
    }

    bool operator<=(const DynamicArray& other) const {
        return !(other < *this);
    }

    bool operator>(const DynamicArray& other) const {
        return other < *this;
    }

    bool operator>=(const DynamicArray& other) const {
        return !(*this < other);
    }

    // C++20 spaceship operator
    auto operator<=>(const DynamicArray& other) const
        requires std::three_way_comparable<T> {
        return std::lexicographical_compare_three_way(begin(), end(), other.begin(), other.end());
    }
};

// Non-member functions
template<typename T, typename Allocator>
void swap(DynamicArray<T, Allocator>& lhs, DynamicArray<T, Allocator>& rhs) noexcept {
    lhs.swap(rhs);
}

/**
 * @brief Template-based smart pointer with custom deleters
 * @tparam T Managed object type
 * @tparam Deleter Custom deleter type
 */
template<typename T, typename Deleter = std::default_delete<T>>
class unique_ptr {
public:
    using element_type = T;
    using deleter_type = Deleter;
    using pointer = T*;

private:
    pointer ptr_;
    [[no_unique_address]] deleter_type deleter_;

public:
    // Constructors
    constexpr unique_ptr() noexcept : ptr_(nullptr) {}
    constexpr unique_ptr(std::nullptr_t) noexcept : ptr_(nullptr) {}
    
    explicit unique_ptr(pointer p) noexcept : ptr_(p) {}
    
    unique_ptr(pointer p, const deleter_type& d) noexcept
        : ptr_(p), deleter_(d) {}
    
    unique_ptr(pointer p, deleter_type&& d) noexcept
        : ptr_(p), deleter_(std::move(d)) {}

    // Move constructor
    unique_ptr(unique_ptr&& other) noexcept
        : ptr_(std::exchange(other.ptr_, nullptr)),
          deleter_(std::move(other.deleter_)) {}

    // Converting move constructor
    template<typename U, typename E>
        requires std::convertible_to<U*, T*> && 
                 (std::is_reference_v<Deleter> ? std::is_same_v<E, Deleter> : std::convertible_to<E, Deleter>)
    unique_ptr(unique_ptr<U, E>&& other) noexcept
        : ptr_(other.release()), deleter_(std::forward<E>(other.get_deleter())) {}

    // Destructor
    ~unique_ptr() {
        if (ptr_) {
            deleter_(ptr_);
        }
    }

    // Assignment
    unique_ptr& operator=(unique_ptr&& other) noexcept {
        if (this != &other) {
            reset(other.release());
            deleter_ = std::move(other.deleter_);
        }
        return *this;
    }

    template<typename U, typename E>
        requires std::convertible_to<U*, T*> && std::assignable_from<Deleter&, E&&>
    unique_ptr& operator=(unique_ptr<U, E>&& other) noexcept {
        reset(other.release());
        deleter_ = std::forward<E>(other.get_deleter());
        return *this;
    }

    unique_ptr& operator=(std::nullptr_t) noexcept {
        reset();
        return *this;
    }

    // Observers
    T& operator*() const noexcept { return *ptr_; }
    pointer operator->() const noexcept { return ptr_; }
    pointer get() const noexcept { return ptr_; }
    deleter_type& get_deleter() noexcept { return deleter_; }
    const deleter_type& get_deleter() const noexcept { return deleter_; }
    explicit operator bool() const noexcept { return ptr_ != nullptr; }

    // Modifiers
    pointer release() noexcept {
        return std::exchange(ptr_, nullptr);
    }

    void reset(pointer p = pointer()) noexcept {
        pointer old_ptr = std::exchange(ptr_, p);
        if (old_ptr) {
            deleter_(old_ptr);
        }
    }

    void swap(unique_ptr& other) noexcept {
        using std::swap;
        swap(ptr_, other.ptr_);
        swap(deleter_, other.deleter_);
    }

    // Delete copy operations
    unique_ptr(const unique_ptr&) = delete;
    unique_ptr& operator=(const unique_ptr&) = delete;
};

// Array specialization
template<typename T, typename Deleter>
class unique_ptr<T[], Deleter> {
public:
    using element_type = T;
    using deleter_type = Deleter;
    using pointer = T*;

private:
    pointer ptr_;
    [[no_unique_address]] deleter_type deleter_;

public:
    // Similar interface to above but with array-specific operations
    constexpr unique_ptr() noexcept : ptr_(nullptr) {}
    constexpr unique_ptr(std::nullptr_t) noexcept : ptr_(nullptr) {}
    
    template<typename U>
        requires std::convertible_to<U(*)[], T(*)[]>
    explicit unique_ptr(U* p) noexcept : ptr_(p) {}

    T& operator[](std::size_t i) const { return ptr_[i]; }

    // Similar implementation as above...
};

// Make functions for unique_ptr
template<typename T, typename... Args>
    requires (!std::is_array_v<T>)
unique_ptr<T> make_unique(Args&&... args) {
    return unique_ptr<T>(new T(std::forward<Args>(args)...));
}

template<typename T>
    requires std::is_array_v<T> && std::extent_v<T> == 0
unique_ptr<T> make_unique(std::size_t size) {
    using U = std::remove_extent_t<T>;
    return unique_ptr<T>(new U[size]());
}

/**
 * @brief Thread-safe reference-counted smart pointer
 * @tparam T Managed object type
 */
template<typename T>
class shared_ptr {
public:
    using element_type = T;
    using weak_type = weak_ptr<T>;

private:
    struct ControlBlock {
        std::atomic<std::size_t> ref_count{1};
        std::atomic<std::size_t> weak_count{1};
        
        virtual ~ControlBlock() = default;
        virtual void destroy_object() = 0;
        virtual void destroy_control_block() = 0;
    };

    template<typename U, typename Deleter, typename Allocator>
    struct ControlBlockImpl : ControlBlock {
        U* ptr;
        [[no_unique_address]] Deleter deleter;
        [[no_unique_address]] Allocator allocator;

        ControlBlockImpl(U* p, Deleter d, Allocator a)
            : ptr(p), deleter(std::move(d)), allocator(std::move(a)) {}

        void destroy_object() override {
            deleter(ptr);
            ptr = nullptr;
        }

        void destroy_control_block() override {
            using AllocTraits = std::allocator_traits<Allocator>;
            using BlockAlloc = typename AllocTraits::template rebind_alloc<ControlBlockImpl>;
            
            BlockAlloc block_alloc(allocator);
            AllocTraits::deallocate(block_alloc, this, 1);
        }
    };

    T* ptr_;
    ControlBlock* control_block_;

public:
    // Constructors
    constexpr shared_ptr() noexcept : ptr_(nullptr), control_block_(nullptr) {}
    constexpr shared_ptr(std::nullptr_t) noexcept : ptr_(nullptr), control_block_(nullptr) {}

    template<typename U>
        requires std::convertible_to<U*, T*>
    explicit shared_ptr(U* ptr) : ptr_(ptr) {
        try {
            control_block_ = new ControlBlockImpl<U, std::default_delete<U>, std::allocator<U>>(
                ptr, std::default_delete<U>{}, std::allocator<U>{}
            );
        } catch (...) {
            delete ptr;
            throw;
        }
    }

    // Copy constructor
    shared_ptr(const shared_ptr& other) noexcept
        : ptr_(other.ptr_), control_block_(other.control_block_) {
        if (control_block_) {
            control_block_->ref_count.fetch_add(1, std::memory_order_relaxed);
        }
    }

    // Move constructor
    shared_ptr(shared_ptr&& other) noexcept
        : ptr_(std::exchange(other.ptr_, nullptr)),
          control_block_(std::exchange(other.control_block_, nullptr)) {}

    // Destructor
    ~shared_ptr() {
        if (control_block_) {
            if (control_block_->ref_count.fetch_sub(1, std::memory_order_acq_rel) == 1) {
                control_block_->destroy_object();
                if (control_block_->weak_count.fetch_sub(1, std::memory_order_acq_rel) == 1) {
                    control_block_->destroy_control_block();
                }
            }
        }
    }

    // Assignment
    shared_ptr& operator=(const shared_ptr& other) noexcept {
        shared_ptr(other).swap(*this);
        return *this;
    }

    shared_ptr& operator=(shared_ptr&& other) noexcept {
        shared_ptr(std::move(other)).swap(*this);
        return *this;
    }

    // Observers
    T* get() const noexcept { return ptr_; }
    T& operator*() const noexcept { return *ptr_; }
    T* operator->() const noexcept { return ptr_; }
    std::size_t use_count() const noexcept {
        return control_block_ ? control_block_->ref_count.load(std::memory_order_relaxed) : 0;
    }
    explicit operator bool() const noexcept { return ptr_ != nullptr; }

    // Modifiers
    void reset() noexcept {
        shared_ptr().swap(*this);
    }

    template<typename U>
    void reset(U* ptr) {
        shared_ptr(ptr).swap(*this);
    }

    void swap(shared_ptr& other) noexcept {
        using std::swap;
        swap(ptr_, other.ptr_);
        swap(control_block_, other.control_block_);
    }
};

// Make functions for shared_ptr
template<typename T, typename... Args>
    requires (!std::is_array_v<T>)
shared_ptr<T> make_shared(Args&&... args) {
    // In a real implementation, this would use a single allocation
    // for both the object and control block
    return shared_ptr<T>(new T(std::forward<Args>(args)...));
}

/**
 * @brief Template-based optional container
 * @tparam T Value type
 */
template<typename T>
class Optional {
private:
    alignas(T) char storage_[sizeof(T)];
    bool has_value_;

    T* get_ptr() noexcept { return reinterpret_cast<T*>(storage_); }
    const T* get_ptr() const noexcept { return reinterpret_cast<const T*>(storage_); }

public:
    using value_type = T;

    // Constructors
    constexpr Optional() noexcept : has_value_(false) {}
    constexpr Optional(std::nullopt_t) noexcept : has_value_(false) {}

    constexpr Optional(const T& value) : has_value_(true) {
        new (storage_) T(value);
    }

    constexpr Optional(T&& value) : has_value_(true) {
        new (storage_) T(std::move(value));
    }

    template<typename... Args>
    constexpr explicit Optional(std::in_place_t, Args&&... args) : has_value_(true) {
        new (storage_) T(std::forward<Args>(args)...);
    }

    // Copy constructor
    Optional(const Optional& other) : has_value_(other.has_value_) {
        if (has_value_) {
            new (storage_) T(*other.get_ptr());
        }
    }

    // Move constructor
    Optional(Optional&& other) noexcept(std::is_nothrow_move_constructible_v<T>)
        : has_value_(other.has_value_) {
        if (has_value_) {
            new (storage_) T(std::move(*other.get_ptr()));
        }
    }

    // Destructor
    ~Optional() {
        if (has_value_) {
            get_ptr()->~T();
        }
    }

    // Assignment
    Optional& operator=(const Optional& other) {
        if (this != &other) {
            if (has_value_ && other.has_value_) {
                *get_ptr() = *other.get_ptr();
            } else if (other.has_value_) {
                emplace(*other.get_ptr());
            } else {
                reset();
            }
        }
        return *this;
    }

    Optional& operator=(Optional&& other) noexcept(std::is_nothrow_move_assignable_v<T> && 
                                                   std::is_nothrow_move_constructible_v<T>) {
        if (this != &other) {
            if (has_value_ && other.has_value_) {
                *get_ptr() = std::move(*other.get_ptr());
            } else if (other.has_value_) {
                emplace(std::move(*other.get_ptr()));
            } else {
                reset();
            }
        }
        return *this;
    }

    Optional& operator=(const T& value) {
        if (has_value_) {
            *get_ptr() = value;
        } else {
            emplace(value);
        }
        return *this;
    }

    Optional& operator=(T&& value) {
        if (has_value_) {
            *get_ptr() = std::move(value);
        } else {
            emplace(std::move(value));
        }
        return *this;
    }

    // Observers
    constexpr bool has_value() const noexcept { return has_value_; }
    constexpr explicit operator bool() const noexcept { return has_value_; }

    constexpr T& value() & {
        if (!has_value_) {
            throw std::bad_optional_access();
        }
        return *get_ptr();
    }

    constexpr const T& value() const& {
        if (!has_value_) {
            throw std::bad_optional_access();
        }
        return *get_ptr();
    }

    constexpr T&& value() && {
        if (!has_value_) {
            throw std::bad_optional_access();
        }
        return std::move(*get_ptr());
    }

    constexpr const T&& value() const&& {
        if (!has_value_) {
            throw std::bad_optional_access();
        }
        return std::move(*get_ptr());
    }

    template<typename U>
    constexpr T value_or(U&& default_value) const& {
        return has_value_ ? *get_ptr() : static_cast<T>(std::forward<U>(default_value));
    }

    template<typename U>
    constexpr T value_or(U&& default_value) && {
        return has_value_ ? std::move(*get_ptr()) : static_cast<T>(std::forward<U>(default_value));
    }

    constexpr T& operator*() & noexcept { return *get_ptr(); }
    constexpr const T& operator*() const& noexcept { return *get_ptr(); }
    constexpr T&& operator*() && noexcept { return std::move(*get_ptr()); }
    constexpr const T&& operator*() const&& noexcept { return std::move(*get_ptr()); }

    constexpr T* operator->() noexcept { return get_ptr(); }
    constexpr const T* operator->() const noexcept { return get_ptr(); }

    // Modifiers
    void reset() noexcept {
        if (has_value_) {
            get_ptr()->~T();
            has_value_ = false;
        }
    }

    template<typename... Args>
    T& emplace(Args&&... args) {
        reset();
        new (storage_) T(std::forward<Args>(args)...);
        has_value_ = true;
        return *get_ptr();
    }

    void swap(Optional& other) noexcept(std::is_nothrow_move_constructible_v<T> && 
                                        std::is_nothrow_move_assignable_v<T>) {
        if (has_value_ && other.has_value_) {
            using std::swap;
            swap(*get_ptr(), *other.get_ptr());
        } else if (has_value_) {
            other.emplace(std::move(*get_ptr()));
            reset();
        } else if (other.has_value_) {
            emplace(std::move(*other.get_ptr()));
            other.reset();
        }
    }
};

// Comparison operators for Optional
template<typename T>
constexpr bool operator==(const Optional<T>& lhs, const Optional<T>& rhs) {
    if (lhs.has_value() != rhs.has_value()) return false;
    return !lhs.has_value() || *lhs == *rhs;
}

template<typename T, typename U>
constexpr bool operator==(const Optional<T>& opt, const U& value) {
    return opt.has_value() && *opt == value;
}

} // namespace Templates
} // namespace CppVerseHub

// Enable range-based for loop support
namespace std {
    template<typename T, typename A>
    struct iterator_traits<typename CppVerseHub::Templates::DynamicArray<T, A>::iterator> {
        using iterator_category = std::random_access_iterator_tag;
        using value_type = T;
        using difference_type = std::ptrdiff_t;
        using pointer = T*;
        using reference = T&;
    };

    template<typename T, typename A>
    struct iterator_traits<typename CppVerseHub::Templates::DynamicArray<T, A>::const_iterator> {
        using iterator_category = std::random_access_iterator_tag;
        using value_type = T;
        using difference_type = std::ptrdiff_t;
        using pointer = const T*;
        using reference = const T&;
    };
}

#endif // GENERIC_CONTAINERS_HPP