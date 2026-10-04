/**
 * @file DataStructures.cpp
 * @brief Out-of-line parts of the data structures (Trie, DisjointSet, BloomFilter) and the
 *        data-structure showcase.
 */

#include "algorithms/DataStructures.hpp"

#include <cmath>
#include <iomanip>
#include <numbers>

namespace CppVerseHub::Algorithms {

// --------------------------------------------------------------------------------------------
// Trie
// --------------------------------------------------------------------------------------------

struct Trie::Node {
    std::map<unsigned char, std::unique_ptr<Node>> children; // unsigned: matches std::string order
    bool terminal = false;

    [[nodiscard]] std::unique_ptr<Node> clone() const {
        auto copy = std::make_unique<Node>();
        copy->terminal = terminal;
        for (const auto& [c, child] : children) {
            copy->children.emplace(c, child->clone());
        }
        return copy;
    }
};

Trie::Trie() : root_(std::make_unique<Node>()) {}
Trie::~Trie() = default;
Trie::Trie(const Trie& other) : root_(other.root_->clone()), size_(other.size_) {}
Trie& Trie::operator=(const Trie& other) {
    if (this != &other) {
        root_ = other.root_->clone();
        size_ = other.size_;
    }
    return *this;
}
Trie::Trie(Trie&& other) noexcept
    : root_(std::exchange(other.root_, std::make_unique<Node>())), size_(std::exchange(other.size_, 0)) {}
Trie& Trie::operator=(Trie&& other) noexcept {
    if (this != &other) {
        std::swap(root_, other.root_);
        std::swap(size_, other.size_);
    }
    return *this;
}

bool Trie::insert(std::string_view word) {
    Node* n = root_.get();
    for (char ch : word) {
        auto& child = n->children[static_cast<unsigned char>(ch)];
        if (!child) {
            child = std::make_unique<Node>();
        }
        n = child.get();
    }
    if (n->terminal) {
        return false;
    }
    n->terminal = true;
    ++size_;
    return true;
}

namespace {

template <class NodeT>
NodeT* walk(NodeT* n, std::string_view s) {
    for (char ch : s) {
        auto it = n->children.find(static_cast<unsigned char>(ch));
        if (it == n->children.end()) {
            return nullptr;
        }
        n = it->second.get();
    }
    return n;
}

} // namespace

bool Trie::contains(std::string_view word) const {
    const Node* n = walk(static_cast<const Node*>(root_.get()), word);
    return n != nullptr && n->terminal;
}

bool Trie::starts_with(std::string_view prefix) const {
    const Node* n = walk(static_cast<const Node*>(root_.get()), prefix);
    return n != nullptr && (n->terminal || !n->children.empty());
}

std::vector<std::string> Trie::with_prefix(std::string_view prefix, std::size_t limit) const {
    std::vector<std::string> out;
    const Node* start = walk(static_cast<const Node*>(root_.get()), prefix);
    if (start == nullptr || limit == 0) {
        return out;
    }
    // Iterative pre-order DFS; children pushed in reverse so output is lexicographic.
    std::vector<std::pair<const Node*, std::string>> stack{{start, std::string(prefix)}};
    while (!stack.empty() && out.size() < limit) {
        auto [n, word] = std::move(stack.back());
        stack.pop_back();
        if (n->terminal) {
            out.push_back(word);
        }
        for (auto it = n->children.rbegin(); it != n->children.rend(); ++it) {
            stack.emplace_back(it->second.get(), word + static_cast<char>(it->first));
        }
    }
    return out;
}

bool Trie::erase(std::string_view word) {
    // Record the path so dead branches can be pruned bottom-up.
    std::vector<Node*> path{root_.get()};
    for (char ch : word) {
        auto it = path.back()->children.find(static_cast<unsigned char>(ch));
        if (it == path.back()->children.end()) {
            return false;
        }
        path.push_back(it->second.get());
    }
    if (!path.back()->terminal) {
        return false;
    }
    path.back()->terminal = false;
    --size_;
    for (std::size_t i = word.size(); i > 0; --i) {
        Node* n = path[i];
        if (n->terminal || !n->children.empty()) {
            break;
        }
        path[i - 1]->children.erase(static_cast<unsigned char>(word[i - 1]));
    }
    return true;
}

std::size_t Trie::node_count() const noexcept {
    std::size_t count = 0;
    std::vector<const Node*> stack{root_.get()};
    while (!stack.empty()) {
        const Node* n = stack.back();
        stack.pop_back();
        ++count;
        for (const auto& [c, child] : n->children) {
            stack.push_back(child.get());
        }
    }
    return count;
}

// --------------------------------------------------------------------------------------------
// DisjointSet
// --------------------------------------------------------------------------------------------

DisjointSet::DisjointSet(std::size_t n) : parent_(n), size_(n, 1), sets_(n) {
    for (std::size_t i = 0; i < n; ++i) {
        parent_[i] = i;
    }
}

std::size_t DisjointSet::find(std::size_t x) {
    if (x >= parent_.size()) {
        throw std::out_of_range("DisjointSet::find: element out of range");
    }
    while (parent_[x] != x) {
        parent_[x] = parent_[parent_[x]]; // path halving
        x = parent_[x];
    }
    return x;
}

bool DisjointSet::unite(std::size_t a, std::size_t b) {
    a = find(a);
    b = find(b);
    if (a == b) {
        return false;
    }
    if (size_[a] < size_[b]) {
        std::swap(a, b);
    }
    parent_[b] = a;
    size_[a] += size_[b];
    --sets_;
    return true;
}

bool DisjointSet::connected(std::size_t a, std::size_t b) {
    return find(a) == find(b);
}

std::size_t DisjointSet::set_size(std::size_t x) {
    return size_[find(x)];
}

// --------------------------------------------------------------------------------------------
// BloomFilter
// --------------------------------------------------------------------------------------------

namespace {

std::uint64_t fnv1a(std::string_view s) noexcept {
    std::uint64_t h = 0xcbf29ce484222325ULL;
    for (char c : s) {
        h ^= static_cast<unsigned char>(c);
        h *= 0x100000001b3ULL;
    }
    return h;
}

std::uint64_t splitmix(std::uint64_t x) noexcept {
    x += 0x9e3779b97f4a7c15ULL;
    x = (x ^ (x >> 30)) * 0xbf58476d1ce4e5b9ULL;
    x = (x ^ (x >> 27)) * 0x94d049bb133111ebULL;
    return x ^ (x >> 31);
}

} // namespace

BloomFilter::BloomFilter(std::size_t expected_elements, double false_positive_rate) {
    if (!(false_positive_rate > 0.0 && false_positive_rate < 1.0)) {
        throw std::invalid_argument("BloomFilter: false_positive_rate must be in (0, 1)");
    }
    const double n = static_cast<double>(std::max<std::size_t>(1, expected_elements));
    const double ln2 = std::numbers::ln2;
    const double m = std::ceil(-n * std::log(false_positive_rate) / (ln2 * ln2));
    bits_ = std::max<std::size_t>(64, static_cast<std::size_t>(m));
    hashes_ =
        std::max<std::size_t>(1, static_cast<std::size_t>(std::lround(static_cast<double>(bits_) / n * ln2)));
    words_.assign((bits_ + 63) / 64, 0);
}

void BloomFilter::insert(std::string_view element) {
    const std::uint64_t h1 = fnv1a(element);
    const std::uint64_t h2 = splitmix(h1) | 1U;
    for (std::size_t i = 0; i < hashes_; ++i) {
        const std::size_t bit = static_cast<std::size_t>((h1 + i * h2) % bits_);
        words_[bit / 64] |= std::uint64_t{1} << (bit % 64);
    }
    ++inserted_;
}

bool BloomFilter::might_contain(std::string_view element) const {
    const std::uint64_t h1 = fnv1a(element);
    const std::uint64_t h2 = splitmix(h1) | 1U;
    for (std::size_t i = 0; i < hashes_; ++i) {
        const std::size_t bit = static_cast<std::size_t>((h1 + i * h2) % bits_);
        if ((words_[bit / 64] & (std::uint64_t{1} << (bit % 64))) == 0) {
            return false;
        }
    }
    return true;
}

void BloomFilter::clear() noexcept {
    std::fill(words_.begin(), words_.end(), 0);
    inserted_ = 0;
}

double BloomFilter::estimated_false_positive_rate() const {
    const double k = static_cast<double>(hashes_);
    const double m = static_cast<double>(bits_);
    const double n = static_cast<double>(inserted_);
    return std::pow(1.0 - std::exp(-k * n / m), k);
}

// --------------------------------------------------------------------------------------------
// Showcase
// --------------------------------------------------------------------------------------------

void demonstrate_data_structures(std::ostream& out) {
    out << "=== Data structures ===\n";

    DynamicArray<std::string> names;
    for (const char* n : {"Sol", "Alpha Centauri", "Sirius", "Vega", "Rigel"}) {
        names.emplace_back(n);
    }
    names.push_back(names[0]); // aliasing push_back during growth is safe
    out << "DynamicArray: size " << names.size() << ", capacity " << names.capacity()
        << ", last = " << names.back() << '\n';

    LinkedList<int> list{1, 2, 3, 4, 5, 6};
    const std::size_t removed = list.remove_if([](int v) { return v % 2 == 0; });
    list.reverse();
    out << "LinkedList: removed " << removed << " evens, reversed ->";
    for (int v : list) {
        out << ' ' << v;
    }
    out << '\n';

    BinarySearchTree<int> bst;
    for (int k : {50, 30, 70, 20, 40, 60, 80}) {
        bst.insert(k);
    }
    bst.erase(30);
    out << "BinarySearchTree: in-order";
    for (int k : bst.in_order()) {
        out << ' ' << k;
    }
    out << "; height " << bst.height()
        << "; LCA(20, 40) = " << bst.lowest_common_ancestor(20, 40).value_or(-1) << '\n';

    MinHeap<int> heap(std::vector<int>{9, 4, 7, 1, 8, 2});
    out << "MinHeap pops:";
    while (!heap.empty()) {
        out << ' ' << heap.pop();
    }
    out << '\n';

    HashTable<std::string, double> masses;
    masses.insert_or_assign("Earth", 1.0);
    masses.insert_or_assign("Jupiter", 317.8);
    masses.insert_or_assign("Mars", 0.107);
    masses.insert_or_assign("Earth", 1.0); // overwrite
    masses.erase("Mars");
    out << "HashTable: size " << masses.size() << ", load " << std::fixed << std::setprecision(3)
        << masses.load_factor() << ", Jupiter = " << masses.at("Jupiter") << " Earth masses\n";
    out.unsetf(std::ios::floatfield);
    out << std::setprecision(6);

    Trie trie;
    for (const char* w : {"star", "start", "stardust", "station", "stellar", "moon"}) {
        trie.insert(w);
    }
    out << "Trie: completions of \"sta\":";
    for (const auto& w : trie.with_prefix("sta")) {
        out << ' ' << w;
    }
    out << " (" << trie.node_count() << " nodes)\n";

    DisjointSet dsu(8);
    dsu.unite(0, 1);
    dsu.unite(2, 3);
    dsu.unite(1, 3);
    dsu.unite(5, 6);
    out << "DisjointSet: " << dsu.set_count() << " sets; 0~3 " << std::boolalpha << dsu.connected(0, 3)
        << "; 0~5 " << dsu.connected(0, 5) << '\n';

    BloomFilter bloom(1000, 0.01);
    for (int i = 0; i < 1000; ++i) {
        bloom.insert("ship-" + std::to_string(i));
    }
    std::size_t false_positives = 0;
    for (int i = 1000; i < 11000; ++i) {
        false_positives += bloom.might_contain("ship-" + std::to_string(i)) ? 1U : 0U;
    }
    out << "BloomFilter: m = " << bloom.bit_count() << " bits, k = " << bloom.hash_count()
        << ", measured FP rate = " << static_cast<double>(false_positives) / 10000.0 << " (theory "
        << bloom.estimated_false_positive_rate() << ")\n";

    SkipList<int> skip(7);
    for (int k : {15, 3, 9, 27, 1, 21}) {
        skip.insert(k);
    }
    skip.erase(9);
    out << "SkipList (" << skip.levels() << " levels):";
    for (int k : skip.to_vector()) {
        out << ' ' << k;
    }
    out << '\n';
}

} // namespace CppVerseHub::Algorithms
