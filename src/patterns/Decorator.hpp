// File: src/patterns/Decorator.hpp
// CppVerseHub - Decorator Pattern Implementation for Mission Enhancement System

#pragma once

#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <chrono>
#include <functional>
#include <any>

namespace CppVerseHub::Patterns {

/**
 * @brief Mission result structure
 */
struct MissionResult {
    bool success;
    double completion_time;
    double resource_cost;
    double experience_gained;
    std::string description;
    std::unordered_map<std::string, std::any> additional_data;
    
    MissionResult(bool success = false, double time = 0.0, double cost = 0.0, double exp = 0.0)
        : success(success), completion_time(time), resource_cost(cost), experience_gained(exp) {}
    
    /**
     * @brief Set additional data
     */
    template<typename T>
    void setData(const std::string& key, const T& value) {
        additional_data[key] = value;
    }
    
    /**
     * @brief Get additional data
     */
    template<typename T>
    std::optional<T> getData(const std::string& key) const {
        auto it = additional_data.find(key);
        if (it != additional_data.end()) {
            try {
                return std::any_cast<T>(it->second);
            } catch (const std::bad_any_cast&) {
                return std::nullopt;
            }
        }
        return std::nullopt;
    }
    
    /**
     * @brief Calculate efficiency score (success rate / (time + cost))
     */
    double getEfficiencyScore() const {
        double denominator = completion_time + resource_cost;
        return denominator > 0.0 ? (success ? 1.0 : 0.0) / denominator : 0.0;
    }
};

/**
 * @brief Mission parameters structure
 */
struct MissionParameters {
    std::string target_location;
    double difficulty_level;
    double available_resources;
    std::vector<std::string> required_skills;
    std::unordered_map<std::string, std::any> custom_params;
    
    MissionParameters(const std::string& target = "", double difficulty = 1.0, double resources = 100.0)
        : target_location(target), difficulty_level(difficulty), available_resources(resources) {}
    
    template<typename T>
    void setParam(const std::string& key, const T& value) {
        custom_params[key] = value;
    }
    
    template<typename T>
    std::optional<T> getParam(const std::string& key) const {
        auto it = custom_params.find(key);
        if (it != custom_params.end()) {
            try {
                return std::any_cast<T>(it->second);
            } catch (const std::bad_any_cast&) {
                return std::nullopt;
            }
        }
        return std::nullopt;
    }
};

/**
 * @brief Base Mission Component interface
 * 
 * Defines the core interface for missions that can be decorated
 * with additional functionality and enhancements.
 */
class IMissionComponent {
public:
    virtual ~IMissionComponent() = default;
    
    /**
     * @brief Execute the mission with given parameters
     * @param params Mission parameters
     * @return Mission result
     */
    virtual MissionResult execute(const MissionParameters& params) = 0;
    
    /**
     * @brief Get mission name/description
     * @return Mission name
     */
    virtual std::string getName() const = 0;
    
    /**
     * @brief Get estimated mission duration
     * @param params Mission parameters
     * @return Estimated duration in time units
     */
    virtual double getEstimatedDuration(const MissionParameters& params) const = 0;
    
    /**
     * @brief Get estimated resource cost
     * @param params Mission parameters
     * @return Estimated resource cost
     */
    virtual double getEstimatedCost(const MissionParameters& params) const = 0;
    
    /**
     * @brief Get required skills for this mission
     * @return Vector of required skill names
     */
    virtual std::vector<std::string> getRequiredSkills() const = 0;
    
    /**
     * @brief Get success probability for given parameters
     * @param params Mission parameters
     * @return Success probability (0.0 - 1.0)
     */
    virtual double getSuccessProbability(const MissionParameters& params) const = 0;
    
    /**
     * @brief Clone this mission component (for composition purposes)
     * @return Unique pointer to cloned component
     */
    virtual std::unique_ptr<IMissionComponent> clone() const = 0;
};

/**
 * @brief Basic Exploration Mission
 * 
 * Concrete implementation of a basic exploration mission
 * that can be enhanced with decorators.
 */
class BasicExplorationMission : public IMissionComponent {
public:
    explicit BasicExplorationMission(const std::string& mission_id = "basic_exploration")
        : mission_id_(mission_id) {}
    
