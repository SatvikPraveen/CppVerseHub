// File: src/patterns/Adapter.hpp
// CppVerseHub - Adapter Pattern Implementation for Legacy System Interfacing

#pragma once

#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <chrono>
#include <optional>

namespace CppVerseHub::Patterns {

/**
 * @brief Modern Fleet Management Interface
 * 
 * Defines the modern interface that the CppVerseHub system expects
 * for fleet management operations.
 */
class IModernFleetManager {
public:
    virtual ~IModernFleetManager() = default;
    
    struct FleetStatus {
        std::string fleet_id;
        std::string location;
        int ship_count;
        double fuel_level;
        double health_percentage;
        std::string status; // "active", "docked", "in_transit", "maintenance"
        std::chrono::system_clock::time_point last_update;
    };
    
    struct MissionOrder {
        std::string mission_id;
        std::string mission_type; // "exploration", "combat", "transport"
        std::string destination;
        double priority_level;
        std::unordered_map<std::string, std::string> parameters;
    };
    
    /**
     * @brief Get status of all fleets
     * @return Vector of fleet statuses
     */
    virtual std::vector<FleetStatus> getAllFleetStatuses() const = 0;
    
    /**
     * @brief Get specific fleet status
     * @param fleet_id Fleet identifier
     * @return Optional fleet status
     */
    virtual std::optional<FleetStatus> getFleetStatus(const std::string& fleet_id) const = 0;
    
    /**
     * @brief Issue mission order to fleet
     * @param fleet_id Fleet identifier
     * @param mission Mission order details
     * @return true if order was successfully issued
     */
    virtual bool issueMissionOrder(const std::string& fleet_id, const MissionOrder& mission) = 0;
    
    /**
     * @brief Recall fleet from current mission
     * @param fleet_id Fleet identifier
     * @return true if recall was successful
     */
    virtual bool recallFleet(const std::string& fleet_id) = 0;
    
    /**
     * @brief Update fleet configuration
     * @param fleet_id Fleet identifier
     * @param config_updates Configuration updates
     * @return true if update was successful
     */
    virtual bool updateFleetConfiguration(const std::string& fleet_id,
                                          const std::unordered_map<std::string, std::string>& config_updates) = 0;
    
    /**
     * @brief Get available fleet capacity for new missions
     * @param mission_type Type of mission to check capacity for
     * @return Vector of fleet IDs with available capacity
     */
    virtual std::vector<std::string> getAvailableFleets(const std::string& mission_type) const = 0;
    
    /**
     * @brief Get estimated time for mission completion
     * @param fleet_id Fleet identifier
     * @param mission Mission details
     * @return Estimated completion time in hours
     */
    virtual double getEstimatedMissionTime(const std::string& fleet_id, const MissionOrder& mission) const = 0;
};

/**
 * @brief Legacy Fleet Management System
 * 
 * Represents an old fleet management system with different interface
 * and data structures that need to be adapted to work with modern code.
 */
class LegacyFleetSystem {
public:
    // Legacy data structures with different naming conventions
    struct OldFleetData {
        char fleet_name[64];
        int vessel_count;
        float fuel_percent;
        float damage_level; // 0.0 = no damage, 1.0 = destroyed
        int current_sector_x;
        int current_sector_y;
        int current_sector_z;
        int mission_status; // 0=idle, 1=active, 2=returning, 3=maintenance
        long last_contact_timestamp;
    };
    
    struct OldMissionData {
        char target_sector[32];
        int mission_type_code; // 1=explore, 2=attack, 3=transport
        float urgency_factor;
        char special_instructions[256];
    };
    
    /**
     * @brief Get legacy fleet data
     * @param fleet_name Legacy fleet name
     * @param data Output parameter for fleet data
     * @return 0 on success, error code otherwise
     */
    int GetFleetInfo(const char* fleet_name, OldFleetData* data);
    
