// File: src/utils/TimeUtils.cpp
// Date/Time Handling Utilities Implementation

#include "TimeUtils.hpp"
#include <iostream>
#include <thread>
#include <cmath>
#include <regex>

namespace CppVerseHub::Utils::Time {

// ===== TIME FORMATTER IMPLEMENTATION =====

const std::string TimeFormatter::ISO8601_FORMAT = "%Y-%m-%dT%H:%M:%SZ";
const std::string TimeFormatter::RFC3339_FORMAT = "%Y-%m-%dT%H:%M:%S.%fZ";
const std::string TimeFormatter::READABLE_FORMAT = "%B %d, %Y %I:%M:%S %p";
const std::string TimeFormatter::SHORT_DATE_FORMAT = "%m/%d/%Y";
const std::string TimeFormatter::LONG_DATE_FORMAT = "%A, %B %d, %Y";
const std::string TimeFormatter::TIME_ONLY_FORMAT = "%H:%M:%S";
const std::string TimeFormatter::TIME_12H_FORMAT = "%I:%M:%S %p";
const std::string TimeFormatter::COMPACT_FORMAT = "%Y%m%d_%H%M%S";
const std::string TimeFormatter::LOG_FORMAT = "[%Y-%m-%d %H:%M:%S.%f]";

std::string TimeFormatter::format(const TimePoint& tp, const std::string& format_str) {
    auto time_t = Clock::to_time_t(tp);
    std::ostringstream oss;
    oss << std::put_time(std::localtime(&time_t), format_str.c_str());
    return oss.str();
}

std::string TimeFormatter::formatISO8601(const TimePoint& tp, bool include_milliseconds) {
    auto time_t = Clock::to_time_t(tp);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        tp.time_since_epoch()) % 1000;
    
    std::ostringstream oss;
    oss << std::put_time(std::gmtime(&time_t), "%Y-%m-%dT%H:%M:%S");
    
    if (include_milliseconds) {
        oss << "." << std::setfill('0') << std::setw(3) << ms.count();
    }
    
    oss << "Z";
    return oss.str();
}

std::string TimeFormatter::formatRFC3339(const TimePoint& tp) {
    return formatISO8601(tp, true);
}

std::string TimeFormatter::formatReadable(const TimePoint& tp) {
    return format(tp, "%B %d, %Y %I:%M:%S %p");
}

std::string TimeFormatter::formatShortDate(const TimePoint& tp) {
    return format(tp, "%m/%d/%Y");
}

std::string TimeFormatter::formatLongDate(const TimePoint& tp) {
    return format(tp, "%A, %B %d, %Y");
}

std::string TimeFormatter::formatTimeOnly(const TimePoint& tp, bool use_24h) {
    return format(tp, use_24h ? "%H:%M:%S" : "%I:%M:%S %p");
}

std::string TimeFormatter::formatCompact(const TimePoint& tp) {
    return format(tp, "%Y%m%d_%H%M%S");
}

std::string TimeFormatter::formatForLog(const TimePoint& tp) {
    auto time_t = Clock::to_time_t(tp);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        tp.time_since_epoch()) % 1000;
    
    std::ostringstream oss;
    oss << "[" << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S")
        << "." << std::setfill('0') << std::setw(3) << ms.count() << "]";
    
    return oss.str();
}

std::string TimeFormatter::formatDuration(const Duration& duration) {
    auto total_seconds = std::chrono::duration_cast<std::chrono::seconds>(duration).count();
    
    int days = static_cast<int>(total_seconds / Constants::SECONDS_PER_DAY);
    total_seconds %= Constants::SECONDS_PER_DAY;
    
    int hours = static_cast<int>(total_seconds / Constants::SECONDS_PER_HOUR);
    total_seconds %= Constants::SECONDS_PER_HOUR;
    
    int minutes = static_cast<int>(total_seconds / Constants::SECONDS_PER_MINUTE);
    int seconds = static_cast<int>(total_seconds % Constants::SECONDS_PER_MINUTE);
    
    std::ostringstream oss;
    
    if (days > 0) {
        oss << days << "d ";
    }
    if (hours > 0 || days > 0) {
        oss << hours << "h ";
    }
    if (minutes > 0 || hours > 0 || days > 0) {
        oss << minutes << "m ";
    }
    
    oss << seconds << "s";
    
    return oss.str();
}

