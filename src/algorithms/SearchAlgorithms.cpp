/**
 * @file SearchAlgorithms.cpp
 * @brief Implementation of comprehensive search algorithm demonstrations
 * @details File location: src/algorithms/SearchAlgorithms.cpp
 */

#include "SearchAlgorithms.hpp"
#include <random>
#include <cmath>
#include <sstream>

namespace CppVerseHub::Algorithms {

    // ========== LinearSearch Implementation ==========

    template<typename T>
    SearchResult LinearSearch<T>::search(const std::vector<T>& arr, const T& target, Variant variant) {
        switch (variant) {
            case Variant::STANDARD:
                return linear_search_standard(arr, target);
            case Variant::SENTINEL:
                return linear_search_sentinel(arr, target);
            case Variant::BIDIRECTIONAL:
                return linear_search_bidirectional(arr, target);
            case Variant::JUMP_SEARCH:
                return jump_search_impl(arr, target);
            default:
                return linear_search_standard(arr, target);
        }
    }

    template<typename T>
    SearchResult LinearSearch<T>::linear_search_standard(const std::vector<T>& arr, const T& target) {
        auto start_time = std::chrono::high_resolution_clock::now();
        
        size_t comparisons = 0;
        bool found = false;
        size_t position = 0;
        
        for (size_t i = 0; i < arr.size(); ++i) {
            comparisons++;
            if (arr[i] == target) {
                found = true;
                position = i;
                break;
            }
        }
        
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
        
        return {
            "Linear Search (Standard)",
            found,
            found ? std::vector<size_t>{position} : std::vector<size_t>{},
            duration,
            comparisons,
            comparisons,
            "O(n)",
            "O(1)",
            "Sequential search through array"
        };
    }

    template<typename T>
    SearchResult LinearSearch<T>::jump_search_impl(const std::vector<T>& arr, const T& target) {
        auto start_time = std::chrono::high_resolution_clock::now();
        
        size_t n = arr.size();
        size_t jump = static_cast<size_t>(std::sqrt(n));
        size_t comparisons = 0;
        size_t prev = 0;
        
        // Jump through the array
        while (prev < n && arr[std::min(jump, n) - 1] < target) {
            comparisons++;
            prev = jump;
            jump += static_cast<size_t>(std::sqrt(n));
        }
        
        // Linear search in the identified block
        for (size_t i = prev; i < std::min(jump, n); ++i) {
            comparisons++;
            if (arr[i] == target) {
                auto end_time = std::chrono::high_resolution_clock::now();
                auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
                
                return {
                    "Jump Search",
                    true,
                    std::vector<size_t>{i},
                    duration,
                    comparisons,
                    comparisons,
                    "O(√n)",
                    "O(1)",
                    "Jump by √n steps, then linear search in block"
                };
            }
        }
        
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
        
        return {
            "Jump Search",
            false,
            std::vector<size_t>{},
            duration,
            comparisons,
            comparisons,
            "O(√n)",
            "O(1)",
            "Element not found"
        };
    }

    // ========== BinarySearch Implementation ==========

    template<typename T>
    SearchResult BinarySearch<T>::search(const std::vector<T>& arr, const T& target, Variant variant) {
        switch (variant) {
            case Variant::ITERATIVE:
                return binary_search_iterative(arr, target);
            case Variant::RECURSIVE: {
                size_t comparisons = 0;
                auto result = binary_search_recursive(arr, target, 0, static_cast<int>(arr.size()) - 1, comparisons);
                result.comparisons = comparisons;
                return result;
            }
            case Variant::LEFTMOST:
                return find_leftmost(arr, target);
            case Variant::RIGHTMOST:
                return find_rightmost(arr, target);
            default:
                return binary_search_iterative(arr, target);
        }
    }

