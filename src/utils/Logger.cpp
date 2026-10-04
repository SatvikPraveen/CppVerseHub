/**
 * @file Logger.cpp
 * @brief Implementation of the thread-safe logging subsystem declared in Logger.hpp.
 */
#include "utils/Logger.hpp"

#include "utils/TimeUtils.hpp"

#include <algorithm>
#include <cctype>
#include <stdexcept>
#include <system_error>

namespace CppVerseHub::Utils {

std::optional<LogLevel> parseLogLevel(std::string_view text) noexcept {
    std::string upper;
    upper.reserve(text.size());
    for (const char c : text) {
        if (std::isspace(static_cast<unsigned char>(c)) == 0) {
            upper.push_back(static_cast<char>(std::toupper(static_cast<unsigned char>(c))));
        }
    }
    if (upper == "TRACE") {
        return LogLevel::Trace;
    }
    if (upper == "DEBUG") {
        return LogLevel::Debug;
    }
    if (upper == "INFO") {
        return LogLevel::Info;
    }
    if (upper == "WARN" || upper == "WARNING") {
        return LogLevel::Warn;
    }
    if (upper == "ERROR") {
        return LogLevel::Error;
    }
    if (upper == "FATAL" || upper == "CRITICAL") {
        return LogLevel::Fatal;
    }
    if (upper == "OFF" || upper == "NONE") {
        return LogLevel::Off;
    }
    return std::nullopt;
}

namespace {

std::string threadIdString(std::thread::id id) {
    std::ostringstream oss;
    oss << id;
    return oss.str();
}

std::string_view baseName(std::string_view path) noexcept {
    const auto pos = path.find_last_of("/\\");
    return pos == std::string_view::npos ? path : path.substr(pos + 1);
}

} // namespace

// ===================================================================================================
// Formatters
// ===================================================================================================

std::string PatternFormatter::format(const LogRecord& record) const {
    std::string line;
    line.reserve(record.message.size() + 64);
    auto separate = [&line] {
        if (!line.empty()) {
            line.push_back(' ');
        }
    };
    if (options_.timestamp) {
        line += Time::formatIso8601(record.timestamp);
    }
    if (options_.level) {
        separate();
        line.push_back('[');
        line += toString(record.level);
        line.push_back(']');
    }
    if (options_.loggerName && !record.loggerName.empty()) {
        separate();
        line.push_back('[');
        line += record.loggerName;
        line.push_back(']');
    }
    if (options_.threadId) {
        separate();
        line += "[tid ";
        line += threadIdString(record.threadId);
        line.push_back(']');
    }
    if (options_.location) {
        separate();
        line.push_back('(');
        line += baseName(record.location.file_name());
        line.push_back(':');
        line += std::to_string(record.location.line());
        line.push_back(')');
    }
    separate();
    line += record.message;
    return line;
}

std::string JsonFormatter::escape(std::string_view text) {
    static constexpr char kHex[] = "0123456789abcdef";
    std::string out;
    out.reserve(text.size() + 8);
    for (const char c : text) {
        switch (c) {
            case '"':
                out += "\\\"";
                break;
            case '\\':
                out += "\\\\";
                break;
            case '\b':
                out += "\\b";
                break;
            case '\f':
                out += "\\f";
                break;
            case '\n':
                out += "\\n";
                break;
            case '\r':
                out += "\\r";
                break;
            case '\t':
                out += "\\t";
                break;
            default: {
                const auto u = static_cast<unsigned char>(c);
                if (u < 0x20U) {
                    out += "\\u00";
                    out.push_back(kHex[u >> 4U]);
                    out.push_back(kHex[u & 0x0FU]);
                } else {
                    out.push_back(c);
                }
            }
        }
    }
    return out;
}

std::string JsonFormatter::format(const LogRecord& record) const {
    std::string line = R"({"timestamp":")";
    line += Time::formatIso8601(record.timestamp);
    line += R"(","level":")";
    line += toString(record.level);
    line += R"(","logger":")";
    line += escape(record.loggerName);
    line += R"(","message":")";
    line += escape(record.message);
    line += R"(","thread":")";
    line += escape(threadIdString(record.threadId));
    line += R"(","file":")";
    line += escape(baseName(record.location.file_name()));
    line += R"(","line":)";
    line += std::to_string(record.location.line());
    line += '}';
    return line;
}

// ===================================================================================================
// Sink base
// ===================================================================================================

