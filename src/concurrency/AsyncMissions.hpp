/**
 * @file AsyncMissions.hpp
 * @brief Futures-based task orchestration: `std::async`, promises, cooperative cancellation,
 *        dependency graphs, data-parallel map, `when_all` and staged pipelines.
 * @details File location: src/concurrency/AsyncMissions.hpp
 *
 * Futures decouple *starting* work from *consuming* its result. This header builds the
 * higher-level patterns on top of them, using a space-mission vocabulary:
 *
 *  - `CancellationSource`/`CancellationToken`: cooperative cancellation (threads cannot be
 *    killed safely; work must poll a flag).
 *  - `AsyncMission<T>`: a named unit of work launched with `std::async`, observable status,
 *    results/errors captured in a `MissionResult<T>`.
 *  - `MissionCoordinator`: executes a DAG of missions on a `ThreadPool` as soon as each
 *    mission's prerequisites succeed; failures cancel all transitive dependents; cycles are
 *    rejected up front (Kahn's algorithm).
 *  - `parallel_transform`, `when_all`: fork/join helpers preserving input order.
 *  - `Pipeline<T>`: one thread per stage connected by bounded queues (pipeline parallelism).
 */

#ifndef CPPVERSEHUB_CONCURRENCY_ASYNCMISSIONS_HPP
#define CPPVERSEHUB_CONCURRENCY_ASYNCMISSIONS_HPP

#include <algorithm>
#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <functional>
#include <future>
#include <iostream>
#include <memory>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>

#include "concurrency/ConditionalVariables.hpp"
#include "concurrency/ThreadPool.hpp"

namespace CppVerseHub::Concurrency {

    /** @brief Life-cycle state of a mission. */
    enum class MissionStatus : std::uint8_t { Pending, Running, Succeeded, Failed, Cancelled };

    /** @brief @param status Status. @return Human-readable name. */
    [[nodiscard]] std::string_view to_string(MissionStatus status) noexcept;

    /** @brief Thrown by `CancellationToken::throw_if_cancelled()`. */
    class MissionCancelled : public std::runtime_error {
    public:
        /** @brief Constructs the exception. */
        MissionCancelled() : std::runtime_error("mission cancelled") {}
    };

    /** @brief Read side of a cancellation flag; cheap to copy and share between threads. */
    class CancellationToken {
    public:
        /** @brief Token that is never cancelled. */
        CancellationToken() = default;
        /** @brief @return true once the owning source requested cancellation. */
        [[nodiscard]] bool is_cancelled() const noexcept {
            // acquire: pairs with the release in cancel(), so data written before cancel()
            // (e.g. a reason) is visible to the observer.
            return flag_ && flag_->load(std::memory_order_acquire);
        }
        /** @brief @throws MissionCancelled if cancellation was requested. */
        void throw_if_cancelled() const {
            if (is_cancelled()) {
                throw MissionCancelled{};
            }
        }

    private:
        friend class CancellationSource;
        explicit CancellationToken(std::shared_ptr<const std::atomic<bool>> flag) noexcept : flag_(std::move(flag)) {}
        std::shared_ptr<const std::atomic<bool>> flag_;
    };

    /** @brief Write side of a cancellation flag. */
    class CancellationSource {
    public:
        /** @brief Creates a fresh, non-cancelled flag. */
        CancellationSource() : flag_(std::make_shared<std::atomic<bool>>(false)) {}
        /** @brief Requests cancellation (idempotent). */
        void cancel() noexcept { flag_->store(true, std::memory_order_release); }
        /** @brief @return true if `cancel()` was called. */
        [[nodiscard]] bool is_cancelled() const noexcept { return flag_->load(std::memory_order_acquire); }
        /** @brief @return Token observing this source. */
        [[nodiscard]] CancellationToken token() const noexcept { return CancellationToken{flag_}; }

    private:
        std::shared_ptr<std::atomic<bool>> flag_;
    };

    /**
     * @brief Outcome of a mission.
     * @tparam T Value type.
     */
    template <typename T>
    struct MissionResult {
        MissionStatus status = MissionStatus::Pending;  ///< Final status.
        std::optional<T> value;                         ///< Set iff `status == Succeeded`.
        std::string error;                              ///< Failure description, if any.
        /** @brief @return true if the mission succeeded. */
        [[nodiscard]] bool ok() const noexcept { return status == MissionStatus::Succeeded; }
    };

