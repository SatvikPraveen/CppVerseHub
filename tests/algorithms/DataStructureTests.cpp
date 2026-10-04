// Tests for algorithms/DataStructures.hpp: each structure is checked against a standard-library
// model under long seeded random operation sequences, plus targeted edge cases.

#include "algorithms/DataStructures.hpp"
#include "algorithms/Demo.hpp"

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

#include <algorithm>
#include <list>
#include <map>
#include <memory>
#include <numeric>
#include <queue>
#include <random>
#include <set>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

using namespace CppVerseHub::Algorithms;

namespace {

// Tracks live instances and can be told to throw on the N-th copy.
struct Tracked {
    static inline int live = 0;
    static inline int copies_until_throw = -1;
    int value = 0;

    explicit Tracked(int v = 0) : value(v) { ++live; }
    Tracked(const Tracked& o) : value(o.value) {
        if (copies_until_throw == 0) {
            throw std::runtime_error("copy failed");
        }
        if (copies_until_throw > 0) {
            --copies_until_throw;
        }
        ++live;
    }
    Tracked(Tracked&& o) noexcept(false) : value(o.value) { ++live; }  // throwing move => vector copies
    Tracked& operator=(const Tracked&) = default;
    Tracked& operator=(Tracked&&) = default;
    ~Tracked() { --live; }
};

}  // namespace

// ---------------------------------------------------------------------------------------------
// DynamicArray
// ---------------------------------------------------------------------------------------------

TEST_CASE("DynamicArray behaves like std::vector under random operations", "[ds][dynamic-array][property]") {
    std::mt19937_64 rng(1);
    DynamicArray<int> a;
    std::vector<int> model;
    for (int step = 0; step < 5000; ++step) {
        const auto op = rng() % 10;
        if (op < 6) {
            const int v = static_cast<int>(rng() % 1000);
            a.push_back(v);
            model.push_back(v);
        } else if (op < 8 && !model.empty()) {
            a.pop_back();
            model.pop_back();
        } else if (op == 8 && !model.empty()) {
            const auto idx = static_cast<std::ptrdiff_t>(rng() % model.size());
            a.erase(a.begin() + idx);
            model.erase(model.begin() + idx);
        } else if (op == 9) {
            a.shrink_to_fit();
            REQUIRE(a.capacity() == a.size());
        }
        REQUIRE(a.size() == model.size());
        REQUIRE(a.capacity() >= a.size());
    }
    REQUIRE(std::equal(a.begin(), a.end(), model.begin(), model.end()));
    STATIC_REQUIRE(std::contiguous_iterator<DynamicArray<int>::iterator>);
    STATIC_REQUIRE(std::ranges::contiguous_range<DynamicArray<int>>);
}

TEST_CASE("DynamicArray copy, move, equality and bounds checking", "[ds][dynamic-array]") {
    DynamicArray<std::string> a{"alpha", "beta", "gamma"};
    DynamicArray<std::string> b = a;
    REQUIRE(a == b);
    b[1] = "BETA";
    REQUIRE_FALSE(a == b);
    REQUIRE(a[1] == "beta");
    DynamicArray<std::string> c = std::move(b);
    REQUIRE(b.empty());  // NOLINT(bugprone-use-after-move): moved-from state is specified as empty
    REQUIRE(c.size() == 3);
    c = a;
    REQUIRE(c == a);
    c = std::move(a);
    REQUIRE(c.size() == 3);
    REQUIRE_THROWS_AS(c.at(3), std::out_of_range);
    REQUIRE(c.at(2) == "gamma");
    c.clear();
    REQUIRE(c.empty());
    REQUIRE(c.capacity() >= 3);
    DynamicArray<int> zeros(5);
    REQUIRE(std::all_of(zeros.begin(), zeros.end(), [](int v) { return v == 0; }));
}

TEST_CASE("DynamicArray push_back of its own element during growth is safe", "[ds][dynamic-array]") {
    DynamicArray<std::string> a;
    a.push_back(std::string(40, 'x'));  // long string: no SSO, so a dangling reference would be visible
    for (int i = 0; i < 100; ++i) {
        a.push_back(a[0]);
        a.emplace_back(a.back());
    }
    REQUIRE(a.size() == 201);
    REQUIRE(std::all_of(a.begin(), a.end(), [](const std::string& s) { return s == std::string(40, 'x'); }));
}