    /**
     * @brief Get all fleet names
     * @param fleet_names Output buffer for fleet names
     * @param max_fleets Maximum number of fleets to retrieve
     * @return Number of fleets found
     */
    int GetAllFleetNames(char fleet_names[][64], int max_fleets);
    
    /**
     * @brief Send mission to fleet
     * @param fleet_name Fleet name
     * @param mission Mission data
     * @return 0 on success, error code otherwise
     */
    int SendMissionToFleet(const char* fleet_name, const OldMissionData* mission);
    
    /**
     * @brief Cancel current mission
     * @param fleet_name Fleet name
     * @return 0 on success, error code otherwise
     */
    int CancelFleetMission(const char* fleet_name);
    
    /**
     * @brief Update fleet settings
     * @param fleet_name Fleet name
     * @param setting_name Setting name
     * @param setting_value Setting value
     * @return 0 on success, error code otherwise
     */
    int UpdateFleetSetting(const char* fleet_name, const char* setting_name, const char* setting_value);
    
    /**
     * @brief Get fleet availability for mission type
     * @param mission_type_code Mission type code
     * @param available_fleets Output buffer
     * @param max_fleets Maximum fleets to return
     * @return Number of available fleets
     */
    int GetAvailableFleetsForMission(int mission_type_code, char available_fleets[][64], int max_fleets);
    
    /**
     * @brief Calculate mission duration
     * @param fleet_name Fleet name
     * @param mission Mission data
     * @return Mission duration in legacy time units (or -1 on error)
     */
    float CalculateMissionDuration(const char* fleet_name, const OldMissionData* mission);

private:
    // Simulated legacy data storage
    std::vector<OldFleetData> legacy_fleets_;
    
    void initializeLegacyData();
};

/**
 * @brief Fleet Management Adapter
 * 
 * Adapts the legacy fleet system to work with the modern interface.
 * Converts between different data formats and calling conventions.
 */
class FleetManagementAdapter : public IModernFleetManager {
public:
    /**
     * @brief Constructor with legacy system instance
     * @param legacy_system Pointer to legacy fleet system
     */
    explicit FleetManagementAdapter(std::unique_ptr<LegacyFleetSystem> legacy_system);
    
    virtual ~FleetManagementAdapter() = default;
    
    // IModernFleetManager interface implementation
    std::vector<FleetStatus> getAllFleetStatuses() const override;
    std::optional<FleetStatus> getFleetStatus(const std::string& fleet_id) const override;
    bool issueMissionOrder(const std::string& fleet_id, const MissionOrder& mission) override;
    bool recallFleet(const std::string& fleet_id) override;
    bool updateFleetConfiguration(const std::string& fleet_id,
                                 const std::unordered_map<std::string, std::string>& config_updates) override;
    std::vector<std::string> getAvailableFleets(const std::string& mission_type) const override;
    double getEstimatedMissionTime(const std::string& fleet_id, const MissionOrder& mission) const override;

private:
    std::unique_ptr<LegacyFleetSystem> legacy_system_;
    
    // Conversion helper methods
    FleetStatus convertLegacyToModern(const LegacyFleetSystem::OldFleetData& legacy_data) const;
    LegacyFleetSystem::OldMissionData convertModernToLegacy(const MissionOrder& mission) const;
    std::string convertLegacyStatus(int legacy_status) const;
    int convertModernMissionType(const std::string& modern_type) const;
    std::string convertLegacyMissionType(int legacy_type) const;
    std::string formatSectorLocation(int x, int y, int z) const;
    std::chrono::system_clock::time_point convertLegacyTimestamp(long timestamp) const;
};

/**
 * @brief Modern Communication Interface
 * 
 * Modern interface for interstellar communication systems.
 */
class IModernCommunication {
public:
    virtual ~IModernCommunication() = default;
    