std::string TimeFormatter::formatDurationPrecise(const Duration& duration) {
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(duration);
    auto seconds_part = std::chrono::duration_cast<std::chrono::seconds>(ms);
    auto ms_part = ms - seconds_part;
    
    std::string base = formatDuration(seconds_part);
    if (ms_part.count() > 0) {
        base += " " + std::to_string(ms_part.count()) + "ms";
    }
    
    return base;
}

std::string TimeFormatter::formatElapsed(const TimePoint& start, const TimePoint& end) {
    return formatDuration(end - start);
}

std::string TimeFormatter::formatAge(const TimePoint& timestamp) {
    return formatElapsed(timestamp, Clock::now());
}

std::string TimeFormatter::formatRelative(const TimePoint& tp) {
    auto now = Clock::now();
    auto diff = now - tp;
    
    if (diff < std::chrono::seconds(60)) {
        return "just now";
    } else if (diff < std::chrono::minutes(60)) {
        auto minutes = std::chrono::duration_cast<std::chrono::minutes>(diff).count();
        return std::to_string(minutes) + " minute" + (minutes == 1 ? "" : "s") + " ago";
    } else if (diff < std::chrono::hours(24)) {
        auto hours = std::chrono::duration_cast<std::chrono::hours>(diff).count();
        return std::to_string(hours) + " hour" + (hours == 1 ? "" : "s") + " ago";
    } else if (diff < Days(7)) {
        auto days = std::chrono::duration_cast<Days>(diff).count();
        return std::to_string(days) + " day" + (days == 1 ? "" : "s") + " ago";
    } else {
        return formatShortDate(tp);
    }
}

std::string TimeFormatter::formatTimeAgo(const TimePoint& tp) {
    return formatRelative(tp);
}

std::string TimeFormatter::formatTimeUntil(const TimePoint& tp) {
    auto now = Clock::now();
    auto diff = tp - now;
    
    if (diff < std::chrono::seconds(0)) {
        return "in the past";
    } else if (diff < std::chrono::minutes(60)) {
        auto minutes = std::chrono::duration_cast<std::chrono::minutes>(diff).count();
        return "in " + std::to_string(minutes) + " minute" + (minutes == 1 ? "" : "s");
    } else if (diff < std::chrono::hours(24)) {
        auto hours = std::chrono::duration_cast<std::chrono::hours>(diff).count();
        return "in " + std::to_string(hours) + " hour" + (hours == 1 ? "" : "s");
    } else if (diff < Days(7)) {
        auto days = std::chrono::duration_cast<Days>(diff).count();
        return "in " + std::to_string(days) + " day" + (days == 1 ? "" : "s");
    } else {
        return "on " + formatShortDate(tp);
    }
}

// ===== TIME PARSER IMPLEMENTATION =====

