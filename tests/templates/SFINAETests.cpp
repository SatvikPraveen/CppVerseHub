// Tests for templates/SFINAE_Examples.hpp
#include "templates/SFINAE_Examples.hpp"

#include <catch2/catch_test_macros.hpp>

#include <array>
#include <deque>
#include <forward_list>
#include <list>
#include <map>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

namespace S = CppVerseHub::Templates::SFINAE;

namespace {

struct Opaque {};
struct WithMembers {
    int data = 0;
    int value() const { return data; }
};
struct Sized {
    [[nodiscard]] std::size_t size() const { return 3; }
};
struct ThrowingMove {
    ThrowingMove() = default;
    ThrowingMove(const ThrowingMove&) = default;
    ThrowingMove(ThrowingMove&&) noexcept(false) {}
    ThrowingMove& operator=(const ThrowingMove&) = default;
    ThrowingMove& operator=(ThrowingMove&&) = default;
    ~ThrowingMove() = default;
};
struct NeedsArgs {
    explicit NeedsArgs(int v) : value(v) {}
    int value;
};

// ----- classic detection -----
static_assert(S::has_size_method_v<std::vector<int>> && S::has_size_method_v<Sized>);
static_assert(!S::has_size_method_v<int> && !S::has_size_method_v<std::forward_list<int>>);
static_assert(S::has_begin_method_v<std::list<int>> && !S::has_begin_method_v<Opaque>);
static_assert(S::has_end_method_v<std::string> && !S::has_end_method_v<int*>);
static_assert(S::is_iterable_v<std::forward_list<int>> && !S::is_iterable_v<int[3]>);

static_assert(S::has_member_data_v<WithMembers> && S::has_member_value_v<WithMembers>);
static_assert(S::has_member_first_v<std::pair<int, char>> && S::has_member_second_v<std::pair<int, char>>);
static_assert(!S::has_member_first_v<WithMembers> && !S::has_member_data_v<int>);

static_assert(S::supports_arithmetic_v<int> && S::supports_arithmetic_v<double>);
static_assert(!S::supports_arithmetic_v<std::string>);
static_assert(S::supports_arithmetic<std::string>::has_add && !S::supports_arithmetic<std::string>::has_mul);
static_assert(S::supports_arithmetic<int*>::has_sub && !S::supports_arithmetic<int*>::has_add);

// ----- void_t / detection idiom -----
static_assert(S::has_insertion_operator_v<int> && S::has_insertion_operator_v<std::string>);
static_assert(!S::has_insertion_operator_v<Opaque> && !S::has_insertion_operator_v<std::vector<int>>);
static_assert(S::is_detected_v<S::size_expression_t, std::vector<int>>);
static_assert(!S::is_detected_v<S::size_expression_t, Opaque>);
static_assert(S::is_detected_v<S::push_back_expression_t, std::vector<int>, int>);
static_assert(!S::is_detected_v<S::push_back_expression_t, std::vector<int>, std::string>);
static_assert(std::is_same_v<S::detected_t<S::value_type_t, std::vector<char>>, char>);
static_assert(std::is_same_v<S::detected_t<S::value_type_t, int>, S::nonesuch>);
static_assert(std::is_same_v<S::detected_or_t<long, S::value_type_t, int>, long>);
static_assert(std::is_same_v<S::detected_t<S::size_expression_t, Sized>, std::size_t>);

// ----- smart pointers, callables, factories -----
static_assert(S::is_smart_pointer_v<std::unique_ptr<int>> && S::is_smart_pointer_v<const std::shared_ptr<int>>);
static_assert(!S::is_smart_pointer_v<int*>);
static_assert(S::is_callable_v<int (*)(int), int> && !S::is_callable_v<int (*)(int), std::string>);
static_assert(S::is_callable_v<decltype([] {})> && !S::is_callable_v<int>);
static_assert(std::is_same_v<S::vector_element_t<std::vector<double>>, double>);

template <typename T>
concept HasVectorElement = requires { typename S::vector_element_t<T>; };
static_assert(HasVectorElement<std::vector<int>> && !HasVectorElement<std::list<int>>);

template <typename T>
concept CanMakeDefault = requires { S::make_unique_default<T>(); };
static_assert(CanMakeDefault<std::string> && !CanMakeDefault<NeedsArgs>);

template <typename T, typename... Args>
concept CanMakeWith = requires(Args... args) { S::make_unique_with_args<T>(args...); };
static_assert(CanMakeWith<NeedsArgs, int> && !CanMakeWith<NeedsArgs, std::string>);

template <typename F, typename... Args>
concept CanSafeInvoke = requires(F f, Args... args) { S::safe_invoke(f, args...); };
static_assert(CanSafeInvoke<int (*)(int), int> && !CanSafeInvoke<int (*)(int), Opaque>);

// ----- conditional_move returns rvalue only for nothrow-movable types -----
static_assert(std::is_same_v<decltype(S::conditional_move(std::declval<std::string&>())), std::string&&>);
static_assert(std::is_same_v<decltype(S::conditional_move(std::declval<ThrowingMove&>())), const ThrowingMove&>);
static_assert(std::is_same_v<decltype(S::conditional_move(std::declval<std::unique_ptr<int>&>())),
                             std::unique_ptr<int>&&>);

// ----- tag selection -----
static_assert(std::is_same_v<S::type_tag_t<int>, S::arithmetic_tag>);
static_assert(std::is_same_v<S::type_tag_t<std::string>, S::string_tag>);
static_assert(std::is_same_v<S::type_tag_t<std::deque<int>>, S::container_tag>);
static_assert(std::is_same_v<S::type_tag_t<Opaque>, S::generic_tag>);

static_assert(S::algorithm_selector<int*>::is_constant_time);
static_assert(!S::algorithm_selector<std::list<int>::iterator>::is_constant_time);
static_assert(S::AdaptiveContainer<std::vector<int>>::has_constant_time_size());
static_assert(!S::AdaptiveContainer<std::forward_list<int>>::has_constant_time_size());

} // namespace

