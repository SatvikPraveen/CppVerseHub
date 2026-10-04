// Tests for stl_showcase/STLUtilities.hpp
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

#include <array>
#include <cmath>
#include <limits>
#include <string>
#include <tuple>
#include <variant>
#include <vector>

#include "stl_showcase/STLUtilities.hpp"

using namespace CppVerseHub::STL;
using Catch::Approx;

TEST_CASE("NavigationCoordinate distance and status names", "[utilities][data]") {
    const NavigationCoordinate a{1, 2, 3};
    const NavigationCoordinate b{4, 6, 3};
    REQUIRE(a.distanceTo(b) == Approx(5.0));
    REQUIRE(a.distanceTo(a) == 0.0);
    STATIC_REQUIRE(toString(VesselStatus::InTransit) == "InTransit");
    STATIC_REQUIRE(toString(VesselStatus::Maintenance) == "Maintenance");
    REQUIRE(toString(static_cast<VesselStatus>(42)) == "Unknown");
}

TEST_CASE("closestPairIndices and distanceRangeFromOrigin return pairs", "[utilities][pair]") {
    const std::vector<NavigationCoordinate> points{{0, 0, 0}, {10, 0, 0}, {10, 1, 0}, {-5, -5, -5}};
    const auto closest = closestPairIndices(points);
    REQUIRE(closest.has_value());
    REQUIRE(*closest == std::pair<std::size_t, std::size_t>{1, 2});
    REQUIRE_FALSE(closestPairIndices(std::vector<NavigationCoordinate>{{1, 1, 1}}).has_value());

    const auto range = distanceRangeFromOrigin(points);
    REQUIRE(range.has_value());
    const auto [nearest, farthest] = *range;
    REQUIRE(nearest == 0.0);
    REQUIRE(farthest == Approx(std::sqrt(101.0)));
    REQUIRE_FALSE(distanceRangeFromOrigin({}).has_value());
}

TEST_CASE("coordinateStatistics returns a tuple", "[utilities][tuple]") {
    const std::vector<NavigationCoordinate> cloud{{0, 0, 0}, {2, 0, 0}, {1, 3, 0}};
    const auto stats = coordinateStatistics(cloud);
    REQUIRE(stats.has_value());
    const auto& [centroid, spread, count] = *stats;
    REQUIRE(centroid.x == Approx(1.0));
    REQUIRE(centroid.y == Approx(1.0));
    REQUIRE(centroid.z == Approx(0.0));
    REQUIRE(spread == Approx(2.0));
    REQUIRE(count == 3);
    REQUIRE(std::get<std::size_t>(*stats) == 3);
    REQUIRE_FALSE(coordinateStatistics({}).has_value());
}

TEST_CASE("formatTuple, forEachElement and transformTuple", "[utilities][tuple]") {
    REQUIRE(formatTuple(std::make_tuple(1, std::string("two"), 3.5)) == "(1, two, 3.5)");
    REQUIRE(formatTuple(std::tuple<>{}) == "()");
    REQUIRE(formatTuple(std::make_tuple('x')) == "(x)");

    double sum = 0.0;
    forEachElement(std::make_tuple(1, 2.5, 3L), [&sum](auto v) { sum += static_cast<double>(v); });
    REQUIRE(sum == Approx(6.5));

    auto mutable_tuple = std::make_tuple(1, 2);
    forEachElement(mutable_tuple, [](int& v) { v *= 10; });
    REQUIRE(mutable_tuple == std::make_tuple(10, 20));

    constexpr auto doubled = transformTuple(std::make_tuple(1, 2.5), [](auto v) { return v * 2; });
    STATIC_REQUIRE(std::get<0>(doubled) == 2);
    STATIC_REQUIRE(std::get<1>(doubled) == 5.0);
    const auto sizes = transformTuple(std::make_tuple(std::string("abc"), std::string("")),
                                      [](const std::string& s) { return s.size(); });
    REQUIRE(sizes == std::make_tuple(std::size_t{3}, std::size_t{0}));
}