    MissionResult execute(const MissionParameters& params) override {
        auto start_time = std::chrono::steady_clock::now();
        
        // Simulate mission execution
        double success_prob = getSuccessProbability(params);
        bool success = (static_cast<double>(rand()) / RAND_MAX) < success_prob;
        
        double duration = getEstimatedDuration(params);
        double cost = getEstimatedCost(params);
        double experience = success ? duration * 0.5 : duration * 0.2;
        
        auto end_time = std::chrono::steady_clock::now();
        auto actual_duration = std::chrono::duration<double>(end_time - start_time).count();
        
        MissionResult result(success, duration, cost, experience);
        result.description = "Basic exploration mission to " + params.target_location;
        result.setData("actual_execution_time", actual_duration);
        result.setData("exploration_data", success ? "Valuable data collected" : "Limited data obtained");
        
        return result;
    }
    
    std::string getName() const override {
        return "Basic Exploration Mission (" + mission_id_ + ")";
    }
    
    double getEstimatedDuration(const MissionParameters& params) const override {
        return 10.0 + params.difficulty_level * 5.0; // Base 10 units + difficulty scaling
    }
    
    double getEstimatedCost(const MissionParameters& params) const override {
        return 50.0 + params.difficulty_level * 20.0; // Base 50 units + difficulty scaling
    }
    
    std::vector<std::string> getRequiredSkills() const override {
        return {"Navigation", "Sensor_Operation", "Data_Analysis"};
    }
    
    double getSuccessProbability(const MissionParameters& params) const override {
        double base_prob = 0.7;
        double difficulty_penalty = params.difficulty_level * 0.1;
        double resource_bonus = params.available_resources > getEstimatedCost(params) ? 0.1 : -0.2;
        
        return std::max(0.1, std::min(0.95, base_prob - difficulty_penalty + resource_bonus));
    }
    
    std::unique_ptr<IMissionComponent> clone() const override {
        return std::make_unique<BasicExplorationMission>(mission_id_);
    }

private:
    std::string mission_id_;
};

/**
 * @brief Basic Combat Mission
 */
class BasicCombatMission : public IMissionComponent {
public:
    explicit BasicCombatMission(const std::string& mission_id = "basic_combat")
        : mission_id_(mission_id) {}
    
    MissionResult execute(const MissionParameters& params) override {
        auto start_time = std::chrono::steady_clock::now();
        
        double success_prob = getSuccessProbability(params);
        bool success = (static_cast<double>(rand()) / RAND_MAX) < success_prob;
        
        double duration = getEstimatedDuration(params);
        double cost = getEstimatedCost(params);
        double experience = success ? duration * 0.8 : duration * 0.3;
        
        MissionResult result(success, duration, cost, experience);
        result.description = "Basic combat mission at " + params.target_location;
        result.setData("casualties", success ? rand() % 3 : rand() % 8 + 2);
        result.setData("enemy_defeated", success);
        
        return result;
    }
    
    std::string getName() const override {
        return "Basic Combat Mission (" + mission_id_ + ")";
    }
    
    double getEstimatedDuration(const MissionParameters& params) const override {
        return 8.0 + params.difficulty_level * 4.0;
    }
    
    double getEstimatedCost(const MissionParameters& params) const override {
        return 80.0 + params.difficulty_level * 30.0;
    }
    
    std::vector<std::string> getRequiredSkills() const override {
        return {"Combat_Tactics", "Weapons_Operation", "Leadership"};
    }
    
    double getSuccessProbability(const MissionParameters& params) const override {
        double base_prob = 0.6;
        double difficulty_penalty = params.difficulty_level * 0.15;
        double resource_bonus = params.available_resources > getEstimatedCost(params) ? 0.15 : -0.25;
        
        return std::max(0.05, std::min(0.9, base_prob - difficulty_penalty + resource_bonus));
    }
    
