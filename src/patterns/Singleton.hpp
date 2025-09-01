// File: src/patterns/Singleton.hpp
// CppVerseHub - Singleton Pattern Implementation for Single-instance Managers

#pragma once

#include <memory>
#include <mutex>
#include <atomic>
#include <string>
#include <unordered_map>

namespace CppVerseHub::Patterns {

/**
 * @brief Thread-safe Singleton base class template
 * 
 * This implementation uses the Curiously Recurring Template Pattern (CRTP)
 * to provide a thread-safe singleton pattern that can be inherited by any class.
 * It prevents copying, assignment, and ensures only one instance exists.
 */
template<typename T>
class Singleton {
public:
    // Delete copy constructor and assignment operator
    Singleton(const Singleton&) = delete;
    Singleton& operator=(const Singleton&) = delete;
    
    // Delete move constructor and assignment operator
    Singleton(Singleton&&) = delete;
    Singleton& operator=(Singleton&&) = delete;
    
    /**
     * @brief Get the singleton instance
     * @return Reference to the singleton instance
     */
    static T& getInstance() {
        std::call_once(initialized_, &Singleton<T>::initSingleton);
        return *instance_;
    }
    
    /**
     * @brief Check if the singleton has been initialized
     * @return true if initialized, false otherwise
     */
    static bool isInitialized() {
        return instance_ != nullptr;
    }
    
    /**
     * @brief Destroy the singleton instance (for testing purposes)
     * Note: This should only be used in test scenarios
     */
    static void destroyInstance() {
        std::lock_guard<std::mutex> lock(mutex_);
        instance_.reset();
        initialized_ = std::once_flag{};
    }

protected:
    /**
     * @brief Protected constructor to prevent direct instantiation
     */
    Singleton() = default;
    
    /**
     * @brief Virtual destructor for proper cleanup
     */
    virtual ~Singleton() = default;

private:
    static std::unique_ptr<T> instance_;
    static std::once_flag initialized_;
    static std::mutex mutex_;
    
    /**
     * @brief Initialize the singleton instance
     */
    static void initSingleton() {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!instance_) {
            instance_ = std::make_unique<T>();
        }
    }
};

// Static member definitions
template<typename T>
std::unique_ptr<T> Singleton<T>::instance_ = nullptr;

template<typename T>
std::once_flag Singleton<T>::initialized_;

template<typename T>
std::mutex Singleton<T>::mutex_;

/**
 * @brief Application Configuration Manager (Singleton)
 * 
 * Manages global application settings and configuration parameters.
 * Ensures only one configuration instance exists throughout the application.
 */
class ConfigManager : public Singleton<ConfigManager> {
    friend class Singleton<ConfigManager>;

public:
    /**
     * @brief Set a configuration value
     * @param key Configuration key
     * @param value Configuration value
     */
    void setConfig(const std::string& key, const std::string& value) {
        std::lock_guard<std::mutex> lock(config_mutex_);
        config_map_[key] = value;
    }
    
    /**
     * @brief Get a configuration value
     * @param key Configuration key
     * @param default_value Default value if key doesn't exist
     * @return Configuration value or default
     */
    std::string getConfig(const std::string& key, const std::string& default_value = "") const {
        std::lock_guard<std::mutex> lock(config_mutex_);
        auto it = config_map_.find(key);
        return (it != config_map_.end()) ? it->second : default_value;
    }
    
    /**
     * @brief Check if a configuration key exists
     * @param key Configuration key
     * @return true if key exists, false otherwise
     */
    bool hasConfig(const std::string& key) const {
        std::lock_guard<std::mutex> lock(config_mutex_);
        return config_map_.find(key) != config_map_.end();
    }
    
    /**
     * @brief Load configuration from file
     * @param filename Configuration file path
     * @return true if loaded successfully, false otherwise
     */
    bool loadFromFile(const std::string& filename);
    
    /**
     * @brief Save configuration to file
     * @param filename Configuration file path
     * @return true if saved successfully, false otherwise
     */
    bool saveToFile(const std::string& filename) const;
    
    /**
     * @brief Clear all configuration settings
     */
    void clear() {
        std::lock_guard<std::mutex> lock(config_mutex_);
        config_map_.clear();
    }
    
    /**
     * @brief Get all configuration keys
     * @return Vector of all configuration keys
     */
    std::vector<std::string> getKeys() const {
        std::lock_guard<std::mutex> lock(config_mutex_);
        std::vector<std::string> keys;
        keys.reserve(config_map_.size());
        for (const auto& pair : config_map_) {
            keys.push_back(pair.first);
        }
        return keys;
    }

private:
    ConfigManager() = default;
    
    mutable std::mutex config_mutex_;
    std::unordered_map<std::string, std::string> config_map_;
};

/**
 * @brief Global Logger Manager (Singleton)
 * 
 * Provides centralized logging functionality across the application.
 * Thread-safe logging with different log levels and output destinations.
 */