std::optional<TimePoint> TimeParser::parseISO8601(const std::string& time_str) {
    std::regex iso_regex(R"((\d{4})-(\d{2})-(\d{2})T(\d{2}):(\d{2}):(\d{2})(?:\.(\d+))?Z?)");
    std::smatch match;
    
    if (!std::regex_match(time_str, match, iso_regex)) {
        return std::nullopt;
    }
    
    try {
        std::tm tm = {};
        tm.tm_year = std::stoi(match[1]) - 1900;
        tm.tm_mon = std::stoi(match[2]) - 1;
        tm.tm_mday = std::stoi(match[3]);
        tm.tm_hour = std::stoi(match[4]);
        tm.tm_min = std::stoi(match[5]);
        tm.tm_sec = std::stoi(match[6]);
        
        auto time_t = std::mktime(&tm);
        auto tp = Clock::from_time_t(time_t);
        
        // Add milliseconds if present
        if (match[7].matched) {
            std::string ms_str = match[7];
            if (ms_str.length() > 3) {
                ms_str = ms_str.substr(0, 3);  // Truncate to milliseconds
            }
            while (ms_str.length() < 3) {
                ms_str += "0";  // Pad to 3 digits
            }
            
            int ms = std::stoi(ms_str);
            tp += std::chrono::milliseconds(ms);
        }
        
        return tp;
    } catch (const std::exception&) {
        return std::nullopt;
    }
}

std::optional<TimePoint> TimeParser::parseRFC3339(const std::string& time_str) {
    return parseISO8601(time_str);  // RFC3339 is a subset of ISO8601
}

std::optional<Duration> TimeParser::parseDuration(const std::string& duration_str) {
    std::regex duration_regex(R"((?:(\d+)d\s*)?(?:(\d+)h\s*)?(?:(\d+)m\s*)?(?:(\d+)s\s*)?(?:(\d+)ms\s*)?)");
    std::smatch match;
    
    if (!std::regex_match(duration_str, match, duration_regex)) {
        return std::nullopt;
    }
    
    try {
        Duration total_duration{0};
        
        if (match[1].matched) {
            int days = std::stoi(match[1]);
            total_duration += Days(days);
        }
        if (match[2].matched) {
            int hours = std::stoi(match[2]);
            total_duration += std::chrono::hours(hours);
        }
        if (match[3].matched) {
            int minutes = std::stoi(match[3]);
            total_duration += std::chrono::minutes(minutes);
        }
        if (match[4].matched) {
            int seconds = std::stoi(match[4]);
            total_duration += std::chrono::seconds(seconds);
        }
        if (match[5].matched) {
            int milliseconds = std::stoi(match[5]);
            total_duration += std::chrono::milliseconds(milliseconds);
        }
        
        return total_duration;
    } catch (const std::exception&) {
        return std::nullopt;
    }
}

std::optional<TimePoint> TimeParser::parseCustomFormat(const std::string& time_str, const std::string& format) {
    std::tm tm = {};
    std::istringstream ss(time_str);
    ss >> std::get_time(&tm, format.c_str());
    
    if (ss.fail()) {
        return std::nullopt;
    }
    
    try {
        auto time_t = std::mktime(&tm);
        return Clock::from_time_t(time_t);
    } catch (const std::exception&) {
        return std::nullopt;
    }
}

std::vector<TimePoint> TimeParser::parseMultiple(const std::vector<std::string>& time_strings,
                                                  const std::string& format) {
    std::vector<TimePoint> results;
    results.reserve(time_strings.size());
    
    for (const auto& time_str : time_strings) {
        auto parsed = parseCustomFormat(time_str, format);
        if (parsed) {
            results.push_back(*parsed);
        }
    }
    
    return results;
}

bool TimeParser::isValidFormat(const std::string& time_str, const std::string& format) {
    return parseCustomFormat(time_str, format).has_value();
}

// ===== TIME ZONE HANDLER IMPLEMENTATION =====

TimeZoneHandler::TimeZoneHandler() : local_offset_(calculateLocalOffset()) {}

std::string TimeZoneHandler::getSystemTimeZone() const {
    return "Local";  // Simplified implementation
}

TimePoint TimeZoneHandler::convertToUTC(const TimePoint& local_time) const {
    return local_time - local_offset_;
}

TimePoint TimeZoneHandler::convertFromUTC(const TimePoint& utc_time) const {
    return utc_time + local_offset_;
}