TEST_CASE("describe_value selects overloads with enable_if", "[templates][sfinae]") {
    CHECK(S::describe_value(42) == "arithmetic: 42");
    CHECK(S::describe_value(2.5) == "arithmetic: 2.5");
    CHECK(S::describe_value(std::string("hi")) == "string: \"hi\"");
    CHECK(S::describe_value(std::vector<int>{1, 2}) == "container: [1, 2]");
    CHECK(S::describe_value(std::list<std::string>{}) == "container: []");
    CHECK(S::describe_value(Opaque{}) == "opaque object");
}

TEST_CASE("get_size uses size(), iteration or the array bound", "[templates][sfinae]") {
    CHECK(S::get_size(std::vector<int>{1, 2, 3}) == 3);
    CHECK(S::get_size(std::forward_list<int>{1, 2, 3, 4}) == 4);
    const int raw[5] = {};
    CHECK(S::get_size(raw) == 5);
    static_assert(S::get_size("abc") == 4);
}

TEST_CASE("serialize recurses through nested containers", "[templates][sfinae]") {
    CHECK(S::serialize(7) == "7");
    CHECK(S::serialize(true) == "true");
    CHECK(S::serialize(std::string("x")) == "\"x\"");
    CHECK(S::serialize(std::vector<std::string>{"a", "b"}) == "[\"a\",\"b\"]");
    CHECK(S::serialize(std::vector<std::vector<int>>{{1}, {}, {2, 3}}) == "[[1],[],[2,3]]");
    CHECK(S::serialize(std::vector<bool>{true, false}) == "[true,false]");
}

TEST_CASE("safe_print falls back for non-streamable types", "[templates][sfinae]") {
    std::ostringstream oss;
    S::safe_print(oss, 12);
    S::safe_print(oss, Opaque{});
    S::safe_print(oss, std::string("s"));
    CHECK(oss.str() == "12\n[non-printable object]\ns\n");
}

TEST_CASE("safe_dereference handles raw and smart pointers", "[templates][sfinae]") {
    int value = 5;
    int* null_raw = nullptr;
    CHECK(S::safe_dereference(&value) == "raw pointer -> 5");
    CHECK(S::safe_dereference(null_raw) == "null raw pointer");
    CHECK(S::safe_dereference(std::make_unique<int>(6)) == "smart pointer -> 6");
    CHECK(S::safe_dereference(std::shared_ptr<int>{}) == "null smart pointer");
}