Sink::Sink() : formatter_(std::make_shared<PatternFormatter>()) {}

void Sink::write(const LogRecord& record) {
    if (record.level == LogLevel::Off || record.level < level()) {
        return;
    }
    const std::lock_guard lock(mutex_);
    if (wantsFormattedLine()) {
        const std::string line = formatter_->format(record);
        consume(line, record);
    } else {
        consume({}, record);
    }
    written_.fetch_add(1, std::memory_order_relaxed);
}

void Sink::flush() {
    const std::lock_guard lock(mutex_);
    doFlush();
}

void Sink::setFormatter(std::shared_ptr<const Formatter> formatter) {
    const std::lock_guard lock(mutex_);
    formatter_ = formatter ? std::move(formatter) : std::make_shared<PatternFormatter>();
}

// ===================================================================================================
// Concrete sinks
// ===================================================================================================

void OStreamSink::consume(std::string_view line, const LogRecord& /*record*/) {
    *stream_ << line << '\n';
    if (flushEachLine_) {
        stream_->flush();
    }
}

void OStreamSink::doFlush() {
    stream_->flush();
}

std::string StringSink::str() const {
    const std::lock_guard lock(mutex_);
    std::string all;
    for (const auto& l : lines_) {
        all += l;
        all.push_back('\n');
    }
    return all;
}

std::vector<std::string> StringSink::lines() const {
    const std::lock_guard lock(mutex_);
    return lines_;
}

void StringSink::clear() {
    const std::lock_guard lock(mutex_);
    lines_.clear();
}

void StringSink::consume(std::string_view line, const LogRecord& /*record*/) {
    lines_.emplace_back(line);
}

RingBufferSink::RingBufferSink(std::size_t capacity) : capacity_(std::max<std::size_t>(capacity, 1)) {}

std::vector<std::string> RingBufferSink::lines() const {
    const std::lock_guard lock(mutex_);
    return {buffer_.begin(), buffer_.end()};
}

void RingBufferSink::consume(std::string_view line, const LogRecord& /*record*/) {
    if (buffer_.size() == capacity_) {
        buffer_.pop_front();
    }
    buffer_.emplace_back(line);
}

void CallbackSink::consume(std::string_view line, const LogRecord& record) {
    if (callback_) {
        callback_(line, record);
    }
}

FileSink::FileSink(std::filesystem::path path, std::uintmax_t maxBytes, unsigned maxBackups)
    : path_(std::move(path)), maxBytes_(maxBytes), maxBackups_(maxBackups) {
    if (path_.has_parent_path()) {
        std::error_code ec;
        std::filesystem::create_directories(path_.parent_path(), ec);
    }
    file_.open(path_, std::ios::out | std::ios::app);
    if (!file_.is_open()) {
        throw std::runtime_error("FileSink: cannot open log file '" + path_.string() + "'");
    }
    std::error_code ec;
    const auto size = std::filesystem::file_size(path_, ec);
    currentBytes_ = ec ? 0 : size;
}

std::size_t FileSink::rotations() const {
    const std::lock_guard lock(mutex_);
    return rotations_;
}

void FileSink::consume(std::string_view line, const LogRecord& /*record*/) {
    if (!file_.is_open()) {
        return;
    }
    file_ << line << '\n';
    currentBytes_ += line.size() + 1;
    if (maxBytes_ > 0 && currentBytes_ >= maxBytes_) {
        rotate();
    }
}

void FileSink::doFlush() {
    if (file_.is_open()) {
        file_.flush();
    }
}

void FileSink::rotate() {
    file_.close();
    std::error_code ec;
    auto backup = [this](unsigned index) {
        std::filesystem::path p = path_;
        p += "." + std::to_string(index);
        return p;
    };
    if (maxBackups_ == 0) {
        std::filesystem::remove(path_, ec);
    } else {
        std::filesystem::remove(backup(maxBackups_), ec);
        for (unsigned i = maxBackups_; i > 1; --i) {
            if (std::filesystem::exists(backup(i - 1), ec)) {
                std::filesystem::rename(backup(i - 1), backup(i), ec);
            }
        }
        std::filesystem::rename(path_, backup(1), ec);
    }
    file_.open(path_, std::ios::out | std::ios::trunc);
    currentBytes_ = 0;
    ++rotations_;
}

