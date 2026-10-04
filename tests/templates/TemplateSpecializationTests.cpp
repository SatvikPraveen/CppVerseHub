// Tests for templates/TemplateSpecialization.hpp
#include "templates/TemplateSpecialization.hpp"

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

#include <array>
#include <cstring>
#include <deque>
#include <forward_list>
#include <iterator>
#include <list>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <tuple>
#include <variant>
#include <vector>

namespace Sp = CppVerseHub::Templates::Specialization;

namespace {

struct Opaque {};

struct Counter {
    int count = 0;
    int add(int n) { return count += n; }
    [[nodiscard]] int get() const { return count; }
};

int twice(int v) {
    return 2 * v;
}
int safe_twice(int v) noexcept {
    return 2 * v;
}

constexpr bool str_eq(const char* a, const char* b) {
    return std::string_view(a) == std::string_view(b);
}

// ----- TypeInfo -----
static_assert(!Sp::TypeInfo<int>::is_pointer && str_eq(Sp::TypeInfo<int>::name, "value"));
static_assert(Sp::TypeInfo<const int>::is_const && !Sp::TypeInfo<int>::is_const);
static_assert(Sp::TypeInfo<volatile int>::is_volatile);
static_assert(Sp::TypeInfo<int*>::is_pointer && std::is_same_v<Sp::TypeInfo<int*>::pointed_type, int>);
static_assert(Sp::TypeInfo<const int*>::is_pointer && !Sp::TypeInfo<const int*>::is_const);
static_assert(Sp::TypeInfo<int* const>::is_pointer && Sp::TypeInfo<int* const>::is_const);
static_assert(Sp::TypeInfo<int&>::is_reference && str_eq(Sp::TypeInfo<int&>::name, "lvalue_reference"));
static_assert(std::is_same_v<Sp::TypeInfo<const double&>::referenced_type, const double>);
static_assert(str_eq(Sp::TypeInfo<int&&>::name, "rvalue_reference"));
static_assert(Sp::TypeInfo<int[7]>::is_array && Sp::TypeInfo<int[7]>::size == 7);
static_assert(Sp::TypeInfo<const int[3]>::is_array && Sp::TypeInfo<const int[3]>::is_const); // no ambiguity
static_assert(str_eq(Sp::TypeInfo<char[]>::name, "unbounded_array"));

// ----- Constness: most specialised cv pattern wins -----
static_assert(str_eq(Sp::Constness<int>::name, "unqualified"));
static_assert(str_eq(Sp::Constness<const int>::name, "const"));
static_assert(str_eq(Sp::Constness<volatile int>::name, "volatile"));
static_assert(str_eq(Sp::Constness<const volatile int>::name, "const volatile"));
static_assert(std::is_same_v<Sp::Constness<const volatile long>::unqualified, long>);

// ----- FunctionWrapper -----
using FreeFn = Sp::FunctionWrapper<int (*)(int)>;
using NoexceptFn = Sp::FunctionWrapper<int (*)(int) noexcept>;
using MemberFn = Sp::FunctionWrapper<int (Counter::*)(int)>;
using ConstMemberFn = Sp::FunctionWrapper<int (Counter::*)() const>;
static_assert(FreeFn::arity == 1 && !FreeFn::is_noexcept && std::is_same_v<FreeFn::return_type, int>);
static_assert(NoexceptFn::is_noexcept);
static_assert(std::is_same_v<MemberFn::class_type, Counter> && MemberFn::arity == 1);
static_assert(ConstMemberFn::arity == 0 && str_eq(ConstMemberFn::type_name, "const_member_function_pointer"));
static_assert(
    std::is_same_v<Sp::FunctionWrapper<double (*)(int, char)>::argument_types, std::tuple<int, char>>);

// ----- variable templates and element types -----
static_assert(Sp::is_numeric_v<int> && Sp::is_numeric_v<double> && !Sp::is_numeric_v<bool>);
static_assert(!Sp::is_numeric_v<std::string>);
static_assert(str_eq(Sp::size_category_v<char>, "tiny") && str_eq(Sp::size_category_v<long long>, "huge"));
static_assert(str_eq(Sp::size_category_v<float>, "unknown"));
static_assert(std::is_same_v<Sp::element_type_t<std::vector<char>>, char>);
static_assert(std::is_same_v<Sp::element_type_t<int[4]>, int>);
static_assert(std::is_same_v<Sp::element_type_t<const double*>, const double>);
static_assert(std::is_same_v<Sp::pointer_t<int>, int*>);
static_assert(std::is_same_v<Sp::unique_pointer_t<int>, std::unique_ptr<int>>);
static_assert(std::is_same_v<Sp::shared_pointer_t<int>, std::shared_ptr<int>>);

// ----- selectors -----
static_assert(str_eq(Sp::AlgorithmSelector<std::vector<int>>::strategy, "introsort"));
static_assert(str_eq(Sp::AlgorithmSelector<std::deque<int>>::strategy, "introsort"));
static_assert(str_eq(Sp::AlgorithmSelector<std::list<int>>::strategy, "copy-sort-copy"));
static_assert(str_eq(Sp::AlgorithmSelector<std::forward_list<int>>::strategy, "unsupported"));
static_assert(str_eq(Sp::AlgorithmSelector<int>::strategy, "unsupported"));

static_assert(Sp::TupleProcessor<std::tuple<int, int, int>>::arity == 3);
static_assert(Sp::TupleProcessor<std::pair<int, int>>::arity == 2);
static_assert(Sp::TupleProcessor<int>::arity == 1);
static_assert(!Sp::Serializer<Opaque>::is_specialized && Sp::Serializer<int>::is_specialized);
static_assert(Sp::Serializer<std::vector<int>>::is_specialized &&
              !Sp::Serializer<std::vector<Opaque>>::is_specialized);

} // namespace

