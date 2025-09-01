// File: src/patterns/Command.hpp
// CppVerseHub - Command Pattern Implementation for Fleet Operations

#pragma once

#include <memory>
#include <string>
#include <vector>
#include <stack>
#include <queue>
#include <unordered_map>
#include <functional>
#include <chrono>
#include <mutex>
#include <any>
#include <optional>

namespace CppVerseHub::Patterns {

/**
 * @brief Command result structure
 */
struct CommandResult {
    bool success;
    std::string message;
    std::chrono::system_clock::time_point execution_time;
    std::unordered_map<std::string, std::any> result_data;
    
    CommandResult(bool success = false, const std::string& message = "")
        : success(success), message(message), execution_time(std::chrono::system_clock::now()) {}
    
    template<typename T>
    void setData(const std::string& key, const T& value) {
        result_data[key] = value;
    }
    
    template<typename T>
    std::optional<T> getData(const std::string& key) const {
        auto it = result_data.find(key);
        if (it != result_data.end()) {
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
 * @brief Abstract Command interface
 * 
 * Base interface for all commands in the system.
 * Supports execution, undo operations, and metadata.
 */
class ICommand {
public:
    virtual ~ICommand() = default;
    
    /**
     * @brief Execute the command
     * @return Command execution result
     */
    virtual CommandResult execute() = 0;
    
    /**
     * @brief Undo the command (if possible)
     * @return Command undo result
     */
    virtual CommandResult undo() = 0;
    
    /**
     * @brief Check if command can be undone
     * @return true if command supports undo
     */
    virtual bool canUndo() const = 0;
    
    /**
     * @brief Get command name/description
     * @return Command name
     */
    virtual std::string getName() const = 0;
    
    /**
     * @brief Get command category
     * @return Command category (e.g., "Fleet", "Mission", "Resource")
     */
    virtual std::string getCategory() const = 0;
    
    /**
     * @brief Get command priority
     * @return Command priority (higher values = higher priority)
     */
    virtual int getPriority() const { return 0; }
    
    /**
     * @brief Estimate execution time
     * @return Estimated execution time in milliseconds
     */
    virtual double getEstimatedExecutionTime() const { return 0.0; }
    
    /**
     * @brief Check if command is valid for execution
     * @return true if command can be executed
     */
    virtual bool isValid() const { return true; }
    
    /**
     * @brief Get command unique identifier
     * @return Command ID
     */
    virtual std::string getId() const = 0;
    
    /**
     * @brief Clone the command
     * @return Unique pointer to cloned command
     */
    virtual std::unique_ptr<ICommand> clone() const = 0;
};

/**
 * @brief Forward declarations
 */
class Fleet;
class Planet;
class Mission;

/**
 * @brief Fleet Receiver - Target of fleet commands
 * 
 * Encapsulates the actual fleet operations that commands will invoke.
 */
class FleetReceiver {
public:
    struct FleetInfo {
        std::string fleet_id;
        std::string current_location;
        int ship_count;
        double fuel_level;
        double health;
        std::string status;
    };
    
    explicit FleetReceiver(const std::string& fleet_id) : fleet_id_(fleet_id) {
        // Initialize fleet data
        fleet_info_.fleet_id = fleet_id;
        fleet_info_.current_location = "Earth Orbit";
        fleet_info_.ship_count = 5;
        fleet_info_.fuel_level = 100.0;
        fleet_info_.health = 100.0;
        fleet_info_.status = "Idle";
    }
    
    // Fleet operation methods
    bool moveToLocation(const std::string& destination, double fuel_cost = 10.0);
    bool attackTarget(const std::string& target, double damage_taken = 5.0);
    bool defendLocation(const std::string& location);
    bool refuelFleet(double fuel_amount);
    bool repairFleet(double repair_amount);
    bool deployShips(int ship_count, const std::string& location);
    bool recallShips(int ship_count);
    bool setFormation(const std::string& formation_type);
    bool upgradeFleet(const std::string& upgrade_type);
    
    // Fleet state queries
    const FleetInfo& getFleetInfo() const { return fleet_info_; }
    std::string getFleetId() const { return fleet_id_; }
    bool isAvailable() const { return fleet_info_.status == "Idle" || fleet_info_.status == "Ready"; }
    
    // State management for undo operations
    void saveState();
    void restoreState();
    
private:
    std::string fleet_id_;
    FleetInfo fleet_info_;
    FleetInfo saved_state_;
    std::mutex fleet_mutex_;
};

/**
 * @brief Move Fleet Command
 * 
 * Command to move a fleet to a new location.
 */
class MoveFleetCommand : public ICommand {
public:
    MoveFleetCommand(std::shared_ptr<FleetReceiver> fleet, const std::string& destination, 
                    const std::string& command_id = "")
        : fleet_(fleet), destination_(destination), 
          command_id_(command_id.empty() ? generateCommandId() : command_id),
          original_location_("") {}
    
    CommandResult execute() override {
        if (!fleet_) {
            return CommandResult(false, "Invalid fleet receiver");
        }
        
        original_location_ = fleet_->getFleetInfo().current_location;
        fleet_->saveState();
        
        bool success = fleet_->moveToLocation(destination_);
        std::string message = success ? 
            "Fleet moved to " + destination_ : 
            "Failed to move fleet to " + destination_;
        
        CommandResult result(success, message);
        result.setData("destination", destination_);
        result.setData("original_location", original_location_);
        result.setData("fleet_id", fleet_->getFleetId());
        
        executed_ = success;
        return result;
    }
    
    CommandResult undo() override {
        if (!executed_ || !fleet_) {
            return CommandResult(false, "Cannot undo: command not executed or invalid fleet");
        }
        
        fleet_->restoreState();
        executed_ = false;
        
        return CommandResult(true, "Fleet movement undone - returned to " + original_location_);
    }
    
    bool canUndo() const override { return executed_; }
    std::string getName() const override { return "Move Fleet"; }
    std::string getCategory() const override { return "Fleet"; }
    int getPriority() const override { return 5; }
    double getEstimatedExecutionTime() const override { return 100.0; } // 100ms
    std::string getId() const override { return command_id_; }
    
    std::unique_ptr<ICommand> clone() const override {
        return std::make_unique<MoveFleetCommand>(fleet_, destination_, command_id_ + "_clone");
    }

private:
    std::shared_ptr<FleetReceiver> fleet_;
    std::string destination_;
    std::string command_id_;
    std::string original_location_;
    bool executed_ = false;
    
    std::string generateCommandId() const {
        return "move_fleet_" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());
    }
};

/**
 * @brief Attack Command
 * 
 * Command to order fleet to attack a target.
 */
class AttackCommand : public ICommand {
public:
    AttackCommand(std::shared_ptr<FleetReceiver> fleet, const std::string& target,
                 const std::string& command_id = "")
        : fleet_(fleet), target_(target),
          command_id_(command_id.empty() ? generateCommandId() : command_id) {}
    
    CommandResult execute() override {
        if (!fleet_) {
            return CommandResult(false, "Invalid fleet receiver");
        }
        
        fleet_->saveState();
        bool success = fleet_->attackTarget(target_);
        
        std::string message = success ?
            "Fleet successfully attacked " + target_ :
            "Fleet failed to attack " + target_;
        
        CommandResult result(success, message);
        result.setData("target", target_);
        result.setData("fleet_id", fleet_->getFleetId());
        
        if (success) {
            result.setData("damage_dealt", 25.0 + (rand() % 50)); // Random damage 25-75
        }
        
        executed_ = success;
        return result;
    }
    
    CommandResult undo() override {
        if (!executed_ || !fleet_) {
            return CommandResult(false, "Cannot undo: attack command not executed or invalid fleet");
        }
        
        fleet_->restoreState();
        executed_ = false;
        
        return CommandResult(true, "Attack undone - fleet status restored");
    }
    
    bool canUndo() const override { return executed_; }
    std::string getName() const override { return "Attack Target"; }
    std::string getCategory() const override { return "Combat"; }
    int getPriority() const override { return 8; }
    double getEstimatedExecutionTime() const override { return 200.0; } // 200ms
    std::string getId() const override { return command_id_; }
    
    std::unique_ptr<ICommand> clone() const override {
        return std::make_unique<AttackCommand>(fleet_, target_, command_id_ + "_clone");
    }

private:
    std::shared_ptr<FleetReceiver> fleet_;
    std::string target_;
    std::string command_id_;
    bool executed_ = false;
    
    std::string generateCommandId() const {
        return "attack_" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());
    }
};

/**
 * @brief Defend Command
 * 
 * Command to order fleet to defend a location.
 */
class DefendCommand : public ICommand {
public:
    DefendCommand(std::shared_ptr<FleetReceiver> fleet, const std::string& location,
                 const std::string& command_id = "")
        : fleet_(fleet), location_(location),
          command_id_(command_id.empty() ? generateCommandId() : command_id) {}
    