    struct Message {
        std::string message_id;
        std::string sender_id;
        std::string recipient_id;
        std::string content;
        std::string priority; // "low", "normal", "high", "critical"
        std::chrono::system_clock::time_point timestamp;
        bool encrypted;
    };
    
    struct CommunicationChannel {
        std::string channel_id;
        std::string channel_name;
        std::vector<std::string> participants;
        bool is_active;
        double signal_strength;
    };
    
    /**
     * @brief Send message to recipient
     * @param message Message to send
     * @return true if message was sent successfully
     */
    virtual bool sendMessage(const Message& message) = 0;
    
    /**
     * @brief Receive pending messages for recipient
     * @param recipient_id Recipient identifier
     * @return Vector of received messages
     */
    virtual std::vector<Message> receiveMessages(const std::string& recipient_id) = 0;
    
    /**
     * @brief Open communication channel
     * @param channel Channel configuration
     * @return true if channel was opened successfully
     */
    virtual bool openChannel(const CommunicationChannel& channel) = 0;
    
    /**
     * @brief Close communication channel
     * @param channel_id Channel identifier
     * @return true if channel was closed successfully
     */
    virtual bool closeChannel(const std::string& channel_id) = 0;
    
    /**
     * @brief Get channel status
     * @param channel_id Channel identifier
     * @return Optional channel information
     */
    virtual std::optional<CommunicationChannel> getChannelStatus(const std::string& channel_id) const = 0;
    
    /**
     * @brief Broadcast message to all participants in channel
     * @param channel_id Channel identifier
     * @param message Message to broadcast
     * @return Number of recipients that received the message
     */
    virtual int broadcastMessage(const std::string& channel_id, const Message& message) = 0;
};

/**
 * @brief Legacy Communication System
 * 
 * Old communication system with C-style interface and different protocols.
 */
class LegacyCommunicationSystem {
public:
    // Legacy message structure
    struct OldMessage {
        int msg_id;
        char from_station[32];
        char to_station[32];
        char msg_text[512];
        int urgency_level; // 1=low, 2=normal, 3=high, 4=critical
        long send_time;
        int encryption_flag; // 0=none, 1=encrypted
    };
    
    struct CommLink {
        int link_id;
        char link_name[64];
        char station_list[10][32]; // Max 10 stations per link
        int station_count;
        int active_flag;
        float signal_quality;
    };
    
    /**
     * @brief Send message through legacy system
     * @param msg Pointer to message structure
     * @return 0 on success, error code otherwise
     */
    int TransmitMessage(const OldMessage* msg);
    
    /**
     * @brief Check for incoming messages
     * @param station_name Station identifier
     * @param messages Output buffer for messages
     * @param max_messages Maximum messages to retrieve
     * @return Number of messages retrieved
     */
    int CheckIncomingMessages(const char* station_name, OldMessage* messages, int max_messages);
    
    /**
     * @brief Establish communication link
     * @param link Pointer to link configuration
     * @return 0 on success, error code otherwise
     */
    int EstablishCommLink(const CommLink* link);
    
    /**
     * @brief Terminate communication link
     * @param link_id Link identifier
     * @return 0 on success, error code otherwise
     */
    int TerminateCommLink(int link_id);
    
    /**
     * @brief Get link information
     * @param link_id Link identifier
     * @param link Output parameter for link data
     * @return 0 on success, error code otherwise
     */
    int GetCommLinkInfo(int link_id, CommLink* link);
    
    /**
     * @brief Broadcast message on link
     * @param link_id Link identifier
     * @param msg Message to broadcast
     * @return Number of stations that received the message
     */
    int BroadcastOnLink(int link_id, const OldMessage* msg);

private:
    std::vector<OldMessage> message_queue_;
    std::vector<CommLink> active_links_;
    int next_message_id_;
    int next_link_id_;
    
