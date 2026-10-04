// Tests for stl_showcase/Iterators.hpp
#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <deque>
#include <forward_list>
#include <iterator>
#include <list>
#include <map>
#include <memory>
#include <numeric>
#include <ranges>
#include <set>
#include <span>
#include <sstream>
#include <string>
#include <vector>

#include "stl_showcase/Iterators.hpp"

using namespace CppVerseHub::STL;

namespace {
bool isOdd(int x) { return x % 2 != 0; }
}  // namespace

TEST_CASE("iteratorCategoryName reports the strongest concept", "[iterators][concepts]") {
    STATIC_REQUIRE(iteratorCategoryName<int*>() == "contiguous");
    STATIC_REQUIRE(iteratorCategoryName<std::vector<int>::iterator>() == "contiguous");
    STATIC_REQUIRE(iteratorCategoryName<std::deque<int>::iterator>() == "random_access");
    STATIC_REQUIRE(iteratorCategoryName<std::list<int>::iterator>() == "bidirectional");
    STATIC_REQUIRE(iteratorCategoryName<std::set<int>::const_iterator>() == "bidirectional");
    STATIC_REQUIRE(iteratorCategoryName<std::forward_list<int>::iterator>() == "forward");
    STATIC_REQUIRE(iteratorCategoryName<std::istream_iterator<int>>() == "input");
    STATIC_REQUIRE(iteratorCategoryName<std::ostream_iterator<int>>() == "output");
    STATIC_REQUIRE(iteratorCategoryName<std::back_insert_iterator<std::vector<int>>>() == "output");
    STATIC_REQUIRE(iteratorCategoryName<int>() == "none");
    STATIC_REQUIRE(iteratorCategoryName<SimpleVector<int>::iterator>() == "contiguous");
    STATIC_REQUIRE(iteratorCategoryName<IntFilterView::iterator>() == "forward");
    STATIC_REQUIRE(iteratorCategoryName<FibonacciRange::iterator>() == "forward");
}

TEST_CASE("advanceBy and distanceBetween dispatch on iterator concepts", "[iterators][utilities]") {
    std::vector<int> v{0, 1, 2, 3, 4, 5};
    auto vit = v.begin();
    advanceBy(vit, 4);
    REQUIRE(*vit == 4);
    advanceBy(vit, -3);
    REQUIRE(*vit == 1);

    std::list<int> l(v.begin(), v.end());
    auto lit = l.end();
    advanceBy(lit, -2);
    REQUIRE(*lit == 4);

    std::forward_list<int> f(v.begin(), v.end());
    auto fit = f.begin();
    advanceBy(fit, 5);
    REQUIRE(*fit == 5);
    REQUIRE_THROWS_AS(advanceBy(fit, -1), std::invalid_argument);

    REQUIRE(distanceBetween(v.begin(), v.end()) == 6);
    REQUIRE(distanceBetween(f.begin(), f.end()) == 6);
    REQUIRE(distanceBetween(l.begin(), l.begin()) == 0);
    const FibonacciRange fib(10);
    REQUIRE(distanceBetween(fib.begin(), fib.end()) == 10);  // iterator/sentinel pair
}

TEST_CASE("SimpleVector basic operations and growth", "[iterators][simplevector]") {
    SimpleVector<std::string> names;
    REQUIRE(names.empty());
    REQUIRE(names.begin() == names.end());
    for (int i = 0; i < 20; ++i) {
        names.push_back("ship-" + std::to_string(i));
    }
    REQUIRE(names.size() == 20);
    REQUIRE(names.capacity() >= 20);
    REQUIRE(names[0] == "ship-0");
    REQUIRE(names.at(19) == "ship-19");
    REQUIRE_THROWS_AS(names.at(20), std::out_of_range);
    names.emplace_back(3, 'x');
    REQUIRE(names[20] == "xxx");
    names.pop_back();
    REQUIRE(names.size() == 20);
    names.clear();
    REQUIRE(names.empty());
    REQUIRE_THROWS_AS(names.pop_back(), std::out_of_range);
}

TEST_CASE("SimpleVector push_back of an aliased element survives reallocation", "[iterators][simplevector]") {
    SimpleVector<std::string> v{"alpha"};
    REQUIRE(v.capacity() == 1);
    v.push_back(v[0]);  // forces growth while referencing an element
    REQUIRE(v.size() == 2);
    REQUIRE(v[1] == "alpha");
    v.emplace_back(v[1]);
    REQUIRE(v[2] == "alpha");
}

TEST_CASE("SimpleVector copy and move semantics", "[iterators][simplevector]") {
    SimpleVector<int> a{1, 2, 3};
    SimpleVector<int> b(a);
    b[0] = 99;
    REQUIRE(a[0] == 1);
    REQUIRE_FALSE(a == b);

    SimpleVector<int> c;
    c = a;
    REQUIRE(c == a);

    SimpleVector<int> d(std::move(c));
    REQUIRE(d == a);
    REQUIRE(c.empty());  // NOLINT(bugprone-use-after-move): moved-from state is specified

    SimpleVector<int> e(5, 7);
    e = std::move(d);
    REQUIRE(e == a);
    REQUIRE(SimpleVector<int>(3, 4) == SimpleVector<int>{4, 4, 4});
}

