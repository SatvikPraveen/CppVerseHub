// File: src/utils/Logger.hpp
// Comprehensive Logging System for Space Game

#pragma once

#include <string>
#include <fstream>
#include <memory>
#include <mutex>
#include <chrono>
#include <queue>
#include <thread>
#include <atomic>
#include <condition_variable>
#include <unordered_map>
#include <iostream>
#include <sstream>
#include <iomanip>

namespace CppVerseHub::Utils {

// ===== LOG LEVEL ENUMERATION =====

enum class LogLevel {
    TRACE = 0,
    DEBUG = 1,
    INFO = 2,
    WARN = 3,
    ERROR = 4,
    FATAL = 5,
    OFF = 6
};

// Helper function to convert LogLevel to string
inline const char* logLevelToString(LogLevel level) {
    switch (level) {
        case LogLevel::TRACE: return "TRACE";
        case LogLevel::DEBUG: return "DEBUG";
        case LogLevel::INFO:  return "INFO ";
        case LogLevel::WARN:  return "WARN ";
        case LogLevel::ERROR: return "ERROR";
        case LogLevel::FATAL: return "FATAL";
        case LogLevel::OFF:   return "OFF  ";
        default:              return "UNKNOWN";
    }
}

// Helper function to convert string to LogLevel
inline LogLevel stringToLogLevel(const std::string& levelStr) {
    std::string upper = levelStr;
    std::transform(upper.begin(), upper.end(), upper.begin(), ::toupper);
    
    if (upper == "TRACE") return LogLevel::TRACE;
    if (upper == "DEBUG") return LogLevel::DEBUG;
    if (upper == "INFO")  return LogLevel::INFO;
    if (upper == "WARN")  return LogLevel::WARN;
    if (upper == "ERROR") return LogLevel::ERROR;
    if (upper == "FATAL") return LogLevel::FATAL;
    if (upper == "OFF")   return LogLevel::OFF;
    
    return LogLevel::INFO; // Default
}

// ===== LOG ENTRY STRUCTURE =====

struct LogEntry {
    std::chrono::system_clock::time_point timestamp;
    LogLevel level;
    std::string logger_name;
    std::string message;
    std::string file;
    std::string function;
    int line;
    std::thread::id thread_id;
    
    LogEntry() = default;
    
    LogEntry(LogLevel lvl, const std::string& name, const std::string& msg,
             const std::string& file_name = "", const std::string& func_name = "", 
             int line_num = 0)
        : timestamp(std::chrono::system_clock::now())
        , level(lvl)
        , logger_name(name)
        , message(msg)
        , file(file_name)
        , function(func_name)
        , line(line_num)
        , thread_id(std::this_thread::get_id()) {
    }
};

// ===== LOG FORMATTER INTERFACE =====

class LogFormatter {
public:
    virtual ~LogFormatter() = default;
    virtual std::string format(const LogEntry& entry) const = 0;
    virtual std::unique_ptr<LogFormatter> clone() const = 0;
};

// ===== DEFAULT FORMATTER IMPLEMENTATION =====

class DefaultFormatter : public LogFormatter {
private:
    std::string date_format_;
    bool show_thread_id_;
    bool show_location_;
    
public:
    explicit DefaultFormatter(const std::string& date_fmt = "%Y-%m-%d %H:%M:%S",
                             bool show_thread = true, bool show_loc = false)
        : date_format_(date_fmt), show_thread_id_(show_thread), show_location_(show_loc) {}
    
    std::string format(const LogEntry& entry) const override {
        std::ostringstream oss;
        
        // Timestamp
        auto time_t = std::chrono::system_clock::to_time_t(entry.timestamp);
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            entry.timestamp.time_since_epoch()) % 1000;
        
        oss << std::put_time(std::localtime(&time_t), date_format_.c_str())
            << "." << std::setfill('0') << std::setw(3) << ms.count();
        
        // Log level
        oss << " [" << logLevelToString(entry.level) << "]";
        
        // Logger name
        if (!entry.logger_name.empty()) {
            oss << " [" << entry.logger_name << "]";
        }
        
        // Thread ID
        if (show_thread_id_) {
            oss << " [Thread-" << entry.thread_id << "]";
        }
        
        // Location information
        if (show_location_ && !entry.file.empty()) {
            oss << " [" << entry.file;
            if (!entry.function.empty()) {
                oss << "::" << entry.function;
            }
            if (entry.line > 0) {
                oss << ":" << entry.line;
            }
            oss << "]";
        }
        
        // Message
        oss << " - " << entry.message;
        