    std::unique_ptr<IMissionComponent> clone() const override {
        return std::make_unique<BasicCombatMission>(mission_id_);
    }

private:
    std::string mission_id_;
};

/**
 * @brief Basic Colonization Mission
 */
class BasicColonizationMission : public IMissionComponent {
public:
    explicit BasicColonizationMission(const std::string& mission_id = "basic_colonization")
        : mission_id_(mission_id) {}
    
    MissionResult execute(const MissionParameters& params) override {
        double success_prob = getSuccessProbability(params);
        bool success = (static_cast<double>(rand()) / RAND_MAX) < success_prob;
        
        double duration = getEstimatedDuration(params);
        double cost = getEstimatedCost(params);
        double experience = success ? duration * 0.6 : duration * 0.1;
        
        MissionResult result(success, duration, cost, experience);
        result.description = "Basic colonization mission to " + params.target_location;
        result.setData("colony_size", success ? rand() % 1000 + 500 : rand() % 200);
        result.setData("infrastructure_level", success ? rand() % 5 + 3 : rand() % 3);
        
        return result;
    }
    
    std::string getName() const override {
        return "Basic Colonization Mission (" + mission_id_ + ")";
    }
    
    double getEstimatedDuration(const MissionParameters& params) const override {
        return 20.0 + params.difficulty_level * 10.0;
    }
    
    double getEstimatedCost(const MissionParameters& params) const override {
        return 200.0 + params.difficulty_level * 50.0;
    }
    
    std::vector<std::string> getRequiredSkills() const override {
        return {"Engineering", "Resource_Management", "Colony_Planning", "Leadership"};
    }
    
    double getSuccessProbability(const MissionParameters& params) const override {
        double base_prob = 0.5;
        double difficulty_penalty = params.difficulty_level * 0.12;
        double resource_bonus = params.available_resources > getEstimatedCost(params) ? 0.2 : -0.3;
        
        return std::max(0.05, std::min(0.85, base_prob - difficulty_penalty + resource_bonus));
    }
    
    std::unique_ptr<IMissionComponent> clone() const override {
        return std::make_unique<BasicColonizationMission>(mission_id_);
    }

private:
    std::string mission_id_;
};

/**
 * @brief Base Mission Decorator
 * 
 * Abstract base class for all mission decorators.
 * Provides common functionality and interface for decorating missions.
 */
class MissionDecorator : public IMissionComponent {
public:
    explicit MissionDecorator(std::unique_ptr<IMissionComponent> mission)
        : wrapped_mission_(std::move(mission)) {}
    
    virtual ~MissionDecorator() = default;
    
    // Delegate basic calls to wrapped mission (can be overridden)
    std::string getName() const override {
        return wrapped_mission_ ? wrapped_mission_->getName() : "Unknown Mission";
    }
    
    double getEstimatedDuration(const MissionParameters& params) const override {
        return wrapped_mission_ ? wrapped_mission_->getEstimatedDuration(params) : 0.0;
    }
    
    double getEstimatedCost(const MissionParameters& params) const override {
        return wrapped_mission_ ? wrapped_mission_->getEstimatedCost(params) : 0.0;
    }
    
    std::vector<std::string> getRequiredSkills() const override {
        return wrapped_mission_ ? wrapped_mission_->getRequiredSkills() : std::vector<std::string>{};
    }
    
    double getSuccessProbability(const MissionParameters& params) const override {
        return wrapped_mission_ ? wrapped_mission_->getSuccessProbability(params) : 0.0;
    }

protected:
    std::unique_ptr<IMissionComponent> wrapped_mission_;
};

/**
 * @brief Stealth Enhancement Decorator
 * 
 * Adds stealth capabilities to missions, reducing detection risk
 * but increasing duration and cost.
 */
class StealthEnhancement : public MissionDecorator {
public:
    explicit StealthEnhancement(std::unique_ptr<IMissionComponent> mission, double stealth_level = 1.0)
        : MissionDecorator(std::move(mission)), stealth_level_(stealth_level) {}
    
