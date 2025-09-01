/**
 * @file AsyncMissions.hpp
 * @brief Parallel mission execution and async task coordination patterns
 * @details File location: src/concurrency/AsyncMissions.hpp
 * 
 * This file demonstrates advanced async patterns for coordinating parallel
 * mission execution, task dependencies, and distributed computation scenarios.
 */

#ifndef ASYNCMISSIONS_HPP
#define ASYNCMISSIONS_HPP

#include <future>
#include <thread>
#include <vector>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <functional>
#include <memory>
#include <string>
#include <chrono>
#include <random>
#include <unordered_map>
#include <unordered_set>
#include <iostream>
#include <algorithm>
#include <numeric>

namespace CppVerseHub::Concurrency {

    /**
     * @enum MissionStatus
     * @brief Status states for mission execution
     */
    enum class MissionStatus {
        PENDING,
        RUNNING,
        COMPLETED,
        FAILED,
        CANCELLED
    };

    /**
     * @struct MissionResult
     * @brief Result of a completed mission
     */
    template<typename T>
    struct MissionResult {
        T data;
        MissionStatus status;
        std::string error_message;
        std::chrono::milliseconds execution_time;
        
        MissionResult() : status(MissionStatus::PENDING), execution_time(0) {}
        MissionResult(T result_data) : data(std::move(result_data)), status(MissionStatus::COMPLETED), execution_time(0) {}
        MissionResult(MissionStatus stat, const std::string& error) 
            : status(stat), error_message(error), execution_time(0) {}
    };

    /**
     * @class AsyncMission
     * @brief Base class for asynchronous mission execution
     */
    template<typename ResultType>
    class AsyncMission {
    public:
        using MissionFunction = std::function<ResultType()>;
        
        AsyncMission(const std::string& name, MissionFunction mission_func)
            : name_(name), mission_func_(std::move(mission_func)), status_(MissionStatus::PENDING) {}
        
        virtual ~AsyncMission() = default;

        // Start the mission asynchronously
        std::future<MissionResult<ResultType>> start() {
            if (status_.load() != MissionStatus::PENDING) {
                throw std::runtime_error("Mission already started or completed");
            }
            
            return std::async(std::launch::async, [this]() {
                return execute_mission();
            });
        }

        // Get current status
        MissionStatus get_status() const { return status_.load(); }
        const std::string& get_name() const { return name_; }
        
        // Cancel the mission (if possible)
        void cancel() { 
            auto expected = MissionStatus::PENDING;
            status_.compare_exchange_strong(expected, MissionStatus::CANCELLED);
        }

    protected:
        virtual MissionResult<ResultType> execute_mission() {
            status_.store(MissionStatus::RUNNING);
            auto start_time = std::chrono::high_resolution_clock::now();
            
            try {
                std::cout << "Mission '" << name_ << "' starting execution\n";
                ResultType result = mission_func_();
                
                auto end_time = std::chrono::high_resolution_clock::now();
                auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
                
                status_.store(MissionStatus::COMPLETED);
                std::cout << "Mission '" << name_ << "' completed in " << duration.count() << "ms\n";
                
                MissionResult<ResultType> mission_result(result);
                mission_result.execution_time = duration;
                return mission_result;
                
            } catch (const std::exception& e) {
                auto end_time = std::chrono::high_resolution_clock::now();
                auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
                
                status_.store(MissionStatus::FAILED);
                std::cout << "Mission '" << name_ << "' failed: " << e.what() << "\n";
                
                MissionResult<ResultType> result(MissionStatus::FAILED, e.what());
                result.execution_time = duration;
                return result;
            }
        }

    private:
        std::string name_;
        MissionFunction mission_func_;
        std::atomic<MissionStatus> status_;
    };

    /**
     * @class MissionCoordinator
     * @brief Coordinates execution of multiple related missions
     */
    class MissionCoordinator {
    public:
        using MissionId = size_t;
        
        template<typename T>
        MissionId add_mission(std::unique_ptr<AsyncMission<T>> mission) {
            std::lock_guard<std::mutex> lock(missions_mutex_);
            MissionId id = next_mission_id_++;
            
            auto wrapper = std::make_unique<MissionWrapper<T>>(std::move(mission));
            missions_[id] = std::move(wrapper);
            
            std::cout << "MissionCoordinator: Added mission " << id << "\n";
            return id;
        }
        
