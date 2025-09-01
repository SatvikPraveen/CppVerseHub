// File: src/utils/Logger.cpp
// Comprehensive Logging System Implementation

#include "Logger.hpp"
#include <filesystem>
#include <cstdlib>

namespace CppVerseHub::Utils {

// ===== LOGGER CONFIGURATION UTILITIES =====

namespace LoggerConfig {
    
    void setupDefaultConsoleLogger(const std::string& logger_name, LogLevel level = LogLevel::INFO) {
        auto logger = LoggerManager::getInstance().getLogger(logger_name);
        logger->setLogLevel(level);
        
        // Add console appender with colors
        auto console_appender = std::make_unique<ConsoleAppender>(true);
        auto formatter = std::make_unique<DefaultFormatter>("%Y-%m-%d %H:%M:%S", true, true);
        console_appender->setFormatter(std::move(formatter));
        
        logger->addAppender(std::move(console_appender));
    }
    
    void setupFileLogger(const std::string& logger_name, const std::string& log_file,
                        LogLevel level = LogLevel::INFO) {
        auto logger = LoggerManager::getInstance().getLogger(logger_name);
        logger->setLogLevel(level);
        
        // Create directory if it doesn't exist
        std::filesystem::path file_path(log_file);
        if (file_path.has_parent_path()) {
            std::filesystem::create_directories(file_path.parent_path());
        }
        
        // Add file appender
        auto file_appender = std::make_unique<FileAppender>(log_file);
        auto formatter = std::make_unique<DefaultFormatter>("%Y-%m-%d %H:%M:%S", true, true);
        file_appender->setFormatter(std::move(formatter));
        
        logger->addAppender(std::move(file_appender));
    }
    
    void setupRotatingFileLogger(const std::string& logger_name, const std::string& base_filename,
                               std::chrono::hours rotation_interval = std::chrono::hours(24),
                               LogLevel level = LogLevel::INFO) {
        auto logger = LoggerManager::getInstance().getLogger(logger_name);
        logger->setLogLevel(level);
        
        // Create directory if it doesn't exist
        std::filesystem::path file_path(base_filename);
        if (file_path.has_parent_path()) {
            std::filesystem::create_directories(file_path.parent_path());
        }
        
        // Add rotating file appender
        auto rotating_appender = std::make_unique<RotatingFileAppender>(base_filename, rotation_interval);
        auto formatter = std::make_unique<DefaultFormatter>("%Y-%m-%d %H:%M:%S", true, true);
        rotating_appender->setFormatter(std::move(formatter));
        
        logger->addAppender(std::move(rotating_appender));
    }
    
    void setupJsonLogger(const std::string& logger_name, const std::string& log_file,
                        LogLevel level = LogLevel::INFO) {
        auto logger = LoggerManager::getInstance().getLogger(logger_name);
        logger->setLogLevel(level);
        
        // Create directory if it doesn't exist
        std::filesystem::path file_path(log_file);
        if (file_path.has_parent_path()) {
            std::filesystem::create_directories(file_path.parent_path());
        }
        
        // Add file appender with JSON formatter
        auto file_appender = std::make_unique<FileAppender>(log_file);
        auto formatter = std::make_unique<JsonFormatter>();
        file_appender->setFormatter(std::move(formatter));
        
        logger->addAppender(std::move(file_appender));
    }
    
    void setupSpaceGameLoggers() {
        // Main game logger
        auto game_logger = LoggerManager::getInstance().getLogger("Game");
        game_logger->setLogLevel(LogLevel::INFO);
        
        // Console output
        auto console_appender = std::make_unique<ConsoleAppender>(true);
        auto console_formatter = std::make_unique<DefaultFormatter>("%H:%M:%S", false, false);
        console_appender->setFormatter(std::move(console_formatter));
        game_logger->addAppender(std::move(console_appender));
        
        // File output
        auto file_appender = std::make_unique<FileAppender>("logs/game.log");
        auto file_formatter = std::make_unique<DefaultFormatter>("%Y-%m-%d %H:%M:%S", true, true);
        file_appender->setFormatter(std::move(file_formatter));
        game_logger->addAppender(std::move(file_appender));
        
        // Fleet operations logger
        setupFileLogger("Fleet", "logs/fleet.log", LogLevel::DEBUG);
        
        // Mission logger with JSON format for analytics
        setupJsonLogger("Mission", "logs/missions.json", LogLevel::INFO);
        
        // Error logger with rotating files
        setupRotatingFileLogger("Error", "logs/errors", std::chrono::hours(168), LogLevel::ERROR); // Weekly rotation
        
        // Performance logger
        setupFileLogger("Performance", "logs/performance.log", LogLevel::INFO);
        
        // Network logger
        setupFileLogger("Network", "logs/network.log", LogLevel::WARN);
    }
    
} // namespace LoggerConfig

// ===== PERFORMANCE LOGGER =====

class PerformanceLogger {
private:
    std::shared_ptr<AsyncLogger> logger_;
    std::unordered_map<std::string, std::chrono::high_resolution_clock::time_point> start_times_;
    std::mutex mutex_;
    
public:
    PerformanceLogger() : logger_(LoggerManager::getInstance().getLogger("Performance")) {}
    
