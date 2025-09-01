/**
 * @file SortingAlgorithms.cpp
 * @brief Implementation of comprehensive sorting algorithm demonstrations
 * @details File location: src/algorithms/SortingAlgorithms.cpp
 */

#include "SortingAlgorithms.hpp"
#include <climits>
#include <cmath>
#include <sstream>

namespace CppVerseHub::Algorithms {

    // Static member definitions
    std::random_device SortingBenchmark::rd_;
    std::mt19937 SortingBenchmark::gen_(SortingBenchmark::rd_());

    // ========== QuickSort Implementation ==========

    template<typename T, typename Compare>
    SortingResult QuickSort<T, Compare>::sort(std::vector<T>& arr, Compare comp, PivotStrategy strategy) {
        auto start_time = std::chrono::high_resolution_clock::now();
        
        size_t comparisons = 0, swaps = 0;
        if (!arr.empty()) {
            quicksort_impl(arr, 0, static_cast<int>(arr.size() - 1), comp, strategy, comparisons, swaps);
        }
        
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
        
        return {
            "QuickSort",
            duration,
            comparisons,
            swaps,
            arr.size(),
            false,  // QuickSort is not stable
            "O(n log n) average, O(n²) worst",
            "O(log n)"
        };
    }

    template<typename T, typename Compare>
    void QuickSort<T, Compare>::quicksort_impl(std::vector<T>& arr, int low, int high, 
                                              Compare& comp, PivotStrategy strategy, 
                                              size_t& comparisons, size_t& swaps) {
        if (low < high) {
            int pivot = partition(arr, low, high, comp, strategy, comparisons, swaps);
            quicksort_impl(arr, low, pivot - 1, comp, strategy, comparisons, swaps);
            quicksort_impl(arr, pivot + 1, high, comp, strategy, comparisons, swaps);
        }
    }

    template<typename T, typename Compare>
    int QuickSort<T, Compare>::partition(std::vector<T>& arr, int low, int high, 
                                        Compare& comp, PivotStrategy strategy, 
                                        size_t& comparisons, size_t& swaps) {
        int pivot_idx = choose_pivot(arr, low, high, strategy);
        if (pivot_idx != high) {
            std::swap(arr[pivot_idx], arr[high]);
            swaps++;
        }
        
        T pivot = arr[high];
        int i = low - 1;
        
        for (int j = low; j < high; ++j) {
            comparisons++;
            if (comp(arr[j], pivot) || (!comp(pivot, arr[j]) && !comp(arr[j], pivot))) {
                i++;
                if (i != j) {
                    std::swap(arr[i], arr[j]);
                    swaps++;
                }
            }
        }
        
        if (i + 1 != high) {
            std::swap(arr[i + 1], arr[high]);
            swaps++;
        }
        
        return i + 1;
    }

    template<typename T, typename Compare>
    int QuickSort<T, Compare>::choose_pivot(const std::vector<T>& arr, int low, int high, 
                                           PivotStrategy strategy) {
        switch (strategy) {
            case PivotStrategy::FIRST:
                return low;
            case PivotStrategy::LAST:
                return high;
            case PivotStrategy::MIDDLE:
                return (low + high) / 2;
            case PivotStrategy::RANDOM: {
                std::uniform_int_distribution<int> dist(low, high);
                return dist(gen_);
            }
            case PivotStrategy::MEDIAN_OF_THREE:
                return median_of_three(arr, low, high, Compare{});
            default:
                return high;
        }
    }

    template<typename T, typename Compare>
    int QuickSort<T, Compare>::median_of_three(const std::vector<T>& arr, int low, int high, Compare& comp) {
        int mid = (low + high) / 2;
        
        if (comp(arr[mid], arr[low])) {
            if (comp(arr[high], arr[mid])) return mid;
            else return comp(arr[high], arr[low]) ? high : low;
        } else {
            if (comp(arr[high], arr[low])) return low;
            else return comp(arr[high], arr[mid]) ? high : mid;
        }
    }

