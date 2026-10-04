/**
 * @file DataStructures.hpp
 * @brief Classic data structures implemented from first principles with modern C++ ownership.
 *
 * Each structure demonstrates a specific implementation technique:
 *  - DynamicArray: manual storage management (allocate / construct_at / destroy) with the rule of
 *    five and the *strong* exception guarantee on reallocation (move_if_noexcept semantics).
 *  - LinkedList: unique_ptr ownership chain plus a non-owning tail pointer, a standard-conforming
 *    forward iterator, and iterative destruction so long lists cannot overflow the stack.
 *  - BinarySearchTree: unique_ptr-owned nodes with iterative insert/erase/copy/destroy.
 *  - MinHeap: implicit binary heap in a vector with Floyd's O(n) heapify.
 *  - HashTable: open addressing with linear probing and tombstone-free backward-shift deletion.
 *  - Trie: prefix tree with ordered children for lexicographic enumeration.
 *  - DisjointSet: union-find with union by size and path halving (inverse-Ackermann time).
 *  - BloomFilter: probabilistic membership with Kirsch-Mitzenmacher double hashing.
 *  - SkipList: randomised ordered set with expected O(log n) operations.
 *
 * None of these print anything; the showcase is `demonstrate_data_structures(std::ostream&)`.
 */

#ifndef CPPVERSEHUB_ALGORITHMS_DATASTRUCTURES_HPP
#define CPPVERSEHUB_ALGORITHMS_DATASTRUCTURES_HPP

#include <algorithm>
#include <array>
#include <bit>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <initializer_list>
#include <iostream>
#include <iterator>
#include <map>
#include <memory>
#include <optional>
#include <random>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

namespace CppVerseHub::Algorithms {

// ============================================================================================
// DynamicArray
// ============================================================================================

/**
 * @class DynamicArray
 * @brief A minimal `std::vector`: contiguous storage with geometric (x2) growth.
 *
 * push_back/emplace_back: amortised O(1); operator[]: O(1); erase: O(n). Space: O(capacity).
 * Reallocation provides the strong exception guarantee: elements are moved only when their move
 * constructor is noexcept (or the type is not copyable), otherwise copied.
 *
 * @tparam T Element type (must be destructible; operations add their own requirements).
 */
template <class T>
class DynamicArray {
public:
    using value_type = T;            ///< Element type.
    using size_type = std::size_t;   ///< Size type.
    using iterator = T*;             ///< Contiguous iterator.
    using const_iterator = const T*; ///< Contiguous const iterator.

    /// @brief Creates an empty array without allocating.
    DynamicArray() noexcept = default;

    /**
     * @brief Creates `count` value-initialised elements.
     * @param count Number of elements.
     */
    explicit DynamicArray(size_type count)
        requires std::default_initializable<T>
    {
        reserve(count);
        for (size_type i = 0; i < count; ++i) {
            emplace_back();
        }
    }

    /**
     * @brief Creates an array holding copies of `init`.
     * @param init Initial elements.
     */
    DynamicArray(std::initializer_list<T> init)
        requires std::copy_constructible<T>
    {
        reserve(init.size());
        for (const T& v : init) {
            emplace_back(v);
        }
    }

    /// @brief Deep copy. Time O(n).
    DynamicArray(const DynamicArray& other)
        requires std::copy_constructible<T>
    {
        reserve(other.size_);
        for (const T& v : other) {
            emplace_back(v);
        }
    }

    /// @brief Steals `other`'s buffer. O(1), leaves `other` empty.
    DynamicArray(DynamicArray&& other) noexcept
        : data_(std::exchange(other.data_, nullptr))
        , size_(std::exchange(other.size_, 0))
        , capacity_(std::exchange(other.capacity_, 0)) {}

    /// @brief Copy assignment with the strong guarantee (copy-and-swap).
    DynamicArray& operator=(const DynamicArray& other)
        requires std::copy_constructible<T>
    {
        if (this != &other) {
            DynamicArray copy(other);
            swap(copy);
        }
        return *this;
    }

    /// @brief Move assignment. O(n) to destroy the old contents.
    DynamicArray& operator=(DynamicArray&& other) noexcept {
        if (this != &other) {
            DynamicArray tmp(std::move(other));
            swap(tmp);
        }
        return *this;
    }

    /// @brief Destroys all elements and releases storage.
    ~DynamicArray() { release(); }

    /**
     * @brief Constructs an element in place at the end.
     * @param args Constructor arguments (may alias existing elements).
     * @return Reference to the new element. Amortised O(1).
     */
    template <class... Args>
        requires std::constructible_from<T, Args&&...>
    T& emplace_back(Args&&... args) {
        if (size_ == capacity_) {
            return grow_and_emplace(std::forward<Args>(args)...);
        }
        T* p = std::construct_at(data_ + size_, std::forward<Args>(args)...);
        ++size_;
        return *p;
    }

    /**
     * @brief Appends a copy of `value`. Amortised O(1).
     * @param value Value to copy.
     */
    void push_back(const T& value)
        requires std::copy_constructible<T>
    {
        emplace_back(value);
    }

    /**
     * @brief Appends `value` by move. Amortised O(1).
     * @param value Value to move from.
     */
    void push_back(T&& value)
        requires std::move_constructible<T>
    {
        emplace_back(std::move(value));
    }

    /// @brief Removes the last element. Precondition: !empty(). O(1).
    void pop_back() noexcept {
        --size_;
        std::destroy_at(data_ + size_);
    }

    /**
     * @brief Removes the element at `pos`, shifting the tail left. O(n).
     * @param pos Iterator to the element to remove.
     * @return Iterator to the element that followed the removed one.
     */
    iterator erase(const_iterator pos)
        requires std::movable<T>
    {
        auto idx = static_cast<size_type>(pos - data_);
        std::move(data_ + idx + 1, data_ + size_, data_ + idx);
        pop_back();
        return data_ + idx;
    }

    /**
     * @brief Ensures capacity() >= new_capacity. O(n) when it reallocates.
     * @param new_capacity Required capacity.
     */
    void reserve(size_type new_capacity) {
        if (new_capacity > capacity_) {
            reallocate(new_capacity);
        }
    }

    /// @brief Releases unused capacity. O(n).
    void shrink_to_fit() {
        if (size_ < capacity_) {
            reallocate(size_);
        }
    }

    /// @brief Destroys all elements, keeping capacity. O(n).
    void clear() noexcept {
        std::destroy_n(data_, size_);
        size_ = 0;
    }

