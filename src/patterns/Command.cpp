// File: src/patterns/Command.cpp
// CppVerseHub - Command Pattern Implementation for Fleet Operations

#include "Command.hpp"
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <thread>
#include <random>

namespace CppVerseHub::Patterns {

// =============================================================================
// Fleet Receiver Implementation
// =============================================================================

bool FleetReceiver::moveToLocation(const std::string& destination, double fuel_cost) {
    std::lock_guard<std::mutex> lock(fleet_mutex_);
    
    if (fleet_info_.fuel_level < fuel_cost) {
        return false; // Not enough fuel
    }
    
    if (fleet_info_.health < 50.0) {
        return false; // Fleet too damaged to move
    }
    
    // Simulate movement time
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    
    fleet_info_.current_location = destination;
    fleet_info_.fuel_level -= fuel_cost;
    fleet_info_.status = "In Transit";
    
    // Simulate arrival
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    fleet_info_.status = "Ready";
    
    return true;
}

bool FleetReceiver::attackTarget(const std::string& target, double damage_taken) {
    std::lock_guard<std::mutex> lock(fleet_mutex_);
    
    if (fleet_info_.health < 30.0) {
        return false; // Fleet too damaged to attack
    }
    
    if (fleet_info_.fuel_level < 5.0) {
        return false; // Not enough fuel for combat operations
    }
    
    // Simulate combat
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    fleet_info_.status = "In Combat";
    fleet_info_.fuel_level -= 5.0; // Combat fuel consumption
    fleet_info_.health -= damage_taken; // Take damage from combat
    
    // Ensure health doesn't go below 0
    fleet_info_.health = std::max(0.0, fleet_info_.health);
    
    // Random chance of losing ships in combat
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_real_distribution<> dis(0.0, 1.0);
    
    if (dis(gen) < 0.1 && fleet_info_.ship_count > 1) { // 10% chance of losing a ship
        fleet_info_.ship_count--;
    }
    
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    fleet_info_.status = "Ready";
    
    return true;
}

bool FleetReceiver::defendLocation(const std::string& location) {
    std::lock_guard<std::mutex> lock(fleet_mutex_);
    
    if (fleet_info_.health < 20.0) {
        return false; // Fleet too damaged to defend effectively
    }
    
    // Move to location if not already there
    if (fleet_info_.current_location != location) {
        fleet_info_.current_location = location;
        fleet_info_.fuel_level -= 3.0; // Small fuel cost for positioning
    }
    
    fleet_info_.status = "Defending " + location;
    
    // Simulate defensive preparations
    std::this_thread::sleep_for(std::chrono::milliseconds(75));
    
    return true;
}

bool FleetReceiver::refuelFleet(double fuel_amount) {
    std::lock_guard<std::mutex> lock(fleet_mutex_);
    
    fleet_info_.fuel_level = std::min(100.0, fleet_info_.fuel_level + fuel_amount);
    fleet_info_.status = "Refueling";
    
    std::this_thread::sleep_for(std::chrono::milliseconds(30));
    fleet_info_.status = "Ready";
    
    return true;
}

bool FleetReceiver::repairFleet(double repair_amount) {
    std::lock_guard<std::mutex> lock(fleet_mutex_);
    
    fleet_info_.health = std::min(100.0, fleet_info_.health + repair_amount);
    fleet_info_.status = "Under Repair";
    
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    fleet_info_.status = "Ready";
    
    return true;
}

bool FleetReceiver::deployShips(int ship_count, const std::string& location) {
    std::lock_guard<std::mutex> lock(fleet_mutex_);
    
    if (fleet_info_.ship_count < ship_count) {
        return false; // Not enough ships to deploy
    }
    
    fleet_info_.ship_count -= ship_count;
    fleet_info_.status = "Deploying Ships to " + location;
    
    std::this_thread::sleep_for(std::chrono::milliseconds(60));
    fleet_info_.status = "Ready";
    
    return true;
}

bool FleetReceiver::recallShips(int ship_count) {
    std::lock_guard<std::mutex> lock(fleet_mutex_);
    
    fleet_info_.ship_count += ship_count;
    fleet_info_.status = "Recalling Ships";
    
    std::this_thread::sleep_for(std::chrono::milliseconds(40));
    fleet_info_.status = "Ready";
    
    return true;
}

bool FleetReceiver::setFormation(const std::string& formation_type) {
    std::lock_guard<std::mutex> lock(fleet_mutex_);
    
    fleet_info_.status = "Formation: " + formation_type;
    
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    
    return true;
}

bool FleetReceiver::upgradeFleet(const std::string& upgrade_type) {
    std::lock_guard<std::mutex> lock(fleet_mutex_);
    
    fleet_info_.status = "Upgrading: " + upgrade_type;
    
    // Simulate upgrade time
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    
    // Upgrade effects (simplified)
    if (upgrade_type == "armor") {
        fleet_info_.health = std::min(100.0, fleet_info_.health + 10.0);
    } else if (upgrade_type == "engines") {
        // Engine upgrade reduces fuel consumption (not directly modeled here)
    } else if (upgrade_type == "weapons") {
        // Weapon upgrades increase combat effectiveness (not directly modeled here)
    }
    
    fleet_info_.status = "Ready";
    return true;
}

void FleetReceiver::saveState() {
    std::lock_guard<std::mutex> lock(fleet_mutex_);
    saved_state_ = fleet_info_;
}

void FleetReceiver::restoreState() {
    std::lock_guard<std::mutex> lock(fleet_mutex_);
    fleet_info_ = saved_state_;
}

} // namespace CppVerseHub::Patterns