    void startTimer(const std::string& operation) {
        std::lock_guard<std::mutex> lock(mutex_);
        start_times_[operation] = std::chrono::high_resolution_clock::now();
        
        std::ostringstream oss;
        oss << "Started operation: " << operation;
        LOG_DEBUG(logger_, oss.str());
    }
    
    void endTimer(const std::string& operation) {
        auto end_time = std::chrono::high_resolution_clock::now();
        
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = start_times_.find(operation);
        if (it != start_times_.end()) {
            auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
                end_time - it->second);
            
            std::ostringstream oss;
            oss << "Completed operation: " << operation 
                << " (duration: " << duration.count() << " μs)";
            LOG_INFO(logger_, oss.str());
            
            start_times_.erase(it);
        } else {
            std::ostringstream oss;
            oss << "End timer called for unknown operation: " << operation;
            LOG_WARN(logger_, oss.str());
        }
    }
    
    static PerformanceLogger& getInstance() {
        static PerformanceLogger instance;
        return instance;
    }
};

// ===== SCOPED PERFORMANCE TIMER =====

class ScopedPerformanceTimer {
private:
    std::string operation_name_;
    
public:
    explicit ScopedPerformanceTimer(const std::string& operation) : operation_name_(operation) {
        PerformanceLogger::getInstance().startTimer(operation_name_);
    }
    
    ~ScopedPerformanceTimer() {
        PerformanceLogger::getInstance().endTimer(operation_name_);
    }
};

// ===== MEMORY APPENDER =====

class MemoryAppender : public LogAppender {
private:
    std::unique_ptr<LogFormatter> formatter_;
    std::deque<std::string> log_buffer_;
    std::mutex mutex_;
    size_t max_entries_;
    
public:
    explicit MemoryAppender(size_t max_entries = 1000)
        : formatter_(std::make_unique<DefaultFormatter>()), max_entries_(max_entries) {}
    
    void append(const LogEntry& entry) override {
        std::lock_guard<std::mutex> lock(mutex_);
        
        log_buffer_.push_back(formatter_->format(entry));
        
        // Remove old entries if we exceed the limit
        while (log_buffer_.size() > max_entries_) {
            log_buffer_.pop_front();
        }
    }
    
    void flush() override {
        // Memory appender doesn't need to flush
    }
    
    void setFormatter(std::unique_ptr<LogFormatter> formatter) override {
        std::lock_guard<std::mutex> lock(mutex_);
        formatter_ = std::move(formatter);
    }
    
    std::unique_ptr<LogAppender> clone() const override {
        auto cloned = std::make_unique<MemoryAppender>(max_entries_);
        cloned->setFormatter(formatter_->clone());
        return std::move(cloned);
    }
    
    std::vector<std::string> getLogs() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return std::vector<std::string>(log_buffer_.begin(), log_buffer_.end());
    }
    
    void clear() {
        std::lock_guard<std::mutex> lock(mutex_);
        log_buffer_.clear();
    }
    
    size_t size() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return log_buffer_.size();
    }
};

// ===== NETWORK APPENDER (TCP/UDP) =====

class NetworkAppender : public LogAppender {
private:
    std::unique_ptr<LogFormatter> formatter_;
    std::string host_;
    int port_;
    std::mutex mutex_;
    // Note: Real implementation would include socket handling
    
public:
    NetworkAppender(const std::string& host, int port)
        : formatter_(std::make_unique<JsonFormatter>()), host_(host), port_(port) {
        // Initialize network connection
        std::ostringstream oss;
        oss << "NetworkAppender configured for " << host_ << ":" << port_;
        std::cout << oss.str() << std::endl; // Placeholder for actual logging
    }
    
