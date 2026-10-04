// Graph algorithm tests: every algorithm is checked against a brute-force oracle or a known answer.

#include "algorithms/GraphAlgorithms.hpp"

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

#include <algorithm>
#include <bit>
#include <limits>
#include <optional>
#include <tuple>
#include <numeric>
#include <random>
#include <set>
#include <sstream>
#include <vector>

using namespace CppVerseHub::Algorithms;

namespace {

WeightedGraph<long long> random_graph(std::size_t n, std::size_t m, bool directed, long long min_w, long long max_w,
                                      std::uint64_t seed) {
    std::mt19937_64 rng(seed);
    std::uniform_int_distribution<std::size_t> vertex(0, n - 1);
    std::uniform_int_distribution<long long> weight(min_w, max_w);
    WeightedGraph<long long> g(n, directed);
    for (std::size_t i = 0; i < m; ++i) {
        g.add_edge(vertex(rng), vertex(rng), weight(rng));
    }
    return g;
}

// Random DAG: edges only from lower to higher index of a random permutation.
WeightedGraph<long long> random_dag(std::size_t n, std::size_t m, long long min_w, long long max_w,
                                    std::uint64_t seed) {
    std::mt19937_64 rng(seed);
    std::vector<std::size_t> perm(n);
    std::iota(perm.begin(), perm.end(), std::size_t{0});
    std::shuffle(perm.begin(), perm.end(), rng);
    std::uniform_int_distribution<std::size_t> vertex(0, n - 1);
    std::uniform_int_distribution<long long> weight(min_w, max_w);
    WeightedGraph<long long> g(n, true);
    for (std::size_t i = 0; i < m; ++i) {
        std::size_t a = vertex(rng);
        std::size_t b = vertex(rng);
        if (a == b) {
            continue;
        }
        if (a > b) {
            std::swap(a, b);
        }
        g.add_edge(perm[a], perm[b], weight(rng));
    }
    return g;
}

template <class W>
W path_cost(const WeightedGraph<W>& g, const std::vector<std::size_t>& path) {
    W total{};
    for (std::size_t i = 1; i < path.size(); ++i) {
        std::optional<W> best;
        for (const auto& arc : g.neighbors(path[i - 1])) {
            if (arc.to == path[i] && (!best || arc.weight < *best)) {
                best = arc.weight;
            }
        }
        REQUIRE(best.has_value());
        total = total + *best;
    }
    return total;
}

// Number of connected components of an undirected graph, skipping one edge and/or vertex.
std::size_t components_without(const WeightedGraph<long long>& g, std::size_t skip_edge, std::size_t skip_vertex) {
    DisjointSet dsu(g.vertex_count());
    for (std::size_t id = 0; id < g.edge_count(); ++id) {
        const auto& e = g.edges()[id];
        if (id == skip_edge || e.from == skip_vertex || e.to == skip_vertex) {
            continue;
        }
        dsu.unite(e.from, e.to);
    }
    return dsu.set_count() - (skip_vertex == kNoVertex ? 0 : 1);
}

}  // namespace

// ---------------------------------------------------------------------------------------------
// Graph representation
// ---------------------------------------------------------------------------------------------

TEST_CASE("WeightedGraph stores arcs, edges and transposes correctly", "[graph][representation]") {
    WeightedGraph<int> und(3);
    und.add_edge(0, 1, 5);
    und.add_edge(1, 2, 7);
    und.add_edge(2, 2, 1);  // self loop stored once
    REQUIRE(und.edge_count() == 3);
    REQUIRE(und.neighbors(1).size() == 2);
    REQUIRE(und.neighbors(2).size() == 2);
    REQUIRE(und.neighbors(0)[0].to == 1);
    REQUIRE(und.neighbors(0)[0].edge_id == und.neighbors(1)[0].edge_id);
    REQUIRE_THROWS_AS(und.add_edge(0, 3, 1), std::out_of_range);
    REQUIRE(und.add_vertex() == 3);

    WeightedGraph<int> dir(3, true);
    dir.add_edge(0, 1, 2);
    dir.add_edge(0, 2, 3);
    REQUIRE(dir.neighbors(1).empty());
    const auto rev = dir.reversed();
    REQUIRE(rev.neighbors(0).empty());
    REQUIRE(rev.neighbors(1).size() == 1);
    REQUIRE(rev.neighbors(2)[0].weight == 3);
    STATIC_REQUIRE(EdgeWeight<int>);
    STATIC_REQUIRE(EdgeWeight<double>);
    STATIC_REQUIRE_FALSE(EdgeWeight<std::vector<int>>);
}