    /**
     * @brief Bounds-checked access.
     * @param i Index.
     * @return Reference to element i.
     * @throws std::out_of_range if i >= size().
     */
    [[nodiscard]] T& at(size_type i) {
        if (i >= size_) {
            throw std::out_of_range("DynamicArray::at");
        }
        return data_[i];
    }

    /// @copydoc at(size_type)
    [[nodiscard]] const T& at(size_type i) const {
        if (i >= size_) {
            throw std::out_of_range("DynamicArray::at");
        }
        return data_[i];
    }

    /// @brief Unchecked access. @param i Index (< size()). @return Element i.
    [[nodiscard]] T& operator[](size_type i) noexcept { return data_[i]; }
    /// @brief Unchecked access. @param i Index (< size()). @return Element i.
    [[nodiscard]] const T& operator[](size_type i) const noexcept { return data_[i]; }

    /// @brief First element (precondition: !empty()).
    [[nodiscard]] T& front() noexcept { return data_[0]; }
    /// @brief Last element (precondition: !empty()).
    [[nodiscard]] T& back() noexcept { return data_[size_ - 1]; }
    /// @brief Pointer to the contiguous storage.
    [[nodiscard]] T* data() noexcept { return data_; }
    /// @brief Pointer to the contiguous storage.
    [[nodiscard]] const T* data() const noexcept { return data_; }

    /// @brief Iterator to the first element.
    [[nodiscard]] iterator begin() noexcept { return data_; }
    /// @brief Iterator past the last element.
    [[nodiscard]] iterator end() noexcept { return data_ + size_; }
    /// @brief Const iterator to the first element.
    [[nodiscard]] const_iterator begin() const noexcept { return data_; }
    /// @brief Const iterator past the last element.
    [[nodiscard]] const_iterator end() const noexcept { return data_ + size_; }

    /// @brief Number of elements.
    [[nodiscard]] size_type size() const noexcept { return size_; }
    /// @brief Number of elements storable without reallocation.
    [[nodiscard]] size_type capacity() const noexcept { return capacity_; }
    /// @brief True when size() == 0.
    [[nodiscard]] bool empty() const noexcept { return size_ == 0; }

    /// @brief Swaps contents with `other`. O(1). @param other Array to swap with.
    void swap(DynamicArray& other) noexcept {
        std::swap(data_, other.data_);
        std::swap(size_, other.size_);
        std::swap(capacity_, other.capacity_);
    }

    /// @brief Element-wise equality. O(n).
    friend bool operator==(const DynamicArray& a, const DynamicArray& b)
        requires std::equality_comparable<T>
    {
        return std::equal(a.begin(), a.end(), b.begin(), b.end());
    }

private:
    static T* allocate(size_type n) { return n == 0 ? nullptr : std::allocator<T>{}.allocate(n); }
    static void deallocate(T* p, size_type n) noexcept {
        if (p != nullptr) {
            std::allocator<T>{}.deallocate(p, n);
        }
    }

    /// Transfers elements into fresh storage; strong guarantee.
    static void transfer(T* from, size_type n, T* to) {
        if constexpr (std::is_nothrow_move_constructible_v<T> || !std::is_copy_constructible_v<T>) {
            std::uninitialized_move_n(from, n, to);
        } else {
            std::uninitialized_copy_n(from, n, to);
        }
    }

    void reallocate(size_type new_capacity) {
        T* fresh = allocate(new_capacity);
        try {
            transfer(data_, size_, fresh);
        } catch (...) {
            deallocate(fresh, new_capacity);
            throw;
        }
        const size_type n = size_;
        release();
        data_ = fresh;
        size_ = n;
        capacity_ = new_capacity;
    }

    template <class... Args>
    T& grow_and_emplace(Args&&... args) {
        const size_type new_capacity = capacity_ == 0 ? 4 : capacity_ * 2;
        T* fresh = allocate(new_capacity);
        T* placed = nullptr;
        try {
            // Construct the new element first: `args` may refer to an element of *this.
            placed = std::construct_at(fresh + size_, std::forward<Args>(args)...);
            transfer(data_, size_, fresh);
        } catch (...) {
            if (placed != nullptr) {
                std::destroy_at(placed);
            }
            deallocate(fresh, new_capacity);
            throw;
        }
        const size_type n = size_;
        release();
        data_ = fresh;
        size_ = n + 1;
        capacity_ = new_capacity;
        return *placed;
    }

    void release() noexcept {
        std::destroy_n(data_, size_);
        deallocate(data_, capacity_);
        data_ = nullptr;
        size_ = 0;
        capacity_ = 0;
    }

    T* data_ = nullptr;
    size_type size_ = 0;
    size_type capacity_ = 0;
};

// ============================================================================================
// LinkedList
// ============================================================================================

/**
 * @class LinkedList
 * @brief Singly linked list with O(1) push_front/push_back/pop_front.
 *
 * Nodes are owned by a chain of `std::unique_ptr`; `tail_` is a non-owning observer. Destruction
 * and clear() are iterative (O(n) time, O(1) stack). reverse(): O(n). remove_if(): O(n).
 *
 * @tparam T Element type.
 */
template <class T>
class LinkedList {
    struct Node {
        T value;
        std::unique_ptr<Node> next;
        template <class... Args>
        explicit Node(std::in_place_t, Args&&... args) : value(std::forward<Args>(args)...) {}
    };

    template <bool Const>
    class Iterator {
    public:
        using iterator_concept = std::forward_iterator_tag;        ///< Iterator category.
        using iterator_category = std::forward_iterator_tag;       ///< Legacy category.
        using value_type = T;                                      ///< Element type.
        using difference_type = std::ptrdiff_t;                    ///< Distance type.
        using reference = std::conditional_t<Const, const T&, T&>; ///< Reference type.
        using pointer = std::conditional_t<Const, const T*, T*>;   ///< Pointer type.

        Iterator() noexcept = default;
        explicit Iterator(Node* node) noexcept : node_(node) {}
        /// Conversion from mutable to const iterator.
        template <bool C = Const>
            requires C
        Iterator(const Iterator<false>& other) noexcept
            : node_(other.node_) {} // NOLINT(google-explicit-constructor)

        reference operator*() const noexcept { return node_->value; }
        pointer operator->() const noexcept { return &node_->value; }
        Iterator& operator++() noexcept {
            node_ = node_->next.get();
            return *this;
        }
        Iterator operator++(int) noexcept {
            Iterator tmp = *this;
            ++*this;
            return tmp;
        }
        friend bool operator==(const Iterator& a, const Iterator& b) noexcept { return a.node_ == b.node_; }

