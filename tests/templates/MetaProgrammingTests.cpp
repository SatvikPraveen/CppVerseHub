// Tests for templates/MetaProgramming.hpp — mostly static_assert, plus runtime checks.
#include "templates/MetaProgramming.hpp"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <array>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <vector>

namespace Meta = CppVerseHub::Templates::Meta;

namespace {

// ----- type traits and type lists -----
static_assert(Meta::is_instantiation_of_v<std::vector, std::vector<int>>);
static_assert(Meta::is_instantiation_of_v<std::map, const std::map<int, int>>);
static_assert(!Meta::is_instantiation_of_v<std::vector, std::map<int, int>>);
static_assert(!Meta::is_instantiation_of_v<std::vector, int>);
static_assert(Meta::is_any_of_v<int, char, int, long> && !Meta::is_any_of_v<float, int, long>);

using list = Meta::type_list<int, double, char>;
static_assert(list::size == 3);
static_assert(std::is_same_v<Meta::type_at_t<1, int, double, char>, double>);
static_assert(std::is_same_v<Meta::list_at_t<2, list>, char>);
static_assert(Meta::type_index_v<char, int, double, char> == 2);
static_assert(Meta::type_index_v<int, int, int> == 0);
static_assert(Meta::type_index_v<float, int, double> == Meta::npos);
static_assert(Meta::type_index_v<float> == Meta::npos);
static_assert(Meta::type_exists_v<double, int, double> && !Meta::type_exists_v<void, int>);
static_assert(std::is_same_v<Meta::push_front_t<bool, list>, Meta::type_list<bool, int, double, char>>);
static_assert(std::is_same_v<Meta::push_back_t<bool, list>, Meta::type_list<int, double, char, bool>>);
static_assert(std::is_same_v<Meta::transform_t<list, std::add_pointer>, Meta::type_list<int*, double*, char*>>);
static_assert(std::is_same_v<Meta::concat_t<list, Meta::type_list<long>>, Meta::type_list<int, double, char, long>>);
static_assert(std::is_same_v<Meta::filter_t<list, std::is_integral>, Meta::type_list<int, char>>);
static_assert(std::is_same_v<Meta::filter_t<Meta::type_list<>, std::is_integral>, Meta::type_list<>>);

// ----- compile-time arithmetic -----
static_assert(Meta::factorial_v<0> == 1 && Meta::factorial_v<5> == 120 && Meta::factorial_v<20> == 2432902008176640000ULL);
static_assert(Meta::factorial_func(10) == Meta::factorial_v<10>);
static_assert(Meta::fibonacci_v<0> == 0 && Meta::fibonacci_v<1> == 1 && Meta::fibonacci_v<50> == 12586269025ULL);
static_assert(Meta::fibonacci_func(50) == Meta::fibonacci_v<50>);
static_assert(Meta::fibonacci_func(0) == 0 && Meta::fibonacci_func(2) == 1);
static_assert(Meta::power_v<3, 4> == 81 && Meta::power_v<7, 0> == 1);
static_assert(Meta::power_func(2, 63) == (std::size_t{1} << 63U));
static_assert(Meta::power_func(10, 0) == 1);
static_assert(Meta::gcd_v<48, 18> == 6 && Meta::gcd_v<17, 5> == 1 && Meta::gcd_v<0, 9> == 9);
static_assert(Meta::is_prime(2) && Meta::is_prime(97) && !Meta::is_prime(1) && !Meta::is_prime(91));
static_assert(Meta::prime_count(100) == 25);
static_assert(Meta::generate_primes<20>() == std::array<std::size_t, 8>{2, 3, 5, 7, 11, 13, 17, 19});
static_assert(Meta::generate_primes<1>().empty());

// ----- strings -----
constexpr Meta::compile_time_string foo("foo");
constexpr Meta::compile_time_string bar("bar");
static_assert(foo.size() == 3 && foo[0] == 'f');
static_assert((foo + bar).view() == "foobar");
static_assert((foo + bar) == Meta::compile_time_string("foobar"));
static_assert(!(foo == bar));
static_assert(Meta::named_tag<"probe">::name == "probe");
static_assert(!std::is_same_v<Meta::named_tag<"a">, Meta::named_tag<"b">>);
static_assert(Meta::hash_string("") == 14695981039346656037ULL);
static_assert(Meta::hash_string("a") == 0xaf63dc4c8601ec8cULL); // published FNV-1a 64 test vector
static_assert(Meta::hash_string("abc") != Meta::hash_string("acb"));

constexpr int dispatch(std::string_view command) {
    switch (Meta::hash_string(command)) {
    case Meta::hash_string("start"):
        return 1;
    case Meta::hash_string("stop"):
        return 2;
    default:
        return 0;
    }
}
static_assert(dispatch("start") == 1 && dispatch("stop") == 2 && dispatch("other") == 0);

// ----- loops and tuples -----
constexpr std::size_t sum_indices() {
    std::size_t total = 0;
    Meta::static_for<5>([&](auto i) { total = total * 10 + i; }); // order 0,1,2,3,4
    return total;
}
static_assert(sum_indices() == 1234);
static_assert(Meta::generate_array<int, 4>([](auto i) { return i * i; }) == std::array<int, 4>{0, 1, 4, 9});
static_assert(Meta::tuple_reverse(std::tuple{1, 'c', 2.0}) == std::tuple{2.0, 'c', 1});
static_assert(Meta::tuple_filter<std::is_integral>(std::tuple{1, 2.5, 'x', 3.5F}) == std::tuple{1, 'x'});
static_assert(Meta::tuple_transform(std::tuple{1, 2}, [](auto v) { return v * 10; }) == std::tuple{10, 20});

// ----- ratios and units -----
using half = Meta::Ratio<1, 2>;
using third = Meta::Ratio<1, 3>;
static_assert(Meta::Ratio<2, 4>::num == 1 && Meta::Ratio<2, 4>::den == 2);
static_assert(Meta::Ratio<3, -6>::num == -1 && Meta::Ratio<3, -6>::den == 2);
static_assert(std::is_same_v<Meta::Ratio<10, 20>::type, half>);
static_assert(std::is_same_v<Meta::ratio_add<half, third>, Meta::Ratio<5, 6>>);
static_assert(std::is_same_v<Meta::ratio_subtract<half, third>, Meta::Ratio<1, 6>>);
static_assert(std::is_same_v<Meta::ratio_multiply<half, third>, Meta::Ratio<1, 6>>);
static_assert(std::is_same_v<Meta::ratio_divide<half, third>, Meta::Ratio<3, 2>>);
static_assert(Meta::ratio_equal_v<Meta::Ratio<2, 6>, third>);

template <typename A, typename B>
concept Addable = requires(A a, B b) { a + b; };
static_assert(Addable<Meta::length, Meta::length>);
static_assert(!Addable<Meta::length, Meta::time_duration>); // dimensional safety
static_assert(!std::is_convertible_v<Meta::length, double>);
static_assert(std::is_same_v<decltype(Meta::length{1.0} / Meta::time_duration{1.0}), Meta::velocity>);
static_assert(std::is_same_v<decltype(Meta::mass{1.0} * Meta::acceleration{1.0}), Meta::force>);
static_assert(std::is_same_v<decltype(Meta::force{1.0} * Meta::length{1.0}), Meta::energy>);
static_assert((Meta::length{3.0} + Meta::length{4.0}).count() == 7.0);
static_assert((2.0 * Meta::length{3.0}).count() == 6.0);
static_assert(Meta::length{1.0} < Meta::length{2.0});

// ----- constexpr sorting and map -----
static_assert(Meta::bubble_sort(std::array{3, 1, 2}) == std::array{1, 2, 3});
static_assert(Meta::insertion_sort(std::array{5, -1, 4, 4, 0}) == std::array{-1, 0, 4, 4, 5});
static_assert(Meta::bubble_sort(std::array<int, 0>{}).empty());

constexpr auto colours = Meta::make_constexpr_map(std::pair{std::string_view("red"), 0xFF0000},
                                                  std::pair{std::string_view("green"), 0x00FF00});
static_assert(colours.size() == 2);
static_assert(colours.at("green") == 0x00FF00);
static_assert(colours.contains("red") && !colours.contains("blue"));
static_assert(!colours.find("blue").has_value());

// ----- CRTP -----
struct Version : Meta::TotallyOrdered<Version> {
    int major = 0;
    int minor = 0;
    constexpr Version(int ma, int mi) : major(ma), minor(mi) {}
    friend constexpr bool operator==(const Version& a, const Version& b) {
        return a.major == b.major && a.minor == b.minor;
    }
    friend constexpr bool operator<(const Version& a, const Version& b) {
        return a.major != b.major ? a.major < b.major : a.minor < b.minor;
    }
};
static_assert(Version(1, 2) != Version(1, 3));
static_assert(Version(2, 0) > Version(1, 9));
static_assert(Version(1, 1) <= Version(1, 1) && Version(1, 2) >= Version(1, 1));
static_assert(Meta::CircleShape(1.0).name() == "circle");
static_assert(Meta::RectangleShape(2.0, 4.0).area() == 8.0);
static_assert(sizeof(Meta::RectangleShape) == 2 * sizeof(double)); // empty CRTP bases add no size

// ----- state machine -----
struct Idle {};
struct Running {
    int speed = 0;
};
struct Stopped {};
struct Start {
    int speed;
};
struct Halt {};
struct IdleState : Idle {
    [[nodiscard]] Running on_event(const Start& e) const { return Running{e.speed}; }
};

class Config : public Meta::Singleton<Config> {
    friend class Meta::Singleton<Config>;
    Config() = default;

public:
    int value = 0;
};

} // namespace

