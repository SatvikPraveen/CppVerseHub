/**
 * @file StateTests.cpp
 * @brief Tests for the OO mission state machine and the std::variant warp-drive machine.
 */

#include "patterns/State.hpp"

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

using namespace CppVerseHub::Patterns;

TEST_CASE("Mission follows the happy path to Completed", "[state][oo]") {
    MissionContext m("Survey");
    CHECK(m.phase() == MissionPhase::Pending);
    CHECK(m.plan());
    CHECK(m.launch());
    CHECK(m.advance(50.0));
    CHECK(m.phase() == MissionPhase::Active);
    CHECK(m.progress() == 50.0);
    CHECK(m.advance(75.0));
    CHECK(m.phase() == MissionPhase::Completed);
    CHECK(m.progress() == 100.0); // clamped
    CHECK(m.finished());
    REQUIRE(m.history().size() == 3);
    CHECK(m.history()[2].from == MissionPhase::Active);
    CHECK(m.history()[2].to == MissionPhase::Completed);
    CHECK(m.history()[2].trigger == "advance");
}

TEST_CASE("Illegal events are rejected without changing state", "[state][oo]") {
    MissionContext m("Probe");
    CHECK_FALSE(m.launch());
    CHECK_FALSE(m.pause());
    CHECK_FALSE(m.resume());
    CHECK_FALSE(m.advance(10.0));
    CHECK_FALSE(m.fail("x"));
    CHECK(m.phase() == MissionPhase::Pending);
    CHECK(m.rejectedEvents() == 5);
    CHECK(m.history().empty());
}

TEST_CASE("Pause and resume preserve progress", "[state][oo]") {
    MissionContext m("Patrol");
    m.plan();
    m.launch();
    m.advance(30.0);
    CHECK(m.pause());
    CHECK(m.phase() == MissionPhase::Paused);
    CHECK_FALSE(m.advance(10.0));
    CHECK(m.progress() == 30.0);
    CHECK(m.resume());
    CHECK(m.phase() == MissionPhase::Active);
    CHECK_FALSE(m.advance(0.0)); // non-positive progress rejected
    CHECK_FALSE(m.advance(-5.0));
}

TEST_CASE("Failure records a reason and is terminal", "[state][oo]") {
    MissionContext m("Raid");
    m.plan();
    m.launch();
    const bool fromPaused = GENERATE(false, true);
    if (fromPaused) {
        m.pause();
    }
    CHECK(m.fail("ambushed"));
    CHECK(m.phase() == MissionPhase::Failed);
    CHECK(m.failureReason() == "ambushed");
    CHECK(m.finished());
    CHECK_FALSE(m.abort());
    CHECK_FALSE(m.resume());
}

TEST_CASE("Abort is accepted from every non-terminal phase", "[state][oo]") {
    const int steps = GENERATE(0, 1, 2, 3);
    MissionContext m("Any");
    if (steps >= 1)
        m.plan();
    if (steps >= 2)
        m.launch();
    if (steps >= 3)
        m.pause();
    CHECK(m.abort());
    CHECK(m.phase() == MissionPhase::Aborted);
    CHECK_FALSE(m.abort());
}

TEST_CASE("toString covers every MissionPhase", "[state]") {
    CHECK(toString(MissionPhase::Pending) == "Pending");
    CHECK(toString(MissionPhase::Planning) == "Planning");
    CHECK(toString(MissionPhase::Active) == "Active");
    CHECK(toString(MissionPhase::Paused) == "Paused");
    CHECK(toString(MissionPhase::Completed) == "Completed");
    CHECK(toString(MissionPhase::Failed) == "Failed");
    CHECK(toString(MissionPhase::Aborted) == "Aborted");
}

TEST_CASE("WarpDrive charges, jumps and cools down", "[state][variant]") {
    WarpDrive drive;
    CHECK(drive.is<Warp::Offline>());
    CHECK(drive.handle(Warp::PowerOn{}));
    CHECK(drive.handle(Warp::Charge{40.0}));
    REQUIRE(drive.is<Warp::Charging>());
    CHECK(std::get<Warp::Charging>(drive.state()).percent == 40.0);
    CHECK(drive.handle(Warp::Charge{60.0}));
    CHECK(drive.is<Warp::Ready>());
    CHECK(drive.handle(Warp::Engage{"Vega"}));
    CHECK(drive.is<Warp::Jumping>());
    CHECK(drive.jumps() == 1);
    CHECK(drive.log() == std::vector<std::string>{"Vega"});
    CHECK(drive.handle(Warp::Tick{}));
    REQUIRE(drive.is<Warp::Cooldown>());
    CHECK(std::get<Warp::Cooldown>(drive.state()).ticksRemaining == Warp::kCooldownTicks);
    for (int i = 0; i < Warp::kCooldownTicks; ++i) {
        CHECK(drive.handle(Warp::Tick{}));
    }
    CHECK(drive.is<Warp::Charging>());
    CHECK(drive.stateName() == "Charging");
}