    void initializeLegacyComm();
};

/**
 * @brief Communication System Adapter
 * 
 * Adapts legacy communication system to modern interface.
 */
class CommunicationAdapter : public IModernCommunication {
public:
    explicit CommunicationAdapter(std::unique_ptr<LegacyCommunicationSystem> legacy_comm);
    virtual ~CommunicationAdapter() = default;
    
    // IModernCommunication interface implementation
    bool sendMessage(const Message& message) override;
    std::vector<Message> receiveMessages(const std::string& recipient_id) override;
    bool openChannel(const CommunicationChannel& channel) override;
    bool closeChannel(const std::string& channel_id) override;
    std::optional<CommunicationChannel> getChannelStatus(const std::string& channel_id) const override;
    int broadcastMessage(const std::string& channel_id, const Message& message) override;

private:
    std::unique_ptr<LegacyCommunicationSystem> legacy_comm_;
    std::unordered_map<std::string, int> channel_id_map_; // Modern ID to legacy ID mapping
    int next_channel_id_;
    
    // Conversion helper methods
    LegacyCommunicationSystem::OldMessage convertModernToLegacyMessage(const Message& modern_msg) const;
    Message convertLegacyToModernMessage(const LegacyCommunicationSystem::OldMessage& legacy_msg) const;
    LegacyCommunicationSystem::CommLink convertModernToLegacyChannel(const CommunicationChannel& modern_channel) const;
    CommunicationChannel convertLegacyToModernChannel(const LegacyCommunicationSystem::CommLink& legacy_channel) const;
    std::string convertLegacyPriority(int urgency_level) const;
    int convertModernPriority(const std::string& priority) const;
    std::string generateChannelId() const;
};

/**
 * @brief Modern Resource Management Interface
 * 
 * Modern interface for managing planetary and fleet resources.
 */
class IModernResourceManager {
public:
    virtual ~IModernResourceManager() = default;
    
    struct ResourceInfo {
        std::string resource_type;
        double available_amount;
        double maximum_capacity;
        double production_rate;
        double consumption_rate;
        std::string location_id;
        std::chrono::system_clock::time_point last_update;
    };
    
    struct ResourceTransfer {
        std::string transfer_id;
        std::string resource_type;
        double amount;
        std::string source_location;
        std::string destination_location;
        std::string status; // "pending", "in_transit", "completed", "failed"
        double progress_percentage;
    };
    
    /**
     * @brief Get resource information for location
     * @param location_id Location identifier
     * @return Vector of resource information
     */
    virtual std::vector<ResourceInfo> getLocationResources(const std::string& location_id) const = 0;
    
    /**
     * @brief Get specific resource information
     * @param location_id Location identifier
     * @param resource_type Resource type
     * @return Optional resource information
     */
    virtual std::optional<ResourceInfo> getResourceInfo(const std::string& location_id,
                                                       const std::string& resource_type) const = 0;
    
    /**
     * @brief Transfer resources between locations
     * @param transfer Transfer details
     * @return true if transfer was initiated successfully
     */
    virtual bool initiateResourceTransfer(const ResourceTransfer& transfer) = 0;
    
    /**
     * @brief Get transfer status
     * @param transfer_id Transfer identifier
     * @return Optional transfer status
     */
    virtual std::optional<ResourceTransfer> getTransferStatus(const std::string& transfer_id) const = 0;
    
    /**
     * @brief Cancel resource transfer
     * @param transfer_id Transfer identifier
     * @return true if cancellation was successful
     */
    virtual bool cancelTransfer(const std::string& transfer_id) = 0;
    
    /**
     * @brief Update resource production/consumption rates
     * @param location_id Location identifier
     * @param resource_type Resource type
     * @param production_rate New production rate
     * @param consumption_rate New consumption rate
     * @return true if update was successful
     */
    virtual bool updateResourceRates(const std::string& location_id,
                                    const std::string& resource_type,
                                    double production_rate,
                                    double consumption_rate) = 0;
};

/**
 * @brief Legacy Resource System
 * 
 * Old resource management system with different data structures.
 */
class LegacyResourceSystem {
public:
    struct OldResourceRecord {
        int resource_id;
        char resource_name[32];
        float current_stock;
        float max_storage;
        float generation_per_hour;
        float usage_per_hour;
        int facility_id;
        long last_updated;
    };
    
