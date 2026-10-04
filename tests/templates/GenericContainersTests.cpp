// Tests for templates/GenericContainers.hpp
#include "templates/GenericContainers.hpp"

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

#include <algorithm>
#include <iterator>
#include <list>
#include <memory>
#include <numeric>
#include <random>
#include <ranges>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

using CppVerseHub::Templates::DynamicArray;
using CppVerseHub::Templates::make_shared_ptr;
using CppVerseHub::Templates::make_unique_ptr;
using CppVerseHub::Templates::Optional;
using CppVerseHub::Templates::SharedPtr;
using CppVerseHub::Templates::UniquePtr;
using CppVerseHub::Templates::WeakPtr;

namespace {

// Counts live instances so tests can detect leaks and double destruction.
struct Tracked {
    static inline int live = 0;
    int value = 0;
    Tracked() noexcept { ++live; }
    explicit Tracked(int v) noexcept : value(v) { ++live; }
    Tracked(const Tracked& other) noexcept : value(other.value) { ++live; }
    Tracked(Tracked&& other) noexcept : value(other.value) { ++live; }
    Tracked& operator=(const Tracked&) = default;
    Tracked& operator=(Tracked&&) = default;
    ~Tracked() { --live; }
    friend bool operator==(const Tracked& a, const Tracked& b) { return a.value == b.value; }
};

// Copy constructor throws on demand: used to verify exception guarantees.
struct ThrowOnCopy {
    static inline int copies_until_throw = -1;
    static inline int live = 0;
    int value = 0;
    explicit ThrowOnCopy(int v) : value(v) { ++live; }
    ThrowOnCopy(const ThrowOnCopy& other) : value(other.value) {
        if (copies_until_throw == 0) {
            throw std::runtime_error("copy failed");
        }
        if (copies_until_throw > 0) {
            --copies_until_throw;
        }
        ++live;
    }
    // Deliberately potentially-throwing move: forces DynamicArray to copy during reallocation.
    ThrowOnCopy(ThrowOnCopy&& other) noexcept(false) : value(other.value) { ++live; }
    ThrowOnCopy& operator=(const ThrowOnCopy&) = default;
    ThrowOnCopy& operator=(ThrowOnCopy&&) = default;
    ~ThrowOnCopy() { --live; }
};

// Stateful allocator that counts outstanding allocations; non-propagating, unequal across ids.
template <typename T>
struct CountingAllocator {
    using value_type = T;
    using propagate_on_container_copy_assignment = std::false_type;
    using propagate_on_container_move_assignment = std::false_type;
    using propagate_on_container_swap = std::false_type;
    using is_always_equal = std::false_type;

    int id = 0;
    std::shared_ptr<long> outstanding = std::make_shared<long>(0);

    CountingAllocator() = default;
    explicit CountingAllocator(int allocator_id) : id(allocator_id) {}
    template <typename U>
    CountingAllocator(const CountingAllocator<U>& other) noexcept // NOLINT
        : id(other.id), outstanding(other.outstanding) {}

