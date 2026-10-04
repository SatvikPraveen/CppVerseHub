/**
 * @file Containers.hpp
 * @brief STL container showcase: sequence containers, associative containers and adapters.
 *
 * Every standard container family is demonstrated through a small, reusable component
 * whose design depends on the container's complexity guarantees:
 *  - std::vector      -> FleetRoster (contiguous storage, erase-remove, sorted copies)
 *  - std::list + std::unordered_map -> LruCache (O(1) splice keeps iterators valid)
 *  - std::deque       -> slidingWindowMaximum (monotonic double-ended queue)
 *  - std::array       -> makeSquares (compile-time fixed-size aggregate)
 *  - std::map / std::multimap -> groupPlanetsBySystem / indexPlanetsByDefense (ordered keys)
 *  - std::unordered_map -> wordFrequencies (hash-based counting)
 *  - std::stack / std::queue / std::priority_queue -> isBalanced, breadthFirstOrder, TaskScheduler
 *
 * The components are quiet library code; the demonstrate* functions narrate them to a stream.
 */
#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <iostream>
#include <list>
#include <map>
#include <optional>
#include <queue>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

namespace CppVerseHub::STL {

/**
 * @brief A spacecraft record used by the container demonstrations.
 */
struct Spacecraft {
    std::string name;        ///< Unique ship name (identity key).
    std::string class_type;  ///< Ship class, e.g. "Frigate".
    double mass{0.0};        ///< Mass in tonnes.
    int crew_size{0};        ///< Number of crew members.
    double max_speed{0.0};   ///< Maximum sub-light speed.
    double firepower{0.0};   ///< Abstract firepower rating.

    /**
     * @brief Combined combat rating derived from firepower, crew and speed.
     * @return firepower * (1 + 0.1 * crew) * (speed / 100).
     */
    [[nodiscard]] constexpr double combatRating() const noexcept {
        return firepower * (1.0 + static_cast<double>(crew_size) * 0.1) * (max_speed / 100.0);
    }

    /// @brief Member-wise equality.
    friend bool operator==(const Spacecraft&, const Spacecraft&) = default;
};

/**
 * @brief Hash functor keyed on Spacecraft::name (for unordered containers keyed by identity).
 */
struct SpacecraftNameHash {
    /**
     * @brief Hash the ship name.
     * @param ship Ship to hash.
     * @return Hash of @p ship.name.
     */
    [[nodiscard]] std::size_t operator()(const Spacecraft& ship) const noexcept {
        return std::hash<std::string>{}(ship.name);
    }
};

/**
 * @brief Equality functor matching SpacecraftNameHash (ships are equal when names match).
 */
struct SpacecraftNameEqual {
    /**
     * @brief Compare two ships by name.
     * @param lhs First ship.
     * @param rhs Second ship.
     * @return true when the names are equal.
     */
    [[nodiscard]] bool operator()(const Spacecraft& lhs, const Spacecraft& rhs) const noexcept {
        return lhs.name == rhs.name;
    }
};

/**
 * @brief A planet record used by the associative-container demonstrations.
 */
struct Planet {
    std::string name;                    ///< Planet name.
    std::string system;                  ///< Star system the planet belongs to.
    double population{0.0};              ///< Population in billions.
    int defense_level{0};                ///< Defense rating 0..10.
    std::vector<std::string> resources;  ///< Resources available on the planet.

    /// @brief Member-wise equality.
    friend bool operator==(const Planet&, const Planet&) = default;
};

/**
 * @brief Deterministic sample fleet used by demos, tests and benchmarks.
 * @return Five ships with distinct names.
 */
[[nodiscard]] std::vector<Spacecraft> sampleFleet();

/**
 * @brief Deterministic sample planets spread over three star systems.
 * @return Six planets.
 */
[[nodiscard]] std::vector<Planet> samplePlanets();

/**
 * @brief Ordered collection of uniquely named ships backed by std::vector.
 *
 * Demonstrates reserve/emplace_back, std::erase_if, linear search with std::ranges::find
 * and projections, and returning sorted copies without disturbing insertion order.
 */
class FleetRoster {
public:
    FleetRoster() = default;

    /**
     * @brief Construct a roster from ships; later duplicates (by name) are ignored.
     * @param ships Initial ships.
     */
    explicit FleetRoster(std::vector<Spacecraft> ships);

