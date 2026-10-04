// Tests for templates/VariadicTemplates.hpp
#include "templates/VariadicTemplates.hpp"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <array>
#include <cmath>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <tuple>
#include <unordered_set>
#include <variant>
#include <vector>

namespace V = CppVerseHub::Templates::Variadic;

namespace {

// ----- folds -----
static_assert(V::count_args() == 0 && V::count_args(1, 'a', 2.0) == 3);
static_assert(V::sum(1, 2, 3) == 6);
static_assert(V::sum(1, 2.5) == 3.5);
static_assert(V::product() == 1 && V::product(2, 3, 7) == 42);
static_assert(V::all_true() && V::all_true(true, 1, 'x') && !V::all_true(true, 0));
static_assert(!V::any_true() && V::any_true(false, 0, 3) && !V::any_true(false, 0));
static_assert(V::min_recursive(4, 2, 8) == 2 && V::min_recursive(7) == 7);
static_assert(V::min_fold(3, -1, 2) == -1 && V::max_fold(3, -1, 2) == 3);
static_assert(V::min_fold(5, 2.5) == 2.5); // common type is double

// ----- pack introspection -----
static_assert(V::all_same_type_v<int, int, int> && !V::all_same_type_v<int, int, long>);
static_assert(V::all_convertible_v<double, int, float, char> && !V::all_convertible_v<int, std::string>);
static_assert(std::is_same_v<V::first_type_t<char, int, double>, char>);
static_assert(std::is_same_v<V::last_type_t<char, int, double>, double>);
static_assert(std::is_same_v<V::last_type_t<long>, long>);
static_assert(V::contains_type_v<int, char, int> && !V::contains_type_v<float, char, int>);
static_assert(V::type_index_v<int, char, int, int> == 1);
static_assert(V::type_index_v<float, char, int> == 2); // == sizeof...(Types) when absent
static_assert(V::count_type_v<int, int, char, int, int> == 3 && V::count_type_v<int> == 0);

// ----- RecursiveTuple in constant expressions -----
constexpr auto ct = V::make_recursive_tuple(1, 2.5, 'z');
static_assert(decltype(ct)::size() == 3);
static_assert(V::get<0>(ct) == 1 && V::get<1>(ct) == 2.5 && V::get<2>(ct) == 'z');
static_assert(V::RecursiveTuple<>::size() == 0);
static_assert(sizeof(V::RecursiveTuple<int>) == sizeof(int)); // empty base optimisation

// ----- utilities -----
static_assert(V::make_array<int>(1, 2.9, 'a') == std::array<int, 3>{1, 2, 97});
static_assert(V::compose([](int x) { return x + 1; }, [](int x) { return x * 3; })(2) == 7);
static_assert(std::is_same_v<decltype(V::transform_args([](auto v) { return v * 2; }, 1, 2.0)), std::tuple<int, double>>);
static_assert(V::filter_args<std::is_integral>(1, 2.0, 'c', 3L) == std::tuple{1, 'c', 3L});
static_assert(V::zip(std::tuple{1, 2, 3}, std::tuple{'a', 'b'}) ==
              std::tuple{std::tuple{1, 'a'}, std::tuple{2, 'b'}});
static_assert(V::optional_chain(std::optional<int>(4), [](int v) { return std::optional<int>(v * 2); }) ==
              std::optional<int>(8));

struct Point3 {
    int x;
    int y;
    int z;
    Point3(int a, int b, int c) : x(a), y(b), z(c) {}
};

} // namespace

TEST_CASE("print and print_recursive produce identical spacing", "[templates][variadic]") {
    std::ostringstream fold;
    std::ostringstream recursive;
    V::print(fold, 1, "two", 3.5);
    V::print_recursive(recursive, 1, "two", 3.5);
    CHECK(fold.str() == "1 two 3.5\n");
    CHECK(recursive.str() == fold.str());

    std::ostringstream empty;
    V::print(empty);
    CHECK(empty.str() == "\n");
}

TEST_CASE("join and format_string", "[templates][variadic]") {
    CHECK(V::join(", ", 1, 'a', std::string("b")) == "1, a, b");
    CHECK(V::join("-") == "");
    CHECK(V::format_string("{} + {} = {}", 1, 2, 3) == "1 + 2 = 3");
    CHECK(V::format_string("no placeholders", 1) == "no placeholders");
    CHECK(V::format_string("{} and {}", "only one") == "only one and {}");
    CHECK(V::format_string("{}{}", 'x', std::string("y")) == "xy");
}

TEST_CASE("min/max helpers on runtime values", "[templates][variadic]") {
    CHECK(V::min_recursive(std::string("pear"), std::string("apple")) == "apple");
    CHECK(V::max_fold(1.5, -2.0, 9.25, 3.0) == 9.25);
    CHECK(V::min_fold(10U, 3U, 7U) == 3U);
    CHECK(V::sum(std::string("a"), std::string("b")) == "ab");
}

