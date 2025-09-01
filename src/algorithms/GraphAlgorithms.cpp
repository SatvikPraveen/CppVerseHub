/**
 * @file GraphAlgorithms.cpp
 * @brief Implementation of graph algorithms for space navigation and route optimization
 * @details File location: src/algorithms/GraphAlgorithms.cpp
 */

#include "GraphAlgorithms.hpp"
#include <random>
#include <iomanip>
#include <sstream>
#include <fstream>

namespace CppVerseHub::Algorithms {

    // Static member definitions
    std::vector<SpaceRoute> SpaceGraph::empty_routes_;

    // ========== SpaceStation Implementation ==========

    SpaceStation::SpaceStation(size_t id, const std::string& name, const SpaceCoordinate& position, StationType type)
        : id_(id), name_(name), position_(position), type_(type), can_refuel_(false), 
          can_repair_(false), danger_level_(0.0), docking_capacity_(5) {
        
        // Set default capabilities based on station type
        switch (type_) {
            case StationType::FUEL_DEPOT:
                can_refuel_ = true;
                docking_capacity_ = 20;
                break;
            case StationType::SPACE_STATION:
                can_refuel_ = true;
                can_repair_ = true;
                docking_capacity_ = 15;
                break;
            case StationType::PLANET:
                can_refuel_ = true;
                can_repair_ = true;
                docking_capacity_ = 50;
                break;
            case StationType::RESEARCH_OUTPOST:
                can_repair_ = true;
                docking_capacity_ = 8;
                danger_level_ = 0.2; // Research areas might have some risk
                break;
            case StationType::ASTEROID_BASE:
                danger_level_ = 0.4; // Asteroid fields are dangerous
                docking_capacity_ = 3;
                break;
            default:
                break;
        }
    }

    std::string SpaceStation::type_to_string() const {
        switch (type_) {
            case StationType::PLANET: return "Planet";
            case StationType::MOON: return "Moon";
            case StationType::SPACE_STATION: return "Space Station";
            case StationType::ASTEROID_BASE: return "Asteroid Base";
            case StationType::RESEARCH_OUTPOST: return "Research Outpost";
            case StationType::FUEL_DEPOT: return "Fuel Depot";
            case StationType::TRADING_POST: return "Trading Post";
            default: return "Unknown";
        }
    }

    void SpaceStation::print_info() const {
        std::cout << "Station " << id_ << ": " << name_ << "\n";
        std::cout << "  Type: " << type_to_string() << "\n";
        std::cout << "  Position: (" << std::fixed << std::setprecision(1) 
                  << position_.x << ", " << position_.y << ", " << position_.z << ")\n";
        std::cout << "  Capabilities: ";
        if (can_refuel_) std::cout << "Refuel ";
        if (can_repair_) std::cout << "Repair ";
        std::cout << "\n  Danger Level: " << std::fixed << std::setprecision(2) 
                  << danger_level_ << "\n";
        std::cout << "  Docking Capacity: " << docking_capacity_ << "\n\n";
    }

    // ========== SpaceGraph Implementation ==========

    SpaceGraph::SpaceGraph(bool directed) : directed_(directed) {}

    size_t SpaceGraph::add_station(const std::string& name, const SpaceCoordinate& position, 
                                  SpaceStation::StationType type) {
        size_t id = stations_.size();
        stations_.emplace_back(id, name, position, type);
        adjacency_list_.emplace_back();
        return id;
    }

    void SpaceGraph::add_route(size_t from, size_t to, double fuel_cost, double time_cost,
                              double danger_level, bool requires_clearance, const std::string& route_type) {
        if (from >= stations_.size() || to >= stations_.size()) {
            return;
        }
        
        adjacency_list_[from].emplace_back(from, to, fuel_cost, time_cost, danger_level, 
                                          requires_clearance, route_type);
        
        if (!directed_) {
            adjacency_list_[to].emplace_back(to, from, fuel_cost, time_cost, danger_level, 
                                            requires_clearance, route_type);
        }
    }

    const SpaceStation& SpaceGraph::get_station(size_t id) const {
        static SpaceStation invalid_station(SIZE_MAX, "Invalid", {0, 0, 0});
        return id < stations_.size() ? stations_[id] : invalid_station;
    }

