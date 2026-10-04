#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <list>
#include <map>
#include <sstream>
#include <string>
#include <vector>

#include "modern/ConceptsAdvanced.hpp"

using namespace CppVerseHub::Modern::Concepts;
using Catch::Approx;

namespace {
struct NotPrintable {};
struct NoVelocity {
    double getX() const { return 0; }
    double getY() const { return 0; }
    double getZ() const { return 0; }
    void setPosition(double, double, double) {}
};
}  // namespace

TEST_CASE("Basic concepts accept and reject the expected types", "[modern][concepts]") {
    STATIC_CHECK(Numeric<int>);
    STATIC_CHECK(Numeric<const double>);
    STATIC_CHECK_FALSE(Numeric<bool>);
    STATIC_CHECK_FALSE(Numeric<char*>);
    STATIC_CHECK(Printable<std::string>);
    STATIC_CHECK_FALSE(Printable<NotPrintable>);
    STATIC_CHECK(Comparable<double>);
    STATIC_CHECK_FALSE(Comparable<NotPrintable>);
    STATIC_CHECK(Hashable<int>);
    STATIC_CHECK_FALSE(Hashable<NotPrintable>);
}

TEST_CASE("Container concepts distinguish container capabilities", "[modern][concepts]") {
    STATIC_CHECK(Container<std::vector<int>>);
    STATIC_CHECK(Container<std::map<int, int>>);
    STATIC_CHECK(RandomAccessContainer<std::vector<int>>);
    STATIC_CHECK_FALSE(RandomAccessContainer<std::list<int>>);
    STATIC_CHECK_FALSE(RandomAccessContainer<std::map<int, int>>);
    STATIC_CHECK(NumericContainer<std::list<float>>);
    STATIC_CHECK_FALSE(NumericContainer<std::vector<bool>>);
    STATIC_CHECK(Iterable<std::string>);
    STATIC_CHECK_FALSE(Iterable<int>);
}

TEST_CASE("Domain concepts model the space-game hierarchy", "[modern][concepts]") {
    STATIC_CHECK(Entity<DemoEntity>);
    STATIC_CHECK(Positionable<NoVelocity>);
    STATIC_CHECK_FALSE(Movable<NoVelocity>);
    STATIC_CHECK_FALSE(Entity<NoVelocity>);
    STATIC_CHECK(MovableSpaceEntity<DemoShip>);
    STATIC_CHECK_FALSE(MovableSpaceEntity<DemoEntity>);
    STATIC_CHECK(ResourceContainer<std::list<DemoResource>>);
    STATIC_CHECK_FALSE(ResourceContainer<std::vector<int>>);
}

TEST_CASE("Callable concepts check signatures", "[modern][concepts]") {
    auto twice = [](const int& x) { return 2 * x; };
    auto toString = [](const int& x) { return std::to_string(x); };
    auto plus = [](const double& a, const double& b) { return a + b; };
    STATIC_CHECK(UnaryOperation<decltype(twice), int>);
    STATIC_CHECK_FALSE(UnaryOperation<decltype(toString), int>);
    STATIC_CHECK(BinaryOperation<decltype(plus), double>);
    STATIC_CHECK(InvocableReturning<decltype(toString), std::string, int>);
}

TEST_CASE("Constrained functions compute correct results", "[modern][concepts]") {
    CHECK(add(2, 3) == 5);
    CHECK(add(0.5, 0.25) == Approx(0.75));
    CHECK(maxOf(std::string("apple"), std::string("pear")) == "pear");
    CHECK(maxOf(3, 3) == 3);
    CHECK(sumContainer(std::vector<int>{1, 2, 3, 4}) == 10);
    CHECK(sumContainer(std::list<double>{}) == 0.0);
    CHECK(foldLeft(std::vector<int>{1, 2, 3, 4}, 1, [](const int& a, const int& b) { return a * b; }) == 24);
}