        void add_dependency(MissionId dependent, MissionId prerequisite) {
            std::lock_guard<std::mutex> lock(dependencies_mutex_);
            dependencies_[dependent].insert(prerequisite);
            std::cout << "MissionCoordinator: Mission " << dependent 
                      << " depends on mission " << prerequisite << "\n";
        }
        
        void execute_all_missions();
        void wait_for_completion();
        
        std::vector<MissionId> get_ready_missions() const;
        bool all_missions_complete() const;
        void print_mission_status() const;
        
    private:
        struct MissionWrapperBase {
            virtual ~MissionWrapperBase() = default;
            virtual std::future<void> start() = 0;
            virtual bool is_complete() const = 0;
            virtual MissionStatus get_status() const = 0;
            virtual std::string get_name() const = 0;
        };
        
        template<typename T>
        struct MissionWrapper : MissionWrapperBase {
            std::unique_ptr<AsyncMission<T>> mission;
            std::future<MissionResult<T>> future;
            bool started = false;
            
            explicit MissionWrapper(std::unique_ptr<AsyncMission<T>> m) : mission(std::move(m)) {}
            
            std::future<void> start() override {
                if (!started) {
                    future = mission->start();
                    started = true;
                }
                return std::async(std::launch::async, [this]() {
                    future.wait();
                });
            }
            
            bool is_complete() const override {
                return started && future.valid() && 
                       future.wait_for(std::chrono::seconds(0)) == std::future_status::ready;
            }
            
            MissionStatus get_status() const override {
                return mission->get_status();
            }
            
            std::string get_name() const override {
                return mission->get_name();
            }
        };
        
        std::unordered_map<MissionId, std::unique_ptr<MissionWrapperBase>> missions_;
        std::unordered_map<MissionId, std::unordered_set<MissionId>> dependencies_;
        std::unordered_set<MissionId> completed_missions_;
        std::vector<std::future<void>> active_futures_;
        
        mutable std::mutex missions_mutex_;
        mutable std::mutex dependencies_mutex_;
        std::atomic<MissionId> next_mission_id_{1};
        
        bool can_start_mission(MissionId mission_id) const;
    };

    /**
     * @class ParallelMissionExecutor
     * @brief Execute missions in parallel with configurable concurrency
     */
    template<typename ResultType>
    class ParallelMissionExecutor {
    public:
        using MissionFunc = std::function<ResultType(size_t)>;
        
        explicit ParallelMissionExecutor(size_t max_concurrent = std::thread::hardware_concurrency())
            : max_concurrent_(max_concurrent) {}

        std::vector<std::future<MissionResult<ResultType>>> 
        execute_batch(const std::vector<MissionFunc>& missions, const std::string& batch_name = "BatchMission") {
            std::cout << "ParallelExecutor: Starting batch '" << batch_name 
                      << "' with " << missions.size() << " missions\n";
            
            std::vector<std::future<MissionResult<ResultType>>> futures;
            std::atomic<size_t> completed_count{0};
            
            // Execute missions with concurrency limit
            for (size_t i = 0; i < missions.size(); ++i) {
                // Wait if we've hit the concurrency limit
                if (futures.size() >= max_concurrent_) {
                    // Wait for at least one to complete
                    for (auto& future : futures) {
                        if (future.wait_for(std::chrono::milliseconds(10)) == std::future_status::ready) {
                            break;
                        }
                    }
                }
                
                futures.push_back(std::async(std::launch::async, 
                    [mission = missions[i], i, &completed_count, batch_name]() {
                        std::string mission_name = batch_name + "_" + std::to_string(i);
                        auto start_time = std::chrono::high_resolution_clock::now();
                        
                        try {
                            std::cout << "Executing " << mission_name << "\n";
                            ResultType result = mission(i);
                            
                            auto end_time = std::chrono::high_resolution_clock::now();
                            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
                            
                            completed_count.fetch_add(1);
                            std::cout << mission_name << " completed (" 
                                      << completed_count.load() << " total)\n";
                            
                            MissionResult<ResultType> mission_result(result);
                            mission_result.execution_time = duration;
                            return mission_result;
                            
                        } catch (const std::exception& e) {
                            auto end_time = std::chrono::high_resolution_clock::now();
                            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
                            
                            std::cout << mission_name << " failed: " << e.what() << "\n";
                            
                            MissionResult<ResultType> result(MissionStatus::FAILED, e.what());
                            result.execution_time = duration;
                            return result;
                        }
                    }));
            }
            
            return futures;
        }

