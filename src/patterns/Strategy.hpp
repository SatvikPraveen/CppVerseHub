/**
 * @file Strategy.hpp
 * @brief Strategy pattern for fleet routing and target selection, in three styles.
 *
 * Strategy encapsulates a family of interchangeable algorithms behind one interface so the
 * client (`FleetRouter`) can switch them at run time without conditional logic. Shown here:
 *  - **Classic runtime polymorphism**: `IRoutingStrategy` with Direct, FuelOptimized, SafeRoute
 *    (hazard avoidance) and Balanced (meta-strategy that picks the best candidate by weighted
 *    score) implementations, plus a factory.
 *  - **Compile-time policy** (`RoutingPolicy` concept + `StaticRouter<Policy>`): zero-overhead
 *    when the algorithm is fixed at compile time.
 *  - **Function objects** (`TargetSelector` = `std::function`): the lightest-weight strategy for
 *    stateless algorithms such as choosing which planet to attack.
 *
 * All algorithms are deterministic and allocation-light; geometry is plain 3-D vector maths.
 */

#pragma once

#include <cmath>
#include <concepts>
#include <cstddef>
#include <functional>
#include <iostream>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace CppVerseHub::Patterns {

/**
 * @brief 3-D point/vector used for navigation.
 */
struct Coordinate3D {
    double x = 0.0;  ///< X component.
    double y = 0.0;  ///< Y component.
    double z = 0.0;  ///< Z component.

    /// @brief Vector sum. @return Sum.
    [[nodiscard]] constexpr Coordinate3D operator+(const Coordinate3D& o) const noexcept {
        return {x + o.x, y + o.y, z + o.z};
    }
    /// @brief Vector difference. @return Difference.
    [[nodiscard]] constexpr Coordinate3D operator-(const Coordinate3D& o) const noexcept {
        return {x - o.x, y - o.y, z - o.z};
    }
    /// @brief Scale. @param k Factor. @return Scaled vector.
    [[nodiscard]] constexpr Coordinate3D operator*(double k) const noexcept { return {x * k, y * k, z * k}; }
    /// @brief Dot product. @param o Other vector. @return Dot product.
    [[nodiscard]] constexpr double dot(const Coordinate3D& o) const noexcept { return x * o.x + y * o.y + z * o.z; }
    /// @brief Cross product. @param o Other vector. @return Cross product.
    [[nodiscard]] constexpr Coordinate3D cross(const Coordinate3D& o) const noexcept {
        return {y * o.z - z * o.y, z * o.x - x * o.z, x * o.y - y * o.x};
    }
    /// @brief Euclidean length. @return Length.
    [[nodiscard]] double length() const noexcept { return std::sqrt(dot(*this)); }
    /// @brief Distance to another point. @param o Other point. @return Distance.
    [[nodiscard]] double distanceTo(const Coordinate3D& o) const noexcept { return (*this - o).length(); }
    /// @brief Exact component-wise equality. @return true if equal.
    friend constexpr bool operator==(const Coordinate3D&, const Coordinate3D&) = default;
};

/**
 * @brief Spherical region of space that is dangerous to cross.
 */
struct Hazard {
    Coordinate3D center;  ///< Centre.
    double radius = 0.0;  ///< Radius.
    double risk = 0.0;    ///< Risk added for every route leg crossing it.
};

/**
 * @brief Environment the strategies plan in.
 */
struct NavigationContext {
    std::vector<Hazard> hazards;  ///< Known hazards.
    double cruiseSpeed = 1.0;     ///< Distance per time unit at full throttle.
    double fuelPerUnit = 1.0;     ///< Fuel per distance at full throttle (scales with throttle^2).
};

/**
 * @brief A planned route and its metrics.
 */
struct Route {
    std::vector<Coordinate3D> waypoints;  ///< Waypoints including start and end.
    double distance = 0.0;                ///< Total path length.
    double time = 0.0;                    ///< Travel time.
    double fuel = 0.0;                    ///< Fuel consumed.
    double risk = 0.0;                    ///< Accumulated hazard risk.
    std::string strategy;                 ///< Name of the strategy that produced it.
};

/**
 * @brief Relative importance of route metrics (lower weighted score is better).
 */
struct RouteWeights {
    double time = 1.0;  ///< Weight of travel time.
    double fuel = 1.0;  ///< Weight of fuel.
    double risk = 1.0;  ///< Weight of risk.
};

/**
 * @brief Weighted score of a route.
 * @param route Route.
 * @param weights Weights.
 * @return `w.time*time + w.fuel*fuel + w.risk*risk`.
 */
