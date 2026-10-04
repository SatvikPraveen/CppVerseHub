// Property-based and behavioural tests for algorithms/SearchAlgorithms.hpp.

#include "algorithms/SearchAlgorithms.hpp"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

#include <algorithm>
#include <cmath>
#include <list>
#include <numeric>
#include <random>
#include <set>
#include <sstream>
#include <string>
#include <vector>

using namespace CppVerseHub::Algorithms;

namespace {

std::vector<int> sorted_with_duplicates(std::size_t n, int spread, std::uint64_t seed) {
    std::mt19937_64 rng(seed);
    std::uniform_int_distribution<int> dist(-spread, spread);
    std::vector<int> v(n);
    for (int& x : v) {
        x = dist(rng);
    }
    std::sort(v.begin(), v.end());
    return v;
}

std::string random_string(std::size_t n, char alphabet_size, std::mt19937_64& rng) {
    std::uniform_int_distribution<int> dist(0, alphabet_size - 1);
    std::string s(n, 'a');
    for (char& c : s) {
        c = static_cast<char>('a' + dist(rng));
    }
    return s;
}

}  // namespace

// ---------------------------------------------------------------------------------------------
// Sequence searches
// ---------------------------------------------------------------------------------------------

TEST_CASE("binary-search family agrees with the standard library", "[search][binary][property]") {
    for (std::size_t n : {0u, 1u, 2u, 3u, 7u, 8u, 9u, 100u, 1000u}) {
        for (std::uint64_t seed = 0; seed < 5; ++seed) {
            const auto v = sorted_with_duplicates(n, 20, seed);
            for (int target = -23; target <= 23; ++target) {
                CAPTURE(n, seed, target);
                const auto lb = std::lower_bound(v.begin(), v.end(), target);
                const auto ub = std::upper_bound(v.begin(), v.end(), target);
                REQUIRE(lower_bound(v, target) == lb);
                REQUIRE(upper_bound(v, target) == ub);
                const auto er = equal_range(v, target);
                REQUIRE(er.begin() == lb);
                REQUIRE(er.end() == ub);
                REQUIRE(binary_search(v, target) == std::binary_search(v.begin(), v.end(), target));
                REQUIRE(exponential_search(v, target) == lb);
                REQUIRE(jump_search(v, target) == lb);
                REQUIRE(linear_search(v, target) == std::find(v.begin(), v.end(), target));
            }
        }
    }
}

TEST_CASE("binary searches respect comparators and projections", "[search][binary]") {
    std::vector<int> desc{9, 7, 7, 5, 3, 3, 3, 1};
    REQUIRE(lower_bound(desc, 3, std::ranges::greater{}) - desc.begin() == 4);
    REQUIRE(upper_bound(desc, 3, std::ranges::greater{}) - desc.begin() == 7);
    REQUIRE(exponential_search(desc, 7, std::ranges::greater{}) - desc.begin() == 1);
    REQUIRE(jump_search(desc, 0, std::ranges::greater{}) == desc.end());

    struct Item {
        std::string name;
        int priority;
    };
    const std::vector<Item> items{{"a", 1}, {"b", 4}, {"c", 4}, {"d", 9}};
    const auto er = equal_range(items, 4, {}, &Item::priority);
    REQUIRE(er.size() == 2);
    REQUIRE(er.begin()->name == "b");
    REQUIRE(binary_search(items, 9, {}, &Item::priority));
    REQUIRE_FALSE(binary_search(items, 5, {}, &Item::priority));
}

