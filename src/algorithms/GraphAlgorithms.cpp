/**
 * @file GraphAlgorithms.cpp
 * @brief Graph-algorithms showcase on a small interstellar route network.
 */

#include "algorithms/GraphAlgorithms.hpp"

#include <array>
#include <cmath>
#include <iomanip>
#include <string>
#include <string_view>

namespace CppVerseHub::Algorithms {

namespace {

struct Station {
    std::string_view name;
    std::array<double, 3> position; // light years
};

constexpr std::array<Station, 9> kStations{{
    {"Sol", {0.0, 0.0, 0.0}},
    {"Alpha Centauri", {-1.6, -1.2, -3.8}},
    {"Barnard", {-0.1, -5.9, 0.5}},
    {"Wolf 359", {-7.4, 2.1, 1.0}},
    {"Sirius", {-1.6, 8.1, -2.5}},
    {"Procyon", {-4.8, 10.3, 1.0}},
    {"Tau Ceti", {10.3, 5.0, -3.4}},
    {"Epsilon Eridani", {6.2, 8.3, -1.7}},
    {"Luyten's Star", {-4.6, 11.4, 1.1}}, // spur: reachable only via Procyon
}};

double distance(std::size_t a, std::size_t b) {
    double s = 0.0;
    for (std::size_t d = 0; d < 3; ++d) {
        const double diff = kStations[a].position[d] - kStations[b].position[d];
        s += diff * diff;
    }
    return std::sqrt(s);
}

void print_path(std::ostream& out, const std::vector<std::size_t>& path) {
    for (std::size_t i = 0; i < path.size(); ++i) {
        out << (i ? " -> " : "") << kStations[path[i]].name;
    }
}

} // namespace

void demonstrate_graphs(std::ostream& out) {
    out << "=== Graph algorithms: interstellar route network ===\n";
    out << std::fixed << std::setprecision(2);

    // Undirected network; jump cost = Euclidean distance (so Euclidean A* heuristic is consistent).
    WeightedGraph<double> net(kStations.size());
    const std::array<std::pair<std::size_t, std::size_t>, 13> lanes{{
        {0, 1},
        {0, 2},
        {1, 2},
        {2, 3},
        {0, 3},
        {3, 5},
        {0, 4},
        {4, 5},
        {4, 7},
        {7, 6},
        {1, 6},
        {5, 7},
        {5, 8},
    }};
    for (const auto& [a, b] : lanes) {
        net.add_edge(a, b, distance(a, b));
    }

    const auto hops = bfs(net, 0);
    out << "BFS from Sol, hop counts:";
    for (std::size_t v = 0; v < kStations.size(); ++v) {
        out << ' ' << kStations[v].name << '=' << hops.hops[v];
    }
    out << '\n';

    const auto order = dfs(net, 0);
    out << "DFS pre-order: ";
    print_path(out, order.preorder);
    out << '\n';

    const auto sp = dijkstra(net, 0);
    out << "Dijkstra Sol -> Tau Ceti: ";
    print_path(out, sp.path_to(6));
    out << " (" << sp.distance[6].value_or(-1.0) << " ly)\n";

    const auto heuristic = [](std::size_t v) { return distance(v, 6); };
    if (const auto path = a_star(net, 0, 6, heuristic)) {
        out << "A* Sol -> Tau Ceti:       ";
        print_path(out, path->vertices);
        out << " (" << path->cost << " ly, " << path->expanded << " expansions)\n";
    }

    const auto mst = kruskal_mst(net);
    const auto prim = prim_mst(net);
    out << "Minimum relay backbone (Kruskal): " << mst.edges.size() << " lanes, " << mst.total_weight
        << " ly; Prim agrees: " << std::boolalpha << (std::abs(mst.total_weight - prim.total_weight) < 1e-9)
        << '\n';

    const auto cuts = find_cut_structure(net);
    out << "Critical lanes (bridges):";
    if (cuts.bridges.empty()) {
        out << " none";
    }
    for (std::size_t id : cuts.bridges) {
        const auto& e = net.edges()[id];
        out << ' ' << kStations[e.from].name << '-' << kStations[e.to].name;
    }
    out << "; articulation stations:";
    if (cuts.articulation_points.empty()) {
        out << " none";
    }
    for (std::size_t v : cuts.articulation_points) {
        out << ' ' << kStations[v].name;
    }
    out << '\n';

    // Directed mission-dependency DAG.
    WeightedGraph<int> missions(6, true);
    missions.add_edge(0, 1); // survey -> mine
    missions.add_edge(0, 2); // survey -> outpost
    missions.add_edge(1, 3); // mine -> refinery
    missions.add_edge(2, 3); // outpost -> refinery
    missions.add_edge(3, 4); // refinery -> shipyard
    missions.add_edge(2, 5); // outpost -> colony
    const std::array<std::string_view, 6> mission_names{"survey",   "mine",     "outpost",
                                                        "refinery", "shipyard", "colony"};
    out << "Topological mission order:";
    if (const auto topo = topological_sort(missions)) {
        for (std::size_t v : *topo) {
            out << ' ' << mission_names[v];
        }
    }
    out << '\n';

    WeightedGraph<int> trade(6, true);
    for (const auto& [a, b] : std::array<std::pair<std::size_t, std::size_t>, 7>{
             {{0, 1}, {1, 2}, {2, 0}, {2, 3}, {3, 4}, {4, 3}, {4, 5}}}) {
        trade.add_edge(a, b);
    }
    out << "Strongly connected trade blocs:";
    for (const auto& comp : strongly_connected_components(trade)) {
        out << " {";
        for (std::size_t i = 0; i < comp.size(); ++i) {
            out << (i ? "," : "") << comp[i];
        }
        out << '}';
    }
    out << '\n';

    // Bellman-Ford with a gravitational slingshot (negative cost) lane.
    WeightedGraph<int> slingshot(4, true);
    slingshot.add_edge(0, 1, 4);
    slingshot.add_edge(0, 2, 5);
    slingshot.add_edge(2, 1, -3);
    slingshot.add_edge(1, 3, 2);
    const auto bf = bellman_ford(slingshot, 0);
    out << "Bellman-Ford with slingshot: dist(0,3) = " << bf.paths.distance[3].value_or(-1)
        << ", negative cycle: " << bf.negative_cycle << '\n';

    // Evacuation capacity (ships/hour) from Sol (0) to a safe haven (5).
    FlowNetwork<long long> evac(6);
    evac.add_edge(0, 1, 16);
    evac.add_edge(0, 2, 13);
    evac.add_edge(1, 2, 10);
    evac.add_edge(2, 1, 4);
    evac.add_edge(1, 3, 12);
    evac.add_edge(3, 2, 9);
    evac.add_edge(2, 4, 14);
    evac.add_edge(4, 3, 7);
    evac.add_edge(3, 5, 20);
    evac.add_edge(4, 5, 4);
    const long long flow = evac.max_flow(0, 5);
    out << "Max evacuation flow (Edmonds-Karp): " << flow << " ships/hour = min cut capacity "
        << evac.cut_capacity(evac.min_cut_source_side()) << '\n';

    // Exact patrol tour over the first 6 stations.
    std::vector<std::vector<double>> dist(6, std::vector<double>(6));
    for (std::size_t a = 0; a < 6; ++a) {
        for (std::size_t b = 0; b < 6; ++b) {
            dist[a][b] = distance(a, b);
        }
    }
    const auto tour = held_karp_tsp(dist);
    out << "Held-Karp patrol tour: ";
    print_path(out, tour.tour);
    out << " -> Sol (" << tour.cost << " ly)\n";
    out.unsetf(std::ios::floatfield);
    out << std::setprecision(6);
}

} // namespace CppVerseHub::Algorithms