[[nodiscard]] double score(const Route& route, const RouteWeights& weights) noexcept;

/**
 * @brief Whether segment [a,b] passes strictly inside a hazard.
 * @param a Segment start.
 * @param b Segment end.
 * @param hazard Hazard sphere.
 * @return true if the minimum distance from the centre to the segment is below the radius.
 */
[[nodiscard]] bool segmentIntersects(const Coordinate3D& a, const Coordinate3D& b, const Hazard& hazard) noexcept;

/**
 * @brief Compute metrics for a polyline flown at a given throttle.
 * @param waypoints Polyline (at least one point).
 * @param throttle Fraction of cruise speed in (0, 1].
 * @param ctx Environment.
 * @param strategy Name recorded in the route.
 * @return Route with all metrics filled in.
 */
[[nodiscard]] Route evaluateRoute(std::vector<Coordinate3D> waypoints, double throttle, const NavigationContext& ctx,
                                  std::string strategy);

// ----------------------------------------------------------------------------
// Classic runtime strategies
// ----------------------------------------------------------------------------

/**
 * @brief Routing strategy interface.
 */
class IRoutingStrategy {
public:
    virtual ~IRoutingStrategy() = default;
    /**
     * @brief Plan a route.
     * @param from Start.
     * @param to Destination.
     * @param ctx Environment.
     * @return Planned route.
     */
    [[nodiscard]] virtual Route plan(const Coordinate3D& from, const Coordinate3D& to,
                                     const NavigationContext& ctx) const = 0;
    /// @brief Strategy name. @return Name.
    [[nodiscard]] virtual std::string_view name() const noexcept = 0;

protected:
    IRoutingStrategy() = default;
    IRoutingStrategy(const IRoutingStrategy&) = default;
    IRoutingStrategy& operator=(const IRoutingStrategy&) = default;
    IRoutingStrategy(IRoutingStrategy&&) = default;
    IRoutingStrategy& operator=(IRoutingStrategy&&) = default;
};

/// @brief Straight line at full throttle: fastest, ignores hazards.
class DirectLineStrategy final : public IRoutingStrategy {
public:
    [[nodiscard]] Route plan(const Coordinate3D& from, const Coordinate3D& to,
                             const NavigationContext& ctx) const override;
    [[nodiscard]] std::string_view name() const noexcept override { return "Direct"; }
};

/// @brief Straight line at reduced throttle: fuel scales with throttle^2, so slower is cheaper.
class FuelOptimizedStrategy final : public IRoutingStrategy {
public:
    /// @brief Construct. @param throttle Throttle in (0,1]; clamped.
    explicit FuelOptimizedStrategy(double throttle = 0.6) noexcept;
    [[nodiscard]] Route plan(const Coordinate3D& from, const Coordinate3D& to,
                             const NavigationContext& ctx) const override;
    [[nodiscard]] std::string_view name() const noexcept override { return "FuelOptimized"; }
    /// @brief Configured throttle. @return Throttle.
    [[nodiscard]] double throttle() const noexcept { return throttle_; }

private:
    double throttle_;
};

/// @brief Inserts detour waypoints around hazards that the straight line would cross.
class SafeRouteStrategy final : public IRoutingStrategy {
public:
    /// @brief Construct. @param margin Detour distance as a multiple of hazard radius (> 1).
    explicit SafeRouteStrategy(double margin = 1.5) noexcept;
    [[nodiscard]] Route plan(const Coordinate3D& from, const Coordinate3D& to,
                             const NavigationContext& ctx) const override;
    [[nodiscard]] std::string_view name() const noexcept override { return "SafeRoute"; }

private:
    double margin_;
};

/// @brief Meta-strategy: runs the other strategies and returns the lowest weighted score.
class BalancedStrategy final : public IRoutingStrategy {
public:
    /// @brief Construct. @param weights Metric weights.
    explicit BalancedStrategy(RouteWeights weights = {}) noexcept : weights_(weights) {}
    [[nodiscard]] Route plan(const Coordinate3D& from, const Coordinate3D& to,
                             const NavigationContext& ctx) const override;
    [[nodiscard]] std::string_view name() const noexcept override { return "Balanced"; }

private:
    RouteWeights weights_;
};

/// @brief Identifiers for the routing factory.
enum class RoutingStrategyType { Direct, FuelOptimized, SafeRoute, Balanced };

/**
 * @brief Factory for routing strategies.
 * @param type Which strategy.
 * @return New strategy instance.
 */