TEST_CASE("interpolation search finds the first equal key on uniform and skewed data",
          "[search][interpolation][property]") {
    for (std::uint64_t seed = 0; seed < 6; ++seed) {
        auto uniform = sorted_with_duplicates(500, 1000, seed);
        std::vector<long long> skewed;
        for (int i = 0; i < 40; ++i) {
            skewed.push_back(1LL << i);  // exponential growth: interpolation's worst case
            skewed.push_back(1LL << i);
        }
        for (int target = -1005; target <= 1005; target += 7) {
            const auto it = interpolation_search(uniform, target);
            const auto lb = std::lower_bound(uniform.begin(), uniform.end(), target);
            if (lb != uniform.end() && *lb == target) {
                REQUIRE(it == lb);
            } else {
                REQUIRE(it == uniform.end());
            }
        }
        for (long long target : {1LL, 2LL, 3LL, 1LL << 20, (1LL << 39), (1LL << 39) + 1}) {
            const auto it = interpolation_search(skewed, target);
            const auto lb = std::lower_bound(skewed.begin(), skewed.end(), target);
            REQUIRE(it == ((lb != skewed.end() && *lb == target) ? lb : skewed.end()));
        }
    }
    std::vector<double> reals{0.5, 1.25, 2.0, 2.0, 7.75};
    REQUIRE(interpolation_search(reals, 2.0) - reals.begin() == 2);
    REQUIRE(interpolation_search(reals, 3.0) == reals.end());
    std::vector<int> empty;
    REQUIRE(interpolation_search(empty, 1) == empty.end());
}

TEST_CASE("linear search works on non-random-access ranges", "[search][linear]") {
    const std::list<std::string> names{"vega", "rigel", "deneb", "rigel"};
    const auto it = linear_search(names, std::string("rigel"));
    REQUIRE(it != names.end());
    REQUIRE(std::distance(names.begin(), it) == 1);
    REQUIRE(linear_search(names, std::string("sol")) == names.end());
    const auto by_len = linear_search(names, std::size_t{5}, [](const std::string& s) { return s.size(); });
    REQUIRE(*by_len == "rigel");
}

TEST_CASE("ternary peak search locates the maximum of unimodal sequences", "[search][ternary][property]") {
    std::mt19937_64 rng(17);
    for (int trial = 0; trial < 200; ++trial) {
        std::uniform_int_distribution<std::size_t> len(1, 60);
        const std::size_t up = len(rng);
        const std::size_t down = len(rng) - 1;
        std::vector<int> v;
        int x = 0;
        for (std::size_t i = 0; i < up; ++i) {
            x += 1 + static_cast<int>(rng() % 5);
            v.push_back(x);
        }
        for (std::size_t i = 0; i < down; ++i) {
            x -= 1 + static_cast<int>(rng() % 5);
            v.push_back(x);
        }
        REQUIRE(ternary_search_peak(v) == std::max_element(v.begin(), v.end()));
    }
    std::vector<int> empty;
    REQUIRE(ternary_search_peak(empty) == empty.end());
    std::vector<int> increasing{1, 2, 3, 4};
    REQUIRE(*ternary_search_peak(increasing) == 4);
    std::vector<int> valley_by_greater{5, 3, 1, 2, 8};  // unimodal under the reversed order
    REQUIRE(*ternary_search_peak(valley_by_greater, std::ranges::greater{}) == 1);
}

TEST_CASE("ternary search finds the maximiser of a unimodal function", "[search][ternary]") {
    using Catch::Approx;
    REQUIRE(ternary_search_max([](double x) { return -(x - 1.7) * (x - 1.7); }, -10.0, 10.0) ==
            Approx(1.7).margin(1e-6));
    REQUIRE(ternary_search_max([](double x) { return std::sin(x); }, 0.0, 3.0) ==
            Approx(1.5707963).margin(1e-6));
    REQUIRE(ternary_search_max([](double x) { return x; }, 2.0, 2.0) == Approx(2.0));
}

// ---------------------------------------------------------------------------------------------
// Exact string matching
// ---------------------------------------------------------------------------------------------