TEST_CASE("WarpDrive rejects invalid events", "[state][variant]") {
    WarpDrive drive;
    CHECK_FALSE(drive.handle(Warp::Charge{10.0})); // offline
    CHECK_FALSE(drive.handle(Warp::Shutdown{}));   // already offline
    drive.handle(Warp::PowerOn{});
    CHECK_FALSE(drive.handle(Warp::Engage{"X"})); // not charged
    CHECK_FALSE(drive.handle(Warp::Charge{0.0})); // non-positive charge
    CHECK_FALSE(drive.handle(Warp::Charge{-5.0}));
    drive.handle(Warp::Charge{100.0});
    CHECK_FALSE(drive.handle(Warp::Engage{""})); // empty destination
    drive.handle(Warp::Engage{"Y"});
    CHECK_FALSE(drive.handle(Warp::Shutdown{})); // cannot shut down mid-jump
    CHECK(drive.is<Warp::Jumping>());
    CHECK(drive.rejected() == 7);
}

TEST_CASE("Shutdown returns to Offline from every interruptible state", "[state][variant]") {
    WarpDrive drive;
    drive.handle(Warp::PowerOn{});
    const int stage = GENERATE(0, 1, 2); // Charging, Ready, Cooldown
    if (stage >= 1)
        drive.handle(Warp::Charge{100.0});
    if (stage >= 2) {
        drive.handle(Warp::Engage{"Z"});
        drive.handle(Warp::Tick{});
    }
    CHECK(drive.handle(Warp::Shutdown{}));
    CHECK(drive.is<Warp::Offline>());
}

TEST_CASE("stateName maps every variant alternative", "[state][variant]") {
    CHECK(stateName(Warp::Offline{}) == "Offline");
    CHECK(stateName(Warp::Charging{}) == "Charging");
    CHECK(stateName(Warp::Ready{}) == "Ready");
    CHECK(stateName(Warp::Jumping{}) == "Jumping");
    CHECK(stateName(Warp::Cooldown{}) == "Cooldown");
}

namespace {
// A second, tiny machine to show VariantStateMachine is generic.
struct Locked {};
struct Unlocked {
    int coins = 0;
};
struct Coin {};
struct Push {};
using TurnState = std::variant<Locked, Unlocked>;
using TurnEvent = std::variant<Coin, Push>;
struct TurnTransitions {
    std::optional<TurnState> operator()(const Locked&, const Coin&) const { return Unlocked{1}; }
    std::optional<TurnState> operator()(const Unlocked& u, const Coin&) const {
        return Unlocked{u.coins + 1};
    }
    std::optional<TurnState> operator()(const Unlocked&, const Push&) const { return Locked{}; }
    template <typename S, typename E>
    std::optional<TurnState> operator()(const S&, const E&) const {
        return std::nullopt;
    }
};
} // namespace

TEST_CASE("VariantStateMachine is reusable for arbitrary state/event sets", "[state][variant][generic]") {
    VariantStateMachine<TurnState, TurnEvent, TurnTransitions> turnstile{Locked{}};
    CHECK_FALSE(turnstile.dispatch(Push{}));
    CHECK(turnstile.dispatch(Coin{}));
    CHECK(turnstile.dispatch(Coin{}));
    REQUIRE(turnstile.getIf<Unlocked>() != nullptr);
    CHECK(turnstile.getIf<Unlocked>()->coins == 2);
    CHECK(turnstile.getIf<Locked>() == nullptr);
    CHECK(turnstile.dispatch(Push{}));
    CHECK(turnstile.is<Locked>());
    CHECK(turnstile.acceptedCount() == 3);
    CHECK(turnstile.rejectedCount() == 1);
}

TEST_CASE("Overloaded builds a visitor from lambdas", "[state][variant]") {
    const std::variant<int, std::string> v = std::string("abc");
    const auto size = std::visit(
        Overloaded{[](int) { return std::size_t{0}; }, [](const std::string& s) { return s.size(); }}, v);
    CHECK(size == 3);
}
