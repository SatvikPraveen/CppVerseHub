#include "modern/MoveSemantics.hpp"

#include <catch2/catch_test_macros.hpp>

#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

using namespace CppVerseHub::Modern::MoveSemantics;

namespace {

/// Throws on the N-th copy; its move constructor is potentially-throwing, so containers must copy it.
struct FragileCopy {
    static inline int copiesUntilThrow = -1;
    int value;
    explicit FragileCopy(int v) : value(v) {}
    FragileCopy(const FragileCopy& o) : value(o.value) {
        if (copiesUntilThrow == 0) {
            throw std::runtime_error("copy failed");
        }
        if (copiesUntilThrow > 0) {
            --copiesUntilThrow;
        }
    }
    FragileCopy(FragileCopy&& o) noexcept(false) : value(o.value) {}
    FragileCopy& operator=(const FragileCopy&) = default;
    FragileCopy& operator=(FragileCopy&&) = default;
    ~FragileCopy() = default;
};

} // namespace

TEST_CASE("TrackedResource copy is deep and counted", "[modern][move]") {
    OperationCounts c;
    TrackedResource a("a", 10, &c);
    TrackedResource b = a;
    CHECK(c.constructions == 1);
    CHECK(c.copies == 1);
    CHECK(c.moves == 0);
    b.append(100);
    CHECK(a.size() == 10);
    CHECK(b.size() == 11);
    CHECK(a.checksum() == 45);
    CHECK(b.checksum() == 145);
}

TEST_CASE("TrackedResource move steals state and leaves source empty", "[modern][move]") {
    OperationCounts c;
    TrackedResource a("ore", 5, &c);
    TrackedResource b = std::move(a);
    CHECK(c.moves == 1);
    CHECK(c.copies == 0);
    CHECK(a.isMovedFrom()); // NOLINT(bugprone-use-after-move)
    CHECK(a.name().empty());
    CHECK(a.size() == 0);
    CHECK(b.name() == "ore");
    CHECK(b.checksum() == 10);
    a.append(3); // a moved-from object is still usable
    CHECK(a.size() == 1);
}

TEST_CASE("TrackedResource assignment operators", "[modern][move]") {
    OperationCounts c;
    TrackedResource a("a", 3, &c);
    TrackedResource b("b", 1, &c);
    b = a;
    CHECK(b.name() == "a");
    CHECK(b.checksum() == 3);
    CHECK(c.copies == 1);
    TrackedResource d("d", 2, &c);
    d = std::move(b);
    CHECK(d.name() == "a");
    CHECK(b.isMovedFrom()); // NOLINT(bugprone-use-after-move)
    CHECK(c.moves == 1);
    const TrackedResource& alias = d;
    d = alias; // self-assignment is safe
    CHECK(d.name() == "a");
}

TEST_CASE("TrackedResource destructions balance constructions", "[modern][move]") {
    OperationCounts c;
    {
        std::vector<TrackedResource> v;
        for (int i = 0; i < 5; ++i) {
            v.emplace_back("r", 1, &c);
        }
        auto copy = v;
        auto moved = std::move(copy);
    }
    CHECK(c.destructions == c.constructions + c.copies + c.moves); // no assignments happened
    CHECK(c.copies == 5);
}

TEST_CASE("std::vector moves noexcept-movable elements on growth", "[modern][move]") {
    OperationCounts c;
    std::vector<TrackedResource> v;
    for (int i = 0; i < 33; ++i) {
        v.emplace_back("r", 1, &c);
    }
    CHECK(c.copies == 0);
    CHECK(c.moves > 0);
}

TEST_CASE("Spacecraft is move-only and transfers ownership", "[modern][move]") {
    STATIC_CHECK_FALSE(std::is_copy_constructible_v<Spacecraft>);
    STATIC_CHECK_FALSE(std::is_copy_assignable_v<Spacecraft>);
    OperationCounts c;
    Spacecraft s(1, "Scout");
    s.log("a");
    s.log("b");
    TrackedResource item("fuel", 4, &c);
    s.loadCargo(item);
    s.loadCargo(std::move(item));
    s.emplaceCargo("water", std::size_t{2}, &c);
    CHECK(s.cargoCount() == 3);
    CHECK(c.copies == 1); // only the lvalue load copied

    Spacecraft t = std::move(s);
    CHECK(t.id() == 1);
    CHECK(t.logSize() == 2);
    CHECK(t.cargoCount() == 3);
    CHECK(s.id() == -1); // NOLINT(bugprone-use-after-move)
    CHECK_FALSE(s.isValid());
    s.log("revived");
    CHECK(s.logSize() == 1);

    Spacecraft u(2, "Other");
    u = std::move(t);
    CHECK(u.id() == 1);
    CHECK(u.unloadCargo().name() == "water");
    CHECK(u.cargoCount() == 2);
}

TEST_CASE("Spacecraft::unloadCargo throws when empty", "[modern][move]") {
    Spacecraft s(3, "Empty");
    CHECK_THROWS_AS(s.unloadCargo(), std::out_of_range);
}

TEST_CASE("categoryOf reports the value category", "[modern][move]") {
    std::string s = "x";
    const std::string cs = "y";
    CHECK(categoryOf(s) == ValueCategory::LValue);
    CHECK(categoryOf(cs) == ValueCategory::ConstLValue);
    CHECK(categoryOf(std::move(s)) == ValueCategory::RValue);
    CHECK(categoryOf(std::string("t")) == ValueCategory::RValue);
    CHECK(categoryOf(42) == ValueCategory::RValue);
    CHECK(toString(ValueCategory::ConstLValue) == "const lvalue");
}