    MissionResult execute(const MissionParameters& params) override {
        if (!wrapped_mission_) {
            return MissionResult(false, 0.0, 0.0, 0.0);
        }
        
        // Execute the wrapped mission
        MissionResult result = wrapped_mission_->execute(params);
        
        // Apply stealth enhancements
        result.completion_time *= (1.0 + stealth_level_ * 0.2); // 20% time increase per level
        result.resource_cost *= (1.0 + stealth_level_ * 0.3); // 30% cost increase per level
        
        // Improve success rate for stealth-sensitive missions
        if (result.success) {
            result.experience_gained *= (1.0 + stealth_level_ * 0.1); // Bonus experience
            result.setData("stealth_success", true);
            result.setData("detection_avoided", true);
        }
        
        result.description += " [Stealth Enhanced]";
        result.setData("stealth_level", stealth_level_);
        
        return result;
    }
    
    std::string getName() const override {
        return MissionDecorator::getName() + " + Stealth Enhancement";
    }
    
    double getEstimatedDuration(const MissionParameters& params) const override {
        return MissionDecorator::getEstimatedDuration(params) * (1.0 + stealth_level_ * 0.2);
    }
    
    double getEstimatedCost(const MissionParameters& params) const override {
        return MissionDecorator::getEstimatedCost(params) * (1.0 + stealth_level_ * 0.3);
    }
    
    std::vector<std::string> getRequiredSkills() const override {
        auto skills = MissionDecorator::getRequiredSkills();
        skills.push_back("Stealth_Operations");
        skills.push_back("Electronic_Warfare");
        return skills;
    }
    
    double getSuccessProbability(const MissionParameters& params) const override {
        double base_prob = MissionDecorator::getSuccessProbability(params);
        return std::min(0.95, base_prob + stealth_level_ * 0.1); // 10% success bonus per level
    }
    
    std::unique_ptr<IMissionComponent> clone() const override {
        return std::make_unique<StealthEnhancement>(wrapped_mission_->clone(), stealth_level_);
    }

private:
    double stealth_level_;
};

/**
 * @brief Speed Boost Decorator
 * 
 * Reduces mission duration but increases resource consumption
 * and may affect success probability.
 */
class SpeedBoost : public MissionDecorator {
public:
    explicit SpeedBoost(std::unique_ptr<IMissionComponent> mission, double boost_factor = 1.5)
        : MissionDecorator(std::move(mission)), boost_factor_(boost_factor) {}
    
    MissionResult execute(const MissionParameters& params) override {
        if (!wrapped_mission_) {
            return MissionResult(false, 0.0, 0.0, 0.0);
        }
        
        MissionResult result = wrapped_mission_->execute(params);
        
        // Apply speed modifications
        result.completion_time /= boost_factor_; // Reduce time
        result.resource_cost *= (1.0 + (boost_factor_ - 1.0) * 0.5); // Increase cost
        
        // Speed may affect mission outcome
        if (boost_factor_ > 2.0) {
            // High speed boost may reduce success due to rushed execution
            double speed_penalty = (boost_factor_ - 2.0) * 0.1;
            if ((static_cast<double>(rand()) / RAND_MAX) < speed_penalty) {
                result.success = false;
                result.setData("rushed_failure", true);
            }
        }
        
        result.description += " [Speed Boost x" + std::to_string(boost_factor_) + "]";
        result.setData("speed_boost_factor", boost_factor_);
        
        return result;
    }
    
    std::string getName() const override {
        return MissionDecorator::getName() + " + Speed Boost";
    }
    
    double getEstimatedDuration(const MissionParameters& params) const override {
        return MissionDecorator::getEstimatedDuration(params) / boost_factor_;
    }
    
    double getEstimatedCost(const MissionParameters& params) const override {
        return MissionDecorator::getEstimatedCost(params) * (1.0 + (boost_factor_ - 1.0) * 0.5);
    }
    
    std::vector<std::string> getRequiredSkills() const override {
        auto skills = MissionDecorator::getRequiredSkills();
        skills.push_back("High_Speed_Maneuvering");
        return skills;
    }
    
