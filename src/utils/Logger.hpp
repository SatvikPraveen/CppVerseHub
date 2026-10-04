/**
 * @file Logger.hpp
 * @brief Thread-safe, level-filtered logging with pluggable formatters and sinks.
 *
 * Demonstrates several classic C++ design techniques in one small subsystem:
 *  - the Non-Virtual Interface (NVI) idiom: `Sink::write()` is public and non-virtual, it takes the
 *    lock, filters by level and formats, then calls the private virtual `consume()`;
 *  - the Strategy pattern for formatting (`Formatter`, `PatternFormatter`, `JsonFormatter`);
 *  - a producer/consumer queue (`AsyncSink`) that moves I/O off the calling thread;
 *  - `std::source_location` instead of `__FILE__`/`__LINE__` macros for call-site capture;
 *  - lock-free level checks through `std::atomic` so disabled log statements cost one load.
 *
 * A `Logger` without sinks is completely silent; nothing is ever written to `std::cout` unless the
 * caller attaches an `OStreamSink` for it explicitly.
 */
#pragma once

#include <array>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <memory>
#include <mutex>
#include <optional>
#include <ostream>
#include <shared_mutex>
#include <source_location>
#include <sstream>
#include <string>
#include <string_view>
#include <thread>
#include <unordered_map>
#include <utility>
#include <vector>

namespace CppVerseHub::Utils {

/// @brief Severity of a log record, ordered from most verbose to most severe.
enum class LogLevel : std::uint8_t { Trace = 0, Debug, Info, Warn, Error, Fatal, Off };

/// @brief Number of real (non-`Off`) log levels.
inline constexpr std::size_t kLogLevelCount = 6;

/**
 * @brief Converts a level to its canonical upper-case name.
 * @param level The level to convert.
 * @return "TRACE", "DEBUG", "INFO", "WARN", "ERROR", "FATAL" or "OFF".
 */
[[nodiscard]] constexpr std::string_view toString(LogLevel level) noexcept {
    switch (level) {
    case LogLevel::Trace: return "TRACE";
    case LogLevel::Debug: return "DEBUG";
    case LogLevel::Info: return "INFO";
    case LogLevel::Warn: return "WARN";
    case LogLevel::Error: return "ERROR";
    case LogLevel::Fatal: return "FATAL";
    case LogLevel::Off: return "OFF";
    }
    return "UNKNOWN";
}

/**
 * @brief Parses a level name case-insensitively ("warn", "WARNING", "Info", ...).
 * @param text The text to parse.
 * @return The level, or `std::nullopt` if the name is not recognised.
 */
[[nodiscard]] std::optional<LogLevel> parseLogLevel(std::string_view text) noexcept;

/// @brief One log event, captured at the call site and handed to every sink.
struct LogRecord {
    std::chrono::system_clock::time_point timestamp{}; ///< Wall-clock time of the call.
    LogLevel level{LogLevel::Info};                     ///< Severity.
    std::string loggerName;                             ///< Name of the emitting logger.
    std::string message;                                ///< Fully formatted message text.
    std::source_location location{};                   ///< Call site.
    std::thread::id threadId{};                         ///< Emitting thread.
};

// ===================================================================================================
// Formatters
// ===================================================================================================

/// @brief Strategy interface that turns a record into a single line of text (without newline).
class Formatter {
public:
    virtual ~Formatter() = default;
    Formatter() = default;
    Formatter(const Formatter&) = default;
    Formatter& operator=(const Formatter&) = default;
    Formatter(Formatter&&) = default;
    Formatter& operator=(Formatter&&) = default;

    /**
     * @brief Formats a record.
     * @param record The record to format.
     * @return The formatted line.
     */
    [[nodiscard]] virtual std::string format(const LogRecord& record) const = 0;
};

/// @brief Human-readable "timestamp [LEVEL] [name] message" formatter with optional fields.
class PatternFormatter final : public Formatter {
public:
    /// @brief Which optional fields are emitted.
    struct Options {
        bool timestamp = true;   ///< ISO-8601 UTC timestamp with milliseconds.
        bool level = true;       ///< "[INFO]".
        bool loggerName = true;  ///< "[name]" (omitted when the name is empty).
        bool threadId = false;   ///< "[tid]".
        bool location = false;   ///< "(file:line)".
    };

    /// @brief Constructs a formatter with default options.
    PatternFormatter() = default;

    /**
     * @brief Constructs a formatter with the given options.
     * @param options Fields to emit.
     */
    explicit PatternFormatter(Options options) noexcept : options_(options) {}

