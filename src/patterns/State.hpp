/**
 * @file State.hpp
 * @brief State pattern in two forms: classic polymorphic states and a `std::variant` state machine.
 *
 * 1. **Classic OO (GoF)** — `MissionContext` delegates every event to its current
 *    `IMissionState` object; each state class decides which events it accepts and which state
 *    comes next. Adding a state means adding a class, and invalid events are rejected by the
 *    default handlers of the base class. Open for extension, but dispatch is virtual and
 *    states live on the heap.
 *
 * 2. **Value-based (`std::variant`)** — `VariantStateMachine` stores the state as a
 *    `std::variant` of plain structs and dispatches an event variant with `std::visit` over the
 *    (state, event) pair into an overload set of transition functions. Every state carries only
 *    the data that is meaningful in that state (e.g. `Charging::percent`), there is no heap
 *    allocation, and the compiler checks the transition table at compile time; a generic fallback
 *    overload makes every unlisted pair an explicit "rejected" transition. `WarpDrive` is a
 *    concrete machine built on it.
 */

#pragma once

#include <cstddef>
#include <iostream>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

namespace CppVerseHub::Patterns {

// ============================================================================
// 1. Classic object-oriented State pattern: mission lifecycle
// ============================================================================

/// @brief Phases of a mission.
enum class MissionPhase { Pending, Planning, Active, Paused, Completed, Failed, Aborted };

/// @brief Name of a phase. @param phase The phase. @return Its name.
[[nodiscard]] std::string_view toString(MissionPhase phase) noexcept;

class MissionContext;
class IMissionState;

/**
 * @brief Result of handling an event in a state.
 *
 * `accepted == false` means the event is illegal in this state. `accepted && next == nullptr`
 * means the event was handled without changing state.
 */
struct StateOutcome {
    bool accepted = false;               ///< Whether the event was legal.
    std::unique_ptr<IMissionState> next; ///< New state, or null to stay.

    /// @brief Event rejected. @return Outcome.
    [[nodiscard]] static StateOutcome reject() { return {}; }
    /// @brief Event handled, stay in the current state. @return Outcome.
    [[nodiscard]] static StateOutcome stay() { return {true, nullptr}; }
    /// @brief Transition to @p state. @param state Next state. @return Outcome.
    [[nodiscard]] static StateOutcome to(std::unique_ptr<IMissionState> state) {
        return {true, std::move(state)};
    }
};

/**
 * @brief Abstract mission state. Every handler rejects by default; concrete states override the
 *        events they accept.
 */
class IMissionState {
public:
    virtual ~IMissionState() = default;

    /// @brief Phase this state represents. @return Phase.
    [[nodiscard]] virtual MissionPhase phase() const noexcept = 0;
    /// @brief Whether no further transitions are possible. @return Flag.
    [[nodiscard]] virtual bool isTerminal() const noexcept { return false; }

    /// @brief Handle "plan". @param ctx Mission. @return Outcome.
    virtual StateOutcome plan(MissionContext& ctx);
    /// @brief Handle "launch". @param ctx Mission. @return Outcome.
    virtual StateOutcome launch(MissionContext& ctx);
    /// @brief Handle "pause". @param ctx Mission. @return Outcome.
    virtual StateOutcome pause(MissionContext& ctx);
    /// @brief Handle "resume". @param ctx Mission. @return Outcome.
    virtual StateOutcome resume(MissionContext& ctx);
    /// @brief Handle progress. @param ctx Mission. @param percent Progress delta. @return Outcome.
    virtual StateOutcome advance(MissionContext& ctx, double percent);
    /// @brief Handle a failure. @param ctx Mission. @param reason Why it failed. @return Outcome.
    virtual StateOutcome fail(MissionContext& ctx, const std::string& reason);
    /// @brief Handle an abort. @param ctx Mission. @return Outcome.
    virtual StateOutcome abort(MissionContext& ctx);

protected:
    IMissionState() = default;
    IMissionState(const IMissionState&) = default;
    IMissionState& operator=(const IMissionState&) = default;
    IMissionState(IMissionState&&) = default;
    IMissionState& operator=(IMissionState&&) = default;
};

/// @brief One recorded transition.
struct PhaseTransition {
    MissionPhase from;   ///< Phase before.
    MissionPhase to;     ///< Phase after.
    std::string trigger; ///< Event name.
};

/**
 * @brief Context object of the classic State pattern.
 *
 * Mission lifecycle:
 * `Pending -plan-> Planning -launch-> Active <-pause/resume-> Paused`,
 * `Active --advance to 100%--> Completed`, `Active/Paused -fail-> Failed`,
 * any non-terminal `-abort-> Aborted`.
 */
class MissionContext {
public:
    /// @brief Create a mission in the Pending phase. @param name Mission name.
    explicit MissionContext(std::string name);