TEST_CASE("SimpleVector works with move-only element types", "[iterators][simplevector]") {
    SimpleVector<std::unique_ptr<int>> owned;
    for (int i = 0; i < 6; ++i) {
        owned.push_back(std::make_unique<int>(i));
    }
    REQUIRE(*owned[5] == 5);
    auto moved = std::move(owned);
    REQUIRE(moved.size() == 6);
    REQUIRE(*moved[0] == 0);
}

TEST_CASE("SimpleVector iterators support random-access arithmetic", "[iterators][simplevector]") {
    SimpleVector<int> v{10, 20, 30, 40, 50};
    auto it = v.begin();
    REQUIRE(*(it + 2) == 30);
    REQUIRE(*(2 + it) == 30);
    REQUIRE(it[4] == 50);
    it += 3;
    REQUIRE(*it == 40);
    it -= 1;
    REQUIRE(*it-- == 30);
    REQUIRE(*it == 20);
    REQUIRE(v.end() - v.begin() == 5);
    REQUIRE(v.begin() < v.end());
    REQUIRE((v.begin() <=> v.begin()) == std::strong_ordering::equal);
    SimpleVector<int>::const_iterator cit = v.begin();  // iterator -> const_iterator
    REQUIRE(cit == v.cbegin());
    REQUIRE(std::to_address(v.begin() + 1) == v.data() + 1);
}

TEST_CASE("SimpleVector cooperates with std::ranges algorithms and std::span", "[iterators][simplevector]") {
    SimpleVector<int> v{5, 3, 9, 1, 7};
    std::ranges::sort(v);
    REQUIRE(std::ranges::is_sorted(v));
    REQUIRE(std::ranges::binary_search(v, 7));
    std::ranges::reverse(v);
    REQUIRE(v == SimpleVector<int>{9, 7, 5, 3, 1});
    const std::span<int> view(v.begin(), v.end());
    REQUIRE(view.data() == v.data());
    REQUIRE(std::accumulate(view.begin(), view.end(), 0) == 25);
    auto doubled = v | std::views::transform([](int x) { return 2 * x; });
    REQUIRE(std::ranges::max(doubled) == 18);
}

TEST_CASE("FilterView yields only matching elements, lazily", "[iterators][filterview]") {
    std::vector<int> data{1, 2, 3, 4, 5, 6, 7, 8};
    FilterView odds(data, &isOdd);
    std::vector<int> collected(odds.begin(), odds.end());
    REQUIRE(collected == std::vector<int>{1, 3, 5, 7});

    data[1] = 11;  // views see later changes to the underlying range
    REQUIRE(std::ranges::count(odds, 11) == 1);
    REQUIRE(std::ranges::distance(odds) == 5);
}

TEST_CASE("FilterView models forward_range and works with ranges algorithms", "[iterators][filterview]") {
    std::vector<int> data{4, 9, 2, 15, 6, 21, 8};
    auto pred = [](int x) { return x > 5; };
    FilterView big(data, pred);
    STATIC_REQUIRE(std::ranges::forward_range<decltype(big)>);
    STATIC_REQUIRE(std::forward_iterator<std::ranges::iterator_t<decltype(big)>>);

    REQUIRE(std::ranges::max(big) == 21);
    REQUIRE(std::ranges::count_if(big, [](int x) { return x % 3 == 0; }) == 4);  // 9, 15, 6, 21
    REQUIRE(*std::ranges::find(big, 15) == 15);
    REQUIRE(std::ranges::is_sorted(big) == false);
    auto it = std::ranges::adjacent_find(big, std::ranges::greater{});
    REQUIRE(*it == 15);  // 15 > 6
    REQUIRE(std::ranges::find(big, 4) == big.end());  // filtered out

    // Multi-pass guarantee: two independent traversals see the same sequence.
    auto first = big.begin();
    auto second = first;
    ++first;
    REQUIRE(*second == 9);
    REQUIRE(*first == 15);
    REQUIRE(*second++ == 9);
    REQUIRE(second == first);
}

TEST_CASE("FilterView writes through to the base and composes with views", "[iterators][filterview]") {
    std::vector<int> data{1, 2, 3, 4, 5};
    FilterView evens(data, [](int x) { return x % 2 == 0; });
    std::ranges::fill(evens, 0);
    REQUIRE(data == std::vector<int>{1, 0, 3, 0, 5});

    std::list<std::string> words{"alpha", "be", "gamma", "pi"};
    FilterView long_words(words, [](const std::string& w) { return w.size() > 2; });
    std::vector<std::size_t> lengths;
    std::ranges::copy(long_words | std::views::transform([](const std::string& w) { return w.size(); }), std::back_inserter(lengths));
    REQUIRE(lengths == std::vector<std::size_t>{5, 5});

    std::vector<int> none{1, 3, 5};
    FilterView empty_view(none, [](int x) { return x > 100; });
    REQUIRE(empty_view.begin() == empty_view.end());
    REQUIRE(empty_view.empty());
}