    /**
     * @brief A named asynchronous computation launched with `std::async`.
     *
     * The callable receives a `CancellationToken` it should poll. Exceptions become a
     * `Failed` result; `MissionCancelled` becomes `Cancelled`. The destructor waits for a
     * started mission to finish, so the mission object can never dangle under its thread.
     * @tparam T Result type.
     */
    template <typename T>
    class AsyncMission {
    public:
        using Work = std::function<T(const CancellationToken&)>;  ///< Mission body.

        /**
         * @brief Creates a pending mission.
         * @param name Mission name.
         * @param work Mission body.
         */
        AsyncMission(std::string name, Work work) : name_(std::move(name)), work_(std::move(work)) {}

        AsyncMission(const AsyncMission&) = delete;
        AsyncMission& operator=(const AsyncMission&) = delete;
        AsyncMission(AsyncMission&&) = delete;
        AsyncMission& operator=(AsyncMission&&) = delete;
        /** @brief Waits for a started mission to finish. */
        ~AsyncMission() {
            if (result_.valid()) {
                result_.wait();
            }
        }

        /**
         * @brief Launches the mission.
         * @param policy `std::launch::async` (new thread) or `std::launch::deferred` (lazy, runs on `get`).
         * @return Shared future for the result (can be read by several consumers).
         * @throws std::logic_error if already started.
         */
        std::shared_future<MissionResult<T>> start(std::launch policy = std::launch::async) {
            if (result_.valid()) {
                throw std::logic_error("mission '" + name_ + "' already started");
            }
            result_ = std::async(policy, [this] { return execute(); }).share();
            return result_;
        }

        /** @brief Requests cooperative cancellation. */
        void cancel() noexcept { cancel_.cancel(); }
        /** @brief @return Current status. */
        [[nodiscard]] MissionStatus status() const noexcept { return status_.load(); }
        /** @brief @return Mission name. */
        [[nodiscard]] const std::string& name() const noexcept { return name_; }

    private:
        MissionResult<T> execute() {
            MissionResult<T> result;
            if (cancel_.is_cancelled()) {
                result.status = MissionStatus::Cancelled;
                status_.store(result.status);
                return result;
            }
            status_.store(MissionStatus::Running);
            try {
                result.value.emplace(work_(cancel_.token()));
                result.status = MissionStatus::Succeeded;
            } catch (const MissionCancelled& e) {
                result.status = MissionStatus::Cancelled;
                result.error = e.what();
            } catch (const std::exception& e) {
                result.status = MissionStatus::Failed;
                result.error = e.what();
            } catch (...) {
                result.status = MissionStatus::Failed;
                result.error = "unknown exception";
            }
            status_.store(result.status);
            return result;
        }

        std::string name_;
        Work work_;
        CancellationSource cancel_;
        std::atomic<MissionStatus> status_{MissionStatus::Pending};
        std::shared_future<MissionResult<T>> result_;
    };

    /**
     * @brief Executes a dependency graph of missions on a thread pool.
     *
     * A mission starts as soon as all of its prerequisites have *succeeded*. If a mission
     * fails (throws), every mission that transitively depends on it is marked `Cancelled`
     * without running. Independent branches keep running.
     */
    class MissionCoordinator {
    public:
        using MissionId = std::size_t;                              ///< Index of a mission.
        using Work = std::function<void(const CancellationToken&)>;  ///< Mission body.

        /**
         * @brief Adds a mission.
         * @param name Mission name.
         * @param work Body.
         * @param prerequisites Missions that must succeed first (must already exist).
         * @return New mission's id.
         * @throws std::out_of_range for an unknown prerequisite; std::logic_error while running.
         */
        MissionId add_mission(std::string name, Work work, const std::vector<MissionId>& prerequisites = {});

        /**
         * @brief Adds an edge `prerequisite -> dependent` (may create a cycle, detected by `run`).
         * @param dependent Mission that waits.
         * @param prerequisite Mission waited for.
         * @throws std::out_of_range for unknown ids.
         */
        void add_dependency(MissionId dependent, MissionId prerequisite);

