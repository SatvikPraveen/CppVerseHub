/**
 * @file Mission.cpp
 * @brief Mission base-class lifecycle implementation.
 */
#include "core/Mission.hpp"

#include <array>
#include <cmath>
#include <sstream>

#include <nlohmann/json.hpp>

#include "core/EventSystem.hpp"
#include "core/Events.hpp"
#include "core/Exceptions.hpp"
#include "core/Galaxy.hpp"

namespace CppVerseHub::Core {

namespace {

constexpr std::array<std::string_view, 3> kTypeNames{"exploration", "combat", "colonization"};
constexpr std::array<std::string_view, 5> kStatusNames{"pending", "active", "completed", "failed", "cancelled"};
constexpr std::array<std::string_view, 2> kPhaseNames{"travel", "execute"};

template <typename Enum, std::size_t N>
std::optional<Enum> parseName(const std::array<std::string_view, N>& names, std::string_view name) noexcept {
    for (std::size_t i = 0; i < N; ++i) {
        if (names[i] == name) {
            return static_cast<Enum>(i);
        }
    }
    return std::nullopt;
}

template <std::size_t N>
std::string_view nameOf(const std::array<std::string_view, N>& names, std::size_t i) noexcept {
    return i < N ? names[i] : std::string_view{"unknown"};
}

double validatedDuration(double duration) {
    if (!std::isfinite(duration) || duration <= 0.0) {
        throw InvalidArgumentException("mission duration must be positive and finite");
    }
    return duration;
}

} // namespace

std::string_view toString(MissionType type) noexcept { return nameOf(kTypeNames, static_cast<std::size_t>(type)); }

std::optional<MissionType> parseMissionType(std::string_view name) noexcept {
    return parseName<MissionType>(kTypeNames, name);
}

std::string_view toString(MissionStatus status) noexcept {
    return nameOf(kStatusNames, static_cast<std::size_t>(status));
}

std::optional<MissionStatus> parseMissionStatus(std::string_view name) noexcept {
    return parseName<MissionStatus>(kStatusNames, name);
}

Mission::Mission(MissionId id, EntityId fleet, EntityId target, double duration)
    : id_(id), fleet_(fleet), target_(target), duration_(validatedDuration(duration)) {
    if (!id_.isValid()) {
        throw InvalidArgumentException("mission id must be valid");
    }
    if (!fleet_.isValid() || !target_.isValid()) {
        throw InvalidArgumentException("mission requires a valid fleet and target");
    }
    if (fleet_ == target_) {
        throw InvalidArgumentException("mission fleet and target must differ");
    }
}

Mission::Mission(MissionId id, const nlohmann::json& params)
    : Mission(id, EntityId{params.at("fleet").get<std::uint64_t>()},
              EntityId{params.at("target").get<std::uint64_t>()}, params.at("duration").get<double>()) {
    if (params.contains("status")) {
        const auto s = parseMissionStatus(params["status"].get<std::string>());
        if (!s) {
            throw SerializationException("unknown mission status");
        }
        status_ = *s;
    }
    if (params.contains("phase")) {
        const auto p = parseName<MissionPhase>(kPhaseNames, params["phase"].get<std::string>());
        if (!p) {
            throw SerializationException("unknown mission phase");
        }
        phase_ = *p;
    }
    elapsed_ = params.value("elapsed", 0.0);
    if (!std::isfinite(elapsed_) || elapsed_ < 0.0) {
        throw SerializationException("mission elapsed time must be finite and non-negative");
    }
    failureReason_ = params.value("failureReason", std::string{});
}

double Mission::progress() const noexcept {
    if (status_ == MissionStatus::Completed) {
        return 1.0;
    }
    const double p = elapsed_ / duration_;
    return p < 0.0 ? 0.0 : (p > 1.0 ? 1.0 : p);
}

bool Mission::isFinished() const noexcept {
    return status_ == MissionStatus::Completed || status_ == MissionStatus::Failed ||
           status_ == MissionStatus::Cancelled;
}

std::optional<std::string> Mission::checkStart(const Fleet& fleet, const Planet& target) const {
    static_cast<void>(target);
    if (fleet.shipCount() == 0) {
        return std::string("fleet has no ships");
    }
    return std::nullopt;
}

void Mission::start(MissionContext& ctx) {
    if (status_ != MissionStatus::Pending) {
        throw InvalidStateException("mission " + std::to_string(id_.value()) + " is not pending");
    }
    status_ = MissionStatus::Active;
    const Fleet* fleet = ctx.galaxy.find<Fleet>(fleet_);
    const Planet* planet = ctx.galaxy.find<Planet>(target_);
    if (fleet == nullptr || !fleet->isAlive()) {
        fail(ctx, "fleet lost");
        return;
    }
    if (planet == nullptr || !planet->isAlive()) {
        fail(ctx, "target lost");
        return;
    }
    if (auto reason = checkStart(*fleet, *planet)) {
        fail(ctx, std::move(*reason));
        return;
    }
    ctx.events.publish(MissionStarted{id_, type(), fleet_, target_});
}

void Mission::cancel() noexcept {
    if (!isFinished()) {
        status_ = MissionStatus::Cancelled;
    }
}

void Mission::update(MissionContext& ctx, double dt) {
    if (status_ != MissionStatus::Active) {
        return;
    }
    Fleet* fleet = ctx.galaxy.find<Fleet>(fleet_);
    if (fleet == nullptr || !fleet->isAlive()) {
        fail(ctx, "fleet lost");
        return;
    }
    Planet* planet = ctx.galaxy.find<Planet>(target_);
    if (planet == nullptr || !planet->isAlive()) {
        fail(ctx, "target lost");
        return;
    }
    if (phase_ == MissionPhase::Travel) {
        if (fleet->position() != planet->position()) {
            fleet->setDestination(planet->position());
            return;
        }
        fleet->clearDestination();
        phase_ = MissionPhase::Execute;
    }
    elapsed_ += dt;
    switch (execute(ctx, *fleet, *planet, dt)) {
    case StepOutcome::Continue:
        break;
    case StepOutcome::Success:
        complete(ctx);
        break;
    case StepOutcome::Failure:
        fail(ctx, failureReason_.empty() ? std::string("mission failed") : failureReason_);
        break;
    }
}

void Mission::fail(MissionContext& ctx, std::string reason) {
    status_ = MissionStatus::Failed;
    failureReason_ = std::move(reason);
    ctx.events.publish(MissionFailed{id_, type(), failureReason_});
}

void Mission::complete(MissionContext& ctx) {
    status_ = MissionStatus::Completed;
    failureReason_.clear();
    ctx.events.publish(MissionCompleted{id_, type()});
}

std::string Mission::describe() const {
    std::ostringstream os;
    os << toString(type()) << " mission " << id_ << " fleet " << fleet_ << " -> planet " << target_ << " ["
       << toString(status_) << ", " << nameOf(kPhaseNames, static_cast<std::size_t>(phase_)) << ", "
       << static_cast<int>(progress() * 100.0) << "%]";
    if (!failureReason_.empty()) {
        os << " reason: " << failureReason_;
    }
    return os.str();
}

void Mission::toJson(nlohmann::json& out) const {
    out = {{"id", id_.value()},
           {"type", std::string(toString(type()))},
           {"fleet", fleet_.value()},
           {"target", target_.value()},
           {"duration", duration_},
           {"elapsed", elapsed_},
           {"status", std::string(toString(status_))},
           {"phase", std::string(nameOf(kPhaseNames, static_cast<std::size_t>(phase_)))},
           {"failureReason", failureReason_}};
}

} // namespace CppVerseHub::Core
