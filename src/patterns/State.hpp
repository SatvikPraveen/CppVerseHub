// File: src/patterns/State.hpp
// CppVerseHub - State Pattern Implementation for Mission State Management

#pragma once

#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <chrono>
#include <functional>
#include <any>
#include <optional>

namespace CppVerseHub::Patterns {

/**
 * @brief Forward declarations
 */
class MissionContext;
class IMissionState;

/**
 * @brief Mission data structure
 */
struct MissionData {
    std::string mission_id;
    std::string mission_type;
    std::string target_location;
    std::string assigned_fleet;
    double priority_level;
    double completion_percentage;
    std::chrono::system_clock::time_point start_time;
    std::chrono::system_clock::time_point estimated_completion;
    std::unordered_map<std::string, std::any> mission_parameters;
    
    MissionData(const std::string& id = "", const std::string& type = "")
        : mission_id(id), mission_type(type), priority_level(1.0), completion_percentage(0.0),
          start_time(std::chrono::system_clock::now()) {}
    
    template<typename T>
    void setParameter(const std::string& key, const T& value) {
        mission_parameters[key] = value;
    }
    
    template<typename T>
    std::optional<T> getParameter(const std::string& key) const {
        auto it = mission_parameters.find(key);
        if (it != mission_parameters.end()) {
            try {
                return std::any_cast<T>(it->second);
            } catch (const std::bad_any_cast&) {
                return std::nullopt;
            }
        }
        return std::nullopt;
    }
    
    bool hasParameter(const std::string& key) const {
        return mission_parameters.find(key) != mission_parameters.end();
    }
};

/**
 * @brief State transition result
 */
struct StateTransitionResult {
    bool success;
    std::string message;
    std::string previous_state;
    std::string new_state;
    std::chrono::system_clock::time_point transition_time;
    
    StateTransitionResult(bool success = false, const std::string& message = "",
                         const std::string& prev_state = "", const std::string& new_state = "")
        : success(success), message(message), previous_state(prev_state), new_state(new_state),
          transition_time(std::chrono::system_clock::now()) {}
};

/**
 * @brief Abstract Mission State interface
 * 
 * Defines the interface for different mission states and their behaviors.
 */
class IMissionState {
public:
    virtual ~IMissionState() = default;
    
    /**
     * @brief Handle mission initiation
     * @param context Mission context
     * @return State transition result
     */
    virtual StateTransitionResult initiate(MissionContext& context) = 0;
    
    /**
     * @brief Handle mission execution progress
     * @param context Mission context
     * @return State transition result
     */
    virtual StateTransitionResult execute(MissionContext& context) = 0;
    
    /**
     * @brief Handle mission pause request
     * @param context Mission context
     * @return State transition result
     */
    virtual StateTransitionResult pause(MissionContext& context) = 0;
    
    /**
     * @brief Handle mission resume request
     * @param context Mission context
     * @return State transition result
     */
    virtual StateTransitionResult resume(MissionContext& context) = 0;
    
    /**
     * @brief Handle mission cancellation
     * @param context Mission context
     * @return State transition result
     */
    virtual StateTransitionResult cancel(MissionContext& context) = 0;
    
    /**
     * @brief Handle mission completion
     * @param context Mission context
     * @return State transition result
     */
    virtual StateTransitionResult complete(MissionContext& context) = 0;
    
    /**
     * @brief Update state (called periodically)
     * @param context Mission context
     * @return State transition result
     */
    virtual StateTransitionResult update(MissionContext& context) = 0;
    
    /**
     * @brief Get state name
     * @return State name
     */
    virtual std::string getStateName() const = 0;
    
    /**
     * @brief Check if state allows specific transitions
     * @param target_state Target state name
     * @return true if transition is allowed
     */
    virtual bool canTransitionTo(const std::string& target_state) const = 0;
    
    /**
     * @brief Get allowed transitions from this state
     * @return Vector of allowed target state names
     */
    virtual std::vector<std::string> getAllowedTransitions() const = 0;
    
    /**
     * @brief Handle entry into this state
     * @param context Mission context
     */
    virtual void onEnter(MissionContext& context) = 0;
    
    /**
     * @brief Handle exit from this state
     * @param context Mission context
     */
    virtual void onExit(MissionContext& context) = 0;
    
    /**
     * @brief Check if state is terminal (mission finished)
     * @return true if state is terminal
     */
    virtual bool isTerminal() const = 0;
    
    /**
     * @brief Get estimated time remaining in this state
     * @param context Mission context
     * @return Estimated time in seconds
     */
    virtual double getEstimatedTimeRemaining(const MissionContext& context) const = 0;
};

/**
 * @brief Mission Context - Maintains mission state and data
 * 
 * Context class that holds the current state and provides interface
 * for state transitions and mission operations.
 */
class MissionContext {
public:
    explicit MissionContext(const MissionData& mission_data)
        : mission_data_(mission_data), current_state_(nullptr) {}
    
    /**
     * @brief Set current state
     * @param state New state
     */
    void setState(std::unique_ptr<IMissionState> state) {
        if (current_state_) {
            current_state_->onExit(*this);
        }
        
        std::string previous_state_name = current_state_ ? current_state_->getStateName() : "None";
        current_state_ = std::move(state);
        
        if (current_state_) {
            current_state_->onEnter(*this);
            addStateTransition(previous_state_name, current_state_->getStateName(), true, "State changed");
        }
    }
    