TEST_CASE("std::forward preserves value category; plain names do not", "[modern][move]") {
    std::string s = "x";
    CHECK(relayForwarded(s) == "copy");
    CHECK(relayForwarded(std::move(s)) == "move");
    CHECK(relayWithoutForward(std::string("t")) == "copy");
    CHECK(relayWithoutForward(s) == "copy");
}

TEST_CASE("forwarding helpers", "[modern][move]") {
    CHECK(forwardTo([](int a, int b) { return a - b; }, 10, 4) == 6);
    int target = 0;
    forwardTo([](int& r) { r = 9; }, target);
    CHECK(target == 9);
    auto p = makeUniqueForwarded<std::vector<int>>(3, 1);
    CHECK(*p == std::vector<int>{1, 1, 1});
}

TEST_CASE("prvalue returns are elided", "[modern][move]") {
    OperationCounts c;
    {
        TrackedResource r = makeResource(&c);
        CHECK(r.size() == 16);
    }
    CHECK(c == OperationCounts{1, 0, 0, 1});
}

TEST_CASE("makeCallSign sink argument", "[modern][move]") {
    std::string base = "Hawk";
    CHECK(makeCallSign(base) == "Hawk-01");
    CHECK(base == "Hawk");
    CHECK(makeCallSign(std::move(base)) == "Hawk-01");
}

TEST_CASE("MoveAwareVector basic operations", "[modern][move]") {
    MoveAwareVector<std::string> v;
    CHECK(v.empty());
    v.push_back("a");
    std::string b = "b";
    v.push_back(b);
    v.emplace_back(3, 'c');
    REQUIRE(v.size() == 3);
    CHECK(v[0] == "a");
    CHECK(v.at(2) == "ccc");
    CHECK(b == "b");
    CHECK_THROWS_AS(v.at(3), std::out_of_range);
    v.pop_back();
    CHECK(v.size() == 2);
    std::string joined;
    for (const auto& s : v) {
        joined += s;
    }
    CHECK(joined == "ab");
    v.clear();
    CHECK(v.empty());
    CHECK(v.capacity() >= 2);
    CHECK_THROWS_AS(v.pop_back(), std::out_of_range);
}

TEST_CASE("MoveAwareVector grows geometrically", "[modern][move]") {
    MoveAwareVector<int> v;
    for (int i = 0; i < 100; ++i) {
        v.push_back(i);
    }
    CHECK(v.size() == 100);
    CHECK(v.capacity() == 128);
    CHECK(v.reallocations() == 8); // 1,2,4,...,128
    CHECK(v[99] == 99);
    MoveAwareVector<int> r;
    r.reserve(50);
    CHECK(r.capacity() == 50);
    r.reserve(10);
    CHECK(r.capacity() == 50);
}

TEST_CASE("MoveAwareVector copy, move and swap", "[modern][move]") {
    MoveAwareVector<std::string> a{"x", "y", "z"};
    MoveAwareVector<std::string> b = a;
    b[0] = "changed";
    CHECK(a[0] == "x");
    MoveAwareVector<std::string> c = std::move(a);
    CHECK(c.size() == 3);
    CHECK(a.empty()); // NOLINT(bugprone-use-after-move)
    CHECK(a.capacity() == 0);
    a = c;
    CHECK(a.size() == 3);
    b = std::move(c);
    CHECK(b[2] == "z");
    a.swap(b);
    CHECK(a[0] == "x");
}

TEST_CASE("MoveAwareVector uses move_if_noexcept on reallocation", "[modern][move]") {
    OperationCounts c;
    {
        MoveAwareVector<TrackedResource> v;
        for (int i = 0; i < 9; ++i) {
            v.emplace_back("r", 1, &c);
        }
        CHECK(v.reallocations() == 5); // capacities 1,2,4,8,16
    }
    CHECK(c.copies == 0);
    CHECK(c.moves == 1 + 2 + 4 + 8);
    CHECK(c.destructions == c.constructions + c.moves);
}

TEST_CASE("MoveAwareVector keeps the strong guarantee when growth throws", "[modern][move]") {
    MoveAwareVector<FragileCopy> v;
    FragileCopy::copiesUntilThrow = -1;
    for (int i = 0; i < 4; ++i) {
        v.emplace_back(i);
    }
    REQUIRE(v.capacity() == 4);
    FragileCopy::copiesUntilThrow = 2; // the third copy during reallocation throws
    CHECK_THROWS_AS(v.emplace_back(99), std::runtime_error);
    FragileCopy::copiesUntilThrow = -1;
    REQUIRE(v.size() == 4);
    CHECK(v.capacity() == 4);
    for (int i = 0; i < 4; ++i) {
        CHECK(v[static_cast<std::size_t>(i)].value == i);
    }
}

TEST_CASE("MoveAwareVector handles aliasing push_back during growth", "[modern][move]") {
    MoveAwareVector<std::string> v{"alpha"};
    REQUIRE(v.size() == v.capacity());
    v.push_back(v[0]); // argument refers into the buffer that is about to be reallocated
    CHECK(v[1] == "alpha");
    v.emplace_back(v[1]);
    CHECK(v[2] == "alpha");
}

TEST_CASE("move semantics showcases write to the stream", "[modern][move]") {
    std::ostringstream os;
    demonstrateAllMoveSemantics(os);
    const auto text = os.str();
    CHECK(text.find("relayWithoutForward(rvalue)   -> copy") != std::string::npos);
    CHECK(text.find("prvalue return: constructions=1 copies=0 moves=0 destructions=1") != std::string::npos);
    CHECK(text.find("scout valid=false") != std::string::npos);
}
