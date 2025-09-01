/**
 * @file DataStructures.cpp
 * @brief Implementation of comprehensive data structure demonstrations
 * @details File location: src/algorithms/DataStructures.cpp
 */

#include "DataStructures.hpp"
#include <random>
#include <iomanip>
#include <sstream>
#include <unordered_map>
#include <queue>
#include <cmath>

namespace CppVerseHub::Algorithms {

    // ========== Trie Implementation ==========

    Trie::Trie() : root_(std::make_shared<TrieNode>()), word_count_(0) {}

    void Trie::insert(const std::string& word) {
        if (word.empty()) return;
        
        NodePtr current = root_;
        
        for (char c : word) {
            if (current->children.find(c) == current->children.end()) {
                current->children[c] = std::make_shared<TrieNode>();
            }
            current = current->children[c];
        }
        
        if (!current->is_end_of_word) {
            current->is_end_of_word = true;
            current->stored_word = word;
            word_count_++;
        }
        current->frequency++;
    }

    bool Trie::search(const std::string& word) const {
        if (word.empty()) return false;
        
        NodePtr current = root_;
        
        for (char c : word) {
            if (current->children.find(c) == current->children.end()) {
                return false;
            }
            current = current->children[c];
        }
        
        return current->is_end_of_word;
    }

    bool Trie::starts_with(const std::string& prefix) const {
        if (prefix.empty()) return true;
        
        NodePtr current = root_;
        
        for (char c : prefix) {
            if (current->children.find(c) == current->children.end()) {
                return false;
            }
            current = current->children[c];
        }
        
        return true;
    }

    std::vector<std::string> Trie::find_all_with_prefix(const std::string& prefix) const {
        std::vector<std::string> words;
        
        NodePtr current = root_;
        for (char c : prefix) {
            if (current->children.find(c) == current->children.end()) {
                return words; // Empty vector - prefix not found
            }
            current = current->children[c];
        }
        
        collect_words(current, prefix, words);
        return words;
    }

    std::vector<std::string> Trie::autocomplete(const std::string& prefix, size_t max_suggestions) const {
        std::vector<std::string> suggestions = find_all_with_prefix(prefix);
        
        if (suggestions.size() > max_suggestions) {
            suggestions.resize(max_suggestions);
        }
        
        return suggestions;
    }

    void Trie::increment_frequency(const std::string& word) {
        insert(word); // This will increment frequency
    }

    int Trie::get_frequency(const std::string& word) const {
        NodePtr current = root_;
        
        for (char c : word) {
            if (current->children.find(c) == current->children.end()) {
                return 0;
            }
            current = current->children[c];
        }
        
        return current->is_end_of_word ? current->frequency : 0;
    }

    std::vector<std::pair<std::string, int>> Trie::get_most_frequent(size_t count) const {
        std::vector<std::pair<std::string, int>> words_with_freq;
        collect_words_with_frequency(root_, "", words_with_freq);
        
        std::sort(words_with_freq.begin(), words_with_freq.end(),
                 [](const auto& a, const auto& b) { return a.second > b.second; });
        
        if (words_with_freq.size() > count) {
            words_with_freq.resize(count);
        }
        
        return words_with_freq;
    }

    void Trie::clear() {
        root_ = std::make_shared<TrieNode>();
        word_count_ = 0;
    }

    void Trie::print_all_words() const {
        std::cout << "Words in Trie (" << word_count_ << " total):\n";
        print_words_recursive(root_, "");
        std::cout << std::endl;
    }

    void Trie::collect_words(NodePtr node, const std::string& prefix, std::vector<std::string>& words) const {
        if (!node) return;
        
        if (node->is_end_of_word) {
            words.push_back(prefix);
        }
        
        for (const auto& [ch, child] : node->children) {
            collect_words(child, prefix + ch, words);
        }
    }

    void Trie::collect_words_with_frequency(NodePtr node, const std::string& prefix, 
                                          std::vector<std::pair<std::string, int>>& words) const {
        if (!node) return;
        
        if (node->is_end_of_word) {
            words.emplace_back(prefix, node->frequency);
        }
        
        for (const auto& [ch, child] : node->children) {
            collect_words_with_frequency(child, prefix + ch, words);
        }
    }