TimePoint TimeZoneHandler::convertToTimeZone(const TimePoint& utc_time, const std::string& timezone) const {
    // Simplified implementation - in a real-world scenario, you'd use a timezone library
    if (timezone == "UTC" || timezone == "GMT") {
        return utc_time;
    } else if (timezone == "EST" || timezone == "EDT") {
        return utc_time - std::chrono::hours(5);
    } else if (timezone == "PST" || timezone == "PDT") {
        return utc_time - std::chrono::hours(8);
    } else if (timezone == "JST") {
        return utc_time + std::chrono::hours(9);
    }
    
    // Default to local time
    return convertFromUTC(utc_time);
}

std::chrono::hours TimeZoneHandler::getOffset(const std::string& timezone) const {
    if (timezone == "UTC" || timezone == "GMT") {
        return std::chrono::hours(0);
    } else if (timezone == "EST" || timezone == "EDT") {
        return std::chrono::hours(-5);
    } else if (timezone == "PST" || timezone == "PDT") {
        return std::chrono::hours(-8);
    } else if (timezone == "JST") {
        return std::chrono::hours(9);
    }
    
    return std::chrono::duration_cast<std::chrono::hours>(local_offset_);
}

bool TimeZoneHandler::isDaylightSavingTime(const TimePoint& tp) const {
    auto time_t = Clock::to_time_t(tp);
    std::tm* local_tm = std::localtime(&time_t);
    return local_tm->tm_isdst > 0;
}

std::string TimeZoneHandler::formatWithTimeZone(const TimePoint& tp, const std::string& timezone) const {
    auto converted = convertToTimeZone(tp, timezone);
    return TimeFormatter::formatISO8601(converted) + " " + timezone;
}

Duration TimeZoneHandler::calculateLocalOffset() const {
    auto now = std::chrono::system_clock::now();
    auto time_t = Clock::to_time_t(now);
    
    std::tm* utc_tm = std::gmtime(&time_t);
    std::tm* local_tm = std::localtime(&time_t);
    
    auto utc_time = std::mktime(utc_tm);
    auto local_time = std::mktime(local_tm);
    
    return std::chrono::seconds(static_cast<int>(local_time - utc_time));
}

// ===== TIMER IMPLEMENTATION =====

Timer::Timer() : start_time_(Clock::now()), is_running_(true) {}

void Timer::start() {
    start_time_ = Clock::now();
    is_running_ = true;
    laps_.clear();
}

void Timer::stop() {
    if (is_running_) {
        stop_time_ = Clock::now();
        is_running_ = false;
    }
}

void Timer::reset() {
    start_time_ = Clock::now();
    is_running_ = true;
    laps_.clear();
}

void Timer::lap() {
    if (is_running_) {
        laps_.push_back(Clock::now());
    }
}

Duration Timer::elapsed() const {
    if (is_running_) {
        return Clock::now() - start_time_;
    } else {
        return stop_time_ - start_time_;
    }
}

Duration Timer::elapsedSinceStart() const {
    return Clock::now() - start_time_;
}

std::vector<Duration> Timer::getLapTimes() const {
    std::vector<Duration> lap_times;
    lap_times.reserve(laps_.size());
    
    TimePoint previous = start_time_;
    for (const auto& lap : laps_) {
        lap_times.push_back(lap - previous);
        previous = lap;
    }
    
    return lap_times;
}

std::vector<Duration> Timer::getCumulativeTimes() const {
    std::vector<Duration> cumulative_times;
    cumulative_times.reserve(laps_.size());
    
    for (const auto& lap : laps_) {
        cumulative_times.push_back(lap - start_time_);
    }
    
    return cumulative_times;
}

bool Timer::isRunning() const {
    return is_running_;
}

std::string Timer::toString() const {
    std::ostringstream oss;
    oss << "Timer: " << TimeFormatter::formatDurationPrecise(elapsed());
    if (!laps_.empty()) {
        oss << " (" << laps_.size() << " laps)";
    }
    oss << " [" << (is_running_ ? "running" : "stopped") << "]";
    return oss.str();
}