    /**
     * @brief Get current state
     * @return Pointer to current state
     */
    IMissionState* getCurrentState() const {
        return current_state_.get();
    }
    
    /**
     * @brief Get current state name
     * @return State name or "None" if no state set
     */
    std::string getCurrentStateName() const {
        return current_state_ ? current_state_->getStateName() : "None";
    }
    
    /**
     * @brief Get mission data (mutable)
     * @return Reference to mission data
     */
    MissionData& getMissionData() {
        return mission_data_;
    }
    
    /**
     * @brief Get mission data (const)
     * @return Const reference to mission data
     */
    const MissionData& getMissionData() const {
        return mission_data_;
    }
    
    /**
     * @brief Initiate mission
     * @return State transition result
     */
    StateTransitionResult initiate() {
        if (!current_state_) {
            return StateTransitionResult(false, "No state set for mission");
        }
        return current_state_->initiate(*this);
    }
    
    /**
     * @brief Execute mission
     * @return State transition result
     */
    StateTransitionResult execute() {
        if (!current_state_) {
            return StateTransitionResult(false, "No state set for mission");
        }
        return current_state_->execute(*this);
    }
    
    /**
     * @brief Pause mission
     * @return State transition result
     */
    StateTransitionResult pause() {
        if (!current_state_) {
            return StateTransitionResult(false, "No state set for mission");
        }
        return current_state_->pause(*this);
    }
    
    /**
     * @brief Resume mission
     * @return State transition result
     */
    StateTransitionResult resume() {
        if (!current_state_) {
            return StateTransitionResult(false, "No state set for mission");
        }
        return current_state_->resume(*this);
    }
    
    /**
     * @brief Cancel mission
     * @return State transition result
     */
    StateTransitionResult cancel() {
        if (!current_state_) {
            return StateTransitionResult(false, "No state set for mission");
        }
        return current_state_->cancel(*this);
    }
    
    /**
     * @brief Complete mission
     * @return State transition result
     */
    StateTransitionResult complete() {
        if (!current_state_) {
            return StateTransitionResult(false, "No state set for mission");
        }
        return current_state_->complete(*this);
    }
    
    /**
     * @brief Update mission state
     * @return State transition result
     */
    StateTransitionResult update() {
        if (!current_state_) {
            return StateTransitionResult(false, "No state set for mission");
        }
        return current_state_->update(*this);
    }
    
    /**
     * @brief Check if mission is complete
     * @return true if mission is in a terminal state
     */
    bool isComplete() const {
        return current_state_ && current_state_->isTerminal();
    }
    
    /**
     * @brief Get state transition history
     * @return Vector of state transition results
     */
    const std::vector<StateTransitionResult>& getStateHistory() const {
        return state_history_;
    }
    
    /**
     * @brief Add state transition to history
     */
    void addStateTransition(const std::string& from_state, const std::string& to_state, 
                           bool success, const std::string& message) {
        state_history_.emplace_back(success, message, from_state, to_state);
    }
    
    /**
     * @brief Get elapsed mission time
     * @return Duration since mission start
     */
    std::chrono::duration<double> getElapsedTime() const {
        return std::chrono::system_clock::now() - mission_data_.start_time;
    }
    
    /**
     * @brief Set completion percentage
     * @param percentage Completion percentage (0-100)
     */
    void setCompletionPercentage(double percentage) {
        mission_data_.completion_percentage = std::max(0.0, std::min(100.0, percentage));
    }

private:
    MissionData mission_data_;
    std::unique_ptr<IMissionState> current_state_;
    std::vector<StateTransitionResult> state_history_;
};

/**
 * @brief Pending State - Mission is created but not yet started
 */
class PendingState : public IMissionState {
public:
    StateTransitionResult initiate(MissionContext& context) override {
        // Transition to Planning state
        context.setState(createPlanningState());
        return StateTransitionResult(true, "Mission initiated, moving to planning phase", 
                                   "Pending", "Planning");
    }
    
    StateTransitionResult execute(MissionContext& context) override {
        return StateTransitionResult(false, "Cannot execute mission from pending state");
    }
    
    StateTransitionResult pause(MissionContext& context) override {
        return StateTransitionResult(false, "Cannot pause mission that hasn't started");
    }
    
    StateTransitionResult resume(MissionContext& context) override {
        return StateTransitionResult(false, "Cannot resume mission that hasn't started");
    }
    
    StateTransitionResult cancel(MissionContext& context) override {
        context.setState(createCancelledState());
        return StateTransitionResult(true, "Mission cancelled from pending state", 
                                   "Pending", "Cancelled");
    }
    
    StateTransitionResult complete(MissionContext& context) override {
        return StateTransitionResult(false, "Cannot complete mission that hasn't started");
    }
    
    StateTransitionResult update(MissionContext& context) override {
        // Check if mission should auto-start based on conditions
        auto& mission_data = context.getMissionData();
        
        // Simple auto-start logic (could be more sophisticated)
        if (!mission_data.assigned_fleet.empty() && !mission_data.target_location.empty()) {
            return initiate(context);
        }
        
        return StateTransitionResult(true, "Pending state maintained", "Pending", "Pending");
    }
    