    CommandResult execute() override {
        if (!fleet_) {
            return CommandResult(false, "Invalid fleet receiver");
        }
        
        fleet_->saveState();
        bool success = fleet_->defendLocation(location_);
        
        std::string message = success ?
            "Fleet is now defending " + location_ :
            "Fleet failed to establish defense at " + location_;
        
        CommandResult result(success, message);
        result.setData("location", location_);
        result.setData("fleet_id", fleet_->getFleetId());
        
        executed_ = success;
        return result;
    }
    
    CommandResult undo() override {
        if (!executed_ || !fleet_) {
            return CommandResult(false, "Cannot undo: defend command not executed");
        }
        
        fleet_->restoreState();
        executed_ = false;
        
        return CommandResult(true, "Defense order cancelled");
    }
    
    bool canUndo() const override { return executed_; }
    std::string getName() const override { return "Defend Location"; }
    std::string getCategory() const override { return "Defense"; }
    int getPriority() const override { return 7; }
    double getEstimatedExecutionTime() const override { return 150.0; } // 150ms
    std::string getId() const override { return command_id_; }
    
    std::unique_ptr<ICommand> clone() const override {
        return std::make_unique<DefendCommand>(fleet_, location_, command_id_ + "_clone");
    }

private:
    std::shared_ptr<FleetReceiver> fleet_;
    std::string location_;
    std::string command_id_;
    bool executed_ = false;
    