TEST_CASE("compile-time arithmetic agrees with runtime computation", "[templates][meta]") {
    for (std::size_t n = 0; n <= 20; ++n) {
        std::size_t expected = 1;
        for (std::size_t i = 2; i <= n; ++i) {
            expected *= i;
        }
        REQUIRE(Meta::factorial_func(n) == expected);
    }
    CHECK(Meta::fibonacci_func(90) == 2880067194370816120ULL);
    CHECK(Meta::power_func(3, 13) == 1594323);
    std::size_t primes_below_1000 = 0;
    for (std::size_t i = 0; i < 1000; ++i) {
        primes_below_1000 += Meta::is_prime(i) ? 1 : 0;
    }
    CHECK(primes_below_1000 == 168);
}

TEST_CASE("compile_time_string supports runtime inspection", "[templates][meta]") {
    constexpr auto joined = Meta::compile_time_string("con") + Meta::compile_time_string("cat");
    CHECK(std::string(joined.c_str()) == "concat");
    CHECK(joined.size() == 6);
    CHECK(Meta::hash_string(std::string("concat")) == Meta::hash_string(joined.view()));
}

TEST_CASE("tuple utilities work on runtime values", "[templates][meta]") {
    const auto reversed = Meta::tuple_reverse(std::make_tuple(std::string("a"), 1, 2.5));
    CHECK(std::get<0>(reversed) == 2.5);
    CHECK(std::get<2>(reversed) == "a");

    auto source = std::make_tuple(std::make_unique<int>(4), 7);
    auto moved = Meta::tuple_reverse(std::move(source)); // forwards rvalues: move-only elements are moved
    CHECK(std::get<0>(moved) == 7);
    CHECK(*std::get<1>(moved) == 4);

    const auto filtered = Meta::tuple_filter<std::is_floating_point>(std::make_tuple(1, 2.0, std::string("s"), 3.0F));
    static_assert(std::is_same_v<std::remove_cvref_t<decltype(filtered)>, std::tuple<double, float>>);
    CHECK(std::get<1>(filtered) == 3.0F);

    const auto lengths = Meta::tuple_transform(std::make_tuple(std::string("ab"), std::string("abcd")),
                                               [](const std::string& s) { return s.size(); });
    CHECK(lengths == std::make_tuple(std::size_t{2}, std::size_t{4}));
}