    T* allocate(std::size_t n) {
        *outstanding += static_cast<long>(n);
        return std::allocator<T>{}.allocate(n);
    }
    void deallocate(T* p, std::size_t n) noexcept {
        *outstanding -= static_cast<long>(n);
        std::allocator<T>{}.deallocate(p, n);
    }
    template <typename U>
    friend bool operator==(const CountingAllocator& a, const CountingAllocator<U>& b) noexcept {
        return a.id == b.id;
    }
};

struct Base {
    Base() = default;
    Base(const Base&) = default;
    Base& operator=(const Base&) = default;
    Base(Base&&) = default;
    Base& operator=(Base&&) = default;
    virtual ~Base() = default;
    [[nodiscard]] virtual int id() const { return 1; }
};
struct Derived final : Base {
    [[nodiscard]] int id() const override { return 2; }
};

static_assert(std::contiguous_iterator<DynamicArray<int>::iterator>);
static_assert(std::contiguous_iterator<DynamicArray<int>::const_iterator>);
static_assert(std::ranges::contiguous_range<DynamicArray<int>>);
static_assert(std::is_nothrow_move_constructible_v<DynamicArray<std::string>>);
static_assert(std::convertible_to<DynamicArray<int>::iterator, DynamicArray<int>::const_iterator>);
static_assert(!std::convertible_to<DynamicArray<int>::const_iterator, DynamicArray<int>::iterator>);
static_assert(sizeof(UniquePtr<int>) == sizeof(int*), "empty deleter must not add size");
static_assert(!std::is_copy_constructible_v<UniquePtr<int>>);
static_assert(!std::is_copy_constructible_v<Optional<std::unique_ptr<int>>>);
static_assert(std::is_nothrow_move_constructible_v<Optional<std::string>>);

// Optional is usable in constant expressions.
constexpr int optional_constexpr_check() {
    Optional<int> a;
    Optional<int> b = 5;
    a = b;
    a.emplace(*a + 1);
    return a.value() + b.value_or(0) + (Optional<int>{}.value_or(100));
}
static_assert(optional_constexpr_check() == 111);
static_assert(Optional<int>(3).transform([](int v) { return v * 2; }).value() == 6);

} // namespace

TEST_CASE("DynamicArray construction forms", "[templates][containers][dynamic_array]") {
    const DynamicArray<int> empty;
    CHECK(empty.empty());
    CHECK(empty.capacity() == 0);
    CHECK(empty.data() == nullptr);

    const DynamicArray<int> zeros(4);
    CHECK(zeros.size() == 4);
    CHECK(std::all_of(zeros.begin(), zeros.end(), [](int v) { return v == 0; }));

    const DynamicArray<std::string> filled(3, std::string("x"));
    CHECK(filled.size() == 3);
    CHECK(filled.back() == "x");

    const std::list<int> source{1, 2, 3};
    const DynamicArray<int> from_list(source.begin(), source.end());
    CHECK(std::equal(from_list.begin(), from_list.end(), source.begin(), source.end()));

    const DynamicArray<int> from_init{5, 6, 7};
    CHECK(from_init.front() == 5);
    CHECK(from_init[1] == 6);
    CHECK(from_init.at(2) == 7);
    CHECK_THROWS_AS(from_init.at(3), std::out_of_range);
}

TEST_CASE("DynamicArray push_back grows geometrically and preserves elements", "[templates][containers][dynamic_array]") {
    DynamicArray<int> values;
    std::size_t reallocations = 0;
    std::size_t last_capacity = 0;
    for (int i = 0; i < 1000; ++i) {
        values.push_back(i);
        if (values.capacity() != last_capacity) {
            ++reallocations;
            last_capacity = values.capacity();
        }
    }
    CHECK(values.size() == 1000);
    CHECK(reallocations <= 11); // 1,2,4,...,1024
    for (int i = 0; i < 1000; ++i) {
        REQUIRE(values[static_cast<std::size_t>(i)] == i);
    }
    values.pop_back();
    CHECK(values.back() == 998);
}

TEST_CASE("DynamicArray emplace_back with an aliasing argument during reallocation", "[templates][containers][dynamic_array]") {
    DynamicArray<std::string> words{"alpha"};
    REQUIRE(words.size() == words.capacity());
    words.emplace_back(words[0]); // argument refers into the buffer being reallocated
    CHECK(words.size() == 2);
    CHECK(words[1] == "alpha");
    words.push_back(words.back());
    CHECK(words[2] == "alpha");
}