// ===== RATE LIMITER IMPLEMENTATION =====

RateLimiter::RateLimiter(size_t max_requests, Duration window_size)
    : max_requests_(max_requests), window_size_(window_size) {
    requests_.reserve(max_requests);
}

bool RateLimiter::tryAcquire() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto now = Clock::now();
    cleanOldRequests(now);
    
    if (requests_.size() < max_requests_) {
        requests_.push_back(now);
        return true;
    }
    
    return false;
}

bool RateLimiter::canAcquire() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto now = Clock::now();
    const_cast<RateLimiter*>(this)->cleanOldRequests(now);
    
    return requests_.size() < max_requests_;
}

Duration RateLimiter::timeUntilNextSlot() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (requests_.size() < max_requests_) {
        return Duration::zero();
    }
    
    auto oldest_request = *std::min_element(requests_.begin(), requests_.end());
    auto next_available = oldest_request + window_size_;
    auto now = Clock::now();
    
    if (next_available <= now) {
        return Duration::zero();
    }
    
    return next_available - now;
}

size_t RateLimiter::getCurrentLoad() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto now = Clock::now();
    const_cast<RateLimiter*>(this)->cleanOldRequests(now);
    
    return requests_.size();
}

double RateLimiter::getLoadPercentage() const {
    return static_cast<double>(getCurrentLoad()) / max_requests_ * 100.0;
}

void RateLimiter::reset() {
    std::lock_guard<std::mutex> lock(mutex_);
    requests_.clear();
}

void RateLimiter::updateLimits(size_t max_requests, Duration window_size) {
    std::lock_guard<std::mutex> lock(mutex_);
    max_requests_ = max_requests;
    window_size_ = window_size;
    requests_.reserve(max_requests);
    
    // Clean requests that no longer fit in the new window
    auto now = Clock::now();
    cleanOldRequests(now);
}

std::string RateLimiter::getStatus() const {
    std::ostringstream oss;
    oss << "RateLimiter: " << getCurrentLoad() << "/" << max_requests_ 
        << " (" << std::fixed << std::setprecision(1) << getLoadPercentage() << "%)";
    
    auto next_slot = timeUntilNextSlot();
    if (next_slot > Duration::zero()) {
        oss << ", next slot in " << TimeFormatter::formatDurationPrecise(next_slot);
    }
    
    return oss.str();
}

void RateLimiter::cleanOldRequests(const TimePoint& now) {
    auto cutoff = now - window_size_;
    requests_.erase(
        std::remove_if(requests_.begin(), requests_.end(),
                      [cutoff](const TimePoint& tp) { return tp < cutoff; }),
        requests_.end()
    );
}

// ===== SCHEDULER IMPLEMENTATION =====

Scheduler::Scheduler() : running_(false) {}

Scheduler::~Scheduler() {
    stop();
}

void Scheduler::start() {
    if (running_) return;
    
    running_ = true;
    worker_thread_ = std::thread(&Scheduler::workerLoop, this);
}

void Scheduler::stop() {
    if (!running_) return;
    
    {
        std::lock_guard<std::mutex> lock(mutex_);
        running_ = false;
    }
    
    condition_.notify_all();
    
    if (worker_thread_.joinable()) {
        worker_thread_.join();
    }
}

TaskId Scheduler::schedule(std::function<void()> task, const TimePoint& when) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    TaskId id = next_task_id_++;
    ScheduledTask scheduled_task;
    scheduled_task.id = id;
    scheduled_task.task = std::move(task);
    scheduled_task.scheduled_time = when;
    scheduled_task.repeat_interval = Duration::zero();
    scheduled_task.is_repeating = false;
    
    tasks_[id] = std::move(scheduled_task);
    condition_.notify_one();
    
    return id;
}