    std::string generateCommandId() const {
        return "defend_" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());
    }
};

/**
 * @brief Composite Command
 * 
 * Command that contains multiple sub-commands and executes them in sequence.
 * Supports undo by reversing the execution order.
 */
class CompositeCommand : public ICommand {
public:
    explicit CompositeCommand(const std::string& name, const std::string& command_id = "")
        : name_(name), command_id_(command_id.empty() ? generateCommandId() : command_id) {}
    
    /**
     * @brief Add sub-command to the composite
     * @param command Sub-command to add
     */
    void addCommand(std::unique_ptr<ICommand> command) {
        if (command) {
            commands_.push_back(std::move(command));
        }
    }
    
    /**
     * @brief Clear all sub-commands
     */
    void clearCommands() {
        commands_.clear();
        executed_commands_.clear();
    }
    
    CommandResult execute() override {
        executed_commands_.clear();
        
        for (auto& command : commands_) {
            CommandResult result = command->execute();
            executed_commands_.push_back(command.get());
            
            if (!result.success) {
                // If any command fails, undo all executed commands
                undoExecutedCommands();
                return CommandResult(false, "Composite command failed at: " + command->getName() + " - " + result.message);
            }
        }
        
        return CommandResult(true, "All " + std::to_string(commands_.size()) + " commands executed successfully");
    }
    
    CommandResult undo() override {
        if (executed_commands_.empty()) {
            return CommandResult(false, "No commands to undo");
        }
        
        return undoExecutedCommands();
    }
    
    bool canUndo() const override {
        return !executed_commands_.empty();
    }
    
    std::string getName() const override { return name_; }
    std::string getCategory() const override { return "Composite"; }
    int getPriority() const override { return 10; } // High priority for composite commands
    
    double getEstimatedExecutionTime() const override {
        double total_time = 0.0;
        for (const auto& command : commands_) {
            total_time += command->getEstimatedExecutionTime();
        }
        return total_time;
    }
    