    private:
        friend class LinkedList;
        template <bool>
        friend class Iterator;
        Node* node_ = nullptr;
    };

public:
    using value_type = T;                  ///< Element type.
    using iterator = Iterator<false>;      ///< Mutable forward iterator.
    using const_iterator = Iterator<true>; ///< Const forward iterator.

    /// @brief Creates an empty list.
    LinkedList() noexcept = default;

    /**
     * @brief Creates a list from `init`.
     * @param init Initial elements, in order.
     */
    LinkedList(std::initializer_list<T> init)
        requires std::copy_constructible<T>
    {
        for (const T& v : init) {
            push_back(v);
        }
    }

    /// @brief Deep copy. O(n).
    LinkedList(const LinkedList& other)
        requires std::copy_constructible<T>
    {
        for (const T& v : other) {
            push_back(v);
        }
    }

    /// @brief Takes ownership of `other`'s nodes. O(1).
    LinkedList(LinkedList&& other) noexcept
        : head_(std::move(other.head_))
        , tail_(std::exchange(other.tail_, nullptr))
        , size_(std::exchange(other.size_, 0)) {}

    /// @brief Copy assignment (copy-and-swap, strong guarantee).
    LinkedList& operator=(const LinkedList& other)
        requires std::copy_constructible<T>
    {
        if (this != &other) {
            LinkedList copy(other);
            swap(copy);
        }
        return *this;
    }

    /// @brief Move assignment.
    LinkedList& operator=(LinkedList&& other) noexcept {
        if (this != &other) {
            LinkedList tmp(std::move(other));
            swap(tmp);
        }
        return *this;
    }

    /// @brief Iteratively destroys every node.
    ~LinkedList() { clear(); }

    /**
     * @brief Constructs an element at the front. O(1).
     * @param args Constructor arguments.
     * @return Reference to the new element.
     */
    template <class... Args>
    T& emplace_front(Args&&... args) {
        auto node = std::make_unique<Node>(std::in_place, std::forward<Args>(args)...);
        node->next = std::move(head_);
        head_ = std::move(node);
        if (tail_ == nullptr) {
            tail_ = head_.get();
        }
        ++size_;
        return head_->value;
    }

    /**
     * @brief Constructs an element at the back. O(1).
     * @param args Constructor arguments.
     * @return Reference to the new element.
     */
    template <class... Args>
    T& emplace_back(Args&&... args) {
        auto node = std::make_unique<Node>(std::in_place, std::forward<Args>(args)...);
        Node* raw = node.get();
        if (tail_ == nullptr) {
            head_ = std::move(node);
        } else {
            tail_->next = std::move(node);
        }
        tail_ = raw;
        ++size_;
        return raw->value;
    }

    /// @brief Prepends a copy. O(1). @param value Value.
    void push_front(const T& value) { emplace_front(value); }
    /// @brief Prepends by move. O(1). @param value Value.
    void push_front(T&& value) { emplace_front(std::move(value)); }
    /// @brief Appends a copy. O(1). @param value Value.
    void push_back(const T& value) { emplace_back(value); }
    /// @brief Appends by move. O(1). @param value Value.
    void push_back(T&& value) { emplace_back(std::move(value)); }

    /**
     * @brief Removes the first element. O(1).
     * @throws std::out_of_range if the list is empty.
     */
    void pop_front() {
        if (!head_) {
            throw std::out_of_range("LinkedList::pop_front on empty list");
        }
        head_ = std::move(head_->next);
        if (!head_) {
            tail_ = nullptr;
        }
        --size_;
    }

    /// @brief First element. @throws std::out_of_range if empty.
    [[nodiscard]] T& front() {
        if (!head_) {
            throw std::out_of_range("LinkedList::front on empty list");
        }
        return head_->value;
    }

    /// @brief Last element. @throws std::out_of_range if empty.
    [[nodiscard]] T& back() {
        if (tail_ == nullptr) {
            throw std::out_of_range("LinkedList::back on empty list");
        }
        return tail_->value;
    }

    /// @brief Reverses the list in place by relinking. O(n), no allocation.
    void reverse() noexcept {
        std::unique_ptr<Node> prev;
        Node* new_tail = head_.get();
        while (head_) {
            std::unique_ptr<Node> next = std::move(head_->next);
            head_->next = std::move(prev);
            prev = std::move(head_);
            head_ = std::move(next);
        }
        head_ = std::move(prev);
        tail_ = new_tail;
    }

    /**
     * @brief Removes every element satisfying `pred`. O(n).
     * @param pred Unary predicate.
     * @return Number of removed elements.
     */
    template <std::predicate<const T&> Pred>
    std::size_t remove_if(Pred pred) {
        std::size_t removed = 0;
        std::unique_ptr<Node>* link = &head_;
        Node* last = nullptr;
        while (*link) {
            if (std::invoke(pred, std::as_const((*link)->value))) {
                *link = std::move((*link)->next);
                ++removed;
            } else {
                last = link->get();
                link = &(*link)->next;
            }
        }
        tail_ = last;
        size_ -= removed;
        return removed;
    }

    /// @brief Destroys all elements iteratively. O(n).
    void clear() noexcept {
        while (head_) {
            head_ = std::move(head_->next);
        }
        tail_ = nullptr;
        size_ = 0;
    }

    /// @brief Number of elements. O(1).
    [[nodiscard]] std::size_t size() const noexcept { return size_; }
    /// @brief True if empty.
    [[nodiscard]] bool empty() const noexcept { return size_ == 0; }

    /// @brief Iterator to the first element.
    [[nodiscard]] iterator begin() noexcept { return iterator(head_.get()); }
    /// @brief Past-the-end iterator.
    [[nodiscard]] iterator end() noexcept { return iterator(nullptr); }
    /// @brief Const iterator to the first element.
    [[nodiscard]] const_iterator begin() const noexcept { return const_iterator(head_.get()); }
    /// @brief Const past-the-end iterator.
    [[nodiscard]] const_iterator end() const noexcept { return const_iterator(nullptr); }

    /// @brief Swaps contents. O(1). @param other List to swap with.
    void swap(LinkedList& other) noexcept {
        std::swap(head_, other.head_);
        std::swap(tail_, other.tail_);
        std::swap(size_, other.size_);
    }

