/**
 * @file DataStructures.hpp
 * @brief Custom data structure implementations for educational purposes
 * @details File location: src/algorithms/DataStructures.hpp
 * 
 * This file contains implementations of fundamental data structures including
 * trees, heaps, hash tables, and specialized containers optimized for
 * different use cases and performance characteristics.
 */

#ifndef DATASTRUCTURES_HPP
#define DATASTRUCTURES_HPP

#include <vector>
#include <memory>
#include <functional>
#include <iostream>
#include <string>
#include <optional>
#include <algorithm>
#include <iterator>
#include <stdexcept>
#include <chrono>

namespace CppVerseHub::Algorithms {

    /**
     * @class DynamicArray
     * @brief Dynamic array with automatic resizing and performance tracking
     */
    template<typename T>
    class DynamicArray {
    public:
        explicit DynamicArray(size_t initial_capacity = 16);
        ~DynamicArray();
        
        // Copy constructor and assignment
        DynamicArray(const DynamicArray& other);
        DynamicArray& operator=(const DynamicArray& other);
        
        // Move constructor and assignment
        DynamicArray(DynamicArray&& other) noexcept;
        DynamicArray& operator=(DynamicArray&& other) noexcept;
        
        // Element access
        T& operator[](size_t index);
        const T& operator[](size_t index) const;
        T& at(size_t index);
        const T& at(size_t index) const;
        
        T& front();
        const T& front() const;
        T& back();
        const T& back() const;
        
        // Capacity
        size_t size() const { return size_; }
        size_t capacity() const { return capacity_; }
        bool empty() const { return size_ == 0; }
        
        // Modifiers
        void push_back(const T& value);
        void push_back(T&& value);
        void pop_back();
        void insert(size_t index, const T& value);
        void erase(size_t index);
        void clear();
        void resize(size_t new_size);
        void reserve(size_t new_capacity);
        
        // Iterator support
        class iterator;
        class const_iterator;
        
        iterator begin();
        iterator end();
        const_iterator begin() const;
        const_iterator end() const;
        const_iterator cbegin() const;
        const_iterator cend() const;
        
        // Performance metrics
        size_t get_resize_count() const { return resize_count_; }
        double get_load_factor() const { return static_cast<double>(size_) / capacity_; }
        
        void print_debug_info() const;

    private:
        T* data_;
        size_t size_;
        size_t capacity_;
        size_t resize_count_;
        
        void resize_if_needed();
        void grow_capacity();
    };

    /**
     * @class LinkedList
     * @brief Doubly linked list implementation
     */
    template<typename T>
    class LinkedList {
    public:
        struct Node {
            T data;
            std::shared_ptr<Node> next;
            std::weak_ptr<Node> prev;
            
            explicit Node(const T& value) : data(value) {}
        };
        
        using NodePtr = std::shared_ptr<Node>;

        LinkedList();
        ~LinkedList() = default;
        
        // Copy constructor and assignment
        LinkedList(const LinkedList& other);
        LinkedList& operator=(const LinkedList& other);
        
        // Move constructor and assignment
        LinkedList(LinkedList&& other) noexcept;
        LinkedList& operator=(LinkedList&& other) noexcept;
        
        // Element access
        T& front();
        const T& front() const;
        T& back();
        const T& back() const;
        
        // Capacity
        size_t size() const { return size_; }
        bool empty() const { return size_ == 0; }
        
        // Modifiers
        void push_front(const T& value);
        void push_back(const T& value);
        void pop_front();
        void pop_back();
        void insert(size_t index, const T& value);
        void erase(size_t index);
        void clear();
        
        // Operations
        void reverse();
        void sort();
        void merge(LinkedList& other);
        size_t count(const T& value) const;
        
        // Iterator support
        class iterator;
        class const_iterator;
        
        iterator begin();
        iterator end();
        const_iterator begin() const;
        const_iterator end() const;
        
        void print() const;

    private:
        NodePtr head_;
        NodePtr tail_;
        size_t size_;
        
        NodePtr get_node_at(size_t index) const;
    };