    const std::vector<SpaceRoute>& SpaceGraph::get_routes_from(size_t station_id) const {
        return station_id < adjacency_list_.size() ? adjacency_list_[station_id] : empty_routes_;
    }

    void SpaceGraph::generate_realistic_space_network() {
        // Clear existing data
        stations_.clear();
        adjacency_list_.clear();
        
        // Create a realistic space network with planets, moons, and stations
        
        // Central system (Sol-like)
        auto earth = add_station("Earth", {0, 0, 0}, SpaceStation::StationType::PLANET);
        auto mars = add_station("Mars", {200, 50, 0}, SpaceStation::StationType::PLANET);
        auto jupiter = add_station("Jupiter", {800, 100, 50}, SpaceStation::StationType::PLANET);
        
        // Moons
        auto luna = add_station("Luna", {15, 2, 0}, SpaceStation::StationType::MOON);
        auto phobos = add_station("Phobos", {205, 48, 2}, SpaceStation::StationType::MOON);
        auto europa = add_station("Europa", {790, 95, 45}, SpaceStation::StationType::MOON);
        
        // Space stations
        auto station_alpha = add_station("Station Alpha", {100, 25, 10}, SpaceStation::StationType::SPACE_STATION);
        auto station_beta = add_station("Station Beta", {400, 75, 20}, SpaceStation::StationType::SPACE_STATION);
        auto station_gamma = add_station("Station Gamma", {600, 85, 30}, SpaceStation::StationType::SPACE_STATION);
        
        // Specialized facilities
        auto fuel_depot_1 = add_station("Fuel Depot Cerberus", {300, 60, 15}, SpaceStation::StationType::FUEL_DEPOT);
        auto research_outpost = add_station("Deep Space Research", {1000, 200, 100}, SpaceStation::StationType::RESEARCH_OUTPOST);
        auto asteroid_base = add_station("Asteroid Mining Base", {450, 200, 80}, SpaceStation::StationType::ASTEROID_BASE);
        
        // Generate routes with realistic costs
        generate_complete_routes(1500.0); // Maximum realistic range
        
        // Add some emergency routes
        add_emergency_routes();
        
        std::cout << "Generated realistic space network with " << stations_.size() << " stations\n";
    }

    void SpaceGraph::generate_complete_routes(double max_distance) {
        for (size_t i = 0; i < stations_.size(); ++i) {
            for (size_t j = i + 1; j < stations_.size(); ++j) {
                double distance = stations_[i].get_position().distance_to(stations_[j].get_position());
                
                if (distance <= max_distance) {
                    double fuel_cost = calculate_realistic_fuel_cost(stations_[i].get_position(), 
                                                                   stations_[j].get_position());
                    double time_cost = calculate_travel_time(stations_[i].get_position(), 
                                                           stations_[j].get_position());
                    
                    // Add some randomness to danger levels
                    std::random_device rd;
                    std::mt19937 gen(rd());
                    std::uniform_real_distribution<> danger_dist(0.0, 0.3);
                    double danger = danger_dist(gen);
                    
                    add_route(i, j, fuel_cost, time_cost, danger);
                }
            }
        }
    }

    double SpaceGraph::calculate_realistic_fuel_cost(const SpaceCoordinate& from, const SpaceCoordinate& to) const {
        double distance = from.distance_to(to);
        // Fuel cost increases non-linearly with distance due to orbital mechanics
        return distance * 2.5 + std::pow(distance / 100.0, 1.2) * 50.0;
    }

    double SpaceGraph::calculate_travel_time(const SpaceCoordinate& from, const SpaceCoordinate& to, 
                                           const std::string& route_type) const {
        double distance = from.distance_to(to);
        double base_speed = 100.0; // Base speed units per time unit
        
        if (route_type == "emergency") {
            base_speed *= 1.5; // Emergency routes are faster but use more fuel
        } else if (route_type == "relay") {
            base_speed *= 0.8; // Relay routes are slower but more fuel efficient
        }
        
        return distance / base_speed;
    }