    /// @brief Element-wise equality. O(n).
    friend bool operator==(const LinkedList& a, const LinkedList& b)
        requires std::equality_comparable<T>
    {
        return a.size_ == b.size_ && std::equal(a.begin(), a.end(), b.begin());
    }

private:
    std::unique_ptr<Node> head_;
    Node* tail_ = nullptr; // non-owning
    std::size_t size_ = 0;
};

// ============================================================================================
// BinarySearchTree
// ============================================================================================

/**
 * @class BinarySearchTree
 * @brief Unbalanced binary search tree implementing an ordered set.
 *
 * insert/contains/erase: O(h) where h is the height: O(log n) expected for random insertion order,
 * O(n) worst case (sorted input). Every operation (including copy and destruction) is iterative,
 * so degenerate trees cannot overflow the call stack. Space: O(n).
 *
 * @tparam K       Key type.
 * @tparam Compare Strict weak ordering on keys.
 */
template <class K, class Compare = std::less<K>>
    requires std::strict_weak_order<Compare&, const K&, const K&>
class BinarySearchTree {
    struct Node {
        K key;
        std::unique_ptr<Node> left;
        std::unique_ptr<Node> right;
        explicit Node(K k) : key(std::move(k)) {}
    };

public:
    /**
     * @brief Creates an empty tree.
     * @param comp Key ordering.
     */
    explicit BinarySearchTree(Compare comp = Compare{}) : comp_(std::move(comp)) {}

    /// @brief Deep copy (iterative). O(n).
    BinarySearchTree(const BinarySearchTree& other)
        requires std::copy_constructible<K>
        : comp_(other.comp_), size_(other.size_) {
        if (!other.root_) {
            return;
        }
        root_ = std::make_unique<Node>(other.root_->key);
        std::vector<std::pair<const Node*, Node*>> stack{{other.root_.get(), root_.get()}};
        while (!stack.empty()) {
            auto [src, dst] = stack.back();
            stack.pop_back();
            if (src->left) {
                dst->left = std::make_unique<Node>(src->left->key);
                stack.emplace_back(src->left.get(), dst->left.get());
            }
            if (src->right) {
                dst->right = std::make_unique<Node>(src->right->key);
                stack.emplace_back(src->right.get(), dst->right.get());
            }
        }
    }

    /// @brief Takes ownership of `other`'s nodes. O(1).
    BinarySearchTree(BinarySearchTree&& other) noexcept(std::is_nothrow_move_constructible_v<Compare>)
        : root_(std::move(other.root_))
        , comp_(std::move(other.comp_))
        , size_(std::exchange(other.size_, 0)) {}

    /// @brief Copy assignment (copy-and-swap).
    BinarySearchTree& operator=(const BinarySearchTree& other)
        requires std::copy_constructible<K>
    {
        if (this != &other) {
            BinarySearchTree copy(other);
            swap(copy);
        }
        return *this;
    }

    /// @brief Move assignment.
    BinarySearchTree& operator=(BinarySearchTree&& other) noexcept(std::is_nothrow_swappable_v<Compare>) {
        if (this != &other) {
            clear();
            swap(other);
        }
        return *this;
    }

    /// @brief Iterative destruction.
    ~BinarySearchTree() { clear(); }

    /**
     * @brief Inserts `key` if absent. O(h).
     * @param key Key to insert.
     * @return true if inserted, false if an equivalent key was present.
     */
    bool insert(K key) {
        std::unique_ptr<Node>* link = &root_;
        while (*link) {
            if (comp_(key, (*link)->key)) {
                link = &(*link)->left;
            } else if (comp_((*link)->key, key)) {
                link = &(*link)->right;
            } else {
                return false;
            }
        }
        *link = std::make_unique<Node>(std::move(key));
        ++size_;
        return true;
    }

    /**
     * @brief Membership test. O(h).
     * @param key Key to look up.
     * @return true if present.
     */
    [[nodiscard]] bool contains(const K& key) const { return find_node(key) != nullptr; }

    /**
     * @brief Removes `key` if present (Hibbard deletion using the in-order successor). O(h).
     * @param key Key to remove.
     * @return true if a key was removed.
     */
    bool erase(const K& key) {
        std::unique_ptr<Node>* link = &root_;
        while (*link) {
            if (comp_(key, (*link)->key)) {
                link = &(*link)->left;
            } else if (comp_((*link)->key, key)) {
                link = &(*link)->right;
            } else {
                break;
            }
        }
        if (!*link) {
            return false;
        }
        Node* node = link->get();
        if (!node->left) {
            *link = std::move(node->right);
        } else if (!node->right) {
            *link = std::move(node->left);
        } else {
            std::unique_ptr<Node>* succ = &node->right;
            while ((*succ)->left) {
                succ = &(*succ)->left;
            }
            node->key = std::move((*succ)->key);
            *succ = std::move((*succ)->right);
        }
        --size_;
        return true;
    }

    /**
     * @brief Smallest key. O(h).
     * @return Reference to the minimum key.
     * @throws std::out_of_range if empty.
     */
    [[nodiscard]] const K& min() const {
        if (!root_) {
            throw std::out_of_range("BinarySearchTree::min on empty tree");
        }
        const Node* n = root_.get();
        while (n->left) {
            n = n->left.get();
        }
        return n->key;
    }

    /**
     * @brief Largest key. O(h).
     * @return Reference to the maximum key.
     * @throws std::out_of_range if empty.
     */
    [[nodiscard]] const K& max() const {
        if (!root_) {
            throw std::out_of_range("BinarySearchTree::max on empty tree");
        }
        const Node* n = root_.get();
        while (n->right) {
            n = n->right.get();
        }
        return n->key;
    }

    /**
     * @brief Lowest common ancestor of two keys (the split point of their search paths). O(h).
     * @param a First key.
     * @param b Second key.
     * @return The LCA key, or std::nullopt if either key is absent.
     */
    [[nodiscard]] std::optional<K> lowest_common_ancestor(const K& a, const K& b) const
        requires std::copy_constructible<K>
    {
        if (!contains(a) || !contains(b)) {
            return std::nullopt;
        }
        const Node* n = root_.get();
        while (n != nullptr) {
            if (comp_(a, n->key) && comp_(b, n->key)) {
                n = n->left.get();
            } else if (comp_(n->key, a) && comp_(n->key, b)) {
                n = n->right.get();
            } else {
                return n->key;
            }
        }
        return std::nullopt;
    }

    /**
     * @brief Keys in sorted order (iterative in-order traversal). O(n).
     * @return Sorted copy of all keys.
     */
    [[nodiscard]] std::vector<K> in_order() const
        requires std::copy_constructible<K>
    {
        std::vector<K> out;
        out.reserve(size_);
        std::vector<const Node*> stack;
        const Node* n = root_.get();
        while (n != nullptr || !stack.empty()) {
            while (n != nullptr) {
                stack.push_back(n);
                n = n->left.get();
            }
            n = stack.back();
            stack.pop_back();
            out.push_back(n->key);
            n = n->right.get();
        }
        return out;
    }