    /**
     * @class BinarySearchTree
     * @brief Self-balancing binary search tree (AVL Tree)
     */
    template<typename T>
    class BinarySearchTree {
    public:
        struct Node {
            T data;
            std::shared_ptr<Node> left;
            std::shared_ptr<Node> right;
            int height;
            size_t size; // Size of subtree rooted at this node
            
            explicit Node(const T& value) 
                : data(value), left(nullptr), right(nullptr), height(1), size(1) {}
        };
        
        using NodePtr = std::shared_ptr<Node>;

        BinarySearchTree() = default;
        ~BinarySearchTree() = default;
        
        // Basic operations
        void insert(const T& value);
        bool remove(const T& value);
        bool find(const T& value) const;
        
        // Tree properties
        size_t size() const;
        int height() const;
        bool empty() const;
        
        // Tree traversals
        std::vector<T> inorder_traversal() const;
        std::vector<T> preorder_traversal() const;
        std::vector<T> postorder_traversal() const;
        std::vector<T> level_order_traversal() const;
        
        // Advanced operations
        std::optional<T> find_min() const;
        std::optional<T> find_max() const;
        std::optional<T> find_kth_smallest(size_t k) const;
        std::optional<T> find_predecessor(const T& value) const;
        std::optional<T> find_successor(const T& value) const;
        
        // Range operations
        std::vector<T> range_query(const T& min_val, const T& max_val) const;
        size_t count_in_range(const T& min_val, const T& max_val) const;
        
        // Tree validation and info
        bool is_valid_bst() const;
        bool is_balanced() const;
        void print_tree() const;
        
        void clear();

    private:
        NodePtr root_;
        
        // AVL tree operations
        NodePtr insert_recursive(NodePtr node, const T& value);
        NodePtr remove_recursive(NodePtr node, const T& value);
        NodePtr find_min_node(NodePtr node) const;
        
        // AVL balancing
        int get_height(NodePtr node) const;
        int get_balance_factor(NodePtr node) const;
        void update_height_and_size(NodePtr node);
        NodePtr rotate_right(NodePtr node);
        NodePtr rotate_left(NodePtr node);
        
        // Traversal helpers
        void inorder_recursive(NodePtr node, std::vector<T>& result) const;
        void preorder_recursive(NodePtr node, std::vector<T>& result) const;
        void postorder_recursive(NodePtr node, std::vector<T>& result) const;
        
        // Utility functions
        bool find_recursive(NodePtr node, const T& value) const;
        void print_recursive(NodePtr node, int depth, const std::string& prefix) const;
        bool is_valid_bst_recursive(NodePtr node, const T* min_val, const T* max_val) const;
    };

    /**
     * @class MinHeap
     * @brief Min heap implementation with priority queue functionality
     */
    template<typename T, typename Compare = std::less<T>>
    class MinHeap {
    public:
        explicit MinHeap(Compare comp = Compare{});
        explicit MinHeap(const std::vector<T>& data, Compare comp = Compare{});
        
        // Basic operations
        void insert(const T& value);
        T extract_min();
        const T& peek_min() const;
        
        // Capacity
        size_t size() const { return heap_.size(); }
        bool empty() const { return heap_.empty(); }
        
        // Heap operations
        void decrease_key(size_t index, const T& new_value);
        void build_heap();
        
        // Utility
        std::vector<T> heap_sort() const;
        void print_heap() const;
        bool is_valid_heap() const;
        void clear();

    private:
        std::vector<T> heap_;
        Compare comp_;
        
        // Index calculations
        size_t parent(size_t index) const { return (index - 1) / 2; }
        size_t left_child(size_t index) const { return 2 * index + 1; }
        size_t right_child(size_t index) const { return 2 * index + 2; }
        
        // Heap maintenance
        void heapify_up(size_t index);
        void heapify_down(size_t index);
        bool is_valid_heap_recursive(size_t index) const;
    };

    /**
     * @class HashTable
     * @brief Hash table with chaining collision resolution
     */
    template<typename K, typename V, typename Hash = std::hash<K>>
    class HashTable {
    public:
        struct KeyValuePair {
            K key;
            V value;
            
            KeyValuePair(const K& k, const V& v) : key(k), value(v) {}
        };