    std::string getStateName() const override { return "Pending"; }
    
    bool canTransitionTo(const std::string& target_state) const override {
        return target_state == "Planning" || target_state == "Cancelled";
    }
    
    std::vector<std::string> getAllowedTransitions() const override {
        return {"Planning", "Cancelled"};
    }
    
    void onEnter(MissionContext& context) override {
        context.setCompletionPercentage(0.0);
    }
    
    void onExit(MissionContext& context) override {
        // Nothing special needed when exiting pending state
    }
    
    bool isTerminal() const override { return false; }
    
    double getEstimatedTimeRemaining(const MissionContext& context) const override {
        return 0.0; // No time estimate for pending missions
    }

private:
    std::unique_ptr<IMissionState> createPlanningState();
    std::unique_ptr<IMissionState> createCancelledState();
};

/**
 * @brief Planning State - Mission is being planned and prepared
 */
class PlanningState : public IMissionState {
public:
    StateTransitionResult initiate(MissionContext& context) override {
        return StateTransitionResult(false, "Mission already initiated");
    }
    
    StateTransitionResult execute(MissionContext& context) override {
        // Validate mission can be executed
        const auto& mission_data = context.getMissionData();
        
        if (mission_data.assigned_fleet.empty()) {
            return StateTransitionResult(false, "Cannot execute: No fleet assigned");
        }
        
        if (mission_data.target_location.empty()) {
            return StateTransitionResult(false, "Cannot execute: No target location specified");
        }
        
        // Transition to Active state
        context.setState(createActiveState());
        return StateTransitionResult(true, "Mission execution started", "Planning", "Active");
    }
    
    StateTransitionResult pause(MissionContext& context) override {
        return StateTransitionResult(false, "Cannot pause mission during planning");
    }
    
    StateTransitionResult resume(MissionContext& context) override {
        return StateTransitionResult(false, "Mission is not paused");
    }
    
    StateTransitionResult cancel(MissionContext& context) override {
        context.setState(createCancelledState());
        return StateTransitionResult(true, "Mission cancelled during planning", 
                                   "Planning", "Cancelled");
    }
    
    StateTransitionResult complete(MissionContext& context) override {
        return StateTransitionResult(false, "Cannot complete mission during planning");
    }
    
    StateTransitionResult update(MissionContext& context) override {
        // Simulate planning progress
        auto& mission_data = context.getMissionData();
        
        // Planning takes time based on mission complexity
        double planning_duration = 30.0; // Base 30 seconds
        if (mission_data.mission_type == "Combat") {
            planning_duration *= 1.5;
        } else if (mission_data.mission_type == "Colonization") {
            planning_duration *= 2.0;
        }
        
        auto elapsed = context.getElapsedTime().count();
        double planning_progress = std::min(1.0, elapsed / planning_duration);
        
        context.setCompletionPercentage(planning_progress * 20.0); // Planning is 20% of mission
        
        // Auto-transition to execution when planning is complete and conditions are met
        if (planning_progress >= 1.0 && 
            !mission_data.assigned_fleet.empty() && 
            !mission_data.target_location.empty()) {
            return execute(context);
        }
        
        return StateTransitionResult(true, "Planning in progress", "Planning", "Planning");
    }
    
    std::string getStateName() const override { return "Planning"; }
    
    bool canTransitionTo(const std::string& target_state) const override {
        return target_state == "Active" || target_state == "Cancelled";
    }
    
    std::vector<std::string> getAllowedTransitions() const override {
        return {"Active", "Cancelled"};
    }
    
    void onEnter(MissionContext& context) override {
        // Mark planning start time
        auto& mission_data = context.getMissionData();
        mission_data.setParameter("planning_start_time", std::chrono::system_clock::now());
    }
    
    void onExit(MissionContext& context) override {
        // Mark planning completion time
        auto& mission_data = context.getMissionData();
        mission_data.setParameter("planning_end_time", std::chrono::system_clock::now());
    }
    
    bool isTerminal() const override { return false; }
    
    double getEstimatedTimeRemaining(const MissionContext& context) const override {
        const auto& mission_data = context.getMissionData();
        double planning_duration = 30.0;
        
        if (mission_data.mission_type == "Combat") {
            planning_duration *= 1.5;
        } else if (mission_data.mission_type == "Colonization") {
            planning_duration *= 2.0;
        }
        
        return std::max(0.0, planning_duration - context.getElapsedTime().count());
    }

private:
    std::unique_ptr<IMissionState> createActiveState();
    std::unique_ptr<IMissionState> createCancelledState();
};

/**
 * @brief Active State - Mission is currently being executed
 */
class ActiveState : public IMissionState {
public:
    StateTransitionResult initiate(MissionContext& context) override {
        return StateTransitionResult(false, "Mission already active");
    }
    
    StateTransitionResult execute(MissionContext& context) override {
        return StateTransitionResult(false, "Mission already executing");
    }
    