    template<typename T>
    SearchResult BinarySearch<T>::binary_search_iterative(const std::vector<T>& arr, const T& target) {
        auto start_time = std::chrono::high_resolution_clock::now();
        
        int left = 0;
        int right = static_cast<int>(arr.size()) - 1;
        size_t comparisons = 0;
        size_t iterations = 0;
        
        while (left <= right) {
            iterations++;
            int mid = left + (right - left) / 2;
            
            comparisons++;
            if (arr[mid] == target) {
                auto end_time = std::chrono::high_resolution_clock::now();
                auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
                
                return {
                    "Binary Search (Iterative)",
                    true,
                    std::vector<size_t>{static_cast<size_t>(mid)},
                    duration,
                    comparisons,
                    iterations,
                    "O(log n)",
                    "O(1)",
                    "Found at position " + std::to_string(mid)
                };
            }
            
            comparisons++;
            if (arr[mid] < target) {
                left = mid + 1;
            } else {
                right = mid - 1;
            }
        }
        
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
        
        return {
            "Binary Search (Iterative)",
            false,
            std::vector<size_t>{},
            duration,
            comparisons,
            iterations,
            "O(log n)",
            "O(1)",
            "Element not found"
        };
    }

    template<typename T>
    SearchResult BinarySearch<T>::find_leftmost(const std::vector<T>& arr, const T& target) {
        auto start_time = std::chrono::high_resolution_clock::now();
        
        int left = 0;
        int right = static_cast<int>(arr.size());
        size_t comparisons = 0;
        size_t iterations = 0;
        
        while (left < right) {
            iterations++;
            int mid = left + (right - left) / 2;
            
            comparisons++;
            if (arr[mid] < target) {
                left = mid + 1;
            } else {
                right = mid;
            }
        }
        
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
        
        bool found = left < static_cast<int>(arr.size()) && arr[left] == target;
        
        return {
            "Binary Search (Leftmost)",
            found,
            found ? std::vector<size_t>{static_cast<size_t>(left)} : std::vector<size_t>{},
            duration,
            comparisons,
            iterations,
            "O(log n)",
            "O(1)",
            found ? "Leftmost occurrence at position " + std::to_string(left) : "Element not found"
        };
    }

    // ========== StringSearch Implementation ==========

    SearchResult StringSearch::search(const std::string& text, const std::string& pattern, Algorithm algorithm) {
        switch (algorithm) {
            case Algorithm::NAIVE:
                return naive_search(text, pattern);
            case Algorithm::KMP:
                return kmp_search(text, pattern);
            case Algorithm::BOYER_MOORE:
                return boyer_moore_search(text, pattern);
            case Algorithm::RABIN_KARP:
                return rabin_karp_search(text, pattern);
            case Algorithm::Z_ALGORITHM:
                return z_algorithm_search(text, pattern);
            default:
                return kmp_search(text, pattern);
        }
    }

    SearchResult StringSearch::kmp_search(const std::string& text, const std::string& pattern) {
        auto start_time = std::chrono::high_resolution_clock::now();
        
        if (pattern.empty()) {
            auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
                std::chrono::high_resolution_clock::now() - start_time);
            return {"KMP Search", false, {}, duration, 0, 0, "O(n + m)", "O(m)", "Empty pattern"};
        }
        
        std::vector<int> lps = compute_lps(pattern);
        size_t comparisons = 0;
        size_t iterations = 0;
        
        int i = 0; // index for text
        int j = 0; // index for pattern
        
        while (i < static_cast<int>(text.length())) {
            iterations++;
            comparisons++;
            
            if (pattern[j] == text[i]) {
                i++;
                j++;
            }
            
            if (j == static_cast<int>(pattern.length())) {
                // Found pattern
                auto end_time = std::chrono::high_resolution_clock::now();
                auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
                
                return {
                    "KMP Search",
                    true,
                    std::vector<size_t>{static_cast<size_t>(i - j)},
                    duration,
                    comparisons,
                    iterations,
                    "O(n + m)",
                    "O(m)",
                    "Pattern found at position " + std::to_string(i - j)
                };
            } else if (i < static_cast<int>(text.length()) && pattern[j] != text[i]) {
                if (j != 0) {
                    j = lps[j - 1];
                } else {
                    i++;
                }
            }
        }
        
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
        