    // ========== MergeSort Implementation ==========

    template<typename T, typename Compare>
    SortingResult MergeSort<T, Compare>::sort(std::vector<T>& arr, Compare comp, Variant variant) {
        auto start_time = std::chrono::high_resolution_clock::now();
        
        size_t comparisons = 0, swaps = 0;
        
        if (arr.size() > 1) {
            if (variant == Variant::TOP_DOWN) {
                std::vector<T> temp(arr.size());
                mergesort_top_down(arr, temp, 0, static_cast<int>(arr.size() - 1), comp, comparisons, swaps);
            } else if (variant == Variant::BOTTOM_UP) {
                mergesort_bottom_up(arr, comp, comparisons, swaps);
            }
        }
        
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
        
        return {
            "MergeSort",
            duration,
            comparisons,
            swaps,
            arr.size(),
            true,  // MergeSort is stable
            "O(n log n)",
            "O(n)"
        };
    }

    template<typename T, typename Compare>
    void MergeSort<T, Compare>::mergesort_top_down(std::vector<T>& arr, std::vector<T>& temp, 
                                                  int left, int right, Compare& comp, 
                                                  size_t& comparisons, size_t& swaps) {
        if (left < right) {
            int mid = left + (right - left) / 2;
            
            mergesort_top_down(arr, temp, left, mid, comp, comparisons, swaps);
            mergesort_top_down(arr, temp, mid + 1, right, comp, comparisons, swaps);
            merge(arr, temp, left, mid, right, comp, comparisons, swaps);
        }
    }

    template<typename T, typename Compare>
    void MergeSort<T, Compare>::merge(std::vector<T>& arr, std::vector<T>& temp, int left, 
                                     int mid, int right, Compare& comp, 
                                     size_t& comparisons, size_t& swaps) {
        int i = left, j = mid + 1, k = left;
        
        // Copy to temp array
        for (int idx = left; idx <= right; ++idx) {
            temp[idx] = arr[idx];
        }
        
        // Merge back to original array
        while (i <= mid && j <= right) {
            comparisons++;
            if (comp(temp[i], temp[j]) || (!comp(temp[j], temp[i]) && i <= mid)) {
                arr[k++] = temp[i++];
            } else {
                arr[k++] = temp[j++];
            }
            swaps++;
        }
        
        // Copy remaining elements
        while (i <= mid) {
            arr[k++] = temp[i++];
            swaps++;
        }
        
        while (j <= right) {
            arr[k++] = temp[j++];
            swaps++;
        }
    }

    // ========== HeapSort Implementation ==========

    template<typename T, typename Compare>
    SortingResult HeapSort<T, Compare>::sort(std::vector<T>& arr, Compare comp) {
        auto start_time = std::chrono::high_resolution_clock::now();
        
        size_t comparisons = 0, swaps = 0;
        int n = static_cast<int>(arr.size());
        
        if (n > 1) {
            // Build max heap
            build_heap(arr, comp, comparisons, swaps);
            
            // Extract elements from heap one by one
            for (int i = n - 1; i > 0; --i) {
                std::swap(arr[0], arr[i]);
                swaps++;
                
                heapify(arr, i, 0, comp, comparisons, swaps);
            }
        }
        
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
        
        return {
            "HeapSort",
            duration,
            comparisons,
            swaps,
            arr.size(),
            false,  // HeapSort is not stable
            "O(n log n)",
            "O(1)"
        };
    }

    template<typename T, typename Compare>
    void HeapSort<T, Compare>::build_heap(std::vector<T>& arr, Compare& comp, 
                                         size_t& comparisons, size_t& swaps) {
        int n = static_cast<int>(arr.size());
        
        // Start from the last non-leaf node and heapify each node
        for (int i = n / 2 - 1; i >= 0; --i) {
            heapify(arr, n, i, comp, comparisons, swaps);
        }
    }