    std::string getId() const override { return command_id_; }
    
    std::unique_ptr<ICommand> clone() const override {
        auto cloned = std::make_unique<CompositeCommand>(name_, command_id_ + "_clone");
        for (const auto& command : commands_) {
            cloned->addCommand(command->clone());
        }
        return cloned;
    }
    
    /**
     * @brief Get number of sub-commands
     * @return Number of sub-commands
     */
    size_t getCommandCount() const { return commands_.size(); }

private:
    std::string name_;
    std::string command_id_;
    std::vector<std::unique_ptr<ICommand>> commands_;
    std::vector<ICommand*> executed_commands_;
    
    CommandResult undoExecutedCommands() {
        int undone_count = 0;
        
        // Undo in reverse order
        for (auto it = executed_commands_.rbegin(); it != executed_commands_.rend(); ++it) {
            if ((*it)->canUndo()) {
                CommandResult undo_result = (*it)->undo();
                if (undo_result.success) {
                    undone_count++;
                }
            }
        }
        
        executed_commands_.clear();
        
        return CommandResult(true, "Undone " + std::to_string(undone_count) + " commands");
    }
    
    std::string generateCommandId() const {
        return "composite_" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());
    }
};

/**
 * @brief Macro Command - Predefined sequence of commands
 * 
 * Special type of composite command with predefined command sequences
 * for common operations.
 */
class MacroCommand : public CompositeCommand {
public:
    enum class MacroType {
        FULL_ATTACK,      // Move + Attack + Defend
        STRATEGIC_RETREAT, // Attack + Move to safety + Repair
        PATROL_ROUTE,     // Move through multiple waypoints
        EMERGENCY_RESPONSE // Move + Defend + Send distress signal
    };
    
    MacroCommand(MacroType type, std::shared_ptr<FleetReceiver> fleet, 
                const std::unordered_map<std::string, std::string>& parameters)
        : CompositeCommand(getMacroName(type)), macro_type_(type) {
        
        buildMacro(fleet, parameters);
    }
    
    std::string getCategory() const override { return "Macro"; }
    
    MacroType getMacroType() const { return macro_type_; }

private:
    MacroType macro_type_;
    
    std::string getMacroName(MacroType type) {
        switch (type) {
            case MacroType::FULL_ATTACK: return "Full Attack Sequence";
            case MacroType::STRATEGIC_RETREAT: return "Strategic Retreat";
            case MacroType::PATROL_ROUTE: return "Patrol Route";
            case MacroType::EMERGENCY_RESPONSE: return "Emergency Response";
            default: return "Unknown Macro";
        }
    }
    
    void buildMacro(std::shared_ptr<FleetReceiver> fleet, 
                   const std::unordered_map<std::string, std::string>& parameters) {
        
        switch (macro_type_) {
            case MacroType::FULL_ATTACK: {
                auto target_loc = parameters.find("target_location");
                auto target = parameters.find("target");
                auto defend_loc = parameters.find("defend_location");
                
                if (target_loc != parameters.end()) {
                    addCommand(std::make_unique<MoveFleetCommand>(fleet, target_loc->second));
                }
                if (target != parameters.end()) {
                    addCommand(std::make_unique<AttackCommand>(fleet, target->second));
                }
                if (defend_loc != parameters.end()) {
                    addCommand(std::make_unique<DefendCommand>(fleet, defend_loc->second));
                }
                break;
            }
            
            case MacroType::STRATEGIC_RETREAT: {
                auto target = parameters.find("target");
                auto safe_location = parameters.find("safe_location");
                
                if (target != parameters.end()) {
                    addCommand(std::make_unique<AttackCommand>(fleet, target->second));
                }
                if (safe_location != parameters.end()) {
                    addCommand(std::make_unique<MoveFleetCommand>(fleet, safe_location->second));
                    // Note: In a real implementation, we would add a RepairCommand here
                }
                break;
            }
            
            case MacroType::PATROL_ROUTE: {
                auto waypoints = parameters.find("waypoints");
                if (waypoints != parameters.end()) {
                    // Parse waypoints (simplified - assumes comma-separated values)
                    std::stringstream ss(waypoints->second);
                    std::string waypoint;
                    while (std::getline(ss, waypoint, ',')) {
                        addCommand(std::make_unique<MoveFleetCommand>(fleet, waypoint));
                    }
                }
                break;
            }
            
            case MacroType::EMERGENCY_RESPONSE: {
                auto emergency_location = parameters.find("emergency_location");
                if (emergency_location != parameters.end()) {
                    addCommand(std::make_unique<MoveFleetCommand>(fleet, emergency_location->second));
                    addCommand(std::make_unique<DefendCommand>(fleet, emergency_location->second));
                    // Note: In a real implementation, we would add a DistressSignalCommand here
                }
                break;
            }
        }
    }
};