        /**
         * @brief Topological order of all missions (Kahn's algorithm, smallest id first).
         * @return Order, or `std::nullopt` if the graph has a cycle.
         */
        [[nodiscard]] std::optional<std::vector<MissionId>> topological_order() const;

        /**
         * @brief Runs every mission and blocks until all are finished or cancelled.
         * @param pool Executor (must not be shut down; must not be the calling thread's pool).
         * @throws std::logic_error if the graph has a cycle or `run` was already called.
         */
        void run(ThreadPool& pool);

        /** @brief Requests cancellation of every mission that has not started yet / polls the token. */
        void cancel_all() noexcept { cancel_.cancel(); }

        /** @brief @param id Mission. @return Its status. */
        [[nodiscard]] MissionStatus status(MissionId id) const;
        /** @brief @param id Mission. @return Its name. */
        [[nodiscard]] const std::string& name(MissionId id) const;
        /** @brief @param id Mission. @return Error message for failed missions. */
        [[nodiscard]] std::string error(MissionId id) const;
        /** @brief @return Ids in the order in which they finished running (succeeded or failed). */
        [[nodiscard]] std::vector<MissionId> completion_order() const;
        /** @brief @return Number of missions. */
        [[nodiscard]] std::size_t size() const;

    private:
        struct Node {
            std::string name;
            Work work;
            std::vector<MissionId> dependents;
            std::size_t prerequisite_count = 0;
            std::size_t remaining = 0;
            MissionStatus status = MissionStatus::Pending;
            std::string error;
        };

        [[nodiscard]] std::optional<std::vector<MissionId>> topological_order_locked() const;
        void launch(ThreadPool& pool, MissionId id);
        void on_finished(ThreadPool& pool, MissionId id, MissionStatus status, std::string error);
        void cancel_dependents_locked(MissionId id);

        mutable std::mutex mutex_;
        std::condition_variable done_cv_;
        std::vector<Node> nodes_;
        std::vector<MissionId> completion_order_;
        std::size_t unfinished_ = 0;
        bool started_ = false;
        CancellationSource cancel_;
    };

    /**
     * @brief Applies `fn` to every element in parallel (chunked) and returns results in input order.
     * @param pool Executor.
     * @param inputs Input elements.
     * @param fn Transformation `Out(const In&)`; invoked concurrently, so it must be thread safe.
     * @param chunk_size Elements per task (0 = choose automatically).
     * @return Transformed elements, `result[i] == fn(inputs[i])`.
     * @throws Whatever `fn` threw for the lowest-indexed failing chunk (after all chunks finish).
     */
    template <typename In, typename F>
    [[nodiscard]] auto parallel_transform(ThreadPool& pool, const std::vector<In>& inputs, F fn,
                                          std::size_t chunk_size = 0)
        -> std::vector<std::invoke_result_t<F&, const In&>> {
        using Out = std::invoke_result_t<F&, const In&>;
        static_assert(std::is_default_constructible_v<Out>, "parallel_transform requires a default-constructible result");
        std::vector<Out> outputs(inputs.size());
        if (inputs.empty()) {
            return outputs;
        }
        if (chunk_size == 0) {
            chunk_size = std::max<std::size_t>(1, inputs.size() / (pool.thread_count() * 4));
        }
        std::vector<std::future<void>> chunks;
        std::exception_ptr first_error;
        try {
            for (std::size_t lo = 0; lo < inputs.size(); lo += chunk_size) {
                const std::size_t hi = std::min(lo + chunk_size, inputs.size());
                // Each chunk writes a disjoint index range of `outputs`: no data race.
                chunks.push_back(pool.submit([&inputs, &outputs, &fn, lo, hi] {
                    for (std::size_t i = lo; i < hi; ++i) {
                        outputs[i] = fn(inputs[i]);
                    }
                }));
            }
        } catch (...) {
            first_error = std::current_exception();  // e.g. PoolShutdownError; still join the rest
        }
        for (auto& chunk : chunks) {
            try {
                chunk.get();  // wait for every chunk: they reference our locals
            } catch (...) {
                if (!first_error) {
                    first_error = std::current_exception();
                }
            }
        }
        if (first_error) {
            std::rethrow_exception(first_error);
        }
        return outputs;
    }