TEST_CASE("DynamicArray supports move-only types and never leaks", "[ds][dynamic-array]") {
    {
        DynamicArray<std::unique_ptr<int>> a;
        for (int i = 0; i < 100; ++i) {
            a.emplace_back(std::make_unique<int>(i));
        }
        REQUIRE(*a[57] == 57);
        DynamicArray<std::unique_ptr<int>> b = std::move(a);
        REQUIRE(*b.back() == 99);
    }
    Tracked::live = 0;
    {
        DynamicArray<Tracked> t;
        for (int i = 0; i < 50; ++i) {
            t.emplace_back(i);
        }
        REQUIRE(Tracked::live == 50);
        auto copy = t;
        REQUIRE(Tracked::live == 100);
    }
    REQUIRE(Tracked::live == 0);
}

TEST_CASE("DynamicArray reallocation gives the strong exception guarantee", "[ds][dynamic-array][exceptions]") {
    Tracked::live = 0;
    {
        DynamicArray<Tracked> a;
        for (int i = 0; i < 4; ++i) {
            a.emplace_back(i);  // fills capacity 4 exactly
        }
        REQUIRE(a.capacity() == 4);
        Tracked::copies_until_throw = 2;  // the 3rd copy during reallocation throws
        REQUIRE_THROWS_AS(a.emplace_back(99), std::runtime_error);
        Tracked::copies_until_throw = -1;
        REQUIRE(a.size() == 4);
        REQUIRE(a.capacity() == 4);
        for (int i = 0; i < 4; ++i) {
            REQUIRE(a[static_cast<std::size_t>(i)].value == i);
        }
        REQUIRE(Tracked::live == 4);
    }
    REQUIRE(Tracked::live == 0);
}

// ---------------------------------------------------------------------------------------------
// LinkedList
// ---------------------------------------------------------------------------------------------

TEST_CASE("LinkedList matches std::list under random operations", "[ds][linked-list][property]") {
    std::mt19937_64 rng(2);
    LinkedList<int> l;
    std::list<int> model;
    for (int step = 0; step < 4000; ++step) {
        const auto op = rng() % 9;
        const int v = static_cast<int>(rng() % 100);
        if (op < 3) {
            l.push_back(v);
            model.push_back(v);
        } else if (op < 5) {
            l.push_front(v);
            model.push_front(v);
        } else if (op < 7 && !model.empty()) {
            REQUIRE(l.front() == model.front());
            l.pop_front();
            model.pop_front();
        } else if (op == 7) {
            l.reverse();
            model.reverse();
        } else {
            const int mod = 2 + v % 7;
            const auto removed = l.remove_if([mod](int x) { return x % mod == 0; });
            const auto before = model.size();
            model.remove_if([mod](int x) { return x % mod == 0; });
            REQUIRE(removed == before - model.size());
        }
        REQUIRE(l.size() == model.size());
        if (!model.empty()) {
            REQUIRE(l.back() == model.back());  // tail pointer stays correct
        }
    }
    REQUIRE(std::equal(l.begin(), l.end(), model.begin(), model.end()));
}

TEST_CASE("LinkedList iterators, copies and edge cases", "[ds][linked-list]") {
    STATIC_REQUIRE(std::forward_iterator<LinkedList<int>::iterator>);
    STATIC_REQUIRE(std::forward_iterator<LinkedList<int>::const_iterator>);
    STATIC_REQUIRE(std::ranges::forward_range<const LinkedList<int>>);
    STATIC_REQUIRE(std::convertible_to<LinkedList<int>::iterator, LinkedList<int>::const_iterator>);

    LinkedList<std::string> a{"x", "y", "z"};
    LinkedList<std::string> b = a;
    *b.begin() = "X";
    REQUIRE(a.front() == "x");
    REQUIRE_FALSE(a == b);
    LinkedList<std::string> c = std::move(b);
    REQUIRE(c.front() == "X");
    REQUIRE(std::ranges::find(c, "y") != c.end());

    LinkedList<int> e;
    REQUIRE_THROWS_AS(e.pop_front(), std::out_of_range);
    REQUIRE_THROWS_AS(e.front(), std::out_of_range);
    REQUIRE_THROWS_AS(e.back(), std::out_of_range);
    e.push_back(1);
    e.remove_if([](int) { return true; });
    REQUIRE(e.empty());
    e.push_back(2);  // tail must have been reset
    REQUIRE(e.front() == 2);
    REQUIRE(e.back() == 2);
}