    /**
     * @brief Height in nodes (empty tree = 0, single node = 1). O(n) breadth-first.
     * @return Tree height.
     */
    [[nodiscard]] std::size_t height() const {
        std::size_t h = 0;
        std::vector<const Node*> level;
        if (root_) {
            level.push_back(root_.get());
        }
        while (!level.empty()) {
            ++h;
            std::vector<const Node*> next;
            for (const Node* n : level) {
                if (n->left) {
                    next.push_back(n->left.get());
                }
                if (n->right) {
                    next.push_back(n->right.get());
                }
            }
            level.swap(next);
        }
        return h;
    }

    /// @brief Number of keys.
    [[nodiscard]] std::size_t size() const noexcept { return size_; }
    /// @brief True if empty.
    [[nodiscard]] bool empty() const noexcept { return size_ == 0; }

    /// @brief Removes all keys using rotations (O(n) time, O(1) extra space, no recursion).
    void clear() noexcept {
        while (root_) {
            if (root_->left) {
                std::unique_ptr<Node> l = std::move(root_->left);
                root_->left = std::move(l->right);
                l->right = std::move(root_);
                root_ = std::move(l);
            } else {
                root_ = std::move(root_->right);
            }
        }
        size_ = 0;
    }

    /// @brief Swaps contents. @param other Tree to swap with.
    void swap(BinarySearchTree& other) noexcept(std::is_nothrow_swappable_v<Compare>) {
        using std::swap;
        swap(root_, other.root_);
        swap(comp_, other.comp_);
        swap(size_, other.size_);
    }

private:
    const Node* find_node(const K& key) const {
        const Node* n = root_.get();
        while (n != nullptr) {
            if (comp_(key, n->key)) {
                n = n->left.get();
            } else if (comp_(n->key, key)) {
                n = n->right.get();
            } else {
                return n;
            }
        }
        return nullptr;
    }

    std::unique_ptr<Node> root_;
    Compare comp_;
    std::size_t size_ = 0;
};

// ============================================================================================
// MinHeap
// ============================================================================================

/**
 * @class MinHeap
 * @brief Binary heap priority queue; `top()` is the minimum under `Compare`.
 *
 * push/pop: O(log n). top: O(1). Construction from a vector: O(n) (Floyd's bottom-up heapify).
 * Space: O(n).
 *
 * @tparam T       Element type.
 * @tparam Compare Strict weak ordering (std::less gives a min-heap, std::greater a max-heap).
 */
template <class T, class Compare = std::less<T>>
    requires std::strict_weak_order<Compare&, const T&, const T&>
class MinHeap {
public:
    /**
     * @brief Creates an empty heap.
     * @param comp Ordering.
     */
    explicit MinHeap(Compare comp = Compare{}) : comp_(std::move(comp)) {}

    /**
     * @brief Heapifies `values` in O(n).
     * @param values Initial elements.
     * @param comp   Ordering.
     */
    explicit MinHeap(std::vector<T> values, Compare comp = Compare{})
        : data_(std::move(values)), comp_(std::move(comp)) {
        for (std::size_t i = data_.size() / 2; i-- > 0;) {
            sift_down(i);
        }
    }

    /**
     * @brief Inserts a value. O(log n).
     * @param value Value to insert.
     */
    void push(T value) {
        data_.push_back(std::move(value));
        sift_up(data_.size() - 1);
    }

    /**
     * @brief Smallest element. O(1).
     * @return Reference to the top.
     * @throws std::out_of_range if empty.
     */
    [[nodiscard]] const T& top() const {
        if (data_.empty()) {
            throw std::out_of_range("MinHeap::top on empty heap");
        }
        return data_.front();
    }

    /**
     * @brief Removes and returns the smallest element. O(log n).
     * @return The former top.
     * @throws std::out_of_range if empty.
     */
    T pop() {
        if (data_.empty()) {
            throw std::out_of_range("MinHeap::pop on empty heap");
        }
        T result = std::move(data_.front());
        if (data_.size() > 1) {
            data_.front() = std::move(data_.back());
        }
        data_.pop_back();
        if (!data_.empty()) {
            sift_down(0);
        }
        return result;
    }

    /// @brief Number of elements.
    [[nodiscard]] std::size_t size() const noexcept { return data_.size(); }
    /// @brief True if empty.
    [[nodiscard]] bool empty() const noexcept { return data_.empty(); }

    /**
     * @brief Checks the heap invariant (for testing). O(n).
     * @return true if no child compares less than its parent.
     */
    [[nodiscard]] bool is_valid_heap() const {
        for (std::size_t i = 1; i < data_.size(); ++i) {
            if (comp_(data_[i], data_[(i - 1) / 2])) {
                return false;
            }
        }
        return true;
    }

private:
    void sift_up(std::size_t i) {
        while (i > 0) {
            const std::size_t parent = (i - 1) / 2;
            if (!comp_(data_[i], data_[parent])) {
                break;
            }
            std::swap(data_[i], data_[parent]);
            i = parent;
        }
    }

    void sift_down(std::size_t i) {
        const std::size_t n = data_.size();
        while (true) {
            std::size_t smallest = i;
            const std::size_t l = 2 * i + 1;
            const std::size_t r = l + 1;
            if (l < n && comp_(data_[l], data_[smallest])) {
                smallest = l;
            }
            if (r < n && comp_(data_[r], data_[smallest])) {
                smallest = r;
            }
            if (smallest == i) {
                return;
            }
            std::swap(data_[i], data_[smallest]);
            i = smallest;
        }
    }

    std::vector<T> data_;
    Compare comp_;
};

// ============================================================================================
// HashTable
// ============================================================================================

/**
 * @class HashTable
 * @brief Open-addressing hash map with linear probing and backward-shift deletion.
 *
 * Capacity is a power of two; the table grows x2 when the load factor would exceed 0.7. The
 * user hash is post-mixed with the SplitMix64 finaliser, so identity hashes (std::hash<int>) do
 * not cluster. Backward-shift deletion keeps probe sequences contiguous without tombstones.
 *
 * insert/find/erase: expected O(1) (expected probe length ~ (1 + 1/(1-a)^2)/2 at load a), worst
 * O(n). Space: O(capacity).
 *
 * @tparam K        Key type.
 * @tparam V        Mapped type.
 * @tparam Hash     Hash function object.
 * @tparam KeyEqual Equality predicate.
 */
template <class K, class V, class Hash = std::hash<K>, class KeyEqual = std::equal_to<K>>
    requires std::regular_invocable<const Hash&, const K&> &&
             std::equivalence_relation<const KeyEqual&, const K&, const K&>
class HashTable {
public:
    /// @brief Maximum load factor before growth.
    static constexpr double kMaxLoad = 0.7;

