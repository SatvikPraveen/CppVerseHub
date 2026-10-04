/**
 * @file TimeUtils.cpp
 * @brief Implementation of the `<chrono>` utilities declared in TimeUtils.hpp.
 */
#include "utils/TimeUtils.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <thread>
#include <utility>

namespace CppVerseHub::Utils::Time {

namespace {

using std::chrono::duration_cast;

std::string fixed3(double value, std::string_view unit) {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(3) << value << ' ' << unit;
    return oss.str();
}

std::string twoDigits(long long value) {
    std::ostringstream oss;
    oss << std::setw(2) << std::setfill('0') << value;
    return oss.str();
}

/// Cursor over the text being parsed.
struct Cursor {
    std::string_view text;
    std::size_t pos = 0;

    [[nodiscard]] bool done() const noexcept { return pos >= text.size(); }
    [[nodiscard]] char peek() const noexcept { return done() ? '\0' : text[pos]; }
    bool consume(char c) noexcept {
        if (peek() == c) {
            ++pos;
            return true;
        }
        return false;
    }
    /// Reads exactly `count` decimal digits.
    std::optional<int> digits(std::size_t count) noexcept {
        if (pos + count > text.size()) {
            return std::nullopt;
        }
        int value = 0;
        for (std::size_t i = 0; i < count; ++i) {
            const char c = text[pos + i];
            if (c < '0' || c > '9') {
                return std::nullopt;
            }
            value = value * 10 + (c - '0');
        }
        pos += count;
        return value;
    }
};

} // namespace

std::string formatDuration(Nanoseconds duration) {
    // NOLINTNEXTLINE(google-build-using-namespace): idiomatic, function-scoped use of <chrono> calendar names
    using namespace std::chrono;
    if (duration < Nanoseconds::zero()) {
        // Avoid overflow on the most negative value by formatting its magnitude via microseconds.
        if (duration == Nanoseconds::min()) {
            return "-" + formatDuration(Nanoseconds::max());
        }
        return "-" + formatDuration(-duration);
    }
    const auto ns = duration.count();
    if (duration < 1us) {
        return std::to_string(ns) + " ns";
    }
    if (duration < 1ms) {
        return fixed3(static_cast<double>(ns) / 1e3, "us");
    }
    if (duration < 1s) {
        return fixed3(static_cast<double>(ns) / 1e6, "ms");
    }
    if (duration < 60s) {
        return fixed3(static_cast<double>(ns) / 1e9, "s");
    }
    const auto totalSeconds = duration_cast<seconds>(duration).count();
    const auto days = totalSeconds / 86'400;
    const auto hours = (totalSeconds % 86'400) / 3'600;
    const auto minutes = (totalSeconds % 3'600) / 60;
    const auto secs = totalSeconds % 60;
    if (days > 0) {
        return std::to_string(days) + "d " + twoDigits(hours) + "h " + twoDigits(minutes) + "m";
    }
    if (hours > 0) {
        return std::to_string(hours) + "h " + twoDigits(minutes) + "m " + twoDigits(secs) + "s";
    }
    return std::to_string(minutes) + "m " + twoDigits(secs) + "s";
}

std::string formatIso8601(SystemClock::time_point tp, bool withMillis, std::chrono::minutes utcOffset) {
    // NOLINTNEXTLINE(google-build-using-namespace): idiomatic, function-scoped use of <chrono> calendar names
    using namespace std::chrono;
    const auto local = time_point_cast<milliseconds>(tp) + utcOffset;
    const auto dayPoint = floor<days>(local);
    const year_month_day ymd{dayPoint};
    const hh_mm_ss<milliseconds> hms{local - dayPoint};

    std::ostringstream oss;
    oss << std::setfill('0') << std::setw(4) << static_cast<int>(ymd.year()) << '-' << std::setw(2)
        << static_cast<unsigned>(ymd.month()) << '-' << std::setw(2) << static_cast<unsigned>(ymd.day())
        << 'T' << std::setw(2) << hms.hours().count() << ':' << std::setw(2) << hms.minutes().count() << ':'
        << std::setw(2) << hms.seconds().count();
    if (withMillis) {
        oss << '.' << std::setw(3) << hms.subseconds().count();
    }
    if (utcOffset == minutes{0}) {
        oss << 'Z';
    } else {
        const auto total = utcOffset.count();
        const auto magnitude = total < 0 ? -total : total;
        oss << (total < 0 ? '-' : '+') << std::setw(2) << magnitude / 60 << ':' << std::setw(2)
            << magnitude % 60;
    }
    return oss.str();
}

std::optional<SystemClock::time_point> parseIso8601(std::string_view text) {
    // NOLINTNEXTLINE(google-build-using-namespace): idiomatic, function-scoped use of <chrono> calendar names
    using namespace std::chrono;
    Cursor cur{text};
    const auto y = cur.digits(4);
    if (!y || !cur.consume('-')) {
        return std::nullopt;
    }
    const auto mo = cur.digits(2);
    if (!mo || !cur.consume('-')) {
        return std::nullopt;
    }
    const auto d = cur.digits(2);
    if (!d) {
        return std::nullopt;
    }
    const year_month_day ymd{year{*y}, month{static_cast<unsigned>(*mo)}, day{static_cast<unsigned>(*d)}};
    if (!ymd.ok()) {
        return std::nullopt;
    }
    Nanoseconds timeOfDay{0};
    if (cur.consume('T') || cur.consume(' ')) {
        const auto h = cur.digits(2);
        if (!h || !cur.consume(':')) {
            return std::nullopt;
        }
        const auto mi = cur.digits(2);
        if (!mi) {
            return std::nullopt;
        }
        int s = 0;
        if (cur.consume(':')) {
            const auto sec = cur.digits(2);
            if (!sec) {
                return std::nullopt;
            }
            s = *sec;
        }
        if (*h > 23 || *mi > 59 || s > 60) {
            return std::nullopt;
        }
        timeOfDay = hours{*h} + minutes{*mi} + seconds{s};
        if (cur.consume('.') || cur.consume(',')) {
            std::int64_t fraction = 0;
            int digitsRead = 0;
            while (!cur.done() && std::isdigit(static_cast<unsigned char>(cur.peek())) != 0) {
                if (digitsRead < 9) {
                    fraction = fraction * 10 + (cur.peek() - '0');
                    ++digitsRead;
                }
                ++cur.pos;
            }
            if (digitsRead == 0) {
                return std::nullopt;
            }
            for (int i = digitsRead; i < 9; ++i) {
                fraction *= 10;
            }
            timeOfDay += Nanoseconds{fraction};
        }
    }
    minutes offset{0};
    if (cur.consume('Z') || cur.consume('z')) {
        // UTC
    } else if (cur.peek() == '+' || cur.peek() == '-') {
        const bool negative = cur.peek() == '-';
        ++cur.pos;
        const auto oh = cur.digits(2);
        if (!oh) {
            return std::nullopt;
        }
        cur.consume(':');
        const auto om = cur.digits(2);
        if (!om || *oh > 23 || *om > 59) {
            return std::nullopt;
        }
        offset = hours{*oh} + minutes{*om};
        if (negative) {
            offset = -offset;
        }
    }
    if (!cur.done()) {
        return std::nullopt;
    }
    const auto local = sys_days{ymd} + duration_cast<SystemClock::duration>(timeOfDay);
    return time_point_cast<SystemClock::duration>(local - offset);
}

std::optional<Nanoseconds> parseDuration(std::string_view text) {
    Cursor cur{text};
    double totalNs = 0.0;
    bool any = false;
    bool sawUnitless = false;
    while (true) {
        while (!cur.done() && std::isspace(static_cast<unsigned char>(cur.peek())) != 0) {
            ++cur.pos;
        }
        if (cur.done()) {
            break;
        }
        if (sawUnitless) {
            return std::nullopt; // a bare number must be the only component
        }
        // Number: digits with an optional single decimal point.
        const std::size_t start = cur.pos;
        bool dot = false;
        while (!cur.done()) {
            const char c = cur.peek();
            if (std::isdigit(static_cast<unsigned char>(c)) != 0) {
                ++cur.pos;
            } else if (c == '.' && !dot) {
                dot = true;
                ++cur.pos;
            } else {
                break;
            }
        }
        const std::string_view number = text.substr(start, cur.pos - start);
        if (number.empty() || number == ".") {
            return std::nullopt;
        }
        // Locale-independent decimal conversion.
        double value = 0.0;
        double fractionScale = 0.0;
        for (const char c : number) {
            if (c == '.') {
                fractionScale = 0.1;
            } else if (fractionScale > 0.0) {
                value += (c - '0') * fractionScale;
                fractionScale *= 0.1;
            } else {
                value = value * 10.0 + (c - '0');
            }
        }
        const std::size_t unitStart = cur.pos;
        while (!cur.done() && std::isalpha(static_cast<unsigned char>(cur.peek())) != 0) {
            ++cur.pos;
        }
        const std::string_view unit = text.substr(unitStart, cur.pos - unitStart);
        double scale = 0.0;
        if (unit.empty()) {
            scale = 1e9;
            sawUnitless = true;
            if (any) {
                return std::nullopt;
            }
        } else if (unit == "d") {
            scale = 86'400e9;
        } else if (unit == "h") {
            scale = 3'600e9;
        } else if (unit == "m" || unit == "min") {
            scale = 60e9;
        } else if (unit == "s") {
            scale = 1e9;
        } else if (unit == "ms") {
            scale = 1e6;
        } else if (unit == "us") {
            scale = 1e3;
        } else if (unit == "ns") {
            scale = 1.0;
        } else {
            return std::nullopt;
        }
        totalNs += value * scale;
        any = true;
    }
    if (!any || totalNs > 9.2e18) {
        return std::nullopt;
    }
    return Nanoseconds{std::llround(totalNs)};
}

// ===================================================================================================
// Stopwatch
// ===================================================================================================

Stopwatch::Stopwatch(bool startImmediately) noexcept {
    if (startImmediately) {
        start();
    }
}

void Stopwatch::start() noexcept {
    if (!running_) {
        startTime_ = SteadyClock::now();
        running_ = true;
    }
}

void Stopwatch::stop() noexcept {
    if (running_) {
        accumulated_ += std::chrono::duration_cast<Nanoseconds>(SteadyClock::now() - startTime_);
        running_ = false;
    }
}

void Stopwatch::reset() noexcept {
    running_ = false;
    accumulated_ = Nanoseconds{0};
    lastLapMark_ = Nanoseconds{0};
    laps_.clear();
}

void Stopwatch::restart() noexcept {
    reset();
    start();
}

Nanoseconds Stopwatch::elapsed() const noexcept {
    if (running_) {
        return accumulated_ + std::chrono::duration_cast<Nanoseconds>(SteadyClock::now() - startTime_);
    }
    return accumulated_;
}

double Stopwatch::elapsedSeconds() const noexcept {
    return std::chrono::duration<double>(elapsed()).count();
}

Nanoseconds Stopwatch::lap() {
    const Nanoseconds now = elapsed();
    const Nanoseconds lapTime = now - lastLapMark_;
    lastLapMark_ = now;
    laps_.push_back(lapTime);
    return lapTime;
}

// ===================================================================================================
// GameTime
// ===================================================================================================

GameTime::Seconds GameTime::advance(Seconds realDelta) noexcept {
    if (realDelta < Seconds::zero()) {
        realDelta = Seconds::zero();
    }
    ++frames_;
    realTime_ += realDelta;
    if (paused_) {
        return Seconds::zero();
    }
    const Seconds simDelta = realDelta * timeScale_;
    simTime_ += simDelta;
    return simDelta;
}

double GameTime::averageFps() const noexcept {
    return realTime_.count() > 0.0 ? static_cast<double>(frames_) / realTime_.count() : 0.0;
}

// ===================================================================================================
// TaskScheduler
// ===================================================================================================

TaskScheduler::TaskId TaskScheduler::scheduleOnce(Duration delay, Task task) {
    const std::lock_guard lock(mutex_);
    const TaskId id = nextId_++;
    const Key key{now_ + std::max(delay, Duration{0}), id};
    queue_.emplace(key, Entry{std::move(task), Duration{0}, 1});
    index_.emplace(id, key);
    return id;
}

TaskScheduler::TaskId TaskScheduler::scheduleRepeating(Duration interval, Task task,
                                                       std::size_t repetitions) {
    if (interval <= Duration{0}) {
        throw std::invalid_argument("TaskScheduler: repeating interval must be positive");
    }
    const std::lock_guard lock(mutex_);
    const TaskId id = nextId_++;
    const Key key{now_ + interval, id};
    queue_.emplace(key, Entry{std::move(task), interval, repetitions});
    index_.emplace(id, key);
    return id;
}

bool TaskScheduler::cancel(TaskId id) {
    const std::lock_guard lock(mutex_);
    const auto it = index_.find(id);
    if (it == index_.end()) {
        return false;
    }
    queue_.erase(it->second);
    index_.erase(it);
    return true;
}

std::size_t TaskScheduler::advance(Duration delta) {
    std::size_t executed = 0;
    Duration target{};
    {
        const std::lock_guard lock(mutex_);
        target = now_ + std::max(delta, Duration{0});
    }
    while (true) {
        Task task;
        {
            const std::lock_guard lock(mutex_);
            if (queue_.empty() || queue_.begin()->first.first > target) {
                now_ = target;
                break;
            }
            auto node = queue_.extract(queue_.begin());
            const auto [due, id] = node.key();
            now_ = due;
            Entry& entry = node.mapped();
            const bool repeat = entry.interval > Duration{0} && entry.remaining != 1;
            if (repeat) {
                // Keep a copy of the callable in the queue for the next run.
                task = entry.task;
                if (entry.remaining > 1) {
                    --entry.remaining;
                }
                node.key() = Key{due + entry.interval, id};
                index_[id] = node.key();
                queue_.insert(std::move(node));
            } else {
                task = std::move(entry.task);
                index_.erase(id);
            }
        }
        if (task) {
            task();
        }
        ++executed;
    }
    return executed;
}

TaskScheduler::Duration TaskScheduler::now() const {
    const std::lock_guard lock(mutex_);
    return now_;
}

std::size_t TaskScheduler::pending() const {
    const std::lock_guard lock(mutex_);
    return queue_.size();
}

// ===================================================================================================
// RateLimiter
// ===================================================================================================

RateLimiter::RateLimiter(double ratePerSecond, double burst)
    : rate_(ratePerSecond), burst_(burst), tokens_(burst) {
    if (!(ratePerSecond > 0.0) || !(burst >= 1.0)) {
        throw std::invalid_argument("RateLimiter: rate must be > 0 and burst >= 1");
    }
}

bool RateLimiter::tryAcquire(SteadyClock::time_point now, double tokens) {
    if (last_ && now > *last_) {
        const double seconds = std::chrono::duration<double>(now - *last_).count();
        tokens_ = std::min(burst_, tokens_ + seconds * rate_);
    }
    if (!last_ || now > *last_) {
        last_ = now;
    }
    if (tokens_ + 1e-12 >= tokens) {
        tokens_ -= tokens;
        return true;
    }
    return false;
}

// ===================================================================================================
// PerformanceProfiler
// ===================================================================================================

PerformanceProfiler::Scope::Scope(PerformanceProfiler& profiler, std::string name)
    : profiler_(&profiler), name_(std::move(name)), start_(SteadyClock::now()) {}

PerformanceProfiler::Scope::~Scope() {
    try {
        profiler_->record(name_, std::chrono::duration_cast<Nanoseconds>(SteadyClock::now() - start_));
    } catch (...) { // NOLINT(bugprone-empty-catch): destructors must not throw
    }
}

void PerformanceProfiler::record(const std::string& name, Nanoseconds duration) {
    const std::lock_guard lock(mutex_);
    Stats& s = stats_[name];
    if (s.count == 0) {
        s.min = duration;
        s.max = duration;
    } else {
        s.min = std::min(s.min, duration);
        s.max = std::max(s.max, duration);
    }
    ++s.count;
    s.total += duration;
}

std::optional<PerformanceProfiler::Stats> PerformanceProfiler::stats(const std::string& name) const {
    const std::lock_guard lock(mutex_);
    const auto it = stats_.find(name);
    if (it == stats_.end()) {
        return std::nullopt;
    }
    return it->second;
}

std::vector<std::string> PerformanceProfiler::sections() const {
    const std::lock_guard lock(mutex_);
    std::vector<std::string> names;
    names.reserve(stats_.size());
    for (const auto& [name, s] : stats_) {
        names.push_back(name);
    }
    return names;
}

void PerformanceProfiler::reset() {
    const std::lock_guard lock(mutex_);
    stats_.clear();
}

void PerformanceProfiler::report(std::ostream& out) const {
    const std::lock_guard lock(mutex_);
    out << std::left << std::setw(20) << "section" << std::right << std::setw(8) << "count" << std::setw(16)
        << "mean" << std::setw(16) << "total" << '\n';
    for (const auto& [name, s] : stats_) {
        out << std::left << std::setw(20) << name << std::right << std::setw(8) << s.count << std::setw(16)
            << formatDuration(s.mean()) << std::setw(16) << formatDuration(s.total) << '\n';
    }
}

// ===================================================================================================
// SpaceTime
// ===================================================================================================

double SpaceTime::decimalYear(SystemClock::time_point tp) {
    // NOLINTNEXTLINE(google-build-using-namespace): idiomatic, function-scoped use of <chrono> calendar names
    using namespace std::chrono;
    const auto dayPoint = floor<days>(tp);
    const year_month_day ymd{dayPoint};
    const sys_days begin{ymd.year() / January / 1};
    const sys_days end{(ymd.year() + years{1}) / January / 1};
    const double fraction = duration<double>(tp - begin).count() / duration<double>(end - begin).count();
    return static_cast<double>(static_cast<int>(ymd.year())) + fraction;
}

// ===================================================================================================
// Demo
// ===================================================================================================

void demonstrateTime(std::ostream& out) {
    // NOLINTNEXTLINE(google-build-using-namespace): idiomatic, function-scoped use of <chrono> calendar names
    using namespace std::chrono;
    out << "=== Time utilities ===\n";

    out << "formatDuration(1500us)  = " << formatDuration(1500us) << '\n';
    out << "formatDuration(3723s)   = " << formatDuration(3723s) << '\n';

    const auto epochPlus = sys_days{2024y / March / 1} + 12h + 34min + 56s + 789ms;
    const SystemClock::time_point tp{duration_cast<SystemClock::duration>(epochPlus.time_since_epoch())};
    out << "formatIso8601           = " << formatIso8601(tp) << '\n';
    out << "formatIso8601(+05:30)   = " << formatIso8601(tp, false, 330min) << '\n';
    if (const auto parsed = parseIso8601("2024-03-01T18:04:56.789+05:30")) {
        out << "parseIso8601 round trip = " << formatIso8601(*parsed) << '\n';
    }
    if (const auto d = parseDuration("1h 30m 15s")) {
        out << "parseDuration(1h30m15s) = " << duration_cast<seconds>(*d).count() << " s\n";
    }

    constexpr year_month_day landing{1969y / July / 20};
    static_assert(dayOfWeek(landing) == 0, "20 July 1969 was a Sunday");
    out << "Days from Apollo 11 landing to 2024-01-01: " << daysBetween(landing, 2024y / January / 1) << '\n';
    out << "2024 is leap year: " << std::boolalpha << isLeapYear(2024) << '\n';

    GameTime clock;
    clock.setTimeScale(10.0);
    for (int frame = 0; frame < 60; ++frame) {
        clock.advance(GameTime::Seconds{1.0 / 60.0});
    }
    out << "GameTime: 1 s of real time at 10x = " << std::fixed << std::setprecision(2)
        << clock.simulationTime().count() << " s simulated\n";

    TaskScheduler scheduler;
    std::vector<std::string> events;
    scheduler.scheduleOnce(milliseconds{250}, [&events] { events.emplace_back("launch"); });
    scheduler.scheduleRepeating(milliseconds{100}, [&events] { events.emplace_back("telemetry"); }, 3);
    const auto runs = scheduler.advance(milliseconds{500});
    out << "TaskScheduler ran " << runs << " tasks:";
    for (const auto& e : events) {
        out << ' ' << e;
    }
    out << '\n';

    RateLimiter limiter{2.0, 3.0};
    const auto t0 = SteadyClock::time_point{};
    int granted = 0;
    for (int i = 0; i < 5; ++i) {
        granted += limiter.tryAcquire(t0) ? 1 : 0;
    }
    out << "RateLimiter burst of 5 requests -> " << granted << " granted\n";

    PerformanceProfiler profiler;
    profiler.record("physics", 1200us);
    profiler.record("physics", 800us);
    profiler.record("render", 4ms);
    profiler.report(out);

    out << "Light delay Earth->Sun: "
        << formatDuration(
               duration_cast<Nanoseconds>(SpaceTime::lightTravelTime(SpaceTime::kAstronomicalUnit)))
        << '\n';
    out << std::defaultfloat;
}

} // namespace CppVerseHub::Utils::Time