    void append(const LogEntry& entry) override {
        std::lock_guard<std::mutex> lock(mutex_);
        
        std::string formatted = formatter_->format(entry);
        
        // Placeholder for actual network transmission
        std::ostringstream oss;
        oss << "[NETWORK " << host_ << ":" << port_ << "] " << formatted;
        std::cout << oss.str() << std::endl;
    }
    
    void flush() override {
        std::lock_guard<std::mutex> lock(mutex_);
        // Flush network buffer
    }
    
    void setFormatter(std::unique_ptr<LogFormatter> formatter) override {
        std::lock_guard<std::mutex> lock(mutex_);
        formatter_ = std::move(formatter);
    }
    
    std::unique_ptr<LogAppender> clone() const override {
        auto cloned = std::make_unique<NetworkAppender>(host_, port_);
        cloned->setFormatter(formatter_->clone());
        return std::move(cloned);
    }
};

// ===== LOGGER UTILITIES =====

namespace LoggerUtils {
    
    void configureFromEnvironment() {
        // Check environment variables for log configuration
        const char* log_level_env = std::getenv("CPPVERSEHUB_LOG_LEVEL");
        if (log_level_env) {
            LogLevel level = stringToLogLevel(log_level_env);
            LoggerManager::getInstance().setDefaultLogLevel(level);
        }
        
        const char* log_file_env = std::getenv("CPPVERSEHUB_LOG_FILE");
        if (log_file_env) {
            LoggerConfig::setupFileLogger("DefaultFile", log_file_env);
        }
        
        const char* log_json_env = std::getenv("CPPVERSEHUB_LOG_JSON");
        if (log_json_env && std::string(log_json_env) == "true") {
            LoggerConfig::setupJsonLogger("DefaultJson", "logs/application.json");
        }
    }
    