TaskId Scheduler::scheduleRepeating(std::function<void()> task, const TimePoint& first_run, const Duration& interval) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    TaskId id = next_task_id_++;
    ScheduledTask scheduled_task;
    scheduled_task.id = id;
    scheduled_task.task = std::move(task);
    scheduled_task.scheduled_time = first_run;
    scheduled_task.repeat_interval = interval;
    scheduled_task.is_repeating = true;
    
    tasks_[id] = std::move(scheduled_task);
    condition_.notify_one();
    
    return id;
}

TaskId Scheduler::scheduleDelayed(std::function<void()> task, const Duration& delay) {
    return schedule(std::move(task), Clock::now() + delay);
}

TaskId Scheduler::scheduleInterval(std::function<void()> task, const Duration& interval) {
    return scheduleRepeating(std::move(task), Clock::now() + interval, interval);
}

bool Scheduler::cancelTask(TaskId task_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = tasks_.find(task_id);
    if (it != tasks_.end()) {
        tasks_.erase(it);
        return true;
    }
    
    return false;
}

bool Scheduler::hasTask(TaskId task_id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    return tasks_.find(task_id) != tasks_.end();
}

size_t Scheduler::getTaskCount() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return tasks_.size();
}

std::vector<TaskId> Scheduler::getScheduledTasks() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::vector<TaskId> task_ids;
    task_ids.reserve(tasks_.size());
    
    for (const auto& pair : tasks_) {
        task_ids.push_back(pair.first);
    }
    
    return task_ids;
}

void Scheduler::clearAllTasks() {
    std::lock_guard<std::mutex> lock(mutex_);
    tasks_.clear();
}

bool Scheduler::isRunning() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return running_;
}

void Scheduler::workerLoop() {
    std::unique_lock<std::mutex> lock(mutex_);
    
    while (running_) {
        if (tasks_.empty()) {
            condition_.wait(lock);
            continue;
        }
        
        // Find the next task to execute
        auto next_task_it = std::min_element(tasks_.begin(), tasks_.end(),
            [](const auto& a, const auto& b) {
                return a.second.scheduled_time < b.second.scheduled_time;
            });
        
        if (next_task_it == tasks_.end()) {
            condition_.wait(lock);
            continue;
        }
        
        auto& next_task = next_task_it->second;
        auto now = Clock::now();
        
        if (next_task.scheduled_time > now) {
            // Wait until the task should be executed
            condition_.wait_until(lock, next_task.scheduled_time);
            continue;
        }
        
        // Execute the task
        auto task_to_execute = next_task.task;
        bool is_repeating = next_task.is_repeating;
        Duration repeat_interval = next_task.repeat_interval;
        TaskId task_id = next_task.id;
        
        if (is_repeating) {
            // Reschedule for next execution
            next_task.scheduled_time = now + repeat_interval;
        } else {
            // Remove one-time task
            tasks_.erase(next_task_it);
        }
        
        // Execute task without holding the lock
        lock.unlock();
        
        try {
            task_to_execute();
        } catch (const std::exception& e) {
            // Log error or handle exception as needed
            std::cerr << "Task " << task_id << " threw exception: " << e.what() << std::endl;
        } catch (...) {
            std::cerr << "Task " << task_id << " threw unknown exception" << std::endl;
        }
        
        lock.lock();
    }
}

// ===== TIME UTILITIES IMPLEMENTATION =====

TimePoint TimeUtils::now() {
    return Clock::now();
}

TimePoint TimeUtils::today() {
    auto now = Clock::now();
    auto time_t = Clock::to_time_t(now);
    std::tm* local_tm = std::localtime(&time_t);
    
    local_tm->tm_hour = 0;
    local_tm->tm_min = 0;
    local_tm->tm_sec = 0;
    
    return Clock::from_time_t(std::mktime(local_tm));
}

TimePoint TimeUtils::tomorrow() {
    return today() + Days(1);
}

TimePoint TimeUtils::yesterday() {
    return today() - Days(1);
}

