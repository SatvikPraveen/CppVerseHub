/**
 * @file SearchAlgorithms.hpp
 * @brief Comprehensive search algorithm implementations and demonstrations
 * @details File location: src/algorithms/SearchAlgorithms.hpp
 * 
 * This file contains implementations of various search algorithms including
 * binary search, graph search algorithms, string searching, and specialized
 * search techniques with performance analysis capabilities.
 */

#ifndef SEARCHALGORITHMS_HPP
#define SEARCHALGORITHMS_HPP

#include <vector>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <queue>
#include <stack>
#include <algorithm>
#include <chrono>
#include <functional>
#include <memory>
#include <optional>
#include <limits>
#include <iostream>
#include <iomanip>
#include <array>

namespace CppVerseHub::Algorithms {

    /**
     * @struct SearchResult
     * @brief Contains results and statistics from search operations
     */
    struct SearchResult {
        std::string algorithm_name;
        bool found;
        std::vector<size_t> positions;  // For multiple matches
        std::chrono::microseconds execution_time;
        size_t comparisons;
        size_t iterations;
        std::string time_complexity;
        std::string space_complexity;
        std::string additional_info;
    };

    /**
     * @class LinearSearch
     * @brief Linear search implementations with various optimizations
     */
    template<typename T>
    class LinearSearch {
    public:
        enum class Variant {
            STANDARD,
            SENTINEL,
            BIDIRECTIONAL,
            JUMP_SEARCH
        };

        static SearchResult search(const std::vector<T>& arr, const T& target, 
                                 Variant variant = Variant::STANDARD);
        
        static SearchResult search_all(const std::vector<T>& arr, const T& target);
        
        static SearchResult search_with_predicate(const std::vector<T>& arr, 
                                                 std::function<bool(const T&)> predicate);

    private:
        static SearchResult linear_search_standard(const std::vector<T>& arr, const T& target);
        static SearchResult linear_search_sentinel(const std::vector<T>& arr, const T& target);
        static SearchResult linear_search_bidirectional(const std::vector<T>& arr, const T& target);
        static SearchResult jump_search_impl(const std::vector<T>& arr, const T& target);
    };

    /**
     * @class BinarySearch
     * @brief Binary search implementations and variants
     */
    template<typename T>
    class BinarySearch {
    public:
        enum class Variant {
            ITERATIVE,
            RECURSIVE,
            LEFTMOST,
            RIGHTMOST,
            RANGE
        };

        static SearchResult search(const std::vector<T>& arr, const T& target, 
                                 Variant variant = Variant::ITERATIVE);
        
        static SearchResult search_range(const std::vector<T>& arr, const T& target);
        
        static SearchResult search_insertion_point(const std::vector<T>& arr, const T& target);
        
        static SearchResult search_peak_element(const std::vector<T>& arr);

    private:
        static SearchResult binary_search_iterative(const std::vector<T>& arr, const T& target);
        static SearchResult binary_search_recursive(const std::vector<T>& arr, const T& target, 
                                                   int left, int right, size_t& comparisons);
        static SearchResult find_leftmost(const std::vector<T>& arr, const T& target);
        static SearchResult find_rightmost(const std::vector<T>& arr, const T& target);
    };

    /**
     * @class TernarySearch
     * @brief Ternary search for unimodal functions and sorted arrays
     */
    template<typename T>
    class TernarySearch {
    public:
        static SearchResult search(const std::vector<T>& arr, const T& target);
        
        static SearchResult find_maximum(const std::vector<T>& arr);
        
        static SearchResult search_unimodal(std::function<T(T)> func, T left, T right, 
                                           T target, T epsilon = 1e-9);

    private:
        static SearchResult ternary_search_impl(const std::vector<T>& arr, const T& target, 
                                               int left, int right, size_t& comparisons);
    };

    /**
     * @class ExponentialSearch
     * @brief Exponential search for unbounded arrays
     */
    template<typename T>
    class ExponentialSearch {
    public:
        static SearchResult search(const std::vector<T>& arr, const T& target);
        
        static SearchResult search_unbounded(std::function<T(size_t)> get_element, 
                                            const T& target, size_t max_size);

