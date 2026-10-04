/**
 * @file SearchAlgorithms.cpp
 * @brief Non-template string-search algorithms, Aho-Corasick, suffix array, edit distance and the
 *        searching showcase.
 */

#include "algorithms/SearchAlgorithms.hpp"

#include <deque>
#include <numeric>
#include <random>

namespace CppVerseHub::Algorithms {

namespace {

std::vector<std::size_t> all_positions(std::size_t n) {
    std::vector<std::size_t> v(n + 1);
    std::iota(v.begin(), v.end(), std::size_t{0});
    return v;
}

// Arithmetic modulo the Mersenne prime 2^61 - 1 without 128-bit integers (portable to MSVC).
constexpr std::uint64_t kMod = (std::uint64_t{1} << 61) - 1;
constexpr std::uint64_t kMask30 = (std::uint64_t{1} << 30) - 1;
constexpr std::uint64_t kMask31 = (std::uint64_t{1} << 31) - 1;

constexpr std::uint64_t mod61(std::uint64_t x) noexcept {
    const std::uint64_t r = (x >> 61) + (x & kMod);
    return r >= kMod ? r - kMod : r;
}

constexpr std::uint64_t mul61(std::uint64_t a, std::uint64_t b) noexcept {
    const std::uint64_t au = a >> 31;
    const std::uint64_t ad = a & kMask31;
    const std::uint64_t bu = b >> 31;
    const std::uint64_t bd = b & kMask31;
    const std::uint64_t mid = ad * bu + au * bd;
    const std::uint64_t midu = mid >> 30;
    const std::uint64_t midd = mid & kMask30;
    return mod61(au * bu * 2 + midu + (midd << 31) + ad * bd);
}

constexpr std::uint64_t add61(std::uint64_t a, std::uint64_t b) noexcept {
    const std::uint64_t s = a + b;
    return s >= kMod ? s - kMod : s;
}

constexpr std::uint64_t sub61(std::uint64_t a, std::uint64_t b) noexcept {
    return a >= b ? a - b : a + kMod - b;
}

constexpr std::uint64_t kHashBase = 1'000'003;

std::uint64_t byte_of(char c) noexcept {
    return static_cast<std::uint64_t>(static_cast<unsigned char>(c)) + 1;
}

} // namespace

// --------------------------------------------------------------------------------------------
// Exact string matching
// --------------------------------------------------------------------------------------------

std::vector<std::size_t> naive_search(std::string_view text, std::string_view pattern) {
    const std::size_t n = text.size();
    const std::size_t m = pattern.size();
    if (m == 0) {
        return all_positions(n);
    }
    std::vector<std::size_t> matches;
    for (std::size_t i = 0; i + m <= n; ++i) {
        std::size_t j = 0;
        while (j < m && text[i + j] == pattern[j]) {
            ++j;
        }
        if (j == m) {
            matches.push_back(i);
        }
    }
    return matches;
}

std::vector<std::size_t> boyer_moore_horspool_search(std::string_view text, std::string_view pattern) {
    const std::size_t n = text.size();
    const std::size_t m = pattern.size();
    if (m == 0) {
        return all_positions(n);
    }
    std::vector<std::size_t> matches;
    if (m > n) {
        return matches;
    }
    std::array<std::size_t, 256> shift{};
    shift.fill(m);
    for (std::size_t i = 0; i + 1 < m; ++i) {
        shift[static_cast<unsigned char>(pattern[i])] = m - 1 - i;
    }
    std::size_t pos = 0;
    while (pos + m <= n) {
        std::size_t j = m;
        while (j > 0 && text[pos + j - 1] == pattern[j - 1]) {
            --j;
        }
        if (j == 0) {
            matches.push_back(pos);
        }
        pos += shift[static_cast<unsigned char>(text[pos + m - 1])];
    }
    return matches;
}

std::vector<std::size_t> rabin_karp_search(std::string_view text, std::string_view pattern) {
    const std::size_t n = text.size();
    const std::size_t m = pattern.size();
    if (m == 0) {
        return all_positions(n);
    }
    std::vector<std::size_t> matches;
    if (m > n) {
        return matches;
    }
    std::uint64_t hp = 0;
    std::uint64_t ht = 0;
    std::uint64_t high = 1; // base^(m-1)
    for (std::size_t i = 0; i < m; ++i) {
        hp = add61(mul61(hp, kHashBase), byte_of(pattern[i]));
        ht = add61(mul61(ht, kHashBase), byte_of(text[i]));
        if (i + 1 < m) {
            high = mul61(high, kHashBase);
        }
    }
    for (std::size_t i = 0;; ++i) {
        if (hp == ht && text.substr(i, m) == pattern) {
            matches.push_back(i);
        }
        if (i + m >= n) {
            break;
        }
        ht = sub61(ht, mul61(byte_of(text[i]), high));
        ht = add61(mul61(ht, kHashBase), byte_of(text[i + m]));
    }
    return matches;
}

std::vector<std::size_t> z_function(std::string_view s) {
    const std::size_t n = s.size();
    std::vector<std::size_t> z(n, 0);
    if (n == 0) {
        return z;
    }
    z[0] = n;
    std::size_t l = 0;
    std::size_t r = 0; // [l, r) is the rightmost window matching a prefix
    for (std::size_t i = 1; i < n; ++i) {
        if (i < r) {
            z[i] = std::min(r - i, z[i - l]);
        }
        while (i + z[i] < n && s[z[i]] == s[i + z[i]]) {
            ++z[i];
        }
        if (i + z[i] > r) {
            l = i;
            r = i + z[i];
        }
    }
    return z;
}

std::vector<std::size_t> z_search(std::string_view text, std::string_view pattern) {
    const std::size_t m = pattern.size();
    if (m == 0) {
        return all_positions(text.size());
    }
    std::string combined;
    combined.reserve(m + text.size());
    combined.append(pattern);
    combined.append(text);
    const auto z = z_function(combined);
    std::vector<std::size_t> matches;
    for (std::size_t i = m; i < combined.size(); ++i) {
        if (z[i] >= m) {
            matches.push_back(i - m);
        }
    }
    return matches;
}

// --------------------------------------------------------------------------------------------
// Aho-Corasick
// --------------------------------------------------------------------------------------------

AhoCorasick::AhoCorasick(std::vector<std::string> patterns) : patterns_(std::move(patterns)) {
    auto new_node = [this] {
        nodes_.emplace_back();
        nodes_.back().next.fill(-1);
        return static_cast<std::int32_t>(nodes_.size() - 1);
    };
    new_node();
    for (std::size_t p = 0; p < patterns_.size(); ++p) {
        if (patterns_[p].empty()) {
            continue;
        }
        std::int32_t state = 0;
        for (char ch : patterns_[p]) {
            const auto c = static_cast<unsigned char>(ch);
            if (nodes_[static_cast<std::size_t>(state)].next[c] < 0) {
                const std::int32_t created = new_node();
                nodes_[static_cast<std::size_t>(state)].next[c] = created;
            }
            state = nodes_[static_cast<std::size_t>(state)].next[c];
        }
        nodes_[static_cast<std::size_t>(state)].output.push_back(p);
    }
    // Breadth-first construction of failure links, folding them into a complete DFA.
    std::deque<std::int32_t> queue;
    for (auto& child : nodes_[0].next) {
        if (child < 0) {
            child = 0;
        } else {
            nodes_[static_cast<std::size_t>(child)].fail = 0;
            queue.push_back(child);
        }
    }
    while (!queue.empty()) {
        const auto u = static_cast<std::size_t>(queue.front());
        queue.pop_front();
        const auto fu = static_cast<std::size_t>(nodes_[u].fail);
        for (std::size_t c = 0; c < 256; ++c) {
            const std::int32_t v = nodes_[u].next[c];
            if (v >= 0) {
                const std::int32_t f = nodes_[fu].next[c];
                auto& node_v = nodes_[static_cast<std::size_t>(v)];
                node_v.fail = f;
                const auto& node_f = nodes_[static_cast<std::size_t>(f)];
                node_v.dict_link = node_f.output.empty() ? node_f.dict_link : f;
                queue.push_back(v);
            } else {
                nodes_[u].next[c] = nodes_[fu].next[c];
            }
        }
    }
}

std::vector<AhoCorasick::Match> AhoCorasick::find_all(std::string_view text) const {
    std::vector<Match> matches;
    std::size_t state = 0;
    for (std::size_t i = 0; i < text.size(); ++i) {
        state = static_cast<std::size_t>(nodes_[state].next[static_cast<unsigned char>(text[i])]);
        for (auto s = static_cast<std::int32_t>(state); s > 0;
             s = nodes_[static_cast<std::size_t>(s)].dict_link) {
            for (std::size_t p : nodes_[static_cast<std::size_t>(s)].output) {
                matches.push_back(Match{i + 1 - patterns_[p].size(), p});
            }
        }
    }
    std::ranges::sort(matches);
    return matches;
}

// --------------------------------------------------------------------------------------------
// Suffix array
// --------------------------------------------------------------------------------------------

SuffixArray::SuffixArray(std::string text)
    : text_(std::move(text)), sa_(text_.size()), lcp_(text_.size(), 0) {
    const std::size_t n = text_.size();
    if (n == 0) {
        return;
    }
    std::vector<std::size_t> rank(n);
    std::vector<std::size_t> tmp(n);
    for (std::size_t i = 0; i < n; ++i) {
        sa_[i] = i;
        rank[i] = static_cast<unsigned char>(text_[i]);
    }
    for (std::size_t k = 1;; k *= 2) {
        // Key of suffix i: (rank[i], rank[i + k] + 1 or 0 past the end).
        auto key = [&](std::size_t i) { return std::pair{rank[i], i + k < n ? rank[i + k] + 1 : 0}; };
        std::ranges::sort(sa_, [&](std::size_t a, std::size_t b) { return key(a) < key(b); });
        tmp[sa_[0]] = 0;
        for (std::size_t i = 1; i < n; ++i) {
            tmp[sa_[i]] = tmp[sa_[i - 1]] + (key(sa_[i - 1]) < key(sa_[i]) ? 1 : 0);
        }
        rank.swap(tmp);
        if (rank[sa_[n - 1]] == n - 1 || k >= n) {
            break; // all ranks distinct
        }
    }
    // Kasai: rank[] is now the inverse permutation of sa_.
    std::size_t h = 0;
    for (std::size_t i = 0; i < n; ++i) {
        if (rank[i] == 0) {
            h = 0;
            continue;
        }
        const std::size_t j = sa_[rank[i] - 1];
        while (i + h < n && j + h < n && text_[i + h] == text_[j + h]) {
            ++h;
        }
        lcp_[rank[i]] = h;
        if (h > 0) {
            --h;
        }
    }
}

std::pair<std::size_t, std::size_t> SuffixArray::range_of(std::string_view pattern) const {
    const std::string_view t(text_);
    const std::size_t m = pattern.size();
    auto prefix = [&](std::size_t suffix) { return t.substr(suffix, m); };
    const auto lo = std::ranges::partition_point(sa_, [&](std::size_t s) { return prefix(s) < pattern; });
    const auto hi = std::ranges::partition_point(sa_, [&](std::size_t s) { return prefix(s) <= pattern; });
    return {static_cast<std::size_t>(lo - sa_.begin()), static_cast<std::size_t>(hi - sa_.begin())};
}

std::vector<std::size_t> SuffixArray::find_all(std::string_view pattern) const {
    if (pattern.empty()) {
        return all_positions(text_.size());
    }
    const auto [lo, hi] = range_of(pattern);
    std::vector<std::size_t> out(sa_.begin() + static_cast<std::ptrdiff_t>(lo),
                                 sa_.begin() + static_cast<std::ptrdiff_t>(hi));
    std::ranges::sort(out);
    return out;
}

std::size_t SuffixArray::count(std::string_view pattern) const {
    if (pattern.empty()) {
        return text_.size() + 1;
    }
    const auto [lo, hi] = range_of(pattern);
    return hi - lo;
}

std::string SuffixArray::longest_repeated_substring() const {
    std::size_t best = 0;
    std::size_t at = 0;
    for (std::size_t i = 1; i < lcp_.size(); ++i) {
        if (lcp_[i] > best) {
            best = lcp_[i];
            at = sa_[i];
        }
    }
    return text_.substr(at, best);
}

// --------------------------------------------------------------------------------------------
// Approximate matching
// --------------------------------------------------------------------------------------------

std::size_t levenshtein_distance(std::string_view a, std::string_view b) {
    if (a.size() < b.size()) {
        std::swap(a, b); // b is the shorter: O(min) space
    }
    std::vector<std::size_t> row(b.size() + 1);
    std::iota(row.begin(), row.end(), std::size_t{0});
    for (std::size_t i = 1; i <= a.size(); ++i) {
        std::size_t diag = row[0]; // D[i-1][j-1]
        row[0] = i;
        for (std::size_t j = 1; j <= b.size(); ++j) {
            const std::size_t up = row[j]; // D[i-1][j]
            const std::size_t cost = a[i - 1] == b[j - 1] ? 0 : 1;
            row[j] = std::min({up + 1, row[j - 1] + 1, diag + cost});
            diag = up;
        }
    }
    return row[b.size()];
}

std::vector<FuzzyMatch> fuzzy_search(const std::vector<std::string>& dictionary, std::string_view query,
                                     std::size_t max_distance) {
    std::vector<FuzzyMatch> out;
    for (const auto& word : dictionary) {
        // Cheap lower bound: the length difference.
        const std::size_t len_diff = word.size() > query.size() ? word.size() - query.size()
                                                                : query.size() - word.size();
        if (len_diff > max_distance) {
            continue;
        }
        const std::size_t d = levenshtein_distance(word, query);
        if (d <= max_distance) {
            out.push_back(FuzzyMatch{word, d});
        }
    }
    std::ranges::sort(out, [](const FuzzyMatch& x, const FuzzyMatch& y) {
        return std::tie(x.distance, x.word) < std::tie(y.distance, y.word);
    });
    return out;
}

// --------------------------------------------------------------------------------------------
// Showcase
// --------------------------------------------------------------------------------------------

namespace {

void print_positions(std::ostream& out, std::string_view label, const std::vector<std::size_t>& v) {
    out << label << " [";
    for (std::size_t i = 0; i < v.size(); ++i) {
        out << (i ? ", " : "") << v[i];
    }
    out << "]\n";
}

} // namespace

void demonstrate_searching(std::ostream& out) {
    out << "=== Sequence search ===\n";
    std::vector<int> sorted{1, 3, 3, 3, 5, 8, 13, 21, 34, 55, 89};
    const int target = 3;
    out << "data: 1 3 3 3 5 8 13 21 34 55 89, target = " << target << '\n';
    out << "linear_search       -> index " << (linear_search(sorted, target) - sorted.begin()) << '\n';
    out << "lower_bound         -> index " << (lower_bound(sorted, target) - sorted.begin()) << '\n';
    out << "upper_bound         -> index " << (upper_bound(sorted, target) - sorted.begin()) << '\n';
    out << "equal_range size    -> " << equal_range(sorted, target).size() << '\n';
    out << "exponential_search  -> index " << (exponential_search(sorted, 21) - sorted.begin()) << " (21)\n";
    out << "jump_search         -> index " << (jump_search(sorted, 55) - sorted.begin()) << " (55)\n";
    out << "interpolation_search-> index " << (interpolation_search(sorted, 34) - sorted.begin())
        << " (34)\n";
    out << "binary_search(4)    -> " << std::boolalpha << binary_search(sorted, 4) << '\n';

    struct Planet {
        std::string name;
        double distance_au;
    };
    const std::vector<Planet> planets{{"Mercury", 0.39}, {"Venus", 0.72},  {"Earth", 1.0},
                                      {"Mars", 1.52},    {"Jupiter", 5.2}, {"Saturn", 9.54}};
    const auto first_outer = upper_bound(planets, 1.6, {}, &Planet::distance_au);
    out << "first planet beyond 1.6 AU (projection) -> " << first_outer->name << '\n';

    const std::vector<int> mountain{1, 4, 9, 15, 22, 18, 7, 2};
    out << "ternary_search_peak  -> value " << *ternary_search_peak(mountain) << '\n';
    const double x = ternary_search_max([](double t) { return -(t - 2.5) * (t - 2.5) + 4.0; }, 0.0, 10.0);
    out << "ternary_search_max of -(x-2.5)^2+4 on [0,10] -> x = " << x << '\n';

    out << "=== String search ===\n";
    const std::string text = "abracadabra abracadabra";
    const std::string pat = "abra";
    out << "text = \"" << text << "\", pattern = \"" << pat << "\"\n";
    print_positions(out, "naive  ", naive_search(text, pat));
    print_positions(out, "kmp    ", kmp_search(text, pat));
    print_positions(out, "bmh    ", boyer_moore_horspool_search(text, pat));
    print_positions(out, "rabin  ", rabin_karp_search(text, pat));
    print_positions(out, "z      ", z_search(text, pat));
    const std::vector<int> tokens{1, 2, 1, 2, 1, 2, 3};
    const std::vector<int> motif{1, 2, 1};
    print_positions(out, "kmp over std::vector<int> tokens, motif 1 2 1:", kmp_search(tokens, motif));

    const AhoCorasick ac({"he", "she", "his", "hers"});
    out << "aho_corasick in \"ushers\":";
    for (const auto& m : ac.find_all("ushers")) {
        out << ' ' << ac.patterns()[m.pattern_index] << '@' << m.position;
    }
    out << " (" << ac.state_count() << " states)\n";

    const SuffixArray sa("banana");
    out << "suffix array of \"banana\":";
    for (std::size_t s : sa.suffixes()) {
        out << ' ' << s;
    }
    out << "; count(\"ana\") = " << sa.count("ana") << "; longest repeat = \""
        << sa.longest_repeated_substring() << "\"\n";

    out << "=== Approximate search ===\n";
    out << "levenshtein(kitten, sitting) = " << levenshtein_distance("kitten", "sitting") << '\n';
    const std::vector<std::string> dict{"mercury", "venus",  "earth",  "mars",
                                        "jupiter", "saturn", "uranus", "neptune"};
    out << "fuzzy_search(\"satrun\", 2):";
    for (const auto& m : fuzzy_search(dict, "satrun", 2)) {
        out << ' ' << m.word << '(' << m.distance << ')';
    }
    out << '\n';

    out << "=== k-d tree nearest neighbours ===\n";
    using Tree = KDTree<double, 3>;
    std::mt19937_64 rng(2024);
    std::uniform_real_distribution<double> coord(-100.0, 100.0);
    std::vector<Tree::Point> stars(500);
    for (auto& p : stars) {
        p = {coord(rng), coord(rng), coord(rng)};
    }
    const Tree tree(stars);
    const Tree::Point probe{0.0, 0.0, 0.0};
    const auto knn = tree.nearest(probe, 3);
    const bool agrees = knn == Tree::brute_force_nearest(stars, probe, 3);
    out << "3 nearest stars to origin:";
    for (std::size_t i : knn) {
        out << ' ' << i << " (d=" << std::sqrt(Tree::squared_distance(stars[i], probe)) << ')';
    }
    out << (agrees ? "  [matches brute force]" : "  [MISMATCH]") << '\n';
    out << "stars within 25 units of origin: " << tree.within_radius(probe, 25.0).size() << '\n';
}

} // namespace CppVerseHub::Algorithms
