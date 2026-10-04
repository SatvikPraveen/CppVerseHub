/**
 * @file MutexExamples.hpp
 * @brief Mutual exclusion: lock types, deadlock avoidance and mutex-protected data structures.
 * @details File location: src/concurrency/MutexExamples.hpp
 *
 * Shows the standard toolbox for protecting shared state and the classic ways to
 * avoid deadlock:
 *
 *  - `HierarchicalMutex`: enforces a global lock order at run time (lock-order
 *    violations throw instead of deadlocking sometimes, in production).
 *  - `BankAccount` + `transfer()`: multi-lock acquisition with `std::scoped_lock`, whose
 *    deadlock-avoidance algorithm makes the argument order irrelevant.
 *  - `run_dining_philosophers()`: the textbook deadlock scenario, solved the same way.
 *  - `Synchronized<T>`: couples data with its mutex so it cannot be touched unlocked.
 *  - `ThreadSafeQueue<T>` / `ThreadSafeMap<K, V>`: exclusive vs reader/writer
 *    (`std::shared_mutex`) locking.
 *  - `LazyValue<T>`: thread-safe one-time initialisation via `std::call_once`.
 */

#ifndef CPPVERSEHUB_CONCURRENCY_MUTEXEXAMPLES_HPP
#define CPPVERSEHUB_CONCURRENCY_MUTEXEXAMPLES_HPP

#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <functional>
#include <iostream>
#include <limits>
#include <map>
#include <mutex>
#include <optional>
#include <shared_mutex>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

namespace CppVerseHub::Concurrency {

    /**
     * @brief Mutex with a hierarchy level; a thread may only lock strictly lower levels
     *        than the lowest one it already holds.
     *
     * If every thread respects one order, a cycle in the wait-for graph is impossible.
     * Violations throw `std::logic_error` deterministically, turning a rare deadlock into a
     * reproducible bug report. Meets *Lockable*; must be unlocked in reverse lock order.
     */
    class HierarchicalMutex {
    public:
        /**
         * @brief Creates the mutex.
         * @param level Hierarchy level (higher levels must be locked first).
         */
        explicit HierarchicalMutex(std::uint64_t level) noexcept : level_(level) {}

        /** @brief Locks. @throws std::logic_error on hierarchy violation. */
        void lock();
        /** @brief Unlocks and restores the thread's previous level. */
        void unlock() noexcept;
        /** @brief @return true if locked. @throws std::logic_error on hierarchy violation. */
        [[nodiscard]] bool try_lock();
        /** @brief @return This mutex's level. */
        [[nodiscard]] std::uint64_t level() const noexcept { return level_; }
        /** @brief @return The current thread's hierarchy ceiling (max value if none held). */
        [[nodiscard]] static std::uint64_t current_thread_level() noexcept;

    private:
        void check_for_violation() const;
        void update_level() noexcept;

        std::mutex mutex_;
        const std::uint64_t level_;
        std::uint64_t previous_level_ = 0;  // only touched by the owner
    };

    /**
     * @brief Value plus mutex; the value is only reachable through a locked callback.
     * @tparam T Protected type.
     */
    template <typename T>
    class Synchronized {
    public:
        /**
         * @brief Constructs the protected value in place.
         * @param args Constructor arguments for `T`.
         */
        template <typename... Args>
        explicit Synchronized(Args&&... args) : value_(std::forward<Args>(args)...) {}

        /**
         * @brief Runs `fn(T&)` under an exclusive lock.
         * @param fn Callback.
         * @return Whatever `fn` returns.
         */
        template <typename F>
        decltype(auto) with_lock(F&& fn) {
            std::unique_lock lock(mutex_);
            return std::invoke(std::forward<F>(fn), value_);
        }

        /**
         * @brief Runs `fn(const T&)` under a shared (reader) lock.
         * @param fn Callback.
         * @return Whatever `fn` returns.
         */
        template <typename F>
        decltype(auto) with_shared_lock(F&& fn) const {
            std::shared_lock lock(mutex_);
            return std::invoke(std::forward<F>(fn), value_);
        }

