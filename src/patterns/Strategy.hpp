// File: src/patterns/Strategy.hpp
// CppVerseHub - Strategy Pattern Implementation for Fleet Routing Strategies

#pragma once

#include <memory>
#include <vector>
#include <string>
#include <unordered_map>
#include <functional>
#include <algorithm>
#include <cmath>

namespace CppVerseHub::Patterns {

// Forward declarations
class Planet;
class Fleet;

/**
 * @brief 3D coordinate structure for space navigation
 */
struct Coordinate3D {
    double x, y, z;
    
    Coordinate3D(double x = 0.0, double y = 0.0, double z = 0.0) : x(x), y(y), z(z) {}
    
    /**
     * @brief Calculate distance to another coordinate
     */
    double distanceTo(const Coordinate3D& other) const {
        double dx = x - other.x;
        double dy = y - other.y;
        double dz = z - other.z;
        return std::sqrt(dx*dx + dy*dy + dz*dz);
    }
    
    bool operator==(const Coordinate3D& other) const {
        const double epsilon = 1e-9;
        return std::abs(x - other.x) < epsilon && 
               std::abs(y - other.y) < epsilon && 
               std::abs(z - other.z) < epsilon;
    }
};

/**
 * @brief Route information structure
 */
struct RouteInfo {
    std::vector<Coordinate3D> waypoints;
    double total_distance = 0.0;
    double estimated_time = 0.0;
    double fuel_cost = 0.0;
    double risk_factor = 0.0;
    std::string route_description;
    
    /**
     * @brief Calculate route efficiency score (lower is better)
     */
    double getEfficiencyScore() const {
        return total_distance * (1.0 + risk_factor) + fuel_cost * 0.5;
    }
};

/**
 * @brief Abstract base class for routing strategies
 * 
 * Defines the interface for different fleet routing algorithms.
 * Each strategy implements a different approach to pathfinding
 * between celestial bodies in space.
 */
class IRoutingStrategy {
public:
    virtual ~IRoutingStrategy() = default;
    
    /**
     * @brief Calculate route between two points
     * @param start Starting coordinate
     * @param destination Destination coordinate
     * @param fleet_speed Speed of the fleet (units per time)
     * @param fleet_capacity Carrying capacity affecting fuel consumption
     * @return Route information with waypoints and metrics
     */
    virtual RouteInfo calculateRoute(
        const Coordinate3D& start,
        const Coordinate3D& destination,
        double fleet_speed = 1.0,
        double fleet_capacity = 1.0
    ) const = 0;
    
    /**
     * @brief Get strategy name
     * @return Human-readable strategy name
     */
    virtual std::string getStrategyName() const = 0;
    
    /**
     * @brief Get strategy description
     * @return Detailed description of the strategy
     */
    virtual std::string getDescription() const = 0;
    
    /**
     * @brief Check if strategy supports multi-waypoint routing
     * @return true if multiple waypoints are supported
     */
    virtual bool supportsMultiWaypoint() const { return false; }
    
    /**
     * @brief Calculate multi-waypoint route (if supported)
     * @param waypoints Vector of coordinates to visit in order
     * @param fleet_speed Speed of the fleet
     * @param fleet_capacity Fleet capacity
     * @return Complete route information
     */
    virtual RouteInfo calculateMultiWaypointRoute(
        const std::vector<Coordinate3D>& waypoints,
        double fleet_speed = 1.0,
        double fleet_capacity = 1.0
    ) const;
};

/**
 * @brief Direct Line Strategy - Shortest path between two points
 * 
 * Simple strategy that calculates direct routes without considering
 * obstacles or optimizations. Fastest computation but may not be
 * most efficient in complex space environments.
 */
class DirectLineStrategy : public IRoutingStrategy {
public:
    RouteInfo calculateRoute(
        const Coordinate3D& start,
        const Coordinate3D& destination,
        double fleet_speed = 1.0,
        double fleet_capacity = 1.0
    ) const override;
    
    std::string getStrategyName() const override {
        return "Direct Line";
    }
    
    std::string getDescription() const override {
        return "Calculates the shortest direct path between two points. "
               "Fast computation but doesn't consider obstacles or fuel optimization.";
    }
    
    bool supportsMultiWaypoint() const override { return true; }
};

/**
 * @brief Fuel Optimized Strategy - Minimizes fuel consumption
 * 
 * Considers fuel efficiency by calculating routes that minimize
 * energy expenditure, including gravitational assists and
 * efficient acceleration/deceleration patterns.
 */
class FuelOptimizedStrategy : public IRoutingStrategy {
public:
    /**
     * @brief Constructor with fuel efficiency parameters
     * @param base_consumption Base fuel consumption rate
     * @param acceleration_factor Fuel cost multiplier for acceleration
     */
    explicit FuelOptimizedStrategy(double base_consumption = 1.0, double acceleration_factor = 1.5)
        : base_fuel_consumption_(base_consumption), acceleration_factor_(acceleration_factor) {}
    