    /**
     * @brief Creates a table with room for at least `expected` entries.
     * @param expected Expected number of entries.
     */
    explicit HashTable(std::size_t expected = 0) { rehash_for(expected); }

    /**
     * @brief Inserts or overwrites the mapping for `key`. Expected O(1).
     * @param key   Key.
     * @param value Mapped value.
     * @return true if a new key was inserted, false if an existing value was replaced.
     */
    bool insert_or_assign(K key, V value) {
        if (static_cast<double>(size_ + 1) > kMaxLoad * static_cast<double>(slots_.size())) {
            rehash_for(size_ + 1);
        }
        std::size_t i = home(key);
        while (slots_[i]) {
            if (eq_(slots_[i]->first, key)) {
                slots_[i]->second = std::move(value);
                return false;
            }
            i = (i + 1) & mask();
        }
        slots_[i].emplace(std::move(key), std::move(value));
        ++size_;
        return true;
    }

    /**
     * @brief Looks up `key`. Expected O(1).
     * @param key Key to find.
     * @return Pointer to the mapped value (non-owning), or nullptr if absent.
     */
    [[nodiscard]] V* find(const K& key) {
        const auto i = locate(key);
        return i ? &slots_[*i]->second : nullptr;
    }

    /// @copydoc find(const K&)
    [[nodiscard]] const V* find(const K& key) const {
        const auto i = locate(key);
        return i ? &slots_[*i]->second : nullptr;
    }

    /**
     * @brief Bounds-checked lookup.
     * @param key Key to find.
     * @return Reference to the mapped value.
     * @throws std::out_of_range if absent.
     */
    [[nodiscard]] const V& at(const K& key) const {
        const V* v = find(key);
        if (v == nullptr) {
            throw std::out_of_range("HashTable::at: key not found");
        }
        return *v;
    }

    /// @brief Membership test. Expected O(1). @param key Key. @return true if present.
    [[nodiscard]] bool contains(const K& key) const { return locate(key).has_value(); }

    /**
     * @brief Removes `key` using backward-shift deletion. Expected O(1).
     * @param key Key to remove.
     * @return true if the key was present.
     */
    bool erase(const K& key) {
        const auto found = locate(key);
        if (!found) {
            return false;
        }
        std::size_t hole = *found;
        slots_[hole].reset();
        std::size_t j = (hole + 1) & mask();
        while (slots_[j]) {
            const std::size_t h = home(slots_[j]->first);
            // Move slot j into the hole if the hole lies on j's probe path [h, j].
            if (((j - h) & mask()) >= ((j - hole) & mask())) {
                slots_[hole] = std::move(slots_[j]);
                slots_[j].reset();
                hole = j;
            }
            j = (j + 1) & mask();
        }
        --size_;
        return true;
    }

    /**
     * @brief Visits every entry (unspecified order). O(capacity).
     * @param f Callable invoked as f(const K&, const V&).
     */
    template <std::invocable<const K&, const V&> F>
    void for_each(F f) const {
        for (const auto& slot : slots_) {
            if (slot) {
                std::invoke(f, slot->first, slot->second);
            }
        }
    }

    /**
     * @brief Longest probe distance of any stored key (diagnostic). O(capacity).
     * @return Maximum displacement from the home slot.
     */
    [[nodiscard]] std::size_t max_probe_distance() const {
        std::size_t worst = 0;
        for (std::size_t i = 0; i < slots_.size(); ++i) {
            if (slots_[i]) {
                worst = std::max(worst, (i - home(slots_[i]->first)) & mask());
            }
        }
        return worst;
    }

    /// @brief Number of entries.
    [[nodiscard]] std::size_t size() const noexcept { return size_; }
    /// @brief True if empty.
    [[nodiscard]] bool empty() const noexcept { return size_ == 0; }
    /// @brief Number of slots.
    [[nodiscard]] std::size_t capacity() const noexcept { return slots_.size(); }
    /// @brief size() / capacity().
    [[nodiscard]] double load_factor() const noexcept {
        return static_cast<double>(size_) / static_cast<double>(slots_.size());
    }

    /// @brief Removes all entries, keeping capacity.
    void clear() noexcept {
        for (auto& s : slots_) {
            s.reset();
        }
        size_ = 0;
    }

private:
    static constexpr std::uint64_t mix(std::uint64_t x) noexcept {
        x ^= x >> 30;
        x *= 0xbf58476d1ce4e5b9ULL;
        x ^= x >> 27;
        x *= 0x94d049bb133111ebULL;
        x ^= x >> 31;
        return x;
    }

    [[nodiscard]] std::size_t mask() const noexcept { return slots_.size() - 1; }

    [[nodiscard]] std::size_t home(const K& key) const {
        return static_cast<std::size_t>(mix(static_cast<std::uint64_t>(hash_(key)))) & mask();
    }

    [[nodiscard]] std::optional<std::size_t> locate(const K& key) const {
        std::size_t i = home(key);
        while (slots_[i]) {
            if (eq_(slots_[i]->first, key)) {
                return i;
            }
            i = (i + 1) & mask();
        }
        return std::nullopt;
    }

    void rehash_for(std::size_t entries) {
        std::size_t cap = 8;
        while (static_cast<double>(entries) > kMaxLoad * static_cast<double>(cap)) {
            cap *= 2;
        }
        if (cap <= slots_.size()) {
            return;
        }
        std::vector<std::optional<std::pair<K, V>>> old(cap);
        old.swap(slots_);
        size_ = 0;
        for (auto& slot : old) {
            if (slot) {
                std::size_t i = home(slot->first);
                while (slots_[i]) {
                    i = (i + 1) & mask();
                }
                slots_[i] = std::move(slot);
                ++size_;
            }
        }
    }