        return oss.str();
    }
    
    std::unique_ptr<LogFormatter> clone() const override {
        return std::make_unique<DefaultFormatter>(date_format_, show_thread_id_, show_location_);
    }
};

// ===== JSON FORMATTER =====

class JsonFormatter : public LogFormatter {
public:
    std::string format(const LogEntry& entry) const override {
        std::ostringstream oss;
        
        auto time_t = std::chrono::system_clock::to_time_t(entry.timestamp);
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            entry.timestamp.time_since_epoch()) % 1000;
        
        oss << "{"
            << "\"timestamp\":\"" << std::put_time(std::gmtime(&time_t), "%Y-%m-%dT%H:%M:%S")
            << "." << std::setfill('0') << std::setw(3) << ms.count() << "Z\","
            << "\"level\":\"" << logLevelToString(entry.level) << "\","
            << "\"logger\":\"" << escapeJson(entry.logger_name) << "\","
            << "\"message\":\"" << escapeJson(entry.message) << "\","
            << "\"thread\":\"" << entry.thread_id << "\"";
            
        if (!entry.file.empty()) {
            oss << ",\"file\":\"" << escapeJson(entry.file) << "\"";
        }
        if (!entry.function.empty()) {
            oss << ",\"function\":\"" << escapeJson(entry.function) << "\"";
        }
        if (entry.line > 0) {
            oss << ",\"line\":" << entry.line;
        }
        
        oss << "}";
        return oss.str();
    }
    
    std::unique_ptr<LogFormatter> clone() const override {
        return std::make_unique<JsonFormatter>();
    }
    
private:
    std::string escapeJson(const std::string& str) const {
        std::string escaped;
        escaped.reserve(str.length() + str.length() / 10); // Reserve extra space
        
        for (char c : str) {
            switch (c) {
                case '"':  escaped += "\\\""; break;
                case '\\': escaped += "\\\\"; break;
                case '\b': escaped += "\\b"; break;
                case '\f': escaped += "\\f"; break;
                case '\n': escaped += "\\n"; break;
                case '\r': escaped += "\\r"; break;
                case '\t': escaped += "\\t"; break;
                default:
                    if (c < 0x20) {
                        escaped += "\\u";
                        escaped += "0000";
                        escaped[escaped.length()-2] = "0123456789abcdef"[c >> 4];
                        escaped[escaped.length()-1] = "0123456789abcdef"[c & 0xF];
                    } else {
                        escaped += c;
                    }
                    break;
            }
        }
        
        return escaped;
    }
};

// ===== LOG APPENDER INTERFACE =====

class LogAppender {
public:
    virtual ~LogAppender() = default;
    virtual void append(const LogEntry& entry) = 0;
    virtual void flush() = 0;
    virtual void setFormatter(std::unique_ptr<LogFormatter> formatter) = 0;
    virtual std::unique_ptr<LogAppender> clone() const = 0;
};

// ===== CONSOLE APPENDER =====

class ConsoleAppender : public LogAppender {
private:
    std::unique_ptr<LogFormatter> formatter_;
    std::mutex mutex_;
    bool use_colors_;
    
    // ANSI color codes
    const char* getColorCode(LogLevel level) const {
        if (!use_colors_) return "";
        
        switch (level) {
            case LogLevel::TRACE: return "\033[37m";   // White
            case LogLevel::DEBUG: return "\033[36m";   // Cyan
            case LogLevel::INFO:  return "\033[32m";   // Green
            case LogLevel::WARN:  return "\033[33m";   // Yellow
            case LogLevel::ERROR: return "\033[31m";   // Red
            case LogLevel::FATAL: return "\033[35m";   // Magenta
            default:              return "\033[0m";    // Reset
        }
    }
    
    const char* getResetCode() const {
        return use_colors_ ? "\033[0m" : "";
    }
    
public:
    explicit ConsoleAppender(bool colors = true)
        : formatter_(std::make_unique<DefaultFormatter>()), use_colors_(colors) {}
    
    void append(const LogEntry& entry) override {
        std::lock_guard<std::mutex> lock(mutex_);
        
        auto& stream = (entry.level >= LogLevel::WARN) ? std::cerr : std::cout;
        
        stream << getColorCode(entry.level)
               << formatter_->format(entry)
               << getResetCode()
               << std::endl;
    }
    
    void flush() override {
        std::lock_guard<std::mutex> lock(mutex_);
        std::cout.flush();
        std::cerr.flush();
    }
    