TEST_CASE("reconstruct_path follows parents and rejects unreachable targets", "[graph][path]") {
    const std::vector<std::size_t> parent{kNoVertex, 0, 1, kNoVertex};
    REQUIRE(reconstruct_path(parent, 0, 2) == std::vector<std::size_t>{0, 1, 2});
    REQUIRE(reconstruct_path(parent, 0, 0) == std::vector<std::size_t>{0});
    REQUIRE(reconstruct_path(parent, 0, 3).empty());
    REQUIRE(reconstruct_path(parent, 0, 9).empty());
}

// ---------------------------------------------------------------------------------------------
// Traversal
// ---------------------------------------------------------------------------------------------

TEST_CASE("BFS hop counts equal unit-weight all-pairs shortest paths", "[graph][bfs][property]") {
    for (std::uint64_t seed = 0; seed < 20; ++seed) {
        const bool directed = seed % 2 == 0;
        const auto g = random_graph(15, 25, directed, 1, 1, seed);
        const auto fw = floyd_warshall(g);
        for (std::size_t s = 0; s < g.vertex_count(); ++s) {
            const auto r = bfs(g, s);
            REQUIRE(r.order.front() == s);
            for (std::size_t v = 0; v < g.vertex_count(); ++v) {
                if (fw[s][v]) {
                    REQUIRE(r.hops[v] == static_cast<std::size_t>(*fw[s][v]));
                    REQUIRE(reconstruct_path(r.parent, s, v).size() == r.hops[v] + 1);
                } else {
                    REQUIRE(r.hops[v] == kNoVertex);
                }
            }
            // BFS order is non-decreasing in hop count.
            for (std::size_t i = 1; i < r.order.size(); ++i) {
                REQUIRE(r.hops[r.order[i - 1]] <= r.hops[r.order[i]]);
            }
        }
    }
    const WeightedGraph<int> g(1);
    REQUIRE_THROWS_AS(bfs(g, 1), std::out_of_range);
}

TEST_CASE("DFS visits exactly the reachable set in recursive order", "[graph][dfs]") {
    WeightedGraph<int> g(7);
    g.add_edge(0, 1);
    g.add_edge(0, 2);
    g.add_edge(1, 3);
    g.add_edge(1, 4);
    g.add_edge(2, 5);
    // vertex 6 isolated
    const auto r = dfs(g, 0);
    REQUIRE(r.preorder == std::vector<std::size_t>{0, 1, 3, 4, 2, 5});
    REQUIRE(r.postorder == std::vector<std::size_t>{3, 4, 1, 5, 2, 0});
    REQUIRE(r.parent[5] == 2);
    REQUIRE(r.parent[6] == kNoVertex);

    for (std::uint64_t seed = 0; seed < 15; ++seed) {
        const auto rg = random_graph(20, 30, seed % 2 == 0, 1, 1, seed);
        const auto d = dfs(rg, 0);
        const auto b = bfs(rg, 0);
        std::set<std::size_t> dset(d.preorder.begin(), d.preorder.end());
        std::set<std::size_t> bset(b.order.begin(), b.order.end());
        REQUIRE(dset == bset);
        REQUIRE(d.postorder.size() == d.preorder.size());
        REQUIRE(d.postorder.back() == 0);
    }
}

TEST_CASE("reverse DFS post-order is a topological order of a DAG", "[graph][dfs][property]") {
    for (std::uint64_t seed = 0; seed < 20; ++seed) {
        const auto g = random_dag(25, 60, 1, 1, seed);
        // Add a super-source so a single DFS reaches everything.
        WeightedGraph<long long> h(26, true);
        for (const auto& e : g.edges()) {
            h.add_edge(e.from, e.to, e.weight);
        }
        for (std::size_t v = 0; v < 25; ++v) {
            h.add_edge(25, v, 1);
        }
        auto post = dfs(h, 25).postorder;
        std::reverse(post.begin(), post.end());
        std::vector<std::size_t> pos(26);
        for (std::size_t i = 0; i < post.size(); ++i) {
            pos[post[i]] = i;
        }
        for (const auto& e : h.edges()) {
            REQUIRE(pos[e.from] < pos[e.to]);
        }
    }
}