TEST_CASE("quantities carry units through arithmetic", "[templates][meta][units]") {
    const Meta::length distance{100.0};
    const Meta::time_duration duration{9.58};
    const Meta::velocity speed = distance / duration;
    CHECK(speed.count() == Catch::Approx(10.438).epsilon(1e-3));

    const Meta::mass m{80.0};
    const Meta::acceleration g{9.81};
    const Meta::force weight = m * g;
    const Meta::energy work = weight * Meta::length{2.0};
    CHECK(work.count() == Catch::Approx(1569.6));

    Meta::length total{1.0};
    total += Meta::length{2.5};
    total -= Meta::length{0.5};
    CHECK(total.count() == 3.0);
    CHECK((-total).count() == -3.0);
    CHECK((total / 2.0).count() == 1.5);
    CHECK(Meta::Ratio<1, 4>::to_double() == 0.25);
}

TEST_CASE("expression templates evaluate lazily and match eager arithmetic", "[templates][meta][expr]") {
    const Meta::ExprVector<double> a{1.0, 2.0, 3.0, 4.0};
    const Meta::ExprVector<double> b{0.5, 0.5, 0.5, 0.5};
    const Meta::ExprVector<double> c{2.0, 2.0, 2.0, 2.0};

    const auto expr = a + b * c - 3.0 * a; // a node tree, nothing computed yet
    static_assert(!std::is_same_v<std::remove_cvref_t<decltype(expr)>, Meta::ExprVector<double>>);
    CHECK(expr.size() == 4);
    CHECK(expr[1] == Catch::Approx(2.0 + 1.0 - 6.0));

    const Meta::ExprVector<double> result = expr; // single fused evaluation
    for (std::size_t i = 0; i < a.size(); ++i) {
        CHECK(result[i] == Catch::Approx(a[i] + b[i] * c[i] - 3.0 * a[i]));
    }

    Meta::ExprVector<double> target(1);
    target = a - b;
    CHECK(target.size() == 4);
    CHECK(target.values() == std::vector<double>{0.5, 1.5, 2.5, 3.5});
}

