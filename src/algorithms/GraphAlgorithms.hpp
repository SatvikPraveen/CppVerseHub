/**
 * @file GraphAlgorithms.hpp
 * @brief Generic graph algorithms over a weighted adjacency-list graph.
 *
 * The graph type `WeightedGraph<W>` is parameterised on any weight type satisfying the
 * `EdgeWeight` concept (regular, totally ordered, closed under +), so the same algorithms run on
 * integer hop counts, floating-point distances or user-defined fixed-point types.
 *
 * Covered: traversal (BFS, DFS, connected components), single-source shortest paths (Dijkstra,
 * Bellman-Ford with negative-cycle detection), all-pairs shortest paths (Floyd-Warshall),
 * heuristic search (A*), DAG ordering (Kahn topological sort), strongly connected components
 * (Tarjan), minimum spanning trees (Kruskal with union-find, Prim), bridges and articulation points,
 * maximum flow / minimum cut (Edmonds-Karp) and the exact travelling-salesman DP (Held-Karp).
 *
 * All traversals are iterative (no recursion depth proportional to V). Vertex ids are dense
 * indices in [0, V). Complexities use V = vertices, E = edges.
 */

#ifndef CPPVERSEHUB_ALGORITHMS_GRAPHALGORITHMS_HPP
#define CPPVERSEHUB_ALGORITHMS_GRAPHALGORITHMS_HPP

#include "algorithms/DataStructures.hpp"

#include <algorithm>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <functional>
#include <iostream>
#include <limits>
#include <optional>
#include <queue>
#include <span>
#include <stdexcept>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

namespace CppVerseHub::Algorithms {

/**
 * @brief Requirements on edge weights: value semantics, a total order, and closure under +.
 *        `W{}` is taken to be the additive identity (zero).
 */
template <class W>
concept EdgeWeight = std::regular<W> && std::totally_ordered<W> && requires(const W a, const W b) {
    { a + b } -> std::convertible_to<W>;
};

/// @brief Sentinel for "no vertex" (unreached parent, absent predecessor).
inline constexpr std::size_t kNoVertex = std::numeric_limits<std::size_t>::max();

/**
 * @class WeightedGraph
 * @brief Adjacency-list graph with dense vertex ids, directed or undirected.
 *
 * add_edge: amortised O(1). neighbors: O(1). Space: O(V + E). Undirected edges are stored once in
 * edges() and as two arcs (sharing the same edge id) in the adjacency lists.
 *
 * @tparam W Edge weight type.
 */
template <EdgeWeight W = double>
class WeightedGraph {
public:
    using weight_type = W;  ///< Weight type.

    /// @brief An edge as added by the user.
    struct Edge {
        std::size_t from;  ///< Source (or one endpoint if undirected).
        std::size_t to;    ///< Target (or the other endpoint).
        W weight;          ///< Weight.
        /// @brief Member-wise equality.
        friend bool operator==(const Edge&, const Edge&) = default;
    };

    /// @brief An adjacency entry.
    struct Arc {
        std::size_t to;       ///< Neighbour.
        W weight;             ///< Weight of the connecting edge.
        std::size_t edge_id;  ///< Index into edges().
    };

    /**
     * @brief Creates a graph with `vertices` isolated vertices.
     * @param vertices Number of vertices.
     * @param directed Whether edges are one-way.
     */
    explicit WeightedGraph(std::size_t vertices = 0, bool directed = false)
        : adjacency_(vertices), directed_(directed) {}

    /**
     * @brief Adds an isolated vertex.
     * @return Its id.
     */
    std::size_t add_vertex() {
        adjacency_.emplace_back();
        return adjacency_.size() - 1;
    }

    /**
     * @brief Adds an edge u -> v (and v -> u if undirected).
     * @param u      Source vertex.
     * @param v      Target vertex.
     * @param weight Edge weight.
     * @return The edge id.
     * @throws std::out_of_range if u or v is not a vertex.
     */
    std::size_t add_edge(std::size_t u, std::size_t v, W weight = W{1}) {
        if (u >= adjacency_.size() || v >= adjacency_.size()) {
            throw std::out_of_range("WeightedGraph::add_edge: vertex out of range");
        }
        const std::size_t id = edges_.size();
        edges_.push_back(Edge{u, v, weight});
        adjacency_[u].push_back(Arc{v, weight, id});
        if (!directed_ && u != v) {
            adjacency_[v].push_back(Arc{u, weight, id});
        }
        return id;
    }

    /**
     * @brief Outgoing arcs of `u`.
     * @param u Vertex.
     * @return View of u's adjacency list (invalidated by add_edge).
     */
    [[nodiscard]] std::span<const Arc> neighbors(std::size_t u) const { return adjacency_.at(u); }