        explicit HashTable(size_t initial_capacity = 16, double max_load_factor = 0.75);
        ~HashTable() = default;
        
        // Basic operations
        void insert(const K& key, const V& value);
        bool remove(const K& key);
        std::optional<V> find(const K& key) const;
        bool contains(const K& key) const;
        
        // Access operators
        V& operator[](const K& key);
        const V& at(const K& key) const;
        
        // Capacity and load factor
        size_t size() const { return size_; }
        size_t capacity() const { return buckets_.size(); }
        bool empty() const { return size_ == 0; }
        double load_factor() const { return static_cast<double>(size_) / buckets_.size(); }
        
        // Hash table operations
        void rehash();
        void clear();
        std::vector<K> keys() const;
        std::vector<V> values() const;
        
        // Statistics
        struct Statistics {
            size_t total_elements;
            size_t num_buckets;
            double load_factor;
            size_t max_chain_length;
            double avg_chain_length;
            size_t empty_buckets;
            size_t collisions;
        };
        
        Statistics get_statistics() const;
        void print_statistics() const;

    private:
        using Bucket = std::vector<KeyValuePair>;
        
        std::vector<Bucket> buckets_;
        size_t size_;
        double max_load_factor_;
        Hash hash_func_;
        size_t collision_count_;
        
        size_t hash_key(const K& key) const;
        void resize_and_rehash();
        typename Bucket::iterator find_in_bucket(Bucket& bucket, const K& key);
        typename Bucket::const_iterator find_in_bucket(const Bucket& bucket, const K& key) const;
    };

    /**
     * @class Trie
     * @brief Trie (prefix tree) for string storage and retrieval
     */
    class Trie {
    public:
        struct TrieNode {
            std::unordered_map<char, std::shared_ptr<TrieNode>> children;
            bool is_end_of_word;
            std::string stored_word; // For convenience
            int frequency; // For word frequency counting
            
            TrieNode() : is_end_of_word(false), frequency(0) {}
        };
        
        using NodePtr = std::shared_ptr<TrieNode>;

        Trie();
        
        // Basic operations
        void insert(const std::string& word);
        bool search(const std::string& word) const;
        bool starts_with(const std::string& prefix) const;
        void remove(const std::string& word);
        
        // Advanced operations
        std::vector<std::string> find_all_with_prefix(const std::string& prefix) const;
        std::vector<std::string> autocomplete(const std::string& prefix, size_t max_suggestions = 10) const;
        std::string longest_common_prefix() const;
        
        // Word counting
        void increment_frequency(const std::string& word);
        int get_frequency(const std::string& word) const;
        std::vector<std::pair<std::string, int>> get_most_frequent(size_t count = 10) const;
        
        // Trie properties
        size_t word_count() const { return word_count_; }
        size_t node_count() const;
        void clear();
        
        void print_all_words() const;

    private:
        NodePtr root_;
        size_t word_count_;
        
        void collect_words(NodePtr node, const std::string& prefix, std::vector<std::string>& words) const;
        void collect_words_with_frequency(NodePtr node, const std::string& prefix, 
                                        std::vector<std::pair<std::string, int>>& words) const;
        bool remove_recursive(NodePtr node, const std::string& word, size_t index);
        size_t count_nodes(NodePtr node) const;
        void print_words_recursive(NodePtr node, const std::string& prefix) const;
    };

    /**
     * @class DisjointSet (Union-Find)
     * @brief Disjoint set data structure with union by rank and path compression
     */
    class DisjointSet {
    public:
        explicit DisjointSet(size_t n);
        
        // Basic operations
        size_t find(size_t x);
        bool unite(size_t x, size_t y);
        bool connected(size_t x, size_t y);
        
        // Set properties
        size_t num_sets() const { return num_sets_; }
        size_t set_size(size_t x);
        std::vector<size_t> get_set_members(size_t representative);
        
        // Statistics
        void print_sets() const;
        size_t get_max_set_size() const;

    private:
        std::vector<size_t> parent_;
        std::vector<size_t> rank_;
        std::vector<size_t> size_;
        size_t num_sets_;
    };

    /**
     * @class BloomFilter
     * @brief Probabilistic data structure for membership testing
     */
    class BloomFilter {
    public:
        explicit BloomFilter(size_t expected_elements, double false_positive_rate = 0.01);
        