    private:
        static SearchResult exponential_search_impl(const std::vector<T>& arr, const T& target);
    };

    /**
     * @class InterpolationSearch
     * @brief Interpolation search for uniformly distributed sorted data
     */
    template<typename T>
    class InterpolationSearch {
    public:
        static SearchResult search(const std::vector<T>& arr, const T& target);

    private:
        static SearchResult interpolation_search_impl(const std::vector<T>& arr, const T& target);
        static size_t interpolate_position(const std::vector<T>& arr, const T& target, 
                                         size_t low, size_t high);
    };

    /**
     * @class StringSearch
     * @brief String searching algorithms
     */
    class StringSearch {
    public:
        enum class Algorithm {
            NAIVE,
            KMP,              // Knuth-Morris-Pratt
            BOYER_MOORE,
            RABIN_KARP,
            Z_ALGORITHM,
            SUFFIX_ARRAY
        };

        static SearchResult search(const std::string& text, const std::string& pattern, 
                                 Algorithm algorithm = Algorithm::KMP);
        
        static SearchResult search_all_occurrences(const std::string& text, 
                                                  const std::string& pattern,
                                                  Algorithm algorithm = Algorithm::KMP);

    private:
        static SearchResult naive_search(const std::string& text, const std::string& pattern);
        static SearchResult kmp_search(const std::string& text, const std::string& pattern);
        static SearchResult boyer_moore_search(const std::string& text, const std::string& pattern);
        static SearchResult rabin_karp_search(const std::string& text, const std::string& pattern);
        static SearchResult z_algorithm_search(const std::string& text, const std::string& pattern);
        
        static std::vector<int> compute_lps(const std::string& pattern);
        static std::vector<int> compute_bad_char_table(const std::string& pattern);
        static std::vector<int> compute_z_array(const std::string& str);
        static size_t rolling_hash(const std::string& str, size_t start, size_t length);
        
        static const int ALPHABET_SIZE = 256;
        static const int HASH_BASE = 256;
        static const int HASH_MOD = 101;
    };

    /**
     * @class Graph
     * @brief Graph representation for search algorithms
     */
    template<typename T>
    class Graph {
    public:
        struct Edge {
            size_t from;
            size_t to;
            T weight;
            
            Edge(size_t f, size_t t, T w = T{}) : from(f), to(t), weight(w) {}
        };

        explicit Graph(size_t num_vertices, bool directed = false);
        
        void add_edge(size_t from, size_t to, T weight = T{});
        void add_vertex_data(size_t vertex, const std::string& data);
        
        size_t vertex_count() const { return adjacency_list_.size(); }
        const std::vector<Edge>& get_adjacent(size_t vertex) const;
        const std::string& get_vertex_data(size_t vertex) const;
        
        void print_graph() const;

    private:
        std::vector<std::vector<Edge>> adjacency_list_;
        std::vector<std::string> vertex_data_;
        bool directed_;
    };

    /**
     * @class GraphSearch
     * @brief Graph traversal and search algorithms
     */
    template<typename T>
    class GraphSearch {
    public:
        struct SearchPath {
            std::vector<size_t> path;
            T total_cost;
            bool found;
            size_t nodes_visited;
        };

        static SearchResult breadth_first_search(const Graph<T>& graph, size_t start, size_t target);
        static SearchResult depth_first_search(const Graph<T>& graph, size_t start, size_t target);
        
        static SearchResult dijkstra_shortest_path(const Graph<T>& graph, size_t start, size_t target);
        static SearchResult a_star_search(const Graph<T>& graph, size_t start, size_t target,
                                         std::function<T(size_t, size_t)> heuristic);
        
        static SearchResult bidirectional_search(const Graph<T>& graph, size_t start, size_t target);
        
        static std::vector<size_t> topological_sort(const Graph<T>& graph);
        static std::vector<std::vector<size_t>> strongly_connected_components(const Graph<T>& graph);