    /// @brief All edges in insertion order.
    [[nodiscard]] const std::vector<Edge>& edges() const noexcept { return edges_; }
    /// @brief Number of vertices.
    [[nodiscard]] std::size_t vertex_count() const noexcept { return adjacency_.size(); }
    /// @brief Number of edges.
    [[nodiscard]] std::size_t edge_count() const noexcept { return edges_.size(); }
    /// @brief Whether the graph is directed.
    [[nodiscard]] bool directed() const noexcept { return directed_; }

    /**
     * @brief The transpose graph (every edge reversed). O(V + E).
     * @return Reversed copy (identical copy for undirected graphs).
     */
    [[nodiscard]] WeightedGraph reversed() const {
        WeightedGraph r(vertex_count(), directed_);
        for (const Edge& e : edges_) {
            r.add_edge(e.to, e.from, e.weight);
        }
        return r;
    }

private:
    std::vector<std::vector<Arc>> adjacency_;
    std::vector<Edge> edges_;
    bool directed_;
};

/**
 * @brief Follows parent pointers from `target` back to `source`.
 * @param parent Parent array (kNoVertex = none).
 * @param source Path start.
 * @param target Path end.
 * @return Vertices source..target, or empty if target is not reachable.
 */
[[nodiscard]] inline std::vector<std::size_t> reconstruct_path(const std::vector<std::size_t>& parent,
                                                               std::size_t source, std::size_t target) {
    std::vector<std::size_t> path;
    if (target >= parent.size()) {
        return path;
    }
    for (std::size_t v = target; v != kNoVertex; v = parent[v]) {
        path.push_back(v);
        if (v == source) {
            std::ranges::reverse(path);
            return path;
        }
        if (path.size() > parent.size()) {
            break;  // malformed parent array (cycle)
        }
    }
    return {};
}

// ============================================================================================
// Traversal
// ============================================================================================

/// @brief Result of a breadth-first search.
struct BfsResult {
    std::vector<std::size_t> order;   ///< Vertices in visitation order.
    std::vector<std::size_t> hops;    ///< Edge count from the source (kNoVertex if unreachable).
    std::vector<std::size_t> parent;  ///< BFS-tree parent (kNoVertex for source/unreached).
};

/**
 * @brief Breadth-first search from `source`; ignores weights.
 * @param g      Graph.
 * @param source Start vertex.
 * @return Visitation order, hop distances (shortest in edge count) and BFS tree.
 *
 * Time: O(V + E). Space: O(V).
 */
template <EdgeWeight W>
[[nodiscard]] BfsResult bfs(const WeightedGraph<W>& g, std::size_t source) {
    const std::size_t n = g.vertex_count();
    BfsResult r{{}, std::vector<std::size_t>(n, kNoVertex), std::vector<std::size_t>(n, kNoVertex)};
    if (source >= n) {
        throw std::out_of_range("bfs: source out of range");
    }
    std::deque<std::size_t> queue{source};
    r.hops[source] = 0;
    while (!queue.empty()) {
        const std::size_t u = queue.front();
        queue.pop_front();
        r.order.push_back(u);
        for (const auto& arc : g.neighbors(u)) {
            if (r.hops[arc.to] == kNoVertex) {
                r.hops[arc.to] = r.hops[u] + 1;
                r.parent[arc.to] = u;
                queue.push_back(arc.to);
            }
        }
    }
    return r;
}

/// @brief Result of a depth-first search.
struct DfsResult {
    std::vector<std::size_t> preorder;   ///< Discovery order.
    std::vector<std::size_t> postorder;  ///< Finish order.
    std::vector<std::size_t> parent;     ///< DFS-tree parent (kNoVertex for source/unreached).
};

/**
 * @brief Iterative depth-first search from `source`, visiting neighbours in adjacency order
 *        (same order as the textbook recursive formulation).
 * @param g      Graph.
 * @param source Start vertex.
 * @return Pre-order, post-order and DFS tree.
 *
 * Time: O(V + E). Space: O(V) explicit stack.
 */
template <EdgeWeight W>
[[nodiscard]] DfsResult dfs(const WeightedGraph<W>& g, std::size_t source) {
    const std::size_t n = g.vertex_count();
    if (source >= n) {
        throw std::out_of_range("dfs: source out of range");
    }
    DfsResult r{{}, {}, std::vector<std::size_t>(n, kNoVertex)};
    std::vector<bool> seen(n, false);
    std::vector<std::pair<std::size_t, std::size_t>> stack{{source, 0}};
    seen[source] = true;
    r.preorder.push_back(source);
    while (!stack.empty()) {
        const std::size_t u = stack.back().first;
        const auto arcs = g.neighbors(u);
        if (stack.back().second < arcs.size()) {
            const std::size_t v = arcs[stack.back().second++].to;
            if (!seen[v]) {
                seen[v] = true;
                r.parent[v] = u;
                r.preorder.push_back(v);
                stack.emplace_back(v, 0);
            }
        } else {
            r.postorder.push_back(u);
            stack.pop_back();
        }
    }
    return r;
}

/**
 * @brief (Weakly) connected components via union-find over the edge list.
 * @param g Graph (edge direction is ignored).
 * @return comp[v] = component id, ids numbered 0.. in order of smallest vertex.
 *
 * Time: O(V + E alpha(V)). Space: O(V).
 */
template <EdgeWeight W>
[[nodiscard]] std::vector<std::size_t> connected_components(const WeightedGraph<W>& g) {
    const std::size_t n = g.vertex_count();
    DisjointSet dsu(n);
    for (const auto& e : g.edges()) {
        dsu.unite(e.from, e.to);
    }
    std::vector<std::size_t> label(n, kNoVertex);
    std::vector<std::size_t> comp(n);
    std::size_t next = 0;
    for (std::size_t v = 0; v < n; ++v) {
        const std::size_t root = dsu.find(v);
        if (label[root] == kNoVertex) {
            label[root] = next++;
        }
        comp[v] = label[root];
    }
    return comp;
}

// ============================================================================================
// Shortest paths
// ============================================================================================

/// @brief Single-source shortest-path tree.
template <EdgeWeight W>
struct ShortestPaths {
    std::size_t source = kNoVertex;          ///< Source vertex.
    std::vector<std::optional<W>> distance;  ///< Distance per vertex (nullopt = unreachable).
    std::vector<std::size_t> parent;         ///< Predecessor on a shortest path.