    void Trie::print_words_recursive(NodePtr node, const std::string& prefix) const {
        if (!node) return;
        
        if (node->is_end_of_word) {
            std::cout << "  " << prefix;
            if (node->frequency > 1) {
                std::cout << " (freq: " << node->frequency << ")";
            }
            std::cout << std::endl;
        }
        
        for (const auto& [ch, child] : node->children) {
            print_words_recursive(child, prefix + ch);
        }
    }

    // ========== DisjointSet Implementation ==========

    DisjointSet::DisjointSet(size_t n) : parent_(n), rank_(n, 0), size_(n, 1), num_sets_(n) {
        for (size_t i = 0; i < n; ++i) {
            parent_[i] = i;
        }
    }

    size_t DisjointSet::find(size_t x) {
        if (parent_[x] != x) {
            parent_[x] = find(parent_[x]); // Path compression
        }
        return parent_[x];
    }

    bool DisjointSet::unite(size_t x, size_t y) {
        size_t root_x = find(x);
        size_t root_y = find(y);
        
        if (root_x == root_y) {
            return false; // Already in same set
        }
        
        // Union by rank
        if (rank_[root_x] < rank_[root_y]) {
            parent_[root_x] = root_y;
            size_[root_y] += size_[root_x];
        } else if (rank_[root_x] > rank_[root_y]) {
            parent_[root_y] = root_x;
            size_[root_x] += size_[root_y];
        } else {
            parent_[root_y] = root_x;
            size_[root_x] += size_[root_y];
            rank_[root_x]++;
        }
        
        num_sets_--;
        return true;
    }

    bool DisjointSet::connected(size_t x, size_t y) {
        return find(x) == find(y);
    }

    size_t DisjointSet::set_size(size_t x) {
        return size_[find(x)];
    }

    std::vector<size_t> DisjointSet::get_set_members(size_t representative) {
        size_t root = find(representative);
        std::vector<size_t> members;
        
        for (size_t i = 0; i < parent_.size(); ++i) {
            if (find(i) == root) {
                members.push_back(i);
            }
        }
        
        return members;
    }

    void DisjointSet::print_sets() const {
        std::unordered_map<size_t, std::vector<size_t>> sets;
        
        for (size_t i = 0; i < parent_.size(); ++i) {
            size_t root = const_cast<DisjointSet*>(this)->find(i);
            sets[root].push_back(i);
        }
        
        std::cout << "Disjoint Sets (" << num_sets_ << " sets):\n";
        int set_id = 0;
        for (const auto& [root, members] : sets) {
            std::cout << "  Set " << set_id++ << " (root: " << root << "): {";
            for (size_t i = 0; i < members.size(); ++i) {
                if (i > 0) std::cout << ", ";
                std::cout << members[i];
            }
            std::cout << "}\n";
        }
        std::cout << std::endl;
    }

    size_t DisjointSet::get_max_set_size() const {
        return *std::max_element(size_.begin(), size_.end());
    }

    // ========== BloomFilter Implementation ==========

    BloomFilter::BloomFilter(size_t expected_elements, double false_positive_rate) 
        : target_false_positive_rate_(false_positive_rate), inserted_count_(0) {
        
        size_t m = optimal_bit_array_size(expected_elements, false_positive_rate);
        size_t k = optimal_num_hash_functions(m, expected_elements);
        
        bit_array_.resize(m, false);
        num_hash_functions_ = k;
    }

    void BloomFilter::insert(const std::string& item) {
        std::vector<size_t> hash_values = get_hash_values(item);
        
        for (size_t hash_val : hash_values) {
            bit_array_[hash_val % bit_array_.size()] = true;
        }
        
        inserted_count_++;
    }

    bool BloomFilter::possibly_contains(const std::string& item) const {
        std::vector<size_t> hash_values = get_hash_values(item);
        
        for (size_t hash_val : hash_values) {
            if (!bit_array_[hash_val % bit_array_.size()]) {
                return false;
            }
        }
        
        return true;
    }

    void BloomFilter::clear() {
        std::fill(bit_array_.begin(), bit_array_.end(), false);
        inserted_count_ = 0;
    }