    RouteInfo calculateRoute(
        const Coordinate3D& start,
        const Coordinate3D& destination,
        double fleet_speed = 1.0,
        double fleet_capacity = 1.0
    ) const override;
    
    std::string getStrategyName() const override {
        return "Fuel Optimized";
    }
    
    std::string getDescription() const override {
        return "Calculates routes that minimize fuel consumption by optimizing "
               "acceleration patterns and considering gravitational effects.";
    }
    
    bool supportsMultiWaypoint() const override { return true; }

private:
    double base_fuel_consumption_;
    double acceleration_factor_;
    
    /**
     * @brief Calculate fuel cost for a route segment
     */
    double calculateFuelCost(const Coordinate3D& start, const Coordinate3D& end, 
                           double fleet_speed, double fleet_capacity) const;
};

/**
 * @brief Safe Route Strategy - Prioritizes safety over speed
 * 
 * Calculates routes that avoid dangerous regions, asteroid fields,
 * and high-radiation zones. May take longer but reduces mission risk.
 */
class SafeRouteStrategy : public IRoutingStrategy {
public:
    /**
     * @brief Constructor with safety parameters
     * @param risk_threshold Maximum acceptable risk level (0.0 - 1.0)
     * @param safety_margin Additional safety buffer distance
     */
    explicit SafeRouteStrategy(double risk_threshold = 0.3, double safety_margin = 10.0)
        : risk_threshold_(risk_threshold), safety_margin_(safety_margin) {}
    
    /**
     * @brief Add a hazardous region to avoid
     * @param center Center of hazardous region
     * @param radius Radius of hazardous region
     * @param risk_level Risk level (0.0 - 1.0)
     */
    void addHazardousRegion(const Coordinate3D& center, double radius, double risk_level) {
        hazardous_regions_.emplace_back(center, radius, risk_level);
    }
    
    /**
     * @brief Clear all hazardous regions
     */
    void clearHazardousRegions() {
        hazardous_regions_.clear();
    }
    
    RouteInfo calculateRoute(
        const Coordinate3D& start,
        const Coordinate3D& destination,
        double fleet_speed = 1.0,
        double fleet_capacity = 1.0
    ) const override;
    
    std::string getStrategyName() const override {
        return "Safe Route";
    }
    
    std::string getDescription() const override {
        return "Calculates routes that prioritize safety by avoiding dangerous regions, "
               "asteroid fields, and high-risk zones.";
    }

private:
    struct HazardousRegion {
        Coordinate3D center;
        double radius;
        double risk_level;
        
        HazardousRegion(const Coordinate3D& c, double r, double risk)
            : center(c), radius(r), risk_level(risk) {}
    };
    
    double risk_threshold_;
    double safety_margin_;
    std::vector<HazardousRegion> hazardous_regions_;
    
    /**
     * @brief Calculate risk factor for a route segment
     */
    double calculateRiskFactor(const Coordinate3D& start, const Coordinate3D& end) const;
    
    /**
     * @brief Check if a point is in a hazardous region
     */
    double getPointRisk(const Coordinate3D& point) const;
};

/**
 * @brief Balanced Strategy - Compromises between speed, fuel, and safety
 * 
 * Attempts to find optimal balance between different routing factors.
 * Uses weighted scoring to determine best overall route.
 */
class BalancedStrategy : public IRoutingStrategy {
public:
    /**
     * @brief Constructor with weighting factors
     * @param time_weight Importance of travel time (0.0 - 1.0)
     * @param fuel_weight Importance of fuel efficiency (0.0 - 1.0)
     * @param safety_weight Importance of safety (0.0 - 1.0)
     */
    explicit BalancedStrategy(double time_weight = 0.4, double fuel_weight = 0.3, double safety_weight = 0.3)
        : time_weight_(time_weight), fuel_weight_(fuel_weight), safety_weight_(safety_weight) {
        normalizeWeights();
    }
    
    /**
     * @brief Set weighting factors
     */
    void setWeights(double time_weight, double fuel_weight, double safety_weight) {
        time_weight_ = time_weight;
        fuel_weight_ = fuel_weight;
        safety_weight_ = safety_weight;
        normalizeWeights();
    }
    
    RouteInfo calculateRoute(
        const Coordinate3D& start,
        const Coordinate3D& destination,
        double fleet_speed = 1.0,
        double fleet_capacity = 1.0
    ) const override;
    
    std::string getStrategyName() const override {
        return "Balanced";
    }
    
