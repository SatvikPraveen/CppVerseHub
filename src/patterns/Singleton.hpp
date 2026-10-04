/**
 * @file Singleton.hpp
 * @brief Thread-safe Meyers singleton via CRTP, plus three typical singleton services.
 *
 * Since C++11 the initialisation of a function-local static is guaranteed to happen exactly once,
 * even when several threads call the function concurrently ("magic statics", [stmt.dcl]/4). The
 * Meyers singleton exploits this: `Singleton<T>::instance()` needs no explicit mutex, no
 * `std::call_once`, and no heap allocation, and the instance is destroyed automatically at program
 * exit in reverse order of construction.
 *
 * The services themselves must still synchronise their *mutable state*: `ConfigManager` uses a
 * `std::shared_mutex` (many readers / one writer), `LogManager` a plain mutex, and `IdGenerator`
 * a lock-free atomic counter.
 *
 * Caveat (documented, not hidden): singletons are global state. Prefer dependency injection in new
 * code; when a singleton is unavoidable, give it a `reset()` for tests, as these services do.
 */

#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <iostream>
#include <mutex>
#include <optional>
#include <shared_mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace CppVerseHub::Patterns {

/**
 * @brief CRTP base providing a lazily-constructed, thread-safe unique instance of @p Derived.
 *
 * Usage:
 * @code
 * class Service final : public Singleton<Service> {
 *     friend class Singleton<Service>;
 *     Service() = default;
 * };
 * Service::instance().doWork();
 * @endcode
 *
 * @tparam Derived The singleton class; its constructor should be private and befriend Singleton.
 */
template <typename Derived>
class Singleton {
public:
    Singleton(const Singleton&) = delete;
    Singleton& operator=(const Singleton&) = delete;
    Singleton(Singleton&&) = delete;
    Singleton& operator=(Singleton&&) = delete;

    /**
     * @brief Access the unique instance, constructing it on first use.
     * @return Reference to the instance (valid until static destruction).
     */
    [[nodiscard]] static Derived& instance() {
        static Derived inst;  // thread-safe initialisation guaranteed by the language
        return inst;
    }

protected:
    Singleton() = default;
    ~Singleton() = default;
};

/**
 * @brief Process-wide string key/value configuration (readers never block each other).
 */
class ConfigManager final : public Singleton<ConfigManager> {
    friend class Singleton<ConfigManager>;

public:
    /**
     * @brief Set (or overwrite) a value.
     * @param key Key.
     * @param value Value.
     */
    void set(const std::string& key, std::string value) {
        std::unique_lock lock(mutex_);
        values_[key] = std::move(value);
    }

    /**
     * @brief Look up a value.
     * @param key Key.
     * @return The value, or std::nullopt if absent.
     */
    [[nodiscard]] std::optional<std::string> get(const std::string& key) const {
        std::shared_lock lock(mutex_);
        const auto it = values_.find(key);
        if (it == values_.end()) {
            return std::nullopt;
        }
        return it->second;
    }

    /**
     * @brief Look up a value with a fallback.
     * @param key Key.
     * @param fallback Returned when the key is absent.
     * @return The value or @p fallback.
     */
    [[nodiscard]] std::string getOr(const std::string& key, const std::string& fallback) const {
        return get(key).value_or(fallback);
    }

    /// @brief Whether a key exists. @param key Key. @return Flag.
    [[nodiscard]] bool contains(const std::string& key) const {
        std::shared_lock lock(mutex_);
        return values_.count(key) != 0;
    }

    /// @brief Number of entries. @return Count.
    [[nodiscard]] std::size_t size() const {
        std::shared_lock lock(mutex_);
        return values_.size();
    }

    /// @brief Remove every entry (for tests). @return void
    void reset() {
        std::unique_lock lock(mutex_);
        values_.clear();
    }

private:
    ConfigManager() = default;
    ~ConfigManager() = default;

    mutable std::shared_mutex mutex_;
    std::unordered_map<std::string, std::string> values_;
};

/// @brief Severity of a log record.
enum class LogLevel { Debug, Info, Warning, Error };

/**
 * @brief In-memory bounded log sink. Never prints; callers decide where records go.
 */
class LogManager final : public Singleton<LogManager> {
    friend class Singleton<LogManager>;

public:
    /// @brief A stored log record.
    struct Record {
        LogLevel level;       ///< Severity.
        std::string message;  ///< Text.
    };

    /// @brief Maximum number of retained records (oldest dropped first).
    static constexpr std::size_t kCapacity = 1024;

    /**
     * @brief Append a record if @p level passes the minimum level.
     * @param level Severity.
     * @param message Text.
     */
    void log(LogLevel level, std::string message) {
        if (level < minLevel_.load(std::memory_order_relaxed)) {
            return;
        }
        std::scoped_lock lock(mutex_);
        records_.push_back({level, std::move(message)});
        if (records_.size() > kCapacity) {
            records_.pop_front();
        }
    }

    /// @brief Set the minimum level that is recorded. @param level Threshold.
    void setMinLevel(LogLevel level) noexcept { minLevel_.store(level, std::memory_order_relaxed); }

    /// @brief Copy of the retained records. @return Records, oldest first.
    [[nodiscard]] std::vector<Record> records() const {
        std::scoped_lock lock(mutex_);
        return {records_.begin(), records_.end()};
    }

    /// @brief Number of retained records. @return Count.
    [[nodiscard]] std::size_t size() const {
        std::scoped_lock lock(mutex_);
        return records_.size();
    }

    /// @brief Clear records and restore the default level (for tests). @return void
    void reset() {
        std::scoped_lock lock(mutex_);
        records_.clear();
        minLevel_.store(LogLevel::Debug, std::memory_order_relaxed);
    }

private:
    LogManager() = default;
    ~LogManager() = default;

    mutable std::mutex mutex_;
    std::deque<Record> records_;
    std::atomic<LogLevel> minLevel_{LogLevel::Debug};
};

/**
 * @brief Lock-free generator of process-unique, monotonically increasing ids.
 */
class IdGenerator final : public Singleton<IdGenerator> {
    friend class Singleton<IdGenerator>;

public:
    /// @brief Next id (starts at 1). @return A value never returned before.
    [[nodiscard]] std::uint64_t next() noexcept { return counter_.fetch_add(1, std::memory_order_relaxed) + 1; }

    /// @brief Number of ids issued so far. @return Count.
    [[nodiscard]] std::uint64_t issued() const noexcept { return counter_.load(std::memory_order_relaxed); }

private:
    IdGenerator() = default;
    ~IdGenerator() = default;

    std::atomic<std::uint64_t> counter_{0};
};

/**
 * @brief Showcase the singleton services.
 * @param out Stream receiving the narration.
 */
inline void demonstrateSingleton(std::ostream& out = std::cout) {
    out << "=== Singleton pattern ===\n";
    auto& config = ConfigManager::instance();
    config.set("galaxy.name", "Andromeda");
    out << "  same ConfigManager instance: " << std::boolalpha
        << (&config == &ConfigManager::instance()) << '\n';
    out << "  galaxy.name = " << ConfigManager::instance().getOr("galaxy.name", "?") << '\n';

    auto& ids = IdGenerator::instance();
    const auto a = ids.next();
    const auto b = ids.next();
    out << "  ids are strictly increasing: " << (b > a) << '\n';

    LogManager::instance().log(LogLevel::Info, "singleton demo ran");
    out << "  log records retained: " << LogManager::instance().size() << '\n';
}

}  // namespace CppVerseHub::Patterns