    private:
        static SearchResult bfs_impl(const Graph<T>& graph, size_t start, size_t target);
        static SearchResult dfs_impl(const Graph<T>& graph, size_t start, size_t target);
        static void dfs_recursive(const Graph<T>& graph, size_t current, size_t target,
                                std::vector<bool>& visited, std::vector<size_t>& path,
                                bool& found, size_t& nodes_visited);
    };

    /**
     * @class TreeSearch
     * @brief Binary search tree and tree traversal algorithms
     */
    template<typename T>
    class TreeSearch {
    public:
        struct TreeNode {
            T data;
            std::shared_ptr<TreeNode> left;
            std::shared_ptr<TreeNode> right;
            
            explicit TreeNode(const T& value) : data(value), left(nullptr), right(nullptr) {}
        };

        using TreeNodePtr = std::shared_ptr<TreeNode>;

        static TreeNodePtr build_bst_from_sorted(const std::vector<T>& arr);
        static SearchResult search_bst(TreeNodePtr root, const T& target);
        
        static SearchResult inorder_search(TreeNodePtr root, const T& target);
        static SearchResult preorder_search(TreeNodePtr root, const T& target);
        static SearchResult postorder_search(TreeNodePtr root, const T& target);
        
        static SearchResult level_order_search(TreeNodePtr root, const T& target);
        
        static TreeNodePtr find_lca(TreeNodePtr root, const T& a, const T& b);

    private:
        static TreeNodePtr build_bst_recursive(const std::vector<T>& arr, int left, int right);
        static bool search_bst_recursive(TreeNodePtr node, const T& target, size_t& comparisons);
    };

    /**
     * @class HashSearch
     * @brief Hash-based search implementations
     */
    template<typename T>
    class HashSearch {
    public:
        class HashTable {
        public:
            enum class CollisionResolution {
                CHAINING,
                LINEAR_PROBING,
                QUADRATIC_PROBING,
                DOUBLE_HASHING
            };

            explicit HashTable(size_t initial_capacity = 16, 
                              CollisionResolution method = CollisionResolution::CHAINING);
            
            void insert(const T& key, const std::string& value);
            SearchResult search(const T& key);
            bool remove(const T& key);
            
            void print_table() const;
            double load_factor() const;
            void resize();

        private:
            struct HashEntry {
                T key;
                std::string value;
                bool deleted;
                
                HashEntry() : deleted(true) {}
                HashEntry(const T& k, const std::string& v) : key(k), value(v), deleted(false) {}
            };

            std::vector<std::vector<HashEntry>> table_; // For chaining
            std::vector<HashEntry> linear_table_;       // For open addressing
            size_t capacity_;
            size_t size_;
            CollisionResolution collision_method_;
            
            size_t hash_function(const T& key) const;
            size_t hash_function2(const T& key) const; // For double hashing
            size_t probe_sequence(const T& key, size_t attempt) const;
        };

        static SearchResult search_in_hash_table(HashTable& table, const T& key);
    };

    /**
     * @class BloomFilter
     * @brief Probabilistic search data structure
     */
    class BloomFilter {
    public:
        explicit BloomFilter(size_t expected_elements, double false_positive_rate = 0.01);
        
        void insert(const std::string& element);
        SearchResult might_contain(const std::string& element) const;
        
        void clear();
        double estimated_false_positive_rate() const;
        size_t size() const { return bit_array_.size(); }

    private:
        std::vector<bool> bit_array_;
        size_t num_hash_functions_;
        size_t num_inserted_;
        
        std::vector<size_t> get_hash_values(const std::string& element) const;
        size_t hash_function(const std::string& element, size_t seed) const;
        
        static size_t optimal_num_hash_functions(size_t m, size_t n);
        static size_t optimal_bit_array_size(size_t n, double p);
    };

    /**
     * @class Trie
     * @brief Trie (Prefix Tree) for string searching
     */
    class Trie {
    public:
        struct TrieNode {
            std::unordered_map<char, std::shared_ptr<TrieNode>> children;
            bool is_end_of_word;
            std::string word;
            
            TrieNode() : is_end_of_word(false) {}
        };

        using TrieNodePtr = std::shared_ptr<TrieNode>;

        Trie();
        
        void insert(const std::string& word);
        SearchResult search(const std::string& word);
        SearchResult starts_with(const std::string& prefix);
        
