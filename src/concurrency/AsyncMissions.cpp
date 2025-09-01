/**
 * @file AsyncMissions.cpp
 * @brief Implementation of parallel mission execution and async coordination
 * @details File location: src/concurrency/AsyncMissions.cpp
 */

#include "AsyncMissions.hpp"
#include <iomanip>
#include <sstream>

namespace CppVerseHub::Concurrency {

    // MissionCoordinator Implementation
    void MissionCoordinator::execute_all_missions() {
        std::cout << "MissionCoordinator: Starting mission execution\n";
        
        while (!all_missions_complete()) {
            auto ready_missions = get_ready_missions();
            
            if (ready_missions.empty() && !all_missions_complete()) {
                std::cout << "MissionCoordinator: Waiting for dependencies to complete\n";
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                continue;
            }
            
            // Start all ready missions
            for (MissionId mission_id : ready_missions) {
                std::lock_guard<std::mutex> lock(missions_mutex_);
                auto it = missions_.find(mission_id);
                if (it != missions_.end()) {
                    std::cout << "MissionCoordinator: Starting mission " << mission_id 
                              << " (" << it->second->get_name() << ")\n";
                    auto future = it->second->start();
                    active_futures_.push_back(std::move(future));
                }
            }
            
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
    }

    void MissionCoordinator::wait_for_completion() {
        for (auto& future : active_futures_) {
            future.wait();
        }
        std::cout << "MissionCoordinator: All missions completed\n";
    }

    std::vector<MissionCoordinator::MissionId> MissionCoordinator::get_ready_missions() const {
        std::vector<MissionId> ready_missions;
        
        std::lock_guard<std::mutex> missions_lock(missions_mutex_);
        std::lock_guard<std::mutex> deps_lock(dependencies_mutex_);
        
        for (const auto& [mission_id, mission] : missions_) {
            if (mission->get_status() == MissionStatus::PENDING && can_start_mission(mission_id)) {
                ready_missions.push_back(mission_id);
            }
        }
        
        return ready_missions;
    }

    bool MissionCoordinator::all_missions_complete() const {
        std::lock_guard<std::mutex> lock(missions_mutex_);
        
        for (const auto& [mission_id, mission] : missions_) {
            MissionStatus status = mission->get_status();
            if (status != MissionStatus::COMPLETED && status != MissionStatus::FAILED) {
                return false;
            }
        }
        return true;
    }

    bool MissionCoordinator::can_start_mission(MissionId mission_id) const {
        auto deps_it = dependencies_.find(mission_id);
        if (deps_it == dependencies_.end()) {
            return true; // No dependencies
        }
        
        // Check if all dependencies are completed
        for (MissionId dep_id : deps_it->second) {
            auto mission_it = missions_.find(dep_id);
            if (mission_it == missions_.end()) {
                continue; // Dependency not found, assume it's okay
            }
            
            MissionStatus dep_status = mission_it->second->get_status();
            if (dep_status != MissionStatus::COMPLETED && dep_status != MissionStatus::FAILED) {
                return false;
            }
        }
        
        return true;
    }

    void MissionCoordinator::print_mission_status() const {
        std::lock_guard<std::mutex> lock(missions_mutex_);
        
        std::cout << "\n=== Mission Status Report ===\n";
        for (const auto& [mission_id, mission] : missions_) {
            std::cout << "Mission " << mission_id << " (" << mission->get_name() << "): ";
            
            switch (mission->get_status()) {
                case MissionStatus::PENDING: std::cout << "PENDING"; break;
                case MissionStatus::RUNNING: std::cout << "RUNNING"; break;
                case MissionStatus::COMPLETED: std::cout << "COMPLETED"; break;
                case MissionStatus::FAILED: std::cout << "FAILED"; break;
                case MissionStatus::CANCELLED: std::cout << "CANCELLED"; break;
            }
            std::cout << "\n";
        }
        std::cout << "==============================\n";
    }

    // AsyncMissionDemo Implementation
    void AsyncMissionDemo::demonstrate_basic_async_missions() {
        std::cout << "\n=== Basic Async Missions Demonstration ===\n";
        
        // Create different types of missions
        auto exploration_mission = std::make_unique<AsyncMission<SpaceExplorationData>>(
            "Mars_Exploration",
            []() { return simulate_planet_exploration("Mars"); }
        );
        
        auto satellite_mission = std::make_unique<AsyncMission<SatelliteData>>(
            "Hubble_Observation",
            []() { return simulate_satellite_mission(1); }
        );
        
        auto calculation_mission = std::make_unique<AsyncMission<double>>(
            "Orbital_Calculation",
            []() {
                simulate_mission_work(std::chrono::milliseconds(200));
                return 42.7; // Mock orbital calculation result
            }
        );
        
        // Start missions and get futures
        auto exploration_future = exploration_mission->start();
        auto satellite_future = satellite_mission->start();
        auto calculation_future = calculation_mission->start();
        
        // Wait for results
        auto exploration_result = exploration_future.get();
        auto satellite_result = satellite_future.get();
        auto calculation_result = calculation_future.get();
        
        // Display results
        std::cout << "Results summary:\n";
        std::cout << "- Exploration mission: " 
                  << (exploration_result.status == MissionStatus::COMPLETED ? "SUCCESS" : "FAILED") << "\n";
        std::cout << "- Satellite mission: " 
                  << (satellite_result.status == MissionStatus::COMPLETED ? "SUCCESS" : "FAILED") << "\n";
        std::cout << "- Calculation mission: " 
                  << (calculation_result.status == MissionStatus::COMPLETED ? "SUCCESS" : "FAILED") << "\n";
        
        if (exploration_result.status == MissionStatus::COMPLETED) {
            const auto& data = exploration_result.data;
            std::cout << "Explored planet: " << data.planet_name 
                      << ", Found water: " << (data.has_water ? "Yes" : "No") << "\n";
        }
    }

    void AsyncMissionDemo::demonstrate_mission_coordinator() {
        std::cout << "\n=== Mission Coordinator Demonstration ===\n";
        
        MissionCoordinator coordinator;
        
        // Create interdependent missions
        auto prep_mission = std::make_unique<AsyncMission<std::string>>(
            "Mission_Preparation",
            []() {
                simulate_mission_work(std::chrono::milliseconds(150));
                return std::string("Equipment prepared and systems checked");
            }
        );
        
        auto launch_mission = std::make_unique<AsyncMission<std::string>>(
            "Rocket_Launch",
            []() {
                simulate_mission_work(std::chrono::milliseconds(300));
                return std::string("Rocket launched successfully");
            }
        );
        
        auto orbit_mission = std::make_unique<AsyncMission<std::string>>(
            "Orbital_Insertion",
            []() {
                simulate_mission_work(std::chrono::milliseconds(200));
                return std::string("Successfully entered orbit");
            }
        );
        
        auto science_mission = std::make_unique<AsyncMission<SpaceExplorationData>>(
            "Science_Operations",
            []() { return simulate_planet_exploration("Jupiter"); }
        );
        
        // Add missions to coordinator
        auto prep_id = coordinator.add_mission(std::move(prep_mission));
        auto launch_id = coordinator.add_mission(std::move(launch_mission));
        auto orbit_id = coordinator.add_mission(std::move(orbit_mission));
        auto science_id = coordinator.add_mission(std::move(science_mission));
        
        // Define dependencies
        coordinator.add_dependency(launch_id, prep_id);      // Launch depends on preparation
        coordinator.add_dependency(orbit_id, launch_id);     // Orbit depends on launch
        coordinator.add_dependency(science_id, orbit_id);    // Science depends on orbit
        
        // Execute all missions
        coordinator.execute_all_missions();
        coordinator.wait_for_completion();
        coordinator.print_mission_status();
    }

    void AsyncMissionDemo::demonstrate_parallel_executor() {
        std::cout << "\n=== Parallel Mission Executor Demonstration ===\n";
        
        ParallelMissionExecutor<SatelliteData> executor(3); // Max 3 concurrent missions
        
        // Create batch of satellite missions
        std::vector<std::function<SatelliteData(size_t)>> satellite_missions;
        
        for (int i = 0; i < 8; ++i) {
            satellite_missions.push_back([](size_t mission_index) {
                return simulate_satellite_mission(static_cast<int>(mission_index + 100));
            });
        }
        
        // Execute batch
        auto futures = executor.execute_batch(satellite_missions, "SatelliteSwarm");
        
        // Wait and collect results
        executor.wait_for_all(futures);
        auto results = executor.collect_results(futures);
        
        // Analyze results
        size_t successful = std::count_if(results.begin(), results.end(),
            [](const MissionResult<SatelliteData>& result) {
                return result.status == MissionStatus::COMPLETED;
            });
        
        auto total_time = std::accumulate(results.begin(), results.end(), std::chrono::milliseconds(0),
            [](std::chrono::milliseconds sum, const MissionResult<SatelliteData>& result) {
                return sum + result.execution_time;
            });
        
        std::cout << "Batch execution summary:\n";
        std::cout << "- Successful missions: " << successful << "/" << results.size() << "\n";
        std::cout << "- Total execution time: " << total_time.count() << "ms\n";
        std::cout << "- Average mission time: " << (total_time.count() / results.size()) << "ms\n";
    }

    void AsyncMissionDemo::demonstrate_pipeline_processor() {
        std::cout << "\n=== Pipeline Mission Processor Demonstration ===\n";
        
        PipelineMissionProcessor<std::string, std::string> pipeline;
        
        // Add processing stages
        pipeline.add_stage("Data_Acquisition", [](const std::string& input, size_t stage_id) {
            simulate_mission_work(std::chrono::milliseconds(50));
            return "Acquired_" + input;
        });
        
        pipeline.add_stage("Data_Processing", [](const std::string& input, size_t stage_id) {
            simulate_mission_work(std::chrono::milliseconds(100));
            return "Processed_" + input;
        });
        
        pipeline.add_stage("Data_Analysis", [](const std::string& input, size_t stage_id) {
            simulate_mission_work(std::chrono::milliseconds(75));
            return "Analyzed_" + input;
        });
        
        // Process batch of data
        std::vector<std::string> input_data = {
            "SensorData_1", "SensorData_2", "SensorData_3", "SensorData_4"
        };
        
        auto results_future = pipeline.process_batch(input_data);
        auto results = results_future.get();
        
        std::cout << "Pipeline processing results:\n";
        for (size_t i = 0; i < results.size(); ++i) {
            std::cout << "Item " << i << ": " << results[i] << "\n";
        }
    }

    void AsyncMissionDemo::demonstrate_space_exploration_scenario() {
        std::cout << "\n=== Comprehensive Space Exploration Scenario ===\n";
        
        // Combine multiple async patterns for a complex scenario
        MissionCoordinator mission_control;
        ParallelMissionExecutor<SpaceExplorationData> explorer(4);
        
        // Phase 1: Preparation missions
        auto systems_check = std::make_unique<AsyncMission<std::string>>(
            "Systems_Check",
            []() {
                simulate_mission_work(std::chrono::milliseconds(100));
                return std::string("All systems nominal");
            }
        );
        
        auto fuel_loading = std::make_unique<AsyncMission<std::string>>(
            "Fuel_Loading",
            []() {
                simulate_mission_work(std::chrono::milliseconds(150));
                return std::string("Fuel tanks at 100%");
            }
        );
        
        // Add preparation missions
        auto systems_id = mission_control.add_mission(std::move(systems_check));
        auto fuel_id = mission_control.add_mission(std::move(fuel_loading));
        
        // Phase 2: Launch mission (depends on preparation)
        auto launch_mission = std::make_unique<AsyncMission<std::string>>(
            "Multi_Planet_Launch",
            []() {
                simulate_mission_work(std::chrono::milliseconds(200));
                return std::string("Multi-probe mission launched");
            }
        );
        
        auto launch_id = mission_control.add_mission(std::move(launch_mission));
        mission_control.add_dependency(launch_id, systems_id);
        mission_control.add_dependency(launch_id, fuel_id);
        
        // Start preparation and launch sequence
        std::thread mission_thread([&mission_control]() {
            mission_control.execute_all_missions();
            mission_control.wait_for_completion();
        });
        
        // Phase 3: Parallel planetary exploration (after launch)
        std::this_thread::sleep_for(std::chrono::milliseconds(500)); // Wait for launch
        
        std::vector<std::function<SpaceExplorationData(size_t)>> exploration_missions;
        std::vector<std::string> target_planets = {
            "Venus", "Mars", "Jupiter", "Saturn", "Uranus", "Neptune"
        };
        
        for (const auto& planet : target_planets) {
            exploration_missions.push_back([planet](size_t index) {
                return simulate_planet_exploration(planet);
            });
        }
        
        std::cout << "Starting parallel planetary exploration...\n";
        auto exploration_futures = explorer.execute_batch(exploration_missions, "PlanetaryExploration");
        
        // Wait for all missions to complete
        mission_thread.join();
        explorer.wait_for_all(exploration_futures);
        auto exploration_results = explorer.collect_results(exploration_futures);
        
        // Generate comprehensive mission report
        std::vector<SpaceExplorationData> successful_explorations;
        for (const auto& result : exploration_results) {
            if (result.status == MissionStatus::COMPLETED) {
                successful_explorations.push_back(result.data);
            }
        }
        
        std::string final_report = generate_mission_report(successful_explorations);
        std::cout << "\n" << final_report << "\n";
        
        mission_control.print_mission_status();
    }

    void AsyncMissionDemo::run_all_demonstrations() {
        std::cout << "\n========== ASYNC MISSIONS COMPREHENSIVE DEMONSTRATION ==========\n";
        
        demonstrate_basic_async_missions();
        demonstrate_mission_coordinator();
        demonstrate_parallel_executor();
        demonstrate_pipeline_processor();
        demonstrate_space_exploration_scenario();
        
        std::cout << "\n========== ASYNC MISSIONS DEMONSTRATION COMPLETE ==========\n";
    }

    // Helper function implementations
    AsyncMissionDemo::SpaceExplorationData 
    AsyncMissionDemo::simulate_planet_exploration(const std::string& planet_name) {
        // Simulate variable exploration time
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> time_dist(200, 800);
        
        simulate_mission_work(std::chrono::milliseconds(time_dist(gen)));
        
        SpaceExplorationData data;
        data.planet_name = planet_name;
        
        // Generate mock exploration data based on planet name
        std::uniform_real_distribution<> distance_dist(0.5, 50.0);
        std::uniform_real_distribution<> pressure_dist(0.0, 2.0);
        std::bernoulli_distribution water_dist(0.3); // 30% chance of water
        
        data.distance_from_earth = distance_dist(gen);
        data.atmospheric_pressure = pressure_dist(gen);
        data.has_water = water_dist(gen);
        
        // Generate discovered elements
        std::vector<std::string> possible_elements = {
            "Hydrogen", "Helium", "Oxygen", "Carbon", "Silicon", "Iron", "Magnesium", "Sulfur"
        };
        
        std::uniform_int_distribution<> element_count_dist(1, 4);
        int element_count = element_count_dist(gen);
        
        std::shuffle(possible_elements.begin(), possible_elements.end(), gen);
        for (int i = 0; i < element_count && i < static_cast<int>(possible_elements.size()); ++i) {
            data.discovered_elements.push_back(possible_elements[i]);
        }
        
        return data;
    }

    AsyncMissionDemo::SatelliteData 
    AsyncMissionDemo::simulate_satellite_mission(int satellite_id) {
        std::random_device rd;
        std::mt19937 gen(rd());
        
        // Simulate mission time
        std::uniform_int_distribution<> time_dist(100, 400);
        simulate_mission_work(std::chrono::milliseconds(time_dist(gen)));
        
        SatelliteData data;
        data.satellite_id = satellite_id;
        
        std::vector<std::string> mission_types = {
            "Earth_Observation", "Communication_Relay", "Weather_Monitoring", 
            "GPS_Navigation", "Scientific_Research"
        };
        
        std::uniform_int_distribution<> mission_dist(0, static_cast<int>(mission_types.size() - 1));
        data.mission_type = mission_types[mission_dist(gen)];
        
        std::uniform_real_distribution<> battery_dist(20.0, 100.0);
        data.battery_level = battery_dist(gen);
        
        // Generate sensor readings
        std::uniform_int_distribution<> reading_count_dist(3, 8);
        int reading_count = reading_count_dist(gen);
        
        std::uniform_real_distribution<> reading_dist(-50.0, 50.0);
        for (int i = 0; i < reading_count; ++i) {
            data.sensor_readings.push_back(reading_dist(gen));
        }
        
        return data;
    }

    void AsyncMissionDemo::simulate_mission_work(std::chrono::milliseconds duration) {
        auto start = std::chrono::high_resolution_clock::now();
        while (std::chrono::high_resolution_clock::now() - start < duration) {
            // Simulate CPU-intensive work
            volatile int dummy = 0;
            for (int i = 0; i < 1000; ++i) {
                dummy += i;
            }
            std::this_thread::yield();
        }
    }

    std::string AsyncMissionDemo::generate_mission_report(const std::vector<SpaceExplorationData>& data) {
        std::stringstream report;
        
        report << "=== SPACE EXPLORATION MISSION REPORT ===\n";
        report << "Total planets explored: " << data.size() << "\n";
        
        if (data.empty()) {
            report << "No successful explorations to report.\n";
            return report.str();
        }
        
        // Summary statistics
        size_t planets_with_water = std::count_if(data.begin(), data.end(),
            [](const SpaceExplorationData& planet) { return planet.has_water; });
        
        double avg_distance = std::accumulate(data.begin(), data.end(), 0.0,
            [](double sum, const SpaceExplorationData& planet) {
                return sum + planet.distance_from_earth;
            }) / data.size();
        
        size_t total_elements = std::accumulate(data.begin(), data.end(), 0ULL,
            [](size_t sum, const SpaceExplorationData& planet) {
                return sum + planet.discovered_elements.size();
            });
        
        report << "Planets with water: " << planets_with_water << "/" << data.size() << "\n";
        report << "Average distance from Earth: " << std::fixed << std::setprecision(2) 
               << avg_distance << " AU\n";
        report << "Total unique elements discovered: " << total_elements << "\n";
        
        report << "\nDetailed exploration results:\n";
        for (const auto& planet : data) {
            report << "- " << planet.planet_name << ": ";
            report << (planet.has_water ? "Water detected" : "No water");
            report << ", Pressure: " << std::fixed << std::setprecision(2) 
                   << planet.atmospheric_pressure << " atm";
            report << ", Elements: ";
            for (size_t i = 0; i < planet.discovered_elements.size(); ++i) {
                report << planet.discovered_elements[i];
                if (i < planet.discovered_elements.size() - 1) report << ", ";
            }
            report << "\n";
        }
        
        report << "==========================================";
        return report.str();
    }

} // namespace CppVerseHub::Concurrency