[[nodiscard]] std::unique_ptr<IRoutingStrategy> makeRoutingStrategy(RoutingStrategyType type);

/**
 * @brief Context of the Strategy pattern: plans routes with a swappable strategy.
 */
class FleetRouter {
public:
    /**
     * @brief Construct.
     * @param strategy Initial strategy (non-null).
     * @param ctx Navigation environment.
     * @throws std::invalid_argument if @p strategy is null.
     */
    explicit FleetRouter(std::unique_ptr<IRoutingStrategy> strategy, NavigationContext ctx = {});

    /// @brief Replace the strategy. @param strategy New strategy (non-null). @throws std::invalid_argument.
    void setStrategy(std::unique_ptr<IRoutingStrategy> strategy);
    /// @brief Name of the current strategy. @return Name.
    [[nodiscard]] std::string_view strategyName() const noexcept { return strategy_->name(); }
    /// @brief Plan a route. @param from Start. @param to Destination. @return Route.
    [[nodiscard]] Route plan(const Coordinate3D& from, const Coordinate3D& to) const;
    /// @brief Navigation environment. @return Mutable context.
    [[nodiscard]] NavigationContext& context() noexcept { return ctx_; }

private:
    std::unique_ptr<IRoutingStrategy> strategy_;
    NavigationContext ctx_;
};

// ----------------------------------------------------------------------------
// Compile-time policy strategies
// ----------------------------------------------------------------------------

/**
 * @brief Any type with a const `plan(from, to, ctx)` returning Route.
 */
template <typename P>
concept RoutingPolicy = requires(const P& p, const Coordinate3D& c, const NavigationContext& ctx) {
    { p.plan(c, c, ctx) } -> std::same_as<Route>;
};

/**
 * @brief Router whose strategy is fixed at compile time (static dispatch, inlinable).
 * @tparam Policy A RoutingPolicy.
 */
template <RoutingPolicy Policy>
class StaticRouter {
public:
    /// @brief Construct. @param policy Policy instance. @param ctx Environment.
    explicit StaticRouter(Policy policy = {}, NavigationContext ctx = {})
        : policy_(std::move(policy)), ctx_(std::move(ctx)) {}
    /// @brief Plan a route. @param from Start. @param to Destination. @return Route.
    [[nodiscard]] Route plan(const Coordinate3D& from, const Coordinate3D& to) const {
        return policy_.plan(from, to, ctx_);
    }

private:
    Policy policy_;
    NavigationContext ctx_;
};

// ----------------------------------------------------------------------------
// Function-object strategies: target selection
// ----------------------------------------------------------------------------

/**
 * @brief Candidate planet for an attack.
 */
struct PlanetTarget {
    std::string name;       ///< Planet name.
    Coordinate3D position;  ///< Location.
    double value = 0.0;     ///< Strategic value.
    double defense = 0.0;   ///< Defence strength.
};

/// @brief Strategy that picks the index of a target, or nullopt if none is acceptable.
using TargetSelector = std::function<std::optional<std::size_t>(std::span<const PlanetTarget>, const Coordinate3D&)>;

/// @brief Choose the nearest target. @param targets Candidates. @param origin Fleet position. @return Index.
[[nodiscard]] std::optional<std::size_t> nearestTarget(std::span<const PlanetTarget> targets,
                                                       const Coordinate3D& origin);
/// @brief Choose the most valuable target. @param targets Candidates. @param origin Unused. @return Index.
[[nodiscard]] std::optional<std::size_t> highestValueTarget(std::span<const PlanetTarget> targets,
                                                            const Coordinate3D& origin);
/**
 * @brief Choose the best value / (1 + distance + defence) ratio.
 * @param targets Candidates.
 * @param origin Fleet position.
 * @return Index of the best ratio.
 */
[[nodiscard]] std::optional<std::size_t> bestValueRatioTarget(std::span<const PlanetTarget> targets,
                                                              const Coordinate3D& origin);
/**
 * @brief Factory for a selector that rejects targets above a defence cap, then delegates.
 * @param maxDefense Maximum acceptable defence.
 * @param inner Selector applied to the remaining targets.
 * @return Composed selector (indices refer to the original span).
 */
[[nodiscard]] TargetSelector weakerThan(double maxDefense, TargetSelector inner);

/**
 * @brief Showcase all three strategy styles.
 * @param out Stream receiving the narration.
 */
void demonstrateStrategy(std::ostream& out = std::cout);

}  // namespace CppVerseHub::Patterns
