/**
 * @file GraphAlgorithms.hpp
 * @brief Graph algorithms for pathfinding and space route optimization
 * @details File location: src/algorithms/GraphAlgorithms.hpp
 * 
 * This file contains implementations of graph algorithms specifically designed
 * for space navigation, route optimization, and pathfinding in 3D space with
 * considerations for fuel efficiency, time optimization, and obstacle avoidance.
 */

#ifndef GRAPHALGORITHMS_HPP
#define GRAPHALGORITHMS_HPP

#include <vector>
#include <queue>
#include <stack>
#include <unordered_map>
#include <unordered_set>
#include <algorithm>
#include <chrono>
#include <functional>
#include <memory>
#include <optional>
#include <limits>
#include <iostream>
#include <string>
#include <cmath>
#include <array>

namespace CppVerseHub::Algorithms {

    /**
     * @struct SpaceCoordinate
     * @brief Represents a 3D coordinate in space
     */
    struct SpaceCoordinate {
        double x, y, z;
        
        SpaceCoordinate(double x = 0, double y = 0, double z = 0) : x(x), y(y), z(z) {}
        
        double distance_to(const SpaceCoordinate& other) const {
            return std::sqrt(std::pow(x - other.x, 2) + std::pow(y - other.y, 2) + std::pow(z - other.z, 2));
        }
        
        SpaceCoordinate operator+(const SpaceCoordinate& other) const {
            return {x + other.x, y + other.y, z + other.z};
        }
        
        SpaceCoordinate operator-(const SpaceCoordinate& other) const {
            return {x - other.x, y - other.y, z - other.z};
        }
        
        bool operator==(const SpaceCoordinate& other) const {
            const double epsilon = 1e-9;
            return std::abs(x - other.x) < epsilon && 
                   std::abs(y - other.y) < epsilon && 
                   std::abs(z - other.z) < epsilon;
        }
    };

    /**
     * @struct SpaceRoute
     * @brief Represents a route in space with associated costs
     */
    struct SpaceRoute {
        size_t from_station;
        size_t to_station;
        double fuel_cost;
        double time_cost;
        double danger_level;
        bool requires_special_clearance;
        std::string route_type; // "direct", "relay", "emergency"
        
        SpaceRoute(size_t from, size_t to, double fuel = 0.0, double time = 0.0, 
                  double danger = 0.0, bool clearance = false, const std::string& type = "direct")
            : from_station(from), to_station(to), fuel_cost(fuel), time_cost(time), 
              danger_level(danger), requires_special_clearance(clearance), route_type(type) {}
    };

    /**
     * @struct PathResult
     * @brief Results from pathfinding algorithms
     */
    struct PathResult {
        std::string algorithm_name;
        std::vector<size_t> path;
        double total_cost;
        double fuel_consumption;
        double travel_time;
        double safety_score;
        bool path_found;
        size_t nodes_explored;
        std::chrono::microseconds computation_time;
        std::string optimization_criteria;
        std::vector<std::string> warnings;
    };

    /**
     * @class SpaceStation
     * @brief Represents a space station or celestial body
     */
    class SpaceStation {
    public:
        enum class StationType {
            PLANET,
            MOON,
            SPACE_STATION,
            ASTEROID_BASE,
            RESEARCH_OUTPOST,
            FUEL_DEPOT,
            TRADING_POST
        };

        SpaceStation(size_t id, const std::string& name, const SpaceCoordinate& position,
                    StationType type = StationType::SPACE_STATION);
        
        // Getters
        size_t get_id() const { return id_; }
        const std::string& get_name() const { return name_; }
        const SpaceCoordinate& get_position() const { return position_; }
        StationType get_type() const { return type_; }
        bool can_refuel() const { return can_refuel_; }
        bool can_repair() const { return can_repair_; }
        double get_danger_level() const { return danger_level_; }
        
        // Setters
        void set_refuel_capability(bool can_refuel) { can_refuel_ = can_refuel; }
        void set_repair_capability(bool can_repair) { can_repair_ = can_repair; }
        void set_danger_level(double level) { danger_level_ = std::max(0.0, std::min(1.0, level)); }
        void set_docking_capacity(size_t capacity) { docking_capacity_ = capacity; }
        