TEST_CASE("FibonacciRange generates terms lazily up to its bound", "[iterators][fibonacci]") {
    std::vector<std::uint64_t> terms;
    std::ranges::copy(FibonacciRange(10), std::back_inserter(terms));
    REQUIRE(terms == std::vector<std::uint64_t>{0, 1, 1, 2, 3, 5, 8, 13, 21, 34});
    REQUIRE(std::ranges::distance(FibonacciRange(0)) == 0);

    std::uint64_t last = 0;
    for (const auto value : FibonacciRange(FibonacciRange::max_count)) {
        last = value;
    }
    REQUIRE(last == 12200160415121876738ULL);  // fib(93)
    REQUIRE_THROWS_AS(FibonacciRange(FibonacciRange::max_count + 1), std::out_of_range);
}

TEST_CASE("FibonacciRange is a borrowed forward range", "[iterators][fibonacci]") {
    const auto it = std::ranges::find_if(FibonacciRange(50), [](std::uint64_t v) { return v > 1000; });
    STATIC_REQUIRE(std::same_as<std::remove_const_t<decltype(it)>, FibonacciRange::iterator>);
    REQUIRE(*it == 1597);
    REQUIRE(it.index() == 17);
    constexpr auto sum = [] {
        std::uint64_t total = 0;
        for (const auto v : FibonacciRange(10)) {
            total += v;
        }
        return total;
    }();
    STATIC_REQUIRE(sum == 88);
    auto copy = it;
    ++copy;
    REQUIRE(*it == 1597);  // multi-pass: copies advance independently
    REQUIRE(*copy == 2584);
}

TEST_CASE("Stream iterator helpers", "[iterators][adapters]") {
    REQUIRE(parseInts("1 2  -3\n40") == std::vector<int>{1, 2, -3, 40});
    REQUIRE(parseInts("5 6 x 7") == std::vector<int>{5, 6});
    REQUIRE(parseInts("").empty());
    REQUIRE(joinInts(std::vector<int>{1, 2, 3}, ", ") == "1, 2, 3");
    REQUIRE(joinInts(std::vector<int>{7}, "-") == "7");
    REQUIRE(joinInts(std::vector<int>{}, ",").empty());
}

TEST_CASE("Reverse iterators and base()", "[iterators][adapters]") {
    const std::vector<int> v{1, 2, 3, 4};
    REQUIRE(reversedCopy(v) == std::vector<int>{4, 3, 2, 1});
    const auto rit = std::ranges::find(v.rbegin(), v.rend(), 3);
    REQUIRE(*rit == 3);
    REQUIRE(*(rit.base() - 1) == 3);  // base() points one past the referenced element
    REQUIRE(rit.base() - v.begin() == 3);
}

TEST_CASE("Insert iterators and move iterators", "[iterators][adapters]") {
    const std::vector<int> front{1, 2};
    const std::vector<int> middle{10, 20};
    const std::vector<int> back{99};
    REQUIRE(buildWithInserters(front, middle, back) == std::list<int>{2, 1, 10, 20, 99});

    std::vector<std::unique_ptr<int>> source;
    source.push_back(std::make_unique<int>(1));
    source.push_back(std::make_unique<int>(2));
    const auto moved = moveAll(source);
    REQUIRE(source.empty());
    REQUIRE(moved.size() == 2);
    REQUIRE(*moved[1] == 2);
}

TEST_CASE("eraseWhileIterating works across container types", "[iterators][invalidation]") {
    std::vector<int> v{1, 2, 3, 4, 5, 6};
    REQUIRE(eraseWhileIterating(v, [](int x) { return x % 2 == 0; }) == 3);
    REQUIRE(v == std::vector<int>{1, 3, 5});

    std::list<int> l{1, 2, 3, 4};
    const auto survivor = std::next(l.begin(), 2);  // 3
    REQUIRE(eraseWhileIterating(l, [](int x) { return x != 3; }) == 3);
    REQUIRE(*survivor == 3);  // list iterators to remaining nodes stay valid
    REQUIRE(l.size() == 1);

    std::map<int, char> m{{1, 'a'}, {2, 'b'}, {3, 'c'}};
    REQUIRE(eraseWhileIterating(m, [](const auto& kv) { return kv.first > 1; }) == 2);
    REQUIRE(m.size() == 1);
    REQUIRE(m.begin()->second == 'a');
}

TEST_CASE("sampleStarSystems is sorted by distance", "[iterators][data]") {
    const auto systems = sampleStarSystems();
    REQUIRE(systems.size() == 5);
    REQUIRE(std::ranges::is_sorted(systems, {}, &StarSystem::distance_ly));
}
