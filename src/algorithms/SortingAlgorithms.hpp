/**
 * @file SortingAlgorithms.hpp
 * @brief Comprehensive sorting algorithm implementations and demonstrations
 * @details File location: src/algorithms/SortingAlgorithms.hpp
 * 
 * This file contains implementations of various sorting algorithms including
 * quicksort, mergesort, heapsort, and other classic sorting techniques with
 * performance analysis and visualization capabilities.
 */

#ifndef SORTINGALGORITHMS_HPP
#define SORTINGALGORITHMS_HPP

#include <vector>
#include <algorithm>
#include <chrono>
#include <random>
#include <functional>
#include <iostream>
#include <string>
#include <memory>
#include <thread>
#include <future>
#include <iomanip>

namespace CppVerseHub::Algorithms {

    /**
     * @struct SortingResult
     * @brief Contains results and statistics from sorting operations
     */
    struct SortingResult {
        std::string algorithm_name;
        std::chrono::microseconds execution_time;
        size_t comparisons;
        size_t swaps;
        size_t array_size;
        bool is_stable;
        std::string time_complexity;
        std::string space_complexity;
    };

    /**
     * @class QuickSort
     * @brief Quick sort implementation with various pivot strategies
     */
    template<typename T, typename Compare = std::less<T>>
    class QuickSort {
    public:
        enum class PivotStrategy {
            FIRST,
            LAST,
            MIDDLE,
            RANDOM,
            MEDIAN_OF_THREE
        };

        static SortingResult sort(std::vector<T>& arr, Compare comp = Compare{}, 
                                PivotStrategy strategy = PivotStrategy::MEDIAN_OF_THREE);
        
        static void sort_parallel(std::vector<T>& arr, Compare comp = Compare{}, 
                                size_t min_parallel_size = 1000);

    private:
        static void quicksort_impl(std::vector<T>& arr, int low, int high, Compare& comp, 
                                 PivotStrategy strategy, size_t& comparisons, size_t& swaps);
        
        static void quicksort_parallel_impl(std::vector<T>& arr, int low, int high, 
                                          Compare& comp, size_t min_parallel_size);
        
        static int partition(std::vector<T>& arr, int low, int high, Compare& comp, 
                           PivotStrategy strategy, size_t& comparisons, size_t& swaps);
        
        static int choose_pivot(const std::vector<T>& arr, int low, int high, 
                              PivotStrategy strategy);
        
        static int median_of_three(const std::vector<T>& arr, int low, int high, Compare& comp);
        
        static std::random_device rd_;
        static std::mt19937 gen_;
    };

    /**
     * @class MergeSort
     * @brief Merge sort implementation with bottom-up and top-down variants
     */
    template<typename T, typename Compare = std::less<T>>
    class MergeSort {
    public:
        enum class Variant {
            TOP_DOWN,
            BOTTOM_UP,
            IN_PLACE
        };

        static SortingResult sort(std::vector<T>& arr, Compare comp = Compare{}, 
                                Variant variant = Variant::TOP_DOWN);
        
        static void sort_parallel(std::vector<T>& arr, Compare comp = Compare{}, 
                                size_t min_parallel_size = 1000);

    private:
        static void mergesort_top_down(std::vector<T>& arr, std::vector<T>& temp, 
                                     int left, int right, Compare& comp, 
                                     size_t& comparisons, size_t& swaps);
        
        static void mergesort_bottom_up(std::vector<T>& arr, Compare& comp, 
                                      size_t& comparisons, size_t& swaps);
        
        static void mergesort_parallel_impl(std::vector<T>& arr, std::vector<T>& temp,
                                          int left, int right, Compare& comp,
                                          size_t min_parallel_size);
        
        static void merge(std::vector<T>& arr, std::vector<T>& temp, int left, 
                        int mid, int right, Compare& comp, size_t& comparisons, size_t& swaps);
        
        static void merge_inplace(std::vector<T>& arr, int left, int mid, int right, 
                                Compare& comp, size_t& comparisons, size_t& swaps);
    };

    /**
     * @class HeapSort
     * @brief Heap sort implementation with max-heap and min-heap variants
     */
    template<typename T, typename Compare = std::less<T>>
    class HeapSort {
    public:
        static SortingResult sort(std::vector<T>& arr, Compare comp = Compare{});

    private:
        static void heapify(std::vector<T>& arr, int n, int i, Compare& comp, 
                          size_t& comparisons, size_t& swaps);
        
        static void build_heap(std::vector<T>& arr, Compare& comp, 
                             size_t& comparisons, size_t& swaps);
    };

    /**
     * @class InsertionSort
     * @brief Insertion sort with binary search optimization
     */
    template<typename T, typename Compare = std::less<T>>
    class InsertionSort {
    public:
        enum class Variant {
            STANDARD,
            BINARY_INSERTION,
            SHELL_SORT
        };

        static SortingResult sort(std::vector<T>& arr, Compare comp = Compare{}, 
                                Variant variant = Variant::STANDARD);