    StateTransitionResult pause(MissionContext& context) override {
        context.setState(createPausedState());
        return StateTransitionResult(true, "Mission paused", "Active", "Paused");
    }
    
    StateTransitionResult resume(MissionContext& context) override {
        return StateTransitionResult(false, "Mission is not paused");
    }
    
    StateTransitionResult cancel(MissionContext& context) override {
        context.setState(createCancelledState());
        return StateTransitionResult(true, "Mission cancelled during execution", 
                                   "Active", "Cancelled");
    }
    
    StateTransitionResult complete(MissionContext& context) override {
        context.setState(createCompletedState());
        return StateTransitionResult(true, "Mission completed successfully", 
                                   "Active", "Completed");
    }
    
    StateTransitionResult update(MissionContext& context) override {
        // Simulate mission execution progress
        auto& mission_data = context.getMissionData();
        
        // Get mission duration based on type
        double mission_duration = getMissionDuration(mission_data.mission_type);
        
        // Factor in mission priority (higher priority = more resources = faster completion)
        mission_duration /= mission_data.priority_level;
        
        auto elapsed = context.getElapsedTime().count();
        double execution_progress = std::min(1.0, elapsed / mission_duration);
        
        // Set completion percentage (20% planning + 80% execution)
        context.setCompletionPercentage(20.0 + execution_progress * 80.0);
        
        // Check for mission completion
        if (execution_progress >= 1.0) {
            // Simulate success probability based on mission parameters
            double success_probability = calculateSuccessProbability(mission_data);
            bool mission_successful = (static_cast<double>(rand()) / RAND_MAX) < success_probability;
            
            if (mission_successful) {
                return complete(context);
            } else {
                context.setState(createFailedState());
                return StateTransitionResult(true, "Mission failed during execution", 
                                           "Active", "Failed");
            }
        }
        
        // Check for random events during execution
        if (checkForRandomEvents(context)) {
            // Random event occurred, might affect mission
            return StateTransitionResult(true, "Mission continues despite complications", 
                                       "Active", "Active");
        }
        
        return StateTransitionResult(true, "Mission execution in progress", "Active", "Active");
    }
    
    std::string getStateName() const override { return "Active"; }
    
    bool canTransitionTo(const std::string& target_state) const override {
        return target_state == "Paused" || target_state == "Cancelled" || 
               target_state == "Completed" || target_state == "Failed";
    }
    
    std::vector<std::string> getAllowedTransitions() const override {
        return {"Paused", "Cancelled", "Completed", "Failed"};
    }
    
    void onEnter(MissionContext& context) override {
        auto& mission_data = context.getMissionData();
        mission_data.setParameter("execution_start_time", std::chrono::system_clock::now());
        
        // Calculate estimated completion time
        double mission_duration = getMissionDuration(mission_data.mission_type);
        mission_duration /= mission_data.priority_level;
        
        auto estimated_completion = std::chrono::system_clock::now() + 
                                  std::chrono::seconds(static_cast<long>(mission_duration));
        mission_data.estimated_completion = estimated_completion;
    }
    
    void onExit(MissionContext& context) override {
        auto& mission_data = context.getMissionData();
        mission_data.setParameter("execution_end_time", std::chrono::system_clock::now());
    }
    
    bool isTerminal() const override { return false; }
    
    double getEstimatedTimeRemaining(const MissionContext& context) const override {
        const auto& mission_data = context.getMissionData();
        double mission_duration = getMissionDuration(mission_data.mission_type);
        mission_duration /= mission_data.priority_level;
        
        return std::max(0.0, mission_duration - context.getElapsedTime().count());
    }

private:
    std::unique_ptr<IMissionState> createPausedState();
    std::unique_ptr<IMissionState> createCancelledState();
    std::unique_ptr<IMissionState> createCompletedState();
    std::unique_ptr<IMissionState> createFailedState();
    
    double getMissionDuration(const std::string& mission_type) const {
        if (mission_type == "Exploration") return 120.0; // 2 minutes
        if (mission_type == "Combat") return 180.0;       // 3 minutes
        if (mission_type == "Colonization") return 300.0; // 5 minutes
        if (mission_type == "Transport") return 90.0;     // 1.5 minutes
        return 150.0; // Default duration
    }
    
    double calculateSuccessProbability(const MissionData& mission_data) const {
        double base_probability = 0.8; // 80% base success rate
        
        // Adjust based on mission type
        if (mission_data.mission_type == "Combat") {
            base_probability = 0.7; // Combat is riskier
        } else if (mission_data.mission_type == "Colonization") {
            base_probability = 0.6; // Colonization is complex
        }
        
        // Adjust based on priority (higher priority = better resources = higher success)
        base_probability += (mission_data.priority_level - 1.0) * 0.1;
        
        return std::min(0.95, std::max(0.1, base_probability));
    }
    
    bool checkForRandomEvents(MissionContext& context) {
        // 5% chance of random event per update
        return (static_cast<double>(rand()) / RAND_MAX) < 0.05;
    }
};

/**
 * @brief Paused State - Mission execution is temporarily halted
 */
class PausedState : public IMissionState {
public:
    StateTransitionResult initiate(MissionContext& context) override {
        return StateTransitionResult(false, "Cannot initiate paused mission");
    }
    
