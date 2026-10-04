/**
 * @file AsyncMissions.cpp
 * @brief Mission DAG coordinator and the async-missions showcase.
 * @details File location: src/concurrency/AsyncMissions.cpp
 */

#include "concurrency/AsyncMissions.hpp"

#include <functional>
#include <numeric>
#include <queue>

namespace CppVerseHub::Concurrency {

    std::string_view to_string(MissionStatus status) noexcept {
        switch (status) {
            case MissionStatus::Pending:
                return "pending";
            case MissionStatus::Running:
                return "running";
            case MissionStatus::Succeeded:
                return "succeeded";
            case MissionStatus::Failed:
                return "failed";
            case MissionStatus::Cancelled:
                return "cancelled";
        }
        return "unknown";
    }

    // ---------------------------------------------------------------- MissionCoordinator

    MissionCoordinator::MissionId MissionCoordinator::add_mission(std::string name, Work work,
                                                                  const std::vector<MissionId>& prerequisites) {
        std::lock_guard lock(mutex_);
        if (started_) {
            throw std::logic_error("cannot add missions after run()");
        }
        const MissionId id = nodes_.size();
        for (const MissionId p : prerequisites) {
            if (p >= id) {
                throw std::out_of_range("unknown prerequisite mission id " + std::to_string(p));
            }
        }
        nodes_.push_back(Node{std::move(name), std::move(work), {}, prerequisites.size(), 0, MissionStatus::Pending, {}});
        for (const MissionId p : prerequisites) {
            nodes_[p].dependents.push_back(id);
        }
        return id;
    }

    void MissionCoordinator::add_dependency(MissionId dependent, MissionId prerequisite) {
        std::lock_guard lock(mutex_);
        if (started_) {
            throw std::logic_error("cannot add dependencies after run()");
        }
        if (dependent >= nodes_.size() || prerequisite >= nodes_.size()) {
            throw std::out_of_range("unknown mission id in add_dependency");
        }
        nodes_[prerequisite].dependents.push_back(dependent);
        ++nodes_[dependent].prerequisite_count;
    }

    std::optional<std::vector<MissionCoordinator::MissionId>> MissionCoordinator::topological_order_locked() const {
        std::vector<std::size_t> indegree(nodes_.size());
        for (std::size_t i = 0; i < nodes_.size(); ++i) {
            indegree[i] = nodes_[i].prerequisite_count;
        }
        std::priority_queue<MissionId, std::vector<MissionId>, std::greater<>> ready;
        for (std::size_t i = 0; i < nodes_.size(); ++i) {
            if (indegree[i] == 0) {
                ready.push(i);
            }
        }
        std::vector<MissionId> order;
        order.reserve(nodes_.size());
        while (!ready.empty()) {
            const MissionId id = ready.top();
            ready.pop();
            order.push_back(id);
            for (const MissionId d : nodes_[id].dependents) {
                if (--indegree[d] == 0) {
                    ready.push(d);
                }
            }
        }
        if (order.size() != nodes_.size()) {
            return std::nullopt;  // some node never reached in-degree 0: cycle
        }
        return order;
    }

    std::optional<std::vector<MissionCoordinator::MissionId>> MissionCoordinator::topological_order() const {
        std::lock_guard lock(mutex_);
        return topological_order_locked();
    }

    void MissionCoordinator::run(ThreadPool& pool) {
        std::vector<MissionId> roots;
        {
            std::lock_guard lock(mutex_);
            if (started_) {
                throw std::logic_error("MissionCoordinator::run() may only be called once");
            }
            if (!topological_order_locked()) {
                throw std::logic_error("mission dependency graph contains a cycle");
            }
            started_ = true;
            unfinished_ = nodes_.size();
            for (std::size_t i = 0; i < nodes_.size(); ++i) {
                nodes_[i].remaining = nodes_[i].prerequisite_count;
                if (nodes_[i].remaining == 0) {
                    roots.push_back(i);
                }
            }
        }
        for (const MissionId r : roots) {
            launch(pool, r);
        }
        std::unique_lock lock(mutex_);
        done_cv_.wait(lock, [this] { return unfinished_ == 0; });
    }