    std::vector<std::optional<std::pair<K, V>>> slots_;
    std::size_t size_ = 0;
    Hash hash_{};
    KeyEqual eq_{};
};

// ============================================================================================
// Trie
// ============================================================================================

/**
 * @class Trie
 * @brief Prefix tree over byte strings with lexicographically ordered children.
 *
 * insert/contains/erase/starts_with: O(L log sigma) for a word of length L (std::map children).
 * with_prefix: O(P + output size). Space: O(total characters).
 */
class Trie {
public:
    /// @brief Creates an empty trie.
    Trie();
    /// @brief Destroys the trie.
    ~Trie();
    /// @brief Deep copy.
    Trie(const Trie& other);
    /// @brief Copy assignment.
    Trie& operator=(const Trie& other);
    /// @brief Move construction.
    Trie(Trie&& other) noexcept;
    /// @brief Move assignment.
    Trie& operator=(Trie&& other) noexcept;

    /**
     * @brief Inserts a word.
     * @param word Word to insert.
     * @return true if the word was not present before.
     */
    bool insert(std::string_view word);

    /**
     * @brief Exact membership test.
     * @param word Word to look up.
     * @return true if `word` was inserted.
     */
    [[nodiscard]] bool contains(std::string_view word) const;

    /**
     * @brief Prefix test.
     * @param prefix Prefix to look up.
     * @return true if some stored word starts with `prefix`.
     */
    [[nodiscard]] bool starts_with(std::string_view prefix) const;

    /**
     * @brief Words with the given prefix, in lexicographic order.
     * @param prefix Prefix to complete.
     * @param limit  Maximum number of results.
     * @return Matching words (sorted).
     */
    [[nodiscard]] std::vector<std::string> with_prefix(
        std::string_view prefix, std::size_t limit = static_cast<std::size_t>(-1)) const;

    /**
     * @brief Removes a word, pruning nodes that no longer lead to any word.
     * @param word Word to remove.
     * @return true if the word was present.
     */
    bool erase(std::string_view word);

    /// @brief Number of stored words.
    [[nodiscard]] std::size_t size() const noexcept { return size_; }
    /// @brief True if no words are stored.
    [[nodiscard]] bool empty() const noexcept { return size_ == 0; }
    /// @brief Number of trie nodes including the root.
    [[nodiscard]] std::size_t node_count() const noexcept;

private:
    struct Node;
    std::unique_ptr<Node> root_;
    std::size_t size_ = 0;
};

// ============================================================================================
// DisjointSet
// ============================================================================================

/**
 * @class DisjointSet
 * @brief Union-find over {0, ..., n-1} with union by size and path halving.
 *
 * find/unite/connected: amortised O(alpha(n)) (inverse Ackermann, < 5 for any practical n).
 * Space: O(n).
 */
class DisjointSet {
public:
    /**
     * @brief Creates n singleton sets.
     * @param n Number of elements.
     */
    explicit DisjointSet(std::size_t n);

    /**
     * @brief Representative of x's set (compresses the path by halving).
     * @param x Element (< size()).
     * @return The set representative.
     * @throws std::out_of_range if x >= size().
     */
    [[nodiscard]] std::size_t find(std::size_t x);

    /**
     * @brief Merges the sets containing a and b.
     * @param a First element.
     * @param b Second element.
     * @return true if they were in different sets.
     */
    bool unite(std::size_t a, std::size_t b);

    /**
     * @brief Same-set test.
     * @param a First element.
     * @param b Second element.
     * @return true if a and b are in the same set.
     */
    [[nodiscard]] bool connected(std::size_t a, std::size_t b);

    /**
     * @brief Size of x's set.
     * @param x Element.
     * @return Number of elements in the set containing x.
     */
    [[nodiscard]] std::size_t set_size(std::size_t x);

    /// @brief Number of disjoint sets.
    [[nodiscard]] std::size_t set_count() const noexcept { return sets_; }
    /// @brief Number of elements.
    [[nodiscard]] std::size_t size() const noexcept { return parent_.size(); }

private:
    std::vector<std::size_t> parent_;
    std::vector<std::size_t> size_;
    std::size_t sets_;
};

// ============================================================================================
// BloomFilter
// ============================================================================================

/**
 * @class BloomFilter
 * @brief Space-efficient probabilistic set: no false negatives, tunable false-positive rate.
 *
 * Sized optimally: m = ceil(-n ln p / (ln 2)^2) bits and k = round((m / n) ln 2) hash functions.
 * The k probe positions are derived from two 64-bit hashes (FNV-1a and its SplitMix64 remix) via
 * Kirsch-Mitzenmacher double hashing g_i = h1 + i * h2. insert / might_contain: O(k).
 */
class BloomFilter {
public:
    /**
     * @brief Creates an empty filter.
     * @param expected_elements   Planned number of insertions n (>= 1).
     * @param false_positive_rate Target false-positive probability p in (0, 1).
     * @throws std::invalid_argument for p outside (0, 1).
     */
    explicit BloomFilter(std::size_t expected_elements, double false_positive_rate = 0.01);

    /**
     * @brief Adds an element.
     * @param element Element to add.
     */
    void insert(std::string_view element);

    /**
     * @brief Probabilistic membership.
     * @param element Element to test.
     * @return false if definitely absent; true if possibly present.
     */
    [[nodiscard]] bool might_contain(std::string_view element) const;

    /// @brief Removes all elements.
    void clear() noexcept;

    /// @brief Number of bits m.
    [[nodiscard]] std::size_t bit_count() const noexcept { return bits_; }
    /// @brief Number of hash functions k.
    [[nodiscard]] std::size_t hash_count() const noexcept { return hashes_; }
    /// @brief Number of insert() calls.
    [[nodiscard]] std::size_t inserted() const noexcept { return inserted_; }

    /**
     * @brief Theoretical false-positive rate at the current fill: (1 - e^(-k n / m))^k.
     * @return Estimated probability.
     */
    [[nodiscard]] double estimated_false_positive_rate() const;

private:
    std::vector<std::uint64_t> words_;
    std::size_t bits_;
    std::size_t hashes_;
    std::size_t inserted_ = 0;
};

// ============================================================================================
// SkipList
// ============================================================================================

/**
 * @class SkipList
 * @brief Randomised ordered set (Pugh 1990) with geometric (p = 1/2) tower heights.
 *
 * insert/contains/erase: O(log n) expected, O(n) worst. Space: O(n) expected (2n pointers).
 * Level-0 links own the nodes through `std::unique_ptr`; higher levels are non-owning express lanes.
 * The RNG is seeded explicitly, so structure (and thus performance) is reproducible.
 *
 * @tparam K       Key type.
 * @tparam Compare Strict weak ordering.
 */
template <class K, class Compare = std::less<K>>
    requires std::strict_weak_order<Compare&, const K&, const K&>
class SkipList {
    static constexpr std::size_t kMaxLevel = 32;