        std::string type_to_string() const;
        void print_info() const;

    private:
        size_t id_;
        std::string name_;
        SpaceCoordinate position_;
        StationType type_;
        bool can_refuel_;
        bool can_repair_;
        double danger_level_;  // 0.0 = safe, 1.0 = extremely dangerous
        size_t docking_capacity_;
    };

    /**
     * @class SpaceGraph
     * @brief Graph representation of space routes and stations
     */
    class SpaceGraph {
    public:
        explicit SpaceGraph(bool directed = false);
        
        // Station management
        size_t add_station(const std::string& name, const SpaceCoordinate& position,
                          SpaceStation::StationType type = SpaceStation::StationType::SPACE_STATION);
        
        void add_route(size_t from, size_t to, double fuel_cost, double time_cost,
                      double danger_level = 0.0, bool requires_clearance = false,
                      const std::string& route_type = "direct");
        
        // Getters
        size_t station_count() const { return stations_.size(); }
        const SpaceStation& get_station(size_t id) const;
        const std::vector<SpaceRoute>& get_routes_from(size_t station_id) const;
        
        // Graph operations
        void generate_complete_routes(double max_distance = 1000.0);
        void generate_realistic_space_network();
        void add_emergency_routes();
        
        // Visualization
        void print_network() const;
        void print_station_info(size_t station_id) const;

    private:
        std::vector<SpaceStation> stations_;
        std::vector<std::vector<SpaceRoute>> adjacency_list_;
        bool directed_;
        static std::vector<SpaceRoute> empty_routes_;
        
        double calculate_realistic_fuel_cost(const SpaceCoordinate& from, const SpaceCoordinate& to) const;
        double calculate_travel_time(const SpaceCoordinate& from, const SpaceCoordinate& to, 
                                   const std::string& route_type = "direct") const;
    };

    /**
     * @class SpacePathfinder
     * @brief Advanced pathfinding algorithms for space navigation
     */
    class SpacePathfinder {
    public:
        enum class OptimizationGoal {
            MINIMUM_FUEL,
            MINIMUM_TIME,
            MAXIMUM_SAFETY,
            BALANCED,
            MINIMUM_HOPS
        };

        explicit SpacePathfinder(const SpaceGraph& graph) : graph_(graph) {}
        
        // Core pathfinding algorithms
        PathResult dijkstra_shortest_path(size_t start, size_t destination, OptimizationGoal goal = OptimizationGoal::BALANCED);
        
        PathResult a_star_pathfinding(size_t start, size_t destination, OptimizationGoal goal = OptimizationGoal::BALANCED);
        
        PathResult bidirectional_search(size_t start, size_t destination, OptimizationGoal goal = OptimizationGoal::BALANCED);
        
        PathResult multi_objective_pathfinding(size_t start, size_t destination,
                                             std::vector<OptimizationGoal> goals,
                                             std::vector<double> weights);
        
        // Advanced pathfinding
        PathResult find_path_with_fuel_stops(size_t start, size_t destination, double max_fuel_range);
        
        PathResult find_safest_path(size_t start, size_t destination, double max_danger_threshold = 0.3);
        
        PathResult find_emergency_evacuation_path(size_t start, size_t destination, 
                                                 const std::vector<size_t>& damaged_stations = {});
        
        std::vector<PathResult> find_k_shortest_paths(size_t start, size_t destination, size_t k = 3,
                                                     OptimizationGoal goal = OptimizationGoal::BALANCED);

        // Multi-destination routing
        PathResult traveling_salesman_space_route(size_t start, const std::vector<size_t>& destinations);
        
        PathResult multi_destination_optimal_route(size_t start, const std::vector<size_t>& destinations,
                                                  bool return_to_start = true);

    private:
        const SpaceGraph& graph_;
        
        // Heuristic functions for A*
        double euclidean_heuristic(size_t current, size_t destination) const;
        double fuel_heuristic(size_t current, size_t destination) const;
        double time_heuristic(size_t current, size_t destination) const;
        double safety_heuristic(size_t current, size_t destination) const;
        