TEST_CASE("all exact string matchers agree with the naive oracle", "[search][string][property]") {
    std::mt19937_64 rng(123);
    for (char alphabet : {char{1}, char{2}, char{3}, char{26}}) {
        for (int trial = 0; trial < 150; ++trial) {
            const std::string text = random_string(rng() % 120, alphabet, rng);
            const std::string pattern = random_string(rng() % 7, alphabet, rng);
            CAPTURE(text, pattern);
            const auto expected = naive_search(text, pattern);
            REQUIRE(kmp_search(text, pattern) == expected);
            REQUIRE(boyer_moore_horspool_search(text, pattern) == expected);
            REQUIRE(rabin_karp_search(text, pattern) == expected);
            REQUIRE(z_search(text, pattern) == expected);
        }
    }
}

TEST_CASE("string matchers handle edge cases", "[search][string]") {
    const std::vector<std::size_t> every{0, 1, 2, 3};
    for (auto* fn : {&naive_search, &boyer_moore_horspool_search, &rabin_karp_search, &z_search}) {
        REQUIRE(fn("abc", "") == every);
        REQUIRE(fn("", "a").empty());
        REQUIRE(fn("ab", "abc").empty());
        REQUIRE(fn("aaaa", "aa") == std::vector<std::size_t>{0, 1, 2});
        REQUIRE(fn("abc", "abc") == std::vector<std::size_t>{0});
        const std::string binary("a\0b\xff" "a\0b", 7);
        REQUIRE(fn(binary, std::string_view("a\0b", 3)) == std::vector<std::size_t>{0, 4});
    }
    REQUIRE(kmp_search(std::string("abc"), std::string()) == every);
}

TEST_CASE("prefix function and Z function match textbook values", "[search][string]") {
    REQUIRE(prefix_function(std::string("aabaaab")) == std::vector<std::size_t>{0, 1, 0, 1, 2, 2, 3});
    REQUIRE(prefix_function(std::string("abcd")) == std::vector<std::size_t>{0, 0, 0, 0});
    REQUIRE(z_function("aaabaab") == std::vector<std::size_t>{7, 2, 1, 0, 2, 1, 0});
    REQUIRE(z_function("").empty());
}

TEST_CASE("KMP is generic over sequences of any equality-comparable type", "[search][string][generic]") {
    const std::vector<int> genome{1, 2, 1, 2, 1, 2, 3, 1, 2, 1};
    REQUIRE(kmp_search(genome, std::vector<int>{1, 2, 1}) == std::vector<std::size_t>{0, 2, 7});
    const std::vector<std::string> words{"to", "be", "or", "not", "to", "be"};
    REQUIRE(kmp_search(words, std::vector<std::string>{"to", "be"}) == std::vector<std::size_t>{0, 4});
}

TEST_CASE("Aho-Corasick reports exactly the per-pattern naive matches", "[search][aho-corasick][property]") {
    std::mt19937_64 rng(5);
    for (int trial = 0; trial < 60; ++trial) {
        std::vector<std::string> patterns;
        for (int p = 0; p < 6; ++p) {
            patterns.push_back(random_string(1 + rng() % 4, 3, rng));
        }
        const std::string text = random_string(200, 3, rng);
        const AhoCorasick ac(patterns);
        std::vector<AhoCorasick::Match> expected;
        for (std::size_t p = 0; p < patterns.size(); ++p) {
            for (std::size_t pos : naive_search(text, patterns[p])) {
                expected.push_back({pos, p});
            }
        }
        std::sort(expected.begin(), expected.end());
        REQUIRE(ac.find_all(text) == expected);
    }
}

TEST_CASE("Aho-Corasick classic example and degenerate inputs", "[search][aho-corasick]") {
    const AhoCorasick ac({"he", "she", "his", "hers"});
    const auto m = ac.find_all("ushers");
    REQUIRE(m == std::vector<AhoCorasick::Match>{{1, 1}, {2, 0}, {2, 3}});
    REQUIRE(ac.state_count() == 10);  // root + h,he,her,hers,hi,his,s,sh,she

    const AhoCorasick with_empty({"", "a", "a"});
    REQUIRE(with_empty.find_all("aa") == std::vector<AhoCorasick::Match>{{0, 1}, {0, 2}, {1, 1}, {1, 2}});
    const AhoCorasick none({});
    REQUIRE(none.find_all("anything").empty());
}

