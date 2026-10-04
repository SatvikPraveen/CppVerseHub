// Tests for utils/TimeUtils.hpp.
#include "utils/TimeUtils.hpp"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

using namespace CppVerseHub::Utils::Time;
using namespace std::chrono;
using Catch::Approx;

TEST_CASE("formatDuration picks a unit by magnitude", "[utils][time]") {
    CHECK(formatDuration(750ns) == "750 ns");
    CHECK(formatDuration(12'500ns) == "12.500 us");
    CHECK(formatDuration(3'250us) == "3.250 ms");
    CHECK(formatDuration(1'500ms) == "1.500 s");
    CHECK(formatDuration(125s) == "2m 05s");
    CHECK(formatDuration(3723s) == "1h 02m 03s");
    CHECK(formatDuration(hours{51}) == "2d 03h 00m");
    CHECK(formatDuration(-1'500ms) == "-1.500 s");
    CHECK(formatDuration(0ns) == "0 ns");
}

TEST_CASE("formatIso8601 renders UTC and fixed offsets", "[utils][time]") {
    const SystemClock::time_point tp{sys_days{2024y / March / 1} + 12h + 34min + 56s + 789ms};
    CHECK(formatIso8601(tp) == "2024-03-01T12:34:56.789Z");
    CHECK(formatIso8601(tp, false) == "2024-03-01T12:34:56Z");
    CHECK(formatIso8601(tp, false, 330min) == "2024-03-01T18:04:56+05:30");
    CHECK(formatIso8601(tp, false, -90min) == "2024-03-01T11:04:56-01:30");
    CHECK(formatIso8601(SystemClock::time_point{}) == "1970-01-01T00:00:00.000Z");
}

TEST_CASE("parseIso8601 accepts valid timestamps and normalises offsets", "[utils][time]") {
    const SystemClock::time_point expected{sys_days{2024y / March / 1} + 12h + 34min + 56s + 789ms};
    const auto text = GENERATE(as<std::string>{}, "2024-03-01T12:34:56.789Z", "2024-03-01 12:34:56.789",
                               "2024-03-01T18:04:56.789+05:30", "2024-03-01T07:04:56.789-0530");
    const auto parsed = parseIso8601(text);
    REQUIRE(parsed.has_value());
    CHECK(*parsed == expected);
}

TEST_CASE("parseIso8601 handles date-only and round-trips formatIso8601", "[utils][time]") {
    const auto dateOnly = parseIso8601("2000-02-29");
    REQUIRE(dateOnly.has_value());
    CHECK(*dateOnly == SystemClock::time_point{sys_days{2000y / February / 29}});
    const SystemClock::time_point tp{sys_days{1999y / December / 31} + 23h + 59min + 59s + 1ms};
    CHECK(parseIso8601(formatIso8601(tp)) == tp);
}

TEST_CASE("parseIso8601 rejects malformed or out-of-range input", "[utils][time]") {
    const auto text = GENERATE(as<std::string>{}, "", "2024", "2024-13-01", "2023-02-29", "2024-01-01T24:00",
                               "2024-01-01T10:61", "2024-01-01T10:00:00.", "2024-01-01T10:00Zjunk", "24-01-01",
                               "2024-01-01T10:00+25:00");
    CHECK_FALSE(parseIso8601(text).has_value());
}

TEST_CASE("parseDuration understands compound units", "[utils][time]") {
    CHECK(parseDuration("1h30m") == Nanoseconds{90min});
    CHECK(parseDuration("250ms") == Nanoseconds{250ms});
    CHECK(parseDuration("2.5s") == Nanoseconds{2500ms});
    CHECK(parseDuration("1d 2h") == Nanoseconds{26h});
    CHECK(parseDuration("90") == Nanoseconds{90s});
    CHECK(parseDuration(" 10us 5ns ") == Nanoseconds{10'005ns});
    CHECK_FALSE(parseDuration("").has_value());
    CHECK_FALSE(parseDuration("5 parsecs").has_value());
    CHECK_FALSE(parseDuration("1h 30").has_value());
    CHECK_FALSE(parseDuration("abc").has_value());
}

TEST_CASE("Calendar helpers are correct and constexpr", "[utils][time]") {
    static_assert(isLeapYear(2000) && !isLeapYear(1900) && isLeapYear(2024) && !isLeapYear(2023));
    static_assert(daysInMonth(2024, 2) == 29 && daysInMonth(2023, 2) == 28 && daysInMonth(2023, 4) == 30);
    static_assert(daysInMonth(2023, 13) == 0);
    static_assert(dayOfWeek(2024y / January / 1) == 1); // Monday
    static_assert(daysBetween(2024y / January / 1, 2025y / January / 1) == 366);
    static_assert(dayOfYear(2024y / December / 31) == 366);
    CHECK(daysBetween(2025y / January / 1, 2024y / January / 1) == -366);
    CHECK(dayOfYear(2023y / March / 1) == 60);
}

TEST_CASE("Stopwatch accumulates across start/stop and records laps", "[utils][time]") {
    Stopwatch sw;
    CHECK_FALSE(sw.running());
    CHECK(sw.elapsed() == 0ns);
    sw.start();
    std::this_thread::sleep_for(2ms);
    sw.stop();
    const auto first = sw.elapsed();
    CHECK(first >= 2ms);
    std::this_thread::sleep_for(2ms);
    CHECK(sw.elapsed() == first); // stopped: no growth
    sw.start();
    const auto lap1 = sw.lap();
    const auto lap2 = sw.lap();
    CHECK(sw.laps().size() == 2);
    CHECK(lap1 >= first);
    CHECK(lap2 >= 0ns);
    CHECK(sw.elapsedAs<microseconds>() >= duration_cast<microseconds>(first));
    sw.reset();
    CHECK_FALSE(sw.running());
    CHECK(sw.elapsed() == 0ns);
    CHECK(sw.laps().empty());
    Stopwatch running{true};
    CHECK(running.running());
    CHECK(running.elapsedSeconds() >= 0.0);
}

TEST_CASE("GameTime scales, pauses and counts frames deterministically", "[utils][time]") {
    GameTime clock;
    clock.setTimeScale(2.0);
    CHECK(clock.advance(GameTime::Seconds{0.5}).count() == Approx(1.0));
    clock.pause();
    CHECK(clock.advance(GameTime::Seconds{1.0}).count() == 0.0);
    clock.resume();
    clock.setTimeScale(-3.0);
    CHECK(clock.timeScale() == 0.0);
    clock.setTimeScale(1.0);
    clock.advance(GameTime::Seconds{0.5});
    clock.advance(GameTime::Seconds{-1.0}); // ignored
    CHECK(clock.simulationTime().count() == Approx(1.5));
    CHECK(clock.realTime().count() == Approx(2.0));
    CHECK(clock.frameCount() == 4);
    CHECK(clock.averageFps() == Approx(2.0));
    CHECK(GameTime{}.averageFps() == 0.0);
}

TEST_CASE("TaskScheduler runs tasks in due-time order", "[utils][time][scheduler]") {
    TaskScheduler scheduler;
    std::vector<std::string> log;
    scheduler.scheduleOnce(milliseconds{300}, [&log] { log.emplace_back("c"); });
    scheduler.scheduleOnce(milliseconds{100}, [&log] { log.emplace_back("a"); });
    scheduler.scheduleOnce(milliseconds{100}, [&log] { log.emplace_back("b"); }); // tie: FIFO
    CHECK(scheduler.pending() == 3);
    CHECK(scheduler.advance(milliseconds{99}) == 0);
    CHECK(scheduler.advance(milliseconds{1}) == 2);
    CHECK(log == std::vector<std::string>{"a", "b"});
    CHECK(scheduler.now() == milliseconds{100});
    CHECK(scheduler.advance(milliseconds{500}) == 1);
    CHECK(log.back() == "c");
    CHECK(scheduler.pending() == 0);
    CHECK(scheduler.now() == milliseconds{600});
}

TEST_CASE("TaskScheduler repeating tasks honour repetition counts and cancellation", "[utils][time][scheduler]") {
    TaskScheduler scheduler;
    int limited = 0;
    int unlimited = 0;
    scheduler.scheduleRepeating(milliseconds{10}, [&limited] { ++limited; }, 3);
    const auto id = scheduler.scheduleRepeating(milliseconds{25}, [&unlimited] { ++unlimited; });
    scheduler.advance(milliseconds{100});
    CHECK(limited == 3);
    CHECK(unlimited == 4);
    CHECK(scheduler.cancel(id));
    CHECK_FALSE(scheduler.cancel(id));
    scheduler.advance(milliseconds{100});
    CHECK(unlimited == 4);
    CHECK(scheduler.pending() == 0);
    CHECK_THROWS_AS(scheduler.scheduleRepeating(milliseconds{0}, [] {}), std::invalid_argument);
}

TEST_CASE("TaskScheduler supports re-entrant scheduling from callbacks", "[utils][time][scheduler]") {
    TaskScheduler scheduler;
    std::vector<milliseconds> times;
    std::function<void()> chain = [&] {
        times.push_back(scheduler.now());
        if (times.size() < 3) {
            scheduler.scheduleOnce(milliseconds{5}, chain);
        }
    };
    scheduler.scheduleOnce(milliseconds{5}, chain);
    scheduler.advance(milliseconds{50});
    CHECK(times == std::vector<milliseconds>{milliseconds{5}, milliseconds{10}, milliseconds{15}});
}

TEST_CASE("RateLimiter implements a token bucket", "[utils][time]") {
    RateLimiter limiter{10.0, 2.0};
    const SteadyClock::time_point t0{};
    CHECK(limiter.tryAcquire(t0));
    CHECK(limiter.tryAcquire(t0));
    CHECK_FALSE(limiter.tryAcquire(t0));
    CHECK(limiter.tryAcquire(t0 + 100ms)); // 1 token refilled
    CHECK_FALSE(limiter.tryAcquire(t0 + 100ms));
    CHECK(limiter.tryAcquire(t0 + 10s, 2.0)); // capped at burst
    CHECK(limiter.available() == Approx(0.0).margin(1e-9));
    CHECK_THROWS_AS(RateLimiter(0.0, 1.0), std::invalid_argument);
    CHECK_THROWS_AS(RateLimiter(1.0, 0.5), std::invalid_argument);
}

TEST_CASE("PerformanceProfiler aggregates samples and reports", "[utils][time]") {
    PerformanceProfiler profiler;
    profiler.record("physics", 2ms);
    profiler.record("physics", 4ms);
    profiler.record("ai", 1ms);
    {
        const PerformanceProfiler::Scope scope{profiler, "scoped"};
    }
    const auto stats = profiler.stats("physics");
    REQUIRE(stats.has_value());
    CHECK(stats->count == 2);
    CHECK(stats->total == 6ms);
    CHECK(stats->min == 2ms);
    CHECK(stats->max == 4ms);
    CHECK(stats->mean() == 3ms);
    CHECK(profiler.stats("scoped")->count == 1);
    CHECK_FALSE(profiler.stats("missing").has_value());
    CHECK(profiler.sections() == std::vector<std::string>{"ai", "physics", "scoped"});
    std::ostringstream oss;
    profiler.report(oss);
    CHECK(oss.str().find("physics") != std::string::npos);
    CHECK(oss.str().find("3.000 ms") != std::string::npos);
    profiler.reset();
    CHECK(profiler.sections().empty());
}

TEST_CASE("SpaceTime helpers", "[utils][time]") {
    static_assert(SpaceTime::lightTravelTime(SpaceTime::kSpeedOfLight).count() == 1.0);
    CHECK(SpaceTime::lightTravelTime(SpaceTime::kAstronomicalUnit).count() == Approx(499.004784).epsilon(1e-6));
    const SystemClock::time_point newYear{sys_days{2023y / January / 1}};
    CHECK(SpaceTime::decimalYear(newYear) == Approx(2023.0));
    const SystemClock::time_point midYear{sys_days{2023y / January / 1} + hours{365 * 12}};
    CHECK(SpaceTime::decimalYear(midYear) == Approx(2023.5));
}
