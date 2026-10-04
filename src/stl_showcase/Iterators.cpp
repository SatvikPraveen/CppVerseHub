/**
 * @file Iterators.cpp
 * @brief Implementation of the iterator showcase.
 */
#include "stl_showcase/Iterators.hpp"

#include <array>
#include <deque>
#include <forward_list>
#include <map>
#include <numeric>
#include <sstream>

namespace CppVerseHub::STL {

std::vector<StarSystem> sampleStarSystems() {
    return {
        {"Alpha Centauri", "G2V", 3, 4.37},   {"Barnard's Star", "M4V", 1, 5.96}, {"Sirius", "A1V", 0, 8.60},
        {"Epsilon Eridani", "K2V", 2, 10.50}, {"Tau Ceti", "G8V", 4, 11.90},
    };
}

std::vector<int> parseInts(std::string_view text) {
    std::istringstream stream{std::string(text)};
    return {std::istream_iterator<int>(stream), std::istream_iterator<int>()};
}

std::string joinInts(std::span<const int> values, std::string_view delimiter) {
    std::ostringstream stream;
    if (!values.empty()) {
        const std::string delim(delimiter);
        std::copy(values.begin(), values.end() - 1, std::ostream_iterator<int>(stream, delim.c_str()));
        stream << values.back();
    }
    return stream.str();
}

std::vector<int> reversedCopy(std::span<const int> values) {
    return {values.rbegin(), values.rend()};
}

std::list<int> buildWithInserters(std::span<const int> front, std::span<const int> middle,
                                  std::span<const int> back) {
    std::list<int> result;
    std::ranges::copy(front, std::front_inserter(result));
    std::ranges::copy(middle, std::inserter(result, result.end()));
    std::ranges::copy(back, std::back_inserter(result));
    return result;
}

// ------------------------------------------------------------------------------ demonstrations

namespace {

template <typename Range>
void printRange(std::ostream& out, std::string_view label, Range&& range) {
    out << label << ":";
    for (auto&& value : range) {
        out << ' ' << value;
    }
    out << '\n';
}

} // namespace

void demonstrateIteratorCategories(std::ostream& out) {
    out << "\n=== Iterator Categories (C++20 concepts) ===\n";
    out << "std::vector<int>::iterator       -> " << iteratorCategoryName<std::vector<int>::iterator>()
        << '\n';
    out << "std::deque<int>::iterator        -> " << iteratorCategoryName<std::deque<int>::iterator>()
        << '\n';
    out << "std::list<int>::iterator         -> " << iteratorCategoryName<std::list<int>::iterator>() << '\n';
    out << "std::forward_list<int>::iterator -> " << iteratorCategoryName<std::forward_list<int>::iterator>()
        << '\n';
    out << "std::istream_iterator<int>       -> " << iteratorCategoryName<std::istream_iterator<int>>()
        << '\n';
    out << "std::ostream_iterator<int>       -> " << iteratorCategoryName<std::ostream_iterator<int>>()
        << '\n';

    const auto systems = sampleStarSystems();
    std::list<StarSystem> route(systems.begin(), systems.end());
    out << "bidirectional walk backwards:";
    for (auto it = route.rbegin(); it != route.rend(); ++it) {
        out << " [" << it->name << ']';
    }
    out << '\n';
    const auto mid = systems.begin() + static_cast<std::ptrdiff_t>(systems.size() / 2);
    out << "random access: middle system is " << mid->name << ", distance from begin "
        << (mid - systems.begin()) << '\n';
}

void demonstrateIteratorAdapters(std::ostream& out) {
    out << "\n=== Iterator Adapters ===\n";
    const std::vector<int> countdown{1, 2, 3, 4, 5};
    printRange(out, "reverse_iterator", reversedCopy(countdown));
    const auto rit = countdown.rbegin() + 1; // refers to 4
    out << "reverse_iterator::base(): *rit = " << *rit << ", *(rit.base() - 1) = " << *(rit.base() - 1)
        << '\n';
    const std::array<int, 2> f{1, 2};
    const std::array<int, 2> m{10, 20};
    const std::array<int, 2> b{98, 99};
    printRange(out, "front_/inserter/back_inserter", buildWithInserters(f, m, b));
    printRange(out, "istream_iterator parse", parseInts("12 7 -3 40"));
    out << "ostream_iterator join: " << joinInts(countdown, ", ") << '\n';

    std::vector<std::unique_ptr<StarSystem>> owned;
    owned.push_back(std::make_unique<StarSystem>(StarSystem{"Vega", "A0V", 0, 25.0}));
    owned.push_back(std::make_unique<StarSystem>(StarSystem{"Altair", "A7V", 0, 16.7}));
    const auto moved = moveAll(owned);
    out << "move_iterator transferred " << moved.size() << " unique_ptrs; source now holds " << owned.size()
        << '\n';
}

void demonstrateCustomIterators(std::ostream& out) {
    out << "\n=== Custom Iterators ===\n";
    SimpleVector<int> fuel{40, 10, 30, 20};
    fuel.push_back(50);
    std::ranges::sort(fuel); // requires random access: contiguous iterator delivers
    printRange(out, "SimpleVector (contiguous_iterator) after ranges::sort", fuel);
    const std::span<const int> as_span(fuel.begin(), fuel.end()); // contiguous -> span
    out << "viewed as std::span of size " << as_span.size() << ", sum "
        << std::accumulate(as_span.begin(), as_span.end(), 0) << '\n';

    std::vector<int> readings{3, 8, 1, 12, 7, 6, 15, 4};
    FilterView evens(readings, [](int x) { return x % 2 == 0; });
    printRange(out, "FilterView (forward_iterator) evens", evens);
    out << "ranges::count_if(evens, > 5) = " << std::ranges::count_if(evens, [](int x) { return x > 5; })
        << ", ranges::max = " << std::ranges::max(evens) << '\n';
    printRange(out, "FilterView | views::transform(x*10)",
               evens | std::views::transform([](int x) { return x * 10; }));

    printRange(out, "FibonacciRange(12) (sentinel-terminated)", FibonacciRange(12));
    const auto big = std::ranges::find_if(FibonacciRange(FibonacciRange::max_count),
                                          [](std::uint64_t v) { return v > 1'000'000; });
    out << "first Fibonacci term above one million: fib(" << big.index() << ") = " << *big << '\n';
}

void demonstrateIteratorUtilities(std::ostream& out) {
    out << "\n=== Iterator Utilities ===\n";
    std::forward_list<int> chain{1, 2, 3, 4, 5, 6};
    auto fit = chain.begin();
    advanceBy(fit, 3);
    out << "advanceBy on forward_list (O(n)) -> " << *fit
        << ", distanceBetween = " << distanceBetween(chain.begin(), chain.end()) << '\n';
    const std::vector<int> fixed{10, 20, 30, 40};
    auto vit = fixed.begin();
    advanceBy(vit, 2);
    out << "advanceBy on vector (O(1)) -> " << *vit << ", std::next = " << *std::next(vit)
        << ", std::prev = " << *std::prev(vit) << '\n';

    std::list<int> stable{1, 2, 3, 4, 5};
    auto keep = std::next(stable.begin(), 4); // list iterators survive erasure of other nodes
    const auto removed_list = eraseWhileIterating(stable, [](int x) { return x % 2 == 0; });
    out << "erase-while-iterating removed " << removed_list << " from list; saved iterator still -> " << *keep
        << '\n';
    std::map<std::string, int> docks{{"A", 1}, {"B", 0}, {"C", 3}};
    const auto removed_map = eraseWhileIterating(docks, [](const auto& kv) { return kv.second == 0; });
    out << "erase-while-iterating removed " << removed_map << " empty dock(s), " << docks.size()
        << " remain\n";
    std::vector<int> grow{1, 2, 3};
    grow.reserve(grow.size() + 1);
    const auto first_ptr = grow.data();
    grow.push_back(4); // within reserved capacity: no reallocation, iterators remain valid
    out << std::boolalpha << "vector push_back within capacity kept storage: " << (first_ptr == grow.data())
        << '\n';
}

void runIteratorsDemo(std::ostream& out) {
    demonstrateIteratorCategories(out);
    demonstrateIteratorAdapters(out);
    demonstrateCustomIterators(out);
    demonstrateIteratorUtilities(out);
}

} // namespace CppVerseHub::STL