/**
 * @brief Command Invoker - Manages command execution and history
 * 
 * Central command dispatcher that handles execution, undo/redo,
 * queuing, and command history management.
 */
class CommandInvoker {
public:
    /**
     * @brief Execute a command immediately
     * @param command Command to execute
     * @return Command execution result
     */
    CommandResult executeCommand(std::unique_ptr<ICommand> command) {
        if (!command) {
            return CommandResult(false, "Invalid command");
        }
        
        if (!command->isValid()) {
            return CommandResult(false, "Command is not valid for execution");
        }
        
        auto start_time = std::chrono::steady_clock::now();
        CommandResult result = command->execute();
        auto end_time = std::chrono::steady_clock::now();
        
        auto execution_time = std::chrono::duration<double, std::milli>(end_time - start_time).count();
        result.setData("actual_execution_time", execution_time);
        
        if (result.success) {
            // Add to history for potential undo
            std::lock_guard<std::mutex> lock(history_mutex_);
            command_history_.push(std::move(command));
            
            // Limit history size
            while (command_history_.size() > max_history_size_) {
                command_history_.pop();
            }
            
            // Clear redo stack when new command is executed
            while (!redo_stack_.empty()) {
                redo_stack_.pop();
            }
        }
        
        return result;
    }
    
    /**
     * @brief Queue a command for later execution
     * @param command Command to queue
     * @param priority Optional priority override
     */
    void queueCommand(std::unique_ptr<ICommand> command, std::optional<int> priority = std::nullopt) {
        if (!command) return;
        
        int cmd_priority = priority.value_or(command->getPriority());
        
        std::lock_guard<std::mutex> lock(queue_mutex_);
        command_queue_.push({std::move(command), cmd_priority});
    }
    
    /**
     * @brief Execute all queued commands
     * @return Vector of execution results
     */
    std::vector<CommandResult> executeQueuedCommands() {
        std::vector<CommandResult> results;
        
        std::lock_guard<std::mutex> lock(queue_mutex_);
        
        while (!command_queue_.empty()) {
            auto queued_command = std::move(const_cast<QueuedCommand&>(command_queue_.top()));
            command_queue_.pop();
            
            // Unlock during execution to avoid deadlock
            queue_mutex_.unlock();
            CommandResult result = executeCommand(std::move(queued_command.command));
            results.push_back(result);
            queue_mutex_.lock();
        }
        
        return results;
    }
    
    /**
     * @brief Undo the last executed command
     * @return Undo result
     */
    CommandResult undoLastCommand() {
        std::lock_guard<std::mutex> lock(history_mutex_);
        
        if (command_history_.empty()) {
            return CommandResult(false, "No commands to undo");
        }
        
        auto command = std::move(command_history_.top());
        command_history_.pop();
        
        CommandResult result = command->undo();
        
        if (result.success) {
            redo_stack_.push(std::move(command));
        } else {
            // Put command back if undo failed
            command_history_.push(std::move(command));
        }
        
        return result;
    }
    
    /**
     * @brief Redo the last undone command
     * @return Redo result
     */
    CommandResult redoLastCommand() {
        std::lock_guard<std::mutex> lock(history_mutex_);
        
        if (redo_stack_.empty()) {
            return CommandResult(false, "No commands to redo");
        }
        
        auto command = std::move(redo_stack_.top());
        redo_stack_.pop();
        
        CommandResult result = command->execute();
        
        if (result.success) {
            command_history_.push(std::move(command));
        } else {
            // Put command back if redo failed
            redo_stack_.push(std::move(command));
        }
        
        return result;
    }
    