    void SpaceGraph::print_network() const {
        std::cout << "\n=== Space Network Overview ===\n";
        std::cout << "Stations: " << stations_.size() << std::endl;
        
        size_t total_routes = 0;
        for (const auto& routes : adjacency_list_) {
            total_routes += routes.size();
        }
        
        std::cout << "Routes: " << (directed_ ? total_routes : total_routes / 2) << std::endl;
        std::cout << "Network Type: " << (directed_ ? "Directed" : "Undirected") << "\n\n";
        
        for (size_t i = 0; i < std::min(stations_.size(), size_t(10)); ++i) {
            stations_[i].print_info();
        }
        
        if (stations_.size() > 10) {
            std::cout << "... and " << (stations_.size() - 10) << " more stations\n\n";
        }
    }

    // ========== SpacePathfinder Implementation ==========

    PathResult SpacePathfinder::dijkstra_shortest_path(size_t start, size_t destination, OptimizationGoal goal) {
        auto start_time = std::chrono::high_resolution_clock::now();
        
        if (start >= graph_.station_count() || destination >= graph_.station_count()) {
            return {"Dijkstra", {}, std::numeric_limits<double>::infinity(), 0, 0, 0, false, 0, 
                   std::chrono::microseconds(0), "Invalid vertices", {"Invalid start or destination"}};
        }
        
        std::vector<double> distance(graph_.station_count(), std::numeric_limits<double>::infinity());
        std::vector<size_t> parent(graph_.station_count(), SIZE_MAX);
        std::vector<bool> visited(graph_.station_count(), false);
        
        using PQElement = std::pair<double, size_t>;
        std::priority_queue<PQElement, std::vector<PQElement>, std::greater<PQElement>> pq;
        
        distance[start] = 0.0;
        pq.push({0.0, start});
        
        size_t nodes_explored = 0;
        
        while (!pq.empty()) {
            double current_dist = pq.top().first;
            size_t current = pq.top().second;
            pq.pop();
            
            if (visited[current]) continue;
            visited[current] = true;
            nodes_explored++;
            
            if (current == destination) break;
            
            for (const auto& route : graph_.get_routes_from(current)) {
                size_t neighbor = route.to_station;
                double edge_cost = calculate_route_cost(route, goal);
                double new_dist = current_dist + edge_cost;
                
                if (new_dist < distance[neighbor]) {
                    distance[neighbor] = new_dist;
                    parent[neighbor] = current;
                    pq.push({new_dist, neighbor});
                }
            }
        }
        
        auto end_time = std::chrono::high_resolution_clock::now();
        auto computation_time = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
        
        if (distance[destination] == std::numeric_limits<double>::infinity()) {
            return {"Dijkstra", {}, std::numeric_limits<double>::infinity(), 0, 0, 0, false, 
                   nodes_explored, computation_time, "No path found", {"Destination unreachable"}};
        }
        
        std::vector<size_t> path = reconstruct_path(
            std::unordered_map<size_t, size_t>(parent.begin(), parent.end()), start, destination);
        
        return create_path_result("Dijkstra", path, goal, computation_time, nodes_explored);
    }

    PathResult SpacePathfinder::a_star_pathfinding(size_t start, size_t destination, OptimizationGoal goal) {
        auto start_time = std::chrono::high_resolution_clock::now();
        
        if (start >= graph_.station_count() || destination >= graph_.station_count()) {
            return {"A*", {}, std::numeric_limits<double>::infinity(), 0, 0, 0, false, 0, 
                   std::chrono::microseconds(0), "Invalid vertices", {"Invalid start or destination"}};
        }
        
        std::vector<double> g_score(graph_.station_count(), std::numeric_limits<double>::infinity());
        std::vector<double> f_score(graph_.station_count(), std::numeric_limits<double>::infinity());
        std::vector<size_t> parent(graph_.station_count(), SIZE_MAX);
        
        using PQElement = std::pair<double, size_t>;
        std::priority_queue<PQElement, std::vector<PQElement>, std::greater<PQElement>> open_set;
        std::unordered_set<size_t> closed_set;
        
        g_score[start] = 0.0;
        f_score[start] = euclidean_heuristic(start, destination);
        open_set.push({f_score[start], start});
        
        size_t nodes_explored = 0;
        
        while (!open_set.empty()) {
            size_t current = open_set.top().second;
            open_set.pop();
            
            if (closed_set.count(current)) continue;
            closed_set.insert(current);
            nodes_explored++;
            
            if (current == destination) {
                auto end_time = std::chrono::high_resolution_clock::now();
                auto computation_time = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
                
                std::vector<size_t> path = reconstruct_path(
                    std::unordered_map<size_t, size_t>(parent.begin(), parent.end()), start, destination);
                
                return create_path_result("A*", path, goal, computation_time, nodes_explored);
            }
            
            for (const auto& route : graph_.get_routes_from(current)) {
                size_t neighbor = route.to_station;
                
                if (closed_set.count(neighbor)) continue;
                
                double tentative_g = g_score[current] + calculate_route_cost(route, goal);
                
                if (tentative_g < g_score[neighbor]) {
                    parent[neighbor] = current;
                    g_score[neighbor] = tentative_g;
                    f_score[neighbor] = tentative_g + euclidean_heuristic(neighbor, destination);
                    open_set.push({f_score[neighbor], neighbor});
                }
            }
        }
        
        auto end_time = std::chrono::high_resolution_clock::now();
        auto computation_time = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
        
        return {"A*", {}, std::numeric_limits<double>::infinity(), 0, 0, 0, false, 
               nodes_explored, computation_time, "No path found", {"Destination unreachable"}};
    }