    struct OldTransferOrder {
        int order_id;
        int resource_id;
        float quantity;
        int source_facility;
        int target_facility;
        int order_status; // 0=queued, 1=active, 2=done, 3=error
        float completion_percent;
    };
    
    /**
     * @brief Get resource data for facility
     * @param facility_id Facility identifier
     * @param resources Output buffer
     * @param max_resources Maximum resources to retrieve
     * @return Number of resources found
     */
    int GetFacilityResources(int facility_id, OldResourceRecord* resources, int max_resources);
    
    /**
     * @brief Get specific resource data
     * @param facility_id Facility identifier
     * @param resource_id Resource identifier
     * @param resource Output parameter
     * @return 0 on success, error code otherwise
     */
    int GetResourceData(int facility_id, int resource_id, OldResourceRecord* resource);
    
    /**
     * @brief Create resource transfer order
     * @param order Transfer order
     * @return Order ID on success, -1 on failure
     */
    int CreateTransferOrder(const OldTransferOrder* order);
    
    /**
     * @brief Check transfer order status
     * @param order_id Order identifier
     * @param order Output parameter
     * @return 0 on success, error code otherwise
     */
    int CheckTransferStatus(int order_id, OldTransferOrder* order);
    
    /**
     * @brief Cancel transfer order
     * @param order_id Order identifier
     * @return 0 on success, error code otherwise
     */
    int CancelTransferOrder(int order_id);
    
    /**
     * @brief Update resource rates
     * @param facility_id Facility identifier
     * @param resource_id Resource identifier
     * @param production_rate Production rate
     * @param consumption_rate Consumption rate
     * @return 0 on success, error code otherwise
     */
    int SetResourceRates(int facility_id, int resource_id, float production_rate, float consumption_rate);

private:
    std::vector<OldResourceRecord> resource_database_;
    std::vector<OldTransferOrder> transfer_orders_;
    int next_order_id_;
    
    void initializeLegacyResources();
    int findResourceId(const std::string& resource_name) const;
    int findFacilityId(const std::string& location_name) const;
};

/**
 * @brief Resource Management Adapter
 * 
 * Adapts legacy resource system to modern interface.
 */
class ResourceManagementAdapter : public IModernResourceManager {
public:
    explicit ResourceManagementAdapter(std::unique_ptr<LegacyResourceSystem> legacy_system);
    virtual ~ResourceManagementAdapter() = default;
    
    // IModernResourceManager interface implementation
    std::vector<ResourceInfo> getLocationResources(const std::string& location_id) const override;
    std::optional<ResourceInfo> getResourceInfo(const std::string& location_id,
                                               const std::string& resource_type) const override;
    bool initiateResourceTransfer(const ResourceTransfer& transfer) override;
    std::optional<ResourceTransfer> getTransferStatus(const std::string& transfer_id) const override;
    bool cancelTransfer(const std::string& transfer_id) override;
    bool updateResourceRates(const std::string& location_id,
                           const std::string& resource_type,
                           double production_rate,
                           double consumption_rate) override;

private:
    std::unique_ptr<LegacyResourceSystem> legacy_system_;
    std::unordered_map<std::string, int> transfer_id_map_; // Modern ID to legacy ID mapping
    std::unordered_map<std::string, int> location_id_map_; // Modern location to legacy facility ID
    std::unordered_map<std::string, int> resource_type_map_; // Modern type to legacy resource ID
    