        /** @brief @return Copy of the value taken under a shared lock. */
        [[nodiscard]] T copy() const {
            std::shared_lock lock(mutex_);
            return value_;
        }

    private:
        mutable std::shared_mutex mutex_;
        T value_;
    };

    /**
     * @brief Unbounded mutex-protected FIFO queue with blocking pop and close.
     * @tparam T Element type.
     */
    template <typename T>
    class ThreadSafeQueue {
    public:
        /**
         * @brief Enqueues a value.
         * @param value Element.
         * @return false if the queue is closed.
         */
        template <typename U = T>
        bool push(U&& value) {
            {
                std::lock_guard lock(mutex_);
                if (closed_) {
                    return false;
                }
                items_.emplace_back(std::forward<U>(value));
            }
            cv_.notify_one();
            return true;
        }

        /** @brief @return Front element, or `std::nullopt` if empty. */
        [[nodiscard]] std::optional<T> try_pop() {
            std::lock_guard lock(mutex_);
            return take_locked();
        }

        /** @brief Blocks for an element. @return Element, or `std::nullopt` once closed and empty. */
        [[nodiscard]] std::optional<T> wait_and_pop() {
            std::unique_lock lock(mutex_);
            cv_.wait(lock, [this] { return closed_ || !items_.empty(); });
            return take_locked();
        }

        /** @brief Closes the queue, waking blocked consumers. */
        void close() {
            {
                std::lock_guard lock(mutex_);
                closed_ = true;
            }
            cv_.notify_all();
        }

        /** @brief @return Element count (snapshot). */
        [[nodiscard]] std::size_t size() const {
            std::lock_guard lock(mutex_);
            return items_.size();
        }
        /** @brief @return true if empty (snapshot). */
        [[nodiscard]] bool empty() const { return size() == 0; }

    private:
        std::optional<T> take_locked() {
            if (items_.empty()) {
                return std::nullopt;
            }
            std::optional<T> value(std::move(items_.front()));
            items_.pop_front();
            return value;
        }

        mutable std::mutex mutex_;
        std::condition_variable cv_;
        std::deque<T> items_;
        bool closed_ = false;
    };

    /**
     * @brief Hash map guarded by a `std::shared_mutex`: concurrent readers, exclusive writers.
     *
     * Lookups return copies, never references, because a reference would escape the lock.
     * @tparam K Key type.
     * @tparam V Mapped type.
     */
    template <typename K, typename V>
    class ThreadSafeMap {
    public:
        /**
         * @brief Inserts or overwrites.
         * @param key Key.
         * @param value Value.
         * @return true if a new key was inserted.
         */
        bool insert_or_assign(const K& key, V value) {
            std::unique_lock lock(mutex_);
            return map_.insert_or_assign(key, std::move(value)).second;
        }

        /**
         * @brief Looks up a key under a shared lock.
         * @param key Key.
         * @return Copy of the value, or `std::nullopt`.
         */
        [[nodiscard]] std::optional<V> find(const K& key) const {
            std::shared_lock lock(mutex_);
            const auto it = map_.find(key);
            if (it == map_.end()) {
                return std::nullopt;
            }
            return it->second;
        }

        /**
         * @brief Atomically read-modify-writes the value for `key` (default-constructed if absent).
         * @param key Key.
         * @param fn Callback `fn(V&)`.
         */
        template <typename F>
        void update(const K& key, F&& fn) {
            std::unique_lock lock(mutex_);
            std::invoke(std::forward<F>(fn), map_[key]);
        }

        /** @brief Removes a key. @param key Key. @return true if it was present. */
        bool erase(const K& key) {
            std::unique_lock lock(mutex_);
            return map_.erase(key) != 0;
        }

