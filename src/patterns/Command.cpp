/**
 * @file Command.cpp
 * @brief Receiver, concrete commands, macro and history implementation.
 */

#include "patterns/Command.hpp"

#include <algorithm>
#include <exception>
#include <utility>

namespace CppVerseHub::Patterns {

// ---------------------------------------------------------------------------
// FleetReceiver
// ---------------------------------------------------------------------------

void FleetReceiver::move(const std::string& destination, double fuelCost) {
    if (fuelCost < 0.0) {
        throw CommandError("negative fuel cost");
    }
    if (status_.health < 20.0) {
        throw CommandError(status_.name + " is too damaged to move");
    }
    if (status_.fuel < fuelCost) {
        throw CommandError(status_.name + " lacks fuel to reach " + destination);
    }
    status_.location = destination;
    status_.fuel -= fuelCost;
}

void FleetReceiver::attack(const std::string& target, double damageTaken) {
    if (status_.health < 30.0) {
        throw CommandError(status_.name + " is too damaged to attack " + target);
    }
    if (status_.fuel < kCombatFuel) {
        throw CommandError(status_.name + " lacks combat fuel");
    }
    status_.fuel -= kCombatFuel;
    status_.health = std::max(0.0, status_.health - std::max(0.0, damageTaken));
}

void FleetReceiver::reinforce(int delta) {
    if (status_.ships + delta < 0) {
        throw CommandError("cannot detach more ships than the fleet has");
    }
    status_.ships += delta;
}

// ---------------------------------------------------------------------------
// Concrete commands
// ---------------------------------------------------------------------------

MoveFleetCommand::MoveFleetCommand(FleetReceiver& fleet, std::string destination, double fuelCost)
    : fleet_(&fleet), destination_(std::move(destination)), fuelCost_(fuelCost) {}

void MoveFleetCommand::execute() {
    std::string previous = fleet_->status().location;
    fleet_->move(destination_, fuelCost_);
    previousLocation_ = std::move(previous);
}

void MoveFleetCommand::undo() {
    FleetStatus s = fleet_->status();
    s.location = previousLocation_;
    s.fuel += fuelCost_;
    fleet_->restore(s);
}

std::string MoveFleetCommand::name() const {
    return "Move to " + destination_;
}

AttackCommand::AttackCommand(FleetReceiver& fleet, std::string target, double damageTaken)
    : fleet_(&fleet), target_(std::move(target)), damage_(damageTaken) {}

void AttackCommand::execute() {
    FleetStatus snapshot = fleet_->status();
    fleet_->attack(target_, damage_);
    before_ = std::move(snapshot);
}

void AttackCommand::undo() {
    FleetStatus s = fleet_->status();
    s.fuel = before_.fuel;
    s.health = before_.health;
    fleet_->restore(s);
}

std::string AttackCommand::name() const {
    return "Attack " + target_;
}

void ReinforceCommand::execute() {
    fleet_->reinforce(delta_);
}

void ReinforceCommand::undo() {
    fleet_->reinforce(-delta_);
}

std::string ReinforceCommand::name() const {
    return (delta_ >= 0 ? "Reinforce +" : "Detach ") + std::to_string(delta_);
}

LambdaCommand::LambdaCommand(std::string name, std::function<void()> doFn, std::function<void()> undoFn)
    : name_(std::move(name)), do_(std::move(doFn)), undo_(std::move(undoFn)) {
    if (!do_ || !undo_) {
        throw std::invalid_argument("LambdaCommand requires both do and undo callables");
    }
}

void LambdaCommand::execute() {
    do_();
}

void LambdaCommand::undo() {
    undo_();
}

// ---------------------------------------------------------------------------
// MacroCommand
// ---------------------------------------------------------------------------

MacroCommand& MacroCommand::add(CommandPtr command) {
    if (!command) {
        throw std::invalid_argument("MacroCommand::add: null command");
    }
    children_.push_back(std::move(command));
    return *this;
}

void MacroCommand::execute() {
    std::size_t done = 0;
    try {
        for (; done < children_.size(); ++done) {
            children_[done]->execute();
        }
    } catch (...) {
        // Roll back what already succeeded so the macro is all-or-nothing.
        while (done > 0) {
            --done;
            children_[done]->undo();
        }
        throw;
    }
}

void MacroCommand::undo() {
    for (auto it = children_.rbegin(); it != children_.rend(); ++it) {
        (*it)->undo();
    }
}

// ---------------------------------------------------------------------------
// CommandHistory
// ---------------------------------------------------------------------------

CommandHistory::CommandHistory(std::size_t capacity) : capacity_(capacity) {
    if (capacity_ == 0) {
        throw std::invalid_argument("CommandHistory capacity must be positive");
    }
}

void CommandHistory::execute(CommandPtr command) {
    if (!command) {
        throw std::invalid_argument("CommandHistory::execute: null command");
    }
    command->execute(); // throws -> nothing recorded
    redo_.clear();
    undo_.push_back(std::move(command));
    if (undo_.size() > capacity_) {
        undo_.pop_front();
        ++evicted_;
    }
}

bool CommandHistory::undo() {
    if (undo_.empty()) {
        return false;
    }
    undo_.back()->undo(); // if this throws the command stays on the undo stack
    redo_.push_back(std::move(undo_.back()));
    undo_.pop_back();
    return true;
}

bool CommandHistory::redo() {
    if (redo_.empty()) {
        return false;
    }
    redo_.back()->execute();
    undo_.push_back(std::move(redo_.back()));
    redo_.pop_back();
    if (undo_.size() > capacity_) {
        undo_.pop_front();
        ++evicted_;
    }
    return true;
}

std::string CommandHistory::nextUndoName() const {
    return undo_.empty() ? std::string{} : undo_.back()->name();
}

std::string CommandHistory::nextRedoName() const {
    return redo_.empty() ? std::string{} : redo_.back()->name();
}

void CommandHistory::clear() noexcept {
    undo_.clear();
    redo_.clear();
}

// ---------------------------------------------------------------------------
// Showcase
// ---------------------------------------------------------------------------

namespace {
void printStatus(std::ostream& out, const FleetStatus& s) {
    out << "  [" << s.name << "] at " << s.location << ", fuel " << s.fuel << ", health " << s.health
        << ", ships " << s.ships << '\n';
}
} // namespace

void demonstrateCommand(std::ostream& out) {
    out << "=== Command pattern ===\n";
    FleetReceiver fleet(FleetStatus{"Vanguard", "Sol", 100.0, 100.0, 4});
    CommandHistory history(3);
    printStatus(out, fleet.status());

    history.execute(std::make_unique<MoveFleetCommand>(fleet, "Alpha Centauri", 30.0));
    history.execute(std::make_unique<AttackCommand>(fleet, "Raider outpost", 25.0));
    history.execute(std::make_unique<ReinforceCommand>(fleet, 2));
    printStatus(out, fleet.status());

    out << "  undo '" << history.nextUndoName() << "'\n";
    (void)history.undo();
    out << "  undo '" << history.nextUndoName() << "'\n";
    (void)history.undo();
    printStatus(out, fleet.status());
    out << "  redo '" << history.nextRedoName() << "'\n";
    (void)history.redo();
    printStatus(out, fleet.status());

    // Transactional macro: the second move fails for lack of fuel, so the first is rolled back.
    auto macro = std::make_unique<MacroCommand>("Deep strike");
    macro->add(std::make_unique<MoveFleetCommand>(fleet, "Sirius", 40.0))
        .add(std::make_unique<MoveFleetCommand>(fleet, "Vega", 500.0));
    try {
        history.execute(std::move(macro));
    } catch (const CommandError& e) {
        out << "  macro rejected: " << e.what() << '\n';
    }
    printStatus(out, fleet.status());
    out << "  undo depth " << history.undoDepth() << "/" << history.capacity() << ", redo depth "
        << history.redoDepth() << '\n';
}

} // namespace CppVerseHub::Patterns