TEST_CASE("connected components agree with BFS reachability", "[graph][components][property]") {
    for (std::uint64_t seed = 0; seed < 20; ++seed) {
        const auto g = random_graph(30, 20, false, 1, 1, seed);
        const auto comp = connected_components(g);
        for (std::size_t s = 0; s < g.vertex_count(); ++s) {
            const auto r = bfs(g, s);
            for (std::size_t v = 0; v < g.vertex_count(); ++v) {
                REQUIRE((comp[s] == comp[v]) == (r.hops[v] != kNoVertex));
            }
        }
        REQUIRE(comp[0] == 0);
    }
}

// ---------------------------------------------------------------------------------------------
// Shortest paths
// ---------------------------------------------------------------------------------------------

TEST_CASE("Dijkstra, Bellman-Ford and Floyd-Warshall agree on non-negative graphs", "[graph][sssp][property]") {
    for (std::uint64_t seed = 0; seed < 30; ++seed) {
        const bool directed = seed % 3 != 0;
        const auto g = random_graph(18, 50, directed, 0, 20, seed);
        const auto fw = floyd_warshall(g);
        for (std::size_t s = 0; s < g.vertex_count(); s += 3) {
            const auto dj = dijkstra(g, s);
            const auto bf = bellman_ford(g, s);
            REQUIRE_FALSE(bf.negative_cycle);
            for (std::size_t v = 0; v < g.vertex_count(); ++v) {
                REQUIRE(dj.distance[v] == fw[s][v]);
                REQUIRE(bf.paths.distance[v] == fw[s][v]);
                if (dj.distance[v]) {
                    const auto path = dj.path_to(v);
                    REQUIRE(path.front() == s);
                    REQUIRE(path.back() == v);
                    REQUIRE(path_cost(g, path) == *dj.distance[v]);
                } else {
                    REQUIRE(dj.path_to(v).empty());
                }
            }
        }
    }
}

TEST_CASE("Dijkstra works with floating-point weights and rejects negative ones", "[graph][dijkstra]") {
    WeightedGraph<double> g(4);
    g.add_edge(0, 1, 1.5);
    g.add_edge(1, 2, 2.25);
    g.add_edge(0, 2, 4.0);
    g.add_edge(2, 3, 0.5);
    const auto sp = dijkstra(g, 0);
    REQUIRE(sp.distance[3] == 4.25);
    REQUIRE(sp.path_to(3) == std::vector<std::size_t>{0, 1, 2, 3});
    g.add_edge(3, 0, -1.0);
    REQUIRE_THROWS_AS(dijkstra(g, 0), std::invalid_argument);
}

TEST_CASE("Bellman-Ford handles negative weights and detects negative cycles", "[graph][bellman-ford]") {
    SECTION("negative-weight DAGs match Floyd-Warshall") {
        for (std::uint64_t seed = 0; seed < 25; ++seed) {
            const auto g = random_dag(16, 40, -10, 15, seed);
            const auto fw = floyd_warshall(g);
            for (std::size_t s = 0; s < g.vertex_count(); ++s) {
                const auto bf = bellman_ford(g, s);
                REQUIRE_FALSE(bf.negative_cycle);
                for (std::size_t v = 0; v < g.vertex_count(); ++v) {
                    REQUIRE(bf.paths.distance[v] == fw[s][v]);
                }
            }
        }
    }
    SECTION("reachable negative cycle is reported") {
        WeightedGraph<int> g(4, true);
        g.add_edge(0, 1, 1);
        g.add_edge(1, 2, -2);
        g.add_edge(2, 1, 1);  // cycle 1->2->1 of weight -1
        g.add_edge(2, 3, 1);
        REQUIRE(bellman_ford(g, 0).negative_cycle);
        const auto fw = floyd_warshall(g);
        REQUIRE(*fw[1][1] < 0);
    }
    SECTION("unreachable negative cycle is ignored") {
        WeightedGraph<int> g(4, true);
        g.add_edge(0, 1, 3);
        g.add_edge(2, 3, -5);
        g.add_edge(3, 2, 1);
        const auto bf = bellman_ford(g, 0);
        REQUIRE_FALSE(bf.negative_cycle);
        REQUIRE(bf.paths.distance[1] == 3);
        REQUIRE_FALSE(bf.paths.distance[2].has_value());
    }
    SECTION("an undirected negative edge is a negative cycle") {
        WeightedGraph<int> g(2);
        g.add_edge(0, 1, -1);
        REQUIRE(bellman_ford(g, 0).negative_cycle);
    }
}