TEST_CASE("DynamicArray insert and emplace at arbitrary positions", "[templates][containers][dynamic_array]") {
    DynamicArray<int> values{1, 2, 5};
    values.reserve(10);

    SECTION("single element without reallocation") {
        auto it = values.insert(values.begin() + 2, 3);
        CHECK(*it == 3);
        it = values.emplace(values.begin() + 3, 4);
        CHECK(*it == 4);
        CHECK(values == DynamicArray<int>{1, 2, 3, 4, 5});
        values.insert(values.begin(), 0);
        values.insert(values.end(), 6);
        CHECK(values == DynamicArray<int>{0, 1, 2, 3, 4, 5, 6});
    }
    SECTION("with reallocation") {
        values.shrink_to_fit();
        REQUIRE(values.capacity() == 3);
        values.insert(values.begin() + 1, 9);
        CHECK(values == DynamicArray<int>{1, 9, 2, 5});
    }
    SECTION("count copies and ranges") {
        values.insert(values.begin() + 1, 2, 7);
        CHECK(values == DynamicArray<int>{1, 7, 7, 2, 5});
        const std::vector<int> extra{8, 9};
        const auto it = values.insert(values.end(), extra.begin(), extra.end());
        CHECK(*it == 8);
        CHECK(values == DynamicArray<int>{1, 7, 7, 2, 5, 8, 9});
    }
    SECTION("aliasing value in count insert") {
        values.insert(values.begin(), 3, values[2]);
        CHECK(values == DynamicArray<int>{5, 5, 5, 1, 2, 5});
    }
}

TEST_CASE("DynamicArray erase ranges and single elements", "[templates][containers][dynamic_array]") {
    DynamicArray<std::string> words{"a", "b", "c", "d", "e"};
    auto it = words.erase(words.begin() + 1);
    CHECK(*it == "c");
    CHECK(words.size() == 4);
    it = words.erase(words.begin() + 1, words.begin() + 3);
    CHECK(*it == "e");
    CHECK(words == DynamicArray<std::string>{"a", "e"});
    it = words.erase(words.begin(), words.begin());
    CHECK(words.size() == 2);
    words.erase(words.begin(), words.end());
    CHECK(words.empty());
}

TEST_CASE("DynamicArray resize, clear, reserve and shrink_to_fit", "[templates][containers][dynamic_array]") {
    DynamicArray<int> values{1, 2, 3};
    values.resize(5);
    CHECK(values == DynamicArray<int>{1, 2, 3, 0, 0});
    values.resize(7, 9);
    CHECK(values.back() == 9);
    values.resize(2);
    CHECK(values == DynamicArray<int>{1, 2});
    values.reserve(100);
    CHECK(values.capacity() >= 100);
    values.shrink_to_fit();
    CHECK(values.capacity() == 2);
    values.clear();
    CHECK(values.empty());
    CHECK(values.capacity() == 2);
    CHECK_THROWS_AS(values.reserve(values.max_size() + 1), std::length_error);
}

TEST_CASE("DynamicArray copy and move semantics", "[templates][containers][dynamic_array]") {
    Tracked::live = 0;
    {
        DynamicArray<Tracked> original;
        for (int i = 0; i < 5; ++i) {
            original.emplace_back(i);
        }
        DynamicArray<Tracked> copy = original;
        CHECK(copy == original);
        CHECK(Tracked::live == 10);

        DynamicArray<Tracked> moved = std::move(copy);
        CHECK(moved.size() == 5);
        CHECK(copy.empty()); // NOLINT(bugprone-use-after-move): moved-from is guaranteed empty here
        CHECK(Tracked::live == 10);

        DynamicArray<Tracked> assigned;
        assigned = original;
        CHECK(assigned == original);
        assigned = std::move(moved);
        CHECK(Tracked::live == 10);
        const auto& alias = assigned;
        assigned = alias; // self-assignment is a no-op
        CHECK(assigned.size() == 5);
        assigned = {Tracked(42)};
        CHECK(assigned.size() == 1);
        CHECK(assigned[0].value == 42);
    }
    CHECK(Tracked::live == 0);
}

TEST_CASE("DynamicArray comparison operators", "[templates][containers][dynamic_array]") {
    const DynamicArray<int> a{1, 2, 3};
    const DynamicArray<int> b{1, 2, 4};
    const DynamicArray<int> c{1, 2};
    CHECK(a == a);
    CHECK(a != b);
    CHECK(a < b);
    CHECK(c < a);
    CHECK(b > a);
    CHECK((a <=> DynamicArray<int>{1, 2, 3}) == std::strong_ordering::equal);
}

