/**
 * @file CommandTests.cpp
 * @brief Tests for commands, macro transactions and the bounded undo/redo history.
 */

#include "patterns/Command.hpp"

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

using namespace CppVerseHub::Patterns;

namespace {
FleetStatus defaultFleet() {
    return FleetStatus{"Alpha", "Sol", 100.0, 100.0, 5};
}

/// Command that appends to a log and can be told to fail.
class LogCommand final : public ICommand {
public:
    LogCommand(std::vector<std::string>& log, std::string tag, bool fail = false)
        : log_(&log), tag_(std::move(tag)), fail_(fail) {}
    void execute() override {
        if (fail_) {
            throw CommandError("fail " + tag_);
        }
        log_->push_back("+" + tag_);
    }
    void undo() override { log_->push_back("-" + tag_); }
    [[nodiscard]] std::string name() const override { return tag_; }

private:
    std::vector<std::string>* log_;
    std::string tag_;
    bool fail_;
};
} // namespace

TEST_CASE("MoveFleetCommand moves, burns fuel, and undo restores both", "[command]") {
    FleetReceiver fleet(defaultFleet());
    MoveFleetCommand move(fleet, "Vega", 30.0);
    move.execute();
    CHECK(fleet.status().location == "Vega");
    CHECK(fleet.status().fuel == 70.0);
    move.undo();
    CHECK(fleet.status() == defaultFleet());
    CHECK(move.name() == "Move to Vega");
}

TEST_CASE("Failed commands leave the receiver untouched (strong guarantee)", "[command]") {
    FleetReceiver fleet(defaultFleet());
    SECTION("insufficient fuel") {
        MoveFleetCommand move(fleet, "Far", 1000.0);
        CHECK_THROWS_AS(move.execute(), CommandError);
    }
    SECTION("negative fuel cost") {
        MoveFleetCommand move(fleet, "Far", -1.0);
        CHECK_THROWS_AS(move.execute(), CommandError);
    }
    SECTION("detaching too many ships") {
        ReinforceCommand detach(fleet, -6);
        CHECK_THROWS_AS(detach.execute(), CommandError);
    }
    CHECK(fleet.status() == defaultFleet());
}

TEST_CASE("AttackCommand applies damage and combat fuel; undo rewinds", "[command]") {
    FleetReceiver fleet(defaultFleet());
    AttackCommand attack(fleet, "Outpost", 40.0);
    attack.execute();
    CHECK(fleet.status().health == 60.0);
    CHECK(fleet.status().fuel == 100.0 - FleetReceiver::kCombatFuel);
    attack.undo();
    CHECK(fleet.status() == defaultFleet());
}

TEST_CASE("Damaged fleets refuse to attack or move", "[command]") {
    FleetReceiver fleet(FleetStatus{"Wreck", "Sol", 100.0, 25.0, 1});
    CHECK_THROWS_AS(fleet.attack("x", 1.0), CommandError);
    CHECK_NOTHROW(fleet.move("Luna", 1.0));
    FleetReceiver crippled(FleetStatus{"Hulk", "Sol", 100.0, 10.0, 1});
    CHECK_THROWS_AS(crippled.move("Luna", 1.0), CommandError);
}

TEST_CASE("ReinforceCommand names and inverse", "[command]") {
    FleetReceiver fleet(defaultFleet());
    const int delta = GENERATE(-3, 0, 4);
    ReinforceCommand cmd(fleet, delta);
    cmd.execute();
    CHECK(fleet.status().ships == 5 + delta);
    cmd.undo();
    CHECK(fleet.status().ships == 5);
    CHECK_FALSE(cmd.name().empty());
}

TEST_CASE("LambdaCommand requires both callables and calls them", "[command]") {
    int value = 0;
    CHECK_THROWS_AS(LambdaCommand("bad", nullptr, [] {}), std::invalid_argument);
    LambdaCommand cmd("inc", [&] { ++value; }, [&] { --value; });
    cmd.execute();
    cmd.execute();
    CHECK(value == 2);
    cmd.undo();
    CHECK(value == 1);
    CHECK(cmd.name() == "inc");
}