    /**
     * @brief Add a ship if no ship with the same name exists.
     * @param ship Ship to add.
     * @return true if inserted, false if the name was already present.
     */
    bool add(Spacecraft ship);

    /**
     * @brief Remove the ship with the given name.
     * @param name Ship name.
     * @return true if a ship was removed.
     */
    bool remove(std::string_view name);

    /**
     * @brief Remove every ship whose crew is smaller than @p min_crew (std::erase_if).
     * @param min_crew Minimum crew to keep.
     * @return Number of ships removed.
     */
    std::size_t removeUndercrewed(int min_crew);

    /**
     * @brief Look up a ship by name.
     * @param name Ship name.
     * @return Non-owning pointer to the ship, or nullptr. Invalidated by add/remove.
     */
    [[nodiscard]] const Spacecraft* find(std::string_view name) const noexcept;

    /**
     * @brief Ships sorted by descending combat rating (ties broken by name).
     * @return Sorted copy; the roster keeps insertion order.
     */
    [[nodiscard]] std::vector<Spacecraft> sortedByCombatRating() const;

    /**
     * @brief Names of all ships of a class, in insertion order.
     * @param class_type Class to filter on.
     * @return Matching ship names.
     */
    [[nodiscard]] std::vector<std::string> namesOfClass(std::string_view class_type) const;

    /**
     * @brief Sum of crew sizes (std::transform_reduce).
     * @return Total crew.
     */
    [[nodiscard]] long long totalCrew() const noexcept;

    /// @brief Number of ships. @return Ship count.
    [[nodiscard]] std::size_t size() const noexcept { return ships_.size(); }
    /// @brief Whether the roster is empty. @return true when empty.
    [[nodiscard]] bool empty() const noexcept { return ships_.empty(); }
    /// @brief Read-only view of the ships in insertion order. @return Span over the ships.
    [[nodiscard]] std::span<const Spacecraft> ships() const noexcept { return ships_; }

private:
    std::vector<Spacecraft> ships_;
};

/**
 * @brief Fixed-capacity least-recently-used cache built from std::list + std::unordered_map.
 *
 * The list stores entries in recency order (front = most recent); the hash map stores list
 * iterators. std::list::splice relinks nodes without invalidating iterators, which is what makes
 * get/put O(1) on average. Because the map holds iterators into *this* list, the copy operations
 * rebuild the index (a defaulted copy would alias the source list); moves are safe because
 * std::list move preserves iterator validity.
 *
 * @tparam Key   Key type (hashable, copyable).
 * @tparam Value Mapped type.
 * @tparam Hash  Hash functor for Key.
 * @tparam KeyEqual Equality functor for Key.
 */
template <typename Key, typename Value, typename Hash = std::hash<Key>, typename KeyEqual = std::equal_to<Key>>
class LruCache {
public:
    /**
     * @brief Create an empty cache.
     * @param capacity Maximum number of entries; must be > 0.
     * @throws std::invalid_argument if capacity is zero.
     */
    explicit LruCache(std::size_t capacity) : capacity_(capacity) {
        if (capacity_ == 0) {
            throw std::invalid_argument("LruCache capacity must be positive");
        }
        index_.reserve(capacity_);
    }

    /// @brief Deep copy that rebuilds the iterator index. @param other Cache to copy.
    LruCache(const LruCache& other) : capacity_(other.capacity_), items_(other.items_) { rebuildIndex(); }

    /// @brief Copy assignment (copy-and-swap). @param other Cache to copy. @return *this.
    LruCache& operator=(const LruCache& other) {
        if (this != &other) {
            LruCache copy(other);
            swap(copy);
        }
        return *this;
    }

    /// @brief Move constructor; list iterators stay valid across a list move.
    LruCache(LruCache&&) noexcept = default;
    /// @brief Move assignment. @return *this.
    LruCache& operator=(LruCache&&) noexcept = default;
    ~LruCache() = default;

    /**
     * @brief Swap contents with another cache.
     * @param other Cache to swap with.
     */
    void swap(LruCache& other) noexcept {
        using std::swap;
        swap(capacity_, other.capacity_);
        items_.swap(other.items_);
        index_.swap(other.index_);
    }