    void setFormatter(std::unique_ptr<LogFormatter> formatter) override {
        std::lock_guard<std::mutex> lock(mutex_);
        formatter_ = std::move(formatter);
    }
    
    std::unique_ptr<LogAppender> clone() const override {
        auto cloned = std::make_unique<ConsoleAppender>(use_colors_);
        cloned->setFormatter(formatter_->clone());
        return std::move(cloned);
    }
};

// ===== FILE APPENDER =====

class FileAppender : public LogAppender {
private:
    std::unique_ptr<LogFormatter> formatter_;
    std::ofstream file_;
    std::string filename_;
    std::mutex mutex_;
    size_t max_file_size_;
    int max_backup_files_;
    
    void rotateFile() {
        if (max_backup_files_ <= 0) return;
        
        file_.close();
        
        // Remove oldest backup
        std::string oldest_backup = filename_ + "." + std::to_string(max_backup_files_);
        std::remove(oldest_backup.c_str());
        
        // Rotate existing backups
        for (int i = max_backup_files_ - 1; i >= 1; --i) {
            std::string old_name = filename_ + "." + std::to_string(i);
            std::string new_name = filename_ + "." + std::to_string(i + 1);
            std::rename(old_name.c_str(), new_name.c_str());
        }
        
        // Move current file to .1
        std::string backup_name = filename_ + ".1";
        std::rename(filename_.c_str(), backup_name.c_str());
        
        // Reopen file
        file_.open(filename_, std::ios::app);
    }
    
public:
    explicit FileAppender(const std::string& filename, 
                         size_t max_size = 10 * 1024 * 1024, // 10MB default
                         int max_backups = 5)
        : formatter_(std::make_unique<DefaultFormatter>())
        , filename_(filename)
        , max_file_size_(max_size)
        , max_backup_files_(max_backups) {
        
        file_.open(filename_, std::ios::app);
        if (!file_.is_open()) {
            throw std::runtime_error("Failed to open log file: " + filename_);
        }
    }
    
    ~FileAppender() {
        if (file_.is_open()) {
            file_.close();
        }
    }
    
    void append(const LogEntry& entry) override {
        std::lock_guard<std::mutex> lock(mutex_);
        
        if (file_.is_open()) {
            std::string formatted = formatter_->format(entry);
            file_ << formatted << std::endl;
            
            // Check if rotation is needed
            if (max_file_size_ > 0 && file_.tellp() > static_cast<std::streampos>(max_file_size_)) {
                rotateFile();
            }
        }
    }
    
    void flush() override {
        std::lock_guard<std::mutex> lock(mutex_);
        if (file_.is_open()) {
            file_.flush();
        }
    }
    
    void setFormatter(std::unique_ptr<LogFormatter> formatter) override {
        std::lock_guard<std::mutex> lock(mutex_);
        formatter_ = std::move(formatter);
    }
    
    std::unique_ptr<LogAppender> clone() const override {
        auto cloned = std::make_unique<FileAppender>(filename_, max_file_size_, max_backup_files_);
        cloned->setFormatter(formatter_->clone());
        return std::move(cloned);
    }
};

// ===== ROTATING FILE APPENDER =====

class RotatingFileAppender : public LogAppender {
private:
    std::unique_ptr<LogFormatter> formatter_;
    std::ofstream file_;
    std::string base_filename_;
    std::mutex mutex_;
    std::chrono::system_clock::time_point last_rotation_;
    std::chrono::hours rotation_interval_;
    
    void rotateFile() {
        if (file_.is_open()) {
            file_.close();
        }
        
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        
        std::ostringstream oss;
        oss << base_filename_ << "."
            << std::put_time(std::localtime(&time_t), "%Y%m%d_%H%M%S")
            << ".log";
        
        file_.open(oss.str(), std::ios::app);
        last_rotation_ = now;
    }
    
public:
    explicit RotatingFileAppender(const std::string& base_filename,
                                 std::chrono::hours interval = std::chrono::hours(24))
        : formatter_(std::make_unique<DefaultFormatter>())
        , base_filename_(base_filename)
        , last_rotation_(std::chrono::system_clock::now())
        , rotation_interval_(interval) {
        
        rotateFile();
    }
    
    ~RotatingFileAppender() {
        if (file_.is_open()) {
            file_.close();
        }
    }
    
    void append(const LogEntry& entry) override {
        std::lock_guard<std::mutex> lock(mutex_);
        
        // Check if rotation is needed
        auto now = std::chrono::system_clock::now();
        if (now - last_rotation_ >= rotation_interval_) {
            rotateFile();
        }
        
        if (file_.is_open()) {
            file_ << formatter_->format(entry) << std::endl;
        }
    }
    