TEST_CASE("Serializer full specialisations round-trip", "[templates][specialization]") {
    const int value = GENERATE(-1000, -1, 0, 7, 123456);
    CHECK(Sp::Serializer<int>::deserialize(Sp::Serializer<int>::serialize(value)) == value);

    const double d = GENERATE(0.1, -2.5, 1e100, 3.141592653589793);
    CHECK(Sp::Serializer<double>::deserialize(Sp::Serializer<double>::serialize(d)) == d);

    CHECK(Sp::Serializer<std::string>::serialize("abc") == "\"abc\"");
    CHECK(Sp::Serializer<std::string>::deserialize("\"abc\"") == "abc");
    CHECK(Sp::Serializer<std::string>::deserialize("raw") == "raw");
    CHECK(Sp::Serializer<bool>::serialize(false) == "false");
    CHECK(Sp::Serializer<bool>::deserialize("true"));
    CHECK_FALSE(Sp::Serializer<bool>::deserialize("yes"));
    CHECK(Sp::Serializer<Opaque>::serialize(Opaque{}) == "<opaque>");
}

TEST_CASE("Serializer errors on malformed input", "[templates][specialization]") {
    CHECK_THROWS_AS(Sp::Serializer<int>::deserialize("not a number"), std::invalid_argument);
    CHECK_THROWS_AS(Sp::Serializer<double>::deserialize("abc"), std::invalid_argument);
}

TEST_CASE("Serializer partial specialisation for vectors composes element serializers",
          "[templates][specialization]") {
    CHECK(Sp::Serializer<std::vector<int>>::serialize({}) == "[]");
    CHECK(Sp::Serializer<std::vector<bool>>::serialize({true, false}) == "[true,false]");
    CHECK(Sp::Serializer<std::vector<std::vector<int>>>::serialize({{1, 2}, {3}}) == "[[1,2],[3]]");
}

TEST_CASE("ContainerPrinter selects the matching partial specialisation", "[templates][specialization]") {
    CHECK(Sp::print_to_string(std::vector<int>{1, 2}) == "vector[2]: [1, 2]");
    CHECK(Sp::print_to_string(std::array<char, 3>{'a', 'b', 'c'}) == "array[3]: [a, b, c]");
    CHECK(Sp::print_to_string(std::make_unique<int>(4)) == "unique_ptr -> 4");
    CHECK(Sp::print_to_string(std::unique_ptr<int>{}) == "unique_ptr -> null");
    const auto shared = std::make_shared<std::string>("s");
    const auto copy = shared;
    CHECK(Sp::print_to_string(shared) == "shared_ptr[2] -> s");
    CHECK(Sp::print_to_string(std::shared_ptr<int>{}) == "shared_ptr -> null");
    CHECK(Sp::print_to_string(42) == "value: 42");
    CHECK(Sp::print_to_string(Opaque{}) == "<opaque object>");
}