    /**
     * @brief Fetch a value and mark it most recently used.
     * @param key Key to look up.
     * @return Copy of the value, or std::nullopt on a miss.
     */
    [[nodiscard]] std::optional<Value> get(const Key& key) {
        const auto found = index_.find(key);
        if (found == index_.end()) {
            return std::nullopt;
        }
        items_.splice(items_.begin(), items_, found->second);
        return found->second->second;
    }

    /**
     * @brief Insert or update an entry, evicting the least recently used one when full.
     * @param key Key to insert.
     * @param value Value to store.
     * @return The evicted key, if an eviction happened.
     */
    std::optional<Key> put(const Key& key, Value value) {
        if (const auto found = index_.find(key); found != index_.end()) {
            found->second->second = std::move(value);
            items_.splice(items_.begin(), items_, found->second);
            return std::nullopt;
        }
        std::optional<Key> evicted;
        if (items_.size() == capacity_) {
            evicted = std::move(items_.back().first);
            index_.erase(*evicted);
            items_.pop_back();
        }
        items_.emplace_front(key, std::move(value));
        index_.emplace(key, items_.begin());
        return evicted;
    }

    /**
     * @brief Check membership without touching recency.
     * @param key Key to test.
     * @return true if present.
     */
    [[nodiscard]] bool contains(const Key& key) const { return index_.find(key) != index_.end(); }

    /**
     * @brief Remove an entry.
     * @param key Key to remove.
     * @return true if an entry was removed.
     */
    bool erase(const Key& key) {
        const auto found = index_.find(key);
        if (found == index_.end()) {
            return false;
        }
        items_.erase(found->second);
        index_.erase(found);
        return true;
    }

    /**
     * @brief Keys from most to least recently used.
     * @return Ordered key list.
     */
    [[nodiscard]] std::vector<Key> keysByRecency() const {
        std::vector<Key> keys;
        keys.reserve(items_.size());
        for (const auto& entry : items_) {
            keys.push_back(entry.first);
        }
        return keys;
    }

    /// @brief Number of cached entries. @return Entry count.
    [[nodiscard]] std::size_t size() const noexcept { return items_.size(); }
    /// @brief Maximum number of entries. @return Capacity.
    [[nodiscard]] std::size_t capacity() const noexcept { return capacity_; }
    /// @brief Whether the cache is empty. @return true when empty.
    [[nodiscard]] bool empty() const noexcept { return items_.empty(); }

private:
    using Entry = std::pair<Key, Value>;
    using ListIterator = typename std::list<Entry>::iterator;

    void rebuildIndex() {
        index_.clear();
        index_.reserve(capacity_);
        for (auto it = items_.begin(); it != items_.end(); ++it) {
            index_.emplace(it->first, it);
        }
    }

