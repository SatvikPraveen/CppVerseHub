// File: src/patterns/Adapter.cpp
// CppVerseHub - Adapter Pattern Implementation for Legacy System Interfacing

#include "Adapter.hpp"
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <cstring>
#include <cstdlib>
#include <chrono>

namespace CppVerseHub::Patterns {

// =============================================================================
// Legacy Fleet System Implementation
// =============================================================================

int LegacyFleetSystem::GetFleetInfo(const char* fleet_name, OldFleetData* data) {
    if (!fleet_name || !data) return -1;
    
    // Initialize legacy data if not done
    if (legacy_fleets_.empty()) {
        initializeLegacyData();
    }
    
    for (const auto& fleet : legacy_fleets_) {
        if (strcmp(fleet.fleet_name, fleet_name) == 0) {
            *data = fleet;
            return 0;
        }
    }
    return -2; // Fleet not found
}

int LegacyFleetSystem::GetAllFleetNames(char fleet_names[][64], int max_fleets) {
    if (!fleet_names || max_fleets <= 0) return 0;
    
    if (legacy_fleets_.empty()) {
        initializeLegacyData();
    }
    
    int count = 0;
    for (const auto& fleet : legacy_fleets_) {
        if (count >= max_fleets) break;
        strncpy(fleet_names[count], fleet.fleet_name, 63);
        fleet_names[count][63] = '\0';
        count++;
    }
    return count;
}

int LegacyFleetSystem::SendMissionToFleet(const char* fleet_name, const OldMissionData* mission) {
    if (!fleet_name || !mission) return -1;
    
    // Find fleet and update mission status
    for (auto& fleet : legacy_fleets_) {
        if (strcmp(fleet.fleet_name, fleet_name) == 0) {
            fleet.mission_status = 1; // Active
            fleet.last_contact_timestamp = time(nullptr);
            return 0;
        }
    }
    return -2; // Fleet not found
}

int LegacyFleetSystem::CancelFleetMission(const char* fleet_name) {
    if (!fleet_name) return -1;
    
    for (auto& fleet : legacy_fleets_) {
        if (strcmp(fleet.fleet_name, fleet_name) == 0) {
            fleet.mission_status = 2; // Returning
            fleet.last_contact_timestamp = time(nullptr);
            return 0;
        }
    }
    return -2; // Fleet not found
}

int LegacyFleetSystem::UpdateFleetSetting(const char* fleet_name, const char* setting_name, const char* setting_value) {
    if (!fleet_name || !setting_name || !setting_value) return -1;
    
    for (auto& fleet : legacy_fleets_) {
        if (strcmp(fleet.fleet_name, fleet_name) == 0) {
            // Simulate updating settings (simplified)
            fleet.last_contact_timestamp = time(nullptr);
            return 0;
        }
    }
    return -2; // Fleet not found
}

int LegacyFleetSystem::GetAvailableFleetsForMission(int mission_type_code, char available_fleets[][64], int max_fleets) {
    if (!available_fleets || max_fleets <= 0) return 0;
    
    if (legacy_fleets_.empty()) {
        initializeLegacyData();
    }
    
    int count = 0;
    for (const auto& fleet : legacy_fleets_) {
        if (count >= max_fleets) break;
        if (fleet.mission_status == 0 && fleet.damage_level < 0.5f) { // Idle and not heavily damaged
            strncpy(available_fleets[count], fleet.fleet_name, 63);
            available_fleets[count][63] = '\0';
            count++;
        }
    }
    return count;
}

float LegacyFleetSystem::CalculateMissionDuration(const char* fleet_name, const OldMissionData* mission) {
    if (!fleet_name || !mission) return -1.0f;
    
    for (const auto& fleet : legacy_fleets_) {
        if (strcmp(fleet.fleet_name, fleet_name) == 0) {
            // Simple calculation based on mission type and urgency
            float base_time = 10.0f;
            if (mission->mission_type_code == 1) base_time = 8.0f;  // Exploration
            else if (mission->mission_type_code == 2) base_time = 6.0f; // Combat
            else if (mission->mission_type_code == 3) base_time = 12.0f; // Transport
            
            return base_time / mission->urgency_factor;
        }
    }
    return -1.0f; // Fleet not found
}

void LegacyFleetSystem::initializeLegacyData() {
    // Create some sample legacy fleet data
    OldFleetData fleet1 = {};
    strncpy(fleet1.fleet_name, "ALPHA_SQUADRON", 63);
    fleet1.vessel_count = 5;
    fleet1.fuel_percent = 0.85f;
    fleet1.damage_level = 0.1f;
    fleet1.current_sector_x = 10;
    fleet1.current_sector_y = 20;
    fleet1.current_sector_z = 5;
    fleet1.mission_status = 0;
    fleet1.last_contact_timestamp = time(nullptr);
    
    OldFleetData fleet2 = {};
    strncpy(fleet2.fleet_name, "BRAVO_FLEET", 63);
    fleet2.vessel_count = 8;
    fleet2.fuel_percent = 0.92f;
    fleet2.damage_level = 0.05f;
    fleet2.current_sector_x = 15;
    fleet2.current_sector_y = 25;
    fleet2.current_sector_z = 8;
    fleet2.mission_status = 1;
    fleet2.last_contact_timestamp = time(nullptr) - 3600;
    
    OldFleetData fleet3 = {};
    strncpy(fleet3.fleet_name, "CHARLIE_WING", 63);
    fleet3.vessel_count = 3;
    fleet3.fuel_percent = 0.45f;
    fleet3.damage_level = 0.3f;
    fleet3.current_sector_x = 5;
    fleet3.current_sector_y = 15;
    fleet3.current_sector_z = 3;
    fleet3.mission_status = 3;
    fleet3.last_contact_timestamp = time(nullptr) - 1800;
    
    legacy_fleets_ = {fleet1, fleet2, fleet3};
}

// =============================================================================
// Fleet Management Adapter Implementation
// =============================================================================

FleetManagementAdapter::FleetManagementAdapter(std::unique_ptr<LegacyFleetSystem> legacy_system)
    : legacy_system_(std::move(legacy_system)) {}

std::vector<IModernFleetManager::FleetStatus> FleetManagementAdapter::getAllFleetStatuses() const {
    std::vector<FleetStatus> statuses;
    
    char fleet_names[100][64];
    int fleet_count = legacy_system_->GetAllFleetNames(fleet_names, 100);
    
    for (int i = 0; i < fleet_count; ++i) {
        LegacyFleetSystem::OldFleetData legacy_data;
        if (legacy_system_->GetFleetInfo(fleet_names[i], &legacy_data) == 0) {
            statuses.push_back(convertLegacyToModern(legacy_data));
        }
    }
    
    return statuses;
}

std::optional<IModernFleetManager::FleetStatus> FleetManagementAdapter::getFleetStatus(const std::string& fleet_id) const {
    LegacyFleetSystem::OldFleetData legacy_data;
    if (legacy_system_->GetFleetInfo(fleet_id.c_str(), &legacy_data) == 0) {
        return convertLegacyToModern(legacy_data);
    }
    return std::nullopt;
}

bool FleetManagementAdapter::issueMissionOrder(const std::string& fleet_id, const MissionOrder& mission) {
    LegacyFleetSystem::OldMissionData legacy_mission = convertModernToLegacy(mission);
    return legacy_system_->SendMissionToFleet(fleet_id.c_str(), &legacy_mission) == 0;
}

bool FleetManagementAdapter::recallFleet(const std::string& fleet_id) {
    return legacy_system_->CancelFleetMission(fleet_id.c_str()) == 0;
}

bool FleetManagementAdapter::updateFleetConfiguration(const std::string& fleet_id,
                                                     const std::unordered_map<std::string, std::string>& config_updates) {
    bool all_success = true;
    for (const auto& [key, value] : config_updates) {
        if (legacy_system_->UpdateFleetSetting(fleet_id.c_str(), key.c_str(), value.c_str()) != 0) {
            all_success = false;
        }
    }
    return all_success;
}

std::vector<std::string> FleetManagementAdapter::getAvailableFleets(const std::string& mission_type) const {
    std::vector<std::string> available;
    
    int mission_type_code = convertModernMissionType(mission_type);
    char fleet_names[50][64];
    int count = legacy_system_->GetAvailableFleetsForMission(mission_type_code, fleet_names, 50);
    
    for (int i = 0; i < count; ++i) {
        available.emplace_back(fleet_names[i]);
    }
    
    return available;
}

double FleetManagementAdapter::getEstimatedMissionTime(const std::string& fleet_id, const MissionOrder& mission) const {
    LegacyFleetSystem::OldMissionData legacy_mission = convertModernToLegacy(mission);
    float legacy_time = legacy_system_->CalculateMissionDuration(fleet_id.c_str(), &legacy_mission);
    return legacy_time > 0 ? static_cast<double>(legacy_time) : 0.0;
}

IModernFleetManager::FleetStatus FleetManagementAdapter::convertLegacyToModern(const LegacyFleetSystem::OldFleetData& legacy_data) const {
    FleetStatus status;
    status.fleet_id = legacy_data.fleet_name;
    status.location = formatSectorLocation(legacy_data.current_sector_x, legacy_data.current_sector_y, legacy_data.current_sector_z);
    status.ship_count = legacy_data.vessel_count;
    status.fuel_level = static_cast<double>(legacy_data.fuel_percent * 100.0f);
    status.health_percentage = static_cast<double>((1.0f - legacy_data.damage_level) * 100.0f);
    status.status = convertLegacyStatus(legacy_data.mission_status);
    status.last_update = convertLegacyTimestamp(legacy_data.last_contact_timestamp);
    
    return status;
}

LegacyFleetSystem::OldMissionData FleetManagementAdapter::convertModernToLegacy(const MissionOrder& mission) const {
    LegacyFleetSystem::OldMissionData legacy_mission = {};
    
    strncpy(legacy_mission.target_sector, mission.destination.c_str(), 31);
    legacy_mission.target_sector[31] = '\0';
    
    legacy_mission.mission_type_code = convertModernMissionType(mission.mission_type);
    legacy_mission.urgency_factor = static_cast<float>(mission.priority_level);
    
    // Combine parameters into special instructions
    std::ostringstream instructions;
    for (const auto& [key, value] : mission.parameters) {
        instructions << key << ":" << value << ";";
    }
    std::string inst_str = instructions.str();
    strncpy(legacy_mission.special_instructions, inst_str.c_str(), 255);
    legacy_mission.special_instructions[255] = '\0';
    
    return legacy_mission;
}

std::string FleetManagementAdapter::convertLegacyStatus(int legacy_status) const {
    switch (legacy_status) {
        case 0: return "docked";
        case 1: return "active";
        case 2: return "in_transit";
        case 3: return "maintenance";
        default: return "unknown";
    }
}

int FleetManagementAdapter::convertModernMissionType(const std::string& modern_type) const {
    if (modern_type == "exploration") return 1;
    if (modern_type == "combat") return 2;
    if (modern_type == "transport") return 3;
    return 1; // Default to exploration
}

std::string FleetManagementAdapter::convertLegacyMissionType(int legacy_type) const {
    switch (legacy_type) {
        case 1: return "exploration";
        case 2: return "combat";
        case 3: return "transport";
        default: return "unknown";
    }
}

std::string FleetManagementAdapter::formatSectorLocation(int x, int y, int z) const {
    return "Sector[" + std::to_string(x) + "," + std::to_string(y) + "," + std::to_string(z) + "]";
}

std::chrono::system_clock::time_point FleetManagementAdapter::convertLegacyTimestamp(long timestamp) const {
    return std::chrono::system_clock::from_time_t(static_cast<time_t>(timestamp));
}

// =============================================================================
// Legacy Communication System Implementation
// =============================================================================

void LegacyCommunicationSystem::initializeLegacyComm() {
    next_message_id_ = 1000;
    next_link_id_ = 100;
    
    // Initialize with some sample data
    CommLink link1 = {};
    link1.link_id = next_link_id_++;
    strncpy(link1.link_name, "COMMAND_CHANNEL", 63);
    strncpy(link1.station_list[0], "HQ_STATION", 31);
    strncpy(link1.station_list[1], "ALPHA_SQUADRON", 31);
    strncpy(link1.station_list[2], "BRAVO_FLEET", 31);
    link1.station_count = 3;
    link1.active_flag = 1;
    link1.signal_quality = 0.95f;
    
    active_links_.push_back(link1);
}

int LegacyCommunicationSystem::TransmitMessage(const OldMessage* msg) {
    if (!msg) return -1;
    
    // Add message to queue (simplified implementation)
    OldMessage new_msg = *msg;
    new_msg.msg_id = next_message_id_++;
    new_msg.send_time = time(nullptr);
    message_queue_.push_back(new_msg);
    
    return 0;
}

int LegacyCommunicationSystem::CheckIncomingMessages(const char* station_name, OldMessage* messages, int max_messages) {
    if (!station_name || !messages || max_messages <= 0) return 0;
    
    int count = 0;
    auto it = message_queue_.begin();
    while (it != message_queue_.end() && count < max_messages) {
        if (strcmp(it->to_station, station_name) == 0) {
            messages[count++] = *it;
            it = message_queue_.erase(it);
        } else {
            ++it;
        }
    }
    
    return count;
}

int LegacyCommunicationSystem::EstablishCommLink(const CommLink* link) {
    if (!link) return -1;
    
    if (active_links_.empty()) {
        initializeLegacyComm();
    }
    
    CommLink new_link = *link;
    new_link.link_id = next_link_id_++;
    active_links_.push_back(new_link);
    
    return new_link.link_id;
}

int LegacyCommunicationSystem::TerminateCommLink(int link_id) {
    auto it = std::find_if(active_links_.begin(), active_links_.end(),
        [link_id](const CommLink& link) { return link.link_id == link_id; });
    
    if (it != active_links_.end()) {
        active_links_.erase(it);
        return 0;
    }
    return -1;
}

int LegacyCommunicationSystem::GetCommLinkInfo(int link_id, CommLink* link) {
    if (!link) return -1;
    
    auto it = std::find_if(active_links_.begin(), active_links_.end(),
        [link_id](const CommLink& l) { return l.link_id == link_id; });
    
    if (it != active_links_.end()) {
        *link = *it;
        return 0;
    }
    return -1;
}

int LegacyCommunicationSystem::BroadcastOnLink(int link_id, const OldMessage* msg) {
    if (!msg) return 0;
    
    auto it = std::find_if(active_links_.begin(), active_links_.end(),
        [link_id](const CommLink& link) { return link.link_id == link_id; });
    
    if (it != active_links_.end()) {
        int recipients = 0;
        for (int i = 0; i < it->station_count; ++i) {
            if (strcmp(it->station_list[i], msg->from_station) != 0) { // Don't send to sender
                OldMessage broadcast_msg = *msg;
                strncpy(broadcast_msg.to_station, it->station_list[i], 31);
                broadcast_msg.msg_id = next_message_id_++;
                message_queue_.push_back(broadcast_msg);
                recipients++;
            }
        }
        return recipients;
    }
    return 0;
}

// =============================================================================
// Communication Adapter Implementation
// =============================================================================

CommunicationAdapter::CommunicationAdapter(std::unique_ptr<LegacyCommunicationSystem> legacy_comm)
    : legacy_comm_(std::move(legacy_comm)), next_channel_id_(1) {}

bool CommunicationAdapter::sendMessage(const Message& message) {
    LegacyCommunicationSystem::OldMessage legacy_msg = convertModernToLegacyMessage(message);
    return legacy_comm_->TransmitMessage(&legacy_msg) == 0;
}

std::vector<IModernCommunication::Message> CommunicationAdapter::receiveMessages(const std::string& recipient_id) {
    std::vector<Message> messages;
    
    LegacyCommunicationSystem::OldMessage legacy_messages[50];
    int count = legacy_comm_->CheckIncomingMessages(recipient_id.c_str(), legacy_messages, 50);
    
    for (int i = 0; i < count; ++i) {
        messages.push_back(convertLegacyToModernMessage(legacy_messages[i]));
    }
    
    return messages;
}

bool CommunicationAdapter::openChannel(const CommunicationChannel& channel) {
    LegacyCommunicationSystem::CommLink legacy_channel = convertModernToLegacyChannel(channel);
    int legacy_id = legacy_comm_->EstablishCommLink(&legacy_channel);
    
    if (legacy_id > 0) {
        channel_id_map_[channel.channel_id] = legacy_id;
        return true;
    }
    return false;
}

bool CommunicationAdapter::closeChannel(const std::string& channel_id) {
    auto it = channel_id_map_.find(channel_id);
    if (it != channel_id_map_.end()) {
        bool success = legacy_comm_->TerminateCommLink(it->second) == 0;
        channel_id_map_.erase(it);
        return success;
    }
    return false;
}

std::optional<IModernCommunication::CommunicationChannel> CommunicationAdapter::getChannelStatus(const std::string& channel_id) const {
    auto it = channel_id_map_.find(channel_id);
    if (it != channel_id_map_.end()) {
        LegacyCommunicationSystem::CommLink legacy_channel;
        if (legacy_comm_->GetCommLinkInfo(it->second, &legacy_channel) == 0) {
            return convertLegacyToModernChannel(legacy_channel);
        }
    }
    return std::nullopt;
}

int CommunicationAdapter::broadcastMessage(const std::string& channel_id, const Message& message) {
    auto it = channel_id_map_.find(channel_id);
    if (it != channel_id_map_.end()) {
        LegacyCommunicationSystem::OldMessage legacy_msg = convertModernToLegacyMessage(message);
        return legacy_comm_->BroadcastOnLink(it->second, &legacy_msg);
    }
    return 0;
}

// Conversion helper methods for CommunicationAdapter would go here...
// (Implementation details for convertModernToLegacyMessage, etc.)

// =============================================================================
// Adapter Performance Monitor Implementation
// =============================================================================

void AdapterPerformanceMonitor::recordConversion(const std::string& operation_name, double conversion_time, bool success) {
    std::lock_guard<std::mutex> lock(metrics_mutex_);
    
    auto& metrics = metrics_[operation_name];
    metrics.total_calls++;
    metrics.total_conversion_time += conversion_time;
    metrics.average_conversion_time = metrics.total_conversion_time / metrics.total_calls;
    
    if (!success) {
        metrics.failed_conversions++;
    }
    
    if (metrics.total_calls == 1) {
        metrics.last_reset = std::chrono::system_clock::now();
    }
}

std::optional<AdapterPerformanceMonitor::PerformanceMetrics> AdapterPerformanceMonitor::getMetrics(const std::string& operation_name) const {
    std::lock_guard<std::mutex> lock(metrics_mutex_);
    
    auto it = metrics_.find(operation_name);
    if (it != metrics_.end()) {
        return it->second;
    }
    return std::nullopt;
}

void AdapterPerformanceMonitor::resetMetrics() {
    std::lock_guard<std::mutex> lock(metrics_mutex_);
    metrics_.clear();
}

std::string AdapterPerformanceMonitor::generatePerformanceReport() const {
    std::lock_guard<std::mutex> lock(metrics_mutex_);
    
    std::ostringstream report;
    report << std::fixed << std::setprecision(4);
    report << "=== Adapter Performance Report ===\n";
    
    for (const auto& [operation, metrics] : metrics_) {
        report << "Operation: " << operation << "\n";
        report << "  Total Calls: " << metrics.total_calls << "\n";
        report << "  Average Conversion Time: " << metrics.average_conversion_time << "ms\n";
        report << "  Failed Conversions: " << metrics.failed_conversions << "\n";
        report << "  Success Rate: " << (100.0 * (metrics.total_calls - metrics.failed_conversions) / metrics.total_calls) << "%\n";
        report << "\n";
    }
    
    return report.str();
}

} // namespace CppVerseHub::Patterns