TEST_CASE("A* finds optimal paths on grids with obstacles", "[graph][astar][property]") {
    constexpr std::size_t W = 24;
    constexpr std::size_t H = 18;
    auto id = [](std::size_t x, std::size_t y) { return y * W + x; };
    for (std::uint64_t seed = 0; seed < 12; ++seed) {
        std::mt19937_64 rng(seed);
        std::vector<bool> wall(W * H, false);
        for (std::size_t i = 0; i < W * H; ++i) {
            wall[i] = rng() % 100 < 25;
        }
        wall[id(0, 0)] = false;
        wall[id(W - 1, H - 1)] = false;
        WeightedGraph<long long> g(W * H);
        std::uniform_int_distribution<long long> cost(1, 5);
        for (std::size_t y = 0; y < H; ++y) {
            for (std::size_t x = 0; x < W; ++x) {
                if (wall[id(x, y)]) {
                    continue;
                }
                if (x + 1 < W && !wall[id(x + 1, y)]) {
                    g.add_edge(id(x, y), id(x + 1, y), cost(rng));
                }
                if (y + 1 < H && !wall[id(x, y + 1)]) {
                    g.add_edge(id(x, y), id(x, y + 1), cost(rng));
                }
            }
        }
        const std::size_t target = id(W - 1, H - 1);
        auto manhattan = [&](std::size_t v) {
            const auto x = static_cast<long long>(v % W);
            const auto y = static_cast<long long>(v / W);
            return static_cast<long long>(W - 1) - x + static_cast<long long>(H - 1) - y;  // min edge cost is 1
        };
        const auto dj = dijkstra(g, 0);
        const auto informed = a_star(g, 0, target, manhattan);
        const auto blind = a_star(g, 0, target, [](std::size_t) { return 0LL; });
        REQUIRE(informed.has_value() == dj.distance[target].has_value());
        REQUIRE(blind.has_value() == dj.distance[target].has_value());
        if (informed) {
            REQUIRE(informed->cost == *dj.distance[target]);
            REQUIRE(blind->cost == *dj.distance[target]);
            REQUIRE(path_cost(g, informed->vertices) == informed->cost);
            REQUIRE(informed->expanded <= blind->expanded);
        }

        // Admissible but inconsistent heuristic: random fraction of the true remaining distance.
        const auto to_target = dijkstra(g, target);  // undirected: dist(v, target)
        std::vector<long long> h(W * H, 0);
        for (std::size_t v = 0; v < W * H; ++v) {
            if (to_target.distance[v]) {
                h[v] = *to_target.distance[v] * static_cast<long long>(rng() % 101) / 100;
            }
        }
        const auto erratic = a_star(g, 0, target, [&h](std::size_t v) { return h[v]; });
        if (dj.distance[target]) {
            REQUIRE(erratic->cost == *dj.distance[target]);
        } else {
            REQUIRE_FALSE(erratic.has_value());
        }
    }
}

TEST_CASE("A* edge cases", "[graph][astar]") {
    WeightedGraph<int> g(3, true);
    g.add_edge(0, 1, 2);
    const auto self = a_star(g, 0, 0, [](std::size_t) { return 0; });
    REQUIRE(self->vertices == std::vector<std::size_t>{0});
    REQUIRE(self->cost == 0);
    REQUIRE_FALSE(a_star(g, 0, 2, [](std::size_t) { return 0; }).has_value());
    REQUIRE_THROWS_AS(a_star(g, 0, 5, [](std::size_t) { return 0; }), std::out_of_range);
}

// ---------------------------------------------------------------------------------------------
// Directed structure
// ---------------------------------------------------------------------------------------------