    /**
     * @brief Get command history count
     * @return Number of commands in history
     */
    size_t getHistorySize() const {
        std::lock_guard<std::mutex> lock(history_mutex_);
        return command_history_.size();
    }
    
    /**
     * @brief Get queued commands count
     * @return Number of queued commands
     */
    size_t getQueueSize() const {
        std::lock_guard<std::mutex> lock(queue_mutex_);
        return command_queue_.size();
    }
    
    /**
     * @brief Clear command history
     */
    void clearHistory() {
        std::lock_guard<std::mutex> lock(history_mutex_);
        while (!command_history_.empty()) command_history_.pop();
        while (!redo_stack_.empty()) redo_stack_.pop();
    }
    
    /**
     * @brief Clear command queue
     */
    void clearQueue() {
        std::lock_guard<std::mutex> lock(queue_mutex_);
        while (!command_queue_.empty()) command_queue_.pop();
    }
    
    /**
     * @brief Set maximum history size
     * @param max_size Maximum number of commands to keep in history
     */
    void setMaxHistorySize(size_t max_size) {
        std::lock_guard<std::mutex> lock(history_mutex_);
        max_history_size_ = max_size;
        
        while (command_history_.size() > max_history_size_) {
            command_history_.pop();
        }
    }

private:
    struct QueuedCommand {
        std::unique_ptr<ICommand> command;
        int priority;
        
        QueuedCommand(std::unique_ptr<ICommand> cmd, int prio)
            : command(std::move(cmd)), priority(prio) {}
        
        // For priority queue (higher priority = higher precedence)
        bool operator<(const QueuedCommand& other) const {
            return priority < other.priority;
        }
    };
    
    mutable std::mutex history_mutex_;
    mutable std::mutex queue_mutex_;
    
    std::stack<std::unique_ptr<ICommand>> command_history_;
    std::stack<std::unique_ptr<ICommand>> redo_stack_;
    std::priority_queue<QueuedCommand> command_queue_;
    
    size_t max_history_size_ = 50;
};

/**
 * @brief Command Factory - Creates different types of commands
 * 
 * Factory for creating standardized commands with proper configuration.
 */
class CommandFactory {
public:
    /**
     * @brief Create move fleet command
     */
    static std::unique_ptr<ICommand> createMoveCommand(
        std::shared_ptr<FleetReceiver> fleet,
        const std::string& destination
    ) {
        return std::make_unique<MoveFleetCommand>(fleet, destination);
    }
    
    /**
     * @brief Create attack command
     */
    static std::unique_ptr<ICommand> createAttackCommand(
        std::shared_ptr<FleetReceiver> fleet,
        const std::string& target
    ) {
        return std::make_unique<AttackCommand>(fleet, target);
    }
    
    /**
     * @brief Create defend command
     */
    static std::unique_ptr<ICommand> createDefendCommand(
        std::shared_ptr<FleetReceiver> fleet,
        const std::string& location
    ) {
        return std::make_unique<DefendCommand>(fleet, location);
    }
    
    /**
     * @brief Create composite command
     */
    static std::unique_ptr<CompositeCommand> createCompositeCommand(
        const std::string& name,
        const std::vector<std::unique_ptr<ICommand>>& commands
    ) {
        auto composite = std::make_unique<CompositeCommand>(name);
        for (const auto& cmd : commands) {
            if (cmd) {
                composite->addCommand(cmd->clone());
            }
        }
        return composite;
    }
    
    /**
     * @brief Create macro command
     */
    static std::unique_ptr<MacroCommand> createMacroCommand(
        MacroCommand::MacroType type,
        std::shared_ptr<FleetReceiver> fleet,
        const std::unordered_map<std::string, std::string>& parameters
    ) {
        return std::make_unique<MacroCommand>(type, fleet, parameters);
    }
    