        // Cost calculation functions
        double calculate_route_cost(const SpaceRoute& route, OptimizationGoal goal) const;
        double calculate_path_cost(const std::vector<size_t>& path, OptimizationGoal goal) const;
        
        // Utility functions
        std::vector<size_t> reconstruct_path(const std::unordered_map<size_t, size_t>& parent,
                                           size_t start, size_t destination) const;
        
        PathResult create_path_result(const std::string& algorithm_name, const std::vector<size_t>& path,
                                    OptimizationGoal goal, std::chrono::microseconds computation_time,
                                    size_t nodes_explored = 0) const;
    };

    /**
     * @class FlowNetwork
     * @brief Maximum flow algorithms for space traffic management
     */
    class FlowNetwork {
    public:
        struct FlowEdge {
            size_t from, to;
            double capacity;
            double flow;
            
            FlowEdge(size_t f, size_t t, double cap) : from(f), to(t), capacity(cap), flow(0.0) {}
        };

        explicit FlowNetwork(size_t num_vertices);
        
        void add_edge(size_t from, size_t to, double capacity);
        
        double max_flow(size_t source, size_t sink);
        double min_cut(size_t source, size_t sink);
        
        std::vector<FlowEdge> get_min_cut_edges(size_t source, size_t sink);
        
        // Space-specific flow problems
        double calculate_max_evacuation_rate(size_t danger_zone, const std::vector<size_t>& safe_zones);
        
        std::vector<size_t> find_bottleneck_stations();
        
        void print_flow_network() const;

    private:
        size_t num_vertices_;
        std::vector<std::vector<size_t>> graph_;
        std::vector<FlowEdge> edges_;
        
        bool bfs_residual_graph(size_t source, size_t sink, std::vector<int>& parent);
        void dfs_min_cut(size_t vertex, std::vector<bool>& visited);
    };

    /**
     * @class SpaceNetworkAnalysis
     * @brief Network analysis tools for space route optimization
     */
    class SpaceNetworkAnalysis {
    public:
        explicit SpaceNetworkAnalysis(const SpaceGraph& graph) : graph_(graph) {}
        
        // Centrality measures
        std::vector<double> calculate_betweenness_centrality();
        std::vector<double> calculate_closeness_centrality();
        std::vector<double> calculate_degree_centrality();
        
        // Network properties
        double calculate_network_density();
        double calculate_average_clustering_coefficient();
        std::vector<size_t> find_articulation_points();
        std::vector<std::pair<size_t, size_t>> find_bridge_routes();
        
        // Space-specific analysis
        std::vector<size_t> identify_critical_stations();
        std::vector<size_t> find_optimal_fuel_depot_locations(size_t num_depots = 3);
        
        struct NetworkResilience {
            double connectivity_score;
            size_t redundant_paths_count;
            std::vector<size_t> single_point_failures;
            double average_path_length;
        };
        
        NetworkResilience analyze_network_resilience();
        
        // Route optimization suggestions
        struct RouteOptimization {
            std::vector<std::pair<size_t, size_t>> recommended_new_routes;
            std::vector<std::pair<size_t, size_t>> routes_to_upgrade;
            std::vector<size_t> stations_needing_expansion;
        };
        
        RouteOptimization suggest_network_improvements();

    private:
        const SpaceGraph& graph_;
        
        void dfs_bridges(size_t vertex, std::vector<bool>& visited, std::vector<int>& discovery,
                        std::vector<int>& low, std::vector<int>& parent, 
                        std::vector<std::pair<size_t, size_t>>& bridges, int& time);
        
        void dfs_articulation_points(size_t vertex, std::vector<bool>& visited, 
                                    std::vector<int>& discovery, std::vector<int>& low,
                                    std::vector<int>& parent, std::vector<bool>& articulation_points, 
                                    int& time);
    };

    /**
     * @class MinimumSpanningTree
     * @brief MST algorithms for optimal space network design
     */
    class MinimumSpanningTree {
    public:
        struct MSTResult {
            std::vector<std::pair<size_t, size_t>> edges;
            double total_cost;
            std::string algorithm_used;
            std::chrono::microseconds computation_time;
        };