    std::string getDescription() const override {
        return "Calculates routes that balance travel time, fuel efficiency, and safety "
               "using configurable weighting factors.";
    }
    
    bool supportsMultiWaypoint() const override { return true; }

private:
    double time_weight_;
    double fuel_weight_;
    double safety_weight_;
    
    /**
     * @brief Normalize weights to sum to 1.0
     */
    void normalizeWeights() {
        double total = time_weight_ + fuel_weight_ + safety_weight_;
        if (total > 0.0) {
            time_weight_ /= total;
            fuel_weight_ /= total;
            safety_weight_ /= total;
        }
    }
    
    /**
     * @brief Calculate weighted score for route evaluation
     */
    double calculateWeightedScore(const RouteInfo& route) const;
};

/**
 * @brief Fleet Router - Context class that uses routing strategies
 * 
 * Manages different routing strategies and provides a unified interface
 * for fleet navigation. Supports strategy switching and comparison.
 */
class FleetRouter {
public:
    /**
     * @brief Constructor with default strategy
     * @param default_strategy Initial routing strategy
     */
    explicit FleetRouter(std::unique_ptr<IRoutingStrategy> default_strategy = nullptr);
    
    /**
     * @brief Set the current routing strategy
     * @param strategy New routing strategy
     */
    void setStrategy(std::unique_ptr<IRoutingStrategy> strategy) {
        current_strategy_ = std::move(strategy);
    }
    
    /**
     * @brief Get current strategy name
     * @return Name of current strategy or "None" if no strategy set
     */
    std::string getCurrentStrategyName() const {
        return current_strategy_ ? current_strategy_->getStrategyName() : "None";
    }
    
    /**
     * @brief Calculate route using current strategy
     */
    RouteInfo calculateRoute(
        const Coordinate3D& start,
        const Coordinate3D& destination,
        double fleet_speed = 1.0,
        double fleet_capacity = 1.0
    ) const;
    
    /**
     * @brief Calculate multi-waypoint route using current strategy
     */
    RouteInfo calculateMultiWaypointRoute(
        const std::vector<Coordinate3D>& waypoints,
        double fleet_speed = 1.0,
        double fleet_capacity = 1.0
    ) const;
    
    /**
     * @brief Compare multiple strategies for a given route
     * @param strategies Vector of strategies to compare
     * @param start Starting coordinate
     * @param destination Destination coordinate
     * @param fleet_speed Fleet speed
     * @param fleet_capacity Fleet capacity
     * @return Map of strategy names to route information
     */
    std::unordered_map<std::string, RouteInfo> compareStrategies(
        const std::vector<std::unique_ptr<IRoutingStrategy>>& strategies,
        const Coordinate3D& start,
        const Coordinate3D& destination,
        double fleet_speed = 1.0,
        double fleet_capacity = 1.0
    ) const;
    
    /**
     * @brief Find best strategy for given criteria
     * @param strategies Vector of strategies to evaluate
     * @param start Starting coordinate
     * @param destination Destination coordinate
     * @param criteria_function Function to evaluate route quality (lower score = better)
     * @param fleet_speed Fleet speed
     * @param fleet_capacity Fleet capacity
     * @return Name of best strategy
     */
    std::string findBestStrategy(
        const std::vector<std::unique_ptr<IRoutingStrategy>>& strategies,
        const Coordinate3D& start,
        const Coordinate3D& destination,
        std::function<double(const RouteInfo&)> criteria_function,
        double fleet_speed = 1.0,
        double fleet_capacity = 1.0
    ) const;
    
    /**
     * @brief Check if current strategy supports multi-waypoint routing
     */
    bool supportsMultiWaypoint() const {
        return current_strategy_ && current_strategy_->supportsMultiWaypoint();
    }

private:
    std::unique_ptr<IRoutingStrategy> current_strategy_;
};

/**
 * @brief Strategy Factory for creating routing strategies
 */
class RoutingStrategyFactory {
public:
    enum class StrategyType {
        DirectLine,
        FuelOptimized,
        SafeRoute,
        Balanced
    };
    
    /**
     * @brief Create a routing strategy of specified type
     * @param type Strategy type to create
     * @param parameters Optional parameters for strategy configuration
     * @return Unique pointer to created strategy
     */
    static std::unique_ptr<IRoutingStrategy> createStrategy(
        StrategyType type,
        const std::unordered_map<std::string, double>& parameters = {}
    );
    
    /**
     * @brief Get all available strategy types
     * @return Vector of available strategy types
     */
    static std::vector<StrategyType> getAvailableStrategies();
    
    /**
     * @brief Convert strategy type to string
     * @param type Strategy type
     * @return String representation of strategy type
     */
    static std::string strategyTypeToString(StrategyType type);
};

} // namespace CppVerseHub::Patterns