        /** @brief @param key Key. @return true if present. */
        [[nodiscard]] bool contains(const K& key) const {
            std::shared_lock lock(mutex_);
            return map_.find(key) != map_.end();
        }

        /** @brief @return Number of entries. */
        [[nodiscard]] std::size_t size() const {
            std::shared_lock lock(mutex_);
            return map_.size();
        }

        /** @brief @return Ordered copy of all entries, taken under one shared lock. */
        [[nodiscard]] std::map<K, V> snapshot() const {
            std::shared_lock lock(mutex_);
            return std::map<K, V>(map_.begin(), map_.end());
        }

    private:
        mutable std::shared_mutex mutex_;
        std::unordered_map<K, V> map_;
    };

    /**
     * @brief Account whose balance is protected by its own mutex.
     */
    class BankAccount {
    public:
        /**
         * @brief Opens an account.
         * @param id Identifier.
         * @param initial_balance Opening balance in cents.
         */
        BankAccount(std::string id, std::int64_t initial_balance) : id_(std::move(id)), balance_(initial_balance) {}

        /** @brief @return Current balance (cents). */
        [[nodiscard]] std::int64_t balance() const;
        /** @brief Adds money. @param amount Cents (non-negative). */
        void deposit(std::int64_t amount);
        /** @brief @return Account identifier. */
        [[nodiscard]] const std::string& id() const noexcept { return id_; }

        /**
         * @brief Moves money between two accounts atomically and deadlock-free.
         *
         * Both mutexes are acquired with `std::scoped_lock`, so `transfer(a, b)` and
         * `transfer(b, a)` running concurrently cannot deadlock.
         * @param from Source account.
         * @param to Destination account.
         * @param amount Cents to move (must be positive).
         * @return false if `from` has insufficient funds, `amount <= 0`, or `from` is `to`.
         */
        friend bool transfer(BankAccount& from, BankAccount& to, std::int64_t amount);

    private:
        std::string id_;
        mutable std::mutex mutex_;
        std::int64_t balance_;
    };

    /** @brief See `BankAccount`. */
    bool transfer(BankAccount& from, BankAccount& to, std::int64_t amount);

    /**
     * @brief Runs the dining-philosophers problem with `std::scoped_lock` on both forks.
     * @param philosophers Number of philosophers / forks (>= 2).
     * @param meals Meals each philosopher eats.
     * @return Meals eaten per philosopher (each equals `meals`; there is no deadlock).
     * @throws std::invalid_argument if `philosophers < 2`.
     */
    [[nodiscard]] std::vector<int> run_dining_philosophers(std::size_t philosophers, int meals);

    /**
     * @brief Thread-safe lazily computed value (`std::call_once`).
     * @tparam T Value type.
     */
    template <typename T>
    class LazyValue {
    public:
        /**
         * @brief Stores the factory; nothing is computed yet.
         * @param factory Callable producing the value; invoked at most once successfully.
         */
        explicit LazyValue(std::function<T()> factory) : factory_(std::move(factory)) {}

        /**
         * @brief Computes the value on first call (concurrent callers wait), then returns it.
         *
         * If the factory throws, the exception propagates and a later call retries.
         * @return Reference to the value.
         */
        [[nodiscard]] const T& get() {
            std::call_once(once_, [this] { value_.emplace(factory_()); });
            return *value_;
        }

        /** @brief @return true once the value has been computed. Only call after a `get()` synchronises. */
        [[nodiscard]] bool has_value() const noexcept { return value_.has_value(); }

    private:
        std::function<T()> factory_;
        std::once_flag once_;
        std::optional<T> value_;
    };

    /**
     * @brief Showcase: lock types, timed and recursive mutexes, hierarchy, transfers, philosophers.
     * @param out Destination stream.
     */
    void demonstrate_mutexes(std::ostream& out = std::cout);

}  // namespace CppVerseHub::Concurrency

#endif  // CPPVERSEHUB_CONCURRENCY_MUTEXEXAMPLES_HPP
