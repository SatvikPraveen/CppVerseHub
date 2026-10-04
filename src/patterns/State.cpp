/**
 * @file State.cpp
 * @brief Concrete mission states, MissionContext, WarpDrive and the state showcase.
 */

#include "patterns/State.hpp"

#include <algorithm>

namespace CppVerseHub::Patterns {

std::string_view toString(MissionPhase phase) noexcept {
    switch (phase) {
        case MissionPhase::Pending:
            return "Pending";
        case MissionPhase::Planning:
            return "Planning";
        case MissionPhase::Active:
            return "Active";
        case MissionPhase::Paused:
            return "Paused";
        case MissionPhase::Completed:
            return "Completed";
        case MissionPhase::Failed:
            return "Failed";
        case MissionPhase::Aborted:
            return "Aborted";
    }
    return "Unknown";
}

// Default handlers: reject.
StateOutcome IMissionState::plan(MissionContext&) {
    return StateOutcome::reject();
}
StateOutcome IMissionState::launch(MissionContext&) {
    return StateOutcome::reject();
}
StateOutcome IMissionState::pause(MissionContext&) {
    return StateOutcome::reject();
}
StateOutcome IMissionState::resume(MissionContext&) {
    return StateOutcome::reject();
}
StateOutcome IMissionState::advance(MissionContext&, double) {
    return StateOutcome::reject();
}
StateOutcome IMissionState::fail(MissionContext&, const std::string&) {
    return StateOutcome::reject();
}
StateOutcome IMissionState::abort(MissionContext&) {
    return StateOutcome::reject();
}

namespace {

template <MissionPhase P>
class TerminalState final : public IMissionState {
public:
    [[nodiscard]] MissionPhase phase() const noexcept override { return P; }
    [[nodiscard]] bool isTerminal() const noexcept override { return true; }
};

using CompletedState = TerminalState<MissionPhase::Completed>;
using FailedState = TerminalState<MissionPhase::Failed>;
using AbortedState = TerminalState<MissionPhase::Aborted>;

/// Shared behaviour for every non-terminal state: abort is always allowed.
class AbortableState : public IMissionState {
public:
    StateOutcome abort(MissionContext&) override {
        return StateOutcome::to(std::make_unique<AbortedState>());
    }
};

class ActiveState;

class PausedState final : public AbortableState {
public:
    [[nodiscard]] MissionPhase phase() const noexcept override { return MissionPhase::Paused; }
    StateOutcome resume(MissionContext& ctx) override;
    StateOutcome fail(MissionContext& ctx, const std::string& reason) override {
        ctx.setFailureReason(reason);
        return StateOutcome::to(std::make_unique<FailedState>());
    }
};

class ActiveState final : public AbortableState {
public:
    [[nodiscard]] MissionPhase phase() const noexcept override { return MissionPhase::Active; }
    StateOutcome pause(MissionContext&) override { return StateOutcome::to(std::make_unique<PausedState>()); }
    StateOutcome advance(MissionContext& ctx, double percent) override {
        if (!(percent > 0.0)) {
            return StateOutcome::reject();
        }
        ctx.setProgress(ctx.progress() + percent);
        if (ctx.progress() >= 100.0) {
            return StateOutcome::to(std::make_unique<CompletedState>());
        }
        return StateOutcome::stay();
    }
    StateOutcome fail(MissionContext& ctx, const std::string& reason) override {
        ctx.setFailureReason(reason);
        return StateOutcome::to(std::make_unique<FailedState>());
    }
};

StateOutcome PausedState::resume(MissionContext&) {
    return StateOutcome::to(std::make_unique<ActiveState>());
}

class PlanningState final : public AbortableState {
public:
    [[nodiscard]] MissionPhase phase() const noexcept override { return MissionPhase::Planning; }
    StateOutcome launch(MissionContext&) override {
        return StateOutcome::to(std::make_unique<ActiveState>());
    }
};

class PendingState final : public AbortableState {
public:
    [[nodiscard]] MissionPhase phase() const noexcept override { return MissionPhase::Pending; }
    StateOutcome plan(MissionContext&) override {
        return StateOutcome::to(std::make_unique<PlanningState>());
    }
};

} // namespace

MissionContext::MissionContext(std::string name)
    : name_(std::move(name)), state_(std::make_unique<PendingState>()) {}

bool MissionContext::apply(StateOutcome outcome, std::string_view trigger) {
    if (!outcome.accepted) {
        ++rejected_;
        return false;
    }
    if (outcome.next) {
        history_.push_back({state_->phase(), outcome.next->phase(), std::string(trigger)});
        state_ = std::move(outcome.next);
    }
    return true;
}

bool MissionContext::plan() {
    return apply(state_->plan(*this), "plan");
}
bool MissionContext::launch() {
    return apply(state_->launch(*this), "launch");
}
bool MissionContext::pause() {
    return apply(state_->pause(*this), "pause");
}
bool MissionContext::resume() {
    return apply(state_->resume(*this), "resume");
}
bool MissionContext::advance(double percent) {
    return apply(state_->advance(*this, percent), "advance");
}
bool MissionContext::fail(const std::string& reason) {
    return apply(state_->fail(*this, reason), "fail");
}
bool MissionContext::abort() {
    return apply(state_->abort(*this), "abort");
}

void MissionContext::setProgress(double percent) noexcept {
    progress_ = std::clamp(percent, 0.0, 100.0);
}

// ---------------------------------------------------------------------------
// Variant machine
// ---------------------------------------------------------------------------

std::string_view stateName(const Warp::State& state) noexcept {
    return std::visit([](const auto& s) noexcept { return std::decay_t<decltype(s)>::name; }, state);
}

bool WarpDrive::handle(const Warp::Event& event) {
    const bool wasReady = machine_.is<Warp::Ready>();
    if (!machine_.dispatch(event)) {
        return false;
    }
    if (wasReady) {
        if (const auto* jumping = machine_.getIf<Warp::Jumping>()) {
            ++jumps_;
            log_.push_back(jumping->destination);
        }
    }
    return true;
}

void demonstrateState(std::ostream& out) {
    out << "=== State pattern ===\n";
    out << "-- classic OO states --\n";
    MissionContext mission("Survey Kepler-22b");
    (void)mission.launch(); // rejected: still Pending
    (void)mission.plan();
    (void)mission.launch();
    (void)mission.advance(40.0);
    (void)mission.pause();
    (void)mission.advance(10.0); // rejected while paused
    (void)mission.resume();
    (void)mission.advance(60.0);
    for (const auto& t : mission.history()) {
        out << "  " << toString(t.from) << " --" << t.trigger << "--> " << toString(t.to) << '\n';
    }
    out << "  final: " << toString(mission.phase()) << " at " << mission.progress() << "%, rejected "
        << mission.rejectedEvents() << " events\n";

    out << "-- std::variant state machine --\n";
    WarpDrive drive;
    const std::vector<Warp::Event> script{Warp::PowerOn{},    Warp::Charge{60.0},   Warp::Engage{"Vega"},
                                          Warp::Charge{50.0}, Warp::Engage{"Vega"}, Warp::Shutdown{},
                                          Warp::Tick{},       Warp::Tick{},         Warp::Tick{},
                                          Warp::Tick{},       Warp::Shutdown{}};
    for (const auto& event : script) {
        const bool ok = drive.handle(event);
        out << "  " << (ok ? "ok      " : "rejected") << " -> " << drive.stateName() << '\n';
    }
    out << "  jumps: " << drive.jumps() << ", rejected: " << drive.rejected() << '\n';
}

} // namespace CppVerseHub::Patterns