    double getSuccessProbability(const MissionParameters& params) const override {
        double base_prob = MissionDecorator::getSuccessProbability(params);
        if (boost_factor_ > 2.0) {
            double penalty = (boost_factor_ - 2.0) * 0.1;
            return std::max(0.1, base_prob - penalty);
        }
        return base_prob;
    }
    
    std::unique_ptr<IMissionComponent> clone() const override {
        return std::make_unique<SpeedBoost>(wrapped_mission_->clone(), boost_factor_);
    }

private:
    double boost_factor_;
};

/**
 * @brief Heavy Armament Decorator
 * 
 * Adds heavy weapons and armor to missions, improving combat effectiveness
 * but significantly increasing cost and duration.
 */
class HeavyArmament : public MissionDecorator {
public:
    explicit HeavyArmament(std::unique_ptr<IMissionComponent> mission, double armament_level = 1.0)
        : MissionDecorator(std::move(mission)), armament_level_(armament_level) {}
    
    MissionResult execute(const MissionParameters& params) override {
        if (!wrapped_mission_) {
            return MissionResult(false, 0.0, 0.0, 0.0);
        }
        
        MissionResult result = wrapped_mission_->execute(params);
        
        // Apply heavy armament effects
        result.completion_time *= (1.0 + armament_level_ * 0.15); // Slower due to heavy equipment
        result.resource_cost *= (1.0 + armament_level_ * 0.4); // Much higher cost
        
        // Improved combat effectiveness
        if (result.success) {
            result.experience_gained *= (1.0 + armament_level_ * 0.15);
            
            // Reduce casualties for combat missions
            auto casualties = result.getData<int>("casualties");
            if (casualties) {
                int reduced_casualties = std::max(0, static_cast<int>(casualties.value() * (1.0 - armament_level_ * 0.2)));
                result.setData("casualties", reduced_casualties);
                result.setData("armament_protection", casualties.value() - reduced_casualties);
            }
        }
        
        result.description += " [Heavy Armament Level " + std::to_string(static_cast<int>(armament_level_)) + "]";
        result.setData("armament_level", armament_level_);
        
        return result;
    }
    
    std::string getName() const override {
        return MissionDecorator::getName() + " + Heavy Armament";
    }
    
    double getEstimatedDuration(const MissionParameters& params) const override {
        return MissionDecorator::getEstimatedDuration(params) * (1.0 + armament_level_ * 0.15);
    }
    
    double getEstimatedCost(const MissionParameters& params) const override {
        return MissionDecorator::getEstimatedCost(params) * (1.0 + armament_level_ * 0.4);
    }
    
    std::vector<std::string> getRequiredSkills() const override {
        auto skills = MissionDecorator::getRequiredSkills();
        skills.push_back("Heavy_Weapons");
        skills.push_back("Armor_Operations");
        return skills;
    }
    
    double getSuccessProbability(const MissionParameters& params) const override {
        double base_prob = MissionDecorator::getSuccessProbability(params);
        return std::min(0.95, base_prob + armament_level_ * 0.15); // Combat bonus
    }
    
    std::unique_ptr<IMissionComponent> clone() const override {
        return std::make_unique<HeavyArmament>(wrapped_mission_->clone(), armament_level_);
    }

private:
    double armament_level_;
};

/**
 * @brief Reconnaissance Support Decorator
 * 
 * Adds advanced scouting and intelligence gathering capabilities.
 */
class ReconnaissanceSupport : public MissionDecorator {
public:
    explicit ReconnaissanceSupport(std::unique_ptr<IMissionComponent> mission)
        : MissionDecorator(std::move(mission)) {}
    
    MissionResult execute(const MissionParameters& params) override {
        if (!wrapped_mission_) {
            return MissionResult(false, 0.0, 0.0, 0.0);
        }
        
        MissionResult result = wrapped_mission_->execute(params);
        
        // Reconnaissance provides intelligence bonus
        result.completion_time *= 1.1; // Slightly longer due to recon phase
        result.resource_cost *= 1.2; // Additional recon equipment cost
        
        if (result.success) {
            result.experience_gained *= 1.2; // Intelligence bonus
            result.setData("intelligence_gathered", true);
            result.setData("enemy_positions_known", rand() % 5 + 3);
            result.setData("terrain_mapped", true);
        }
        
        result.description += " [Reconnaissance Support]";
        
        return result;
    }
    