    /// @brief Begin planning. @return true if accepted.
    bool plan();
    /// @brief Launch a planned mission. @return true if accepted.
    bool launch();
    /// @brief Pause an active mission. @return true if accepted.
    bool pause();
    /// @brief Resume a paused mission. @return true if accepted.
    bool resume();
    /// @brief Report progress (only while active). @param percent Delta in percent (> 0). @return Accepted.
    bool advance(double percent);
    /// @brief Fail the mission. @param reason Reason. @return true if accepted.
    bool fail(std::string reason);
    /// @brief Abort the mission. @return true if accepted.
    bool abort();

    /// @brief Mission name. @return Name.
    [[nodiscard]] const std::string& name() const noexcept { return name_; }
    /// @brief Current phase. @return Phase.
    [[nodiscard]] MissionPhase phase() const noexcept { return state_->phase(); }
    /// @brief Whether the mission reached a terminal phase. @return Flag.
    [[nodiscard]] bool finished() const noexcept { return state_->isTerminal(); }
    /// @brief Progress in [0, 100]. @return Percent.
    [[nodiscard]] double progress() const noexcept { return progress_; }
    /// @brief Failure reason (empty unless Failed). @return Reason.
    [[nodiscard]] const std::string& failureReason() const noexcept { return failureReason_; }
    /// @brief Number of rejected events. @return Count.
    [[nodiscard]] std::size_t rejectedEvents() const noexcept { return rejected_; }
    /// @brief Transition log. @return Transitions in order.
    [[nodiscard]] const std::vector<PhaseTransition>& history() const noexcept { return history_; }

    /// @brief Used by states: set progress (clamped to [0,100]). @param percent Progress.
    void setProgress(double percent) noexcept;
    /// @brief Used by states: record a failure reason. @param reason Reason.
    void setFailureReason(std::string reason) { failureReason_ = std::move(reason); }

private:
    bool apply(StateOutcome outcome, std::string_view trigger);

    std::string name_;
    std::unique_ptr<IMissionState> state_;
    double progress_ = 0.0;
    std::string failureReason_;
    std::size_t rejected_ = 0;
    std::vector<PhaseTransition> history_;
};

// ============================================================================
// 2. std::variant-based state machine
// ============================================================================

/**
 * @brief Overload-set helper for std::visit.
 * @tparam Ts Callable types whose call operators are merged.
 */
template <typename... Ts>
struct Overloaded : Ts... {
    using Ts::operator()...;
};
/// @brief Deduction guide for Overloaded.
template <typename... Ts>
Overloaded(Ts...) -> Overloaded<Ts...>;

/**
 * @brief Generic finite-state machine over variant states and events.
 *
 * @tparam StateVariant A `std::variant` of state types.
 * @tparam EventVariant A `std::variant` of event types.
 * @tparam Transitions  A callable such that `t(const S&, const E&)` returns
 *                      `std::optional<StateVariant>` for every state S and event E
 *                      (`std::nullopt` = event rejected).
 */
template <typename StateVariant, typename EventVariant, typename Transitions>
class VariantStateMachine {
public:
    /**
     * @brief Construct with an initial state.
     * @param initial Initial state.
     * @param transitions Transition table object.
     */
    explicit VariantStateMachine(StateVariant initial, Transitions transitions = {})
        : state_(std::move(initial)), transitions_(std::move(transitions)) {}

    /**
     * @brief Feed an event to the machine.
     * @param event The event.
     * @return true if a transition happened (possibly to a state of the same type).
     */
    bool dispatch(const EventVariant& event) {
        std::optional<StateVariant> next =
            std::visit([this](const auto& s,
                              const auto& e) -> std::optional<StateVariant> { return transitions_(s, e); },
                       state_, event);
        if (!next) {
            ++rejected_;
            return false;
        }
        state_ = std::move(*next);
        ++accepted_;
        return true;
    }

    /// @brief Whether the current state is @p S. @tparam S State type. @return Flag.
    template <typename S>
    [[nodiscard]] bool is() const noexcept {
        return std::holds_alternative<S>(state_);
    }

    /// @brief Pointer to the current state if it is @p S. @tparam S State type. @return Pointer or null.
    template <typename S>
    [[nodiscard]] const S* getIf() const noexcept {
        return std::get_if<S>(&state_);
    }