TEST_CASE("DynamicArray iterators work with standard algorithms", "[templates][containers][dynamic_array]") {
    std::mt19937 rng(12345);
    DynamicArray<int> values;
    for (int i = 0; i < 200; ++i) {
        values.push_back(static_cast<int>(rng() % 1000));
    }
    std::sort(values.begin(), values.end());
    CHECK(std::is_sorted(values.begin(), values.end()));
    std::ranges::reverse(values);
    CHECK(std::is_sorted(values.rbegin(), values.rend()));

    const DynamicArray<int>& cref = values;
    DynamicArray<int>::const_iterator cit = values.begin(); // iterator -> const_iterator
    CHECK(cit == cref.begin());
    CHECK(cref.end() - cit == 200);
    CHECK(std::accumulate(cref.begin(), cref.end(), 0L) == std::accumulate(values.begin(), values.end(), 0L));

    auto it = values.begin();
    it += 5;
    CHECK(it - values.begin() == 5);
    CHECK(*(it - 1) == values[4]);
    CHECK(it[1] == values[6]);
    CHECK(std::to_address(it) == values.data() + 5);
}

TEST_CASE("DynamicArray gives the strong guarantee when a copy throws during growth", "[templates][containers][dynamic_array]") {
    ThrowOnCopy::live = 0;
    {
        DynamicArray<ThrowOnCopy> values;
        values.reserve(3);
        values.emplace_back(1);
        values.emplace_back(2);
        values.emplace_back(3);
        REQUIRE(values.size() == values.capacity());

        ThrowOnCopy::copies_until_throw = 1; // second copy of the reallocation fails
        CHECK_THROWS_AS(values.emplace_back(4), std::runtime_error);
        ThrowOnCopy::copies_until_throw = -1;

        CHECK(values.size() == 3); // unchanged
        CHECK(values.capacity() == 3);
        CHECK(values[0].value == 1);
        CHECK(values[2].value == 3);
        CHECK(ThrowOnCopy::live == 3); // nothing leaked
    }
    CHECK(ThrowOnCopy::live == 0);
}

TEST_CASE("DynamicArray copy constructor cleans up when an element copy throws", "[templates][containers][dynamic_array]") {
    ThrowOnCopy::live = 0;
    {
        DynamicArray<ThrowOnCopy> values;
        values.reserve(4);
        for (int i = 0; i < 4; ++i) {
            values.emplace_back(i);
        }
        ThrowOnCopy::copies_until_throw = 2;
        CHECK_THROWS_AS(DynamicArray<ThrowOnCopy>(values), std::runtime_error);
        ThrowOnCopy::copies_until_throw = -1;
        CHECK(ThrowOnCopy::live == 4);
    }
    CHECK(ThrowOnCopy::live == 0);
}

TEST_CASE("DynamicArray honours stateful allocators", "[templates][containers][dynamic_array]") {
    CountingAllocator<int> alloc_a(1);
    CountingAllocator<int> alloc_b(2);
    {
        DynamicArray<int, CountingAllocator<int>> a(alloc_a);
        for (int i = 0; i < 10; ++i) {
            a.push_back(i);
        }
        CHECK(*alloc_a.outstanding == static_cast<long>(a.capacity()));

        DynamicArray<int, CountingAllocator<int>> b(alloc_b);
        b = std::move(a); // unequal, non-propagating: element-wise move into b's allocator
        CHECK(b.get_allocator().id == 2);
        CHECK(b.size() == 10);
        CHECK(*alloc_b.outstanding == static_cast<long>(b.capacity()));

        DynamicArray<int, CountingAllocator<int>> c(alloc_a);
        c = b; // copy assignment keeps c's allocator
        CHECK(c.get_allocator().id == 1);
        CHECK(c == b);
    }
    CHECK(*alloc_a.outstanding == 0);
    CHECK(*alloc_b.outstanding == 0);
}