        std::vector<std::string> get_all_words_with_prefix(const std::string& prefix);
        std::vector<std::string> auto_complete(const std::string& prefix, size_t max_suggestions = 10);
        
        void remove(const std::string& word);
        void print_all_words() const;

    private:
        TrieNodePtr root_;
        
        void collect_words(TrieNodePtr node, std::vector<std::string>& words);
        void print_words_recursive(TrieNodePtr node, const std::string& prefix) const;
        bool remove_recursive(TrieNodePtr node, const std::string& word, size_t index);
    };

    /**
     * @class SuffixArray
     * @brief Suffix array for efficient string searching
     */
    class SuffixArray {
    public:
        explicit SuffixArray(const std::string& text);
        
        SearchResult search(const std::string& pattern);
        SearchResult count_occurrences(const std::string& pattern);
        
        std::vector<size_t> get_all_occurrences(const std::string& pattern);
        std::string longest_common_substring(const std::string& other);
        
        void print_suffixes() const;

    private:
        std::string text_;
        std::vector<size_t> suffix_array_;
        std::vector<size_t> lcp_array_; // Longest Common Prefix
        
        void build_suffix_array();
        void build_lcp_array();
        
        std::pair<size_t, size_t> binary_search_range(const std::string& pattern);
        bool compare_suffix(size_t suffix_idx, const std::string& pattern);
    };

    /**
     * @class NearestNeighborSearch
     * @brief Algorithms for finding nearest neighbors in multidimensional space
     */
    template<typename T, size_t Dimensions>
    class NearestNeighborSearch {
    public:
        struct Point {
            std::array<T, Dimensions> coordinates;
            std::string data;
            
            Point() = default;
            Point(const std::array<T, Dimensions>& coords, const std::string& d = "") 
                : coordinates(coords), data(d) {}
        };

        struct SearchResultNN {
            std::vector<Point> neighbors;
            std::vector<T> distances;
            size_t comparisons;
            std::chrono::microseconds execution_time;
        };

        static SearchResultNN brute_force_nearest(const std::vector<Point>& points, 
                                                 const Point& query, size_t k = 1);
        
        static SearchResultNN range_search(const std::vector<Point>& points, 
                                          const Point& center, T radius);

        // KD-Tree implementation
        class KDTree {
        public:
            explicit KDTree(const std::vector<Point>& points);
            
            SearchResultNN find_nearest(const Point& query, size_t k = 1);
            SearchResultNN range_search(const Point& center, T radius);

        private:
            struct KDNode {
                Point point;
                size_t split_dimension;
                std::shared_ptr<KDNode> left;
                std::shared_ptr<KDNode> right;
                
                KDNode(const Point& p, size_t dim) : point(p), split_dimension(dim) {}
            };

            using KDNodePtr = std::shared_ptr<KDNode>;
            KDNodePtr root_;
            
            KDNodePtr build_tree(std::vector<Point>& points, size_t depth = 0);
            void find_nearest_recursive(KDNodePtr node, const Point& query, size_t k,
                                       std::priority_queue<std::pair<T, Point>>& best,
                                       size_t& comparisons);
        };

    private:
        static T euclidean_distance(const Point& a, const Point& b);
        static T manhattan_distance(const Point& a, const Point& b);
    };

    /**
     * @class SearchBenchmark
     * @brief Performance benchmarking for search algorithms
     */
    class SearchBenchmark {
    public:
        enum class DataPattern {
            SORTED,
            RANDOM,
            NEARLY_SORTED,
            REVERSE_SORTED,
            MANY_DUPLICATES
        };

        struct BenchmarkResult {
            std::vector<SearchResult> results;
            std::string fastest_algorithm;
            std::string most_consistent;
            double average_time;
            double success_rate;
        };

        static BenchmarkResult compare_search_algorithms(const std::vector<int>& data, 
                                                        int target, DataPattern pattern);
        
        static BenchmarkResult compare_string_search_algorithms(const std::string& text, 
                                                               const std::string& pattern);
        
        static BenchmarkResult benchmark_graph_algorithms(size_t num_vertices, 
                                                         double edge_density = 0.3);
        