    std::string getName() const override {
        return MissionDecorator::getName() + " + Reconnaissance Support";
    }
    
    double getEstimatedDuration(const MissionParameters& params) const override {
        return MissionDecorator::getEstimatedDuration(params) * 1.1;
    }
    
    double getEstimatedCost(const MissionParameters& params) const override {
        return MissionDecorator::getEstimatedCost(params) * 1.2;
    }
    
    std::vector<std::string> getRequiredSkills() const override {
        auto skills = MissionDecorator::getRequiredSkills();
        skills.push_back("Intelligence_Analysis");
        skills.push_back("Reconnaissance");
        return skills;
    }
    
    double getSuccessProbability(const MissionParameters& params) const override {
        double base_prob = MissionDecorator::getSuccessProbability(params);
        return std::min(0.9, base_prob + 0.1); // Intelligence improves success rate
    }
    
    std::unique_ptr<IMissionComponent> clone() const override {
        return std::make_unique<ReconnaissanceSupport>(wrapped_mission_->clone());
    }
};

/**
 * @brief Medical Support Decorator
 * 
 * Adds medical team and equipment for crew safety and mission continuity.
 */
class MedicalSupport : public MissionDecorator {
public:
    explicit MedicalSupport(std::unique_ptr<IMissionComponent> mission, int medical_team_size = 2)
        : MissionDecorator(std::move(mission)), medical_team_size_(medical_team_size) {}
    
    MissionResult execute(const MissionParameters& params) override {
        if (!wrapped_mission_) {
            return MissionResult(false, 0.0, 0.0, 0.0);
        }
        
        MissionResult result = wrapped_mission_->execute(params);
        
        // Medical support effects
        result.completion_time *= (1.0 + medical_team_size_ * 0.05); // Slight time increase
        result.resource_cost *= (1.0 + medical_team_size_ * 0.1); // Medical equipment cost
        
        // Reduce casualties and improve survival rates
        auto casualties = result.getData<int>("casualties");
        if (casualties) {
            int medical_saves = std::min(casualties.value(), medical_team_size_);
            int final_casualties = casualties.value() - medical_saves;
            result.setData("casualties", final_casualties);
            result.setData("medical_saves", medical_saves);
        }
        
        result.description += " [Medical Support Team: " + std::to_string(medical_team_size_) + "]";
        result.setData("medical_team_size", medical_team_size_);
        
        return result;
    }
    
    std::string getName() const override {
        return MissionDecorator::getName() + " + Medical Support";
    }
    
    double getEstimatedDuration(const MissionParameters& params) const override {
        return MissionDecorator::getEstimatedDuration(params) * (1.0 + medical_team_size_ * 0.05);
    }
    
    double getEstimatedCost(const MissionParameters& params) const override {
        return MissionDecorator::getEstimatedCost(params) * (1.0 + medical_team_size_ * 0.1);
    }
    
    std::vector<std::string> getRequiredSkills() const override {
        auto skills = MissionDecorator::getRequiredSkills();
        skills.push_back("Medical_Treatment");
        skills.push_back("Emergency_Response");
        return skills;
    }
    
    std::unique_ptr<IMissionComponent> clone() const override {
        return std::make_unique<MedicalSupport>(wrapped_mission_->clone(), medical_team_size_);
    }

private:
    int medical_team_size_;
};

/**
 * @brief Mission Builder - Factory for creating decorated missions
 * 
 * Provides a fluent interface for building complex missions with multiple decorators.
 */
class MissionBuilder {
public:
    /**
     * @brief Start building with a base mission
     */
    static MissionBuilder create(std::unique_ptr<IMissionComponent> base_mission) {
        MissionBuilder builder;
        builder.mission_ = std::move(base_mission);
        return builder;
    }
    