TEST_CASE("suffix array and LCP match brute force", "[search][suffix-array][property]") {
    const SuffixArray banana("banana");
    REQUIRE(banana.suffixes() == std::vector<std::size_t>{5, 3, 1, 0, 4, 2});
    REQUIRE(banana.lcp() == std::vector<std::size_t>{0, 1, 3, 0, 0, 2});
    REQUIRE(banana.longest_repeated_substring() == "ana");
    REQUIRE(banana.count("an") == 2);
    REQUIRE(banana.find_all("a") == std::vector<std::size_t>{1, 3, 5});
    REQUIRE(banana.count("x") == 0);

    std::mt19937_64 rng(31);
    for (int trial = 0; trial < 80; ++trial) {
        const std::string text = random_string(rng() % 80, static_cast<char>(1 + rng() % 3), rng);
        const SuffixArray sa(text);
        std::vector<std::size_t> brute(text.size());
        std::iota(brute.begin(), brute.end(), std::size_t{0});
        std::sort(brute.begin(), brute.end(),
                  [&](std::size_t a, std::size_t b) { return text.substr(a) < text.substr(b); });
        REQUIRE(sa.suffixes() == brute);
        for (std::size_t i = 1; i < brute.size(); ++i) {
            const std::string a = text.substr(brute[i - 1]);
            const std::string b = text.substr(brute[i]);
            std::size_t l = 0;
            while (l < a.size() && l < b.size() && a[l] == b[l]) {
                ++l;
            }
            REQUIRE(sa.lcp()[i] == l);
        }
        std::size_t longest = 0;
        for (std::size_t i = 0; i < text.size(); ++i) {
            for (std::size_t j = i + 1; j < text.size(); ++j) {
                std::size_t l = 0;
                while (j + l < text.size() && text[i + l] == text[j + l]) {
                    ++l;
                }
                longest = std::max(longest, l);
            }
        }
        const std::string lrs = sa.longest_repeated_substring();
        REQUIRE(lrs.size() == longest);
        if (!lrs.empty()) {
            REQUIRE(naive_search(text, lrs).size() >= 2);
        }
        const std::string pattern = random_string(1 + rng() % 3, 3, rng);
        REQUIRE(sa.find_all(pattern) == naive_search(text, pattern));
        REQUIRE(sa.count(pattern) == naive_search(text, pattern).size());
    }
}

// ---------------------------------------------------------------------------------------------
// Approximate matching
// ---------------------------------------------------------------------------------------------

TEST_CASE("Levenshtein distance known values and metric axioms", "[search][levenshtein][property]") {
    REQUIRE(levenshtein_distance("kitten", "sitting") == 3);
    REQUIRE(levenshtein_distance("flaw", "lawn") == 2);
    REQUIRE(levenshtein_distance("", "abc") == 3);
    REQUIRE(levenshtein_distance("abc", "") == 3);
    REQUIRE(levenshtein_distance("same", "same") == 0);
    REQUIRE(levenshtein_distance("intention", "execution") == 5);

    std::mt19937_64 rng(8);
    for (int trial = 0; trial < 200; ++trial) {
        const auto a = random_string(rng() % 9, 3, rng);
        const auto b = random_string(rng() % 9, 3, rng);
        const auto c = random_string(rng() % 9, 3, rng);
        const auto ab = levenshtein_distance(a, b);
        REQUIRE(ab == levenshtein_distance(b, a));
        REQUIRE(ab <= levenshtein_distance(a, c) + levenshtein_distance(c, b));
        REQUIRE(ab >= (a.size() > b.size() ? a.size() - b.size() : b.size() - a.size()));
        REQUIRE(ab <= std::max(a.size(), b.size()));
        REQUIRE((ab == 0) == (a == b));
    }
}