    void flush() override {
        std::lock_guard<std::mutex> lock(mutex_);
        if (file_.is_open()) {
            file_.flush();
        }
    }
    
    void setFormatter(std::unique_ptr<LogFormatter> formatter) override {
        std::lock_guard<std::mutex> lock(mutex_);
        formatter_ = std::move(formatter);
    }
    
    std::unique_ptr<LogAppender> clone() const override {
        auto cloned = std::make_unique<RotatingFileAppender>(base_filename_, rotation_interval_);
        cloned->setFormatter(formatter_->clone());
        return std::move(cloned);
    }
};

// ===== ASYNC LOGGER IMPLEMENTATION =====

class AsyncLogger {
private:
    std::queue<LogEntry> log_queue_;
    std::vector<std::unique_ptr<LogAppender>> appenders_;
    std::mutex queue_mutex_;
    std::condition_variable condition_;
    std::thread worker_thread_;
    std::atomic<bool> shutdown_;
    LogLevel min_level_;
    std::string name_;
    
    void workerFunction() {
        while (!shutdown_.load()) {
            std::unique_lock<std::mutex> lock(queue_mutex_);
            condition_.wait(lock, [this]() { return !log_queue_.empty() || shutdown_.load(); });
            
            while (!log_queue_.empty()) {
                LogEntry entry = std::move(log_queue_.front());
                log_queue_.pop();
                lock.unlock();
                
                // Write to all appenders
                for (auto& appender : appenders_) {
                    try {
                        appender->append(entry);
                    } catch (const std::exception& e) {
                        std::cerr << "Logger error in appender: " << e.what() << std::endl;
                    }
                }
                
                lock.lock();
            }
        }
        
        // Flush all appenders on shutdown
        for (auto& appender : appenders_) {
            appender->flush();
        }
    }
    
public:
    explicit AsyncLogger(const std::string& name, LogLevel min_level = LogLevel::INFO)
        : shutdown_(false), min_level_(min_level), name_(name) {
        worker_thread_ = std::thread(&AsyncLogger::workerFunction, this);
    }
    
    ~AsyncLogger() {
        shutdown();
    }
    
    void shutdown() {
        if (!shutdown_.load()) {
            shutdown_.store(true);
            condition_.notify_all();
            if (worker_thread_.joinable()) {
                worker_thread_.join();
            }
        }
    }
    
    void addAppender(std::unique_ptr<LogAppender> appender) {
        std::lock_guard<std::mutex> lock(queue_mutex_);
        appenders_.push_back(std::move(appender));
    }
    
    void setLogLevel(LogLevel level) {
        min_level_ = level;
    }
    
    LogLevel getLogLevel() const {
        return min_level_;
    }
    
    bool shouldLog(LogLevel level) const {
        return level >= min_level_;
    }
    
    void log(LogLevel level, const std::string& message,
             const std::string& file = "", const std::string& function = "", int line = 0) {
        if (!shouldLog(level)) return;
        
        LogEntry entry(level, name_, message, file, function, line);
        
        {
            std::lock_guard<std::mutex> lock(queue_mutex_);
            log_queue_.push(std::move(entry));
        }
        condition_.notify_one();
    }
    
    // Convenience methods
    void trace(const std::string& message, const std::string& file = "", 
               const std::string& function = "", int line = 0) {
        log(LogLevel::TRACE, message, file, function, line);
    }
    
    void debug(const std::string& message, const std::string& file = "", 
               const std::string& function = "", int line = 0) {
        log(LogLevel::DEBUG, message, file, function, line);
    }
    
    void info(const std::string& message, const std::string& file = "", 
              const std::string& function = "", int line = 0) {
        log(LogLevel::INFO, message, file, function, line);
    }
    
    void warn(const std::string& message, const std::string& file = "", 
              const std::string& function = "", int line = 0) {
        log(LogLevel::WARN, message, file, function, line);
    }
    
    void error(const std::string& message, const std::string& file = "", 
               const std::string& function = "", int line = 0) {
        log(LogLevel::ERROR, message, file, function, line);
    }
    
    void fatal(const std::string& message, const std::string& file = "", 
               const std::string& function = "", int line = 0) {
        log(LogLevel::FATAL, message, file, function, line);
    }
    
    void flush() {
        std::lock_guard<std::mutex> lock(queue_mutex_);
        for (auto& appender : appenders_) {
            appender->flush();
        }
    }
    
    const std::string& getName() const { return name_; }
};