    /// @copydoc Formatter::format
    [[nodiscard]] std::string format(const LogRecord& record) const override;

    /// @brief Returns the configured options.
    /// @return The options.
    [[nodiscard]] const Options& options() const noexcept { return options_; }

private:
    Options options_{};
};

/// @brief Emits one JSON object per record (JSON Lines), suitable for log shippers.
class JsonFormatter final : public Formatter {
public:
    /// @copydoc Formatter::format
    [[nodiscard]] std::string format(const LogRecord& record) const override;

    /**
     * @brief Escapes a string for inclusion inside a JSON string literal.
     * @param text Raw text.
     * @return Escaped text (without surrounding quotes).
     */
    [[nodiscard]] static std::string escape(std::string_view text);
};

// ===================================================================================================
// Sinks
// ===================================================================================================

/**
 * @brief Destination for log records.
 *
 * NVI: `write()` serialises access with the sink's own mutex, applies the sink's level filter and its
 * formatter, then hands the finished line to the private virtual `consume()`. Derived classes therefore
 * never need their own locking for `consume()`/`doFlush()`.
 */
class Sink {
public:
    /// @brief Creates a sink using a default `PatternFormatter` and accepting every level.
    Sink();
    virtual ~Sink() = default;
    Sink(const Sink&) = delete;
    Sink& operator=(const Sink&) = delete;
    Sink(Sink&&) = delete;
    Sink& operator=(Sink&&) = delete;

    /**
     * @brief Formats and writes a record if it passes the sink's level filter. Thread-safe.
     * @param record The record.
     */
    void write(const LogRecord& record);

    /// @brief Flushes buffered output. Thread-safe.
    void flush();

    /**
     * @brief Replaces the formatter. Thread-safe.
     * @param formatter New formatter; `nullptr` restores the default `PatternFormatter`.
     */
    void setFormatter(std::shared_ptr<const Formatter> formatter);

    /**
     * @brief Sets the minimum level this sink accepts (independent of the logger's level).
     * @param level Minimum level.
     */
    void setLevel(LogLevel level) noexcept { level_.store(level, std::memory_order_relaxed); }

    /// @brief Returns the sink's minimum level.
    /// @return The level.
    [[nodiscard]] LogLevel level() const noexcept { return level_.load(std::memory_order_relaxed); }

    /// @brief Returns how many records this sink has accepted.
    /// @return The count.
    [[nodiscard]] std::size_t recordsWritten() const noexcept {
        return written_.load(std::memory_order_relaxed);
    }

protected:
    /// @brief Mutex guarding the sink's state; derived accessors may lock it too.
    mutable std::mutex mutex_;

private:
    /**
     * @brief Receives a formatted line. Called with `mutex_` held.
     * @param line Formatted line without trailing newline.
     * @param record The original record.
     */
    virtual void consume(std::string_view line, const LogRecord& record) = 0;

    /// @brief Flushes any buffered output. Called with `mutex_` held.
    virtual void doFlush() {}

    /// @brief Whether `write()` should run the formatter (decorators that forward records skip it).
    /// @return True by default.
    [[nodiscard]] virtual bool wantsFormattedLine() const noexcept { return true; }

    std::shared_ptr<const Formatter> formatter_;
    std::atomic<LogLevel> level_{LogLevel::Trace};
    std::atomic<std::size_t> written_{0};
};

/// @brief Writes lines to a caller-owned `std::ostream` (e.g. `std::cout` or a `std::ostringstream`).
class OStreamSink final : public Sink {
public:
    /**
     * @brief Creates a sink over a stream. The stream must outlive the sink.
     * @param stream Destination stream.
     * @param flushEachLine Whether to flush after every line.
     */
    explicit OStreamSink(std::ostream& stream, bool flushEachLine = false) noexcept
        : stream_(&stream), flushEachLine_(flushEachLine) {}

private:
    void consume(std::string_view line, const LogRecord& record) override;
    void doFlush() override;

    std::ostream* stream_;
    bool flushEachLine_;
};

/// @brief Accumulates every line in an internal string buffer; ideal for tests and demos.
class StringSink final : public Sink {
public:
    /// @brief Returns everything written so far (newline-separated). Thread-safe.
    /// @return A copy of the buffer.
    [[nodiscard]] std::string str() const;

    /// @brief Returns the individual lines written so far. Thread-safe.
    /// @return Copy of the lines.
    [[nodiscard]] std::vector<std::string> lines() const;

    /// @brief Discards everything written so far. Thread-safe.
    void clear();

private:
    void consume(std::string_view line, const LogRecord& record) override;

