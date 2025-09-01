// File: src/patterns/Strategy.cpp
// CppVerseHub - Strategy Pattern Implementation for Fleet Routing Strategies

#include "Strategy.hpp"
#include <algorithm>
#include <numeric>
#include <cmath>
#include <sstream>
#include <iomanip>

namespace CppVerseHub::Patterns {

// Base Strategy Implementation
RouteInfo IRoutingStrategy::calculateMultiWaypointRoute(
    const std::vector<Coordinate3D>& waypoints,
    double fleet_speed,
    double fleet_capacity
) const {
    if (waypoints.size() < 2) {
        return RouteInfo{};
    }
    
    RouteInfo combined_route;
    combined_route.waypoints = waypoints;
    combined_route.route_description = getStrategyName() + " multi-waypoint route";
    
    for (size_t i = 0; i < waypoints.size() - 1; ++i) {
        RouteInfo segment = calculateRoute(waypoints[i], waypoints[i + 1], fleet_speed, fleet_capacity);
        combined_route.total_distance += segment.total_distance;
        combined_route.estimated_time += segment.estimated_time;
        combined_route.fuel_cost += segment.fuel_cost;
        combined_route.risk_factor = std::max(combined_route.risk_factor, segment.risk_factor);
    }
    
    return combined_route;
}

// Direct Line Strategy Implementation
RouteInfo DirectLineStrategy::calculateRoute(
    const Coordinate3D& start,
    const Coordinate3D& destination,
    double fleet_speed,
    double fleet_capacity
) const {
    RouteInfo route;
    route.waypoints = {start, destination};
    route.total_distance = start.distanceTo(destination);
    route.estimated_time = route.total_distance / fleet_speed;
    route.fuel_cost = route.total_distance * 1.0; // Base fuel consumption
    route.risk_factor = 0.1; // Low risk for direct routes
    
    std::ostringstream desc;
    desc << std::fixed << std::setprecision(2)
         << "Direct line route: " << route.total_distance << " units, "
         << route.estimated_time << " time units";
    route.route_description = desc.str();
    
    return route;
}

// Fuel Optimized Strategy Implementation
RouteInfo FuelOptimizedStrategy::calculateRoute(
    const Coordinate3D& start,
    const Coordinate3D& destination,
    double fleet_speed,
    double fleet_capacity
) const {
    RouteInfo route;
    
    // Calculate direct distance
    double direct_distance = start.distanceTo(destination);
    
    // For fuel optimization, we might add intermediate waypoints
    // to take advantage of gravitational assists or optimal acceleration curves
    std::vector<Coordinate3D> optimized_waypoints;
    optimized_waypoints.push_back(start);
    
    // Add intermediate waypoint for fuel efficiency (simplified approach)
    if (direct_distance > 50.0) { // Only for longer routes
        Coordinate3D midpoint{
            (start.x + destination.x) / 2.0,
            (start.y + destination.y) / 2.0,
            (start.z + destination.z) / 2.0 + 5.0 // Slight detour for efficiency
        };
        optimized_waypoints.push_back(midpoint);
    }
    
    optimized_waypoints.push_back(destination);
    route.waypoints = optimized_waypoints;
    
    // Calculate total distance
    route.total_distance = 0.0;
    for (size_t i = 0; i < optimized_waypoints.size() - 1; ++i) {
        route.total_distance += optimized_waypoints[i].distanceTo(optimized_waypoints[i + 1]);
    }
    
    route.estimated_time = route.total_distance / fleet_speed * 0.9; // 10% time savings from optimization
    route.fuel_cost = calculateFuelCost(start, destination, fleet_speed, fleet_capacity);
    route.risk_factor = 0.15;
    
    std::ostringstream desc;
    desc << std::fixed << std::setprecision(2)
         << "Fuel optimized route: " << route.total_distance << " units, "
         << "fuel cost: " << route.fuel_cost;
    route.route_description = desc.str();
    
    return route;
}

double FuelOptimizedStrategy::calculateFuelCost(
    const Coordinate3D& start,
    const Coordinate3D& end,
    double fleet_speed,
    double fleet_capacity
) const {
    double distance = start.distanceTo(end);
    double base_cost = distance * base_fuel_consumption_;
    
    // Factor in fleet capacity (larger fleets use more fuel)
    double capacity_multiplier = 1.0 + (fleet_capacity - 1.0) * 0.2;
    
    // Factor in speed (higher speeds require more fuel for acceleration)
    double speed_multiplier = 1.0 + (fleet_speed - 1.0) * acceleration_factor_;
    
    return base_cost * capacity_multiplier * speed_multiplier * 0.8; // 20% fuel savings
}

// Safe Route Strategy Implementation
RouteInfo SafeRouteStrategy::calculateRoute(
    const Coordinate3D& start,
    const Coordinate3D& destination,
    double fleet_speed,
    double fleet_capacity
) const {
    RouteInfo route;
    std::vector<Coordinate3D> safe_waypoints;
    safe_waypoints.push_back(start);
    
    // Calculate risk factor for direct path
    double direct_risk = calculateRiskFactor(start, destination);
    
    if (direct_risk > risk_threshold_) {
        // Need to find safer path by avoiding hazardous regions
        // Simplified approach: add waypoints that go around dangerous areas
        
        // Find the most dangerous region that intersects with direct path
        double max_detour = 0.0;
        Coordinate3D detour_point = destination;
        
        for (const auto& hazard : hazardous_regions_) {
            // Calculate if direct path intersects with hazardous region
            double distance_to_hazard = hazard.center.distanceTo(start);
            if (distance_to_hazard < hazard.radius + safety_margin_) {
                // Calculate detour point
                double detour_distance = hazard.radius + safety_margin_;
                if (detour_distance > max_detour) {
                    max_detour = detour_distance;
                    
                    // Simple detour calculation (perpendicular to hazard center)
                    double dx = destination.x - start.x;
                    double dy = destination.y - start.y;
                    double dz = destination.z - start.z;
                    double length = std::sqrt(dx*dx + dy*dy + dz*dz);
                    
                    if (length > 0) {
                        detour_point = Coordinate3D{
                            hazard.center.x + (dy / length) * detour_distance,
                            hazard.center.y - (dx / length) * detour_distance,
                            hazard.center.z + (dz / length) * detour_distance * 0.5
                        };
                    }
                }
            }
        }
        
        if (max_detour > 0.0) {
            safe_waypoints.push_back(detour_point);
        }
    }
    
    safe_waypoints.push_back(destination);
    route.waypoints = safe_waypoints;
    
    // Calculate total distance
    route.total_distance = 0.0;
    for (size_t i = 0; i < safe_waypoints.size() - 1; ++i) {
        route.total_distance += safe_waypoints[i].distanceTo(safe_waypoints[i + 1]);
    }
    
    route.estimated_time = route.total_distance / fleet_speed * 1.1; // 10% time penalty for safety
    route.fuel_cost = route.total_distance * 1.1; // 10% fuel penalty
    route.risk_factor = std::max(0.05, direct_risk - 0.2); // Reduced risk
    
    std::ostringstream desc;
    desc << std::fixed << std::setprecision(2)
         << "Safe route: " << route.total_distance << " units, "
         << "risk factor: " << route.risk_factor;
    route.route_description = desc.str();
    
    return route;
}

double SafeRouteStrategy::calculateRiskFactor(const Coordinate3D& start, const Coordinate3D& end) const {
    double max_risk = 0.0;
    
    // Sample points along the route to check for hazards
    const int samples = 20;
    for (int i = 0; i <= samples; ++i) {
        double t = static_cast<double>(i) / samples;
        Coordinate3D sample_point{
            start.x + t * (end.x - start.x),
            start.y + t * (end.y - start.y),
            start.z + t * (end.z - start.z)
        };
        
        double point_risk = getPointRisk(sample_point);
        max_risk = std::max(max_risk, point_risk);
    }
    
    return max_risk;
}

double SafeRouteStrategy::getPointRisk(const Coordinate3D& point) const {
    double total_risk = 0.0;
    
    for (const auto& hazard : hazardous_regions_) {
        double distance = point.distanceTo(hazard.center);
        if (distance < hazard.radius) {
            // Inside hazardous region
            double proximity_factor = 1.0 - (distance / hazard.radius);
            total_risk += hazard.risk_level * proximity_factor;
        }
    }
    
    return std::min(total_risk, 1.0); // Cap at maximum risk
}

// Balanced Strategy Implementation
RouteInfo BalancedStrategy::calculateRoute(
    const Coordinate3D& start,
    const Coordinate3D& destination,
    double fleet_speed,
    double fleet_capacity
) const {
    // Generate multiple route options using different approaches
    DirectLineStrategy direct_strategy;
    FuelOptimizedStrategy fuel_strategy;
    SafeRouteStrategy safe_strategy;
    
    RouteInfo direct_route = direct_strategy.calculateRoute(start, destination, fleet_speed, fleet_capacity);
    RouteInfo fuel_route = fuel_strategy.calculateRoute(start, destination, fleet_speed, fleet_capacity);
    RouteInfo safe_route = safe_strategy.calculateRoute(start, destination, fleet_speed, fleet_capacity);
    
    // Calculate weighted scores for each route
    double direct_score = calculateWeightedScore(direct_route);
    double fuel_score = calculateWeightedScore(fuel_route);
    double safe_score = calculateWeightedScore(safe_route);
    
    // Select the route with the best (lowest) score
    RouteInfo best_route;
    std::string chosen_approach;
    
    if (direct_score <= fuel_score && direct_score <= safe_score) {
        best_route = direct_route;
        chosen_approach = "direct";
    } else if (fuel_score <= safe_score) {
        best_route = fuel_route;
        chosen_approach = "fuel-optimized";
    } else {
        best_route = safe_route;
        chosen_approach = "safe";
    }
    
    std::ostringstream desc;
    desc << std::fixed << std::setprecision(2)
         << "Balanced route (" << chosen_approach << "): " 
         << best_route.total_distance << " units, "
         << "score: " << calculateWeightedScore(best_route);
    best_route.route_description = desc.str();
    
    return best_route;
}

double BalancedStrategy::calculateWeightedScore(const RouteInfo& route) const {
    // Normalize metrics to similar scales for fair comparison
    double time_score = route.estimated_time * time_weight_;
    double fuel_score = route.fuel_cost * fuel_weight_;
    double safety_score = route.risk_factor * 100.0 * safety_weight_; // Scale risk to similar range
    
    return time_score + fuel_score + safety_score;
}

// Fleet Router Implementation
FleetRouter::FleetRouter(std::unique_ptr<IRoutingStrategy> default_strategy)
    : current_strategy_(std::move(default_strategy)) {
    
    if (!current_strategy_) {
        current_strategy_ = std::make_unique<DirectLineStrategy>();
    }
}

RouteInfo FleetRouter::calculateRoute(
    const Coordinate3D& start,
    const Coordinate3D& destination,
    double fleet_speed,
    double fleet_capacity
) const {
    if (!current_strategy_) {
        return RouteInfo{}; // Return empty route if no strategy set
    }
    
    return current_strategy_->calculateRoute(start, destination, fleet_speed, fleet_capacity);
}

RouteInfo FleetRouter::calculateMultiWaypointRoute(
    const std::vector<Coordinate3D>& waypoints,
    double fleet_speed,
    double fleet_capacity
) const {
    if (!current_strategy_) {
        return RouteInfo{}; // Return empty route if no strategy set
    }
    
    return current_strategy_->calculateMultiWaypointRoute(waypoints, fleet_speed, fleet_capacity);
}

std::unordered_map<std::string, RouteInfo> FleetRouter::compareStrategies(
    const std::vector<std::unique_ptr<IRoutingStrategy>>& strategies,
    const Coordinate3D& start,
    const Coordinate3D& destination,
    double fleet_speed,
    double fleet_capacity
) const {
    std::unordered_map<std::string, RouteInfo> results;
    
    for (const auto& strategy : strategies) {
        if (strategy) {
            RouteInfo route = strategy->calculateRoute(start, destination, fleet_speed, fleet_capacity);
            results[strategy->getStrategyName()] = route;
        }
    }
    
    return results;
}

std::string FleetRouter::findBestStrategy(
    const std::vector<std::unique_ptr<IRoutingStrategy>>& strategies,
    const Coordinate3D& start,
    const Coordinate3D& destination,
    std::function<double(const RouteInfo&)> criteria_function,
    double fleet_speed,
    double fleet_capacity
) const {
    std::string best_strategy_name;
    double best_score = std::numeric_limits<double>::max();
    
    for (const auto& strategy : strategies) {
        if (strategy) {
            RouteInfo route = strategy->calculateRoute(start, destination, fleet_speed, fleet_capacity);
            double score = criteria_function(route);
            
            if (score < best_score) {
                best_score = score;
                best_strategy_name = strategy->getStrategyName();
            }
        }
    }
    
    return best_strategy_name;
}

// Strategy Factory Implementation
std::unique_ptr<IRoutingStrategy> RoutingStrategyFactory::createStrategy(
    StrategyType type,
    const std::unordered_map<std::string, double>& parameters
) {
    switch (type) {
        case StrategyType::DirectLine:
            return std::make_unique<DirectLineStrategy>();
            
        case StrategyType::FuelOptimized: {
            double base_consumption = 1.0;
            double acceleration_factor = 1.5;
            
            auto it = parameters.find("base_consumption");
            if (it != parameters.end()) {
                base_consumption = it->second;
            }
            
            it = parameters.find("acceleration_factor");
            if (it != parameters.end()) {
                acceleration_factor = it->second;
            }
            
            return std::make_unique<FuelOptimizedStrategy>(base_consumption, acceleration_factor);
        }
        
        case StrategyType::SafeRoute: {
            double risk_threshold = 0.3;
            double safety_margin = 10.0;
            
            auto it = parameters.find("risk_threshold");
            if (it != parameters.end()) {
                risk_threshold = it->second;
            }
            
            it = parameters.find("safety_margin");
            if (it != parameters.end()) {
                safety_margin = it->second;
            }
            
            return std::make_unique<SafeRouteStrategy>(risk_threshold, safety_margin);
        }
        
        case StrategyType::Balanced: {
            double time_weight = 0.4;
            double fuel_weight = 0.3;
            double safety_weight = 0.3;
            
            auto it = parameters.find("time_weight");
            if (it != parameters.end()) {
                time_weight = it->second;
            }
            
            it = parameters.find("fuel_weight");
            if (it != parameters.end()) {
                fuel_weight = it->second;
            }
            
            it = parameters.find("safety_weight");
            if (it != parameters.end()) {
                safety_weight = it->second;
            }
            
            return std::make_unique<BalancedStrategy>(time_weight, fuel_weight, safety_weight);
        }
        
        default:
            return std::make_unique<DirectLineStrategy>();
    }
}

std::vector<RoutingStrategyFactory::StrategyType> RoutingStrategyFactory::getAvailableStrategies() {
    return {
        StrategyType::DirectLine,
        StrategyType::FuelOptimized,
        StrategyType::SafeRoute,
        StrategyType::Balanced
    };
}

std::string RoutingStrategyFactory::strategyTypeToString(StrategyType type) {
    switch (type) {
        case StrategyType::DirectLine:
            return "DirectLine";
        case StrategyType::FuelOptimized:
            return "FuelOptimized";
        case StrategyType::SafeRoute:
            return "SafeRoute";
        case StrategyType::Balanced:
            return "Balanced";
        default:
            return "Unknown";
    }
}

} // namespace CppVerseHub::Patterns