AsyncSink::AsyncSink(std::shared_ptr<Sink> target, std::size_t maxQueue)
    : target_(std::move(target)), maxQueue_(std::max<std::size_t>(maxQueue, 1)) {
    if (!target_) {
        throw std::invalid_argument("AsyncSink: target sink must not be null");
    }
    worker_ = std::thread([this] { run(); });
}

AsyncSink::~AsyncSink() {
    {
        const std::lock_guard lock(queueMutex_);
        stopping_ = true;
    }
    queueCv_.notify_all();
    if (worker_.joinable()) {
        worker_.join();
    }
}

void AsyncSink::consume(std::string_view /*line*/, const LogRecord& record) {
    {
        const std::lock_guard lock(queueMutex_);
        if (queue_.size() >= maxQueue_) {
            queue_.pop_front();
            dropped_.fetch_add(1, std::memory_order_relaxed);
        }
        queue_.push_back(record);
    }
    queueCv_.notify_one();
}

void AsyncSink::doFlush() {
    {
        std::unique_lock lock(queueMutex_);
        idleCv_.wait(lock, [this] { return queue_.empty() && !busy_; });
    }
    target_->flush();
}

void AsyncSink::run() {
    std::unique_lock lock(queueMutex_);
    while (true) {
        queueCv_.wait(lock, [this] { return stopping_ || !queue_.empty(); });
        if (queue_.empty()) {
            if (stopping_) {
                break;
            }
            continue;
        }
        LogRecord record = std::move(queue_.front());
        queue_.pop_front();
        busy_ = true;
        lock.unlock();
        try {
            target_->write(record);
        } catch (...) { // NOLINT(bugprone-empty-catch): a failing sink must not kill the worker
        }
        lock.lock();
        busy_ = false;
        if (queue_.empty()) {
            idleCv_.notify_all();
        }
    }
    lock.unlock();
    try {
        target_->flush();
    } catch (...) { // NOLINT(bugprone-empty-catch)
    }
}

// ===================================================================================================
// Logger
// ===================================================================================================

Logger::Logger(std::string name, LogLevel level) : name_(std::move(name)), level_(level) {}

void Logger::addSink(std::shared_ptr<Sink> sink) {
    if (!sink) {
        return;
    }
    const std::unique_lock lock(sinksMutex_);
    sinks_.push_back(std::move(sink));
    sinkCount_.store(sinks_.size(), std::memory_order_relaxed);
}

bool Logger::removeSink(const std::shared_ptr<Sink>& sink) {
    const std::unique_lock lock(sinksMutex_);
    const auto it = std::find(sinks_.begin(), sinks_.end(), sink);
    if (it == sinks_.end()) {
        return false;
    }
    sinks_.erase(it);
    sinkCount_.store(sinks_.size(), std::memory_order_relaxed);
    return true;
}

void Logger::clearSinks() {
    const std::unique_lock lock(sinksMutex_);
    sinks_.clear();
    sinkCount_.store(0, std::memory_order_relaxed);
}

std::vector<std::shared_ptr<Sink>> Logger::snapshotSinks() const {
    const std::shared_lock lock(sinksMutex_);
    return sinks_;
}

void Logger::log(LogLevel messageLevel, std::string_view message, std::source_location location) {
    if (!shouldLog(messageLevel)) {
        return;
    }
    const auto sinks = snapshotSinks();
    if (sinks.empty()) {
        return;
    }
    LogRecord record;
    record.timestamp = std::chrono::system_clock::now();
    record.level = messageLevel;
    record.loggerName = name_;
    record.message = std::string{message};
    record.location = location;
    record.threadId = std::this_thread::get_id();

    counts_[static_cast<std::size_t>(messageLevel)].fetch_add(1, std::memory_order_relaxed);
    for (const auto& sink : sinks) {
        try {
            sink->write(record);
        } catch (...) {
            sinkErrors_.fetch_add(1, std::memory_order_relaxed);
        }
    }
}

void Logger::flush() {
    for (const auto& sink : snapshotSinks()) {
        try {
            sink->flush();
        } catch (...) {
            sinkErrors_.fetch_add(1, std::memory_order_relaxed);
        }
    }
}

std::size_t Logger::count(LogLevel messageLevel) const noexcept {
    const auto index = static_cast<std::size_t>(messageLevel);
    return index < counts_.size() ? counts_[index].load(std::memory_order_relaxed) : 0;
}

// ===================================================================================================
// LoggerRegistry
// ===================================================================================================

LoggerRegistry& LoggerRegistry::instance() {
    static LoggerRegistry registry;
    return registry;
}