        return {
            "KMP Search",
            false,
            std::vector<size_t>{},
            duration,
            comparisons,
            iterations,
            "O(n + m)",
            "O(m)",
            "Pattern not found"
        };
    }

    std::vector<int> StringSearch::compute_lps(const std::string& pattern) {
        int m = static_cast<int>(pattern.length());
        std::vector<int> lps(m, 0);
        int len = 0;
        int i = 1;
        
        while (i < m) {
            if (pattern[i] == pattern[len]) {
                len++;
                lps[i] = len;
                i++;
            } else {
                if (len != 0) {
                    len = lps[len - 1];
                } else {
                    lps[i] = 0;
                    i++;
                }
            }
        }
        
        return lps;
    }

    SearchResult StringSearch::rabin_karp_search(const std::string& text, const std::string& pattern) {
        auto start_time = std::chrono::high_resolution_clock::now();
        
        int n = static_cast<int>(text.length());
        int m = static_cast<int>(pattern.length());
        
        if (m > n || m == 0) {
            auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
                std::chrono::high_resolution_clock::now() - start_time);
            return {"Rabin-Karp Search", false, {}, duration, 0, 0, "O(n)", "O(1)", 
                    m == 0 ? "Empty pattern" : "Pattern longer than text"};
        }
        
        size_t pattern_hash = rolling_hash(pattern, 0, m);
        size_t text_hash = rolling_hash(text, 0, m);
        
        size_t comparisons = 0;
        size_t iterations = 0;
        
        // Calculate h = pow(HASH_BASE, m-1) % HASH_MOD
        size_t h = 1;
        for (int i = 0; i < m - 1; i++) {
            h = (h * HASH_BASE) % HASH_MOD;
        }
        
        for (int i = 0; i <= n - m; i++) {
            iterations++;
            
            if (pattern_hash == text_hash) {
                // Hash match - verify character by character
                bool match = true;
                for (int j = 0; j < m; j++) {
                    comparisons++;
                    if (text[i + j] != pattern[j]) {
                        match = false;
                        break;
                    }
                }
                
                if (match) {
                    auto end_time = std::chrono::high_resolution_clock::now();
                    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
                    
                    return {
                        "Rabin-Karp Search",
                        true,
                        std::vector<size_t>{static_cast<size_t>(i)},
                        duration,
                        comparisons,
                        iterations,
                        "O(n) average, O(nm) worst",
                        "O(1)",
                        "Pattern found at position " + std::to_string(i)
                    };
                }
            }
            
            // Calculate rolling hash for next window
            if (i < n - m) {
                text_hash = (HASH_BASE * (text_hash - text[i] * h) + text[i + m]) % HASH_MOD;
                if (text_hash < 0) {
                    text_hash = (text_hash + HASH_MOD) % HASH_MOD;
                }
            }
        }
        
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
        
        return {
            "Rabin-Karp Search",
            false,
            std::vector<size_t>{},
            duration,
            comparisons,
            iterations,
            "O(n) average, O(nm) worst",
            "O(1)",
            "Pattern not found"
        };
    }

    size_t StringSearch::rolling_hash(const std::string& str, size_t start, size_t length) {
        size_t hash_value = 0;
        for (size_t i = start; i < start + length; i++) {
            hash_value = (hash_value * HASH_BASE + str[i]) % HASH_MOD;
        }
        return hash_value;
    }

    // ========== Graph Implementation ==========

    template<typename T>
    Graph<T>::Graph(size_t num_vertices, bool directed) : directed_(directed) {
        adjacency_list_.resize(num_vertices);
        vertex_data_.resize(num_vertices);
    }

    template<typename T>
    void Graph<T>::add_edge(size_t from, size_t to, T weight) {
        if (from >= adjacency_list_.size() || to >= adjacency_list_.size()) {
            return;
        }
        
        adjacency_list_[from].emplace_back(from, to, weight);
        if (!directed_) {
            adjacency_list_[to].emplace_back(to, from, weight);
        }
    }

    template<typename T>
    void Graph<T>::add_vertex_data(size_t vertex, const std::string& data) {
        if (vertex < vertex_data_.size()) {
            vertex_data_[vertex] = data;
        }
    }

    template<typename T>
    const std::vector<typename Graph<T>::Edge>& Graph<T>::get_adjacent(size_t vertex) const {
        static std::vector<Edge> empty;
        return vertex < adjacency_list_.size() ? adjacency_list_[vertex] : empty;
    }

    template<typename T>
    const std::string& Graph<T>::get_vertex_data(size_t vertex) const {
        static std::string empty;
        return vertex < vertex_data_.size() ? vertex_data_[vertex] : empty;
    }

    // ========== GraphSearch Implementation ==========

    template<typename T>
    SearchResult GraphSearch<T>::breadth_first_search(const Graph<T>& graph, size_t start, size_t target) {
        return bfs_impl(graph, start, target);
    }

    template<typename T>
    SearchResult GraphSearch<T>::bfs_impl(const Graph<T>& graph, size_t start, size_t target) {
        auto start_time = std::chrono::high_resolution_clock::now();
        
        if (start >= graph.vertex_count() || target >= graph.vertex_count()) {
            auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
                std::chrono::high_resolution_clock::now() - start_time);
            return {"BFS", false, {}, duration, 0, 0, "O(V + E)", "O(V)", "Invalid vertices"};
        }
        
        std::vector<bool> visited(graph.vertex_count(), false);
        std::vector<size_t> parent(graph.vertex_count(), SIZE_MAX);
        std::queue<size_t> queue;
        
        size_t nodes_visited = 0;
        size_t iterations = 0;
        
        queue.push(start);
        visited[start] = true;
        parent[start] = start;
        
        while (!queue.empty()) {
            iterations++;
            size_t current = queue.front();
            queue.pop();
            nodes_visited++;
            
            if (current == target) {
                // Reconstruct path
                std::vector<size_t> path;
                size_t vertex = target;
                while (vertex != start) {
                    path.push_back(vertex);
                    vertex = parent[vertex];
                }
                path.push_back(start);
                std::reverse(path.begin(), path.end());
                
                auto end_time = std::chrono::high_resolution_clock::now();
                auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
                
                return {
                    "BFS",
                    true,
                    path,
                    duration,
                    nodes_visited,
                    iterations,
                    "O(V + E)",
                    "O(V)",
                    "Path found with " + std::to_string(path.size() - 1) + " edges"
                };
            }
            
            for (const auto& edge : graph.get_adjacent(current)) {
                if (!visited[edge.to]) {
                    visited[edge.to] = true;
                    parent[edge.to] = current;
                    queue.push(edge.to);
                }
            }
        }
        
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
        
        return {
            "BFS",
            false,
            std::vector<size_t>{},
            duration,
            nodes_visited,
            iterations,
            "O(V + E)",
            "O(V)",
            "No path found"
        };
    }

    // ========== SearchAlgorithmsDemo Implementation ==========

    void SearchAlgorithmsDemo::demonstrate_basic_search_algorithms() {
        print_section_header("Basic Search Algorithms");
        
        // Generate test data
        auto unsorted_data = generate_sample_data(1000, false);
        auto sorted_data = generate_sample_data(1000, true);
        
        int target = sorted_data[500]; // Ensure target exists
        
        std::cout << "Testing with arrays of 1000 elements, searching for: " << target << "\n\n";
        
        // Linear Search on unsorted data
        {
            auto result = LinearSearch<int>::search(unsorted_data, target);
            print_search_result(result);
        }
        
        // Binary Search on sorted data
        {
            auto result = BinarySearch<int>::search(sorted_data, target);
            print_search_result(result);
        }
        
        // Jump Search on sorted data
        {
            auto result = LinearSearch<int>::search(sorted_data, target, LinearSearch<int>::Variant::JUMP_SEARCH);
            print_search_result(result);
        }
        
        // Interpolation Search on sorted data
        {
            auto result = InterpolationSearch<int>::search(sorted_data, target);
            print_search_result(result);
        }
        
        print_section_footer();
    }

    void SearchAlgorithmsDemo::demonstrate_string_search_algorithms() {
        print_section_header("String Search Algorithms");
        
        std::string text = "The quick brown fox jumps over the lazy dog. The fox is quick and the dog is lazy.";
        std::string pattern = "quick";
        
        std::cout << "Searching for pattern '" << pattern << "' in text:\n";
        std::cout << "\"" << text.substr(0, 60) << "...\"\n\n";
        
        // Test different string search algorithms
        std::vector<StringSearch::Algorithm> algorithms = {
            StringSearch::Algorithm::NAIVE,
            StringSearch::Algorithm::KMP,
            StringSearch::Algorithm::BOYER_MOORE,
            StringSearch::Algorithm::RABIN_KARP
        };
        
        for (auto algorithm : algorithms) {
            auto result = StringSearch::search(text, pattern, algorithm);
            print_search_result(result);
        }
        
        print_section_footer();
    }

    void SearchAlgorithmsDemo::demonstrate_graph_search_algorithms() {
        print_section_header("Graph Search Algorithms");
        
        // Create a sample graph
        auto graph = generate_sample_graph(8);
        
        std::cout << "Testing graph search algorithms on 8-vertex graph\n";
        std::cout << "Searching path from vertex 0 to vertex 7\n\n";
        
        // BFS
        {
            auto result = GraphSearch<int>::breadth_first_search(graph, 0, 7);
            print_search_result(result);
        }
        
        // DFS
        {
            auto result = GraphSearch<int>::depth_first_search(graph, 0, 7);
            print_search_result(result);
        }
        
        print_section_footer();
    }

    void SearchAlgorithmsDemo::run_comprehensive_search_demo() {
        std::cout << "\n🎯 ============================================\n";
        std::cout << "🎯 COMPREHENSIVE SEARCH ALGORITHMS DEMONSTRATION\n";
        std::cout << "🎯 ============================================\n\n";
        
        demonstrate_basic_search_algorithms();
        demonstrate_string_search_algorithms();
        demonstrate_graph_search_algorithms();
        
        std::cout << "\n🎉 ===================================\n";
        std::cout << "🎉 ALL SEARCH DEMONSTRATIONS COMPLETED!\n";
        std::cout << "🎉 ===================================\n\n";
    }

    void SearchAlgorithmsDemo::print_section_header(const std::string& title) {
        std::cout << "\n" << std::string(60, '=') << std::endl;
        std::cout << "🔍 " << title << std::endl;
        std::cout << std::string(60, '=') << std::endl << std::endl;
    }

    void SearchAlgorithmsDemo::print_section_footer() {
        std::cout << std::string(60, '-') << std::endl;
        std::cout << "✅ Section Complete\n" << std::endl;
    }

    void SearchAlgorithmsDemo::print_search_result(const SearchResult& result) {
        std::cout << "🔍 " << result.algorithm_name << " Results:\n";
        std::cout << "   Found: " << (result.found ? "Yes" : "No") << "\n";
        
        if (result.found && !result.positions.empty()) {
            std::cout << "   Position(s): ";
            for (size_t i = 0; i < result.positions.size(); ++i) {
                if (i > 0) std::cout << ", ";
                std::cout << result.positions[i];
            }
            std::cout << "\n";
        }
        
        std::cout << "   Time: " << result.execution_time.count() << " μs\n";
        std::cout << "   Comparisons: " << result.comparisons << "\n";
        std::cout << "   Iterations: " << result.iterations << "\n";
        std::cout << "   Time Complexity: " << result.time_complexity << "\n";
        std::cout << "   Space Complexity: " << result.space_complexity << "\n";
        
        if (!result.additional_info.empty()) {
            std::cout << "   Info: " << result.additional_info << "\n";
        }
        
        std::cout << std::endl;
    }

    std::vector<int> SearchAlgorithmsDemo::generate_sample_data(size_t size, bool sorted) {
        std::vector<int> data(size);
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<int> dist(1, static_cast<int>(size * 2));
        
        for (size_t i = 0; i < size; ++i) {
            data[i] = sorted ? static_cast<int>(i * 2 + 1) : dist(gen);
        }
        
        if (!sorted) {
            // Ensure we have some duplicates for testing
            for (size_t i = 0; i < size / 10; ++i) {
                data[i * 10] = data[0];
            }
        }
        
        return data;
    }

    Graph<int> SearchAlgorithmsDemo::generate_sample_graph(size_t vertices) {
        Graph<int> graph(vertices, false);
        
        // Create a connected graph with some cycles
        for (size_t i = 0; i < vertices - 1; ++i) {
            graph.add_edge(i, i + 1, static_cast<int>(i + 1));
        }
        
        // Add some additional edges to create cycles and alternative paths
        if (vertices >= 4) {
            graph.add_edge(0, 2, 3);
            graph.add_edge(1, 3, 2);
        }
        
        if (vertices >= 6) {
            graph.add_edge(2, 5, 4);
            graph.add_edge(3, 6, 5);
        }
        
        // Add vertex data
        for (size_t i = 0; i < vertices; ++i) {
            graph.add_vertex_data(i, "Vertex_" + std::to_string(i));
        }
        
        return graph;
    }

    // ========== InterpolationSearch Implementation ==========

    template<typename T>
    SearchResult InterpolationSearch<T>::search(const std::vector<T>& arr, const T& target) {
        return interpolation_search_impl(arr, target);
    }

    template<typename T>
    SearchResult InterpolationSearch<T>::interpolation_search_impl(const std::vector<T>& arr, const T& target) {
        auto start_time = std::chrono::high_resolution_clock::now();
        
        if (arr.empty()) {
            auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
                std::chrono::high_resolution_clock::now() - start_time);
            return {"Interpolation Search", false, {}, duration, 0, 0, "O(log log n)", "O(1)", "Empty array"};
        }
        
        size_t low = 0;
        size_t high = arr.size() - 1;
        size_t comparisons = 0;
        size_t iterations = 0;
        
        while (low <= high && target >= arr[low] && target <= arr[high]) {
            iterations++;
            
            if (low == high) {
                comparisons++;
                if (arr[low] == target) {
                    auto end_time = std::chrono::high_resolution_clock::now();
                    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
                    
                    return {
                        "Interpolation Search",
                        true,
                        std::vector<size_t>{low},
                        duration,
                        comparisons,
                        iterations,
                        "O(log log n) average, O(n) worst",
                        "O(1)",
                        "Found at position " + std::to_string(low)
                    };
                }
                break;
            }
            
            size_t pos = interpolate_position(arr, target, low, high);
            
            comparisons++;
            if (arr[pos] == target) {
                auto end_time = std::chrono::high_resolution_clock::now();
                auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
                
                return {
                    "Interpolation Search",
                    true,
                    std::vector<size_t>{pos},
                    duration,
                    comparisons,
                    iterations,
                    "O(log log n) average, O(n) worst",
                    "O(1)",
                    "Found at position " + std::to_string(pos)
                };
            }
            
            comparisons++;
            if (arr[pos] < target) {
                low = pos + 1;
            } else {
                high = pos - 1;
            }
        }
        
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
        
        return {
            "Interpolation Search",
            false,
            std::vector<size_t>{},
            duration,
            comparisons,
            iterations,
            "O(log log n) average, O(n) worst",
            "O(1)",
            "Element not found"
        };
    }

    template<typename T>
    size_t InterpolationSearch<T>::interpolate_position(const std::vector<T>& arr, const T& target, 
                                                       size_t low, size_t high) {
        if (arr[high] == arr[low]) {
            return low;
        }
        
        double ratio = static_cast<double>(target - arr[low]) / (arr[high] - arr[low]);
        size_t pos = low + static_cast<size_t>(ratio * (high - low));
        
        // Ensure pos is within bounds
        return std::min(std::max(pos, low), high);
    }

    // Explicit template instantiations for common types
    template class LinearSearch<int>;
    template class BinarySearch<int>;
    template class TernarySearch<int>;
    template class InterpolationSearch<int>;
    template class Graph<int>;
    template class GraphSearch<int>;

} // namespace CppVerseHub::Algorithms