    PathResult SpacePathfinder::find_safest_path(size_t start, size_t destination, double max_danger_threshold) {
        auto start_time = std::chrono::high_resolution_clock::now();
        
        // Use Dijkstra with safety as primary criterion
        std::vector<double> safety_score(graph_.station_count(), -std::numeric_limits<double>::infinity());
        std::vector<size_t> parent(graph_.station_count(), SIZE_MAX);
        std::vector<bool> visited(graph_.station_count(), false);
        
        using PQElement = std::pair<double, size_t>;
        std::priority_queue<PQElement> pq; // Max heap for safety scores
        
        safety_score[start] = 1.0; // Perfect safety at start
        pq.push({1.0, start});
        
        size_t nodes_explored = 0;
        std::vector<std::string> warnings;
        
        while (!pq.empty()) {
            double current_safety = pq.top().first;
            size_t current = pq.top().second;
            pq.pop();
            
            if (visited[current]) continue;
            visited[current] = true;
            nodes_explored++;
            
            if (current == destination) break;
            
            for (const auto& route : graph_.get_routes_from(current)) {
                size_t neighbor = route.to_station;
                double route_safety = 1.0 - route.danger_level;
                double new_safety = current_safety * route_safety;
                
                if (route.danger_level > max_danger_threshold) {
                    warnings.push_back("High danger route from " + 
                                     graph_.get_station(current).get_name() + " to " + 
                                     graph_.get_station(neighbor).get_name());
                }
                
                if (new_safety > safety_score[neighbor]) {
                    safety_score[neighbor] = new_safety;
                    parent[neighbor] = current;
                    pq.push({new_safety, neighbor});
                }
            }
        }
        
        auto end_time = std::chrono::high_resolution_clock::now();
        auto computation_time = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
        
        if (safety_score[destination] == -std::numeric_limits<double>::infinity()) {
            return {"Safest Path", {}, std::numeric_limits<double>::infinity(), 0, 0, 0, false, 
                   nodes_explored, computation_time, "No safe path found", warnings};
        }
        
        std::vector<size_t> path = reconstruct_path(
            std::unordered_map<size_t, size_t>(parent.begin(), parent.end()), start, destination);
        
        PathResult result = create_path_result("Safest Path", path, OptimizationGoal::MAXIMUM_SAFETY, 
                                              computation_time, nodes_explored);
        result.safety_score = safety_score[destination];
        result.warnings = warnings;
        
        return result;
    }

    double SpacePathfinder::euclidean_heuristic(size_t current, size_t destination) const {
        return graph_.get_station(current).get_position().distance_to(
               graph_.get_station(destination).get_position());
    }

    double SpacePathfinder::calculate_route_cost(const SpaceRoute& route, OptimizationGoal goal) const {
        switch (goal) {
            case OptimizationGoal::MINIMUM_FUEL:
                return route.fuel_cost;
            case OptimizationGoal::MINIMUM_TIME:
                return route.time_cost;
            case OptimizationGoal::MAXIMUM_SAFETY:
                return route.danger_level;
            case OptimizationGoal::BALANCED:
                return route.fuel_cost * 0.4 + route.time_cost * 0.4 + route.danger_level * 200.0;
            case OptimizationGoal::MINIMUM_HOPS:
                return 1.0;
            default:
                return route.fuel_cost + route.time_cost;
        }
    }