    std::size_t capacity_;
    std::list<Entry> items_;
    std::unordered_map<Key, ListIterator, Hash, KeyEqual> index_;
};

/**
 * @brief Maximum of every contiguous window of size @p window using a monotonic std::deque.
 *
 * Each index enters and leaves the deque once, so the algorithm is O(n) rather than O(n*k).
 * @param values Input sequence.
 * @param window Window width (> 0).
 * @return values.size() - window + 1 maxima, or empty if window > values.size().
 * @throws std::invalid_argument if window is zero.
 */
[[nodiscard]] std::vector<int> slidingWindowMaximum(std::span<const int> values, std::size_t window);

/**
 * @brief Compile-time table of squares 0^2 .. (N-1)^2 stored in a std::array.
 * @tparam N Number of entries.
 * @return The table.
 */
template <std::size_t N>
[[nodiscard]] constexpr std::array<std::size_t, N> makeSquares() noexcept {
    std::array<std::size_t, N> table{};
    for (std::size_t i = 0; i < N; ++i) {
        table[i] = i * i;
    }
    return table;
}

/**
 * @brief Group planet names by star system (std::map keeps systems sorted).
 * @param planets Planets to group.
 * @return system -> sorted planet names.
 */
[[nodiscard]] std::map<std::string, std::vector<std::string>> groupPlanetsBySystem(
    std::span<const Planet> planets);

/**
 * @brief Index planets by defense level in a std::multimap (duplicate keys allowed).
 * @param planets Planets to index.
 * @return defense level -> planet name, preserving input order among equal keys.
 */
[[nodiscard]] std::multimap<int, std::string> indexPlanetsByDefense(std::span<const Planet> planets);

/**
 * @brief Planet names whose defense level lies in [low, high] using multimap range queries.
 * @param index Index built by indexPlanetsByDefense.
 * @param low Inclusive lower bound.
 * @param high Inclusive upper bound.
 * @return Names in ascending defense order.
 */
[[nodiscard]] std::vector<std::string> planetsWithDefenseBetween(const std::multimap<int, std::string>& index,
                                                                 int low, int high);

/**
 * @brief Count lower-cased alphabetic words with a std::unordered_map.
 * @param text Input text; non-letters separate words.
 * @return word -> occurrence count.
 */
[[nodiscard]] std::unordered_map<std::string, std::size_t> wordFrequencies(std::string_view text);

/**
 * @brief The @p k most frequent words, by descending count then ascending word.
 * @param frequencies Counts from wordFrequencies.
 * @param k Number of entries wanted.
 * @return Up to k (word, count) pairs.
 */
[[nodiscard]] std::vector<std::pair<std::string, std::size_t>> topWords(
    const std::unordered_map<std::string, std::size_t>& frequencies, std::size_t k);

/**
 * @brief Check that (), [] and {} are balanced and properly nested, using std::stack.
 * @param text Text to check; other characters are ignored.
 * @return true if balanced.
 */
[[nodiscard]] bool isBalanced(std::string_view text);

/**
 * @brief Breadth-first visit order of a graph using std::queue.
 * @param graph Adjacency lists; neighbours are visited in list order.
 * @param start Start vertex (need not have an entry in @p graph).
 * @return Vertices in the order they were first reached, starting with @p start.
 */
[[nodiscard]] std::vector<int> breadthFirstOrder(const std::map<int, std::vector<int>>& graph, int start);

/**
 * @brief Priority-based task queue on std::priority_queue with FIFO tie-breaking.
 *
 * std::priority_queue is not stable, so a monotonically increasing sequence number is stored
 * with each task and used as the secondary key.
 */
class TaskScheduler {
public:
    /**
     * @brief A queued task.
     */
    struct Task {
        std::string name;         ///< Task name.
        int priority{0};          ///< Higher runs first.
        std::uint64_t sequence{}; ///< Submission order.
    };

    /**
     * @brief Submit a task.
     * @param name Task name.
     * @param priority Priority; higher values are served first.
     */
    void submit(std::string name, int priority);

    /**
     * @brief Remove and return the most urgent task.
     * @return The task, or std::nullopt when empty.
     */
    [[nodiscard]] std::optional<Task> next();

    /**
     * @brief Inspect the most urgent task without removing it.
     * @return Non-owning pointer to the top task, or nullptr when empty.
     */
    [[nodiscard]] const Task* peek() const noexcept;

    /// @brief Number of queued tasks. @return Task count.
    [[nodiscard]] std::size_t size() const noexcept { return queue_.size(); }
    /// @brief Whether no tasks are queued. @return true when empty.
    [[nodiscard]] bool empty() const noexcept { return queue_.empty(); }

private:
    struct LowerUrgency {
        bool operator()(const Task& lhs, const Task& rhs) const noexcept {
            if (lhs.priority != rhs.priority) {
                return lhs.priority < rhs.priority;
            }
            return lhs.sequence > rhs.sequence;
        }
    };

    std::priority_queue<Task, std::vector<Task>, LowerUrgency> queue_;
    std::uint64_t next_sequence_{0};
};

/**
 * @brief Narrate vector, deque, list, forward_list and array usage.
 * @param out Destination stream.
 */
void demonstrateSequenceContainers(std::ostream& out = std::cout);

/**
 * @brief Narrate map, multimap, set, unordered_map and unordered_set usage.
 * @param out Destination stream.
 */
void demonstrateAssociativeContainers(std::ostream& out = std::cout);

/**
 * @brief Narrate stack, queue and priority_queue usage.
 * @param out Destination stream.
 */
void demonstrateContainerAdapters(std::ostream& out = std::cout);

/**
 * @brief Run every container demonstration.
 * @param out Destination stream.
 */
void runContainersDemo(std::ostream& out = std::cout);

}  // namespace CppVerseHub::STL