TEST_CASE("UniquePtr ownership transfer and reset", "[templates][smart_pointers]") {
    Tracked::live = 0;
    {
        auto p = make_unique_ptr<Tracked>(7);
        REQUIRE(p);
        CHECK(p->value == 7);
        CHECK((*p).value == 7);

        UniquePtr<Tracked> q = std::move(p);
        CHECK_FALSE(p); // NOLINT(bugprone-use-after-move)
        CHECK(p == nullptr);
        CHECK(q->value == 7);

        q.reset(new Tracked(8));
        CHECK(Tracked::live == 1);
        Tracked* raw = q.release();
        CHECK_FALSE(q);
        CHECK(Tracked::live == 1);
        q.reset(raw);
        q = nullptr;
        CHECK(Tracked::live == 0);

        UniquePtr<Tracked> a = make_unique_ptr<Tracked>(1);
        UniquePtr<Tracked> b = make_unique_ptr<Tracked>(2);
        a.swap(b);
        CHECK(a->value == 2);
        CHECK(b->value == 1);
    }
    CHECK(Tracked::live == 0);
}

TEST_CASE("UniquePtr custom deleters, conversions and arrays", "[templates][smart_pointers]") {
    int deletions = 0;
    {
        auto deleter = [&deletions](int* p) {
            ++deletions;
            delete p;
        };
        UniquePtr<int, decltype(deleter)> p(new int(3), deleter);
        CHECK(*p == 3);
    }
    CHECK(deletions == 1);

    UniquePtr<Base> base = make_unique_ptr<Derived>();
    CHECK(base->id() == 2);

    auto array = make_unique_ptr<int[]>(5);
    for (std::size_t i = 0; i < 5; ++i) {
        CHECK(array[i] == 0); // value-initialised
        array[i] = static_cast<int>(i * i);
    }
    CHECK(array[4] == 16);
    UniquePtr<int[]> moved = std::move(array);
    CHECK_FALSE(array); // NOLINT(bugprone-use-after-move)
    CHECK(moved[3] == 9);
}

TEST_CASE("SharedPtr reference counting", "[templates][smart_pointers]") {
    Tracked::live = 0;
    {
        auto a = make_shared_ptr<Tracked>(5);
        CHECK(a.use_count() == 1);
        {
            SharedPtr<Tracked> b = a;
            SharedPtr<Tracked> c;
            c = b;
            CHECK(a.use_count() == 3);
            CHECK(c == a);
            SharedPtr<Tracked> d = std::move(c);
            CHECK(a.use_count() == 3);
            CHECK(c == nullptr); // NOLINT(bugprone-use-after-move)
        }
        CHECK(a.use_count() == 1);
        CHECK(Tracked::live == 1);
        a.reset(new Tracked(6));
        CHECK(a->value == 6);
        CHECK(Tracked::live == 1);
    }
    CHECK(Tracked::live == 0);
}

TEST_CASE("SharedPtr custom deleter, UniquePtr adoption, conversions and aliasing", "[templates][smart_pointers]") {
    int deletions = 0;
    {
        SharedPtr<int> p(new int(1), [&deletions](int* raw) {
            ++deletions;
            delete raw;
        });
        SharedPtr<int> q = p;
        CHECK(q.use_count() == 2);
    }
    CHECK(deletions == 1);

    SharedPtr<Base> base = SharedPtr<Derived>(new Derived);
    CHECK(base->id() == 2);

    SharedPtr<Tracked> adopted = make_unique_ptr<Tracked>(9);
    CHECK(adopted->value == 9);
    CHECK(adopted.use_count() == 1);

    struct Pair {
        int first = 1;
        int second = 2;
    };
    auto owner = make_shared_ptr<Pair>();
    SharedPtr<int> member(owner, &owner->second);
    CHECK(*member == 2);
    CHECK(owner.use_count() == 2);
    owner.reset();
    CHECK(*member == 2); // aliasing pointer keeps the whole Pair alive
    CHECK(member.use_count() == 1);
}