TEST_CASE("topological sort orders DAGs and rejects cycles", "[graph][topo][property]") {
    for (std::uint64_t seed = 0; seed < 30; ++seed) {
        auto g = random_dag(20, 50, 1, 1, seed);
        const auto order = topological_sort(g);
        REQUIRE(order.has_value());
        REQUIRE(order->size() == 20);
        std::vector<std::size_t> pos(20);
        for (std::size_t i = 0; i < 20; ++i) {
            pos[(*order)[i]] = i;
        }
        for (const auto& e : g.edges()) {
            REQUIRE(pos[e.from] < pos[e.to]);
        }
        REQUIRE_FALSE(has_cycle(g));
        if (!g.edges().empty()) {
            const auto e = g.edges().front();
            g.add_edge(e.to, e.from, 1);  // close a cycle
            REQUIRE_FALSE(topological_sort(g).has_value());
            REQUIRE(has_cycle(g));
        }
    }
    WeightedGraph<int> fixed(4, true);
    fixed.add_edge(3, 1);
    fixed.add_edge(2, 1);
    fixed.add_edge(1, 0);
    REQUIRE(topological_sort(fixed) == std::vector<std::size_t>{2, 3, 1, 0});  // lexicographically smallest
    REQUIRE_THROWS_AS(topological_sort(WeightedGraph<int>(2)), std::invalid_argument);
}

TEST_CASE("Tarjan SCCs equal mutual reachability classes", "[graph][scc][property]") {
    for (std::uint64_t seed = 0; seed < 25; ++seed) {
        const auto g = random_graph(16, 24, true, 1, 1, seed);
        const auto fw = floyd_warshall(g);
        const auto sccs = strongly_connected_components(g);
        std::vector<std::size_t> label(16, kNoVertex);
        std::size_t total = 0;
        for (std::size_t c = 0; c < sccs.size(); ++c) {
            REQUIRE(std::is_sorted(sccs[c].begin(), sccs[c].end()));
            if (c > 0) {
                REQUIRE(sccs[c - 1].front() < sccs[c].front());
            }
            for (std::size_t v : sccs[c]) {
                label[v] = c;
                ++total;
            }
        }
        REQUIRE(total == 16);
        for (std::size_t u = 0; u < 16; ++u) {
            for (std::size_t v = 0; v < 16; ++v) {
                const bool mutual = fw[u][v].has_value() && fw[v][u].has_value();
                REQUIRE((label[u] == label[v]) == mutual);
            }
        }
    }
    WeightedGraph<int> g(5, true);
    g.add_edge(0, 1);
    g.add_edge(1, 2);
    g.add_edge(2, 0);
    g.add_edge(3, 4);
    REQUIRE(strongly_connected_components(g) ==
            std::vector<std::vector<std::size_t>>{{0, 1, 2}, {3}, {4}});
}

TEST_CASE("has_cycle on undirected graphs", "[graph][cycle]") {
    WeightedGraph<int> tree(4);
    tree.add_edge(0, 1);
    tree.add_edge(1, 2);
    tree.add_edge(1, 3);
    REQUIRE_FALSE(has_cycle(tree));
    tree.add_edge(3, 0);
    REQUIRE(has_cycle(tree));
    WeightedGraph<int> loop(1);
    loop.add_edge(0, 0);
    REQUIRE(has_cycle(loop));
    WeightedGraph<int> parallel(2);
    parallel.add_edge(0, 1);
    parallel.add_edge(1, 0);
    REQUIRE(has_cycle(parallel));
}

// ---------------------------------------------------------------------------------------------
// Minimum spanning trees
// ---------------------------------------------------------------------------------------------

TEST_CASE("Kruskal and Prim match a brute-force minimum spanning tree", "[graph][mst][property]") {
    for (std::uint64_t seed = 0; seed < 40; ++seed) {
        const std::size_t n = 2 + seed % 5;  // 2..6 vertices
        const auto g = random_graph(n, n + 4, false, 1, 9, seed);
        const auto kr = kruskal_mst(g);
        const auto pr = prim_mst(g);
        REQUIRE(kr.total_weight == pr.total_weight);
        REQUIRE(kr.components == pr.components);
        REQUIRE(kr.edges.size() == n - kr.components);
        REQUIRE(pr.edges.size() == n - pr.components);

        // Brute force: among all edge subsets forming a spanning forest with the same number of
        // components, find the minimum weight.
        const std::size_t m = g.edge_count();
        std::optional<long long> best;
        for (std::size_t mask = 0; mask < (std::size_t{1} << m); ++mask) {
            if (static_cast<std::size_t>(std::popcount(mask)) != n - kr.components) {
                continue;
            }
            DisjointSet dsu(n);
            long long w = 0;
            bool acyclic = true;
            for (std::size_t i = 0; i < m && acyclic; ++i) {
                if ((mask >> i) & 1U) {
                    acyclic = dsu.unite(g.edges()[i].from, g.edges()[i].to);
                    w += g.edges()[i].weight;
                }
            }
            if (acyclic && dsu.set_count() == kr.components && (!best || w < *best)) {
                best = w;
            }
        }
        REQUIRE(best.has_value());
        REQUIRE(kr.total_weight == *best);
    }
}