    /**
     * @brief Waits for every future and collects the values in order.
     * @param futures Futures to join (consumed).
     * @return Values in the same order.
     * @throws The first (by index) stored exception, after all futures are ready.
     */
    template <typename T>
    [[nodiscard]] std::vector<T> when_all(std::vector<std::future<T>> futures) {
        std::vector<T> values;
        values.reserve(futures.size());
        std::exception_ptr first_error;
        for (auto& f : futures) {
            try {
                values.push_back(f.get());
            } catch (...) {
                if (!first_error) {
                    first_error = std::current_exception();
                }
            }
        }
        if (first_error) {
            std::rethrow_exception(first_error);
        }
        return values;
    }

    /**
     * @brief Linear pipeline where every stage runs on its own thread, connected by bounded queues.
     *
     * Different items occupy different stages at the same time (pipeline parallelism), while
     * each stage processes items strictly in order, so output order equals input order.
     * @tparam T Item type flowing through all stages.
     */
    template <typename T>
    class Pipeline {
    public:
        using Stage = std::function<T(T)>;  ///< Stage transformation.

        /**
         * @brief Appends a stage.
         * @param name Stage name (for diagnostics).
         * @param stage Transformation.
         * @return *this for chaining.
         */
        Pipeline& add_stage(std::string name, Stage stage) {
            stages_.push_back(NamedStage{std::move(name), std::move(stage)});
            return *this;
        }

        /** @brief @return Number of stages. */
        [[nodiscard]] std::size_t stage_count() const noexcept { return stages_.size(); }

        /** @brief @return Stage names in order. */
        [[nodiscard]] std::vector<std::string> stage_names() const {
            std::vector<std::string> names;
            for (const auto& s : stages_) {
                names.push_back(s.name);
            }
            return names;
        }

        /**
         * @brief Pushes all inputs through the pipeline.
         * @param inputs Items (consumed).
         * @param queue_capacity Capacity of each inter-stage queue.
         * @return Outputs in input order.
         * @throws The first exception thrown by any stage (remaining items are discarded).
         */
        [[nodiscard]] std::vector<T> process(std::vector<T> inputs, std::size_t queue_capacity = 16) const {
            if (stages_.empty()) {
                return inputs;
            }
            std::vector<std::unique_ptr<BoundedQueue<T>>> queues;
            for (std::size_t i = 0; i <= stages_.size(); ++i) {
                queues.push_back(std::make_unique<BoundedQueue<T>>(queue_capacity));
            }
            std::mutex error_mutex;
            std::exception_ptr error;
            std::vector<std::thread> workers;
            workers.reserve(stages_.size());
            for (std::size_t s = 0; s < stages_.size(); ++s) {
                workers.emplace_back([&, s] {
                    BoundedQueue<T>& in = *queues[s];
                    BoundedQueue<T>& out = *queues[s + 1];
                    while (auto item = in.pop()) {
                        try {
                            if (!out.push(stages_[s].stage(std::move(*item)))) {
                                break;
                            }
                        } catch (...) {
                            {
                                std::lock_guard lock(error_mutex);
                                if (!error) {
                                    error = std::current_exception();
                                }
                            }
                            for (auto& q : queues) {
                                q->close();  // unblock every stage, discard the rest
                            }
                            break;
                        }
                    }
                    out.close();
                });
            }
            std::vector<T> outputs;
            outputs.reserve(inputs.size());
            std::thread feeder([&] {
                for (auto& item : inputs) {
                    if (!queues.front()->push(std::move(item))) {
                        break;
                    }
                }
                queues.front()->close();
            });
            while (auto item = queues.back()->pop()) {
                outputs.push_back(std::move(*item));
            }
            feeder.join();
            for (auto& w : workers) {
                w.join();
            }
            if (error) {
                std::rethrow_exception(error);
            }
            return outputs;
        }

    private:
        struct NamedStage {
            std::string name;
            Stage stage;
        };
        std::vector<NamedStage> stages_;
    };

    /**
     * @brief Showcase: std::async/promise/packaged_task, cancellation, mission DAG, parallel map, pipeline.
     * @param out Destination stream.
     */
    void demonstrate_async_missions(std::ostream& out = std::cout);

}  // namespace CppVerseHub::Concurrency

#endif  // CPPVERSEHUB_CONCURRENCY_ASYNCMISSIONS_HPP