    // Conversion helper methods
    ResourceInfo convertLegacyToModernResource(const LegacyResourceSystem::OldResourceRecord& legacy_record) const;
    ResourceTransfer convertLegacyToModernTransfer(const LegacyResourceSystem::OldTransferOrder& legacy_order) const;
    LegacyResourceSystem::OldTransferOrder convertModernToLegacyTransfer(const ResourceTransfer& modern_transfer) const;
    std::string convertLegacyTransferStatus(int legacy_status) const;
    int convertModernTransferStatus(const std::string& modern_status) const;
    std::string generateTransferId() const;
    void initializeIdMappings();
};

/**
 * @brief Adapter Factory
 * 
 * Factory class for creating different types of adapters for legacy systems.
 */
class AdapterFactory {
public:
    /**
     * @brief Create fleet management adapter
     * @param legacy_system Legacy fleet system instance
     * @return Unique pointer to adapter
     */
    static std::unique_ptr<IModernFleetManager> createFleetAdapter(
        std::unique_ptr<LegacyFleetSystem> legacy_system
    ) {
        return std::make_unique<FleetManagementAdapter>(std::move(legacy_system));
    }
    
    /**
     * @brief Create communication adapter
     * @param legacy_comm Legacy communication system instance
     * @return Unique pointer to adapter
     */
    static std::unique_ptr<IModernCommunication> createCommunicationAdapter(
        std::unique_ptr<LegacyCommunicationSystem> legacy_comm
    ) {
        return std::make_unique<CommunicationAdapter>(std::move(legacy_comm));
    }
    
    /**
     * @brief Create resource management adapter
     * @param legacy_system Legacy resource system instance
     * @return Unique pointer to adapter
     */
    static std::unique_ptr<IModernResourceManager> createResourceAdapter(
        std::unique_ptr<LegacyResourceSystem> legacy_system
    ) {
        return std::make_unique<ResourceManagementAdapter>(std::move(legacy_system));
    }
    
    /**
     * @brief Create legacy fleet system instance (for testing/demo)
     * @return Unique pointer to legacy system
     */
    static std::unique_ptr<LegacyFleetSystem> createLegacyFleetSystem() {
        return std::make_unique<LegacyFleetSystem>();
    }
    
    /**
     * @brief Create legacy communication system instance (for testing/demo)
     * @return Unique pointer to legacy system
     */
    static std::unique_ptr<LegacyCommunicationSystem> createLegacyCommunicationSystem() {
        return std::make_unique<LegacyCommunicationSystem>();
    }
    
    /**
     * @brief Create legacy resource system instance (for testing/demo)
     * @return Unique pointer to legacy system
     */
    static std::unique_ptr<LegacyResourceSystem> createLegacyResourceSystem() {
        return std::make_unique<LegacyResourceSystem>();
    }
};

/**
 * @brief Adapter Performance Monitor
 * 
 * Monitors the performance of adapters and tracks conversion overhead.
 */
class AdapterPerformanceMonitor {
public:
    struct PerformanceMetrics {
        size_t total_calls = 0;
        double total_conversion_time = 0.0;
        double average_conversion_time = 0.0;
        size_t failed_conversions = 0;
        std::chrono::system_clock::time_point last_reset;
    };
    
    /**
     * @brief Record a conversion operation
     * @param operation_name Name of the operation
     * @param conversion_time Time taken for conversion
     * @param success Whether conversion was successful
     */
    void recordConversion(const std::string& operation_name, double conversion_time, bool success = true);
    
    /**
     * @brief Get performance metrics for operation
     * @param operation_name Name of the operation
     * @return Optional performance metrics
     */
    std::optional<PerformanceMetrics> getMetrics(const std::string& operation_name) const;
    
    /**
     * @brief Reset metrics for all operations
     */
    void resetMetrics();
    
    /**
     * @brief Get performance report
     * @return Formatted performance report string
     */
    std::string generatePerformanceReport() const;

private:
    mutable std::mutex metrics_mutex_;
    std::unordered_map<std::string, PerformanceMetrics> metrics_;
};

} // namespace CppVerseHub::Patterns