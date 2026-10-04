/**
 * @file Command.hpp
 * @brief Command pattern: fleet orders as objects with undo/redo and a bounded history.
 *
 * Encapsulating a request as an object decouples the invoker (a UI, a scheduler, a script) from
 * the receiver (`FleetReceiver`) and makes requests storable, composable and reversible.
 * This file demonstrates:
 *  - concrete commands that remember exactly what they need to reverse themselves;
 *  - `MacroCommand`, a composite that executes transactionally (rolls back on partial failure);
 *  - `LambdaCommand`, a lightweight command built from two callables;
 *  - `CommandHistory`, an invoker with undo/redo stacks whose undo depth is bounded so long-running
 *    sessions use O(capacity) memory (the oldest entry is discarded first).
 *
 * Every command gives the strong exception guarantee: `execute()` either succeeds or throws
 * `CommandError` leaving the receiver untouched.
 */

#pragma once

#include <cstddef>
#include <deque>
#include <functional>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

namespace CppVerseHub::Patterns {

/**
 * @brief Thrown when a command cannot be executed or undone.
 */
class CommandError : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

/**
 * @brief Abstract command.
 */
class ICommand {
public:
    virtual ~ICommand() = default;

    /// @brief Perform the action. @throws CommandError on failure (receiver unchanged).
    virtual void execute() = 0;
    /// @brief Reverse a previously successful execute(). @throws CommandError on failure.
    virtual void undo() = 0;
    /// @brief Short human-readable name. @return Name.
    [[nodiscard]] virtual std::string name() const = 0;

protected:
    ICommand() = default;
    ICommand(const ICommand&) = default;
    ICommand& operator=(const ICommand&) = default;
    ICommand(ICommand&&) = default;
    ICommand& operator=(ICommand&&) = default;
};

/// @brief Owning pointer to a command.
using CommandPtr = std::unique_ptr<ICommand>;

/**
 * @brief Observable state of a fleet (the receiver's data).
 */
struct FleetStatus {
    std::string name;            ///< Fleet name.
    std::string location;        ///< Current system.
    double fuel = 100.0;         ///< Fuel units.
    double health = 100.0;       ///< Hull integrity, 0..100.
    int ships = 1;               ///< Number of ships.

    /// @brief Member-wise equality. @return true if equal.
    friend bool operator==(const FleetStatus&, const FleetStatus&) = default;
};

/**
 * @brief Receiver: performs the actual fleet operations.
 */
class FleetReceiver {
public:
    /**
     * @brief Construct a receiver.
     * @param initial Initial fleet status.
     */
    explicit FleetReceiver(FleetStatus initial) : status_(std::move(initial)) {}

    /// @brief Current status. @return Status.
    [[nodiscard]] const FleetStatus& status() const noexcept { return status_; }

    /**
     * @brief Move to a destination.
     * @param destination Target system.
     * @param fuelCost Fuel consumed (must be <= current fuel).
     * @throws CommandError if fuel is insufficient or the fleet is crippled (health < 20).
     */
    void move(const std::string& destination, double fuelCost);

    /**
     * @brief Engage a target, taking damage and burning combat fuel.
     * @param target Target name (informational).
     * @param damageTaken Hull damage suffered.
     * @throws CommandError if the fleet is too damaged or low on fuel.
     */
    void attack(const std::string& target, double damageTaken);

    /**
     * @brief Add (or remove, if negative) ships.
     * @param delta Ship delta.
     * @throws CommandError if the result would be negative.
     */
    void reinforce(int delta);

    /// @brief Restore an exact snapshot (used by undo). @param snapshot State to restore.
    void restore(const FleetStatus& snapshot) { status_ = snapshot; }