    void MissionCoordinator::launch(ThreadPool& pool, MissionId id) {
        try {
            pool.post(UniqueTask([this, &pool, id] {
                const Work* work = nullptr;
                {
                    std::lock_guard lock(mutex_);
                    nodes_[id].status = MissionStatus::Running;
                    work = &nodes_[id].work;  // nodes_ is frozen once started_
                }
                if (cancel_.is_cancelled()) {
                    on_finished(pool, id, MissionStatus::Cancelled, "cancelled before start");
                    return;
                }
                MissionStatus status = MissionStatus::Succeeded;
                std::string error;
                try {
                    (*work)(cancel_.token());
                } catch (const MissionCancelled& e) {
                    status = MissionStatus::Cancelled;
                    error = e.what();
                } catch (const std::exception& e) {
                    status = MissionStatus::Failed;
                    error = e.what();
                } catch (...) {
                    status = MissionStatus::Failed;
                    error = "unknown exception";
                }
                on_finished(pool, id, status, std::move(error));
                // No member may be touched past this point: run() may have returned.
            }));
        } catch (...) {
            on_finished(pool, id, MissionStatus::Cancelled, "executor unavailable");
        }
    }

    void MissionCoordinator::on_finished(ThreadPool& pool, MissionId id, MissionStatus status, std::string error) {
        std::vector<MissionId> ready;
        {
            std::lock_guard lock(mutex_);
            Node& node = nodes_[id];
            node.status = status;
            node.error = std::move(error);
            if (status == MissionStatus::Succeeded || status == MissionStatus::Failed) {
                completion_order_.push_back(id);
            }
            --unfinished_;
            if (status == MissionStatus::Succeeded) {
                for (const MissionId d : node.dependents) {
                    if (--nodes_[d].remaining == 0 && nodes_[d].status == MissionStatus::Pending) {
                        ready.push_back(d);
                    }
                }
            } else {
                cancel_dependents_locked(id);
            }
            if (unfinished_ == 0) {
                done_cv_.notify_all();  // under the lock: run() cannot return before we release it
            }
        }
        for (const MissionId d : ready) {
            launch(pool, d);
        }
    }

    void MissionCoordinator::cancel_dependents_locked(MissionId id) {
        std::vector<MissionId> stack(nodes_[id].dependents.begin(), nodes_[id].dependents.end());
        while (!stack.empty()) {
            const MissionId d = stack.back();
            stack.pop_back();
            Node& node = nodes_[d];
            if (node.status != MissionStatus::Pending) {
                continue;
            }
            node.status = MissionStatus::Cancelled;
            node.error = "prerequisite '" + nodes_[id].name + "' did not succeed";
            --unfinished_;
            stack.insert(stack.end(), node.dependents.begin(), node.dependents.end());
        }
    }

    MissionStatus MissionCoordinator::status(MissionId id) const {
        std::lock_guard lock(mutex_);
        return nodes_.at(id).status;
    }

    const std::string& MissionCoordinator::name(MissionId id) const {
        std::lock_guard lock(mutex_);
        return nodes_.at(id).name;  // names are immutable after insertion
    }

    std::string MissionCoordinator::error(MissionId id) const {
        std::lock_guard lock(mutex_);
        return nodes_.at(id).error;
    }

    std::vector<MissionCoordinator::MissionId> MissionCoordinator::completion_order() const {
        std::lock_guard lock(mutex_);
        return completion_order_;
    }

    std::size_t MissionCoordinator::size() const {
        std::lock_guard lock(mutex_);
        return nodes_.size();
    }

    // ---------------------------------------------------------------- showcase