        // Basic operations
        void insert(const std::string& item);
        bool possibly_contains(const std::string& item) const;
        void clear();
        
        // Filter properties
        size_t size() const { return bit_array_.size(); }
        size_t inserted_elements() const { return inserted_count_; }
        double estimated_false_positive_rate() const;
        
        // Statistics
        size_t set_bits_count() const;
        double fill_ratio() const;
        void print_statistics() const;

    private:
        std::vector<bool> bit_array_;
        size_t num_hash_functions_;
        size_t inserted_count_;
        double target_false_positive_rate_;
        
        std::vector<size_t> get_hash_values(const std::string& item) const;
        size_t hash_function(const std::string& item, size_t seed) const;
        
        static size_t optimal_bit_array_size(size_t n, double p);
        static size_t optimal_num_hash_functions(size_t m, size_t n);
    };

    /**
     * @class SkipList
     * @brief Probabilistic data structure for fast search in ordered sequences
     */
    template<typename T>
    class SkipList {
    public:
        static constexpr int MAX_LEVEL = 16;
        static constexpr double P = 0.5;
        
        struct Node {
            T data;
            std::vector<std::shared_ptr<Node>> forward;
            
            explicit Node(const T& value, int level) : data(value), forward(level + 1) {}
        };
        
        using NodePtr = std::shared_ptr<Node>;

        SkipList();
        
        // Basic operations
        void insert(const T& value);
        bool remove(const T& value);
        bool find(const T& value) const;
        
        // List properties
        size_t size() const { return size_; }
        bool empty() const { return size_ == 0; }
        
        // Range operations
        std::vector<T> range_search(const T& min_val, const T& max_val) const;
        
        // Utility
        void print() const;
        void clear();

    private:
        NodePtr header_;
        int current_level_;
        size_t size_;
        mutable std::mt19937 rng_;
        
        int random_level();
        NodePtr find_node(const T& value) const;
    };

    /**
     * @class DataStructuresDemo
     * @brief Main demonstration coordinator for data structures
     */
    class DataStructuresDemo {
    public:
        static void demonstrate_dynamic_array();
        static void demonstrate_linked_list();
        static void demonstrate_binary_search_tree();
        static void demonstrate_min_heap();
        static void demonstrate_hash_table();
        static void demonstrate_trie();
        static void demonstrate_disjoint_set();
        static void demonstrate_bloom_filter();
        static void demonstrate_skip_list();
        static void demonstrate_performance_comparison();
        static void run_comprehensive_demo();

    private:
        static void print_section_header(const std::string& title);
        static void print_section_footer();
        static std::vector<int> generate_test_data(size_t size);
        static std::vector<std::string> generate_test_words(size_t count);
        
        template<typename Container>
        static void benchmark_container_operations(Container& container, const std::string& name);
    };

    /**
     * @class PerformanceBenchmark
     * @brief Benchmarking tools for data structure performance analysis
     */
    class PerformanceBenchmark {
    public:
        struct BenchmarkResult {
            std::string operation;
            std::string data_structure;
            std::chrono::microseconds avg_time;
            size_t operations_count;
            double ops_per_second;
        };

        template<typename DataStructure>
        static BenchmarkResult benchmark_insertion(DataStructure& ds, const std::vector<int>& data);
        
        template<typename DataStructure>
        static BenchmarkResult benchmark_search(DataStructure& ds, const std::vector<int>& search_keys);
        
        template<typename DataStructure>
        static BenchmarkResult benchmark_deletion(DataStructure& ds, const std::vector<int>& delete_keys);
        
        static std::vector<BenchmarkResult> comprehensive_benchmark(size_t data_size = 10000);
        static void print_benchmark_results(const std::vector<BenchmarkResult>& results);

    private:
        static std::vector<int> generate_random_data(size_t size);
        static std::vector<int> generate_search_keys(const std::vector<int>& data, size_t num_keys);
    };

} // namespace CppVerseHub::Algorithms

// Template method implementations would go in a separate .tpp file
// or be included at the end of this header file

#endif // DATASTRUCTURES_HPP