    StateTransitionResult execute(MissionContext& context) override {
        return StateTransitionResult(false, "Cannot execute paused mission - resume first");
    }
    
    StateTransitionResult pause(MissionContext& context) override {
        return StateTransitionResult(false, "Mission is already paused");
    }
    
    StateTransitionResult resume(MissionContext& context) override {
        context.setState(createActiveState());
        return StateTransitionResult(true, "Mission resumed", "Paused", "Active");
    }
    
    StateTransitionResult cancel(MissionContext& context) override {
        context.setState(createCancelledState());
        return StateTransitionResult(true, "Mission cancelled while paused", "Paused", "Cancelled");
    }
    
    StateTransitionResult complete(MissionContext& context) override {
        return StateTransitionResult(false, "Cannot complete paused mission - resume first");
    }
    
    StateTransitionResult update(MissionContext& context) override {
        // Paused missions don't progress but we can check for timeout
        auto& mission_data = context.getMissionData();
        auto pause_start = mission_data.getParameter<std::chrono::system_clock::time_point>("pause_start_time");
        
        if (pause_start) {
            auto pause_duration = std::chrono::system_clock::now() - pause_start.value();
            auto pause_seconds = std::chrono::duration_cast<std::chrono::seconds>(pause_duration).count();
            
            // Auto-cancel if paused for too long (1 hour)
            if (pause_seconds > 3600) {
                return cancel(context);
            }
        }
        
        return StateTransitionResult(true, "Mission remains paused", "Paused", "Paused");
    }
    
    std::string getStateName() const override { return "Paused"; }
    
    bool canTransitionTo(const std::string& target_state) const override {
        return target_state == "Active" || target_state == "Cancelled";
    }
    
    std::vector<std::string> getAllowedTransitions() const override {
        return {"Active", "Cancelled"};
    }
    
    void onEnter(MissionContext& context) override {
        auto& mission_data = context.getMissionData();
        mission_data.setParameter("pause_start_time", std::chrono::system_clock::now());
    }
    
    void onExit(MissionContext& context) override {
        auto& mission_data = context.getMissionData();
        mission_data.setParameter("pause_end_time", std::chrono::system_clock::now());
        
        // Calculate total pause duration for metrics
        auto pause_start = mission_data.getParameter<std::chrono::system_clock::time_point>("pause_start_time");
        if (pause_start) {
            auto pause_duration = std::chrono::system_clock::now() - pause_start.value();
            auto existing_pause = mission_data.getParameter<double>("total_pause_duration").value_or(0.0);
            mission_data.setParameter("total_pause_duration", 
                existing_pause + std::chrono::duration<double>(pause_duration).count());
        }
    }
    
    bool isTerminal() const override { return false; }
    
    double getEstimatedTimeRemaining(const MissionContext& context) const override {
        // Return the same estimate as if mission were active
        // (pausing doesn't change the remaining work)
        const auto& mission_data = context.getMissionData();
        double mission_duration = 150.0; // Default
        
        if (mission_data.mission_type == "Exploration") mission_duration = 120.0;
        else if (mission_data.mission_type == "Combat") mission_duration = 180.0;
        else if (mission_data.mission_type == "Colonization") mission_duration = 300.0;
        else if (mission_data.mission_type == "Transport") mission_duration = 90.0;
        
        mission_duration /= mission_data.priority_level;
        
        return std::max(0.0, mission_duration - context.getElapsedTime().count());
    }

private:
    std::unique_ptr<IMissionState> createActiveState();
    std::unique_ptr<IMissionState> createCancelledState();
};

/**
 * @brief Completed State - Mission finished successfully (Terminal state)
 */
class CompletedState : public IMissionState {
public:
    StateTransitionResult initiate(MissionContext& context) override {
        return StateTransitionResult(false, "Mission already completed");
    }
    
    StateTransitionResult execute(MissionContext& context) override {
        return StateTransitionResult(false, "Mission already completed");
    }
    
    StateTransitionResult pause(MissionContext& context) override {
        return StateTransitionResult(false, "Cannot pause completed mission");
    }
    
    StateTransitionResult resume(MissionContext& context) override {
        return StateTransitionResult(false, "Cannot resume completed mission");
    }
    
    StateTransitionResult cancel(MissionContext& context) override {
        return StateTransitionResult(false, "Cannot cancel completed mission");
    }
    
    StateTransitionResult complete(MissionContext& context) override {
        return StateTransitionResult(false, "Mission already completed");
    }
    
    StateTransitionResult update(MissionContext& context) override {
        // Completed missions don't change state
        return StateTransitionResult(true, "Mission completed", "Completed", "Completed");
    }
    
    std::string getStateName() const override { return "Completed"; }
    
    bool canTransitionTo(const std::string& target_state) const override {
        return false; // Terminal state - no transitions allowed
    }
    
    std::vector<std::string> getAllowedTransitions() const override {
        return {}; // No transitions from completed state
    }
    