    void demonstrate_async_missions(std::ostream& out) {
        out << "=== Async missions (futures) ===\n";

        {
            std::promise<double> fuel_reading;
            std::future<double> fuel = fuel_reading.get_future();
            std::thread sensor([&fuel_reading] { fuel_reading.set_value(87.5); });
            out << "std::promise -> std::future: fuel level " << fuel.get() << "%\n";
            sensor.join();

            std::packaged_task<int(int, int)> burn([](int dv, int seconds) { return dv * seconds; });
            std::future<int> impulse = burn.get_future();
            std::thread engine(std::move(burn), 12, 30);
            out << "std::packaged_task on a thread: impulse " << impulse.get() << '\n';
            engine.join();

            bool deferred_ran = false;
            auto deferred = std::async(std::launch::deferred, [&deferred_ran] {
                deferred_ran = true;
                return 1;
            });
            const bool before = deferred_ran;
            static_cast<void>(deferred.get());
            out << "std::launch::deferred runs lazily: before get() " << std::boolalpha << before << ", after "
                << deferred_ran << '\n';
        }

        {
            AsyncMission<int> survey("asteroid-survey", [](const CancellationToken& token) {
                int sum = 0;
                for (int i = 1; i <= 100; ++i) {
                    token.throw_if_cancelled();
                    sum += i;
                }
                return sum;
            });
            const auto result = survey.start().get();
            out << "AsyncMission '" << survey.name() << "' " << to_string(result.status) << " with value "
                << result.value.value_or(-1) << '\n';

            AsyncMission<int> aborted("deep-space-probe", [](const CancellationToken& token) {
                token.throw_if_cancelled();
                return 1;
            });
            aborted.cancel();
            const auto aborted_result = aborted.start().get();
            out << "AsyncMission '" << aborted.name() << "' " << to_string(aborted_result.status) << '\n';
        }

        {
            ThreadPool pool(3);
            MissionCoordinator coordinator;
            std::atomic<int> work_done{0};
            auto step = [&work_done](const CancellationToken&) { work_done.fetch_add(1); };
            const auto fuel = coordinator.add_mission("fuel-up", step);
            const auto crew = coordinator.add_mission("crew-boarding", step);
            const auto launch = coordinator.add_mission("launch", step, {fuel, crew});
            const auto orbit = coordinator.add_mission("orbit-insertion", step, {launch});
            const auto broken = coordinator.add_mission("deploy-antenna", [](const CancellationToken&) {
                throw std::runtime_error("hinge jammed");
            }, {orbit});
            const auto relay = coordinator.add_mission("relay-telemetry", step, {broken});
            const auto science = coordinator.add_mission("science-ops", step, {orbit});
            coordinator.run(pool);
            out << "Mission DAG:";
            for (MissionCoordinator::MissionId id : {fuel, crew, launch, orbit, broken, relay, science}) {
                out << ' ' << coordinator.name(id) << '=' << to_string(coordinator.status(id));
            }
            out << "\n  bodies executed: " << work_done.load() << ", relay-telemetry reason: "
                << coordinator.error(relay) << '\n';

            std::vector<int> radii(16);
            std::iota(radii.begin(), radii.end(), 1);
            const auto areas = parallel_transform(pool, radii, [](int r) { return r * r; });
            out << "parallel_transform squares of 1..16, sum = " << std::accumulate(areas.begin(), areas.end(), 0)
                << '\n';

            std::vector<std::future<int>> probes;
            for (int i = 0; i < 4; ++i) {
                probes.push_back(pool.submit([i] { return 10 * i; }));
            }
            const auto readings = when_all(std::move(probes));
            out << "when_all collected " << readings.size() << " probe readings, last = " << readings.back() << '\n';
        }

        {
            Pipeline<int> pipeline;
            pipeline.add_stage("calibrate", [](int x) { return x + 1; })
                .add_stage("amplify", [](int x) { return x * 10; })
                .add_stage("offset", [](int x) { return x - 3; });
            const auto outputs = pipeline.process({1, 2, 3, 4, 5});
            out << "Pipeline (" << pipeline.stage_count() << " threaded stages) output:";
            for (int v : outputs) {
                out << ' ' << v;
            }
            out << '\n';
        }
        out << '\n';
    }

}  // namespace CppVerseHub::Concurrency