    std::vector<std::string> lines_;
};

/// @brief Keeps only the most recent `capacity` lines (a "flight recorder").
class RingBufferSink final : public Sink {
public:
    /**
     * @brief Creates a ring buffer.
     * @param capacity Maximum number of retained lines (at least 1).
     */
    explicit RingBufferSink(std::size_t capacity);

    /// @brief Returns the retained lines, oldest first. Thread-safe.
    /// @return Copy of the lines.
    [[nodiscard]] std::vector<std::string> lines() const;

    /// @brief Returns the capacity.
    /// @return The capacity.
    [[nodiscard]] std::size_t capacity() const noexcept { return capacity_; }

private:
    void consume(std::string_view line, const LogRecord& record) override;

    std::size_t capacity_;
    std::deque<std::string> buffer_;
};

/// @brief Forwards each record to a user callback (bridge to other logging systems).
class CallbackSink final : public Sink {
public:
    /// @brief Callback signature: formatted line and original record.
    using Callback = std::function<void(std::string_view line, const LogRecord& record)>;

    /**
     * @brief Creates a callback sink.
     * @param callback Function invoked for every accepted record (under the sink's lock).
     */
    explicit CallbackSink(Callback callback) : callback_(std::move(callback)) {}

private:
    void consume(std::string_view line, const LogRecord& record) override;

    Callback callback_;
};

/// @brief Appends lines to a file, rotating to `file.1`, `file.2`, ... once a size limit is reached.
class FileSink final : public Sink {
public:
    /**
     * @brief Opens (or creates) the file in append mode, creating parent directories.
     * @param path Log file path.
     * @param maxBytes Rotate once the file grows beyond this many bytes (0 = never rotate).
     * @param maxBackups Number of rotated backups to keep.
     * @throws std::runtime_error if the file cannot be opened.
     */
    explicit FileSink(std::filesystem::path path, std::uintmax_t maxBytes = 0, unsigned maxBackups = 3);

    /// @brief Returns the log file path.
    /// @return The path.
    [[nodiscard]] const std::filesystem::path& path() const noexcept { return path_; }

    /// @brief Returns how many times the file has been rotated.
    /// @return Rotation count.
    [[nodiscard]] std::size_t rotations() const;

private:
    void consume(std::string_view line, const LogRecord& record) override;
    void doFlush() override;
    void rotate();

    std::filesystem::path path_;
    std::uintmax_t maxBytes_;
    unsigned maxBackups_;
    std::ofstream file_;
    std::uintmax_t currentBytes_ = 0;
    std::size_t rotations_ = 0;
};

/**
 * @brief Decorator that moves writes to a background thread (producer/consumer queue).
 *
 * Records are copied into a bounded queue; when the queue is full the oldest record is dropped and
 * counted, so logging never blocks the caller for long. The destructor drains the queue.
 */
class AsyncSink final : public Sink {
public:
    /**
     * @brief Starts the worker thread.
     * @param target Sink that receives records on the worker thread (must not be null).
     * @param maxQueue Maximum queued records before dropping the oldest.
     */
    explicit AsyncSink(std::shared_ptr<Sink> target, std::size_t maxQueue = 8192);

    /// @brief Drains outstanding records and joins the worker.
    ~AsyncSink() override;
    AsyncSink(const AsyncSink&) = delete;
    AsyncSink& operator=(const AsyncSink&) = delete;
    AsyncSink(AsyncSink&&) = delete;
    AsyncSink& operator=(AsyncSink&&) = delete;

    /// @brief Returns the number of records dropped because the queue was full.
    /// @return Dropped count.
    [[nodiscard]] std::size_t dropped() const noexcept { return dropped_.load(std::memory_order_relaxed); }

private:
    void consume(std::string_view line, const LogRecord& record) override;
    void doFlush() override;
    [[nodiscard]] bool wantsFormattedLine() const noexcept override { return false; }
    void run();