std::shared_ptr<Logger> LoggerRegistry::get(const std::string& name) {
    const std::lock_guard lock(mutex_);
    auto& slot = loggers_[name];
    if (!slot) {
        slot = std::make_shared<Logger>(name, defaultLevel_);
    }
    return slot;
}

bool LoggerRegistry::contains(const std::string& name) const {
    const std::lock_guard lock(mutex_);
    return loggers_.find(name) != loggers_.end();
}

void LoggerRegistry::setGlobalLevel(LogLevel level) {
    const std::lock_guard lock(mutex_);
    defaultLevel_ = level;
    for (auto& [name, logger] : loggers_) {
        logger->setLevel(level);
    }
}

bool LoggerRegistry::remove(const std::string& name) {
    const std::lock_guard lock(mutex_);
    return loggers_.erase(name) > 0;
}

void LoggerRegistry::clear() {
    const std::lock_guard lock(mutex_);
    loggers_.clear();
}

std::size_t LoggerRegistry::size() const {
    const std::lock_guard lock(mutex_);
    return loggers_.size();
}

// ===================================================================================================
// ScopedLogTimer
// ===================================================================================================

ScopedLogTimer::ScopedLogTimer(Logger& logger, std::string label, LogLevel level)
    : logger_(&logger), label_(std::move(label)), level_(level), start_(std::chrono::steady_clock::now()) {}

ScopedLogTimer::~ScopedLogTimer() {
    try {
        if (logger_->shouldLog(level_)) {
            const auto us = std::chrono::duration_cast<std::chrono::microseconds>(
                std::chrono::steady_clock::now() - start_);
            logger_->log(level_, detail::concat(label_, " took ", us.count(), " us"));
        }
    } catch (...) { // NOLINT(bugprone-empty-catch): destructors must not throw
    }
}

// ===================================================================================================
// Demo
// ===================================================================================================

void demonstrateLogging(std::ostream& out) {
    out << "=== Logging ===\n";

    Logger silent{"silent"};
    silent.info("nobody hears this");
    out << "Logger without sinks emitted " << silent.count(LogLevel::Info)
        << " records (silent by default)\n";

    // Human-readable output straight to the caller's stream (timestamps omitted for determinism).
    Logger logger{"demo", LogLevel::Debug};
    auto streamSink = std::make_shared<OStreamSink>(out);
    PatternFormatter::Options opts;
    opts.timestamp = false;
    streamSink->setFormatter(std::make_shared<PatternFormatter>(opts));
    logger.addSink(streamSink);

    logger.trace("filtered out: below Debug");
    logger.debug("debug message");
    logger.info("fleet ready");
    logger.logArgs(LogLevel::Warn, "fuel at ", 12.5, "%");
    CPPVERSEHUB_LOG(logger, LogLevel::Error, "shield failure on deck ", 7);

    // Per-sink filtering and JSON formatting.
    auto jsonSink = std::make_shared<StringSink>();
    jsonSink->setFormatter(std::make_shared<JsonFormatter>());
    jsonSink->setLevel(LogLevel::Error);
    logger.addSink(jsonSink);
    logger.error("reactor \"core\" overheating");
    out << "JSON sink captured: " << jsonSink->lines().size() << " line(s)\n";

    // Asynchronous fan-in from several threads into a ring buffer.
    auto ring = std::make_shared<RingBufferSink>(5);
    {
        Logger asyncLogger{"async", LogLevel::Info};
        auto async = std::make_shared<AsyncSink>(ring);
        asyncLogger.addSink(async);
        std::vector<std::thread> threads;
        for (int t = 0; t < 4; ++t) {
            threads.emplace_back([&asyncLogger, t] {
                for (int i = 0; i < 25; ++i) {
                    asyncLogger.logArgs(LogLevel::Info, "thread ", t, " message ", i);
                }
            });
        }
        for (auto& th : threads) {
            th.join();
        }
        asyncLogger.flush();
        out << "AsyncSink delivered " << ring->recordsWritten() << " records; ring keeps last "
            << ring->lines().size() << '\n';
    }

    { ScopedLogTimer timer{logger, "scoped work", LogLevel::Off}; }
    out << "Records emitted by 'demo': info=" << logger.count(LogLevel::Info)
        << " warn=" << logger.count(LogLevel::Warn) << " error=" << logger.count(LogLevel::Error) << '\n';
    logger.flush();
}

} // namespace CppVerseHub::Utils