    void demonstrateLogging() {
        std::cout << "\n=== Comprehensive Logging System Demonstration ===" << std::endl;
        
        // Setup space game loggers
        LoggerConfig::setupSpaceGameLoggers();
        
        // Get various loggers
        auto game_logger = LoggerManager::getInstance().getLogger("Game");
        auto fleet_logger = LoggerManager::getInstance().getLogger("Fleet");
        auto mission_logger = LoggerManager::getInstance().getLogger("Mission");
        auto error_logger = LoggerManager::getInstance().getLogger("Error");
        
        // Demonstrate different log levels
        LOG_TRACE(game_logger, "Game initialization starting...");
        LOG_DEBUG(game_logger, "Loading configuration files");
        LOG_INFO(game_logger, "CppVerseHub Space Game v1.0 started successfully");
        LOG_WARN(game_logger, "Low memory warning: 85% used");
        LOG_ERROR(error_logger, "Failed to connect to remote server");
        LOG_FATAL(error_logger, "Critical system failure detected");
        
        // Fleet operations
        LOG_INFO(fleet_logger, "Fleet Alpha deployed to sector 7");
        LOG_DEBUG(fleet_logger, "Fleet composition: 5 fighters, 2 cruisers, 1 carrier");
        LOG_WARN(fleet_logger, "Fleet Beta fuel level below 30%");
        
        // Mission logging (JSON format)
        LOG_INFO(mission_logger, "Mission 'Explore Alpha Centauri' completed successfully");
        LOG_INFO(mission_logger, "Mission 'Colonize Kepler-442b' started with 3 ships");
        LOG_WARN(mission_logger, "Mission 'Rescue Stranded Crew' delayed due to asteroid field");
        
        // Performance logging demonstration
        {
            ScopedPerformanceTimer timer("GameUpdate");
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        
        PerformanceLogger::getInstance().startTimer("ResourceCalculation");
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
        PerformanceLogger::getInstance().endTimer("ResourceCalculation");
        
        // Memory appender demonstration
        auto memory_logger = LoggerManager::getInstance().getLogger("Memory");
        auto memory_appender = std::make_unique<MemoryAppender>(100);
        memory_logger->addAppender(std::move(memory_appender));
        
        LOG_INFO(memory_logger, "First memory log entry");
        LOG_INFO(memory_logger, "Second memory log entry");
        LOG_INFO(memory_logger, "Third memory log entry");
        
        // Network appender demonstration
        auto network_logger = LoggerManager::getInstance().getLogger("Network");
        auto network_appender = std::make_unique<NetworkAppender>("logserver.example.com", 9999);
        network_logger->addAppender(std::move(network_appender));
        
        LOG_INFO(network_logger, "Network log entry sent to remote server");
        
        // Function scope logging
        {
            LOG_FUNCTION_SCOPE(game_logger);
            LOG_INFO(game_logger, "Performing complex calculation inside scoped function");
            std::this_thread::sleep_for(std::chrono::milliseconds(2));
        }
        
        // Wait a moment for async logging to complete
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        // Flush all loggers
        game_logger->flush();
        fleet_logger->flush();
        mission_logger->flush();
        error_logger->flush();
        
        std::cout << "Logging demonstration completed. Check log files in 'logs/' directory." << std::endl;
        std::cout << "Active loggers: " << LoggerManager::getInstance().getLoggerCount() << std::endl;
    }
    
    void benchmarkLogging() {
        std::cout << "\n=== Logging Performance Benchmark ===" << std::endl;
        
        const int num_messages = 10000;
        
        // Setup benchmark logger
        auto benchmark_logger = LoggerManager::getInstance().getLogger("Benchmark");
        benchmark_logger->setLogLevel(LogLevel::INFO);
        
        auto file_appender = std::make_unique<FileAppender>("logs/benchmark.log");
        benchmark_logger->addAppender(std::move(file_appender));
        
        // Benchmark async logging
        auto start_time = std::chrono::high_resolution_clock::now();
        
        for (int i = 0; i < num_messages; ++i) {
            std::ostringstream oss;
            oss << "Benchmark message #" << i << " with some additional data";
            LOG_INFO(benchmark_logger, oss.str());
        }
        
        benchmark_logger->flush();
        
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        
        std::cout << "Logged " << num_messages << " messages in " << duration.count() << " ms" << std::endl;
        std::cout << "Average: " << (static_cast<double>(duration.count()) / num_messages) << " ms per message" << std::endl;
        std::cout << "Throughput: " << (num_messages * 1000 / duration.count()) << " messages/second" << std::endl;
    }
    
    void testErrorRecovery() {
        std::cout << "\n=== Testing Error Recovery ===" << std::endl;
        
        auto test_logger = LoggerManager::getInstance().getLogger("ErrorRecoveryTest");
        
        // Test with invalid file path
        try {
            auto invalid_appender = std::make_unique<FileAppender>("/invalid/path/test.log");
            test_logger->addAppender(std::move(invalid_appender));
        } catch (const std::exception& e) {
            std::cout << "Expected error caught: " << e.what() << std::endl;
        }
        
        // Test with valid appender
        auto valid_appender = std::make_unique<ConsoleAppender>();
        test_logger->addAppender(std::move(valid_appender));
        
        LOG_INFO(test_logger, "Error recovery test completed successfully");
        
        std::cout << "Logger continues to work after error recovery" << std::endl;
    }
    
} // namespace LoggerUtils

// ===== CLEANUP UTILITIES =====

class LoggerCleanup {
public:
    static void cleanupOldLogs(const std::string& log_directory, int max_age_days = 30) {
        try {
            if (!std::filesystem::exists(log_directory)) {
                return;
            }
            
            auto cutoff_time = std::chrono::system_clock::now() - std::chrono::hours(24 * max_age_days);
            
            for (const auto& entry : std::filesystem::directory_iterator(log_directory)) {
                if (entry.is_regular_file()) {
                    auto file_time = std::filesystem::last_write_time(entry);
                    auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
                        file_time - std::filesystem::file_time_type::clock::now() + 
                        std::chrono::system_clock::now());
                    
                    if (sctp < cutoff_time) {
                        std::filesystem::remove(entry.path());
                        std::cout << "Removed old log file: " << entry.path() << std::endl;
                    }
                }
            }
        } catch (const std::exception& e) {
            std::cerr << "Error during log cleanup: " << e.what() << std::endl;
        }
    }
    
    static void compressOldLogs(const std::string& log_directory, int compress_age_days = 7) {
        // Placeholder for log compression functionality
        std::cout << "Log compression functionality would be implemented here for directory: " 
                  << log_directory << " (age: " << compress_age_days << " days)" << std::endl;
    }
};

} // namespace CppVerseHub::Utils

// ===== GLOBAL CLEANUP FUNCTION =====

namespace {
    bool cleanup_registered = false;
    
    void cleanup_loggers() {
        CppVerseHub::Utils::LoggerManager::getInstance().shutdown();
    }
    
    void register_cleanup() {
        if (!cleanup_registered) {
            std::atexit(cleanup_loggers);
            cleanup_registered = true;
        }
    }
}

// Auto-register cleanup on library load
static int auto_register = (CppVerseHub::Utils::register_cleanup(), 0);