    std::shared_ptr<Sink> target_;
    std::size_t maxQueue_;
    std::mutex queueMutex_;
    std::condition_variable queueCv_;
    std::condition_variable idleCv_;
    std::deque<LogRecord> queue_;
    bool stopping_ = false;
    bool busy_ = false;
    std::atomic<std::size_t> dropped_{0};
    std::thread worker_;
};

// ===================================================================================================
// Logger
// ===================================================================================================

namespace detail {
/**
 * @brief Streams all arguments into one string (used by the variadic logging helpers).
 * @param args Values with an `operator<<`.
 * @return The concatenation.
 */
template <typename... Args>
[[nodiscard]] std::string concat(const Args&... args) {
    std::ostringstream oss;
    (oss << ... << args);
    return oss.str();
}
} // namespace detail

/**
 * @brief Named, thread-safe logger fanning records out to any number of sinks.
 *
 * The level check is a single relaxed atomic load, so disabled statements are nearly free. Sinks are
 * stored as `shared_ptr` and copied out under a shared lock before writing, so sinks can be added or
 * removed concurrently with logging. Exceptions thrown by sinks are swallowed and counted: logging
 * never propagates errors to the caller.
 */
class Logger {
public:
    /**
     * @brief Creates a logger with no sinks (silent).
     * @param name Logger name, copied into each record.
     * @param level Initial minimum level.
     */
    explicit Logger(std::string name = {}, LogLevel level = LogLevel::Info);

    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;
    Logger(Logger&&) = delete;
    Logger& operator=(Logger&&) = delete;
    ~Logger() = default;

    /// @brief Returns the logger name.
    /// @return The name.
    [[nodiscard]] const std::string& name() const noexcept { return name_; }

    /**
     * @brief Sets the minimum level.
     * @param level New level; `LogLevel::Off` disables the logger.
     */
    void setLevel(LogLevel level) noexcept { level_.store(level, std::memory_order_relaxed); }

    /// @brief Returns the minimum level.
    /// @return The level.
    [[nodiscard]] LogLevel level() const noexcept { return level_.load(std::memory_order_relaxed); }

    /**
     * @brief Returns whether a record at `level` would be emitted.
     * @param messageLevel Level to test.
     * @return True when enabled and at least one sink is attached.
     */
    [[nodiscard]] bool shouldLog(LogLevel messageLevel) const noexcept {
        return messageLevel != LogLevel::Off && messageLevel >= level() &&
               sinkCount_.load(std::memory_order_relaxed) > 0;
    }

    /**
     * @brief Attaches a sink. Thread-safe.
     * @param sink Sink to attach; null pointers are ignored.
     */
    void addSink(std::shared_ptr<Sink> sink);

    /**
     * @brief Detaches a sink. Thread-safe.
     * @param sink Sink to remove.
     * @return True if the sink was attached.
     */
    bool removeSink(const std::shared_ptr<Sink>& sink);

    /// @brief Detaches every sink, making the logger silent again.
    void clearSinks();

    /// @brief Returns the number of attached sinks.
    /// @return Sink count.
    [[nodiscard]] std::size_t sinkCount() const noexcept { return sinkCount_.load(std::memory_order_relaxed); }

    /**
     * @brief Emits a message.
     * @param messageLevel Severity.
     * @param message Message text.
     * @param location Call site (captured automatically).
     */
    void log(LogLevel messageLevel, std::string_view message,
             std::source_location location = std::source_location::current());

    /// @brief Emits at Trace. @param message Text. @param location Call site.
    void trace(std::string_view message, std::source_location location = std::source_location::current()) {
        log(LogLevel::Trace, message, location);
    }
    /// @brief Emits at Debug. @param message Text. @param location Call site.
    void debug(std::string_view message, std::source_location location = std::source_location::current()) {
        log(LogLevel::Debug, message, location);
    }
    /// @brief Emits at Info. @param message Text. @param location Call site.
    void info(std::string_view message, std::source_location location = std::source_location::current()) {
        log(LogLevel::Info, message, location);
    }
    /// @brief Emits at Warn. @param message Text. @param location Call site.
    void warn(std::string_view message, std::source_location location = std::source_location::current()) {
        log(LogLevel::Warn, message, location);
    }
    /// @brief Emits at Error. @param message Text. @param location Call site.
    void error(std::string_view message, std::source_location location = std::source_location::current()) {
        log(LogLevel::Error, message, location);
    }
    /// @brief Emits at Fatal (does not terminate). @param message Text. @param location Call site.
    void fatal(std::string_view message, std::source_location location = std::source_location::current()) {
        log(LogLevel::Fatal, message, location);
    }

    /**
     * @brief Streams all arguments into a message, only if the level is enabled.
     * @param messageLevel Severity.
     * @param args Values with an `operator<<`.
     */
    template <typename... Args>
    void logArgs(LogLevel messageLevel, const Args&... args) {
        if (shouldLog(messageLevel)) {
            log(messageLevel, detail::concat(args...));
        }
    }

    /// @brief Flushes every attached sink.
    void flush();