// ===== LOGGER MANAGER =====

class LoggerManager {
private:
    std::unordered_map<std::string, std::unique_ptr<AsyncLogger>> loggers_;
    std::mutex loggers_mutex_;
    LogLevel default_level_;
    
    LoggerManager() : default_level_(LogLevel::INFO) {}
    
public:
    static LoggerManager& getInstance() {
        static LoggerManager instance;
        return instance;
    }
    
    // Delete copy constructor and assignment operator
    LoggerManager(const LoggerManager&) = delete;
    LoggerManager& operator=(const LoggerManager&) = delete;
    
    std::shared_ptr<AsyncLogger> getLogger(const std::string& name) {
        std::lock_guard<std::mutex> lock(loggers_mutex_);
        
        auto it = loggers_.find(name);
        if (it != loggers_.end()) {
            return std::shared_ptr<AsyncLogger>(it->second.get(), [](AsyncLogger*){});
        }
        
        // Create new logger
        auto logger = std::make_unique<AsyncLogger>(name, default_level_);
        auto* logger_ptr = logger.get();
        loggers_[name] = std::move(logger);
        
        return std::shared_ptr<AsyncLogger>(logger_ptr, [](AsyncLogger*){});
    }
    
    void setDefaultLogLevel(LogLevel level) {
        std::lock_guard<std::mutex> lock(loggers_mutex_);
        default_level_ = level;
        
        for (auto& pair : loggers_) {
            pair.second->setLogLevel(level);
        }
    }
    
    void shutdown() {
        std::lock_guard<std::mutex> lock(loggers_mutex_);
        for (auto& pair : loggers_) {
            pair.second->shutdown();
        }
        loggers_.clear();
    }
    
    size_t getLoggerCount() const {
        std::lock_guard<std::mutex> lock(loggers_mutex_);
        return loggers_.size();
    }
};

// ===== LOGGING MACROS =====

#define LOG_GET_LOGGER(name) CppVerseHub::Utils::LoggerManager::getInstance().getLogger(name)

#define LOG_TRACE(logger, message) \
    do { if ((logger)->shouldLog(CppVerseHub::Utils::LogLevel::TRACE)) { \
        (logger)->trace(message, __FILE__, __FUNCTION__, __LINE__); } } while(0)

#define LOG_DEBUG(logger, message) \
    do { if ((logger)->shouldLog(CppVerseHub::Utils::LogLevel::DEBUG)) { \
        (logger)->debug(message, __FILE__, __FUNCTION__, __LINE__); } } while(0)

#define LOG_INFO(logger, message) \
    do { if ((logger)->shouldLog(CppVerseHub::Utils::LogLevel::INFO)) { \
        (logger)->info(message, __FILE__, __FUNCTION__, __LINE__); } } while(0)

#define LOG_WARN(logger, message) \
    do { if ((logger)->shouldLog(CppVerseHub::Utils::LogLevel::WARN)) { \
        (logger)->warn(message, __FILE__, __FUNCTION__, __LINE__); } } while(0)

#define LOG_ERROR(logger, message) \
    do { if ((logger)->shouldLog(CppVerseHub::Utils::LogLevel::ERROR)) { \
        (logger)->error(message, __FILE__, __FUNCTION__, __LINE__); } } while(0)

#define LOG_FATAL(logger, message) \
    do { if ((logger)->shouldLog(CppVerseHub::Utils::LogLevel::FATAL)) { \
        (logger)->fatal(message, __FILE__, __FUNCTION__, __LINE__); } } while(0)

// ===== SCOPED LOGGER FOR RAII =====

class ScopedLogger {
private:
    std::shared_ptr<AsyncLogger> logger_;
    std::string function_name_;
    std::chrono::high_resolution_clock::time_point start_time_;
    
public:
    ScopedLogger(std::shared_ptr<AsyncLogger> logger, const std::string& function_name)
        : logger_(std::move(logger)), function_name_(function_name),
          start_time_(std::chrono::high_resolution_clock::now()) {
        LOG_TRACE(logger_, "Entering function: " + function_name_);
    }
    
    ~ScopedLogger() {
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time_);
        
        std::ostringstream oss;
        oss << "Exiting function: " << function_name_ 
            << " (took " << duration.count() << " μs)";
        LOG_TRACE(logger_, oss.str());
    }
};

#define LOG_FUNCTION_SCOPE(logger) \
    CppVerseHub::Utils::ScopedLogger _scoped_logger((logger), __FUNCTION__)

} // namespace CppVerseHub::Utils