    private:
        static void insertion_sort_standard(std::vector<T>& arr, Compare& comp, 
                                          size_t& comparisons, size_t& swaps);
        
        static void binary_insertion_sort(std::vector<T>& arr, Compare& comp, 
                                        size_t& comparisons, size_t& swaps);
        
        static void shell_sort(std::vector<T>& arr, Compare& comp, 
                             size_t& comparisons, size_t& swaps);
        
        static int binary_search_insertion_point(const std::vector<T>& arr, int left, 
                                                int right, const T& key, Compare& comp, 
                                                size_t& comparisons);
    };

    /**
     * @class SelectionSort
     * @brief Selection sort with min-max variant for better performance
     */
    template<typename T, typename Compare = std::less<T>>
    class SelectionSort {
    public:
        enum class Variant {
            STANDARD,
            MIN_MAX_SELECTION
        };

        static SortingResult sort(std::vector<T>& arr, Compare comp = Compare{}, 
                                Variant variant = Variant::STANDARD);

    private:
        static void selection_sort_standard(std::vector<T>& arr, Compare& comp, 
                                          size_t& comparisons, size_t& swaps);
        
        static void min_max_selection_sort(std::vector<T>& arr, Compare& comp, 
                                         size_t& comparisons, size_t& swaps);
    };

    /**
     * @class BubbleSort
     * @brief Bubble sort with cocktail shaker optimization
     */
    template<typename T, typename Compare = std::less<T>>
    class BubbleSort {
    public:
        enum class Variant {
            STANDARD,
            OPTIMIZED,
            COCKTAIL_SHAKER
        };

        static SortingResult sort(std::vector<T>& arr, Compare comp = Compare{}, 
                                Variant variant = Variant::OPTIMIZED);

    private:
        static void bubble_sort_standard(std::vector<T>& arr, Compare& comp, 
                                       size_t& comparisons, size_t& swaps);
        
        static void bubble_sort_optimized(std::vector<T>& arr, Compare& comp, 
                                        size_t& comparisons, size_t& swaps);
        
        static void cocktail_shaker_sort(std::vector<T>& arr, Compare& comp, 
                                       size_t& comparisons, size_t& swaps);
    };

    /**
     * @class RadixSort
     * @brief Radix sort for integer types with LSD and MSD variants
     */
    class RadixSort {
    public:
        enum class Variant {
            LSD,  // Least Significant Digit
            MSD   // Most Significant Digit
        };

        static SortingResult sort(std::vector<int>& arr, Variant variant = Variant::LSD);
        static SortingResult sort(std::vector<std::string>& arr, Variant variant = Variant::LSD);

    private:
        static void counting_sort_for_radix(std::vector<int>& arr, int exp, 
                                          size_t& comparisons, size_t& swaps);
        
        static void radix_sort_lsd(std::vector<int>& arr, size_t& comparisons, size_t& swaps);
        static void radix_sort_msd(std::vector<int>& arr, size_t& comparisons, size_t& swaps);
        
        static void radix_sort_strings_lsd(std::vector<std::string>& arr, 
                                         size_t& comparisons, size_t& swaps);
        
        static int get_max(const std::vector<int>& arr);
    };

    /**
     * @class CountingSort
     * @brief Counting sort for integers within a known range
     */
    class CountingSort {
    public:
        static SortingResult sort(std::vector<int>& arr, int min_val = INT_MIN, int max_val = INT_MAX);

    private:
        static void find_range(const std::vector<int>& arr, int& min_val, int& max_val);
    };

    /**
     * @class BucketSort
     * @brief Bucket sort for uniformly distributed data
     */
    template<typename T>
    class BucketSort {
    public:
        static SortingResult sort(std::vector<T>& arr, size_t bucket_count = 10, 
                                std::function<size_t(const T&, size_t)> hash_func = nullptr);

    private:
        static size_t default_hash(const T& value, size_t bucket_count);
    };

    /**
     * @class HybridSort
     * @brief Hybrid sorting algorithms combining multiple techniques
     */
    template<typename T, typename Compare = std::less<T>>
    class HybridSort {
    public:
        enum class Strategy {
            INTROSORT,     // Quicksort -> Heapsort -> Insertion
            TIMSORT_LIKE,  // Merge-based with runs
            ADAPTIVE       // Chooses algorithm based on input characteristics
        };

        static SortingResult sort(std::vector<T>& arr, Compare comp = Compare{}, 
                                Strategy strategy = Strategy::INTROSORT);

    private:
        static void introsort_impl(std::vector<T>& arr, int low, int high, int max_depth, 
                                 Compare& comp, size_t& comparisons, size_t& swaps);
        
        static void analyze_input_and_sort(std::vector<T>& arr, Compare& comp, 
                                         size_t& comparisons, size_t& swaps);
        
        static bool is_nearly_sorted(const std::vector<T>& arr, Compare& comp, double threshold = 0.1);
        static bool has_many_duplicates(const std::vector<T>& arr, double threshold = 0.1);
        static int calculate_max_depth(size_t n);
    };