    /**
     * @brief Returns how many records were emitted at a level.
     * @param messageLevel The level (Off returns 0).
     * @return Count.
     */
    [[nodiscard]] std::size_t count(LogLevel messageLevel) const noexcept;

    /// @brief Returns how many sink exceptions were swallowed.
    /// @return Count.
    [[nodiscard]] std::size_t sinkErrors() const noexcept { return sinkErrors_.load(std::memory_order_relaxed); }

private:
    [[nodiscard]] std::vector<std::shared_ptr<Sink>> snapshotSinks() const;

    std::string name_;
    std::atomic<LogLevel> level_;
    mutable std::shared_mutex sinksMutex_;
    std::vector<std::shared_ptr<Sink>> sinks_;
    std::atomic<std::size_t> sinkCount_{0};
    std::array<std::atomic<std::size_t>, kLogLevelCount> counts_{};
    std::atomic<std::size_t> sinkErrors_{0};
};

/**
 * @brief Process-wide registry of named loggers (thread-safe Meyers singleton).
 *
 * Loggers are created lazily with the registry's default level and live until `clear()` or process
 * exit. A fresh logger has no sinks, so the registry never produces output by itself.
 */
class LoggerRegistry {
public:
    /// @brief Returns the global registry.
    /// @return The instance.
    [[nodiscard]] static LoggerRegistry& instance();

    LoggerRegistry() = default;
    LoggerRegistry(const LoggerRegistry&) = delete;
    LoggerRegistry& operator=(const LoggerRegistry&) = delete;
    LoggerRegistry(LoggerRegistry&&) = delete;
    LoggerRegistry& operator=(LoggerRegistry&&) = delete;
    ~LoggerRegistry() = default;

    /**
     * @brief Returns the logger with `name`, creating it if necessary.
     * @param name Logger name.
     * @return Shared handle (never null).
     */
    [[nodiscard]] std::shared_ptr<Logger> get(const std::string& name);

    /**
     * @brief Returns whether a logger exists.
     * @param name Logger name.
     * @return True if present.
     */
    [[nodiscard]] bool contains(const std::string& name) const;

    /**
     * @brief Sets the default level for new loggers and applies it to existing ones.
     * @param level Level.
     */
    void setGlobalLevel(LogLevel level);

    /**
     * @brief Removes a logger (outstanding handles stay valid).
     * @param name Logger name.
     * @return True if a logger was removed.
     */
    bool remove(const std::string& name);

    /// @brief Removes every logger.
    void clear();

    /// @brief Returns the number of registered loggers.
    /// @return Count.
    [[nodiscard]] std::size_t size() const;

private:
    mutable std::mutex mutex_;
    std::unordered_map<std::string, std::shared_ptr<Logger>> loggers_;
    LogLevel defaultLevel_ = LogLevel::Info;
};

/**
 * @brief RAII timer that logs the elapsed time of a scope when destroyed.
 */
class ScopedLogTimer {
public:
    /**
     * @brief Starts timing.
     * @param logger Logger to report to (must outlive the timer).
     * @param label Description of the timed scope.
     * @param level Level of the completion message.
     */
    ScopedLogTimer(Logger& logger, std::string label, LogLevel level = LogLevel::Debug);

    /// @brief Logs "label took N us". Never throws.
    ~ScopedLogTimer();
    ScopedLogTimer(const ScopedLogTimer&) = delete;
    ScopedLogTimer& operator=(const ScopedLogTimer&) = delete;
    ScopedLogTimer(ScopedLogTimer&&) = delete;
    ScopedLogTimer& operator=(ScopedLogTimer&&) = delete;

private:
    Logger* logger_;
    std::string label_;
    LogLevel level_;
    std::chrono::steady_clock::time_point start_;
};

/**
 * @brief Runs the logging showcase, writing only to `out`.
 * @param out Destination stream.
 */
void demonstrateLogging(std::ostream& out = std::cout);

} // namespace CppVerseHub::Utils

/**
 * @brief Lazily-evaluated logging macro: the streamed arguments are only evaluated when enabled.
 *
 * Usage: `CPPVERSEHUB_LOG(logger, CppVerseHub::Utils::LogLevel::Info, "x=", x);`
 */
#define CPPVERSEHUB_LOG(logger, lvl, ...)                                                                 \
    do {                                                                                                  \
        auto& cppversehub_logger_ref_ = (logger);                                                         \
        if (cppversehub_logger_ref_.shouldLog(lvl)) {                                                     \
            cppversehub_logger_ref_.log((lvl), ::CppVerseHub::Utils::detail::concat(__VA_ARGS__));        \
        }                                                                                                 \
    } while (false)