TEST_CASE("RecursiveTuple stores heterogeneous runtime values", "[templates][variadic]") {
    auto tuple = V::make_recursive_tuple(std::string("name"), 42, std::vector<int>{1, 2});
    CHECK(V::get<0>(tuple) == "name");
    CHECK(V::get<1>(tuple) == 42);
    V::get<1>(tuple) = 7;
    CHECK(V::get<1>(tuple) == 7);
    V::get<2>(tuple).push_back(3);
    CHECK(V::get<2>(tuple).size() == 3);

    const auto copy = tuple; // copy constructor is not hijacked by the forwarding constructor
    CHECK(V::get<0>(copy) == "name");
    V::RecursiveTuple<int, double> defaulted;
    CHECK(V::get<0>(defaulted) == 0);
    CHECK(V::get<1>(defaulted) == 0.0);
}

TEST_CASE("overload builds visitor sets from lambdas", "[templates][variadic]") {
    const auto describe = V::overload{
        [](int i) { return "int:" + std::to_string(i); },
        [](const std::string& s) { return "string:" + s; },
        [](double d) { return "double:" + std::to_string(static_cast<int>(d)); },
    };
    std::vector<std::variant<int, std::string, double>> values{1, std::string("x"), 2.0};
    std::vector<std::string> out;
    for (const auto& v : values) {
        out.push_back(std::visit(describe, v));
    }
    CHECK(out == std::vector<std::string>{"int:1", "string:x", "double:2"});

    const std::variant<int, std::string> var = 5;
    CHECK(V::visit_variant(var, [](int i) { return i * 2; }, [](const std::string&) { return -1; }) == 10);
}

TEST_CASE("multifunction dispatches to the first viable callable", "[templates][variadic]") {
    const V::multifunction dispatch(
        [](const std::string& s) { return "string " + s; }, [](double) { return std::string("double"); },
        [](int) { return std::string("int"); }, [](auto&&) { return std::string("other"); });
    CHECK(dispatch(std::string("s")) == "string s");
    CHECK(dispatch(2.5) == "double");
    CHECK(dispatch(3) == "double"); // first match, not best match: int converts to double before int is tried
    CHECK(dispatch(std::vector<int>{}) == "other");
}

TEST_CASE("hash_combine is order sensitive and deterministic", "[templates][variadic]") {
    const V::hash_combine<int, std::string> hasher;
    const auto h1 = hasher(1, "a");
    CHECK(h1 == hasher(1, "a"));
    CHECK(h1 != hasher(2, "a"));
    std::unordered_set<std::size_t> distinct;
    for (int i = 0; i < 100; ++i) {
        distinct.insert(hasher(i, "k"));
    }
    CHECK(distinct.size() == 100);
    CHECK(V::hash_combine<int, int>{}(1, 2) != V::hash_combine<int, int>{}(2, 1));
}

TEST_CASE("Factory replays stored arguments", "[templates][variadic]") {
    const V::Factory<std::string, std::size_t> repeat(std::size_t{3});
    CHECK(repeat.create('a') == "aaa");
    CHECK(repeat.create('b') == "bbb");

    const V::Factory<Point3, int, int> plane(1, 2);
    const Point3 p = plane.create(3);
    CHECK(p.x == 1);
    CHECK(p.y == 2);
    CHECK(p.z == 3);
}

TEST_CASE("Builder accumulates a typed argument pack", "[templates][variadic]") {
    auto builder = V::make_builder<Point3>().with(4).with(5);
    static_assert(decltype(builder)::field_count() == 2);
    const Point3 p = std::move(builder).with(6).build();
    CHECK(p.x == 4);
    CHECK(p.y == 5);
    CHECK(p.z == 6);

    const auto vec = V::make_builder<std::vector<int>>().with(std::size_t{4}).with(9).build();
    CHECK(vec == std::vector<int>{9, 9, 9, 9});
}

TEST_CASE("for_each_arg and transform_args visit every argument in order", "[templates][variadic]") {
    std::vector<std::string> seen;
    V::for_each_arg(
        [&seen](const auto& v) {
            std::ostringstream oss;
            oss << v;
            seen.push_back(oss.str());
        },
        1, "b", 2.5);
    CHECK(seen == std::vector<std::string>{"1", "b", "2.5"});

    const auto lengths = V::transform_args([](const std::string& s) { return s.size(); }, std::string("a"),
                                           std::string("abc"));
    CHECK(lengths == std::make_tuple(std::size_t{1}, std::size_t{3}));

    const auto strings = V::filter_args<std::is_class>(1, std::string("kept"), 2.0);
    static_assert(std::tuple_size_v<std::remove_cvref_t<decltype(strings)>> == 1);
    CHECK(std::get<0>(strings) == "kept");
}