    template<typename T, typename Compare>
    void HeapSort<T, Compare>::heapify(std::vector<T>& arr, int n, int i, Compare& comp, 
                                      size_t& comparisons, size_t& swaps) {
        int largest = i;
        int left = 2 * i + 1;
        int right = 2 * i + 2;
        
        if (left < n) {
            comparisons++;
            if (comp(arr[largest], arr[left])) {
                largest = left;
            }
        }
        
        if (right < n) {
            comparisons++;
            if (comp(arr[largest], arr[right])) {
                largest = right;
            }
        }
        
        if (largest != i) {
            std::swap(arr[i], arr[largest]);
            swaps++;
            heapify(arr, n, largest, comp, comparisons, swaps);
        }
    }

    // ========== RadixSort Implementation ==========

    SortingResult RadixSort::sort(std::vector<int>& arr, Variant variant) {
        auto start_time = std::chrono::high_resolution_clock::now();
        
        size_t comparisons = 0, swaps = 0;
        
        if (!arr.empty()) {
            if (variant == Variant::LSD) {
                radix_sort_lsd(arr, comparisons, swaps);
            } else {
                radix_sort_msd(arr, comparisons, swaps);
            }
        }
        
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
        
        return {
            "RadixSort",
            duration,
            comparisons,
            swaps,
            arr.size(),
            true,  // RadixSort is stable
            "O(d * (n + k))",
            "O(n + k)"
        };
    }

    void RadixSort::radix_sort_lsd(std::vector<int>& arr, size_t& comparisons, size_t& swaps) {
        int max_val = get_max(arr);
        
        for (int exp = 1; max_val / exp > 0; exp *= 10) {
            counting_sort_for_radix(arr, exp, comparisons, swaps);
        }
    }

    void RadixSort::counting_sort_for_radix(std::vector<int>& arr, int exp, 
                                          size_t& comparisons, size_t& swaps) {
        int n = static_cast<int>(arr.size());
        std::vector<int> output(n);
        std::vector<int> count(10, 0);
        
        // Count occurrences of each digit
        for (int i = 0; i < n; ++i) {
            count[(arr[i] / exp) % 10]++;
        }
        
        // Change count[i] so that it contains actual position of digit in output[]
        for (int i = 1; i < 10; ++i) {
            count[i] += count[i - 1];
        }
        
        // Build output array
        for (int i = n - 1; i >= 0; --i) {
            output[count[(arr[i] / exp) % 10] - 1] = arr[i];
            count[(arr[i] / exp) % 10]--;
            swaps++;
        }
        
        // Copy output array to arr[]
        for (int i = 0; i < n; ++i) {
            arr[i] = output[i];
            swaps++;
        }
    }

    int RadixSort::get_max(const std::vector<int>& arr) {
        int max_val = arr[0];
        for (size_t i = 1; i < arr.size(); ++i) {
            if (arr[i] > max_val) {
                max_val = arr[i];
            }
        }
        return max_val;
    }

    // ========== SortingBenchmark Implementation ==========

