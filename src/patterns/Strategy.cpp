/**
 * @file Strategy.cpp
 * @brief Routing strategies, router, target selectors and the strategy showcase.
 */

#include "patterns/Strategy.hpp"

#include <algorithm>
#include <limits>
#include <stdexcept>
#include <utility>

namespace CppVerseHub::Patterns {

double score(const Route& route, const RouteWeights& weights) noexcept {
    return weights.time * route.time + weights.fuel * route.fuel + weights.risk * route.risk;
}

namespace {
/// Closest point on segment [a,b] to p.
Coordinate3D closestPointOnSegment(const Coordinate3D& a, const Coordinate3D& b,
                                   const Coordinate3D& p) noexcept {
    const Coordinate3D ab = b - a;
    const double len2 = ab.dot(ab);
    if (len2 == 0.0) {
        return a;
    }
    const double t = std::clamp((p - a).dot(ab) / len2, 0.0, 1.0);
    return a + ab * t;
}

bool contains(const Hazard& h, const Coordinate3D& p) noexcept {
    return p.distanceTo(h.center) < h.radius;
}
} // namespace

bool segmentIntersects(const Coordinate3D& a, const Coordinate3D& b, const Hazard& hazard) noexcept {
    return closestPointOnSegment(a, b, hazard.center).distanceTo(hazard.center) < hazard.radius;
}

Route evaluateRoute(std::vector<Coordinate3D> waypoints, double throttle, const NavigationContext& ctx,
                    std::string strategy) {
    Route r;
    r.waypoints = std::move(waypoints);
    r.strategy = std::move(strategy);
    throttle = std::clamp(throttle, 0.01, 1.0);
    for (std::size_t i = 1; i < r.waypoints.size(); ++i) {
        const auto& a = r.waypoints[i - 1];
        const auto& b = r.waypoints[i];
        r.distance += a.distanceTo(b);
        for (const auto& h : ctx.hazards) {
            if (segmentIntersects(a, b, h)) {
                r.risk += h.risk;
            }
        }
    }
    const double speed = ctx.cruiseSpeed * throttle;
    r.time = speed > 0.0 ? r.distance / speed : std::numeric_limits<double>::infinity();
    r.fuel = r.distance * ctx.fuelPerUnit * throttle * throttle;
    return r;
}

Route DirectLineStrategy::plan(const Coordinate3D& from, const Coordinate3D& to,
                               const NavigationContext& ctx) const {
    return evaluateRoute({from, to}, 1.0, ctx, std::string(name()));
}

FuelOptimizedStrategy::FuelOptimizedStrategy(double throttle) noexcept
    : throttle_(std::clamp(throttle, 0.05, 1.0)) {}

Route FuelOptimizedStrategy::plan(const Coordinate3D& from, const Coordinate3D& to,
                                  const NavigationContext& ctx) const {
    return evaluateRoute({from, to}, throttle_, ctx, std::string(name()));
}

SafeRouteStrategy::SafeRouteStrategy(double margin) noexcept : margin_(std::max(margin, 1.05)) {}

Route SafeRouteStrategy::plan(const Coordinate3D& from, const Coordinate3D& to,
                              const NavigationContext& ctx) const {
    std::vector<Coordinate3D> path{from, to};
    constexpr int kMaxRefinements = 32;
    for (int iter = 0; iter < kMaxRefinements; ++iter) {
        bool changed = false;
        for (std::size_t i = 1; i < path.size() && !changed; ++i) {
            const Coordinate3D a = path[i - 1];
            const Coordinate3D b = path[i];
            for (const auto& h : ctx.hazards) {
                // A hazard containing an endpoint cannot be avoided; skip it rather than loop.
                if (contains(h, a) || contains(h, b) || !segmentIntersects(a, b, h)) {
                    continue;
                }
                Coordinate3D away = closestPointOnSegment(a, b, h.center) - h.center;
                if (away.length() < 1e-9) {
                    // Segment goes through the centre: push perpendicular to the leg.
                    const Coordinate3D leg = b - a;
                    away = leg.cross({0.0, 0.0, 1.0});
                    if (away.length() < 1e-9) {
                        away = leg.cross({0.0, 1.0, 0.0});
                    }
                }
                const Coordinate3D detour = h.center + away * (h.radius * margin_ / away.length());
                path.insert(path.begin() + static_cast<std::ptrdiff_t>(i), detour);
                changed = true;
                break;
            }
        }
        if (!changed) {
            break;
        }
    }
    return evaluateRoute(std::move(path), 0.9, ctx, std::string(name()));
}

Route BalancedStrategy::plan(const Coordinate3D& from, const Coordinate3D& to,
                             const NavigationContext& ctx) const {
    const DirectLineStrategy direct;
    const FuelOptimizedStrategy fuel;
    const SafeRouteStrategy safe;
    const IRoutingStrategy* candidates[] = {&direct, &fuel, &safe};
    Route best;
    double bestScore = std::numeric_limits<double>::infinity();
    for (const auto* s : candidates) {
        Route r = s->plan(from, to, ctx);
        const double sc = score(r, weights_);
        if (sc < bestScore) {
            bestScore = sc;
            best = std::move(r);
        }
    }
    best.strategy = std::string(name()) + "(" + best.strategy + ")";
    return best;
}

std::unique_ptr<IRoutingStrategy> makeRoutingStrategy(RoutingStrategyType type) {
    switch (type) {
        case RoutingStrategyType::Direct:
            return std::make_unique<DirectLineStrategy>();
        case RoutingStrategyType::FuelOptimized:
            return std::make_unique<FuelOptimizedStrategy>();
        case RoutingStrategyType::SafeRoute:
            return std::make_unique<SafeRouteStrategy>();
        case RoutingStrategyType::Balanced:
            return std::make_unique<BalancedStrategy>();
    }
    throw std::invalid_argument("unknown RoutingStrategyType");
}

FleetRouter::FleetRouter(std::unique_ptr<IRoutingStrategy> strategy, NavigationContext ctx)
    : strategy_(std::move(strategy)), ctx_(std::move(ctx)) {
    if (!strategy_) {
        throw std::invalid_argument("FleetRouter requires a strategy");
    }
}

void FleetRouter::setStrategy(std::unique_ptr<IRoutingStrategy> strategy) {
    if (!strategy) {
        throw std::invalid_argument("FleetRouter::setStrategy: null strategy");
    }
    strategy_ = std::move(strategy);
}

Route FleetRouter::plan(const Coordinate3D& from, const Coordinate3D& to) const {
    return strategy_->plan(from, to, ctx_);
}

// ---------------------------------------------------------------------------
// Target selectors
// ---------------------------------------------------------------------------

namespace {
template <typename Key>
std::optional<std::size_t> argBest(std::span<const PlanetTarget> targets, Key key) {
    if (targets.empty()) {
        return std::nullopt;
    }
    std::size_t best = 0;
    double bestKey = key(targets[0]);
    for (std::size_t i = 1; i < targets.size(); ++i) {
        const double k = key(targets[i]);
        if (k > bestKey) { // strict: ties resolve to the first candidate (deterministic)
            bestKey = k;
            best = i;
        }
    }
    return best;
}
} // namespace

std::optional<std::size_t> nearestTarget(std::span<const PlanetTarget> targets, const Coordinate3D& origin) {
    return argBest(targets, [&](const PlanetTarget& t) { return -t.position.distanceTo(origin); });
}

std::optional<std::size_t> highestValueTarget(std::span<const PlanetTarget> targets, const Coordinate3D&) {
    return argBest(targets, [](const PlanetTarget& t) { return t.value; });
}

std::optional<std::size_t> bestValueRatioTarget(std::span<const PlanetTarget> targets,
                                                const Coordinate3D& origin) {
    return argBest(targets, [&](const PlanetTarget& t) {
        return t.value / (1.0 + t.position.distanceTo(origin) + std::max(0.0, t.defense));
    });
}

TargetSelector weakerThan(double maxDefense, TargetSelector inner) {
    if (!inner) {
        throw std::invalid_argument("weakerThan: null inner selector");
    }
    return [maxDefense, inner = std::move(inner)](std::span<const PlanetTarget> targets,
                                                  const Coordinate3D& origin) -> std::optional<std::size_t> {
        std::vector<PlanetTarget> eligible;
        std::vector<std::size_t> original;
        for (std::size_t i = 0; i < targets.size(); ++i) {
            if (targets[i].defense <= maxDefense) {
                eligible.push_back(targets[i]);
                original.push_back(i);
            }
        }
        const auto pick = inner(eligible, origin);
        if (!pick) {
            return std::nullopt;
        }
        return original.at(*pick);
    };
}

// ---------------------------------------------------------------------------
// Showcase
// ---------------------------------------------------------------------------

void demonstrateStrategy(std::ostream& out) {
    out << "=== Strategy pattern ===\n";
    NavigationContext ctx;
    ctx.hazards.push_back({{50.0, 0.0, 0.0}, 10.0, 5.0});
    const Coordinate3D from{0.0, 0.0, 0.0};
    const Coordinate3D to{100.0, 0.0, 0.0};

    FleetRouter router(makeRoutingStrategy(RoutingStrategyType::Direct), ctx);
    for (auto type : {RoutingStrategyType::Direct, RoutingStrategyType::FuelOptimized,
                      RoutingStrategyType::SafeRoute, RoutingStrategyType::Balanced}) {
        router.setStrategy(makeRoutingStrategy(type));
        const Route r = router.plan(from, to);
        out << "  " << r.strategy << ": waypoints " << r.waypoints.size() << ", distance " << r.distance
            << ", time " << r.time << ", fuel " << r.fuel << ", risk " << r.risk << '\n';
    }

    const StaticRouter<SafeRouteStrategy> fixed(SafeRouteStrategy{2.0}, ctx);
    out << "  static SafeRoute(margin 2) distance: " << fixed.plan(from, to).distance << '\n';

    const std::vector<PlanetTarget> targets{
        {"Ceres", {10.0, 0.0, 0.0}, 20.0, 5.0},
        {"Titan", {80.0, 0.0, 0.0}, 90.0, 60.0},
        {"Io", {30.0, 0.0, 0.0}, 60.0, 10.0},
    };
    const std::pair<std::string_view, TargetSelector> selectors[] = {
        {"nearest", nearestTarget},
        {"highest value", highestValueTarget},
        {"best ratio", bestValueRatioTarget},
        {"highest value, defence <= 20", weakerThan(20.0, highestValueTarget)},
    };
    for (const auto& [label, select] : selectors) {
        const auto pick = select(targets, from);
        out << "  target (" << label << "): " << (pick ? targets[*pick].name : std::string("none")) << '\n';
    }
}

} // namespace CppVerseHub::Patterns