TEST_CASE("zip truncates to the shortest tuple", "[templates][variadic]") {
    const auto zipped = V::zip(std::make_tuple(1, 2, 3), std::make_tuple(std::string("a"), std::string("b")),
                               std::make_tuple('x', 'y', 'z', 'w'));
    static_assert(std::tuple_size_v<std::remove_cvref_t<decltype(zipped)>> == 2);
    CHECK(std::get<1>(zipped) == std::make_tuple(2, std::string("b"), 'y'));
}

TEST_CASE("compose and Pipeline chain transformations", "[templates][variadic]") {
    const auto f = V::compose([](double x) { return std::sqrt(x); }, [](double x) { return x * x + 16.0; },
                              [](int x) { return static_cast<double>(x); });
    CHECK(f(3) == Catch::Approx(5.0));

    const auto result = V::make_pipeline(std::string("  value  "))
                            .then([](std::string s) { return s.substr(2, 5); })
                            .then([](const std::string& s) { return s.size(); })
                            .then([](std::size_t n) { return n * 10; })
                            .get();
    CHECK(result == 50);
}

TEST_CASE("make_vector deduces the common type", "[templates][variadic]") {
    const auto mixed = V::make_vector(1, 2.5, 3.0F);
    static_assert(std::is_same_v<std::remove_cvref_t<decltype(mixed)>, std::vector<double>>);
    CHECK(mixed == std::vector<double>{1.0, 2.5, 3.0});
    const auto words = V::make_vector(std::string("a"), "b");
    CHECK(words == std::vector<std::string>{"a", "b"});
}

TEST_CASE("optional_chain short-circuits on the first empty result", "[templates][variadic]") {
    int calls = 0;
    const auto parse = [&calls](const std::string& s) -> std::optional<int> {
        ++calls;
        if (s.empty() || s.find_first_not_of("0123456789") != std::string::npos) {
            return std::nullopt;
        }
        return std::stoi(s);
    };
    const auto reciprocal = [&calls](int v) -> std::optional<double> {
        ++calls;
        return v == 0 ? std::nullopt : std::optional<double>(1.0 / v);
    };

    CHECK(V::optional_chain(std::optional<std::string>("4"), parse, reciprocal) == std::optional<double>(0.25));
    CHECK(calls == 2);
    CHECK_FALSE(V::optional_chain(std::optional<std::string>("x"), parse, reciprocal));
    CHECK(calls == 3); // reciprocal skipped
    CHECK_FALSE(V::optional_chain(std::optional<std::string>{}, parse, reciprocal));
    CHECK(calls == 3); // nothing called
}

TEST_CASE("safe_call converts exceptions into empty results", "[templates][variadic]") {
    const auto ok = V::safe_call([](int a, int b) { return a + b; }, 2, 3);
    static_assert(std::is_same_v<std::remove_cvref_t<decltype(ok)>, std::optional<int>>);
    CHECK(ok == 5);
    const auto failed = V::safe_call([]() -> int { throw std::runtime_error("boom"); });
    CHECK_FALSE(failed);

    int side_effect = 0;
    CHECK(V::safe_call([&side_effect] { side_effect = 1; }));
    CHECK(side_effect == 1);
    CHECK_FALSE(V::safe_call([] { throw std::logic_error("void boom"); }));
}

TEST_CASE("perfect_forwarder preserves value categories", "[templates][variadic]") {
    auto category = V::make_perfect_forwarder(V::overload{
        [](int&) { return std::string("lvalue"); },
        [](int&&) { return std::string("rvalue"); },
        [](const int&) { return std::string("const lvalue"); },
    });
    int x = 1;
    const int cx = 2;
    CHECK(category(x) == "lvalue");
    CHECK(category(3) == "rvalue");
    CHECK(category(cx) == "const lvalue");
}

TEST_CASE("memoize caches results keyed by the argument pack", "[templates][variadic]") {
    int evaluations = 0;
    auto slow_add = V::memoize<long(int, int)>([&evaluations](int a, int b) {
        ++evaluations;
        return static_cast<long>(a) + b;
    });
    CHECK(slow_add(1, 2) == 3);
    CHECK(slow_add(1, 2) == 3);
    CHECK(slow_add(2, 1) == 3);
    CHECK(evaluations == 2);
    CHECK(slow_add.hits() == 1);
    CHECK(slow_add.cache_size() == 2);

    auto lengths = V::memoize<std::size_t(const std::string&)>([](const std::string& s) { return s.size(); });
    CHECK(lengths("abc") == 3);
    CHECK(lengths("abc") == 3);
    CHECK(lengths.hits() == 1);
}