    std::vector<size_t> SpacePathfinder::reconstruct_path(const std::unordered_map<size_t, size_t>& parent,
                                                         size_t start, size_t destination) const {
        std::vector<size_t> path;
        size_t current = destination;
        
        while (current != SIZE_MAX && current != start) {
            path.push_back(current);
            auto it = parent.find(current);
            current = (it != parent.end()) ? it->second : SIZE_MAX;
        }
        
        if (current == start) {
            path.push_back(start);
            std::reverse(path.begin(), path.end());
        }
        
        return path;
    }

    PathResult SpacePathfinder::create_path_result(const std::string& algorithm_name, 
                                                  const std::vector<size_t>& path,
                                                  OptimizationGoal goal, 
                                                  std::chrono::microseconds computation_time,
                                                  size_t nodes_explored) const {
        PathResult result;
        result.algorithm_name = algorithm_name;
        result.path = path;
        result.path_found = !path.empty();
        result.nodes_explored = nodes_explored;
        result.computation_time = computation_time;
        
        if (!path.empty()) {
            result.total_cost = calculate_path_cost(path, goal);
            
            // Calculate detailed metrics
            double total_fuel = 0.0, total_time = 0.0, min_safety = 1.0;
            
            for (size_t i = 0; i < path.size() - 1; ++i) {
                for (const auto& route : graph_.get_routes_from(path[i])) {
                    if (route.to_station == path[i + 1]) {
                        total_fuel += route.fuel_cost;
                        total_time += route.time_cost;
                        min_safety = std::min(min_safety, 1.0 - route.danger_level);
                        break;
                    }
                }
            }
            
            result.fuel_consumption = total_fuel;
            result.travel_time = total_time;
            result.safety_score = min_safety;
        }
        
        // Set optimization criteria description
        switch (goal) {
            case OptimizationGoal::MINIMUM_FUEL:
                result.optimization_criteria = "Minimum Fuel";
                break;
            case OptimizationGoal::MINIMUM_TIME:
                result.optimization_criteria = "Minimum Time";
                break;
            case OptimizationGoal::MAXIMUM_SAFETY:
                result.optimization_criteria = "Maximum Safety";
                break;
            case OptimizationGoal::BALANCED:
                result.optimization_criteria = "Balanced (Fuel/Time/Safety)";
                break;
            case OptimizationGoal::MINIMUM_HOPS:
                result.optimization_criteria = "Minimum Hops";
                break;
        }
        
        return result;
    }

    double SpacePathfinder::calculate_path_cost(const std::vector<size_t>& path, OptimizationGoal goal) const {
        if (path.size() < 2) return 0.0;
        
        double total_cost = 0.0;
        
        for (size_t i = 0; i < path.size() - 1; ++i) {
            for (const auto& route : graph_.get_routes_from(path[i])) {
                if (route.to_station == path[i + 1]) {
                    total_cost += calculate_route_cost(route, goal);
                    break;
                }
            }
        }
        
        return total_cost;
    }

    // ========== GraphAlgorithmsDemo Implementation ==========

    void GraphAlgorithmsDemo::demonstrate_space_pathfinding() {
        print_section_header("Space Pathfinding Algorithms");
        
        auto space_network = create_sample_space_network();
        SpacePathfinder pathfinder(space_network);
        
        std::cout << "Finding optimal routes in space network...\n\n";
        
        size_t start_station = 0; // Earth
        size_t dest_station = space_network.station_count() - 1; // Farthest station
        
        // Test different optimization goals
        std::vector<SpacePathfinder::OptimizationGoal> goals = {
            SpacePathfinder::OptimizationGoal::MINIMUM_FUEL,
            SpacePathfinder::OptimizationGoal::MINIMUM_TIME,
            SpacePathfinder::OptimizationGoal::MAXIMUM_SAFETY,
            SpacePathfinder::OptimizationGoal::BALANCED
        };
        
        for (auto goal : goals) {
            auto result = pathfinder.dijkstra_shortest_path(start_station, dest_station, goal);
            print_path_result(result, space_network);
        }
        
        // Test A* algorithm
        std::cout << "Comparing with A* algorithm:\n";
        auto a_star_result = pathfinder.a_star_pathfinding(start_station, dest_station, 
                                                         SpacePathfinder::OptimizationGoal::BALANCED);
        print_path_result(a_star_result, space_network);
        
        // Test safest path
        std::cout << "Finding safest path with danger threshold 0.3:\n";
        auto safest_result = pathfinder.find_safest_path(start_station, dest_station, 0.3);
        print_path_result(safest_result, space_network);
        
        print_section_footer();
    }