    /**
     * @brief Create command from configuration
     */
    static std::unique_ptr<ICommand> createFromConfig(
        const std::string& command_type,
        std::shared_ptr<FleetReceiver> fleet,
        const std::unordered_map<std::string, std::string>& config
    ) {
        if (command_type == "move") {
            auto dest = config.find("destination");
            if (dest != config.end()) {
                return createMoveCommand(fleet, dest->second);
            }
        } else if (command_type == "attack") {
            auto target = config.find("target");
            if (target != config.end()) {
                return createAttackCommand(fleet, target->second);
            }
        } else if (command_type == "defend") {
            auto location = config.find("location");
            if (location != config.end()) {
                return createDefendCommand(fleet, location);
            }
        }
        
        return nullptr;
    }
};

/**
 * @brief Command Scheduler - Schedules commands for future execution
 * 
 * Manages timed execution of commands and recurring command sequences.
 */
class CommandScheduler {
public:
    struct ScheduledCommand {
        std::unique_ptr<ICommand> command;
        std::chrono::system_clock::time_point execution_time;
        bool recurring;
        std::chrono::milliseconds interval;
        std::string schedule_id;
        
        ScheduledCommand(std::unique_ptr<ICommand> cmd, 
                        std::chrono::system_clock::time_point exec_time,
                        bool is_recurring = false,
                        std::chrono::milliseconds recur_interval = std::chrono::milliseconds::zero())
            : command(std::move(cmd)), execution_time(exec_time), 
              recurring(is_recurring), interval(recur_interval) {
            
            schedule_id = "schedule_" + std::to_string(
                std::chrono::steady_clock::now().time_since_epoch().count());
        }
    };
    
    /**
     * @brief Schedule command for future execution
     * @param command Command to schedule
     * @param delay Delay before execution
     * @param recurring Whether to repeat the command
     * @param interval Interval between repetitions (if recurring)
     * @return Schedule ID for management
     */
    std::string scheduleCommand(
        std::unique_ptr<ICommand> command,
        std::chrono::milliseconds delay,
        bool recurring = false,
        std::chrono::milliseconds interval = std::chrono::milliseconds::zero()
    ) {
        auto execution_time = std::chrono::system_clock::now() + delay;
        
        std::lock_guard<std::mutex> lock(schedule_mutex_);
        auto scheduled_cmd = std::make_unique<ScheduledCommand>(
            std::move(command), execution_time, recurring, interval);
        
        std::string schedule_id = scheduled_cmd->schedule_id;
        scheduled_commands_.push_back(std::move(scheduled_cmd));
        
        return schedule_id;
    }
    
    /**
     * @brief Cancel scheduled command
     * @param schedule_id Schedule identifier
     * @return true if command was found and cancelled
     */
    bool cancelScheduledCommand(const std::string& schedule_id) {
        std::lock_guard<std::mutex> lock(schedule_mutex_);
        
        auto it = std::find_if(scheduled_commands_.begin(), scheduled_commands_.end(),
            [&schedule_id](const std::unique_ptr<ScheduledCommand>& cmd) {
                return cmd->schedule_id == schedule_id;
            });
        
        if (it != scheduled_commands_.end()) {
            scheduled_commands_.erase(it);
            return true;
        }
        return false;
    }
    
    /**
     * @brief Process scheduled commands (should be called regularly)
     * @param invoker Command invoker to execute ready commands
     * @return Number of commands executed
     */
    int processScheduledCommands(CommandInvoker& invoker) {
        auto now = std::chrono::system_clock::now();
        int executed_count = 0;
        
        std::lock_guard<std::mutex> lock(schedule_mutex_);
        
        auto it = scheduled_commands_.begin();
        while (it != scheduled_commands_.end()) {
            if ((*it)->execution_time <= now) {
                // Execute the command
                invoker.executeCommand((*it)->command->clone());
                executed_count++;
                
                if ((*it)->recurring && (*it)->interval > std::chrono::milliseconds::zero()) {
                    // Reschedule for next execution
                    (*it)->execution_time = now + (*it)->interval;
                    ++it;
                } else {
                    // Remove one-time command
                    it = scheduled_commands_.erase(it);
                }
            } else {
                ++it;
            }
        }
        
        return executed_count;
    }
    