    /**
     * @brief Add stealth enhancement
     */
    MissionBuilder& withStealth(double stealth_level = 1.0) {
        if (mission_) {
            mission_ = std::make_unique<StealthEnhancement>(std::move(mission_), stealth_level);
        }
        return *this;
    }
    
    /**
     * @brief Add speed boost
     */
    MissionBuilder& withSpeedBoost(double boost_factor = 1.5) {
        if (mission_) {
            mission_ = std::make_unique<SpeedBoost>(std::move(mission_), boost_factor);
        }
        return *this;
    }
    
    /**
     * @brief Add heavy armament
     */
    MissionBuilder& withHeavyArmament(double armament_level = 1.0) {
        if (mission_) {
            mission_ = std::make_unique<HeavyArmament>(std::move(mission_), armament_level);
        }
        return *this;
    }
    
    /**
     * @brief Add reconnaissance support
     */
    MissionBuilder& withReconnaissance() {
        if (mission_) {
            mission_ = std::make_unique<ReconnaissanceSupport>(std::move(mission_));
        }
        return *this;
    }
    
    /**
     * @brief Add medical support
     */
    MissionBuilder& withMedicalSupport(int team_size = 2) {
        if (mission_) {
            mission_ = std::make_unique<MedicalSupport>(std::move(mission_), team_size);
        }
        return *this;
    }
    
    /**
     * @brief Build and return the final mission
     */
    std::unique_ptr<IMissionComponent> build() {
        return std::move(mission_);
    }

private:
    std::unique_ptr<IMissionComponent> mission_;
};

/**
 * @brief Mission Enhancement Analyzer
 * 
 * Analyzes the effects of different enhancement combinations
 * and provides recommendations.
 */
class MissionEnhancementAnalyzer {
public:
    /**
     * @brief Analyze enhancement impact
     */
    struct EnhancementAnalysis {
        double time_multiplier = 1.0;
        double cost_multiplier = 1.0;
        double success_bonus = 0.0;
        std::vector<std::string> added_skills;
        std::string recommendation;
    };
    
    /**
     * @brief Analyze the impact of applying enhancements to a mission
     */
    static EnhancementAnalysis analyzeEnhancements(
        const IMissionComponent& base_mission,
        const IMissionComponent& enhanced_mission,
        const MissionParameters& params
    ) {
        EnhancementAnalysis analysis;
        
        double base_time = base_mission.getEstimatedDuration(params);
        double enhanced_time = enhanced_mission.getEstimatedDuration(params);
        analysis.time_multiplier = base_time > 0 ? enhanced_time / base_time : 1.0;
        
        double base_cost = base_mission.getEstimatedCost(params);
        double enhanced_cost = enhanced_mission.getEstimatedCost(params);
        analysis.cost_multiplier = base_cost > 0 ? enhanced_cost / base_cost : 1.0;
        
        double base_success = base_mission.getSuccessProbability(params);
        double enhanced_success = enhanced_mission.getSuccessProbability(params);
        analysis.success_bonus = enhanced_success - base_success;
        
        auto base_skills = base_mission.getRequiredSkills();
        auto enhanced_skills = enhanced_mission.getRequiredSkills();
        
        for (const auto& skill : enhanced_skills) {
            if (std::find(base_skills.begin(), base_skills.end(), skill) == base_skills.end()) {
                analysis.added_skills.push_back(skill);
            }
        }
        
        // Generate recommendation
        if (analysis.success_bonus > 0.2 && analysis.cost_multiplier < 1.5) {
            analysis.recommendation = "Highly recommended - significant success improvement at reasonable cost";
        } else if (analysis.success_bonus > 0.1) {
            analysis.recommendation = "Recommended - good success improvement";
        } else if (analysis.cost_multiplier > 2.0) {
            analysis.recommendation = "Expensive - consider if resources allow";
        } else {
            analysis.recommendation = "Moderate enhancement - evaluate based on mission importance";
        }
        
        return analysis;
    }
};

} // namespace CppVerseHub::Patterns