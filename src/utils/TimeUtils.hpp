// File: src/utils/TimeUtils.hpp
// Date/Time Handling Utilities for Space Game

#pragma once

#include <chrono>
#include <string>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <vector>
#include <optional>
#include <functional>

namespace CppVerseHub::Utils::Time {

// ===== TYPE ALIASES =====

using Clock = std::chrono::system_clock;
using TimePoint = std::chrono::system_clock::time_point;
using Duration = std::chrono::system_clock::duration;
using Milliseconds = std::chrono::milliseconds;
using Seconds = std::chrono::seconds;
using Minutes = std::chrono::minutes;
using Hours = std::chrono::hours;
using Days = std::chrono::duration<int, std::ratio<86400>>;

// ===== TIME CONSTANTS =====

namespace Constants {
    constexpr int SECONDS_PER_MINUTE = 60;
    constexpr int MINUTES_PER_HOUR = 60;
    constexpr int HOURS_PER_DAY = 24;
    constexpr int DAYS_PER_WEEK = 7;
    constexpr int SECONDS_PER_HOUR = SECONDS_PER_MINUTE * MINUTES_PER_HOUR;
    constexpr int SECONDS_PER_DAY = SECONDS_PER_HOUR * HOURS_PER_DAY;
    constexpr int MILLISECONDS_PER_SECOND = 1000;
    constexpr int MICROSECONDS_PER_MILLISECOND = 1000;
    constexpr int NANOSECONDS_PER_MICROSECOND = 1000;
}

// ===== TIME FORMATTING =====

class TimeFormatter {
public:
    // Standard format strings
    static const std::string ISO8601_FORMAT;           // "2023-12-25T14:30:45Z"
    static const std::string RFC3339_FORMAT;           // "2023-12-25T14:30:45.123Z"
    static const std::string READABLE_FORMAT;          // "December 25, 2023 2:30:45 PM"
    static const std::string SHORT_DATE_FORMAT;        // "12/25/2023"
    static const std::string LONG_DATE_FORMAT;         // "Monday, December 25, 2023"
    static const std::string TIME_ONLY_FORMAT;         // "14:30:45"
    static const std::string TIME_12H_FORMAT;          // "2:30:45 PM"
    static const std::string COMPACT_FORMAT;           // "20231225_143045"
    static const std::string LOG_FORMAT;               // "[2023-12-25 14:30:45.123]"
    
    // Format time point to string
    static std::string format(const TimePoint& tp, const std::string& format_str);
    static std::string formatISO8601(const TimePoint& tp, bool include_milliseconds = false);
    static std::string formatRFC3339(const TimePoint& tp);
    static std::string formatReadable(const TimePoint& tp);
    static std::string formatShortDate(const TimePoint& tp);
    static std::string formatLongDate(const TimePoint& tp);
    static std::string formatTimeOnly(const TimePoint& tp, bool use_24h = true);
    static std::string formatCompact(const TimePoint& tp);
    static std::string formatForLog(const TimePoint& tp);
    
    // Format duration to string
    static std::string formatDuration(const Duration& duration);
    static std::string formatDurationPrecise(const Duration& duration);
    static std::string formatElapsed(const TimePoint& start, const TimePoint& end);
    static std::string formatAge(const TimePoint& timestamp);
    
    // Relative time formatting
    static std::string formatRelative(const TimePoint& tp);
    static std::string formatTimeAgo(const TimePoint& tp);
    static std::string formatTimeUntil(const TimePoint& tp);
};

// ===== TIME PARSING =====

class TimeParser {
public:
    // Parse string to time point
    static std::optional<TimePoint> parse(const std::string& time_str, const std::string& format_str);
    static std::optional<TimePoint> parseISO8601(const std::string& time_str);
    static std::optional<TimePoint> parseRFC3339(const std::string& time_str);
    static std::optional<TimePoint> parseReadable(const std::string& time_str);
    
    // Parse duration from string
    static std::optional<Duration> parseDuration(const std::string& duration_str);
    static std::optional<Seconds> parseSeconds(const std::string& seconds_str);
    
    // Smart parsing (auto-detect format)
    static std::optional<TimePoint> parseAuto(const std::string& time_str);
    