    /**
     * @brief Vertices of a shortest path to `target`.
     * @param target Destination.
     * @return source..target, or empty if unreachable.
     */
    [[nodiscard]] std::vector<std::size_t> path_to(std::size_t target) const {
        if (target >= distance.size() || !distance[target]) {
            return {};
        }
        return reconstruct_path(parent, source, target);
    }
};

/**
 * @brief Dijkstra's algorithm with a binary heap and lazy deletion.
 * @param g      Graph with non-negative weights.
 * @param source Start vertex.
 * @return Shortest distances and tree.
 * @throws std::invalid_argument if any edge weight is negative.
 *
 * Time: O((V + E) log V). Space: O(V + E) (heap may hold one entry per relaxation).
 */
template <EdgeWeight W>
[[nodiscard]] ShortestPaths<W> dijkstra(const WeightedGraph<W>& g, std::size_t source) {
    const std::size_t n = g.vertex_count();
    if (source >= n) {
        throw std::out_of_range("dijkstra: source out of range");
    }
    for (const auto& e : g.edges()) {
        if (e.weight < W{}) {
            throw std::invalid_argument("dijkstra: negative edge weight");
        }
    }
    ShortestPaths<W> sp{source, std::vector<std::optional<W>>(n), std::vector<std::size_t>(n, kNoVertex)};
    using Entry = std::pair<W, std::size_t>;
    std::priority_queue<Entry, std::vector<Entry>, std::greater<>> heap;
    sp.distance[source] = W{};
    heap.emplace(W{}, source);
    std::vector<bool> done(n, false);
    while (!heap.empty()) {
        const auto [d, u] = heap.top();
        heap.pop();
        if (done[u]) {
            continue;  // stale entry
        }
        done[u] = true;
        for (const auto& arc : g.neighbors(u)) {
            const W nd = d + arc.weight;
            auto& dv = sp.distance[arc.to];
            if (!dv || nd < *dv) {
                dv = nd;
                sp.parent[arc.to] = u;
                heap.emplace(nd, arc.to);
            }
        }
    }
    return sp;
}

/// @brief Result of Bellman-Ford.
template <EdgeWeight W>
struct BellmanFordResult {
    ShortestPaths<W> paths;      ///< Valid only when !negative_cycle.
    bool negative_cycle = false; ///< A negative cycle is reachable from the source.
};

/**
 * @brief Bellman-Ford single-source shortest paths; supports negative weights.
 * @param g      Graph (an undirected negative edge is itself a negative cycle).
 * @param source Start vertex.
 * @return Distances, tree and whether a reachable negative cycle exists.
 *
 * Runs V - 1 relaxation rounds (stopping early when nothing changes) plus one detection round.
 * Time: O(V * E). Space: O(V).
 */
template <EdgeWeight W>
[[nodiscard]] BellmanFordResult<W> bellman_ford(const WeightedGraph<W>& g, std::size_t source) {
    const std::size_t n = g.vertex_count();
    if (source >= n) {
        throw std::out_of_range("bellman_ford: source out of range");
    }
    BellmanFordResult<W> r;
    r.paths = ShortestPaths<W>{source, std::vector<std::optional<W>>(n), std::vector<std::size_t>(n, kNoVertex)};
    auto& dist = r.paths.distance;
    dist[source] = W{};
    auto relax_all = [&]() {
        bool changed = false;
        for (std::size_t u = 0; u < n; ++u) {
            if (!dist[u]) {
                continue;
            }
            for (const auto& arc : g.neighbors(u)) {
                const W nd = *dist[u] + arc.weight;
                if (!dist[arc.to] || nd < *dist[arc.to]) {
                    dist[arc.to] = nd;
                    r.paths.parent[arc.to] = u;
                    changed = true;
                }
            }
        }
        return changed;
    };
    for (std::size_t round = 0; round + 1 < n; ++round) {
        if (!relax_all()) {
            return r;
        }
    }
    r.negative_cycle = relax_all();
    return r;
}

/**
 * @brief Floyd-Warshall all-pairs shortest paths.
 * @param g Graph.
 * @return dist[u][v] (nullopt = unreachable). With a negative cycle some dist[v][v] < 0.
 *
 * Time: Theta(V^3). Space: O(V^2). Also serves as the brute-force oracle in the tests.
 */
template <EdgeWeight W>
[[nodiscard]] std::vector<std::vector<std::optional<W>>> floyd_warshall(const WeightedGraph<W>& g) {
    const std::size_t n = g.vertex_count();
    std::vector<std::vector<std::optional<W>>> d(n, std::vector<std::optional<W>>(n));
    for (std::size_t v = 0; v < n; ++v) {
        d[v][v] = W{};
    }
    for (std::size_t u = 0; u < n; ++u) {
        for (const auto& arc : g.neighbors(u)) {
            if (!d[u][arc.to] || arc.weight < *d[u][arc.to]) {
                d[u][arc.to] = arc.weight;
            }
        }
    }
    for (std::size_t k = 0; k < n; ++k) {
        for (std::size_t i = 0; i < n; ++i) {
            if (!d[i][k]) {
                continue;
            }
            for (std::size_t j = 0; j < n; ++j) {
                if (d[k][j]) {
                    const W via = *d[i][k] + *d[k][j];
                    if (!d[i][j] || via < *d[i][j]) {
                        d[i][j] = via;
                    }
                }
            }
        }
    }
    return d;
}

/// @brief A path with its total cost.
template <EdgeWeight W>
struct Path {
    std::vector<std::size_t> vertices;  ///< source..target.
    W cost{};                           ///< Sum of edge weights.
    std::size_t expanded = 0;           ///< Number of vertex expansions performed by the search.
};

/**
 * @brief A* search from `source` to `target`.
 * @param g         Graph with non-negative weights.
 * @param source    Start vertex.
 * @param target    Goal vertex.
 * @param heuristic h(v): an *admissible* (never overestimating) estimate of dist(v, target).
 *                  With a consistent heuristic every vertex is expanded at most once; an
 *                  admissible but inconsistent one may re-expand vertices but stays optimal.
 * @return The optimal path, or nullopt if `target` is unreachable.
 *
 * Time: O((V + E) log V) with a consistent heuristic (h = 0 degenerates to Dijkstra).
 * Space: O(V + E).
 */
template <EdgeWeight W, std::invocable<std::size_t> H>
    requires std::convertible_to<std::invoke_result_t<H&, std::size_t>, W>
[[nodiscard]] std::optional<Path<W>> a_star(const WeightedGraph<W>& g, std::size_t source, std::size_t target,
                                            H heuristic) {
    const std::size_t n = g.vertex_count();
    if (source >= n || target >= n) {
        throw std::out_of_range("a_star: vertex out of range");
    }
    std::vector<std::optional<W>> best(n);
    std::vector<std::size_t> parent(n, kNoVertex);
    using Entry = std::tuple<W, W, std::size_t>;  // (f = g + h, g, vertex)
    std::priority_queue<Entry, std::vector<Entry>, std::greater<>> open;
    best[source] = W{};
    open.emplace(static_cast<W>(std::invoke(heuristic, source)), W{}, source);
    std::size_t expanded = 0;
    while (!open.empty()) {
        const auto [f, gu, u] = open.top();
        open.pop();
        if (best[u] && *best[u] < gu) {
            continue;  // stale
        }
        ++expanded;
        if (u == target) {
            return Path<W>{reconstruct_path(parent, source, target), gu, expanded};
        }
        for (const auto& arc : g.neighbors(u)) {
            if (arc.weight < W{}) {
                throw std::invalid_argument("a_star: negative edge weight");
            }
            const W ng = gu + arc.weight;
            if (!best[arc.to] || ng < *best[arc.to]) {
                best[arc.to] = ng;
                parent[arc.to] = u;
                open.emplace(ng + static_cast<W>(std::invoke(heuristic, arc.to)), ng, arc.to);
            }
        }
    }
    return std::nullopt;
}

// ============================================================================================
// Directed structure
// ============================================================================================

/**
 * @brief Kahn's algorithm: topological order of a directed graph.
 * @param g Directed graph.
 * @return An order where every edge u->v has u before v (smallest available vertex first, so the
 *         result is the lexicographically smallest order), or nullopt if g has a cycle.
 * @throws std::invalid_argument for undirected graphs.
 *
 * Time: O(V log V + E) (a min-heap picks the next source for determinism). Space: O(V).
 */
template <EdgeWeight W>
[[nodiscard]] std::optional<std::vector<std::size_t>> topological_sort(const WeightedGraph<W>& g) {
    if (!g.directed()) {
        throw std::invalid_argument("topological_sort: graph must be directed");
    }
    const std::size_t n = g.vertex_count();
    std::vector<std::size_t> indegree(n, 0);
    for (const auto& e : g.edges()) {
        ++indegree[e.to];
    }
    std::priority_queue<std::size_t, std::vector<std::size_t>, std::greater<>> ready;
    for (std::size_t v = 0; v < n; ++v) {
        if (indegree[v] == 0) {
            ready.push(v);
        }
    }
    std::vector<std::size_t> order;
    order.reserve(n);
    while (!ready.empty()) {
        const std::size_t u = ready.top();
        ready.pop();
        order.push_back(u);
        for (const auto& arc : g.neighbors(u)) {
            if (--indegree[arc.to] == 0) {
                ready.push(arc.to);
            }
        }
    }
    if (order.size() != n) {
        return std::nullopt;
    }
    return order;
}

/**
 * @brief Tarjan's strongly connected components (iterative).
 * @param g Graph (for undirected graphs this yields the connected components).
 * @return Components, each sorted ascending, ordered by their smallest vertex.
 *
 * Time: O(V + E). Space: O(V).
 */
template <EdgeWeight W>
[[nodiscard]] std::vector<std::vector<std::size_t>> strongly_connected_components(const WeightedGraph<W>& g) {
    const std::size_t n = g.vertex_count();
    std::vector<std::size_t> index(n, kNoVertex);
    std::vector<std::size_t> low(n, 0);
    std::vector<bool> on_stack(n, false);
    std::vector<std::size_t> scc_stack;
    std::vector<std::vector<std::size_t>> components;
    std::vector<std::pair<std::size_t, std::size_t>> call;  // (vertex, next arc)
    std::size_t counter = 0;
    auto open_vertex = [&](std::size_t v) {
        index[v] = low[v] = counter++;
        scc_stack.push_back(v);
        on_stack[v] = true;
        call.emplace_back(v, 0);
    };
    for (std::size_t root = 0; root < n; ++root) {
        if (index[root] != kNoVertex) {
            continue;
        }
        open_vertex(root);
        while (!call.empty()) {
            const std::size_t v = call.back().first;
            const auto arcs = g.neighbors(v);
            if (call.back().second < arcs.size()) {
                const std::size_t w = arcs[call.back().second++].to;
                if (index[w] == kNoVertex) {
                    open_vertex(w);
                } else if (on_stack[w]) {
                    low[v] = std::min(low[v], index[w]);
                }
                continue;
            }
            if (low[v] == index[v]) {
                std::vector<std::size_t> comp;
                std::size_t w = kNoVertex;
                do {
                    w = scc_stack.back();
                    scc_stack.pop_back();
                    on_stack[w] = false;
                    comp.push_back(w);
                } while (w != v);
                std::ranges::sort(comp);
                components.push_back(std::move(comp));
            }
            call.pop_back();
            if (!call.empty()) {
                const std::size_t u = call.back().first;
                low[u] = std::min(low[u], low[v]);
            }
        }
    }
    std::ranges::sort(components, {}, [](const auto& c) { return c.front(); });
    return components;
}

/**
 * @brief Whether the graph contains a cycle.
 * @param g Graph. Directed: any directed cycle (including self-loops). Undirected: any cycle,
 *          including self-loops and parallel edges.
 * @return true if a cycle exists.
 *
 * Time: O(V log V + E) directed (via Kahn), O(E alpha(V)) undirected (via union-find).
 */
template <EdgeWeight W>
[[nodiscard]] bool has_cycle(const WeightedGraph<W>& g) {
    if (g.directed()) {
        return !topological_sort(g).has_value();
    }
    DisjointSet dsu(g.vertex_count());
    for (const auto& e : g.edges()) {
        if (!dsu.unite(e.from, e.to)) {
            return true;
        }
    }
    return false;
}

// ============================================================================================
// Minimum spanning trees
// ============================================================================================

/// @brief A minimum spanning forest.
template <EdgeWeight W>
struct MstResult {
    std::vector<typename WeightedGraph<W>::Edge> edges;  ///< Chosen edges.
    W total_weight{};                                    ///< Sum of chosen weights.
    std::size_t components = 0;                          ///< Trees in the forest (1 if connected).
};

/**
 * @brief Kruskal's algorithm: scan edges by weight, keep those joining different trees.
 * @param g Undirected graph.
 * @return Minimum spanning forest (ties broken by edge id, so the result is deterministic).
 * @throws std::invalid_argument for directed graphs.
 *
 * Time: O(E log E) for sorting + O(E alpha(V)) union-find. Space: O(V + E).
 */
template <EdgeWeight W>
[[nodiscard]] MstResult<W> kruskal_mst(const WeightedGraph<W>& g) {
    if (g.directed()) {
        throw std::invalid_argument("kruskal_mst: graph must be undirected");
    }
    const auto& edges = g.edges();
    std::vector<std::size_t> ids(edges.size());
    for (std::size_t i = 0; i < ids.size(); ++i) {
        ids[i] = i;
    }
    std::ranges::stable_sort(ids, {}, [&edges](std::size_t i) { return edges[i].weight; });
    DisjointSet dsu(g.vertex_count());
    MstResult<W> r;
    for (std::size_t i : ids) {
        if (dsu.unite(edges[i].from, edges[i].to)) {
            r.edges.push_back(edges[i]);
            r.total_weight = r.total_weight + edges[i].weight;
        }
    }
    r.components = dsu.set_count();
    return r;
}

/**
 * @brief Prim's algorithm (lazy, binary heap), restarted from every unvisited vertex so it
 *        returns a spanning forest on disconnected graphs.
 * @param g Undirected graph.
 * @return Minimum spanning forest (same total weight as Kruskal).
 * @throws std::invalid_argument for directed graphs.
 *
 * Time: O(E log E). Space: O(V + E).
 */
template <EdgeWeight W>
[[nodiscard]] MstResult<W> prim_mst(const WeightedGraph<W>& g) {
    if (g.directed()) {
        throw std::invalid_argument("prim_mst: graph must be undirected");
    }
    const std::size_t n = g.vertex_count();
    std::vector<bool> in_tree(n, false);
    MstResult<W> r;
    using Entry = std::tuple<W, std::size_t, std::size_t>;  // (weight, edge id, vertex)
    std::priority_queue<Entry, std::vector<Entry>, std::greater<>> heap;
    for (std::size_t root = 0; root < n; ++root) {
        if (in_tree[root]) {
            continue;
        }
        ++r.components;
        in_tree[root] = true;
        for (const auto& arc : g.neighbors(root)) {
            heap.emplace(arc.weight, arc.edge_id, arc.to);
        }
        while (!heap.empty()) {
            const auto [w, id, v] = heap.top();
            heap.pop();
            if (in_tree[v]) {
                continue;
            }
            in_tree[v] = true;
            r.edges.push_back(g.edges()[id]);
            r.total_weight = r.total_weight + w;
            for (const auto& arc : g.neighbors(v)) {
                if (!in_tree[arc.to]) {
                    heap.emplace(arc.weight, arc.edge_id, arc.to);
                }
            }
        }
    }
    return r;
}

// ============================================================================================
// Bridges and articulation points
// ============================================================================================

/// @brief Cut structure of an undirected graph.
struct CutStructure {
    std::vector<std::size_t> bridges;              ///< Edge ids whose removal disconnects the graph.
    std::vector<std::size_t> articulation_points;  ///< Vertices whose removal disconnects the graph.
};

/**
 * @brief Tarjan/Hopcroft low-link computation of bridges and articulation points (iterative).
 * @param g Undirected graph (parallel edges and self-loops are handled correctly).
 * @return Sorted bridge edge ids and sorted articulation vertices.
 * @throws std::invalid_argument for directed graphs.
 *
 * Time: O(V + E). Space: O(V).
 */
template <EdgeWeight W>
[[nodiscard]] CutStructure find_cut_structure(const WeightedGraph<W>& g) {
    if (g.directed()) {
        throw std::invalid_argument("find_cut_structure: graph must be undirected");
    }
    const std::size_t n = g.vertex_count();
    std::vector<std::size_t> tin(n, kNoVertex);
    std::vector<std::size_t> low(n, 0);
    std::vector<bool> is_cut(n, false);
    CutStructure cs;
    struct Frame {
        std::size_t v;
        std::size_t parent_edge;
        std::size_t next;
    };
    std::vector<Frame> stack;
    std::size_t timer = 0;
    for (std::size_t root = 0; root < n; ++root) {
        if (tin[root] != kNoVertex) {
            continue;
        }
        std::size_t root_children = 0;
        tin[root] = low[root] = timer++;
        stack.push_back(Frame{root, kNoVertex, 0});
        while (!stack.empty()) {
            const std::size_t v = stack.back().v;
            const auto arcs = g.neighbors(v);
            if (stack.back().next < arcs.size()) {
                const auto& arc = arcs[stack.back().next++];
                if (arc.edge_id == stack.back().parent_edge) {
                    continue;  // don't reuse the tree edge we arrived by (parallel edges still count)
                }
                if (tin[arc.to] != kNoVertex) {
                    low[v] = std::min(low[v], tin[arc.to]);
                } else {
                    tin[arc.to] = low[arc.to] = timer++;
                    stack.push_back(Frame{arc.to, arc.edge_id, 0});
                }
                continue;
            }
            const std::size_t via = stack.back().parent_edge;
            stack.pop_back();
            if (stack.empty()) {
                break;
            }
            const std::size_t p = stack.back().v;
            low[p] = std::min(low[p], low[v]);
            if (low[v] > tin[p]) {
                cs.bridges.push_back(via);
            }
            if (p == root) {
                ++root_children;
            } else if (low[v] >= tin[p]) {
                is_cut[p] = true;
            }
        }
        if (root_children > 1) {
            is_cut[root] = true;
        }
    }
    for (std::size_t v = 0; v < n; ++v) {
        if (is_cut[v]) {
            cs.articulation_points.push_back(v);
        }
    }
    std::ranges::sort(cs.bridges);
    return cs;
}

// ============================================================================================
// Maximum flow
// ============================================================================================

/**
 * @class FlowNetwork
 * @brief Directed capacitated network with Edmonds-Karp maximum flow and minimum cut.
 *
 * Each edge is stored with its residual twin (index ^ 1). max_flow: O(V * E^2) (BFS augmenting
 * paths). Space: O(V + E). By the max-flow/min-cut theorem the returned flow value equals the
 * capacity of min_cut_source_side(), which the tests verify by brute force.
 *
 * @tparam Cap Arithmetic capacity type.
 */
template <class Cap = long long>
    requires std::is_arithmetic_v<Cap>
class FlowNetwork {
public:
    /**
     * @brief Creates a network with `vertices` vertices and no edges.
     * @param vertices Number of vertices.
     */
    explicit FlowNetwork(std::size_t vertices) : adjacency_(vertices) {}

