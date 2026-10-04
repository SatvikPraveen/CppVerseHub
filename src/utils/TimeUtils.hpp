/**
 * @file TimeUtils.hpp
 * @brief `std::chrono`-based time utilities: formatting, parsing, calendars, timers and schedulers.
 *
 * Demonstrates modern `<chrono>`: duration arithmetic and `duration_cast`, the C++20 calendar types
 * (`year_month_day`, `weekday`, `sys_days`, `hh_mm_ss`) used for thread-safe UTC formatting without
 * `gmtime`, `steady_clock` for measuring intervals, and deterministic, externally-driven time
 * (`GameTime`, `TaskScheduler`, `RateLimiter`) so that time-dependent logic is unit-testable without
 * sleeping. Time-zone database support (`std::chrono::zoned_time`) is intentionally not used because
 * it is unavailable in libc++; fixed UTC offsets are supported instead.
 */
#pragma once

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <iostream>
#include <map>
#include <mutex>
#include <optional>
#include <ostream>
#include <string>
#include <string_view>
#include <vector>

namespace CppVerseHub::Utils::Time {

using SystemClock = std::chrono::system_clock; ///< Wall clock.
using SteadyClock = std::chrono::steady_clock; ///< Monotonic clock for intervals.
using Nanoseconds = std::chrono::nanoseconds;  ///< Default duration resolution.

// ===================================================================================================
// Formatting and parsing
// ===================================================================================================

/**
 * @brief Formats a duration for humans, choosing the unit by magnitude.
 *
 * Examples: "750 ns", "12.500 us", "3.250 ms", "1.500 s", "2m 05s", "1h 02m 03s", "2d 03h 00m".
 * Negative durations are prefixed with '-'.
 * @param duration Duration to format.
 * @return Formatted text.
 */
[[nodiscard]] std::string formatDuration(Nanoseconds duration);

/**
 * @brief Formats a time point as ISO-8601, e.g. "2024-03-01T12:34:56.789Z".
 * @param tp Time point (interpreted as UTC).
 * @param withMillis Whether to include milliseconds.
 * @param utcOffset Fixed offset applied for display; non-zero offsets print "+HH:MM" instead of "Z".
 * @return Formatted text.
 */
[[nodiscard]] std::string formatIso8601(SystemClock::time_point tp, bool withMillis = true,
                                        std::chrono::minutes utcOffset = std::chrono::minutes{0});

/**
 * @brief Parses ISO-8601 "YYYY-MM-DD[THH:MM[:SS[.fraction]]][Z|+HH:MM|-HH:MM]".
 *
 * A space is accepted instead of 'T'. Missing offset means UTC. Fractions up to nanoseconds.
 * @param text Text to parse.
 * @return The UTC time point, or `std::nullopt` on any syntax or range error.
 */
[[nodiscard]] std::optional<SystemClock::time_point> parseIso8601(std::string_view text);

/**
 * @brief Parses compound durations such as "1h30m", "250ms", "2.5s", "1d 2h", "90" (seconds).
 *
 * Units: d, h, m, s, ms, us, ns.
 * @param text Text to parse.
 * @return The duration, or `std::nullopt` if malformed.
 */
[[nodiscard]] std::optional<Nanoseconds> parseDuration(std::string_view text);

// ===================================================================================================
// Calendar helpers (all constexpr)
// ===================================================================================================

/**
 * @brief Gregorian leap-year test.
 * @param year Year.
 * @return True for leap years.
 */
[[nodiscard]] constexpr bool isLeapYear(int year) noexcept {
    return (year % 4 == 0 && year % 100 != 0) || year % 400 == 0;
}

/**
 * @brief Number of days in a month.
 * @param year Year.
 * @param month Month 1..12.
 * @return 28..31, or 0 for an invalid month.
 */
[[nodiscard]] constexpr unsigned daysInMonth(int year, unsigned month) noexcept {
    constexpr unsigned kDays[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    if (month < 1 || month > 12) {
        return 0;
    }
    return (month == 2 && isLeapYear(year)) ? 29U : kDays[month - 1];
}

/**
 * @brief Day of the week of a civil date.
 * @param date Date.
 * @return 0 = Sunday ... 6 = Saturday (C encoding).
 */
[[nodiscard]] constexpr unsigned dayOfWeek(std::chrono::year_month_day date) noexcept {
    return std::chrono::weekday{std::chrono::sys_days{date}}.c_encoding();
}

/**
 * @brief Signed number of days from `from` to `to`.
 * @param from Start date.
 * @param to End date.
 * @return `to - from` in days.
 */
[[nodiscard]] constexpr std::int64_t daysBetween(std::chrono::year_month_day from,
                                                 std::chrono::year_month_day to) noexcept {
    return (std::chrono::sys_days{to} - std::chrono::sys_days{from}).count();
}

/**
 * @brief Day of the year (1-based).
 * @param date Date.
 * @return 1..366.
 */
[[nodiscard]] constexpr unsigned dayOfYear(std::chrono::year_month_day date) noexcept {
    const std::chrono::year_month_day jan1{date.year(), std::chrono::January, std::chrono::day{1}};
    return static_cast<unsigned>(daysBetween(jan1, date)) + 1U;
}

// ===================================================================================================
// Stopwatch
// ===================================================================================================

/**
 * @brief High-resolution stopwatch over `steady_clock` with start/stop/lap semantics.
 */
class Stopwatch {
public:
    /**
     * @brief Creates a stopwatch.
     * @param startImmediately Whether to start running immediately.
     */
    explicit Stopwatch(bool startImmediately = false) noexcept;

    /// @brief Starts (or resumes) timing. No effect if already running.
    void start() noexcept;

    /// @brief Stops timing, accumulating the elapsed interval. No effect if stopped.
    void stop() noexcept;

    /// @brief Stops and clears accumulated time and laps.
    void reset() noexcept;

    /// @brief Equivalent to `reset(); start();`.
    void restart() noexcept;

    /// @brief Returns whether the stopwatch is running.
    /// @return True while running.
    [[nodiscard]] bool running() const noexcept { return running_; }

    /// @brief Returns total accumulated time (including the current run).
    /// @return Elapsed time.
    [[nodiscard]] Nanoseconds elapsed() const noexcept;

    /**
     * @brief Returns elapsed time converted to `Duration`.
     * @tparam Duration Target duration type.
     * @return Elapsed time.
     */
    template <typename Duration>
    [[nodiscard]] Duration elapsedAs() const noexcept {
        return std::chrono::duration_cast<Duration>(elapsed());
    }

    /// @brief Returns elapsed seconds as a double.
    /// @return Seconds.
    [[nodiscard]] double elapsedSeconds() const noexcept;

    /**
     * @brief Records a lap: the time since the previous lap (or start).
     * @return The lap duration.
     */
    Nanoseconds lap();

    /// @brief Returns all recorded laps.
    /// @return Laps in order.
    [[nodiscard]] const std::vector<Nanoseconds>& laps() const noexcept { return laps_; }

private:
    SteadyClock::time_point startTime_;
    Nanoseconds accumulated_{0};
    Nanoseconds lastLapMark_{0};
    std::vector<Nanoseconds> laps_;
    bool running_ = false;
};

// ===================================================================================================
// Deterministic simulation time
// ===================================================================================================

/**
 * @brief Simulation clock decoupled from wall time: scaled, pausable, advanced explicitly.
 */
class GameTime {
public:
    /// @brief Simulation duration type (double seconds).
    using Seconds = std::chrono::duration<double>;

    /**
     * @brief Advances the clock by a real-time delta.
     * @param realDelta Real elapsed time since the last frame (negative values are ignored).
     * @return The simulated delta (0 when paused).
     */
    Seconds advance(Seconds realDelta) noexcept;

    /**
     * @brief Sets the time scale (1 = real time, 2 = double speed). Negative values clamp to 0.
     * @param scale Scale factor.
     */
    void setTimeScale(double scale) noexcept { timeScale_ = scale < 0.0 ? 0.0 : scale; }

    /// @brief Returns the time scale.
    /// @return The scale.
    [[nodiscard]] double timeScale() const noexcept { return timeScale_; }

    /// @brief Pauses the simulation.
    void pause() noexcept { paused_ = true; }

    /// @brief Resumes the simulation.
    void resume() noexcept { paused_ = false; }

    /// @brief Returns whether the simulation is paused.
    /// @return True when paused.
    [[nodiscard]] bool paused() const noexcept { return paused_; }

    /// @brief Returns total simulated time.
    /// @return Simulated time.
    [[nodiscard]] Seconds simulationTime() const noexcept { return simTime_; }

    /// @brief Returns total real time passed to `advance()`.
    /// @return Real time.
    [[nodiscard]] Seconds realTime() const noexcept { return realTime_; }

    /// @brief Returns the number of `advance()` calls.
    /// @return Frame count.
    [[nodiscard]] std::uint64_t frameCount() const noexcept { return frames_; }

    /// @brief Returns frames per real second averaged over the clock's lifetime.
    /// @return Average FPS, or 0 when no time has passed.
    [[nodiscard]] double averageFps() const noexcept;

private:
    Seconds simTime_{0.0};
    Seconds realTime_{0.0};
    double timeScale_ = 1.0;
    std::uint64_t frames_ = 0;
    bool paused_ = false;
};

/**
 * @brief Deterministic task scheduler driven by an explicit virtual clock.
 *
 * Tasks run inside `advance()` in order of due time (ties by scheduling order). Callbacks may schedule
 * or cancel tasks re-entrantly. Thread-safe: the internal lock is never held while a task runs.
 */
class TaskScheduler {
public:
    using Duration = std::chrono::milliseconds; ///< Virtual time resolution.
    using TaskId = std::uint64_t;               ///< Handle for cancellation.
    using Task = std::function<void()>;         ///< Task body.

    /**
     * @brief Schedules a one-shot task.
     * @param delay Delay from the current virtual time (negative treated as 0).
     * @param task Task body.
     * @return Task id.
     */
    TaskId scheduleOnce(Duration delay, Task task);

    /**
     * @brief Schedules a repeating task.
     * @param interval Interval between runs (must be > 0).
     * @param task Task body.
     * @param repetitions Number of runs (0 = unlimited).
     * @return Task id.
     * @throws std::invalid_argument if `interval <= 0`.
     */
    TaskId scheduleRepeating(Duration interval, Task task, std::size_t repetitions = 0);

    /**
     * @brief Cancels a pending task.
     * @param id Task id.
     * @return True if a task was cancelled.
     */
    bool cancel(TaskId id);

    /**
     * @brief Advances virtual time, running every task that becomes due.
     * @param delta Amount of virtual time to advance.
     * @return Number of task executions.
     */
    std::size_t advance(Duration delta);

    /// @brief Returns the current virtual time.
    /// @return Virtual time since construction.
    [[nodiscard]] Duration now() const;

    /// @brief Returns the number of pending tasks.
    /// @return Count.
    [[nodiscard]] std::size_t pending() const;

private:
    struct Entry {
        Task task;
        Duration interval{0};
        std::size_t remaining = 1; ///< 0 = unlimited.
    };
    using Key = std::pair<Duration, TaskId>;

    mutable std::mutex mutex_;
    std::map<Key, Entry> queue_;
    std::map<TaskId, Key> index_;
    Duration now_{0};
    TaskId nextId_ = 1;
};

/**
 * @brief Token-bucket rate limiter with explicit time input (deterministic and testable).
 */
class RateLimiter {
public:
    /**
     * @brief Creates a full bucket.
     * @param ratePerSecond Token refill rate (> 0).
     * @param burst Bucket capacity (>= 1).
     * @throws std::invalid_argument on invalid parameters.
     */
    RateLimiter(double ratePerSecond, double burst);

    /**
     * @brief Tries to take tokens at time `now`.
     * @param now Current time (monotonically non-decreasing between calls).
     * @param tokens Number of tokens requested.
     * @return True if enough tokens were available (and they were consumed).
     */
    [[nodiscard]] bool tryAcquire(SteadyClock::time_point now, double tokens = 1.0);

    /// @brief Returns tokens currently available (as of the last call).
    /// @return Token count.
    [[nodiscard]] double available() const noexcept { return tokens_; }

private:
    double rate_;
    double burst_;
    double tokens_;
    std::optional<SteadyClock::time_point> last_;
};

// ===================================================================================================
// Profiling
// ===================================================================================================

/**
 * @brief Thread-safe accumulator of named timing samples.
 */
class PerformanceProfiler {
public:
    /// @brief Aggregated statistics for one section.
    struct Stats {
        std::size_t count = 0; ///< Number of samples.
        Nanoseconds total{0};  ///< Sum of samples.
        Nanoseconds min{0};    ///< Shortest sample.
        Nanoseconds max{0};    ///< Longest sample.
        /// @brief Mean sample duration.
        /// @return Mean (0 if no samples).
        [[nodiscard]] Nanoseconds mean() const noexcept {
            return count == 0 ? Nanoseconds{0} : total / static_cast<std::int64_t>(count);
        }
    };

    /// @brief RAII helper that records the lifetime of a scope.
    class Scope {
    public:
        /**
         * @brief Starts timing.
         * @param profiler Target profiler (must outlive the scope).
         * @param name Section name.
         */
        Scope(PerformanceProfiler& profiler, std::string name);
        /// @brief Records the elapsed time. Never throws.
        ~Scope();
        Scope(const Scope&) = delete;
        Scope& operator=(const Scope&) = delete;
        Scope(Scope&&) = delete;
        Scope& operator=(Scope&&) = delete;

    private:
        PerformanceProfiler* profiler_;
        std::string name_;
        SteadyClock::time_point start_;
    };

    /**
     * @brief Records one sample.
     * @param name Section name.
     * @param duration Sample duration.
     */
    void record(const std::string& name, Nanoseconds duration);

    /**
     * @brief Returns statistics for a section.
     * @param name Section name.
     * @return Stats, or `std::nullopt` if unknown.
     */
    [[nodiscard]] std::optional<Stats> stats(const std::string& name) const;

    /// @brief Returns all section names (sorted).
    /// @return Names.
    [[nodiscard]] std::vector<std::string> sections() const;

    /// @brief Removes all samples.
    void reset();

    /**
     * @brief Writes a table of all sections.
     * @param out Destination stream.
     */
    void report(std::ostream& out) const;

private:
    mutable std::mutex mutex_;
    std::map<std::string, Stats> stats_;
};

// ===================================================================================================
// Space-flavoured helpers
// ===================================================================================================

namespace SpaceTime {
/// @brief Speed of light in m/s.
inline constexpr double kSpeedOfLight = 299'792'458.0;
/// @brief Astronomical unit in metres.
inline constexpr double kAstronomicalUnit = 149'597'870'700.0;

/**
 * @brief Light travel time over a distance.
 * @param metres Distance in metres.
 * @return Travel time.
 */
[[nodiscard]] constexpr std::chrono::duration<double> lightTravelTime(double metres) noexcept {
    return std::chrono::duration<double>{metres / kSpeedOfLight};
}

/**
 * @brief Converts a duration to the "stardate"-like decimal year used by the demo (year + fraction).
 * @param tp Time point.
 * @return Decimal year, e.g. 2024.5.
 */
[[nodiscard]] double decimalYear(SystemClock::time_point tp);
} // namespace SpaceTime

/**
 * @brief Runs the time utilities showcase, writing only to `out`.
 * @param out Destination stream.
 */
void demonstrateTime(std::ostream& out = std::cout);

} // namespace CppVerseHub::Utils::Time