    double BloomFilter::estimated_false_positive_rate() const {
        if (inserted_count_ == 0) return 0.0;
        
        double k = static_cast<double>(num_hash_functions_);
        double m = static_cast<double>(bit_array_.size());
        double n = static_cast<double>(inserted_count_);
        
        return std::pow(1.0 - std::exp(-k * n / m), k);
    }

    size_t BloomFilter::set_bits_count() const {
        return std::count(bit_array_.begin(), bit_array_.end(), true);
    }

    double BloomFilter::fill_ratio() const {
        return static_cast<double>(set_bits_count()) / bit_array_.size();
    }

    void BloomFilter::print_statistics() const {
        std::cout << "Bloom Filter Statistics:\n";
        std::cout << "  Size: " << bit_array_.size() << " bits\n";
        std::cout << "  Hash Functions: " << num_hash_functions_ << "\n";
        std::cout << "  Inserted Elements: " << inserted_count_ << "\n";
        std::cout << "  Set Bits: " << set_bits_count() << "\n";
        std::cout << "  Fill Ratio: " << std::fixed << std::setprecision(3) << fill_ratio() << "\n";
        std::cout << "  Estimated FP Rate: " << std::fixed << std::setprecision(6) 
                  << estimated_false_positive_rate() << std::endl;
    }

    std::vector<size_t> BloomFilter::get_hash_values(const std::string& item) const {
        std::vector<size_t> hash_values;
        hash_values.reserve(num_hash_functions_);
        
        for (size_t i = 0; i < num_hash_functions_; ++i) {
            hash_values.push_back(hash_function(item, i));
        }
        
        return hash_values;
    }

    size_t BloomFilter::hash_function(const std::string& item, size_t seed) const {
        std::hash<std::string> hasher;
        return hasher(item + std::to_string(seed));
    }

    size_t BloomFilter::optimal_bit_array_size(size_t n, double p) {
        return static_cast<size_t>(-n * std::log(p) / (std::log(2) * std::log(2)));
    }

    size_t BloomFilter::optimal_num_hash_functions(size_t m, size_t n) {
        if (n == 0) return 1;
        return static_cast<size_t>(std::round(static_cast<double>(m) / n * std::log(2)));
    }

    // ========== DataStructuresDemo Implementation ==========

    void DataStructuresDemo::demonstrate_trie() {
        print_section_header("Trie (Prefix Tree)");
        
        Trie trie;
        
        std::vector<std::string> words = {
            "hello", "world", "help", "hero", "helicopter", 
            "world", "word", "work", "working", "hello"
        };
        
        std::cout << "Inserting words: ";
        for (const auto& word : words) {
            std::cout << word << " ";
            trie.insert(word);
        }
        std::cout << "\n\n";
        
        // Test search operations
        std::vector<std::string> search_words = {"hello", "help", "xyz", "work"};
        std::cout << "Search results:\n";
        for (const auto& word : search_words) {
            bool found = trie.search(word);
            int freq = trie.get_frequency(word);
            std::cout << "  '" << word << "': " << (found ? "Found" : "Not found");
            if (found) {
                std::cout << " (frequency: " << freq << ")";
            }
            std::cout << "\n";
        }
        
        // Test prefix operations
        std::cout << "\nPrefix operations:\n";
        std::string prefix = "he";
        std::cout << "  Words with prefix '" << prefix << "':\n";
        auto prefix_words = trie.find_all_with_prefix(prefix);
        for (const auto& word : prefix_words) {
            std::cout << "    " << word << "\n";
        }
        
        // Autocomplete
        std::cout << "\nAutocomplete for 'wor': ";
        auto suggestions = trie.autocomplete("wor", 3);
        for (size_t i = 0; i < suggestions.size(); ++i) {
            if (i > 0) std::cout << ", ";
            std::cout << suggestions[i];
        }
        std::cout << std::endl;
        
        // Most frequent words
        std::cout << "\nMost frequent words:\n";
        auto frequent = trie.get_most_frequent(5);
        for (const auto& [word, freq] : frequent) {
            std::cout << "  " << word << ": " << freq << " times\n";
        }
        
        print_section_footer();
    }