    /**
     * @brief Adds a directed edge with the given capacity.
     * @param u        Tail.
     * @param v        Head.
     * @param capacity Non-negative capacity.
     * @return Edge handle for flow().
     * @throws std::invalid_argument for negative capacity; std::out_of_range for bad vertices.
     */
    std::size_t add_edge(std::size_t u, std::size_t v, Cap capacity) {
        if (u >= adjacency_.size() || v >= adjacency_.size()) {
            throw std::out_of_range("FlowNetwork::add_edge: vertex out of range");
        }
        if (capacity < Cap{}) {
            throw std::invalid_argument("FlowNetwork::add_edge: negative capacity");
        }
        const std::size_t id = edges_.size();
        edges_.push_back(Residual{v, capacity, Cap{}});
        adjacency_[u].push_back(id);
        edges_.push_back(Residual{u, Cap{}, Cap{}});
        adjacency_[v].push_back(id + 1);
        return id;
    }

    /**
     * @brief Computes a maximum s-t flow (resets any previous flow first).
     * @param s Source.
     * @param t Sink (!= s).
     * @return The maximum flow value.
     */
    Cap max_flow(std::size_t s, std::size_t t) {
        if (s >= adjacency_.size() || t >= adjacency_.size() || s == t) {
            throw std::invalid_argument("FlowNetwork::max_flow: invalid source/sink");
        }
        for (auto& e : edges_) {
            e.flow = Cap{};
        }
        source_ = s;
        Cap total{};
        std::vector<std::size_t> via(adjacency_.size());
        while (true) {
            std::fill(via.begin(), via.end(), kNoVertex);
            std::deque<std::size_t> queue{s};
            via[s] = edges_.size();  // marks visited
            while (!queue.empty() && via[t] == kNoVertex) {
                const std::size_t u = queue.front();
                queue.pop_front();
                for (std::size_t id : adjacency_[u]) {
                    const auto& e = edges_[id];
                    if (via[e.to] == kNoVertex && e.capacity - e.flow > Cap{}) {
                        via[e.to] = id;
                        queue.push_back(e.to);
                    }
                }
            }
            if (via[t] == kNoVertex) {
                break;
            }
            Cap bottleneck = std::numeric_limits<Cap>::max();
            for (std::size_t v = t; v != s; v = edges_[via[v] ^ 1].to) {
                const auto& e = edges_[via[v]];
                bottleneck = std::min(bottleneck, e.capacity - e.flow);
            }
            for (std::size_t v = t; v != s; v = edges_[via[v] ^ 1].to) {
                edges_[via[v]].flow += bottleneck;
                edges_[via[v] ^ 1].flow -= bottleneck;
            }
            total += bottleneck;
        }
        return total;
    }