    /**
     * @class SortingVisualizer
     * @brief Provides visualization and step-by-step analysis of sorting algorithms
     */
    template<typename T>
    class SortingVisualizer {
    public:
        struct SortingStep {
            std::vector<T> array_state;
            std::string operation;
            std::vector<size_t> highlighted_indices;
            size_t comparison_count;
            size_t swap_count;
        };

        SortingVisualizer(bool enable_visualization = false) : visualize_(enable_visualization) {}

        void set_visualization_enabled(bool enabled) { visualize_ = enabled; }
        
        template<typename SortFunc>
        SortingResult sort_with_visualization(std::vector<T>& arr, SortFunc sort_function, 
                                            const std::string& algorithm_name);
        
        const std::vector<SortingStep>& get_steps() const { return steps_; }
        void print_visualization() const;
        void clear_steps() { steps_.clear(); }

    private:
        bool visualize_;
        std::vector<SortingStep> steps_;
        
        void record_step(const std::vector<T>& arr, const std::string& operation, 
                        const std::vector<size_t>& highlighted = {}, 
                        size_t comparisons = 0, size_t swaps = 0);
    };

    /**
     * @class SortingBenchmark
     * @brief Performance benchmarking and comparison of sorting algorithms
     */
    class SortingBenchmark {
    public:
        enum class DataPattern {
            RANDOM,
            NEARLY_SORTED,
            REVERSE_SORTED,
            MANY_DUPLICATES,
            SORTED,
            SAWTOOTH,
            ORGAN_PIPE
        };

        struct BenchmarkResult {
            std::vector<SortingResult> results;
            std::string fastest_algorithm;
            std::string most_stable;
            std::string best_for_pattern;
        };

        static BenchmarkResult run_comprehensive_benchmark(size_t array_size = 10000, 
                                                          DataPattern pattern = DataPattern::RANDOM);
        
        static std::vector<int> generate_test_data(size_t size, DataPattern pattern, int min_val = 0, int max_val = 100000);
        
        static void print_benchmark_results(const BenchmarkResult& result);
        
        static BenchmarkResult compare_on_different_sizes(const std::vector<size_t>& sizes, 
                                                         DataPattern pattern = DataPattern::RANDOM);

    private:
        static std::vector<int> generate_random_data(size_t size, int min_val, int max_val);
        static std::vector<int> generate_nearly_sorted_data(size_t size, double sorted_fraction = 0.9);
        static std::vector<int> generate_many_duplicates_data(size_t size, int num_unique = 100);
        static std::vector<int> generate_sawtooth_data(size_t size, int num_teeth = 10);
        static std::vector<int> generate_organ_pipe_data(size_t size);
        
        static std::random_device rd_;
        static std::mt19937 gen_;
    };

    /**
     * @class ParallelSort
     * @brief Parallel implementations of sorting algorithms
     */
    template<typename T, typename Compare = std::less<T>>
    class ParallelSort {
    public:
        static SortingResult parallel_quicksort(std::vector<T>& arr, Compare comp = Compare{}, 
                                               size_t num_threads = std::thread::hardware_concurrency());
        
        static SortingResult parallel_mergesort(std::vector<T>& arr, Compare comp = Compare{}, 
                                               size_t num_threads = std::thread::hardware_concurrency());
        
        static SortingResult parallel_radix_sort(std::vector<int>& arr, 
                                                size_t num_threads = std::thread::hardware_concurrency());

    private:
        static void parallel_quicksort_impl(std::vector<T>& arr, int low, int high, 
                                          Compare& comp, size_t depth, size_t max_depth);
        
        static void parallel_mergesort_impl(std::vector<T>& arr, std::vector<T>& temp,
                                          int left, int right, Compare& comp, 
                                          size_t depth, size_t max_depth);
    };

    /**
     * @class SortingAlgorithmsDemo
     * @brief Main demonstration coordinator for all sorting algorithms
     */
    class SortingAlgorithmsDemo {
    public:
        static void demonstrate_basic_sorting_algorithms();
        static void demonstrate_advanced_sorting_algorithms();
        static void demonstrate_specialized_sorting();
        static void demonstrate_hybrid_sorting();
        static void demonstrate_parallel_sorting();
        static void demonstrate_sorting_visualization();
        static void demonstrate_performance_analysis();
        static void run_comprehensive_sorting_demo();

    private:
        static void print_section_header(const std::string& title);
        static void print_section_footer();
        static void print_sorting_result(const SortingResult& result);
        static std::vector<int> generate_sample_data(size_t size, const std::string& pattern = "random");
    };

    // Template static member definitions
    template<typename T, typename Compare>
    std::random_device QuickSort<T, Compare>::rd_;
    
    template<typename T, typename Compare>
    std::mt19937 QuickSort<T, Compare>::gen_(QuickSort<T, Compare>::rd_());

} // namespace CppVerseHub::Algorithms

#endif // SORTINGALGORITHMS_HPP