    void GraphAlgorithmsDemo::demonstrate_network_analysis() {
        print_section_header("Space Network Analysis");
        
        auto space_network = create_sample_space_network();
        SpaceNetworkAnalysis analyzer(space_network);
        
        std::cout << "Analyzing space network topology...\n\n";
        
        // Network properties
        double density = analyzer.calculate_network_density();
        std::cout << "📊 Network Density: " << std::fixed << std::setprecision(3) << density << std::endl;
        
        // Find critical stations
        auto critical_stations = analyzer.identify_critical_stations();
        std::cout << "🏛️  Critical Stations: ";
        for (size_t i = 0; i < std::min(critical_stations.size(), size_t(5)); ++i) {
            if (i > 0) std::cout << ", ";
            std::cout << space_network.get_station(critical_stations[i]).get_name();
        }
        std::cout << std::endl;
        
        // Network resilience analysis
        auto resilience = analyzer.analyze_network_resilience();
        std::cout << "🛡️  Network Resilience Score: " << std::fixed << std::setprecision(2) 
                  << resilience.connectivity_score << std::endl;
        std::cout << "🔄 Redundant Paths: " << resilience.redundant_paths_count << std::endl;
        std::cout << "⚠️  Single Point Failures: " << resilience.single_point_failures.size() << std::endl;
        
        // Optimization suggestions
        auto optimization = analyzer.suggest_network_improvements();
        std::cout << "\n💡 Optimization Suggestions:\n";
        std::cout << "   New Routes Recommended: " << optimization.recommended_new_routes.size() << std::endl;
        std::cout << "   Routes to Upgrade: " << optimization.routes_to_upgrade.size() << std::endl;
        std::cout << "   Stations Needing Expansion: " << optimization.stations_needing_expansion.size() << std::endl;
        
        print_section_footer();
    }

    void GraphAlgorithmsDemo::run_comprehensive_graph_demo() {
        std::cout << "\n🎯 =============================================\n";
        std::cout << "🎯 COMPREHENSIVE GRAPH ALGORITHMS DEMONSTRATION\n";
        std::cout << "🎯 =============================================\n\n";
        
        demonstrate_space_pathfinding();
        demonstrate_network_analysis();
        
        std::cout << "\n🎉 ===================================\n";
        std::cout << "🎉 ALL GRAPH DEMONSTRATIONS COMPLETED!\n";
        std::cout << "🎉 ===================================\n\n";
    }

    void GraphAlgorithmsDemo::print_section_header(const std::string& title) {
        std::cout << "\n" << std::string(60, '=') << std::endl;
        std::cout << "🌌 " << title << std::endl;
        std::cout << std::string(60, '=') << std::endl << std::endl;
    }

    void GraphAlgorithmsDemo::print_section_footer() {
        std::cout << std::string(60, '-') << std::endl;
        std::cout << "✅ Section Complete\n" << std::endl;
    }

    void GraphAlgorithmsDemo::print_path_result(const PathResult& result, const SpaceGraph& graph) {
        std::cout << "🚀 " << result.algorithm_name << " Results:\n";
        std::cout << "   Path Found: " << (result.path_found ? "Yes" : "No") << std::endl;
        
        if (result.path_found) {
            std::cout << "   Route: ";
            for (size_t i = 0; i < result.path.size(); ++i) {
                if (i > 0) std::cout << " → ";
                std::cout << graph.get_station(result.path[i]).get_name();
            }
            std::cout << std::endl;
            
            std::cout << "   Optimization: " << result.optimization_criteria << std::endl;
            std::cout << "   Total Cost: " << std::fixed << std::setprecision(1) << result.total_cost << std::endl;
            std::cout << "   Fuel Consumption: " << std::fixed << std::setprecision(1) << result.fuel_consumption << std::endl;
            std::cout << "   Travel Time: " << std::fixed << std::setprecision(1) << result.travel_time << std::endl;
            std::cout << "   Safety Score: " << std::fixed << std::setprecision(3) << result.safety_score << std::endl;
        }
        
        std::cout << "   Computation Time: " << result.computation_time.count() << " μs" << std::endl;
        std::cout << "   Nodes Explored: " << result.nodes_explored << std::endl;
        
        if (!result.warnings.empty()) {
            std::cout << "   Warnings: " << result.warnings.size() << " issues detected" << std::endl;
        }
        
        std::cout << std::endl;
    }