    void onEnter(MissionContext& context) override {
        auto& mission_data = context.getMissionData();
        mission_data.setParameter("completion_time", std::chrono::system_clock::now());
        context.setCompletionPercentage(100.0);
        
        // Calculate mission metrics
        auto execution_start = mission_data.getParameter<std::chrono::system_clock::time_point>("execution_start_time");
        if (execution_start) {
            auto execution_duration = std::chrono::system_clock::now() - execution_start.value();
            mission_data.setParameter("execution_duration", std::chrono::duration<double>(execution_duration).count());
        }
    }
    
    void onExit(MissionContext& context) override {
        // Terminal state - should never exit
    }
    
    bool isTerminal() const override { return true; }
    
    double getEstimatedTimeRemaining(const MissionContext& context) const override {
        return 0.0; // Mission is complete
    }
};

/**
 * @brief Failed State - Mission failed during execution (Terminal state)
 */
class FailedState : public IMissionState {
public:
    StateTransitionResult initiate(MissionContext& context) override {
        return StateTransitionResult(false, "Mission has failed");
    }
    
    StateTransitionResult execute(MissionContext& context) override {
        return StateTransitionResult(false, "Mission has failed");
    }
    
    StateTransitionResult pause(MissionContext& context) override {
        return StateTransitionResult(false, "Cannot pause failed mission");
    }
    
    StateTransitionResult resume(MissionContext& context) override {
        return StateTransitionResult(false, "Cannot resume failed mission");
    }
    
    StateTransitionResult cancel(MissionContext& context) override {
        return StateTransitionResult(false, "Mission already failed");
    }
    
    StateTransitionResult complete(MissionContext& context) override {
        return StateTransitionResult(false, "Mission has failed");
    }
    
    StateTransitionResult update(MissionContext& context) override {
        return StateTransitionResult(true, "Mission failed", "Failed", "Failed");
    }
    
    std::string getStateName() const override { return "Failed"; }
    
    bool canTransitionTo(const std::string& target_state) const override {
        return false; // Terminal state
    }
    
    std::vector<std::string> getAllowedTransitions() const override {
        return {};
    }
    
    void onEnter(MissionContext& context) override {
        auto& mission_data = context.getMissionData();
        mission_data.setParameter("failure_time", std::chrono::system_clock::now());
        
        // Set completion percentage to reflect partial completion
        // (keep current percentage as it represents work done before failure)
        
        // Record failure reason (could be expanded with more specific reasons)
        mission_data.setParameter("failure_reason", "Mission execution failed");
    }
    
    void onExit(MissionContext& context) override {
        // Terminal state - should never exit
    }
    
    bool isTerminal() const override { return true; }
    
    double getEstimatedTimeRemaining(const MissionContext& context) const override {
        return 0.0; // Mission has failed
    }
};

/**
 * @brief Cancelled State - Mission was cancelled before completion (Terminal state)
 */
class CancelledState : public IMissionState {
public:
    StateTransitionResult initiate(MissionContext& context) override {
        return StateTransitionResult(false, "Mission has been cancelled");
    }
    
    StateTransitionResult execute(MissionContext& context) override {
        return StateTransitionResult(false, "Mission has been cancelled");
    }
    
    StateTransitionResult pause(MissionContext& context) override {
        return StateTransitionResult(false, "Cannot pause cancelled mission");
    }
    
    StateTransitionResult resume(MissionContext& context) override {
        return StateTransitionResult(false, "Cannot resume cancelled mission");
    }
    
    StateTransitionResult cancel(MissionContext& context) override {
        return StateTransitionResult(false, "Mission already cancelled");
    }
    
    StateTransitionResult complete(MissionContext& context) override {
        return StateTransitionResult(false, "Mission has been cancelled");
    }
    
    StateTransitionResult update(MissionContext& context) override {
        return StateTransitionResult(true, "Mission cancelled", "Cancelled", "Cancelled");
    }
    
    std::string getStateName() const override { return "Cancelled"; }
    
    bool canTransitionTo(const std::string& target_state) const override {
        return false; // Terminal state
    }
    
    std::vector<std::string> getAllowedTransitions() const override {
        return {};
    }
    
    void onEnter(MissionContext& context) override {
        auto& mission_data = context.getMissionData();
        mission_data.setParameter("cancellation_time", std::chrono::system_clock::now());
        
        // Record cancellation reason
        mission_data.setParameter("cancellation_reason", "Mission cancelled by user/system");
    }
    
    void onExit(MissionContext& context) override {
        // Terminal state - should never exit
    }
    
    bool isTerminal() const override { return true; }
    
    double getEstimatedTimeRemaining(const MissionContext& context) const override {
        return 0.0; // Mission was cancelled
    }
};

/**
 * @brief State Factory - Creates state instances
 */
class MissionStateFactory {
public:
    static std::unique_ptr<IMissionState> createState(const std::string& state_name) {
        if (state_name == "Pending") return std::make_unique<PendingState>();
        if (state_name == "Planning") return std::make_unique<PlanningState>();
        if (state_name == "Active") return std::make_unique<ActiveState>();
        if (state_name == "Paused") return std::make_unique<PausedState>();
        if (state_name == "Completed") return std::make_unique<CompletedState>();
        if (state_name == "Failed") return std::make_unique<FailedState>();
        if (state_name == "Cancelled") return std::make_unique<CancelledState>();
        
        return nullptr; // Unknown state
    }
    