TEST_CASE("MacroCommand executes in order and undoes in reverse", "[command][macro]") {
    std::vector<std::string> log;
    MacroCommand macro("m");
    macro.add(std::make_unique<LogCommand>(log, "a")).add(std::make_unique<LogCommand>(log, "b"));
    CHECK(macro.size() == 2);
    macro.execute();
    macro.undo();
    CHECK(log == std::vector<std::string>{"+a", "+b", "-b", "-a"});
    CHECK_THROWS_AS(macro.add(nullptr), std::invalid_argument);
}

TEST_CASE("MacroCommand rolls back completed children on failure", "[command][macro]") {
    std::vector<std::string> log;
    MacroCommand macro("tx");
    macro.add(std::make_unique<LogCommand>(log, "a"))
        .add(std::make_unique<LogCommand>(log, "b"))
        .add(std::make_unique<LogCommand>(log, "c", true));
    CHECK_THROWS_AS(macro.execute(), CommandError);
    CHECK(log == std::vector<std::string>{"+a", "+b", "-b", "-a"});
}

TEST_CASE("CommandHistory undo/redo round trip", "[command][history]") {
    FleetReceiver fleet(defaultFleet());
    CommandHistory history(10);
    CHECK_FALSE(history.undo());
    CHECK_FALSE(history.redo());
    history.execute(std::make_unique<MoveFleetCommand>(fleet, "A", 10.0));
    history.execute(std::make_unique<MoveFleetCommand>(fleet, "B", 10.0));
    CHECK(history.nextUndoName() == "Move to B");
    CHECK(history.undo());
    CHECK(fleet.status().location == "A");
    CHECK(history.nextRedoName() == "Move to B");
    CHECK(history.undo());
    CHECK(fleet.status() == defaultFleet());
    CHECK(history.redo());
    CHECK(history.redo());
    CHECK(fleet.status().location == "B");
    CHECK(fleet.status().fuel == 80.0);
    CHECK(history.undoDepth() == 2);
    CHECK(history.redoDepth() == 0);
    CHECK(history.nextRedoName().empty());
}

TEST_CASE("Executing a new command clears the redo stack", "[command][history]") {
    FleetReceiver fleet(defaultFleet());
    CommandHistory history;
    history.execute(std::make_unique<ReinforceCommand>(fleet, 1));
    history.undo();
    CHECK(history.canRedo());
    history.execute(std::make_unique<ReinforceCommand>(fleet, 2));
    CHECK_FALSE(history.canRedo());
    CHECK(fleet.status().ships == 7);
}

TEST_CASE("CommandHistory is bounded and evicts the oldest entries", "[command][history]") {
    FleetReceiver fleet(defaultFleet());
    CommandHistory history(3);
    for (int i = 0; i < 5; ++i) {
        history.execute(std::make_unique<ReinforceCommand>(fleet, 1));
    }
    CHECK(fleet.status().ships == 10);
    CHECK(history.undoDepth() == 3);
    CHECK(history.evicted() == 2);
    while (history.undo()) {}
    CHECK(fleet.status().ships == 7); // the two evicted commands cannot be undone
    CHECK(history.redoDepth() == 3);
    CHECK_THROWS_AS(CommandHistory(0), std::invalid_argument);
}

TEST_CASE("Failed execution is not recorded in history", "[command][history]") {
    FleetReceiver fleet(defaultFleet());
    CommandHistory history;
    CHECK_THROWS_AS(history.execute(std::make_unique<MoveFleetCommand>(fleet, "X", 1e9)), CommandError);
    CHECK_FALSE(history.canUndo());
    CHECK_THROWS_AS(history.execute(nullptr), std::invalid_argument);
    history.execute(std::make_unique<ReinforceCommand>(fleet, 1));
    history.clear();
    CHECK_FALSE(history.canUndo());
    CHECK_FALSE(history.canRedo());
}