    SpaceGraph GraphAlgorithmsDemo::create_sample_space_network() {
        SpaceGraph graph(false); // Undirected graph
        
        // Generate a realistic space network
        graph.generate_realistic_space_network();
        
        return graph;
    }

    // ========== Placeholder implementations for demonstration ==========
    
    std::vector<double> SpaceNetworkAnalysis::calculate_betweenness_centrality() {
        // Placeholder implementation
        std::vector<double> centrality(graph_.station_count(), 0.5);
        return centrality;
    }

    double SpaceNetworkAnalysis::calculate_network_density() {
        size_t n = graph_.station_count();
        if (n < 2) return 0.0;
        
        size_t edge_count = 0;
        for (size_t i = 0; i < n; ++i) {
            edge_count += graph_.get_routes_from(i).size();
        }
        
        size_t max_edges = n * (n - 1);
        return static_cast<double>(edge_count) / max_edges;
    }

    std::vector<size_t> SpaceNetworkAnalysis::identify_critical_stations() {
        // Identify stations with high connectivity
        std::vector<std::pair<size_t, size_t>> station_connectivity;
        
        for (size_t i = 0; i < graph_.station_count(); ++i) {
            size_t connections = graph_.get_routes_from(i).size();
            station_connectivity.push_back({connections, i});
        }
        
        std::sort(station_connectivity.rbegin(), station_connectivity.rend());
        
        std::vector<size_t> critical_stations;
        size_t num_critical = std::min(size_t(3), graph_.station_count());
        
        for (size_t i = 0; i < num_critical; ++i) {
            critical_stations.push_back(station_connectivity[i].second);
        }
        
        return critical_stations;
    }

    SpaceNetworkAnalysis::NetworkResilience SpaceNetworkAnalysis::analyze_network_resilience() {
        NetworkResilience resilience;
        
        // Calculate basic resilience metrics
        size_t total_stations = graph_.station_count();
        size_t total_routes = 0;
        
        for (size_t i = 0; i < total_stations; ++i) {
            total_routes += graph_.get_routes_from(i).size();
        }
        
        // Simple resilience calculation
        resilience.connectivity_score = total_stations > 0 ? 
            static_cast<double>(total_routes) / (total_stations * 2) : 0.0;
        
        resilience.redundant_paths_count = total_routes / 2; // Assuming undirected graph
        resilience.average_path_length = 3.5; // Placeholder
        
        // Find stations with only one connection (single point failures)
        for (size_t i = 0; i < total_stations; ++i) {
            if (graph_.get_routes_from(i).size() == 1) {
                resilience.single_point_failures.push_back(i);
            }
        }
        
        return resilience;
    }

    SpaceNetworkAnalysis::RouteOptimization SpaceNetworkAnalysis::suggest_network_improvements() {
        RouteOptimization optimization;
        
        // Find stations that need more connections
        for (size_t i = 0; i < graph_.station_count(); ++i) {
            size_t connections = graph_.get_routes_from(i).size();
            
            if (connections < 2) {
                optimization.stations_needing_expansion.push_back(i);
            }
            
            // Suggest connections to nearby stations without direct routes
            for (size_t j = i + 1; j < graph_.station_count(); ++j) {
                bool has_direct_route = false;
                for (const auto& route : graph_.get_routes_from(i)) {
                    if (route.to_station == j) {
                        has_direct_route = true;
                        break;
                    }
                }
                
                if (!has_direct_route && connections < 4) {
                    double distance = graph_.get_station(i).get_position().distance_to(
                                    graph_.get_station(j).get_position());
                    if (distance < 300.0) { // Reasonable connection distance
                        optimization.recommended_new_routes.push_back({i, j});
                    }
                }
            }
        }
        
        return optimization;
    }

} // namespace CppVerseHub::Algorithms