    /**
     * @brief Flow currently on an edge.
     * @param edge Handle returned by add_edge().
     * @return Flow value.
     */
    [[nodiscard]] Cap flow(std::size_t edge) const { return edges_.at(edge).flow; }

    /**
     * @brief Source side of a minimum cut (vertices reachable in the residual graph).
     *        Call after max_flow().
     * @return side[v] == true iff v is on the source side.
     */
    [[nodiscard]] std::vector<bool> min_cut_source_side() const {
        std::vector<bool> side(adjacency_.size(), false);
        if (source_ == kNoVertex) {
            return side;
        }
        std::deque<std::size_t> queue{source_};
        side[source_] = true;
        while (!queue.empty()) {
            const std::size_t u = queue.front();
            queue.pop_front();
            for (std::size_t id : adjacency_[u]) {
                const auto& e = edges_[id];
                if (!side[e.to] && e.capacity - e.flow > Cap{}) {
                    side[e.to] = true;
                    queue.push_back(e.to);
                }
            }
        }
        return side;
    }

    /**
     * @brief Total capacity of user edges crossing from `side` to its complement.
     * @param side Partition (true = source side).
     * @return Cut capacity.
     */
    [[nodiscard]] Cap cut_capacity(const std::vector<bool>& side) const {
        Cap c{};
        for (std::size_t u = 0; u < adjacency_.size(); ++u) {
            for (std::size_t id : adjacency_[u]) {
                if (id % 2 == 0 && side[u] && !side[edges_[id].to]) {
                    c += edges_[id].capacity;
                }
            }
        }
        return c;
    }