    static std::vector<std::string> getAvailableStates() {
        return {"Pending", "Planning", "Active", "Paused", "Completed", "Failed", "Cancelled"};
    }
    
    static bool isValidState(const std::string& state_name) {
        auto states = getAvailableStates();
        return std::find(states.begin(), states.end(), state_name) != states.end();
    }
    
    static bool isTerminalState(const std::string& state_name) {
        return state_name == "Completed" || state_name == "Failed" || state_name == "Cancelled";
    }
};

/**
 * @brief Mission Manager - High-level mission management
 * 
 * Manages multiple missions and their state transitions,
 * provides mission scheduling and monitoring capabilities.
 */
class MissionManager {
public:
    /**
     * @brief Create a new mission
     * @param mission_data Mission data
     * @return Mission context pointer or nullptr on failure
     */
    std::shared_ptr<MissionContext> createMission(const MissionData& mission_data) {
        auto context = std::make_shared<MissionContext>(mission_data);
        context->setState(MissionStateFactory::createState("Pending"));
        
        missions_[mission_data.mission_id] = context;
        return context;
    }
    
    /**
     * @brief Get mission by ID
     * @param mission_id Mission identifier
     * @return Mission context pointer or nullptr if not found
     */
    std::shared_ptr<MissionContext> getMission(const std::string& mission_id) const {
        auto it = missions_.find(mission_id);
        return (it != missions_.end()) ? it->second : nullptr;
    }
    
    /**
     * @brief Remove mission from manager
     * @param mission_id Mission identifier
     * @return true if mission was removed
     */
    bool removeMission(const std::string& mission_id) {
        auto it = missions_.find(mission_id);
        if (it != missions_.end()) {
            missions_.erase(it);
            return true;
        }
        return false;
    }
    
    /**
     * @brief Get all missions
     * @return Vector of mission contexts
     */
    std::vector<std::shared_ptr<MissionContext>> getAllMissions() const {
        std::vector<std::shared_ptr<MissionContext>> result;
        result.reserve(missions_.size());
        
        for (const auto& [id, context] : missions_) {
            result.push_back(context);
        }
        
        return result;
    }
    
    /**
     * @brief Get missions by state
     * @param state_name State name to filter by
     * @return Vector of matching mission contexts
     */
    std::vector<std::shared_ptr<MissionContext>> getMissionsByState(const std::string& state_name) const {
        std::vector<std::shared_ptr<MissionContext>> result;
        
        for (const auto& [id, context] : missions_) {
            if (context->getCurrentStateName() == state_name) {
                result.push_back(context);
            }
        }
        
        return result;
    }
    
    /**
     * @brief Get active missions (non-terminal states)
     * @return Vector of active mission contexts
     */
    std::vector<std::shared_ptr<MissionContext>> getActiveMissions() const {
        std::vector<std::shared_ptr<MissionContext>> result;
        
        for (const auto& [id, context] : missions_) {
            if (!context->isComplete()) {
                result.push_back(context);
            }
        }
        
        return result;
    }
    
    /**
     * @brief Update all active missions
     * @return Number of missions updated
     */
    int updateAllMissions() {
        int updated_count = 0;
        
        for (auto& [id, context] : missions_) {
            if (!context->isComplete()) {
                context->update();
                updated_count++;
            }
        }
        
        return updated_count;
    }
    
    /**
     * @brief Get mission statistics
     * @return Map of state names to counts
     */
    std::unordered_map<std::string, int> getMissionStatistics() const {
        std::unordered_map<std::string, int> stats;
        
        // Initialize all states with 0 count
        for (const auto& state : MissionStateFactory::getAvailableStates()) {
            stats[state] = 0;
        }
        
        // Count missions in each state
        for (const auto& [id, context] : missions_) {
            stats[context->getCurrentStateName()]++;
        }
        
        return stats;
    }
    
    /**
     * @brief Get total number of missions
     * @return Total mission count
     */
    size_t getMissionCount() const {
        return missions_.size();
    }
    
    /**
     * @brief Clear completed missions
     * @return Number of missions removed
     */
    int clearCompletedMissions() {
        int removed_count = 0;
        
        auto it = missions_.begin();
        while (it != missions_.end()) {
            if (it->second->isComplete()) {
                it = missions_.erase(it);
                removed_count++;
            } else {
                ++it;
            }
        }
        
        return removed_count;
    }
    
    /**
     * @brief Generate mission report
     * @return Formatted mission report string
     */
    std::string generateMissionReport() const {
        std::ostringstream report;
        report << "=== Mission Status Report ===\n\n";
        
        auto stats = getMissionStatistics();
        report << "Mission Statistics:\n";
        for (const auto& [state, count] : stats) {
            if (count > 0) {
                report << "  " << state << ": " << count << " missions\n";
            }
        }
        report << "\nTotal Missions: " << getMissionCount() << "\n\n";
        
        // List active missions
        auto active_missions = getActiveMissions();
        if (!active_missions.empty()) {
            report << "Active Missions:\n";
            for (const auto& context : active_missions) {
                const auto& data = context->getMissionData();
                report << "  " << data.mission_id << " (" << data.mission_type << ") - "
                       << context->getCurrentStateName() << " - "
                       << std::fixed << std::setprecision(1) << data.completion_percentage << "%\n";
            }
        }
        
        return report.str();
    }

private:
    std::unordered_map<std::string, std::shared_ptr<MissionContext>> missions_;
};

/**
 * @brief State Transition Monitor - Tracks state transitions and patterns
 */
class StateTransitionMonitor {
public:
    struct TransitionStats {
        int count = 0;
        double average_duration = 0.0;
        double total_duration = 0.0;
        std::chrono::system_clock::time_point first_occurrence;
        std::chrono::system_clock::time_point last_occurrence;
    };
    