    /**
     * @brief Get count of scheduled commands
     * @return Number of scheduled commands
     */
    size_t getScheduledCount() const {
        std::lock_guard<std::mutex> lock(schedule_mutex_);
        return scheduled_commands_.size();
    }
    
    /**
     * @brief Clear all scheduled commands
     */
    void clearSchedule() {
        std::lock_guard<std::mutex> lock(schedule_mutex_);
        scheduled_commands_.clear();
    }

private:
    mutable std::mutex schedule_mutex_;
    std::vector<std::unique_ptr<ScheduledCommand>> scheduled_commands_;
};

/**
 * @brief Command Statistics - Tracks command execution metrics
 * 
 * Collects and analyzes statistics about command execution for
 * performance monitoring and system optimization.
 */
class CommandStatistics {
public:
    struct ExecutionStats {
        size_t total_executions = 0;
        size_t successful_executions = 0;
        size_t failed_executions = 0;
        double average_execution_time = 0.0;
        double total_execution_time = 0.0;
        std::chrono::system_clock::time_point first_execution;
        std::chrono::system_clock::time_point last_execution;
        
        double getSuccessRate() const {
            return total_executions > 0 ? 
                static_cast<double>(successful_executions) / total_executions * 100.0 : 0.0;
        }
    };
    
    /**
     * @brief Record command execution
     * @param command_name Name of the executed command
     * @param success Whether execution was successful
     * @param execution_time Execution time in milliseconds
     */
    void recordExecution(const std::string& command_name, bool success, double execution_time) {
        std::lock_guard<std::mutex> lock(stats_mutex_);
        
        auto& stats = command_stats_[command_name];
        
        if (stats.total_executions == 0) {
            stats.first_execution = std::chrono::system_clock::now();
        }
        
        stats.total_executions++;
        stats.total_execution_time += execution_time;
        stats.average_execution_time = stats.total_execution_time / stats.total_executions;
        stats.last_execution = std::chrono::system_clock::now();
        
        if (success) {
            stats.successful_executions++;
        } else {
            stats.failed_executions++;
        }
    }
    
    /**
     * @brief Get statistics for a specific command
     * @param command_name Command name
     * @return Optional execution statistics
     */
    std::optional<ExecutionStats> getCommandStats(const std::string& command_name) const {
        std::lock_guard<std::mutex> lock(stats_mutex_);
        
        auto it = command_stats_.find(command_name);
        return (it != command_stats_.end()) ? std::make_optional(it->second) : std::nullopt;
    }
    
    /**
     * @brief Get all command names with statistics
     * @return Vector of command names
     */
    std::vector<std::string> getTrackedCommands() const {
        std::lock_guard<std::mutex> lock(stats_mutex_);
        
        std::vector<std::string> command_names;
        command_names.reserve(command_stats_.size());
        
        for (const auto& [name, stats] : command_stats_) {
            command_names.push_back(name);
        }
        
        return command_names;
    }
    
    /**
     * @brief Generate statistics report
     * @return Formatted statistics report
     */
    std::string generateReport() const {
        std::lock_guard<std::mutex> lock(stats_mutex_);
        
        std::ostringstream report;
        report << std::fixed << std::setprecision(2);
        report << "=== Command Execution Statistics ===\n\n";
        
        for (const auto& [name, stats] : command_stats_) {
            report << "Command: " << name << "\n";
            report << "  Total Executions: " << stats.total_executions << "\n";
            report << "  Success Rate: " << stats.getSuccessRate() << "%\n";
            report << "  Average Execution Time: " << stats.average_execution_time << "ms\n";
            report << "  Total Execution Time: " << stats.total_execution_time << "ms\n";
            report << "\n";
        }
        
        return report.str();
    }
    
    /**
     * @brief Reset all statistics
     */
    void resetStatistics() {
        std::lock_guard<std::mutex> lock(stats_mutex_);
        command_stats_.clear();
    }

private:
    mutable std::mutex stats_mutex_;
    std::unordered_map<std::string, ExecutionStats> command_stats_;
};

} // namespace CppVerseHub::Patterns