    /// @brief Current state. @return State variant.
    [[nodiscard]] const StateVariant& state() const noexcept { return state_; }
    /// @brief Accepted transitions so far. @return Count.
    [[nodiscard]] std::size_t acceptedCount() const noexcept { return accepted_; }
    /// @brief Rejected events so far. @return Count.
    [[nodiscard]] std::size_t rejectedCount() const noexcept { return rejected_; }

private:
    StateVariant state_;
    Transitions transitions_;
    std::size_t accepted_ = 0;
    std::size_t rejected_ = 0;
};

/// @brief Warp-drive states (each carries only the data valid in that state).
namespace Warp {
/// @brief Drive powered down.
struct Offline {
    static constexpr std::string_view name = "Offline";
};
/// @brief Capacitors charging.
struct Charging {
    static constexpr std::string_view name = "Charging";
    double percent = 0.0; ///< Charge level, 0..100.
};
/// @brief Fully charged, ready to jump.
struct Ready {
    static constexpr std::string_view name = "Ready";
};
/// @brief Jump in progress.
struct Jumping {
    static constexpr std::string_view name = "Jumping";
    std::string destination; ///< Jump target.
};
/// @brief Cooling down after a jump.
struct Cooldown {
    static constexpr std::string_view name = "Cooldown";
    int ticksRemaining = 0; ///< Ticks until charging may resume.
};

/// @brief Power the drive on.
struct PowerOn {};
/// @brief Add charge.
struct Charge {
    double amount = 0.0;
}; ///< Percent to add.
/// @brief Start a jump.
struct Engage {
    std::string destination;
}; ///< Jump target.
/// @brief Time step.
struct Tick {};
/// @brief Power the drive off.
struct Shutdown {};

/// @brief State variant.
using State = std::variant<Offline, Charging, Ready, Jumping, Cooldown>;
/// @brief Event variant.
using Event = std::variant<PowerOn, Charge, Engage, Tick, Shutdown>;

/// @brief Ticks of cooldown after every jump.
inline constexpr int kCooldownTicks = 3;

/**
 * @brief Transition table. Overload resolution picks the most specific handler; the variadic
 *        fallback rejects every other (state, event) pair.
 */
struct Transitions {
    /// @brief Offline + PowerOn. @return Charging at 0%.
    std::optional<State> operator()(const Offline&, const PowerOn&) const { return Charging{0.0}; }
    /// @brief Charging + Charge. @param s State. @param e Event. @return Charging or Ready.
    std::optional<State> operator()(const Charging& s, const Charge& e) const {
        if (!(e.amount > 0.0)) {
            return std::nullopt;
        }
        const double level = s.percent + e.amount;
        if (level >= 100.0) {
            return Ready{};
        }
        return Charging{level};
    }
    /// @brief Ready + Engage. @param e Event. @return Jumping (rejects empty destinations).
    std::optional<State> operator()(const Ready&, const Engage& e) const {
        if (e.destination.empty()) {
            return std::nullopt;
        }
        return Jumping{e.destination};
    }
    /// @brief Jumping + Tick. @return Cooldown.
    std::optional<State> operator()(const Jumping&, const Tick&) const { return Cooldown{kCooldownTicks}; }
    /// @brief Cooldown + Tick. @param s State. @return Cooldown or Charging.
    std::optional<State> operator()(const Cooldown& s, const Tick&) const {
        if (s.ticksRemaining > 1) {
            return Cooldown{s.ticksRemaining - 1};
        }
        return Charging{0.0};
    }
    /// @brief Shutdown is illegal mid-jump. @return nullopt.
    std::optional<State> operator()(const Jumping&, const Shutdown&) const { return std::nullopt; }
    /// @brief Shutdown when already offline is a no-op rejection. @return nullopt.
    std::optional<State> operator()(const Offline&, const Shutdown&) const { return std::nullopt; }
    /// @brief Any other state + Shutdown. @return Offline.
    template <typename S>
    std::optional<State> operator()(const S&, const Shutdown&) const {
        return Offline{};
    }
    /// @brief Fallback: every unlisted pair is rejected. @return nullopt.
    template <typename S, typename E>
    std::optional<State> operator()(const S&, const E&) const {
        return std::nullopt;
    }
};
} // namespace Warp

/// @brief Name of a warp state. @param state State. @return Its name.
[[nodiscard]] std::string_view stateName(const Warp::State& state) noexcept;

/**
 * @brief A warp drive driven by the variant state machine; counts completed jumps.
 */
class WarpDrive {
public:
    WarpDrive() = default;

    /// @brief Dispatch an event. @param event Event. @return true if accepted.
    bool handle(const Warp::Event& event);

    /// @brief Whether the drive is in state @p S. @tparam S State type. @return Flag.
    template <typename S>
    [[nodiscard]] bool is() const noexcept {
        return machine_.template is<S>();
    }
    /// @brief Current state. @return State variant.
    [[nodiscard]] const Warp::State& state() const noexcept { return machine_.state(); }
    /// @brief Current state's name. @return Name.
    [[nodiscard]] std::string_view stateName() const noexcept {
        return Patterns::stateName(machine_.state());
    }
    /// @brief Jumps started so far. @return Count.
    [[nodiscard]] std::size_t jumps() const noexcept { return jumps_; }
    /// @brief Destinations jumped to, in order. @return Destinations.
    [[nodiscard]] const std::vector<std::string>& log() const noexcept { return log_; }
    /// @brief Rejected events. @return Count.
    [[nodiscard]] std::size_t rejected() const noexcept { return machine_.rejectedCount(); }

private:
    VariantStateMachine<Warp::State, Warp::Event, Warp::Transitions> machine_{Warp::Offline{}};
    std::size_t jumps_ = 0;
    std::vector<std::string> log_;
};

/**
 * @brief Showcase both state-machine styles.
 * @param out Stream receiving the narration.
 */
void demonstrateState(std::ostream& out = std::cout);

} // namespace CppVerseHub::Patterns