TimePoint TimeUtils::startOfWeek(const TimePoint& tp) {
    auto time_t = Clock::to_time_t(tp);
    std::tm* local_tm = std::localtime(&time_t);
    
    // Calculate days since Monday (assuming Monday is start of week)
    int days_since_monday = (local_tm->tm_wday + 6) % 7;
    
    // Go back to Monday
    auto monday = tp - Days(days_since_monday);
    
    // Set to start of day
    time_t = Clock::to_time_t(monday);
    local_tm = std::localtime(&time_t);
    local_tm->tm_hour = 0;
    local_tm->tm_min = 0;
    local_tm->tm_sec = 0;
    
    return Clock::from_time_t(std::mktime(local_tm));
}

TimePoint TimeUtils::endOfWeek(const TimePoint& tp) {
    return startOfWeek(tp) + Days(7) - std::chrono::seconds(1);
}

TimePoint TimeUtils::startOfMonth(const TimePoint& tp) {
    auto time_t = Clock::to_time_t(tp);
    std::tm* local_tm = std::localtime(&time_t);
    
    local_tm->tm_mday = 1;
    local_tm->tm_hour = 0;
    local_tm->tm_min = 0;
    local_tm->tm_sec = 0;
    
    return Clock::from_time_t(std::mktime(local_tm));
}

TimePoint TimeUtils::endOfMonth(const TimePoint& tp) {
    auto time_t = Clock::to_time_t(tp);
    std::tm* local_tm = std::localtime(&time_t);
    
    // Go to first day of next month
    local_tm->tm_mon++;
    local_tm->tm_mday = 1;
    local_tm->tm_hour = 0;
    local_tm->tm_min = 0;
    local_tm->tm_sec = 0;
    
    auto next_month = Clock::from_time_t(std::mktime(local_tm));
    return next_month - std::chrono::seconds(1);
}

TimePoint TimeUtils::startOfYear(const TimePoint& tp) {
    auto time_t = Clock::to_time_t(tp);
    std::tm* local_tm = std::localtime(&time_t);
    
    local_tm->tm_mon = 0;
    local_tm->tm_mday = 1;
    local_tm->tm_hour = 0;
    local_tm->tm_min = 0;
    local_tm->tm_sec = 0;
    
    return Clock::from_time_t(std::mktime(local_tm));
}

TimePoint TimeUtils::endOfYear(const TimePoint& tp) {
    auto time_t = Clock::to_time_t(tp);
    std::tm* local_tm = std::localtime(&time_t);
    
    local_tm->tm_year++;
    local_tm->tm_mon = 0;
    local_tm->tm_mday = 1;
    local_tm->tm_hour = 0;
    local_tm->tm_min = 0;
    local_tm->tm_sec = 0;
    
    auto next_year = Clock::from_time_t(std::mktime(local_tm));
    return next_year - std::chrono::seconds(1);
}

bool TimeUtils::isSameDay(const TimePoint& tp1, const TimePoint& tp2) {
    auto time1 = Clock::to_time_t(tp1);
    auto time2 = Clock::to_time_t(tp2);
    
    std::tm* tm1 = std::localtime(&time1);
    std::tm* tm2 = std::localtime(&time2);
    
    return tm1->tm_year == tm2->tm_year &&
           tm1->tm_mon == tm2->tm_mon &&
           tm1->tm_mday == tm2->tm_mday;
}

bool TimeUtils::isWeekend(const TimePoint& tp) {
    auto time_t = Clock::to_time_t(tp);
    std::tm* local_tm = std::localtime(&time_t);
    
    return local_tm->tm_wday == 0 || local_tm->tm_wday == 6; // Sunday or Saturday
}

bool TimeUtils::isWeekday(const TimePoint& tp) {
    return !isWeekend(tp);
}