    /// @brief Number of vertices.
    [[nodiscard]] std::size_t vertex_count() const noexcept { return adjacency_.size(); }

private:
    struct Residual {
        std::size_t to;
        Cap capacity;
        Cap flow;
    };
    std::vector<std::vector<std::size_t>> adjacency_;
    std::vector<Residual> edges_;
    std::size_t source_ = kNoVertex;
};

// ============================================================================================
// Travelling salesman (exact)
// ============================================================================================

/// @brief An optimal closed tour.
template <class W>
struct TspResult {
    std::vector<std::size_t> tour;  ///< Visiting order starting at vertex 0 (return to 0 implied).
    W cost{};                       ///< Total length including the return edge.
};

/**
 * @brief Held-Karp dynamic programme for the exact travelling-salesman tour.
 * @param dist Square distance matrix (need not be symmetric).
 * @return Optimal tour from vertex 0.
 * @throws std::invalid_argument if the matrix is not square or has more than 20 vertices.
 *
 * dp[S][j] = cheapest path from 0 through set S ending at j. Time: Theta(2^n * n^2).
 * Space: Theta(2^n * n) - exponential, but vastly better than the O(n!) brute force.
 */
template <class W>
    requires std::is_arithmetic_v<W>
[[nodiscard]] TspResult<W> held_karp_tsp(const std::vector<std::vector<W>>& dist) {
    const std::size_t n = dist.size();
    for (const auto& row : dist) {
        if (row.size() != n) {
            throw std::invalid_argument("held_karp_tsp: distance matrix must be square");
        }
    }
    if (n > 20) {
        throw std::invalid_argument("held_karp_tsp: too many vertices (max 20)");
    }
    if (n <= 1) {
        return TspResult<W>{n == 1 ? std::vector<std::size_t>{0} : std::vector<std::size_t>{}, W{}};
    }
    const std::size_t full = std::size_t{1} << n;
    constexpr W kInf = std::numeric_limits<W>::max();
    std::vector<W> dp(full * n, kInf);
    std::vector<std::size_t> from(full * n, kNoVertex);
    auto at = [n](std::size_t mask, std::size_t j) { return mask * n + j; };
    dp[at(1, 0)] = W{};
    for (std::size_t mask = 1; mask < full; mask += 2) {  // masks containing vertex 0
        for (std::size_t j = 0; j < n; ++j) {
            const W cur = dp[at(mask, j)];
            if (cur == kInf || (mask & (std::size_t{1} << j)) == 0) {
                continue;
            }
            for (std::size_t k = 1; k < n; ++k) {
                if ((mask & (std::size_t{1} << k)) != 0) {
                    continue;
                }
                const std::size_t next = mask | (std::size_t{1} << k);
                const W cand = cur + dist[j][k];
                if (cand < dp[at(next, k)]) {
                    dp[at(next, k)] = cand;
                    from[at(next, k)] = j;
                }
            }
        }
    }
    TspResult<W> best{{}, kInf};
    std::size_t last = kNoVertex;
    for (std::size_t j = 1; j < n; ++j) {
        const W cur = dp[at(full - 1, j)];
        if (cur != kInf && cur + dist[j][0] < best.cost) {
            best.cost = cur + dist[j][0];
            last = j;
        }
    }
    std::size_t mask = full - 1;
    for (std::size_t v = last; v != kNoVertex;) {
        best.tour.push_back(v);
        const std::size_t prev = from[at(mask, v)];
        mask &= ~(std::size_t{1} << v);
        v = prev;
    }
    std::ranges::reverse(best.tour);
    return best;
}

/**
 * @brief Showcase: a small interstellar route network exercising every algorithm.
 * @param out Stream to write to.
 */
void demonstrate_graphs(std::ostream& out = std::cout);

}  // namespace CppVerseHub::Algorithms

#endif  // CPPVERSEHUB_ALGORITHMS_GRAPHALGORITHMS_HPP