TEST_CASE("sortVesselRecords orders by status, priority desc, name", "[utilities][tuple]") {
    std::vector<VesselRecord> records{{"C", VesselStatus::InTransit, 2},
                                      {"A", VesselStatus::Combat, 9},
                                      {"B", VesselStatus::InTransit, 7},
                                      {"D", VesselStatus::Docked, 5},
                                      {"A", VesselStatus::InTransit, 7}};
    sortVesselRecords(records);
    std::vector<std::string> names;
    for (const auto& r : records) {
        names.push_back(r.name);
    }
    REQUIRE(names == std::vector<std::string>{"D", "A", "B", "C", "A"});
    REQUIRE(records[4].status == VesselStatus::Combat);
}

TEST_CASE("parseInt accepts only complete integers", "[utilities][optional]") {
    REQUIRE(parseInt("42") == 42);
    REQUIRE(parseInt("-17") == -17);
    REQUIRE(parseInt("+8") == 8);
    REQUIRE(parseInt("2147483647") == std::numeric_limits<int>::max());
    const auto bad = GENERATE(as<std::string>{}, "", "abc", "12abc", " 12", "2147483648", "+", "+-3", "1.5");
    CAPTURE(bad);
    REQUIRE_FALSE(parseInt(bad).has_value());
}

TEST_CASE("safeDivide, andThen and transformOptional", "[utilities][optional]") {
    STATIC_REQUIRE(safeDivide(9.0, 3.0) == 3.0);
    STATIC_REQUIRE_FALSE(safeDivide(1.0, 0.0).has_value());

    const auto chain = andThen(parseInt("10"), [](int v) { return safeDivide(100.0, v); });
    REQUIRE(chain == 10.0);
    const auto zero = andThen(parseInt("0"), [](int v) { return safeDivide(100.0, v); });
    REQUIRE_FALSE(zero.has_value());
    const auto unparsed = andThen(parseInt("x"), [](int v) { return safeDivide(100.0, v); });
    REQUIRE_FALSE(unparsed.has_value());

    const auto text = transformOptional(std::optional<int>{4}, [](int v) { return std::string(v, '*'); });
    REQUIRE(text == "****");
    REQUIRE_FALSE(transformOptional(std::optional<int>{}, [](int v) { return v + 1; }).has_value());
    constexpr auto squared = transformOptional(std::optional<int>{5}, [](int v) { return v * v; });
    STATIC_REQUIRE(*squared == 25);
}

TEST_CASE("findVesselStatus uses heterogeneous lookup", "[utilities][optional]") {
    const std::map<std::string, VesselStatus, std::less<>> registry{{"Roci", VesselStatus::Combat}};
    REQUIRE(findVesselStatus(registry, "Roci") == VesselStatus::Combat);
    REQUIRE_FALSE(findVesselStatus(registry, "Ghost").has_value());
}

TEST_CASE("Command variant visitation", "[utilities][variant]") {
    const Command move = MoveCommand{{1, 2, 3}};
    const Command attack = AttackCommand{"Drone", 4};
    const Command scan = ScanCommand{2.5};
    const Command dock = DockCommand{"Ceres"};
    REQUIRE(describeCommand(move) == "Move to (1, 2, 3)");
    REQUIRE(describeCommand(attack) == "Attack Drone at intensity 4");
    REQUIRE(describeCommand(scan) == "Scan radius 2.5");
    REQUIRE(describeCommand(dock) == "Dock at Ceres");
    REQUIRE(statusAfter(move) == VesselStatus::InTransit);
    REQUIRE(statusAfter(attack) == VesselStatus::Combat);
    REQUIRE(statusAfter(scan) == VesselStatus::Exploring);
    REQUIRE(statusAfter(dock) == VesselStatus::Docked);
    REQUIRE(attack.index() == 1);
    REQUIRE(std::holds_alternative<ScanCommand>(scan));
    REQUIRE(std::get_if<DockCommand>(&move) == nullptr);
    REQUIRE_THROWS_AS(std::get<DockCommand>(move), std::bad_variant_access);
}