class LogManager : public Singleton<LogManager> {
    friend class Singleton<LogManager>;

public:
    enum class LogLevel {
        DEBUG = 0,
        INFO = 1,
        WARNING = 2,
        ERROR = 3,
        CRITICAL = 4
    };

    /**
     * @brief Set the minimum log level
     * @param level Minimum log level to output
     */
    void setLogLevel(LogLevel level) {
        current_level_.store(level);
    }
    
    /**
     * @brief Get current log level
     * @return Current minimum log level
     */
    LogLevel getLogLevel() const {
        return current_level_.load();
    }
    
    /**
     * @brief Log a message with specified level
     * @param level Log level
     * @param message Log message
     * @param category Optional category/component name
     */
    void log(LogLevel level, const std::string& message, const std::string& category = "");
    
    /**
     * @brief Convenience methods for different log levels
     */
    void debug(const std::string& message, const std::string& category = "") {
        log(LogLevel::DEBUG, message, category);
    }
    
    void info(const std::string& message, const std::string& category = "") {
        log(LogLevel::INFO, message, category);
    }
    
    void warning(const std::string& message, const std::string& category = "") {
        log(LogLevel::WARNING, message, category);
    }
    
    void error(const std::string& message, const std::string& category = "") {
        log(LogLevel::ERROR, message, category);
    }
    
    void critical(const std::string& message, const std::string& category = "") {
        log(LogLevel::CRITICAL, message, category);
    }
    
    /**
     * @brief Set log file path
     * @param filepath Path to log file
     * @return true if file opened successfully, false otherwise
     */
    bool setLogFile(const std::string& filepath);
    
    /**
     * @brief Enable/disable console output
     * @param enable true to enable console output, false to disable
     */
    void enableConsoleOutput(bool enable) {
        console_output_.store(enable);
    }
    
    /**
     * @brief Flush all pending log messages
     */
    void flush();

private:
    LogManager();
    ~LogManager();
    
    std::atomic<LogLevel> current_level_{LogLevel::INFO};
    std::atomic<bool> console_output_{true};
    
    mutable std::mutex log_mutex_;
    std::unique_ptr<std::ofstream> log_file_;
    
    /**
     * @brief Convert log level to string
     * @param level Log level
     * @return String representation of log level
     */
    std::string levelToString(LogLevel level) const;
    
    /**
     * @brief Get current timestamp string
     * @return Formatted timestamp string
     */
    std::string getCurrentTimestamp() const;
};

/**
 * @brief Performance Metrics Collector (Singleton)
 * 
 * Collects and manages performance metrics across the application.
 * Provides timing measurements and statistical analysis.
 */
class MetricsCollector : public Singleton<MetricsCollector> {
    friend class Singleton<MetricsCollector>;

public:
    struct TimingData {
        double total_time = 0.0;
        double min_time = std::numeric_limits<double>::max();
        double max_time = 0.0;
        size_t count = 0;
        
        double getAverageTime() const {
            return count > 0 ? total_time / count : 0.0;
        }
    };
    
    /**
     * @brief Record timing data for a named operation
     * @param operation_name Name of the operation
     * @param duration_ms Duration in milliseconds
     */
    void recordTiming(const std::string& operation_name, double duration_ms);
    
    /**
     * @brief Get timing statistics for an operation
     * @param operation_name Name of the operation
     * @return Timing statistics or nullptr if operation not found
     */
    const TimingData* getTimingData(const std::string& operation_name) const;
    
    /**
     * @brief Get all recorded operation names
     * @return Vector of operation names
     */
    std::vector<std::string> getOperationNames() const;
    
    /**
     * @brief Clear all timing data
     */
    void clearTimings();
    
    /**
     * @brief Generate performance report
     * @return Formatted performance report string
     */
    std::string generateReport() const;
    
    /**
     * @brief RAII Timer class for automatic timing measurement
     */
    class Timer {
    public:
        explicit Timer(const std::string& operation_name);
        ~Timer();
        
        // Disable copy and move
        Timer(const Timer&) = delete;
        Timer& operator=(const Timer&) = delete;
        Timer(Timer&&) = delete;
        Timer& operator=(Timer&&) = delete;
        
    private:
        std::string operation_name_;
        std::chrono::steady_clock::time_point start_time_;
    };

private:
    MetricsCollector() = default;
    
    mutable std::mutex metrics_mutex_;
    std::unordered_map<std::string, TimingData> timing_data_;
};

} // namespace CppVerseHub::Patterns

/**
 * @brief Convenience macro for automatic timing measurement
 * Usage: MEASURE_TIME("operation_name") within a scope
 */
#define MEASURE_TIME(op_name) \
    CppVerseHub::Patterns::MetricsCollector::Timer timer_##__LINE__(op_name)