    SortingBenchmark::BenchmarkResult 
    SortingBenchmark::run_comprehensive_benchmark(size_t array_size, DataPattern pattern) {
        std::vector<int> original_data = generate_test_data(array_size, pattern);
        BenchmarkResult result;
        
        std::cout << "🏁 Running comprehensive benchmark with " << array_size 
                  << " elements...\n\n";
        
        // Test different sorting algorithms
        std::vector<std::function<SortingResult(std::vector<int>&)>> algorithms = {
            [](std::vector<int>& arr) { return QuickSort<int>::sort(arr); },
            [](std::vector<int>& arr) { return MergeSort<int>::sort(arr); },
            [](std::vector<int>& arr) { return HeapSort<int>::sort(arr); },
            [](std::vector<int>& arr) { return InsertionSort<int>::sort(arr); },
            [](std::vector<int>& arr) { return RadixSort::sort(arr); }
        };
        
        for (auto& algorithm : algorithms) {
            std::vector<int> test_data = original_data;  // Copy for each test
            
            auto sorting_result = algorithm(test_data);
            result.results.push_back(sorting_result);
            
            std::cout << "✅ " << sorting_result.algorithm_name << " completed in " 
                      << sorting_result.execution_time.count() << " μs\n";
        }
        
        // Find fastest algorithm
        auto fastest = std::min_element(result.results.begin(), result.results.end(),
                                      [](const SortingResult& a, const SortingResult& b) {
                                          return a.execution_time < b.execution_time;
                                      });
        
        result.fastest_algorithm = fastest->algorithm_name;
        
        // Find most stable algorithms
        std::vector<std::string> stable_algorithms;
        for (const auto& res : result.results) {
            if (res.is_stable) {
                stable_algorithms.push_back(res.algorithm_name);
            }
        }
        
        result.most_stable = stable_algorithms.empty() ? "None" : stable_algorithms[0];
        result.best_for_pattern = fastest->algorithm_name;
        
        return result;
    }

    std::vector<int> SortingBenchmark::generate_test_data(size_t size, DataPattern pattern, 
                                                        int min_val, int max_val) {
        switch (pattern) {
            case DataPattern::RANDOM:
                return generate_random_data(size, min_val, max_val);
            case DataPattern::NEARLY_SORTED:
                return generate_nearly_sorted_data(size);
            case DataPattern::REVERSE_SORTED: {
                auto data = generate_random_data(size, min_val, max_val);
                std::sort(data.rbegin(), data.rend());
                return data;
            }
            case DataPattern::MANY_DUPLICATES:
                return generate_many_duplicates_data(size);
            case DataPattern::SORTED: {
                auto data = generate_random_data(size, min_val, max_val);
                std::sort(data.begin(), data.end());
                return data;
            }
            case DataPattern::SAWTOOTH:
                return generate_sawtooth_data(size);
            case DataPattern::ORGAN_PIPE:
                return generate_organ_pipe_data(size);
            default:
                return generate_random_data(size, min_val, max_val);
        }
    }

    std::vector<int> SortingBenchmark::generate_random_data(size_t size, int min_val, int max_val) {
        std::uniform_int_distribution<int> dist(min_val, max_val);
        std::vector<int> data(size);
        
        for (size_t i = 0; i < size; ++i) {
            data[i] = dist(gen_);
        }
        
        return data;
    }

    std::vector<int> SortingBenchmark::generate_nearly_sorted_data(size_t size, double sorted_fraction) {
        std::vector<int> data(size);
        
        // Generate sorted sequence
        for (size_t i = 0; i < size; ++i) {
            data[i] = static_cast<int>(i);
        }
        
        // Randomly shuffle a fraction of elements
        size_t num_to_shuffle = static_cast<size_t>(size * (1.0 - sorted_fraction));
        std::uniform_int_distribution<size_t> dist(0, size - 1);
        
        for (size_t i = 0; i < num_to_shuffle; ++i) {
            size_t idx1 = dist(gen_);
            size_t idx2 = dist(gen_);
            std::swap(data[idx1], data[idx2]);
        }
        
        return data;
    }

    std::vector<int> SortingBenchmark::generate_many_duplicates_data(size_t size, int num_unique) {
        std::uniform_int_distribution<int> dist(0, num_unique - 1);
        std::vector<int> data(size);
        
        for (size_t i = 0; i < size; ++i) {
            data[i] = dist(gen_);
        }
        
        return data;
    }