TEST_CASE("MST on disconnected graphs and invalid input", "[graph][mst]") {
    WeightedGraph<double> g(5);
    g.add_edge(0, 1, 1.0);
    g.add_edge(1, 2, 2.0);
    g.add_edge(0, 2, 0.5);
    g.add_edge(3, 4, 4.0);
    const auto kr = kruskal_mst(g);
    REQUIRE(kr.components == 2);
    REQUIRE(kr.total_weight == 5.5);
    REQUIRE(prim_mst(g).total_weight == 5.5);
    REQUIRE_THROWS_AS(kruskal_mst(WeightedGraph<int>(2, true)), std::invalid_argument);
    REQUIRE_THROWS_AS(prim_mst(WeightedGraph<int>(2, true)), std::invalid_argument);
    REQUIRE(kruskal_mst(WeightedGraph<int>(0)).components == 0);
}

// ---------------------------------------------------------------------------------------------
// Bridges / articulation points
// ---------------------------------------------------------------------------------------------

TEST_CASE("bridges and articulation points match brute-force removal", "[graph][cuts][property]") {
    for (std::uint64_t seed = 0; seed < 40; ++seed) {
        const auto g = random_graph(12, 14, false, 1, 1, seed);  // sparse: many bridges, parallel edges
        const auto cs = find_cut_structure(g);
        const std::size_t base = components_without(g, kNoVertex, kNoVertex);
        std::vector<std::size_t> bridges;
        for (std::size_t id = 0; id < g.edge_count(); ++id) {
            if (components_without(g, id, kNoVertex) > base) {
                bridges.push_back(id);
            }
        }
        std::vector<std::size_t> points;
        for (std::size_t v = 0; v < g.vertex_count(); ++v) {
            if (components_without(g, kNoVertex, v) > base) {
                points.push_back(v);
            }
        }
        REQUIRE(cs.bridges == bridges);
        REQUIRE(cs.articulation_points == points);
    }
    REQUIRE_THROWS_AS(find_cut_structure(WeightedGraph<int>(2, true)), std::invalid_argument);
}

// ---------------------------------------------------------------------------------------------
// Flow
// ---------------------------------------------------------------------------------------------

TEST_CASE("Edmonds-Karp solves the CLRS example and conserves flow", "[graph][flow]") {
    FlowNetwork<int> f(6);
    std::vector<std::size_t> ids;
    ids.push_back(f.add_edge(0, 1, 16));
    ids.push_back(f.add_edge(0, 2, 13));
    ids.push_back(f.add_edge(1, 2, 10));
    ids.push_back(f.add_edge(2, 1, 4));
    ids.push_back(f.add_edge(1, 3, 12));
    ids.push_back(f.add_edge(3, 2, 9));
    ids.push_back(f.add_edge(2, 4, 14));
    ids.push_back(f.add_edge(4, 3, 7));
    ids.push_back(f.add_edge(3, 5, 20));
    ids.push_back(f.add_edge(4, 5, 4));
    REQUIRE(f.max_flow(0, 5) == 23);
    REQUIRE(f.max_flow(0, 5) == 23);  // idempotent: flow is reset first
    const auto side = f.min_cut_source_side();
    REQUIRE(side[0]);
    REQUIRE_FALSE(side[5]);
    REQUIRE(f.cut_capacity(side) == 23);
    for (std::size_t id : ids) {
        REQUIRE(f.flow(id) >= 0);
    }
    REQUIRE_THROWS_AS(f.add_edge(0, 1, -1), std::invalid_argument);
    REQUIRE_THROWS_AS(f.max_flow(2, 2), std::invalid_argument);
}