    void DataStructuresDemo::demonstrate_disjoint_set() {
        print_section_header("Disjoint Set (Union-Find)");
        
        const size_t n = 10;
        DisjointSet ds(n);
        
        std::cout << "Initial state (each element in its own set):\n";
        ds.print_sets();
        
        // Perform union operations
        std::cout << "Performing unions:\n";
        std::vector<std::pair<size_t, size_t>> unions = {
            {0, 1}, {2, 3}, {0, 2}, {5, 6}, {7, 8}, {5, 7}
        };
        
        for (const auto& [x, y] : unions) {
            bool united = ds.unite(x, y);
            std::cout << "  Union(" << x << ", " << y << "): " 
                      << (united ? "Success" : "Already connected") << "\n";
        }
        
        std::cout << "\nAfter unions:\n";
        ds.print_sets();
        
        // Test connectivity
        std::cout << "Connectivity tests:\n";
        std::vector<std::pair<size_t, size_t>> tests = {
            {0, 3}, {1, 2}, {5, 8}, {0, 9}, {4, 9}
        };
        
        for (const auto& [x, y] : tests) {
            bool connected = ds.connected(x, y);
            std::cout << "  Connected(" << x << ", " << y << "): " 
                      << (connected ? "Yes" : "No") << "\n";
        }
        
        std::cout << "\nSet sizes:\n";
        for (size_t i = 0; i < n; ++i) {
            std::cout << "  Element " << i << " is in set of size " << ds.set_size(i) << "\n";
        }
        
        print_section_footer();
    }

    void DataStructuresDemo::demonstrate_bloom_filter() {
        print_section_header("Bloom Filter");
        
        BloomFilter bloom_filter(1000, 0.01); // Expected 1000 elements, 1% FP rate
        
        std::vector<std::string> items_to_insert = {
            "apple", "banana", "cherry", "date", "elderberry",
            "fig", "grape", "honeydew", "kiwi", "lemon"
        };
        
        std::cout << "Inserting items into Bloom Filter:\n";
        for (const auto& item : items_to_insert) {
            std::cout << "  " << item;
            bloom_filter.insert(item);
        }
        std::cout << "\n\n";
        
        // Test membership
        std::vector<std::string> test_items = {
            "apple", "orange", "banana", "mango", "cherry", "pineapple"
        };
        
        std::cout << "Membership tests:\n";
        for (const auto& item : test_items) {
            bool might_contain = bloom_filter.possibly_contains(item);
            bool actually_inserted = std::find(items_to_insert.begin(), 
                                             items_to_insert.end(), item) != items_to_insert.end();
            
            std::cout << "  '" << item << "': " << (might_contain ? "Possibly in set" : "Definitely not in set");
            if (might_contain && !actually_inserted) {
                std::cout << " (FALSE POSITIVE!)";
            }
            std::cout << "\n";
        }
        
        std::cout << std::endl;
        bloom_filter.print_statistics();
        
        print_section_footer();
    }

    void DataStructuresDemo::run_comprehensive_demo() {
        std::cout << "\n🎯 =============================================\n";
        std::cout << "🎯 COMPREHENSIVE DATA STRUCTURES DEMONSTRATION\n";
        std::cout << "🎯 =============================================\n\n";
        
        demonstrate_trie();
        demonstrate_disjoint_set();
        demonstrate_bloom_filter();
        
        std::cout << "\n🎉 ====================================\n";
        std::cout << "🎉 ALL DATA STRUCTURE DEMONSTRATIONS COMPLETED!\n";
        std::cout << "🎉 ====================================\n\n";
    }

    void DataStructuresDemo::print_section_header(const std::string& title) {
        std::cout << "\n" << std::string(60, '=') << std::endl;
        std::cout << "📊 " << title << std::endl;
        std::cout << std::string(60, '=') << std::endl << std::endl;
    }

    void DataStructuresDemo::print_section_footer() {
        std::cout << std::string(60, '-') << std::endl;
        std::cout << "✅ Section Complete\n" << std::endl;
    }

    std::vector<int> DataStructuresDemo::generate_test_data(size_t size) {
        std::vector<int> data(size);
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<int> dist(1, static_cast<int>(size * 2));
        
        for (size_t i = 0; i < size; ++i) {
            data[i] = dist(gen);
        }
        
        return data;
    }