    void SortingBenchmark::print_benchmark_results(const BenchmarkResult& result) {
        std::cout << "\n📊 === SORTING BENCHMARK RESULTS ===\n";
        std::cout << std::left << std::setw(15) << "Algorithm"
                  << std::setw(12) << "Time (μs)"
                  << std::setw(12) << "Comparisons"
                  << std::setw(10) << "Swaps"
                  << std::setw(8) << "Stable"
                  << std::setw(20) << "Time Complexity"
                  << std::setw(15) << "Space Complexity\n";
        std::cout << std::string(100, '-') << std::endl;
        
        for (const auto& res : result.results) {
            std::cout << std::left << std::setw(15) << res.algorithm_name
                      << std::setw(12) << res.execution_time.count()
                      << std::setw(12) << res.comparisons
                      << std::setw(10) << res.swaps
                      << std::setw(8) << (res.is_stable ? "Yes" : "No")
                      << std::setw(20) << res.time_complexity
                      << std::setw(15) << res.space_complexity << std::endl;
        }
        
        std::cout << std::string(100, '-') << std::endl;
        std::cout << "🏆 Fastest Algorithm: " << result.fastest_algorithm << std::endl;
        std::cout << "🛡️  Most Stable: " << result.most_stable << std::endl;
        std::cout << "🎯 Best for Pattern: " << result.best_for_pattern << std::endl << std::endl;
    }

    // ========== SortingAlgorithmsDemo Implementation ==========

    void SortingAlgorithmsDemo::demonstrate_basic_sorting_algorithms() {
        print_section_header("Basic Sorting Algorithms");
        
        auto test_data = generate_sample_data(1000, "random");
        
        std::cout << "Testing with 1000 random integers...\n\n";
        
        // QuickSort
        {
            auto data_copy = test_data;
            auto result = QuickSort<int>::sort(data_copy);
            print_sorting_result(result);
        }
        
        // MergeSort
        {
            auto data_copy = test_data;
            auto result = MergeSort<int>::sort(data_copy);
            print_sorting_result(result);
        }
        
        // HeapSort
        {
            auto data_copy = test_data;
            auto result = HeapSort<int>::sort(data_copy);
            print_sorting_result(result);
        }
        
        // InsertionSort
        {
            auto small_data = generate_sample_data(100, "random"); // Smaller for insertion sort
            auto result = InsertionSort<int>::sort(small_data);
            print_sorting_result(result);
        }
        
        print_section_footer();
    }

    void SortingAlgorithmsDemo::demonstrate_specialized_sorting() {
        print_section_header("Specialized Sorting Algorithms");
        
        // RadixSort for integers
        {
            auto int_data = generate_sample_data(5000, "random");
            auto result = RadixSort::sort(int_data);
            print_sorting_result(result);
        }
        
        // CountingSort
        {
            std::vector<int> small_range_data(1000);
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_int_distribution<int> dist(0, 100);
            
            for (auto& val : small_range_data) {
                val = dist(gen);
            }
            
            auto result = CountingSort::sort(small_range_data);
            print_sorting_result(result);
        }
        
        print_section_footer();
    }

    void SortingAlgorithmsDemo::demonstrate_performance_analysis() {
        print_section_header("Performance Analysis");
        
        std::vector<SortingBenchmark::DataPattern> patterns = {
            SortingBenchmark::DataPattern::RANDOM,
            SortingBenchmark::DataPattern::NEARLY_SORTED,
            SortingBenchmark::DataPattern::REVERSE_SORTED,
            SortingBenchmark::DataPattern::MANY_DUPLICATES
        };
        
        std::vector<std::string> pattern_names = {
            "Random", "Nearly Sorted", "Reverse Sorted", "Many Duplicates"
        };
        
        for (size_t i = 0; i < patterns.size(); ++i) {
            std::cout << "📈 Testing with " << pattern_names[i] << " data pattern:\n";
            auto benchmark_result = SortingBenchmark::run_comprehensive_benchmark(5000, patterns[i]);
            SortingBenchmark::print_benchmark_results(benchmark_result);
        }
        
        print_section_footer();
    }