TEST_CASE("process_value dispatches on computed tags", "[templates][sfinae]") {
    CHECK(S::process_value(std::vector<int>{1, 2, 3}) == "container with 3 elements");
    CHECK(S::process_value(std::forward_list<int>{1}) == "container with 1 elements");
    CHECK(S::process_value(3) == "arithmetic 3");
    CHECK(S::process_value(std::string("abcd")) == "string of length 4");
    CHECK(S::process_value(Opaque{}) == "generic object");
}

TEST_CASE("algorithm_selector advances iterators of every category", "[templates][sfinae]") {
    std::vector<int> vec{0, 1, 2, 3, 4};
    auto vit = vec.begin();
    S::algorithm_selector<decltype(vit)>::advance(vit, 3);
    CHECK(*vit == 3);

    std::list<int> lst{0, 1, 2, 3, 4};
    auto lit = lst.begin();
    S::algorithm_selector<decltype(lit)>::advance(lit, 4);
    CHECK(*lit == 4);
    S::algorithm_selector<decltype(lit)>::advance(lit, -2);
    CHECK(*lit == 2);
}

TEST_CASE("optimized_copy uses memcpy or std::copy transparently", "[templates][sfinae]") {
    const std::array<int, 4> source{1, 2, 3, 4};
    std::array<int, 4> dest{};
    const int* end = S::optimized_copy(source.data(), source.data() + source.size(), dest.data());
    CHECK(end == dest.data() + 4);
    CHECK(dest == source);

    CHECK(S::optimized_copy(source.data(), source.data(), dest.data()) == dest.data()); // empty range

    const std::list<std::string> words{"x", "y"};
    std::vector<std::string> out(2);
    S::optimized_copy(words.begin(), words.end(), out.begin());
    CHECK(out == std::vector<std::string>{"x", "y"});
}

TEST_CASE("conditional_move moves nothrow types and copies others", "[templates][sfinae]") {
    std::string source = "payload";
    std::string target = S::conditional_move(source);
    CHECK(target == "payload");
    CHECK(source.empty()); // NOLINT(bugprone-use-after-move): libc++/libstdc++ leave moved-from strings empty

    ThrowingMove throwing;
    [[maybe_unused]] const ThrowingMove& ref = S::conditional_move(throwing);
    CHECK(&ref == &throwing);
}

TEST_CASE("safe_invoke forwards to callables", "[templates][sfinae]") {
    CHECK(S::safe_invoke([](int a, int b) { return a * b; }, 6, 7) == 42);
    std::string text = "abc";
    S::safe_invoke([](std::string& s) { s += "d"; }, text);
    CHECK(text == "abcd");
}

TEST_CASE("AdaptiveContainer chooses its implementation by detection", "[templates][sfinae]") {
    S::AdaptiveContainer<std::vector<int>> sized(std::vector<int>{1, 2, 3});
    CHECK(sized.size() == 3);
    CHECK_FALSE(sized.empty());

    S::AdaptiveContainer<std::forward_list<int>> unsized(std::forward_list<int>{4, 5});
    CHECK(unsized.size() == 2);
    CHECK_FALSE(unsized.empty());
    int total = 0;
    for (int v : unsized) {
        total += v;
    }
    CHECK(total == 9);

    S::AdaptiveContainer<std::forward_list<int>> empty(std::forward_list<int>{});
    CHECK(empty.empty());
}

TEST_CASE("SFINAE-constrained factories", "[templates][sfinae]") {
    const auto text = S::make_unique_default<std::string>();
    CHECK(text->empty());
    const auto obj = S::make_unique_with_args<NeedsArgs>(9);
    CHECK(obj->value == 9);
}

TEST_CASE("modern_describe expresses the same dispatch with concepts", "[templates][sfinae][concepts]") {
    CHECK(S::modern_describe(5) == "arithmetic 5");
    CHECK(S::modern_describe(std::vector<int>{1, 2}) == "container with 2 elements");
    CHECK(S::modern_describe(std::map<int, int>{{1, 1}}) == "container with 1 elements");
    CHECK(S::modern_describe(std::string("str")) == "printable str");
    CHECK(S::modern_describe(Opaque{}) == "generic object");
}