    // Validate time strings
    static bool isValidTimeString(const std::string& time_str, const std::string& format_str);
    static bool isValidISO8601(const std::string& time_str);
    static bool isValidRFC3339(const std::string& time_str);
};

// ===== TIME CALCULATIONS =====

class TimeCalculator {
public:
    // Basic calculations
    static Duration timeBetween(const TimePoint& start, const TimePoint& end);
    static TimePoint addDuration(const TimePoint& tp, const Duration& duration);
    static TimePoint subtractDuration(const TimePoint& tp, const Duration& duration);
    
    // Date arithmetic
    static TimePoint addDays(const TimePoint& tp, int days);
    static TimePoint addWeeks(const TimePoint& tp, int weeks);
    static TimePoint addMonths(const TimePoint& tp, int months);
    static TimePoint addYears(const TimePoint& tp, int years);
    
    // Time components
    static int getDayOfWeek(const TimePoint& tp);  // 0 = Sunday, 6 = Saturday
    static int getDayOfMonth(const TimePoint& tp);
    static int getDayOfYear(const TimePoint& tp);
    static int getWeekOfYear(const TimePoint& tp);
    static int getMonth(const TimePoint& tp);       // 1-12
    static int getYear(const TimePoint& tp);
    static int getHour(const TimePoint& tp);        // 0-23
    static int getMinute(const TimePoint& tp);      // 0-59
    static int getSecond(const TimePoint& tp);      // 0-59
    
    // Boundary calculations
    static TimePoint startOfDay(const TimePoint& tp);
    static TimePoint endOfDay(const TimePoint& tp);
    static TimePoint startOfWeek(const TimePoint& tp);
    static TimePoint endOfWeek(const TimePoint& tp);
    static TimePoint startOfMonth(const TimePoint& tp);
    static TimePoint endOfMonth(const TimePoint& tp);
    static TimePoint startOfYear(const TimePoint& tp);
    static TimePoint endOfYear(const TimePoint& tp);
    
    // Utility functions
    static bool isLeapYear(int year);
    static int daysInMonth(int year, int month);
    static bool isSameDay(const TimePoint& tp1, const TimePoint& tp2);
    static bool isSameWeek(const TimePoint& tp1, const TimePoint& tp2);
    static bool isSameMonth(const TimePoint& tp1, const TimePoint& tp2);
    static bool isSameYear(const TimePoint& tp1, const TimePoint& tp2);
};

// ===== TIME ZONES =====

class TimeZone {
public:
    struct ZoneInfo {
        std::string name;
        std::string abbreviation;
        int offset_hours;
        int offset_minutes;
        bool is_dst;
    };
    
    // Common time zones
    static const ZoneInfo UTC;
    static const ZoneInfo EST;  // Eastern Standard Time
    static const ZoneInfo PST;  // Pacific Standard Time
    static const ZoneInfo GMT;  // Greenwich Mean Time
    static const ZoneInfo CET;  // Central European Time
    static const ZoneInfo JST;  // Japan Standard Time
    
    // Convert between time zones
    static TimePoint convertToUTC(const TimePoint& tp, const ZoneInfo& from_zone);
    static TimePoint convertFromUTC(const TimePoint& utc_tp, const ZoneInfo& to_zone);
    static TimePoint convertBetweenZones(const TimePoint& tp, const ZoneInfo& from_zone, const ZoneInfo& to_zone);
    
    // Get local time zone info
    static ZoneInfo getLocalTimeZone();
    static int getLocalUtcOffset();
    static bool isLocalDST();
    
    // Format with time zone
    static std::string formatWithTimeZone(const TimePoint& tp, const ZoneInfo& zone, const std::string& format_str);
};

// ===== HIGH PRECISION TIMING =====

class HighPrecisionTimer {
private:
    std::chrono::high_resolution_clock::time_point start_time_;
    bool is_running_;
    std::chrono::nanoseconds accumulated_time_;
    
public:
    HighPrecisionTimer();
    
    void start();
    void stop();
    void reset();
    void restart();
    
    bool isRunning() const { return is_running_; }
    
    // Get elapsed time in various units
    std::chrono::nanoseconds elapsedNanoseconds() const;
    std::chrono::microseconds elapsedMicroseconds() const;
    std::chrono::milliseconds elapsedMilliseconds() const;
    std::chrono::seconds elapsedSeconds() const;
    double elapsedSecondsDouble() const;
    