        void wait_for_all(std::vector<std::future<MissionResult<ResultType>>>& futures) {
            for (auto& future : futures) {
                future.wait();
            }
        }

        std::vector<MissionResult<ResultType>> 
        collect_results(std::vector<std::future<MissionResult<ResultType>>>& futures) {
            std::vector<MissionResult<ResultType>> results;
            results.reserve(futures.size());
            
            for (auto& future : futures) {
                results.push_back(future.get());
            }
            
            return results;
        }

    private:
        size_t max_concurrent_;
    };

    /**
     * @class PipelineMissionProcessor
     * @brief Process missions through a pipeline of stages
     */
    template<typename InputType, typename OutputType>
    class PipelineMissionProcessor {
    public:
        using StageProcessor = std::function<OutputType(const InputType&, size_t stage_id)>;
        
        void add_stage(const std::string& stage_name, StageProcessor processor) {
            stages_.emplace_back(stage_name, std::move(processor));
            std::cout << "Pipeline: Added stage '" << stage_name << "'\n";
        }

        std::future<std::vector<OutputType>> process_batch(std::vector<InputType> inputs) {
            return std::async(std::launch::async, [this, inputs = std::move(inputs)]() mutable {
                std::vector<OutputType> results;
                results.reserve(inputs.size());
                
                for (size_t i = 0; i < inputs.size(); ++i) {
                    auto result = process_single_item(inputs[i], i);
                    results.push_back(result);
                }
                
                return results;
            });
        }

        OutputType process_single_item(InputType input, size_t item_id) {
            std::cout << "Pipeline: Processing item " << item_id 
                      << " through " << stages_.size() << " stages\n";
            
            // For simplicity, we'll process sequentially through stages
            // In a real pipeline, stages could run in parallel
            auto current_input = std::move(input);
            
            for (size_t stage_id = 0; stage_id < stages_.size(); ++stage_id) {
                const auto& [stage_name, processor] = stages_[stage_id];
                std::cout << "  Stage " << stage_id << " (" << stage_name 
                          << ") processing item " << item_id << "\n";
                
                if (stage_id == stages_.size() - 1) {
                    // Last stage produces final output
                    return processor(current_input, stage_id);
                } else {
                    // Intermediate stages (would need different typing in real implementation)
                    // This is simplified for demonstration
                    auto intermediate_result = processor(current_input, stage_id);
                    // In reality, we'd need to handle type transformations between stages
                }
            }
            
            // This shouldn't be reached, but needed for compilation
            throw std::runtime_error("Pipeline processing error");
        }

    private:
        std::vector<std::pair<std::string, StageProcessor>> stages_;
    };

    /**
     * @class AsyncMissionDemo
     * @brief Comprehensive demonstration of async mission patterns
     */
    class AsyncMissionDemo {
    public:
        static void demonstrate_basic_async_missions();
        static void demonstrate_mission_coordinator();
        static void demonstrate_parallel_executor();
        static void demonstrate_pipeline_processor();
        static void demonstrate_space_exploration_scenario();
        static void run_all_demonstrations();

    private:
        struct SpaceExplorationData {
            std::string planet_name;
            double distance_from_earth;
            std::vector<std::string> discovered_elements;
            bool has_water;
            double atmospheric_pressure;
        };

        struct SatelliteData {
            int satellite_id;
            std::string mission_type;
            double battery_level;
            std::vector<double> sensor_readings;
        };

        static SpaceExplorationData simulate_planet_exploration(const std::string& planet_name);
        static SatelliteData simulate_satellite_mission(int satellite_id);
        static void simulate_mission_work(std::chrono::milliseconds duration);
        static std::string generate_mission_report(const std::vector<SpaceExplorationData>& data);
    };

} // namespace CppVerseHub::Concurrency

#endif // ASYNCMISSIONS_HPP