        static void print_benchmark_results(const BenchmarkResult& result);

    private:
        static std::vector<int> generate_test_data(size_t size, DataPattern pattern);
        static std::string generate_test_text(size_t length);
        static Graph<int> generate_test_graph(size_t vertices, double density);
    };

    /**
     * @class SearchVisualization
     * @brief Visualization and step-by-step analysis of search algorithms
     */
    template<typename T>
    class SearchVisualization {
    public:
        struct SearchStep {
            std::string operation;
            std::vector<T> search_space;
            std::vector<size_t> active_indices;
            std::vector<size_t> compared_indices;
            size_t current_position;
            bool found;
            std::string description;
        };

        SearchVisualization(bool enable_visualization = true) : visualize_(enable_visualization) {}
        
        void set_visualization_enabled(bool enabled) { visualize_ = enabled; }
        
        template<typename SearchFunc>
        SearchResult visualized_search(const std::vector<T>& data, const T& target,
                                      SearchFunc search_function, const std::string& algorithm_name);
        
        const std::vector<SearchStep>& get_steps() const { return steps_; }
        void print_visualization() const;
        void clear_steps() { steps_.clear(); }

    private:
        bool visualize_;
        std::vector<SearchStep> steps_;
        
        void record_step(const std::string& operation, const std::vector<T>& search_space,
                        const std::vector<size_t>& active_indices = {},
                        const std::vector<size_t>& compared_indices = {},
                        size_t current_position = SIZE_MAX,
                        bool found = false, const std::string& description = "");
    };

    /**
     * @class AdvancedSearchTechniques
     * @brief Advanced and specialized search algorithms
     */
    class AdvancedSearchTechniques {
    public:
        // Fuzzy string matching
        static SearchResult levenshtein_search(const std::string& text, 
                                              const std::string& pattern, 
                                              int max_distance = 2);
        
        static SearchResult approximate_string_matching(const std::vector<std::string>& dictionary,
                                                       const std::string& query,
                                                       int max_distance = 2);
        
        // Geometric search
        template<typename T>
        static SearchResult convex_hull_search(const std::vector<std::pair<T, T>>& points,
                                              const std::pair<T, T>& query);
        
        // Probabilistic search
        static SearchResult monte_carlo_search(std::function<bool(double)> predicate,
                                              double lower_bound, double upper_bound,
                                              size_t iterations = 100000);
        
        // Multi-pattern search
        static SearchResult aho_corasick_search(const std::string& text,
                                               const std::vector<std::string>& patterns);

    private:
        static int levenshtein_distance(const std::string& a, const std::string& b);
        
        // Aho-Corasick automaton
        struct ACNode {
            std::unordered_map<char, std::shared_ptr<ACNode>> children;
            std::shared_ptr<ACNode> failure_link;
            std::vector<std::string> output;
            
            ACNode() : failure_link(nullptr) {}
        };
        
        static std::shared_ptr<ACNode> build_aho_corasick_automaton(const std::vector<std::string>& patterns);
    };

    /**
     * @class SearchAlgorithmsDemo
     * @brief Main demonstration coordinator for all search algorithms
     */
    class SearchAlgorithmsDemo {
    public:
        static void demonstrate_basic_search_algorithms();
        static void demonstrate_binary_search_variants();
        static void demonstrate_string_search_algorithms();
        static void demonstrate_graph_search_algorithms();
        static void demonstrate_tree_search_algorithms();
        static void demonstrate_hash_based_search();
        static void demonstrate_advanced_data_structures();
        static void demonstrate_geometric_search();
        static void demonstrate_approximate_search();
        static void demonstrate_performance_comparison();
        static void run_comprehensive_search_demo();

    private:
        static void print_section_header(const std::string& title);
        static void print_section_footer();
        static void print_search_result(const SearchResult& result);
        
        static std::vector<int> generate_sample_data(size_t size, bool sorted = false);
        static std::string generate_sample_text(size_t length);
        static Graph<int> generate_sample_graph(size_t vertices);
    };

} // namespace CppVerseHub::Algorithms

#endif // SEARCHALGORITHMS_HPP