    void SortingAlgorithmsDemo::run_comprehensive_sorting_demo() {
        std::cout << "\n🎯 ===============================================\n";
        std::cout << "🎯 COMPREHENSIVE SORTING ALGORITHMS DEMONSTRATION\n";
        std::cout << "🎯 ===============================================\n\n";
        
        demonstrate_basic_sorting_algorithms();
        demonstrate_specialized_sorting();
        demonstrate_performance_analysis();
        
        std::cout << "\n🎉 =====================================\n";
        std::cout << "🎉 ALL SORTING DEMONSTRATIONS COMPLETED!\n";
        std::cout << "🎉 =====================================\n\n";
    }

    void SortingAlgorithmsDemo::print_section_header(const std::string& title) {
        std::cout << "\n" << std::string(60, '=') << std::endl;
        std::cout << "🔢 " << title << std::endl;
        std::cout << std::string(60, '=') << std::endl << std::endl;
    }

    void SortingAlgorithmsDemo::print_section_footer() {
        std::cout << std::string(60, '-') << std::endl;
        std::cout << "✅ Section Complete\n" << std::endl;
    }

    void SortingAlgorithmsDemo::print_sorting_result(const SortingResult& result) {
        std::cout << "📊 " << result.algorithm_name << " Results:\n";
        std::cout << "   Time: " << result.execution_time.count() << " μs\n";
        std::cout << "   Comparisons: " << result.comparisons << "\n";
        std::cout << "   Swaps: " << result.swaps << "\n";
        std::cout << "   Stable: " << (result.is_stable ? "Yes" : "No") << "\n";
        std::cout << "   Time Complexity: " << result.time_complexity << "\n";
        std::cout << "   Space Complexity: " << result.space_complexity << "\n\n";
    }

    std::vector<int> SortingAlgorithmsDemo::generate_sample_data(size_t size, const std::string& pattern) {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<int> dist(1, static_cast<int>(size * 10));
        
        std::vector<int> data(size);
        for (size_t i = 0; i < size; ++i) {
            data[i] = dist(gen);
        }
        
        return data;
    }

    // ========== CountingSort Implementation ==========

    SortingResult CountingSort::sort(std::vector<int>& arr, int min_val, int max_val) {
        auto start_time = std::chrono::high_resolution_clock::now();
        
        if (arr.empty()) {
            return {"CountingSort", std::chrono::microseconds(0), 0, 0, 0, true, "O(n + k)", "O(k)"};
        }
        
        // Find range if not provided
        if (min_val == INT_MIN || max_val == INT_MAX) {
            find_range(arr, min_val, max_val);
        }
        
        size_t range = static_cast<size_t>(max_val - min_val + 1);
        std::vector<int> count(range, 0);
        std::vector<int> output(arr.size());
        
        size_t comparisons = 0, swaps = 0;
        
        // Count occurrences
        for (int val : arr) {
            count[val - min_val]++;
        }
        
        // Transform count array to position array
        for (size_t i = 1; i < range; ++i) {
            count[i] += count[i - 1];
        }
        
        // Build output array
        for (int i = static_cast<int>(arr.size()) - 1; i >= 0; --i) {
            output[count[arr[i] - min_val] - 1] = arr[i];
            count[arr[i] - min_val]--;
            swaps++;
        }
        
        // Copy back to original array
        for (size_t i = 0; i < arr.size(); ++i) {
            arr[i] = output[i];
            swaps++;
        }
        
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
        
        return {
            "CountingSort",
            duration,
            comparisons,
            swaps,
            arr.size(),
            true,
            "O(n + k)",
            "O(k)"
        };
    }

    void CountingSort::find_range(const std::vector<int>& arr, int& min_val, int& max_val) {
        if (arr.empty()) return;
        
        min_val = max_val = arr[0];
        for (int val : arr) {
            min_val = std::min(min_val, val);
            max_val = std::max(max_val, val);
        }
    }

    // Explicit template instantiations for common types
    template class QuickSort<int>;
    template class MergeSort<int>;
    template class HeapSort<int>;
    template class InsertionSort<int>;
    template class SelectionSort<int>;
    template class BubbleSort<int>;

} // namespace CppVerseHub::Algorithms