TEST_CASE("LinkedList destroys very long chains without recursion", "[ds][linked-list]") {
    LinkedList<int> l;
    for (int i = 0; i < 300'000; ++i) {
        l.push_back(i);
    }
    l.reverse();
    REQUIRE(l.front() == 299'999);
    REQUIRE(l.back() == 0);
    // destructor runs here; a recursive unique_ptr chain would overflow the stack
}

// ---------------------------------------------------------------------------------------------
// BinarySearchTree
// ---------------------------------------------------------------------------------------------

TEST_CASE("BinarySearchTree matches std::set under random operations", "[ds][bst][property]") {
    std::mt19937_64 rng(3);
    BinarySearchTree<int> t;
    std::set<int> model;
    for (int step = 0; step < 6000; ++step) {
        const int v = static_cast<int>(rng() % 500);
        switch (rng() % 3) {
            case 0:
                REQUIRE(t.insert(v) == model.insert(v).second);
                break;
            case 1:
                REQUIRE(t.erase(v) == (model.erase(v) == 1));
                break;
            default:
                REQUIRE(t.contains(v) == model.contains(v));
                break;
        }
        REQUIRE(t.size() == model.size());
    }
    REQUIRE(t.in_order() == std::vector<int>(model.begin(), model.end()));
    REQUIRE(t.min() == *model.begin());
    REQUIRE(t.max() == *model.rbegin());
    REQUIRE(t.height() >= static_cast<std::size_t>(std::bit_width(model.size())));
}

TEST_CASE("BinarySearchTree LCA, copies, custom order and degenerate shape", "[ds][bst]") {
    BinarySearchTree<int> t;
    for (int k : {50, 30, 70, 20, 40, 60, 80, 35}) {
        t.insert(k);
    }
    REQUIRE(t.lowest_common_ancestor(20, 35) == 30);
    REQUIRE(t.lowest_common_ancestor(35, 80) == 50);
    REQUIRE(t.lowest_common_ancestor(60, 60) == 60);
    REQUIRE_FALSE(t.lowest_common_ancestor(20, 99).has_value());
    REQUIRE(t.height() == 4);

    auto copy = t;
    copy.erase(50);
    REQUIRE(t.contains(50));
    REQUIRE_FALSE(copy.contains(50));
    REQUIRE(copy.in_order() == std::vector<int>{20, 30, 35, 40, 60, 70, 80});

    BinarySearchTree<std::string, std::greater<>> desc;
    for (const char* s : {"b", "d", "a", "c"}) {
        desc.insert(s);
    }
    REQUIRE(desc.in_order() == std::vector<std::string>{"d", "c", "b", "a"});
    REQUIRE(desc.min() == "d");

    BinarySearchTree<int> chain;
    for (int i = 0; i < 4'000; ++i) {
        chain.insert(i);  // sorted insertion: height == n (insertion is O(n^2) overall)
    }
    REQUIRE(chain.height() == 4'000);
    auto chain_copy = chain;  // iterative copy of a linear chain
    REQUIRE(chain_copy.size() == 4'000);
    REQUIRE(chain_copy.in_order().back() == 3'999);
    chain.clear();
    REQUIRE(chain.empty());
    BinarySearchTree<int> empty;
    REQUIRE_THROWS_AS(empty.min(), std::out_of_range);
    REQUIRE_THROWS_AS(empty.max(), std::out_of_range);
}

// ---------------------------------------------------------------------------------------------
// MinHeap
// ---------------------------------------------------------------------------------------------

TEST_CASE("MinHeap matches std::priority_queue and heapifies in place", "[ds][heap][property]") {
    std::mt19937_64 rng(4);
    MinHeap<int> h;
    std::priority_queue<int, std::vector<int>, std::greater<>> model;
    for (int step = 0; step < 5000; ++step) {
        if (rng() % 3 != 0 || model.empty()) {
            const int v = static_cast<int>(rng() % 1000);
            h.push(v);
            model.push(v);
        } else {
            REQUIRE(h.top() == model.top());
            REQUIRE(h.pop() == model.top());
            model.pop();
        }
        REQUIRE(h.size() == model.size());
    }
    REQUIRE(h.is_valid_heap());

    std::vector<int> values(1000);
    for (int& v : values) {
        v = static_cast<int>(rng() % 100);
    }
    MinHeap<int, std::greater<>> max_heap(values);
    REQUIRE(max_heap.is_valid_heap());
    std::vector<int> drained;
    while (!max_heap.empty()) {
        drained.push_back(max_heap.pop());
    }
    std::sort(values.begin(), values.end(), std::greater<>{});
    REQUIRE(drained == values);
    REQUIRE_THROWS_AS(max_heap.pop(), std::out_of_range);
    REQUIRE_THROWS_AS(max_heap.top(), std::out_of_range);
}

// ---------------------------------------------------------------------------------------------
// HashTable
// ---------------------------------------------------------------------------------------------

TEST_CASE("HashTable matches std::unordered_map under heavy insert/erase churn", "[ds][hash][property]") {
    std::mt19937_64 rng(5);
    HashTable<int, int> h;
    std::unordered_map<int, int> model;
    for (int step = 0; step < 20000; ++step) {
        const int k = static_cast<int>(rng() % 700);
        switch (rng() % 4) {
            case 0:
            case 1: {
                const int v = static_cast<int>(rng());
                REQUIRE(h.insert_or_assign(k, v) == !model.contains(k));
                model[k] = v;
                break;
            }
            case 2:
                REQUIRE(h.erase(k) == (model.erase(k) == 1));
                break;
            default: {
                const int* found = h.find(k);
                const auto it = model.find(k);
                REQUIRE((found != nullptr) == (it != model.end()));
                if (found != nullptr) {
                    REQUIRE(*found == it->second);
                }
                break;
            }
        }
        REQUIRE(h.size() == model.size());
        REQUIRE(h.load_factor() <= HashTable<int, int>::kMaxLoad);
    }
    std::size_t visited = 0;
    h.for_each([&](const int& k, const int& v) {
        ++visited;
        REQUIRE(model.at(k) == v);
    });
    REQUIRE(visited == model.size());
}

TEST_CASE("HashTable stays correct with a pathological constant hash", "[ds][hash]") {
    struct ConstantHash {
        std::size_t operator()(const std::string&) const noexcept { return 42; }
    };
    HashTable<std::string, int, ConstantHash> h;
    for (int i = 0; i < 200; ++i) {
        h.insert_or_assign("key" + std::to_string(i), i);
    }
    for (int i = 0; i < 200; i += 2) {
        REQUIRE(h.erase("key" + std::to_string(i)));
    }
    for (int i = 0; i < 200; ++i) {
        REQUIRE(h.contains("key" + std::to_string(i)) == (i % 2 == 1));
    }
    REQUIRE(h.at("key7") == 7);
    REQUIRE_THROWS_AS(h.at("key8"), std::out_of_range);
    REQUIRE(h.max_probe_distance() >= 99);  // everything collides: one long cluster
    h.clear();
    REQUIRE(h.empty());
    REQUIRE_FALSE(h.contains("key1"));
}

// ---------------------------------------------------------------------------------------------
// Trie
// ---------------------------------------------------------------------------------------------

TEST_CASE("Trie prefix queries match a std::set model", "[ds][trie][property]") {
    std::mt19937_64 rng(6);
    Trie t;
    std::set<std::string> model;
    auto word = [&rng] {
        std::string w(rng() % 6, 'a');
        for (char& c : w) {
            c = static_cast<char>('a' + rng() % 3);
        }
        return w;
    };
    for (int step = 0; step < 3000; ++step) {
        const std::string w = word();
        if (rng() % 3 == 0) {
            REQUIRE(t.erase(w) == (model.erase(w) == 1));
        } else {
            REQUIRE(t.insert(w) == model.insert(w).second);
        }
        REQUIRE(t.size() == model.size());
        const std::string p = word().substr(0, 2);
        std::vector<std::string> expected;
        for (const auto& m : model) {
            if (m.starts_with(p)) {
                expected.push_back(m);
            }
        }
        REQUIRE(t.with_prefix(p) == expected);
        REQUIRE(t.starts_with(p) == !expected.empty());
        REQUIRE(t.contains(w) == model.contains(w));
    }
}

TEST_CASE("Trie erase prunes nodes; copies are deep; limits are honoured", "[ds][trie]") {
    Trie t;
    REQUIRE(t.node_count() == 1);
    REQUIRE(t.insert("star"));
    REQUIRE(t.insert("start"));
    REQUIRE_FALSE(t.insert("star"));
    REQUIRE(t.node_count() == 6);
    Trie copy = t;
    REQUIRE(t.erase("start"));
    REQUIRE(t.node_count() == 5);
    REQUIRE_FALSE(t.erase("sta"));
    REQUIRE(t.erase("star"));
    REQUIRE(t.node_count() == 1);
    REQUIRE(t.empty());
    REQUIRE(copy.contains("start"));
    REQUIRE(copy.with_prefix("s", 1) == std::vector<std::string>{"star"});
    REQUIRE(copy.with_prefix("", 0).empty());
    REQUIRE(t.insert(""));
    REQUIRE(t.contains(""));
    Trie moved = std::move(copy);
    REQUIRE(moved.size() == 2);
    REQUIRE(copy.size() == 0);  // NOLINT(bugprone-use-after-move): specified empty
    REQUIRE(copy.insert("reuse"));
}

// ---------------------------------------------------------------------------------------------
// DisjointSet
// ---------------------------------------------------------------------------------------------

TEST_CASE("DisjointSet agrees with naive relabelling", "[ds][union-find][property]") {
    std::mt19937_64 rng(7);
    const std::size_t n = 200;
    DisjointSet dsu(n);
    std::vector<std::size_t> label(n);
    std::iota(label.begin(), label.end(), std::size_t{0});
    for (int step = 0; step < 3000; ++step) {
        const std::size_t a = rng() % n;
        const std::size_t b = rng() % n;
        if (rng() % 2 == 0) {
            const bool merged = label[a] != label[b];
            REQUIRE(dsu.unite(a, b) == merged);
            const std::size_t from = label[b];
            for (auto& l : label) {
                if (l == from) {
                    l = label[a];
                }
            }
        } else {
            REQUIRE(dsu.connected(a, b) == (label[a] == label[b]));
            REQUIRE(dsu.set_size(a) ==
                    static_cast<std::size_t>(std::count(label.begin(), label.end(), label[a])));
        }
    }
    std::set<std::size_t> distinct(label.begin(), label.end());
    REQUIRE(dsu.set_count() == distinct.size());
    REQUIRE(dsu.size() == n);
    REQUIRE_THROWS_AS(dsu.find(n), std::out_of_range);
}

// ---------------------------------------------------------------------------------------------
// BloomFilter
// ---------------------------------------------------------------------------------------------

TEST_CASE("BloomFilter has no false negatives and meets its false-positive target", "[ds][bloom][property]") {
    BloomFilter f(2000, 0.01);
    REQUIRE(f.bit_count() == 19171);  // ceil(-2000 ln 0.01 / ln^2 2)
    REQUIRE(f.hash_count() == 7);
    for (int i = 0; i < 2000; ++i) {
        f.insert("item-" + std::to_string(i));
    }
    for (int i = 0; i < 2000; ++i) {
        REQUIRE(f.might_contain("item-" + std::to_string(i)));
    }
    int fp = 0;
    for (int i = 0; i < 20000; ++i) {
        fp += f.might_contain("other-" + std::to_string(i)) ? 1 : 0;
    }
    const double rate = fp / 20000.0;
    REQUIRE(rate < 0.02);
    REQUIRE(f.estimated_false_positive_rate() > 0.005);
    REQUIRE(f.estimated_false_positive_rate() < 0.015);
    REQUIRE(f.inserted() == 2000);
    f.clear();
    REQUIRE_FALSE(f.might_contain("item-1"));
    REQUIRE(f.estimated_false_positive_rate() == 0.0);
    REQUIRE_THROWS_AS(BloomFilter(10, 0.0), std::invalid_argument);
    REQUIRE_THROWS_AS(BloomFilter(10, 1.0), std::invalid_argument);
}

// ---------------------------------------------------------------------------------------------
// SkipList
// ---------------------------------------------------------------------------------------------

TEST_CASE("SkipList matches std::set under random operations", "[ds][skiplist][property]") {
    std::mt19937_64 rng(8);
    SkipList<int> s(99);
    std::set<int> model;
    for (int step = 0; step < 8000; ++step) {
        const int v = static_cast<int>(rng() % 800);
        switch (rng() % 3) {
            case 0:
                REQUIRE(s.insert(v) == model.insert(v).second);
                break;
            case 1:
                REQUIRE(s.erase(v) == (model.erase(v) == 1));
                break;
            default:
                REQUIRE(s.contains(v) == model.contains(v));
                break;
        }
        REQUIRE(s.size() == model.size());
    }
    REQUIRE(s.to_vector() == std::vector<int>(model.begin(), model.end()));
    REQUIRE(s.levels() <= 32);
    REQUIRE(s.levels() >= 1);
}

TEST_CASE("SkipList copy/move semantics, custom order and large sizes", "[ds][skiplist]") {
    SkipList<std::string, std::greater<>> desc;
    for (const char* w : {"b", "a", "c", "b"}) {
        desc.insert(w);
    }
    REQUIRE(desc.to_vector() == std::vector<std::string>{"c", "b", "a"});
    auto copy = desc;
    copy.erase("b");
    REQUIRE(desc.contains("b"));
    REQUIRE_FALSE(copy.contains("b"));
    auto moved = std::move(desc);
    REQUIRE(moved.size() == 3);
    REQUIRE(desc.empty());  // NOLINT(bugprone-use-after-move): specified empty
    REQUIRE(desc.insert("z"));
    REQUIRE(desc.contains("z"));

    SkipList<int> big(1);
    for (int i = 0; i < 100'000; ++i) {
        big.insert(i);
    }
    REQUIRE(big.size() == 100'000);
    REQUIRE(big.levels() >= 10);  // expected ~log2(n) = 17
    REQUIRE(big.levels() <= 32);
    for (int i = 0; i < 100'000; i += 2) {
        REQUIRE(big.erase(i));
    }
    REQUIRE(big.size() == 50'000);
    REQUIRE(big.contains(99'999));
    REQUIRE_FALSE(big.contains(50'000));
}

// ---------------------------------------------------------------------------------------------
// Demos
// ---------------------------------------------------------------------------------------------

TEST_CASE("data-structure demo prints every structure", "[ds][demo]") {
    std::ostringstream oss;
    demonstrate_data_structures(oss);
    const std::string s = oss.str();
    for (const char* name : {"DynamicArray", "LinkedList", "BinarySearchTree", "MinHeap", "HashTable", "Trie",
                             "DisjointSet", "BloomFilter", "SkipList"}) {
        REQUIRE(s.find(name) != std::string::npos);
    }
    REQUIRE(s.find("MinHeap pops: 1 2 4 7 8 9") != std::string::npos);
}

TEST_CASE("runDemo runs every showcase without throwing", "[algorithms][demo]") {
    std::ostringstream oss;
    REQUIRE_NOTHROW(runDemo(oss));
    const std::string s = oss.str();
    REQUIRE_FALSE(s.empty());
    REQUIRE(s.find("demo failed") == std::string::npos);
    REQUIRE(s.find("=== Sorting") != std::string::npos);
    REQUIRE(s.find("=== Graph") != std::string::npos);
    REQUIRE(s.find("=== Data structures") != std::string::npos);
    REQUIRE(s.find("=== String search") != std::string::npos);
}