TEST_CASE("max flow equals brute-force minimum cut", "[graph][flow][property]") {
    for (std::uint64_t seed = 0; seed < 40; ++seed) {
        std::mt19937_64 rng(seed);
        const std::size_t n = 6;
        FlowNetwork<long long> f(n);
        std::vector<std::tuple<std::size_t, std::size_t, long long>> edges;
        for (int i = 0; i < 12; ++i) {
            const std::size_t u = rng() % n;
            const std::size_t v = rng() % n;
            const auto c = static_cast<long long>(rng() % 10);
            f.add_edge(u, v, c);
            edges.emplace_back(u, v, c);
        }
        const long long flow = f.max_flow(0, n - 1);
        long long best = std::numeric_limits<long long>::max();
        for (std::size_t mask = 0; mask < (std::size_t{1} << (n - 2)); ++mask) {
            std::vector<bool> side(n, false);
            side[0] = true;
            for (std::size_t v = 1; v + 1 < n; ++v) {
                side[v] = ((mask >> (v - 1)) & 1U) != 0;
            }
            long long cap = 0;
            for (const auto& [u, v, c] : edges) {
                if (side[u] && !side[v]) {
                    cap += c;
                }
            }
            best = std::min(best, cap);
        }
        REQUIRE(flow == best);
        REQUIRE(f.cut_capacity(f.min_cut_source_side()) == flow);
    }
}

// ---------------------------------------------------------------------------------------------
// TSP
// ---------------------------------------------------------------------------------------------

TEST_CASE("Held-Karp matches brute-force permutation search", "[graph][tsp][property]") {
    for (std::uint64_t seed = 0; seed < 20; ++seed) {
        std::mt19937_64 rng(seed);
        const std::size_t n = 2 + seed % 6;  // 2..7
        std::vector<std::vector<int>> d(n, std::vector<int>(n, 0));
        for (std::size_t i = 0; i < n; ++i) {
            for (std::size_t j = 0; j < n; ++j) {
                d[i][j] = i == j ? 0 : 1 + static_cast<int>(rng() % 50);  // asymmetric
            }
        }
        const auto r = held_karp_tsp(d);
        std::vector<std::size_t> perm(n - 1);
        std::iota(perm.begin(), perm.end(), std::size_t{1});
        int best = std::numeric_limits<int>::max();
        do {
            int c = d[0][perm.front()] + d[perm.back()][0];
            for (std::size_t i = 1; i < perm.size(); ++i) {
                c += d[perm[i - 1]][perm[i]];
            }
            best = std::min(best, c);
        } while (std::next_permutation(perm.begin(), perm.end()));
        REQUIRE(r.cost == best);
        REQUIRE(r.tour.size() == n);
        REQUIRE(r.tour.front() == 0);
        int tour_cost = d[r.tour.back()][0];
        for (std::size_t i = 1; i < n; ++i) {
            tour_cost += d[r.tour[i - 1]][r.tour[i]];
        }
        REQUIRE(tour_cost == r.cost);
        auto sorted = r.tour;
        std::sort(sorted.begin(), sorted.end());
        REQUIRE(std::adjacent_find(sorted.begin(), sorted.end()) == sorted.end());
    }
    REQUIRE(held_karp_tsp(std::vector<std::vector<int>>{}).tour.empty());
    REQUIRE(held_karp_tsp(std::vector<std::vector<int>>{{0}}).tour == std::vector<std::size_t>{0});
    REQUIRE_THROWS_AS(held_karp_tsp(std::vector<std::vector<int>>{{0, 1}}), std::invalid_argument);
    REQUIRE_THROWS_AS(held_karp_tsp(std::vector<std::vector<int>>(21, std::vector<int>(21))),
                      std::invalid_argument);
}

TEST_CASE("graph demo runs and reports consistent results", "[graph][demo]") {
    std::ostringstream oss;
    demonstrate_graphs(oss);
    const std::string s = oss.str();
    REQUIRE(s.find("Prim agrees: true") != std::string::npos);
    REQUIRE(s.find("Max evacuation flow (Edmonds-Karp): 23 ships/hour = min cut capacity 23") != std::string::npos);
    REQUIRE(s.find("dist(0,3) = 4") != std::string::npos);
    REQUIRE(s.find("survey") < s.find("shipyard"));
}