TEST_CASE("IteratorHelper specialisations by category", "[templates][specialization]") {
    std::vector<int> vec{1, 2, 3, 4, 5};
    using VecHelper = Sp::IteratorHelper<std::vector<int>::iterator>;
    CHECK(std::string(VecHelper::category_name) == "random_access_iterator");
    CHECK(VecHelper::distance(vec.begin(), vec.end()) == 5);
    auto vit = vec.begin();
    VecHelper::advance(vit, 3);
    CHECK(*vit == 4);
    VecHelper::retreat(vit, 2);
    CHECK(*vit == 2);

    std::list<int> lst{1, 2, 3};
    using ListHelper = Sp::IteratorHelper<std::list<int>::iterator>;
    CHECK(std::string(ListHelper::category_name) == "bidirectional_iterator");
    auto lit = lst.begin();
    ListHelper::advance(lit, 2);
    CHECK(*lit == 3);
    ListHelper::retreat(lit, 1);
    CHECK(*lit == 2);
    CHECK(ListHelper::distance(lst.begin(), lst.end()) == 3);

    std::forward_list<int> fl{1, 2, 3, 4};
    using FwdHelper = Sp::IteratorHelper<std::forward_list<int>::iterator>;
    CHECK(std::string(FwdHelper::category_name) == "forward_iterator");
    CHECK(FwdHelper::distance(fl.begin(), fl.end()) == 4);

    std::istringstream input("10 20 30");
    using InHelper = Sp::IteratorHelper<std::istream_iterator<int>>;
    CHECK(std::string(InHelper::category_name) == "input_iterator");
    CHECK(InHelper::distance(std::istream_iterator<int>(input), std::istream_iterator<int>()) == 3);
}

TEST_CASE("FunctionWrapper invokes free and member functions", "[templates][specialization]") {
    const FreeFn free_fn(&twice);
    CHECK(free_fn(21) == 42);
    const NoexceptFn noexcept_fn(&safe_twice);
    CHECK(noexcept_fn(4) == 8);
    static_assert(noexcept(noexcept_fn(4)));

    Counter counter;
    const MemberFn add(&Counter::add);
    CHECK(add(counter, 5) == 5);
    CHECK(add(counter, 2) == 7);
    const ConstMemberFn get(&Counter::get);
    CHECK(get(counter) == 7);
}

TEST_CASE("TupleProcessor and OptionHandler describe their arguments", "[templates][specialization]") {
    CHECK(Sp::TupleProcessor<std::tuple<int, std::string>>::describe({1, "a"}) == "tuple<2>(1, a)");
    CHECK(Sp::TupleProcessor<std::tuple<>>::describe({}) == "tuple<0>()");
    CHECK(Sp::TupleProcessor<std::pair<char, double>>::describe({'x', 0.5}) == "pair(x, 0.5)");
    CHECK(Sp::TupleProcessor<int>::describe(3) == "not a tuple");

    CHECK(Sp::OptionHandler<int>::describe(5) == "value 5");
    CHECK(Sp::OptionHandler<std::optional<int>>::describe(std::nullopt) == "empty optional");
    CHECK(Sp::OptionHandler<std::optional<int>>::describe(9) == "optional 9");
    using V = std::variant<int, std::string, double>;
    CHECK(Sp::OptionHandler<V>::describe(V{2.5}) == "variant#2 2.5");
    CHECK(Sp::OptionHandler<V>::describe(V{std::string("s")}) == "variant#1 s");
}

TEST_CASE("AlgorithmSelector sorts random-access and bidirectional containers",
          "[templates][specialization]") {
    std::vector<int> vec{3, 1, 2};
    Sp::AlgorithmSelector<std::vector<int>>::sort(vec);
    CHECK(vec == std::vector<int>{1, 2, 3});

    std::list<std::string> lst{"pear", "apple", "fig"};
    Sp::AlgorithmSelector<std::list<std::string>>::sort(lst);
    CHECK(lst == std::list<std::string>{"apple", "fig", "pear"});
}