TEST_CASE("CRTP shapes compute areas without virtual dispatch", "[templates][meta][crtp]") {
    const Meta::CircleShape circle(2.0);
    const Meta::RectangleShape rect(3.0, 4.0);
    CHECK(circle.area() == Catch::Approx(12.566370614));
    CHECK(rect.name() == "rectangle");
    CHECK(Meta::total_area(circle, rect, rect) == Catch::Approx(12.566370614 + 24.0));
    CHECK(Meta::total_area() == 0.0);
}

TEST_CASE("StateMachine transitions on handled events only", "[templates][meta][state]") {
    struct RunningState {
        int speed = 0;
        [[nodiscard]] Stopped on_event(const Halt&) const { return Stopped{}; }
    };
    struct IdleLocal {
        [[nodiscard]] RunningState on_event(const Start& e) const { return RunningState{e.speed}; }
    };

    Meta::StateMachine<IdleLocal, RunningState, Stopped> machine{IdleLocal{}};
    CHECK(machine.is_state<IdleLocal>());
    CHECK_FALSE(machine.process_event(Halt{})); // Idle ignores Halt
    CHECK(machine.is_state<IdleLocal>());

    CHECK(machine.process_event(Start{42}));
    REQUIRE(machine.is_state<RunningState>());
    CHECK(machine.get_state<RunningState>()->speed == 42);
    CHECK(machine.get_state<IdleLocal>() == nullptr);

    CHECK(machine.process_event(Halt{}));
    CHECK(machine.is_state<Stopped>());
    CHECK_FALSE(machine.process_event(Start{1})); // terminal state
    const auto index = machine.visit([](const auto& state) {
        return std::is_same_v<std::remove_cvref_t<decltype(state)>, Stopped> ? 2 : -1;
    });
    CHECK(index == 2);

    Meta::StateMachine<IdleState, Running> other{IdleState{}};
    CHECK(other.process_event(Start{5}));
    CHECK(other.get_state<Running>()->speed == 5);
}

TEST_CASE("ConstexprMap works at runtime too", "[templates][meta]") {
    const auto map = Meta::make_constexpr_map(std::pair{1, std::string_view("one")}, std::pair{2, std::string_view("two")},
                                              std::pair{3, std::string_view("three")});
    CHECK(map.at(3) == "three");
    CHECK(map.find(4) == std::nullopt);
    CHECK_THROWS_AS(map.at(4), std::out_of_range);
}

TEST_CASE("Singleton returns one instance", "[templates][meta][patterns]") {
    Config& a = Config::instance();
    Config& b = Config::instance();
    CHECK(&a == &b);
    a.value = 17;
    CHECK(b.value == 17);
    static_assert(!std::is_copy_constructible_v<Config>);
    static_assert(!std::is_default_constructible_v<Config>);
}

TEST_CASE("Observable delivers events in order and supports unsubscribe", "[templates][meta][patterns]") {
    Meta::Observable<int> subject;
    std::vector<std::string> log;
    const auto first = subject.subscribe([&log](int v) { log.push_back("a" + std::to_string(v)); });
    subject.subscribe([&log](int v) { log.push_back("b" + std::to_string(v)); });
    CHECK(subject.observer_count() == 2);

    subject.notify(1);
    CHECK(subject.unsubscribe(first));
    CHECK_FALSE(subject.unsubscribe(first));
    subject.notify(2);
    CHECK(log == std::vector<std::string>{"a1", "b1", "b2"});

    subject.clear_observers();
    subject.notify(3);
    CHECK(log.size() == 3);
}

TEST_CASE("Command wraps callables polymorphically", "[templates][meta][patterns]") {
    int counter = 0;
    auto increment = Meta::make_command([&counter] { ++counter; });
    increment->execute();
    increment->execute();
    increment->undo(); // default no-op
    CHECK(counter == 2);

    auto compute = Meta::make_command([] { return std::string("result"); });
    static_assert(std::is_same_v<decltype(compute), std::unique_ptr<Meta::Command<std::string>>>);
    CHECK(compute->execute() == "result");
}