    struct Node {
        K key;
        std::vector<Node*> forward;  // non-owning, one per level
        std::unique_ptr<Node> next0; // owning level-0 successor (== forward[0])
        Node(K k, std::size_t levels) : key(std::move(k)), forward(levels, nullptr) {}
    };

    struct Search {
        std::array<std::vector<Node*>*, kMaxLevel> update{};
        std::unique_ptr<Node>* owner = nullptr;
    };

public:
    /**
     * @brief Creates an empty skip list.
     * @param seed Seed for tower-height randomisation.
     * @param comp Key ordering.
     */
    explicit SkipList(std::uint64_t seed = 0x5eed, Compare comp = Compare{})
        : head_(kMaxLevel, nullptr), rng_(seed), comp_(std::move(comp)) {}

    /// @brief Deep copy (re-inserts every key; same seed). O(n log n) expected.
    SkipList(const SkipList& other)
        requires std::copy_constructible<K>
        : head_(kMaxLevel, nullptr), rng_(other.rng_), comp_(other.comp_) {
        for (const Node* n = other.first_.get(); n != nullptr; n = n->next0.get()) {
            insert(n->key);
        }
    }

    /// @brief Move construction; leaves `other` empty but usable.
    SkipList(SkipList&& other) noexcept : head_(kMaxLevel, nullptr), rng_(other.rng_), comp_(other.comp_) {
        swap(other);
    }

    /// @brief Copy assignment (copy-and-swap).
    SkipList& operator=(const SkipList& other)
        requires std::copy_constructible<K>
    {
        if (this != &other) {
            SkipList copy(other);
            swap(copy);
        }
        return *this;
    }

    /// @brief Move assignment.
    SkipList& operator=(SkipList&& other) noexcept {
        if (this != &other) {
            clear();
            swap(other);
        }
        return *this;
    }

    /// @brief Iterative destruction along level 0.
    ~SkipList() { clear(); }

    /**
     * @brief Inserts `key` if absent. O(log n) expected.
     * @param key Key to insert.
     * @return true if inserted.
     */
    bool insert(K key) {
        Search s = search(key);
        Node* candidate = (*s.update[0])[0];
        if (candidate != nullptr && !comp_(key, candidate->key)) {
            return false; // equivalent key present
        }
        const std::size_t lvl = random_level();
        for (std::size_t l = level_; l < lvl; ++l) {
            s.update[l] = &head_;
        }
        level_ = std::max(level_, lvl);
        auto node = std::make_unique<Node>(std::move(key), lvl);
        Node* raw = node.get();
        for (std::size_t l = 0; l < lvl; ++l) {
            raw->forward[l] = (*s.update[l])[l];
            (*s.update[l])[l] = raw;
        }
        raw->next0 = std::move(*s.owner);
        *s.owner = std::move(node);
        ++size_;
        return true;
    }

    /**
     * @brief Membership test. O(log n) expected.
     * @param key Key to find.
     * @return true if present.
     */
    [[nodiscard]] bool contains(const K& key) const {
        const std::vector<Node*>* fwd = &head_;
        for (std::size_t l = level_; l-- > 0;) {
            while ((*fwd)[l] != nullptr && comp_((*fwd)[l]->key, key)) {
                fwd = &(*fwd)[l]->forward;
            }
        }
        const Node* c = (*fwd)[0];
        return c != nullptr && !comp_(key, c->key);
    }

    /**
     * @brief Removes `key` if present. O(log n) expected.
     * @param key Key to remove.
     * @return true if removed.
     */
    bool erase(const K& key) {
        Search s = search(key);
        Node* target = (*s.update[0])[0];
        if (target == nullptr || comp_(key, target->key)) {
            return false;
        }
        for (std::size_t l = 0; l < level_; ++l) {
            if ((*s.update[l])[l] == target) {
                (*s.update[l])[l] = target->forward[l];
            }
        }
        std::unique_ptr<Node> dead = std::move(*s.owner);
        *s.owner = std::move(dead->next0);
        while (level_ > 1 && head_[level_ - 1] == nullptr) {
            --level_;
        }
        --size_;
        return true;
    }

    /**
     * @brief Keys in order. O(n).
     * @return Sorted copy of all keys.
     */
    [[nodiscard]] std::vector<K> to_vector() const
        requires std::copy_constructible<K>
    {
        std::vector<K> out;
        out.reserve(size_);
        for (const Node* n = first_.get(); n != nullptr; n = n->next0.get()) {
            out.push_back(n->key);
        }
        return out;
    }

    /// @brief Removes all keys. O(n), iterative.
    void clear() noexcept {
        while (first_) {
            first_ = std::move(first_->next0);
        }
        std::fill(head_.begin(), head_.end(), nullptr);
        level_ = 1;
        size_ = 0;
    }

    /// @brief Number of keys.
    [[nodiscard]] std::size_t size() const noexcept { return size_; }
    /// @brief True if empty.
    [[nodiscard]] bool empty() const noexcept { return size_ == 0; }
    /// @brief Current number of levels in use.
    [[nodiscard]] std::size_t levels() const noexcept { return level_; }

    /// @brief Swaps contents. @param other List to swap with.
    void swap(SkipList& other) noexcept {
        using std::swap;
        swap(head_, other.head_);
        swap(first_, other.first_);
        swap(level_, other.level_);
        swap(size_, other.size_);
        swap(rng_, other.rng_);
        swap(comp_, other.comp_);
    }

private:
    Search search(const K& key) {
        Search s;
        s.owner = &first_;
        std::vector<Node*>* fwd = &head_;
        for (std::size_t l = level_; l-- > 0;) {
            while ((*fwd)[l] != nullptr && comp_((*fwd)[l]->key, key)) {
                Node* n = (*fwd)[l];
                s.owner = &n->next0;
                fwd = &n->forward;
            }
            s.update[l] = fwd;
        }
        return s;
    }

    std::size_t random_level() {
        const auto bits = static_cast<std::uint64_t>(rng_());
        return std::min<std::size_t>(kMaxLevel, 1 + static_cast<std::size_t>(std::countr_one(bits)));
    }

    std::vector<Node*> head_;
    std::unique_ptr<Node> first_;
    std::size_t level_ = 1;
    std::size_t size_ = 0;
    std::mt19937_64 rng_;
    Compare comp_;
};

/**
 * @brief Showcase of every data structure in this header.
 * @param out Stream to write to.
 */
void demonstrate_data_structures(std::ostream& out = std::cout);

} // namespace CppVerseHub::Algorithms

#endif // CPPVERSEHUB_ALGORITHMS_DATASTRUCTURES_HPP