TEST_CASE("WeakPtr observes without owning and lock() is safe", "[templates][smart_pointers]") {
    WeakPtr<std::string> weak;
    CHECK(weak.expired());
    CHECK_FALSE(weak.lock());
    {
        auto strong = make_shared_ptr<std::string>("alive");
        weak = strong;
        CHECK(weak.use_count() == 1);
        auto locked = weak.lock();
        REQUIRE(locked);
        CHECK(*locked == "alive");
        CHECK(strong.use_count() == 2);
        WeakPtr<std::string> copy = weak;
        CHECK_FALSE(copy.expired());
    }
    CHECK(weak.expired());
    CHECK_FALSE(weak.lock());
    weak.reset();
    CHECK(weak.use_count() == 0);
}

TEST_CASE("SharedPtr counts are thread-safe under concurrent copies", "[templates][smart_pointers][threads]") {
    Tracked::live = 0;
    {
        auto shared = make_shared_ptr<Tracked>(1);
        WeakPtr<Tracked> weak = shared;
        std::vector<std::thread> threads;
        for (int t = 0; t < 4; ++t) {
            threads.emplace_back([shared, weak] {
                for (int i = 0; i < 2000; ++i) {
                    SharedPtr<Tracked> copy = shared;
                    auto locked = weak.lock();
                    (void)copy;
                    (void)locked;
                }
            });
        }
        for (auto& thread : threads) {
            thread.join();
        }
        CHECK(shared.use_count() == 1);
        CHECK(Tracked::live == 1);
    }
    CHECK(Tracked::live == 0);
}

TEST_CASE("Optional engagement, access and assignment", "[templates][optional]") {
    Optional<std::string> empty;
    CHECK_FALSE(empty);
    CHECK_FALSE(empty.has_value());
    CHECK(empty == std::nullopt);
    CHECK_THROWS_AS(empty.value(), std::bad_optional_access);
    CHECK(empty.value_or("fallback") == "fallback");

    Optional<std::string> name = std::string("Ada");
    REQUIRE(name);
    CHECK(*name == "Ada");
    CHECK(name->size() == 3);
    CHECK(name == std::string("Ada"));

    empty = name;
    CHECK(empty == name);
    name = std::nullopt;
    CHECK_FALSE(name);
    CHECK(empty != name);

    Optional<std::string> in_place(std::in_place, 3, 'q');
    CHECK(in_place.value() == "qqq");
    in_place.emplace("new");
    CHECK(*in_place == "new");

    Optional<std::string> moved = std::move(in_place);
    CHECK(moved.value() == "new");
}

TEST_CASE("Optional lifetime management, swap and monadic operations", "[templates][optional]") {
    Tracked::live = 0;
    {
        Optional<Tracked> a(Tracked(1));
        Optional<Tracked> b;
        CHECK(Tracked::live == 1);
        a.swap(b);
        CHECK_FALSE(a);
        CHECK(b->value == 1);
        CHECK(Tracked::live == 1);
        a.emplace(2);
        a.swap(b);
        CHECK(a->value == 1);
        CHECK(b->value == 2);
        b.reset();
        CHECK(Tracked::live == 1);
    }
    CHECK(Tracked::live == 0);

    const Optional<int> five = 5;
    const auto text = five.transform([](int v) { return std::to_string(v); });
    CHECK(text.value() == "5");
    const auto half = [](int v) { return v % 2 == 0 ? Optional<int>(v / 2) : Optional<int>(); };
    CHECK_FALSE(five.and_then(half));
    CHECK(Optional<int>(8).and_then(half).value() == 4);
    CHECK_FALSE(Optional<int>().transform([](int v) { return v; }));

    Optional<std::unique_ptr<int>> move_only(std::make_unique<int>(3));
    Optional<std::unique_ptr<int>> target;
    target = std::move(move_only);
    CHECK(**target == 3);
}