        static MSTResult kruskal_mst(const SpaceGraph& graph, 
                                   std::function<double(const SpaceRoute&)> cost_function);
        
        static MSTResult prim_mst(const SpaceGraph& graph, 
                                std::function<double(const SpaceRoute&)> cost_function,
                                size_t start_vertex = 0);
        
        // Space-specific MST variants
        static MSTResult minimum_fuel_spanning_tree(const SpaceGraph& graph);
        static MSTResult minimum_time_spanning_tree(const SpaceGraph& graph);
        static MSTResult maximum_safety_spanning_tree(const SpaceGraph& graph);
        
        static void print_mst_result(const MSTResult& result, const SpaceGraph& graph);

    private:
        class UnionFind {
        public:
            explicit UnionFind(size_t n);
            size_t find(size_t x);
            bool unite(size_t x, size_t y);
            
        private:
            std::vector<size_t> parent_, rank_;
        };
    };

    /**
     * @class SpaceRouteSimulation
     * @brief Simulation and visualization of space routes
     */
    class SpaceRouteSimulation {
    public:
        struct SimulationParameters {
            double fuel_efficiency = 1.0;
            double speed_multiplier = 1.0;
            double danger_sensitivity = 1.0;
            bool enable_fuel_stops = true;
            bool enable_emergency_protocols = true;
            size_t max_concurrent_ships = 10;
        };

        struct ShipStatus {
            size_t ship_id;
            size_t current_station;
            size_t destination;
            std::vector<size_t> planned_route;
            double fuel_remaining;
            double cargo_capacity;
            std::string ship_type;
            bool emergency_status;
        };

        explicit SpaceRouteSimulation(const SpaceGraph& graph) : graph_(graph) {}
        
        void set_simulation_parameters(const SimulationParameters& params) { params_ = params; }
        
        // Ship management
        size_t add_ship(const std::string& name, size_t starting_station, 
                       double fuel_capacity = 1000.0, const std::string& type = "cargo");
        
        void set_ship_destination(size_t ship_id, size_t destination);
        
        // Simulation control
        void run_simulation_step();
        void run_simulation(size_t num_steps);
        
        // Status and reporting
        std::vector<ShipStatus> get_all_ship_status() const;
        void print_simulation_status() const;
        
        // Analytics
        struct SimulationStats {
            double total_fuel_consumed;
            double average_travel_time;
            size_t successful_deliveries;
            size_t emergency_situations;
            std::vector<size_t> busiest_routes;
        };
        
        SimulationStats get_simulation_statistics() const;

    /**
     * @class GraphAlgorithmsDemo
     * @brief Main demonstration coordinator for graph algorithms
     */
    class GraphAlgorithmsDemo {
    public:
        static void demonstrate_space_pathfinding();
        static void demonstrate_network_analysis();
        static void demonstrate_flow_networks();
        static void demonstrate_minimum_spanning_trees();
        static void demonstrate_route_simulation();
        static void demonstrate_space_network_optimization();
        static void run_comprehensive_graph_demo();

    private:
        static void print_section_header(const std::string& title);
        static void print_section_footer();
        static void print_path_result(const PathResult& result, const SpaceGraph& graph);
        
        static SpaceGraph create_sample_space_network();
        static SpaceGraph create_solar_system_network();
        static void add_sample_stations_and_routes(SpaceGraph& graph);
    };

    /**
     * @class GraphVisualization
     * @brief Visualization tools for graph algorithms
     */
    class GraphVisualization {
    public:
        static void print_path_visualization(const std::vector<size_t>& path, const SpaceGraph& graph);
        static void print_network_topology(const SpaceGraph& graph);
        static void print_adjacency_matrix(const SpaceGraph& graph);
        static void export_graph_to_dot(const SpaceGraph& graph, const std::string& filename);
        
        // ASCII art visualization for small graphs
        static void print_ascii_graph(const SpaceGraph& graph, const std::vector<size_t>& highlight_path = {});

    private:
        static std::string generate_station_symbol(SpaceStation::StationType type);
        static std::string get_route_visualization(const SpaceRoute& route);
    };

} // namespace CppVerseHub::Algorithms

#endif // GRAPHALGORITHMS_HPP