int TimeUtils::dayOfWeek(const TimePoint& tp) {
    auto time_t = Clock::to_time_t(tp);
    std::tm* local_tm = std::localtime(&time_t);
    
    return local_tm->tm_wday;
}

int TimeUtils::dayOfMonth(const TimePoint& tp) {
    auto time_t = Clock::to_time_t(tp);
    std::tm* local_tm = std::localtime(&time_t);
    
    return local_tm->tm_mday;
}

int TimeUtils::dayOfYear(const TimePoint& tp) {
    auto time_t = Clock::to_time_t(tp);
    std::tm* local_tm = std::localtime(&time_t);
    
    return local_tm->tm_yday + 1; // tm_yday is 0-based
}

int TimeUtils::weekOfYear(const TimePoint& tp) {
    auto time_t = Clock::to_time_t(tp);
    std::tm* local_tm = std::localtime(&time_t);
    
    // Simple calculation - more sophisticated implementations would handle edge cases
    return (local_tm->tm_yday + 7 - local_tm->tm_wday) / 7;
}

TimePoint TimeUtils::addDays(const TimePoint& tp, int days) {
    return tp + Days(days);
}

TimePoint TimeUtils::addWeeks(const TimePoint& tp, int weeks) {
    return tp + Weeks(weeks);
}

TimePoint TimeUtils::addMonths(const TimePoint& tp, int months) {
    auto time_t = Clock::to_time_t(tp);
    std::tm* local_tm = std::localtime(&time_t);
    
    local_tm->tm_mon += months;
    
    return Clock::from_time_t(std::mktime(local_tm));
}

TimePoint TimeUtils::addYears(const TimePoint& tp, int years) {
    auto time_t = Clock::to_time_t(tp);
    std::tm* local_tm = std::localtime(&time_t);
    
    local_tm->tm_year += years;
    
    return Clock::from_time_t(std::mktime(local_tm));
}

Duration TimeUtils::timeBetween(const TimePoint& start, const TimePoint& end) {
    return end - start;
}

double TimeUtils::daysBetween(const TimePoint& start, const TimePoint& end) {
    auto diff = timeBetween(start, end);
    return std::chrono::duration<double, std::ratio<86400>>(diff).count();
}

double TimeUtils::hoursBetween(const TimePoint& start, const TimePoint& end) {
    auto diff = timeBetween(start, end);
    return std::chrono::duration<double, std::ratio<3600>>(diff).count();
}

double TimeUtils::minutesBetween(const TimePoint& start, const TimePoint& end) {
    auto diff = timeBetween(start, end);
    return std::chrono::duration<double, std::ratio<60>>(diff).count();
}

void TimeUtils::sleep(const Duration& duration) {
    std::this_thread::sleep_for(duration);
}

void TimeUtils::sleepUntil(const TimePoint& time_point) {
    std::this_thread::sleep_until(time_point);
}

TimePoint TimeUtils::fromUnixTimestamp(int64_t timestamp) {
    return Clock::from_time_t(static_cast<std::time_t>(timestamp));
}

int64_t TimeUtils::toUnixTimestamp(const TimePoint& tp) {
    return static_cast<int64_t>(Clock::to_time_t(tp));
}

std::vector<TimePoint> TimeUtils::generateTimeRange(const TimePoint& start, const TimePoint& end, const Duration& step) {
    std::vector<TimePoint> result;
    
    if (step <= Duration::zero()) {
        return result;
    }
    
    for (auto current = start; current <= end; current += step) {
        result.push_back(current);
    }
    
    return result;
}

bool TimeUtils::isInRange(const TimePoint& tp, const TimePoint& start, const TimePoint& end) {
    return tp >= start && tp <= end;
}

TimePoint TimeUtils::clamp(const TimePoint& tp, const TimePoint& min_time, const TimePoint& max_time) {
    if (tp < min_time) return min_time;
    if (tp > max_time) return max_time;
    return tp;
}

} // namespace CppVerseHub::Utils::Time