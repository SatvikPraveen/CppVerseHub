/**
 * @file Decorator.hpp
 * @brief Decorator pattern: stacking mission enhancements at run time, plus function decorators.
 *
 * A decorator implements the same interface as the object it wraps and forwards to it, adding
 * behaviour before/after. Decorators compose: `Stealth(SpeedBoost(Exploration))` is still an
 * `IMission`, and the client cannot tell the difference. Unlike inheritance, the combination is
 * chosen at run time and the number of classes grows linearly, not combinatorially.
 *
 * Two forms are shown:
 *  - **Object decorators** over `IMission` (ownership via `std::unique_ptr`; `decorate<D>()` helper);
 *  - **Function decorators**: higher-order templates (`withRetry`, `withCallCounter`, `memoize`)
 *    that wrap any callable — the idiomatic modern-C++ equivalent for cross-cutting concerns.
 */

#pragma once

#include <cstddef>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

namespace CppVerseHub::Patterns {

// ============================================================================
// Object decorators
// ============================================================================

/**
 * @brief Component interface: a mission with cost, duration and success odds.
 */
class IMission {
public:
    virtual ~IMission() = default;
    /// @brief Description including all enhancements. @return Text.
    [[nodiscard]] virtual std::string description() const = 0;
    /// @brief Cost in credits. @return Cost.
    [[nodiscard]] virtual double cost() const = 0;
    /// @brief Duration in hours. @return Hours.
    [[nodiscard]] virtual double durationHours() const = 0;
    /// @brief Probability of success in [0, 1]. @return Probability.
    [[nodiscard]] virtual double successProbability() const = 0;
    /// @brief Names of the enhancements applied, innermost first. @return Names.
    [[nodiscard]] virtual std::vector<std::string> enhancements() const { return {}; }

protected:
    IMission() = default;
    IMission(const IMission&) = default;
    IMission& operator=(const IMission&) = default;
    IMission(IMission&&) = default;
    IMission& operator=(IMission&&) = default;
};

/// @brief Owning pointer to a mission.
using MissionPtr = std::unique_ptr<IMission>;

/// @brief Kinds of base mission.
enum class MissionKind { Exploration, Combat, Colonization };

/**
 * @brief Concrete component: an undecorated mission.
 */
class BasicMission final : public IMission {
public:
    /**
     * @brief Construct.
     * @param kind Mission kind (determines base cost/duration/odds).
     * @param target Target system name.
     */
    BasicMission(MissionKind kind, std::string target);
    [[nodiscard]] std::string description() const override;
    [[nodiscard]] double cost() const override { return cost_; }
    [[nodiscard]] double durationHours() const override { return duration_; }
    [[nodiscard]] double successProbability() const override { return success_; }
    /// @brief Mission kind. @return Kind.
    [[nodiscard]] MissionKind kind() const noexcept { return kind_; }

private:
    MissionKind kind_;
    std::string target_;
    double cost_;
    double duration_;
    double success_;
};

/**
 * @brief Base decorator: owns the wrapped mission and forwards every call.
 */
class MissionDecorator : public IMission {
public:
    /// @brief Wrap a mission. @param inner Mission to decorate (non-null). @throws std::invalid_argument.
    explicit MissionDecorator(MissionPtr inner);
    [[nodiscard]] std::string description() const override { return inner_->description(); }
    [[nodiscard]] double cost() const override { return inner_->cost(); }
    [[nodiscard]] double durationHours() const override { return inner_->durationHours(); }
    [[nodiscard]] double successProbability() const override { return inner_->successProbability(); }
    [[nodiscard]] std::vector<std::string> enhancements() const override { return inner_->enhancements(); }

protected:
    /// @brief The wrapped mission. @return Reference.
    [[nodiscard]] const IMission& inner() const noexcept { return *inner_; }
    /// @brief Helper: inner enhancements plus @p name. @param name Enhancement name. @return Names.
    [[nodiscard]] std::vector<std::string> enhancementsPlus(const std::string& name) const;

private:
    MissionPtr inner_;
};

/// @brief Stealth systems: +15% success (relative to failure), +2000 cost.
class StealthEnhancement final : public MissionDecorator {
public:
    using MissionDecorator::MissionDecorator;
    [[nodiscard]] std::string description() const override;
    [[nodiscard]] double cost() const override { return inner().cost() + 2000.0; }
    [[nodiscard]] double successProbability() const override;
    [[nodiscard]] std::vector<std::string> enhancements() const override {
        return enhancementsPlus("Stealth");
    }
};

/// @brief Afterburners: duration x0.7, cost x1.25.
class SpeedBoost final : public MissionDecorator {
public:
    using MissionDecorator::MissionDecorator;
    [[nodiscard]] std::string description() const override;
    [[nodiscard]] double cost() const override { return inner().cost() * 1.25; }
    [[nodiscard]] double durationHours() const override { return inner().durationHours() * 0.7; }
    [[nodiscard]] std::vector<std::string> enhancements() const override {
        return enhancementsPlus("SpeedBoost");
    }
};

/// @brief Extra weaponry: +5000 cost, success bonus that is larger for combat missions.
class HeavyArmament final : public MissionDecorator {
public:
    /// @brief Wrap. @param inner Mission. @param combat Whether the base mission is combat (bigger bonus).
    HeavyArmament(MissionPtr inner, bool combat);
    [[nodiscard]] std::string description() const override;
    [[nodiscard]] double cost() const override { return inner().cost() + 5000.0; }
    [[nodiscard]] double successProbability() const override;
    [[nodiscard]] std::vector<std::string> enhancements() const override {
        return enhancementsPlus("HeavyArmament");
    }

private:
    bool combat_;
};

/// @brief Medical team: +800 cost, +1 h, +5% success.
class MedicalSupport final : public MissionDecorator {
public:
    using MissionDecorator::MissionDecorator;
    [[nodiscard]] std::string description() const override;
    [[nodiscard]] double cost() const override { return inner().cost() + 800.0; }
    [[nodiscard]] double durationHours() const override { return inner().durationHours() + 1.0; }
    [[nodiscard]] double successProbability() const override;
    [[nodiscard]] std::vector<std::string> enhancements() const override {
        return enhancementsPlus("Medical");
    }
};

/**
 * @brief Construct decorator @p D around @p inner.
 * @tparam D Decorator type.
 * @param inner Mission to wrap.
 * @param args Extra constructor arguments for @p D.
 * @return The decorated mission.
 */
template <typename D, typename... Args>
    requires std::is_base_of_v<MissionDecorator, D>
[[nodiscard]] MissionPtr decorate(MissionPtr inner, Args&&... args) {
    return std::make_unique<D>(std::move(inner), std::forward<Args>(args)...);
}

// ============================================================================
// Function decorators
// ============================================================================

/**
 * @brief Wrap @p fn so it is retried up to @p attempts times while it throws.
 * @param fn Callable.
 * @param attempts Total attempts (>= 1).
 * @return Callable with the same arguments; rethrows the last exception if every attempt fails.
 */
template <typename F>
[[nodiscard]] auto withRetry(F fn, std::size_t attempts) {
    if (attempts == 0) {
        throw std::invalid_argument("withRetry: attempts must be >= 1");
    }
    return [fn = std::move(fn), attempts](auto&&... args) mutable -> decltype(auto) {
        for (std::size_t i = 1;; ++i) {
            try {
                return std::invoke(fn, args...);
            } catch (...) {
                if (i >= attempts) {
                    throw;
                }
            }
        }
    };
}

/**
 * @brief Wrap @p fn so each call increments @p counter.
 * @param fn Callable.
 * @param counter Counter shared with the caller.
 * @return Callable forwarding to @p fn.
 */
template <typename F>
[[nodiscard]] auto withCallCounter(F fn, std::shared_ptr<std::size_t> counter) {
    return [fn = std::move(fn), counter = std::move(counter)](auto&&... args) mutable -> decltype(auto) {
        ++*counter;
        return std::invoke(fn, std::forward<decltype(args)>(args)...);
    };
}

/**
 * @brief Thread-safe memoising decorator for a pure function.
 *
 * Results are cached in a `std::map` keyed by the argument tuple (arguments must be copyable and
 * less-than comparable). The wrapper is copyable; copies share the cache.
 *
 * @tparam R Result type.
 * @tparam Args Argument types.
 * @param fn Pure function.
 * @return Memoised function.
 */
template <typename R, typename... Args>
[[nodiscard]] std::function<R(Args...)> memoize(std::type_identity_t<std::function<R(Args...)>> fn) {
    struct Cache {
        std::mutex mutex;
        std::map<std::tuple<std::decay_t<Args>...>, R> values;
    };
    auto cache = std::make_shared<Cache>();
    return [fn = std::move(fn), cache](Args... args) -> R {
        auto key = std::make_tuple(args...);
        {
            std::scoped_lock lock(cache->mutex);
            if (auto it = cache->values.find(key); it != cache->values.end()) {
                return it->second;
            }
        }
        R value = fn(args...); // computed outside the lock; a racing duplicate is harmless
        std::scoped_lock lock(cache->mutex);
        return cache->values.emplace(std::move(key), std::move(value)).first->second;
    };
}

/**
 * @brief Showcase object and function decorators.
 * @param out Stream receiving the narration.
 */
void demonstrateDecorator(std::ostream& out = std::cout);

} // namespace CppVerseHub::Patterns