    // Static timing utilities
    template<typename Func>
    static auto measureTime(Func&& func) {
        auto start = std::chrono::high_resolution_clock::now();
        auto result = std::forward<Func>(func)();
        auto end = std::chrono::high_resolution_clock::now();
        
        return std::make_pair(result, end - start);
    }
    
    template<typename Func>
    static auto measureTimeVoid(Func&& func) {
        auto start = std::chrono::high_resolution_clock::now();
        std::forward<Func>(func)();
        auto end = std::chrono::high_resolution_clock::now();
        
        return end - start;
    }
};

// ===== GAME TIME SYSTEM =====

class GameTime {
private:
    TimePoint game_start_time_;
    TimePoint pause_start_time_;
    Duration total_pause_time_;
    double time_scale_;
    bool is_paused_;
    
public:
    GameTime(double initial_time_scale = 1.0);
    
    // Time control
    void pause();
    void resume();
    void setTimeScale(double scale);
    void reset();
    
    // Get current game time
    TimePoint getCurrentGameTime() const;
    Duration getElapsedGameTime() const;
    Duration getRealElapsedTime() const;
    
    // Game time utilities
    double getTimeScale() const { return time_scale_; }
    bool isPaused() const { return is_paused_; }
    Duration getTotalPauseTime() const { return total_pause_time_; }
    
    // Game time formatting
    std::string formatGameTime(const std::string& format_str = "%Y-%m-%d %H:%M:%S") const;
    std::string formatElapsedTime() const;
    
    // Mission time calculations (space game specific)
    Duration timeToReachDestination(double distance_ly, double velocity_c) const;
    std::string formatMissionETA(double distance_ly, double velocity_c) const;
};

// ===== PERIODIC TASKS =====

class TaskScheduler {
public:
    using TaskFunction = std::function<void()>;
    
    struct ScheduledTask {
        std::string name;
        TaskFunction function;
        Duration interval;
        TimePoint next_execution;
        bool is_active;
        size_t execution_count;
        
        ScheduledTask(const std::string& task_name, TaskFunction func, Duration task_interval)
            : name(task_name), function(std::move(func)), interval(task_interval),
              next_execution(Clock::now() + task_interval), is_active(true), execution_count(0) {}
    };
    
private:
    std::vector<ScheduledTask> tasks_;
    bool is_running_;
    std::chrono::milliseconds check_interval_;
    
public:
    TaskScheduler(std::chrono::milliseconds check_interval = std::chrono::milliseconds(100));
    
    // Task management
    void addTask(const std::string& name, TaskFunction function, Duration interval);
    void removeTask(const std::string& name);
    void pauseTask(const std::string& name);
    void resumeTask(const std::string& name);
    
    // Scheduler control
    void start();
    void stop();
    void tick();  // Manual tick for single-threaded operation
    
    // Information
    size_t getTaskCount() const { return tasks_.size(); }
    bool hasTask(const std::string& name) const;
    size_t getTaskExecutionCount(const std::string& name) const;
    
    // Convenience methods for common intervals
    void addSecondlyTask(const std::string& name, TaskFunction function);
    void addMinutelyTask(const std::string& name, TaskFunction function);
    void addHourlyTask(const std::string& name, TaskFunction function);
    void addDailyTask(const std::string& name, TaskFunction function);
};

// ===== PERFORMANCE PROFILING =====

class PerformanceProfiler {
public:
    struct ProfileData {
        std::string name;
        std::chrono::nanoseconds total_time{0};
        std::chrono::nanoseconds min_time{std::chrono::nanoseconds::max()};
        std::chrono::nanoseconds max_time{0};
        size_t call_count{0};
        
        double averageTimeNs() const { 
            return call_count > 0 ? static_cast<double>(total_time.count()) / call_count : 0.0; 
        }
        
        double averageTimeMs() const { 
            return averageTimeNs() / 1000000.0; 
        }
    };
    
private:
    std::unordered_map<std::string, ProfileData> profiles_;
    mutable std::mutex mutex_;
    
public:
    // Record a timing measurement
    void record(const std::string& name, std::chrono::nanoseconds duration);
    