    /**
     * @brief Record a state transition
     * @param from_state Source state
     * @param to_state Target state
     * @param duration Transition duration in seconds
     */
    void recordTransition(const std::string& from_state, const std::string& to_state, double duration) {
        std::string transition_key = from_state + "->" + to_state;
        
        auto& stats = transition_stats_[transition_key];
        
        if (stats.count == 0) {
            stats.first_occurrence = std::chrono::system_clock::now();
        }
        
        stats.count++;
        stats.total_duration += duration;
        stats.average_duration = stats.total_duration / stats.count;
        stats.last_occurrence = std::chrono::system_clock::now();
    }
    
    /**
     * @brief Get transition statistics
     * @param from_state Source state
     * @param to_state Target state
     * @return Optional transition statistics
     */
    std::optional<TransitionStats> getTransitionStats(const std::string& from_state, 
                                                     const std::string& to_state) const {
        std::string transition_key = from_state + "->" + to_state;
        auto it = transition_stats_.find(transition_key);
        
        return (it != transition_stats_.end()) ? std::make_optional(it->second) : std::nullopt;
    }
    
    /**
     * @brief Get all recorded transitions
     * @return Vector of transition key strings
     */
    std::vector<std::string> getAllTransitions() const {
        std::vector<std::string> transitions;
        transitions.reserve(transition_stats_.size());
        
        for (const auto& [key, stats] : transition_stats_) {
            transitions.push_back(key);
        }
        
        return transitions;
    }
    
    /**
     * @brief Get most common transitions
     * @param limit Maximum number of transitions to return
     * @return Vector of transition keys sorted by frequency
     */
    std::vector<std::string> getMostCommonTransitions(int limit = 10) const {
        std::vector<std::pair<std::string, int>> transition_counts;
        
        for (const auto& [key, stats] : transition_stats_) {
            transition_counts.emplace_back(key, stats.count);
        }
        
        // Sort by count (descending)
        std::sort(transition_counts.begin(), transition_counts.end(),
                 [](const auto& a, const auto& b) { return a.second > b.second; });
        
        std::vector<std::string> result;
        int count = std::min(limit, static_cast<int>(transition_counts.size()));
        
        for (int i = 0; i < count; ++i) {
            result.push_back(transition_counts[i].first);
        }
        
        return result;
    }
    
    /**
     * @brief Clear all transition statistics
     */
    void clearStats() {
        transition_stats_.clear();
    }
    
    /**
     * @brief Generate transition report
     * @return Formatted transition report
     */
    std::string generateTransitionReport() const {
        std::ostringstream report;
        report << "=== State Transition Analysis ===\n\n";
        
        auto common_transitions = getMostCommonTransitions(5);
        if (!common_transitions.empty()) {
            report << "Most Common Transitions:\n";
            for (const auto& transition : common_transitions) {
                auto stats = transition_stats_.at(transition);
                report << "  " << transition << ": " << stats.count << " times, "
                       << "avg duration: " << std::fixed << std::setprecision(2) 
                       << stats.average_duration << "s\n";
            }
        }
        
        report << "\nTotal Unique Transitions: " << transition_stats_.size() << "\n";
        
        return report.str();
    }

private:
    std::unordered_map<std::string, TransitionStats> transition_stats_;
};

// Forward declaration implementations for state creation methods
// These need to be defined after all state classes are declared

inline std::unique_ptr<IMissionState> PendingState::createPlanningState() {
    return std::make_unique<PlanningState>();
}

inline std::unique_ptr<IMissionState> PendingState::createCancelledState() {
    return std::make_unique<CancelledState>();
}

inline std::unique_ptr<IMissionState> PlanningState::createActiveState() {
    return std::make_unique<ActiveState>();
}

inline std::unique_ptr<IMissionState> PlanningState::createCancelledState() {
    return std::make_unique<CancelledState>();
}

inline std::unique_ptr<IMissionState> ActiveState::createPausedState() {
    return std::make_unique<PausedState>();
}

inline std::unique_ptr<IMissionState> ActiveState::createCancelledState() {
    return std::make_unique<CancelledState>();
}

inline std::unique_ptr<IMissionState> ActiveState::createCompletedState() {
    return std::make_unique<CompletedState>();
}

inline std::unique_ptr<IMissionState> ActiveState::createFailedState() {
    return std::make_unique<FailedState>();
}

inline std::unique_ptr<IMissionState> PausedState::createActiveState() {
    return std::make_unique<ActiveState>();
}

inline std::unique_ptr<IMissionState> PausedState::createCancelledState() {
    return std::make_unique<CancelledState>();
}

} // namespace CppVerseHub::Patterns