TEST_CASE("Overloaded combines lambdas for std::visit", "[utilities][variant]") {
    const std::variant<int, std::string, double> values[] = {7, std::string("seven"), 7.5};
    std::vector<std::string> kinds;
    for (const auto& v : values) {
        kinds.push_back(std::visit(Overloaded{[](int) { return std::string("int"); },
                                              [](const std::string&) { return std::string("string"); },
                                              [](double) { return std::string("double"); }},
                                   v));
    }
    REQUIRE(kinds == std::vector<std::string>{"int", "string", "double"});
}

TEST_CASE("parseCommand produces commands", "[utilities][variant]") {
    const auto move = parseCommand("move 1 -2.5 3e2");
    REQUIRE(std::holds_alternative<Command>(move));
    REQUIRE(std::get<Command>(move) == Command{MoveCommand{{1.0, -2.5, 300.0}}});
    REQUIRE(std::get<Command>(parseCommand("  attack  Pirate 10 ")) == Command{AttackCommand{"Pirate", 10}});
    REQUIRE(std::get<Command>(parseCommand("scan 12.5")) == Command{ScanCommand{12.5}});
    REQUIRE(std::get<Command>(parseCommand("dock Tycho")) == Command{DockCommand{"Tycho"}});
}

TEST_CASE("parseCommand reports errors as values", "[utilities][variant]") {
    struct Case {
        const char* text;
        std::size_t token;
    };
    const auto c = GENERATE(Case{"", 0}, Case{"warp 9", 0}, Case{"move 1 2", 3}, Case{"move 1 x 3", 2},
                            Case{"attack Drone 0", 2}, Case{"attack Drone 11", 2}, Case{"attack Drone", 2},
                            Case{"scan -1", 1}, Case{"scan nan", 1}, Case{"dock", 1}, Case{"dock A B", 2});
    CAPTURE(c.text);
    const auto result = parseCommand(c.text);
    REQUIRE(std::holds_alternative<ParseError>(result));
    const auto& error = std::get<ParseError>(result);
    REQUIRE_FALSE(error.message.empty());
    REQUIRE(error.token == c.token);
}

TEST_CASE("PropertyBag stores heterogeneous values with exact-type retrieval", "[utilities][any]") {
    PropertyBag bag;
    bag.set("name", std::string("Roci"));
    bag.set("crew", 4);
    bag.set("warp", 9.5);
    bag.set("tags", std::vector<std::string>{"corvette"});
    REQUIRE(bag.size() == 4);
    REQUIRE(bag.get<std::string>("name") == "Roci");
    REQUIRE(bag.get<int>("crew") == 4);
    REQUIRE_FALSE(bag.get<long>("crew").has_value());  // exact type required
    REQUIRE_FALSE(bag.get<int>("missing").has_value());
    REQUIRE(bag.get<std::vector<std::string>>("tags")->front() == "corvette");
    REQUIRE(bag.holds<double>("warp"));
    REQUIRE_FALSE(bag.holds<float>("warp"));
    REQUIRE_FALSE(bag.holds<int>("missing"));

    bag.set("crew", std::string("four"));  // replace with a different type
    REQUIRE(bag.size() == 4);
    REQUIRE(bag.holds<std::string>("crew"));
    REQUIRE(bag.keys() == std::vector<std::string>{"crew", "name", "tags", "warp"});
    REQUIRE(bag.erase("crew"));
    REQUIRE_FALSE(bag.erase("crew"));
    REQUIRE_FALSE(bag.contains("crew"));
    REQUIRE(bag.contains("name"));
}

TEST_CASE("splitView keeps empty fields and aliases the input", "[utilities][string_view]") {
    const std::string text = "a,,bc,";
    const auto fields = splitView(text, ',');
    REQUIRE(fields == std::vector<std::string_view>{"a", "", "bc", ""});
    REQUIRE(fields[2].data() == text.data() + 3);
    REQUIRE(splitView("", ',') == std::vector<std::string_view>{""});
    REQUIRE(splitView("solo", ',') == std::vector<std::string_view>{"solo"});
}

TEST_CASE("mean over std::span", "[utilities][span]") {
    const std::array<double, 4> values{1.0, 2.0, 3.0, 6.0};
    REQUIRE(mean(values) == Approx(3.0));
    REQUIRE(mean(std::span(values).last(2)) == Approx(4.5));
    REQUIRE_FALSE(mean({}).has_value());
}
