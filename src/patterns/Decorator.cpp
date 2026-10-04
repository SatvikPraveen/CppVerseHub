/**
 * @file Decorator.cpp
 * @brief Mission decorators and the decorator showcase.
 */

#include "patterns/Decorator.hpp"

#include <algorithm>
#include <cstdint>

namespace CppVerseHub::Patterns {

namespace {
/// Reduce the failure probability by @p fraction (keeps the result inside [0, 1]).
double improve(double p, double fraction) noexcept {
    p = std::clamp(p, 0.0, 1.0);
    return p + (1.0 - p) * std::clamp(fraction, 0.0, 1.0);
}
}  // namespace

BasicMission::BasicMission(MissionKind kind, std::string target) : kind_(kind), target_(std::move(target)) {
    switch (kind_) {
        case MissionKind::Exploration:
            cost_ = 10000.0;
            duration_ = 48.0;
            success_ = 0.80;
            break;
        case MissionKind::Combat:
            cost_ = 25000.0;
            duration_ = 12.0;
            success_ = 0.55;
            break;
        case MissionKind::Colonization:
        default:
            cost_ = 60000.0;
            duration_ = 240.0;
            success_ = 0.65;
            break;
    }
}

std::string BasicMission::description() const {
    switch (kind_) {
        case MissionKind::Exploration: return "Exploration of " + target_;
        case MissionKind::Combat: return "Combat operation at " + target_;
        case MissionKind::Colonization: return "Colonization of " + target_;
    }
    return "Mission to " + target_;
}

MissionDecorator::MissionDecorator(MissionPtr inner) : inner_(std::move(inner)) {
    if (!inner_) {
        throw std::invalid_argument("MissionDecorator requires a mission to wrap");
    }
}

std::vector<std::string> MissionDecorator::enhancementsPlus(const std::string& name) const {
    auto names = inner_->enhancements();
    names.push_back(name);
    return names;
}

std::string StealthEnhancement::description() const { return inner().description() + " + stealth"; }
double StealthEnhancement::successProbability() const { return improve(inner().successProbability(), 0.15); }

std::string SpeedBoost::description() const { return inner().description() + " + speed boost"; }

HeavyArmament::HeavyArmament(MissionPtr inner, bool combat) : MissionDecorator(std::move(inner)), combat_(combat) {}
std::string HeavyArmament::description() const { return inner().description() + " + heavy armament"; }
double HeavyArmament::successProbability() const {
    return improve(inner().successProbability(), combat_ ? 0.40 : 0.05);
}

std::string MedicalSupport::description() const { return inner().description() + " + medical support"; }
double MedicalSupport::successProbability() const { return improve(inner().successProbability(), 0.05); }

void demonstrateDecorator(std::ostream& out) {
    out << "=== Decorator pattern ===\n";
    MissionPtr mission = std::make_unique<BasicMission>(MissionKind::Combat, "Rigel IV");
    auto report = [&out](const IMission& m) {
        out << "  " << m.description() << "\n    cost " << m.cost() << ", " << m.durationHours() << " h, success "
            << m.successProbability() * 100.0 << "%\n";
    };
    report(*mission);
    mission = decorate<HeavyArmament>(std::move(mission), true);
    mission = decorate<StealthEnhancement>(std::move(mission));
    mission = decorate<SpeedBoost>(std::move(mission));
    report(*mission);
    out << "  enhancement stack:";
    for (const auto& e : mission->enhancements()) {
        out << ' ' << e;
    }
    out << '\n';

    // Function decorators.
    int failuresLeft = 2;
    auto flakyScan = [&failuresLeft](int sector) {
        if (failuresLeft-- > 0) {
            throw std::runtime_error("sensor glitch");
        }
        return sector * 10;
    };
    auto robustScan = withRetry(flakyScan, 3);
    out << "  retrying scan succeeded with " << robustScan(4) << '\n';

    auto calls = std::make_shared<std::size_t>(0);
    std::function<std::uint64_t(int)> slowFib;
    slowFib = withCallCounter(
        [&slowFib](int n) -> std::uint64_t { return n < 2 ? static_cast<std::uint64_t>(n) : slowFib(n - 1) + slowFib(n - 2); },
        calls);
    const auto plain = slowFib(20);
    const std::size_t plainCalls = *calls;

    *calls = 0;
    std::function<std::uint64_t(int)> fastFib;
    fastFib = memoize<std::uint64_t, int>(withCallCounter(
        [&fastFib](int n) -> std::uint64_t { return n < 2 ? static_cast<std::uint64_t>(n) : fastFib(n - 1) + fastFib(n - 2); },
        calls));
    const auto memo = fastFib(20);
    out << "  fib(20) = " << plain << " in " << plainCalls << " calls; memoized " << memo << " in " << *calls
        << " calls\n";
}

}  // namespace CppVerseHub::Patterns
