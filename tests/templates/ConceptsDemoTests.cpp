// Tests for templates/ConceptsDemo.hpp
#include "templates/ConceptsDemo.hpp"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

#include <array>
#include <cstdint>
#include <list>
#include <map>
#include <memory>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

using namespace CppVerseHub::Templates::Concepts;

namespace {

struct Opaque {};

struct Point {
    int x = 0;
    int y = 0;
};

std::ostream& operator<<(std::ostream& os, const Point& p) {
    return os << '(' << p.x << ',' << p.y << ')';
}

// ----- compile-time concept checks -----
static_assert(Arithmetic<int> && Arithmetic<double> && !Arithmetic<std::string>);
static_assert(Integral<char> && !Integral<float>);
static_assert(SignedIntegral<long> && !SignedIntegral<unsigned>);
static_assert(Unsigned<std::uint8_t> && !Unsigned<int>);
static_assert(FloatingPoint<float> && !FloatingPoint<int>);
static_assert(Signed<int> && Signed<double> && !Signed<unsigned>);

static_assert(HasSize<std::vector<int>> && HasSize<std::string> && !HasSize<int>);
static_assert(Iterable<std::list<int>> && !Iterable<int>);
static_assert(Container<std::vector<int>> && Container<std::map<int, int>> && !Container<int[3]>);
static_assert(SequenceContainer<std::vector<int>> && SequenceContainer<std::list<int>>);
static_assert(!SequenceContainer<std::set<int>> && !SequenceContainer<std::array<int, 3>>);
static_assert(AssociativeContainer<std::map<int, std::string>> && !AssociativeContainer<std::set<int>>);
static_assert(Printable<int> && Printable<std::string> && Printable<Point> && !Printable<Opaque>);
static_assert(Comparable<int> && Comparable<std::string> && !Comparable<Opaque>);
static_assert(Hashable<std::string> && !Hashable<Opaque>);
static_assert(Copyable<std::string> && !Copyable<std::unique_ptr<int>>);
static_assert(Movable<std::unique_ptr<int>>);
static_assert(SmartPointer<std::unique_ptr<int>> && SmartPointer<std::shared_ptr<int>> &&
              !SmartPointer<int*>);
static_assert(Predicate<bool (*)(int), int> && !Predicate<void (*)(int), int>);
static_assert(UnaryPredicate<decltype([](int v) { return v > 0; }), int>);
static_assert(BinaryPredicate<std::less<int>, int>);
static_assert(Range<std::vector<int>> && RandomAccessRange<std::vector<int>> &&
              !RandomAccessRange<std::list<int>>);
static_assert(Numeric<int> && Numeric<double> && !Numeric<std::string>);
static_assert(Additive<std::string> && !Multiplicative<std::string>);
static_assert(Ring<int> && Ring<double> && !Ring<unsigned char*>);
static_assert(Field<double> && Field<int>); // concepts are syntactic: int "is" a field
static_assert(Serializable<int> && !Serializable<Opaque>);

// classify / power are constexpr
static_assert(classify(1) == "signed integral");
static_assert(classify(1U) == "integral");
static_assert(classify(1.0F) == "floating point");
static_assert(classify(Opaque{}) == "non-arithmetic");
static_assert(power(3, 4U) == 81);
static_assert(power(2.0, 0U) == 1.0);
static_assert(sum_range(std::array{1, 2, 3, 4}) == 10);
static_assert(product_range(std::array{1, 2, 3, 4}) == 24);

// Constrained members appear only when the constraint holds.
template <typename T>
concept HasFront = requires(T t) { t.front(); };
template <typename T>
concept HasContainsKey = requires(const T t) { t.contains_key(1); };
static_assert(HasFront<ContainerAdapter<std::vector<int>>>);
static_assert(!HasFront<ContainerAdapter<std::set<int>>>);
static_assert(HasContainsKey<ContainerAdapter<std::map<int, int>>>);
static_assert(!HasContainsKey<ContainerAdapter<std::vector<int>>>);

template <typename T>
concept HasMagnitude = requires(const T t) { t.magnitude(); };
static_assert(HasMagnitude<MathVector<double>> && !HasMagnitude<MathVector<int>>);

} // namespace

TEST_CASE("classify picks the most constrained overload via subsumption", "[templates][concepts]") {
    CHECK(classify(static_cast<short>(1)) == "signed integral");
    CHECK(classify(std::uint64_t{1}) == "integral");
    CHECK(classify(2.5) == "floating point");
    CHECK(classify(std::string("s")) == "non-arithmetic");
    CHECK(classify(Point{}) == "non-arithmetic");
}

TEST_CASE("power computes integer and floating powers by squaring", "[templates][concepts]") {
    const auto exponent = GENERATE(0U, 1U, 2U, 5U, 10U, 13U);
    long expected = 1;
    for (unsigned i = 0; i < exponent; ++i) {
        expected *= 3;
    }
    CHECK(power(3L, exponent) == expected);
    CHECK(power(1.5, 2U) == Catch::Approx(2.25));
    CHECK(power(-2, 3U) == -8);
}

TEST_CASE("format_value formats printable values and containers", "[templates][concepts]") {
    CHECK(format_value(42) == "42");
    CHECK(format_value(std::string("text")) == "text"); // string is Printable, not formatted as a container
    CHECK(format_value(std::vector<int>{1, 2, 3}) == "[1, 2, 3]");
    CHECK(format_value(std::vector<int>{}) == "[]");
    CHECK(format_value(std::list<Point>{{1, 2}, {3, 4}}) == "[(1,2), (3,4)]");

    std::ostringstream oss;
    print(oss, std::vector<double>{0.5});
    print(oss, 7);
    CHECK(oss.str() == "[0.5]\n7\n");
}