TEST_CASE("transformInPlace mutates each element", "[modern][concepts]") {
    std::vector<int> v{1, 2, 3};
    transformInPlace(v, [](const int& x) { return x + 10; });
    CHECK(v == std::vector<int>{11, 12, 13});
    int arr[3] = {1, 2, 3};
    transformInPlace(arr, [](const int& x) { return -x; });
    CHECK(arr[2] == -3);
}

TEST_CASE("joinPrintable joins with separators", "[modern][concepts]") {
    CHECK(joinPrintable(std::vector<int>{1, 2, 3}) == "1, 2, 3");
    CHECK(joinPrintable(std::vector<std::string>{"a", "b"}, "|") == "a|b");
    CHECK(joinPrintable(std::vector<int>{}).empty());
}

TEST_CASE("Overload resolution picks the most constrained classify", "[modern][concepts]") {
    CHECK(classify(1) == "integral");
    CHECK(classify(1UL) == "integral");
    CHECK(classify(2.5f) == "numeric");
    CHECK(classify(true) == "generic");  // bool is excluded from Numeric
    CHECK(classify(std::vector<int>{}) == "random-access container");
    CHECK(classify(std::list<int>{}) == "container");
    CHECK(classify(DemoEntity{}) == "space entity");
    CHECK(classify(DemoShip{}) == "movable space entity");
    CHECK(classify(NotPrintable{}) == "generic");
}

TEST_CASE("profileOf reports satisfied concepts", "[modern][concepts]") {
    constexpr auto s = profileOf<std::string>();
    CHECK_FALSE(s.numeric);
    CHECK(s.printable);
    CHECK(s.comparable);
    CHECK(s.hashable);
    CHECK(s.container);
    CHECK(s.iterable);
    std::ostringstream os;
    printProfile(os, "string", s);
    CHECK(os.str().find("container=yes") != std::string::npos);
    CHECK(os.str().find("numeric=no") != std::string::npos);
}

TEST_CASE("ConceptFactory forwards constructor arguments", "[modern][concepts]") {
    auto byValue = ConceptFactory<DemoEntity>::create(42, "Station");
    CHECK(byValue.getId() == 42);
    CHECK(byValue.getName() == "Station");
    auto unique = ConceptFactory<std::string>::createUnique(3, 'x');
    CHECK(*unique == "xxx");
    auto shared = ConceptFactory<std::vector<int>>::createShared(4, 7);
    CHECK(shared->size() == 4);
    CHECK(shared.use_count() == 1);
}

TEST_CASE("RunningStats accumulates min, max and mean", "[modern][concepts]") {
    RunningStats<int> stats;
    CHECK(stats.count() == 0);
    CHECK(stats.mean() == 0.0);
    for (int v : {4, -2, 10, 8}) {
        stats.add(v);
    }
    CHECK(stats.count() == 4);
    CHECK(stats.min() == -2);
    CHECK(stats.max() == 10);
    CHECK(stats.mean() == Approx(5.0));
}

TEST_CASE("Domain helpers operate on constrained ranges", "[modern][concepts]") {
    std::vector<DemoShip> ships{{1, "A", 1.0, 2.0, 3.0}, {2, "B", -1.0, 0.0, 0.5}};
    advanceAll(ships, 2.0);
    CHECK(ships[0].getX() == Approx(2.0));
    CHECK(ships[0].getZ() == Approx(6.0));
    CHECK(ships[1].getX() == Approx(-2.0));
    std::vector<DemoResource> res{{"Ore", 5, false}, {"Gas", 7, true}};
    CHECK(totalResourceAmount(res) == 12);
}

TEST_CASE("demonstrateConcepts writes to the given stream", "[modern][concepts]") {
    std::ostringstream os;
    demonstrateConcepts(os);
    CHECK(os.str().find("movable space entity") != std::string::npos);
    CHECK(os.str().find("sumContainer") != std::string::npos);
}