TEST_CASE("fuzzy search returns close words ordered by distance then word", "[search][levenshtein]") {
    const std::vector<std::string> dict{"mars", "mass", "maps", "venus", "marsh", "bars", "jupiter"};
    const auto m = fuzzy_search(dict, "mars", 1);
    REQUIRE(m == std::vector<FuzzyMatch>{{"mars", 0}, {"bars", 1}, {"maps", 1}, {"marsh", 1}, {"mass", 1}});
    REQUIRE(fuzzy_search(dict, "zzzzzz", 2).empty());
    REQUIRE(fuzzy_search({}, "mars", 3).empty());
}

// ---------------------------------------------------------------------------------------------
// Nearest neighbours
// ---------------------------------------------------------------------------------------------

TEST_CASE("k-d tree k-NN and radius queries match brute force", "[search][kdtree][property]") {
    std::mt19937_64 rng(2025);
    std::uniform_real_distribution<double> coord(-10.0, 10.0);
    SECTION("2-D with duplicate points") {
        using Tree = KDTree<double, 2>;
        std::vector<Tree::Point> pts;
        for (int i = 0; i < 300; ++i) {
            pts.push_back({std::round(coord(rng)), std::round(coord(rng))});  // integer grid: many ties
        }
        const Tree tree(pts);
        for (int q = 0; q < 100; ++q) {
            const Tree::Point query{coord(rng), coord(rng)};
            for (std::size_t k : {1u, 4u, 17u}) {
                REQUIRE(tree.nearest(query, k) == Tree::brute_force_nearest(pts, query, k));
            }
            const double r = std::abs(coord(rng));
            std::vector<std::size_t> expected;
            for (std::size_t i = 0; i < pts.size(); ++i) {
                if (Tree::squared_distance(pts[i], query) <= r * r) {
                    expected.push_back(i);
                }
            }
            REQUIRE(tree.within_radius(query, r) == expected);
        }
    }
    SECTION("3-D float") {
        using Tree = KDTree<float, 3>;
        std::uniform_real_distribution<float> fc(-5.0F, 5.0F);
        std::vector<Tree::Point> pts(400);
        for (auto& p : pts) {
            p = {fc(rng), fc(rng), fc(rng)};
        }
        const Tree tree(pts);
        REQUIRE(tree.size() == 400);
        for (int q = 0; q < 50; ++q) {
            const Tree::Point query{fc(rng), fc(rng), fc(rng)};
            REQUIRE(tree.nearest(query, 5) == Tree::brute_force_nearest(pts, query, 5));
        }
    }
    SECTION("degenerate trees") {
        using Tree = KDTree<double, 2>;
        const Tree empty(std::vector<Tree::Point>{});
        REQUIRE(empty.nearest({0.0, 0.0}, 3).empty());
        REQUIRE(empty.within_radius({0.0, 0.0}, 1.0).empty());
        const Tree single(std::vector<Tree::Point>{Tree::Point{1.0, 1.0}});
        REQUIRE(single.nearest({5.0, 5.0}, 3) == std::vector<std::size_t>{0});
        REQUIRE(single.nearest({5.0, 5.0}, 0).empty());
    }
}

TEST_CASE("searching demo produces output that agrees with brute force", "[search][demo]") {
    std::ostringstream oss;
    demonstrate_searching(oss);
    const std::string s = oss.str();
    REQUIRE_FALSE(s.empty());
    REQUIRE(s.find("matches brute force") != std::string::npos);
    REQUIRE(s.find("first planet beyond 1.6 AU (projection) -> Jupiter") != std::string::npos);
    REQUIRE(s.find("longest repeat = \"ana\"") != std::string::npos);
}