    /// @brief Fuel burned by each attack. @return Fuel units.
    static constexpr double kCombatFuel = 5.0;

private:
    FleetStatus status_;
};

/**
 * @brief Move the fleet; undo returns it to the previous location and refunds the fuel.
 */
class MoveFleetCommand final : public ICommand {
public:
    /**
     * @brief Construct.
     * @param fleet Receiver (must outlive the command).
     * @param destination Target system.
     * @param fuelCost Fuel consumed.
     */
    MoveFleetCommand(FleetReceiver& fleet, std::string destination, double fuelCost);
    void execute() override;
    void undo() override;
    [[nodiscard]] std::string name() const override;

private:
    FleetReceiver* fleet_;
    std::string destination_;
    double fuelCost_;
    std::string previousLocation_;
};

/**
 * @brief Attack a target; undo restores the pre-combat snapshot (simulation rewind).
 */
class AttackCommand final : public ICommand {
public:
    /**
     * @brief Construct.
     * @param fleet Receiver (must outlive the command).
     * @param target Target name.
     * @param damageTaken Damage the fleet will take.
     */
    AttackCommand(FleetReceiver& fleet, std::string target, double damageTaken);
    void execute() override;
    void undo() override;
    [[nodiscard]] std::string name() const override;

private:
    FleetReceiver* fleet_;
    std::string target_;
    double damage_;
    FleetStatus before_;
};

/**
 * @brief Change the number of ships; undo applies the inverse delta.
 */
class ReinforceCommand final : public ICommand {
public:
    /**
     * @brief Construct.
     * @param fleet Receiver (must outlive the command).
     * @param delta Ships to add (negative to detach).
     */
    ReinforceCommand(FleetReceiver& fleet, int delta) noexcept : fleet_(&fleet), delta_(delta) {}
    void execute() override;
    void undo() override;
    [[nodiscard]] std::string name() const override;

private:
    FleetReceiver* fleet_;
    int delta_;
};

/**
 * @brief Command assembled from a pair of callables.
 */
class LambdaCommand final : public ICommand {
public:
    /**
     * @brief Construct.
     * @param name Command name.
     * @param doFn Action.
     * @param undoFn Inverse action.
     */
    LambdaCommand(std::string name, std::function<void()> doFn, std::function<void()> undoFn);
    void execute() override;
    void undo() override;
    [[nodiscard]] std::string name() const override { return name_; }

private:
    std::string name_;
    std::function<void()> do_;
    std::function<void()> undo_;
};

/**
 * @brief Composite command executed as a transaction.
 *
 * Children execute in order and are undone in reverse order. If child *k* throws, children
 * 0..k-1 are undone before the exception propagates, so the macro is all-or-nothing.
 */
class MacroCommand final : public ICommand {
public:
    /// @brief Construct an empty macro. @param name Macro name.
    explicit MacroCommand(std::string name) : name_(std::move(name)) {}

    /// @brief Append a child command. @param command Child (non-null). @return *this for chaining.
    MacroCommand& add(CommandPtr command);
    /// @brief Number of children. @return Count.
    [[nodiscard]] std::size_t size() const noexcept { return children_.size(); }

    void execute() override;
    void undo() override;
    [[nodiscard]] std::string name() const override { return name_; }

private:
    std::string name_;
    std::vector<CommandPtr> children_;
};

/**
 * @brief Invoker with bounded undo history and a redo stack.
 *
 * Executing a new command clears the redo stack (standard editor semantics). When the undo stack
 * exceeds `capacity()`, the oldest command is discarded and can no longer be undone.
 */
class CommandHistory {
public:
    /**
     * @brief Construct a history.
     * @param capacity Maximum undo depth (must be > 0).
     * @throws std::invalid_argument if capacity == 0.
     */
    explicit CommandHistory(std::size_t capacity = 64);

    /**
     * @brief Execute a command and record it.
     * @param command Command to run (non-null).
     * @throws CommandError (propagated) if execution fails; nothing is recorded in that case.
     */
    void execute(CommandPtr command);

    /// @brief Undo the most recent command. @return false if there is nothing to undo.
    bool undo();
    /// @brief Redo the most recently undone command. @return false if there is nothing to redo.
    bool redo();

    /// @brief Whether undo() would do something. @return Flag.
    [[nodiscard]] bool canUndo() const noexcept { return !undo_.empty(); }
    /// @brief Whether redo() would do something. @return Flag.
    [[nodiscard]] bool canRedo() const noexcept { return !redo_.empty(); }
    /// @brief Undo stack depth. @return Depth.
    [[nodiscard]] std::size_t undoDepth() const noexcept { return undo_.size(); }
    /// @brief Redo stack depth. @return Depth.
    [[nodiscard]] std::size_t redoDepth() const noexcept { return redo_.size(); }
    /// @brief Maximum undo depth. @return Capacity.
    [[nodiscard]] std::size_t capacity() const noexcept { return capacity_; }
    /// @brief Commands evicted because the history was full. @return Count.
    [[nodiscard]] std::size_t evicted() const noexcept { return evicted_; }
    /// @brief Name of the command undo() would reverse. @return Name or empty string.
    [[nodiscard]] std::string nextUndoName() const;
    /// @brief Name of the command redo() would re-apply. @return Name or empty string.
    [[nodiscard]] std::string nextRedoName() const;
    /// @brief Drop all history. @return void
    void clear() noexcept;

private:
    std::size_t capacity_;
    std::size_t evicted_ = 0;
    std::deque<CommandPtr> undo_;
    std::vector<CommandPtr> redo_;
};

/**
 * @brief Showcase commands, macros and undo/redo.
 * @param out Stream receiving the narration.
 */
void demonstrateCommand(std::ostream& out = std::cout);

}  // namespace CppVerseHub::Patterns