    // Get profile data
    ProfileData getProfile(const std::string& name) const;
    std::vector<ProfileData> getAllProfiles() const;
    
    // Reset profiles
    void reset();
    void resetProfile(const std::string& name);
    
    // Generate reports
    std::string generateReport() const;
    void printReport() const;
    
    // Scoped profiler helper
    class ScopedProfiler {
    private:
        PerformanceProfiler& profiler_;
        std::string name_;
        std::chrono::high_resolution_clock::time_point start_time_;
        
    public:
        ScopedProfiler(PerformanceProfiler& profiler, const std::string& name)
            : profiler_(profiler), name_(name), 
              start_time_(std::chrono::high_resolution_clock::now()) {}
        
        ~ScopedProfiler() {
            auto end_time = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time_);
            profiler_.record(name_, duration);
        }
    };
};

// Macro for easy scoped profiling
#define PROFILE_SCOPE(profiler, name) \
    CppVerseHub::Utils::Time::PerformanceProfiler::ScopedProfiler _prof(profiler, name)

// ===== SPACE GAME SPECIFIC TIME UTILITIES =====

namespace SpaceTime {
    
    // Astronomical time constants
    constexpr double SECONDS_PER_EARTH_YEAR = 365.25 * 24 * 3600;
    constexpr double SECONDS_PER_MARS_YEAR = 686.98 * 24 * 3600;
    constexpr double LIGHT_SPEED_MS = 299792458.0;  // meters per second
    constexpr double AU_IN_METERS = 149597870700.0;  // Astronomical Unit in meters
    constexpr double LIGHTYEAR_IN_METERS = 9.4607304725808e15;  // Light year in meters
    
    // Time dilation calculations
    double calculateTimeDilation(double velocity_c);  // velocity as fraction of c
    Duration applyTimeDilation(const Duration& proper_time, double velocity_c);
    
    // Travel time calculations
    Duration calculateTravelTime(double distance_ly, double velocity_c);
    Duration calculateAccelerationTime(double target_velocity_c, double acceleration_g);
    
    // Orbital period calculations
    Duration calculateOrbitalPeriod(double semi_major_axis_au, double star_mass_solar);
    
    // Mission time estimations
    struct MissionTiming {
        Duration travel_time;
        Duration mission_duration;
        Duration return_time;
        Duration total_time;
        TimePoint estimated_departure;
        TimePoint estimated_arrival;
        TimePoint estimated_completion;
        TimePoint estimated_return;
    };
    
    MissionTiming calculateMissionTiming(double distance_ly, double velocity_c, 
                                       Duration mission_duration, 
                                       const TimePoint& departure_time = Clock::now());
    
    // Format space-specific times
    std::string formatSpaceTime(const Duration& duration);
    std::string formatLightTravelTime(double distance_ly);
    std::string formatOrbitalPeriod(const Duration& period);
    
    // Space calendar system (fictional)
    struct SpaceDate {
        int galactic_year;
        int sector_period;
        int local_cycle;
        int time_unit;
        
        SpaceDate(const TimePoint& earth_time);
        std::string toString() const;
        TimePoint toEarthTime() const;
    };
    
} // namespace SpaceTime

// ===== UTILITY FUNCTIONS =====

// Get current time
TimePoint now();
TimePoint utcNow();

// Common time points
TimePoint epoch();
TimePoint endOfTime();

// Sleep functions
void sleepFor(const Duration& duration);
void sleepUntil(const TimePoint& time_point);

// Time conversion utilities
double toUnixTimestamp(const TimePoint& tp);
TimePoint fromUnixTimestamp(double timestamp);
std::string toISOString(const TimePoint& tp);
TimePoint fromISOString(const std::string& iso_string);

// Duration conversion utilities
template<typename ToDuration, typename Rep, typename Period>
constexpr ToDuration duration_cast_safe(const std::chrono::duration<Rep, Period>& duration) {
    return std::chrono::duration_cast<ToDuration>(duration);
}

// Comparison utilities
bool isOlderThan(const TimePoint& tp, const Duration& age);
bool isNewerThan(const TimePoint& tp, const Duration& age);
bool isBetween(const TimePoint& tp, const TimePoint& start, const TimePoint& end);

// Demonstration function
void demonstrateTimeUtils();

} // namespace CppVerseHub::Utils::Time