    std::vector<std::string> DataStructuresDemo::generate_test_words(size_t count) {
        std::vector<std::string> words;
        std::vector<std::string> prefixes = {"pre", "post", "anti", "pro", "inter", "over", "under"};
        std::vector<std::string> roots = {"fix", "test", "work", "play", "run", "jump", "think"};
        std::vector<std::string> suffixes = {"ed", "ing", "er", "ly", "tion", "ment", "ness"};
        
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<size_t> prefix_dist(0, prefixes.size() - 1);
        std::uniform_int_distribution<size_t> root_dist(0, roots.size() - 1);
        std::uniform_int_distribution<size_t> suffix_dist(0, suffixes.size() - 1);
        
        for (size_t i = 0; i < count; ++i) {
            std::string word = prefixes[prefix_dist(gen)] + roots[root_dist(gen)] + suffixes[suffix_dist(gen)];
            words.push_back(word);
        }
        
        return words;
    }

    // ========== PerformanceBenchmark Implementation ==========

    std::vector<PerformanceBenchmark::BenchmarkResult> 
    PerformanceBenchmark::comprehensive_benchmark(size_t data_size) {
        std::vector<BenchmarkResult> results;
        
        std::cout << "🏁 Running comprehensive data structure benchmark...\n\n";
        
        // Generate test data
        auto test_data = generate_random_data(data_size);
        auto search_keys = generate_search_keys(test_data, data_size / 4);
        
        // Test different data structures
        std::cout << "Testing Trie with string operations...\n";
        {
            Trie trie;
            auto words = DataStructuresDemo::generate_test_words(data_size / 10);
            
            auto start_time = std::chrono::high_resolution_clock::now();
            for (const auto& word : words) {
                trie.insert(word);
            }
            auto end_time = std::chrono::high_resolution_clock::now();
            
            auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
            results.push_back({
                "Insert",
                "Trie",
                duration,
                words.size(),
                static_cast<double>(words.size()) / duration.count() * 1000000
            });
        }
        
        std::cout << "Testing Bloom Filter...\n";
        {
            BloomFilter bloom_filter(data_size, 0.01);
            auto words = DataStructuresDemo::generate_test_words(data_size / 10);
            
            auto start_time = std::chrono::high_resolution_clock::now();
            for (const auto& word : words) {
                bloom_filter.insert(word);
            }
            auto end_time = std::chrono::high_resolution_clock::now();
            
            auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
            results.push_back({
                "Insert",
                "Bloom Filter",
                duration,
                words.size(),
                static_cast<double>(words.size()) / duration.count() * 1000000
            });
        }
        
        return results;
    }

    void PerformanceBenchmark::print_benchmark_results(const std::vector<BenchmarkResult>& results) {
        std::cout << "\n📊 === DATA STRUCTURE PERFORMANCE RESULTS ===\n";
        std::cout << std::left << std::setw(15) << "Data Structure"
                  << std::setw(12) << "Operation"
                  << std::setw(15) << "Avg Time (μs)"
                  << std::setw(12) << "Count"
                  << std::setw(15) << "Ops/Second\n";
        std::cout << std::string(75, '-') << std::endl;
        
        for (const auto& result : results) {
            std::cout << std::left << std::setw(15) << result.data_structure
                      << std::setw(12) << result.operation
                      << std::setw(15) << result.avg_time.count()
                      << std::setw(12) << result.operations_count
                      << std::setw(15) << std::fixed << std::setprecision(0) << result.ops_per_second
                      << std::endl;
        }
        std::cout << std::string(75, '-') << std::endl << std::endl;
    }

    std::vector<int> PerformanceBenchmark::generate_random_data(size_t size) {
        std::vector<int> data(size);
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<int> dist(1, static_cast<int>(size * 2));
        
        for (size_t i = 0; i < size; ++i) {
            data[i] = dist(gen);
        }
        
        return data;
    }

    std::vector<int> PerformanceBenchmark::generate_search_keys(const std::vector<int>& data, size_t num_keys) {
        std::vector<int> search_keys;
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<size_t> dist(0, data.size() - 1);
        
        for (size_t i = 0; i < num_keys && i < data.size(); ++i) {
            search_keys.push_back(data[dist(gen)]);
        }
        
        return search_keys;
    }

} // namespace CppVerseHub::Algorithms