TEST_CASE("sum_range and product_range fold ranges", "[templates][concepts]") {
    const std::vector<int> values{1, 2, 3, 4, 5};
    CHECK(sum_range(values) == 15);
    CHECK(product_range(values) == 120);
    CHECK(sum_range(std::vector<int>{}) == 0);
    CHECK(product_range(std::vector<int>{}) == 1);
    CHECK(sum_range(std::vector<std::string>{"a", "b", "c"}) == "abc");
    CHECK(sum_range(std::list<double>{0.5, 0.25}) == Catch::Approx(0.75));
}

TEST_CASE("sort_range, find_in_range and find_if_in_range", "[templates][concepts]") {
    std::vector<int> values{4, 1, 3, 2};
    sort_range(values);
    CHECK(values == std::vector<int>{1, 2, 3, 4});
    sort_range(values, std::greater<>{});
    CHECK(values == std::vector<int>{4, 3, 2, 1});

    const auto it = find_in_range(values, 3);
    REQUIRE(it != values.end());
    CHECK(std::distance(values.begin(), it) == 1);
    CHECK(find_in_range(values, 99) == values.end());

    const auto even = find_if_in_range(values, [](int v) { return v % 2 == 0; });
    REQUIRE(even != values.end());
    CHECK(*even == 4);
}

TEST_CASE("copy_range and transform_range write through output iterators", "[templates][concepts]") {
    const std::vector<int> source{1, 2, 3};
    std::vector<int> copied;
    copy_range(source, std::back_inserter(copied));
    CHECK(copied == source);

    std::vector<std::string> transformed;
    transform_range(source, std::back_inserter(transformed), [](int v) { return std::string(v, '*'); });
    CHECK(transformed == std::vector<std::string>{"*", "**", "***"});
}

TEST_CASE("constrained algorithm wrappers", "[templates][concepts]") {
    std::vector<int> values{1, 1, 2, 3, 3, 3, 4};
    CHECK(algorithms::all_of(values, [](int v) { return v > 0; }));
    CHECK(algorithms::any_of(values, [](int v) { return v == 4; }));
    CHECK(algorithms::none_of(values, [](int v) { return v > 10; }));
    CHECK(algorithms::count_if(values, [](int v) { return v == 3; }) == 3);

    values.erase(algorithms::unique(values), values.end());
    CHECK(values == std::vector<int>{1, 2, 3, 4});

    algorithms::sort(values, [](int a, int b) { return a > b; });
    CHECK(values == std::vector<int>{4, 3, 2, 1});
    algorithms::sort(values);
    CHECK(values == std::vector<int>{1, 2, 3, 4});
}

TEST_CASE("ContainerAdapter exposes sequence and associative operations conditionally",
          "[templates][concepts]") {
    ContainerAdapter<std::vector<int>> seq;
    CHECK(seq.empty());
    seq.push_back(1);
    seq.push_back(2);
    CHECK(seq.size() == 2);
    CHECK(seq.front() == 1);
    CHECK(seq.back() == 2);
    seq.front() = 10;
    CHECK(seq.underlying() == std::vector<int>{10, 2});
    int total = 0;
    for (int v : seq) {
        total += v;
    }
    CHECK(total == 12);

    const ContainerAdapter<std::map<int, std::string>> assoc(std::map<int, std::string>{{1, "one"}});
    CHECK(assoc.contains_key(1));
    CHECK_FALSE(assoc.contains_key(2));
    CHECK(assoc.size() == 1);
}

TEST_CASE("MathVector arithmetic, dot product and normalisation", "[templates][concepts]") {
    MathVector<double> a{1.0, 2.0, 2.0};
    const MathVector<double> b{2.0, 0.0, 1.0};

    CHECK((a + b) == MathVector<double>{3.0, 2.0, 3.0});
    CHECK((a - b) == MathVector<double>{-1.0, 2.0, 1.0});
    CHECK((a * 2.0) == MathVector<double>{2.0, 4.0, 4.0});
    CHECK((a / 2.0) == MathVector<double>{0.5, 1.0, 1.0});
    CHECK(a.dot(b) == Catch::Approx(4.0));
    CHECK(a.magnitude() == Catch::Approx(3.0));

    const auto unit = a.normalized();
    CHECK(unit.magnitude() == Catch::Approx(1.0));
    CHECK(unit[0] == Catch::Approx(1.0 / 3.0));

    SECTION("size mismatches throw") {
        const MathVector<double> c{1.0};
        CHECK_THROWS_AS(a + c, std::invalid_argument);
        CHECK_THROWS_AS(a.dot(c), std::invalid_argument);
    }
    SECTION("zero vector cannot be normalised") {
        CHECK_THROWS_AS(MathVector<double>(3).normalized(), std::invalid_argument);
    }
    SECTION("bounds-checked access") {
        CHECK_THROWS_AS(a.at(3), std::out_of_range);
        a.at(0) = 5.0;
        CHECK(a[0] == 5.0);
    }
    SECTION("integer vectors work without magnitude") {
        MathVector<int> v(2, 3);
        v *= 2;
        CHECK(v.dot(v) == 72);
    }
}

TEST_CASE("SmartPtrWrapper works with unique_ptr and shared_ptr", "[templates][concepts]") {
    SmartPtrWrapper<std::unique_ptr<std::string>> unique(std::make_unique<std::string>("abc"));
    REQUIRE(unique);
    CHECK(*unique == "abc");
    CHECK(unique->size() == 3);
    unique.reset();
    CHECK_FALSE(unique);
    CHECK(unique.get() == nullptr);

    auto shared = std::make_shared<int>(5);
    const SmartPtrWrapper<std::shared_ptr<int>> wrapped(shared);
    CHECK(*wrapped == 5);
    CHECK(wrapped.get_pointer().use_count() == 2);
}
