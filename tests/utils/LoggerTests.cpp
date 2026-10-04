// Tests for utils/Logger.hpp.
#include "utils/Logger.hpp"

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <thread>
#include <vector>

using namespace CppVerseHub::Utils;
using Catch::Matchers::ContainsSubstring;
using Catch::Matchers::StartsWith;

namespace {
std::shared_ptr<StringSink> plainStringSink() {
    auto sink = std::make_shared<StringSink>();
    PatternFormatter::Options opts;
    opts.timestamp = false;
    sink->setFormatter(std::make_shared<PatternFormatter>(opts));
    return sink;
}
} // namespace

TEST_CASE("LogLevel names round-trip through parseLogLevel", "[utils][logger]") {
    const auto level = GENERATE(LogLevel::Trace, LogLevel::Debug, LogLevel::Info, LogLevel::Warn,
                                LogLevel::Error, LogLevel::Fatal, LogLevel::Off);
    const auto parsed = parseLogLevel(toString(level));
    REQUIRE(parsed.has_value());
    CHECK(*parsed == level);
}

TEST_CASE("parseLogLevel accepts aliases and rejects junk", "[utils][logger]") {
    CHECK(parseLogLevel("warning") == LogLevel::Warn);
    CHECK(parseLogLevel(" info ") == LogLevel::Info);
    CHECK(parseLogLevel("critical") == LogLevel::Fatal);
    CHECK_FALSE(parseLogLevel("loud").has_value());
    CHECK_FALSE(parseLogLevel("").has_value());
}

TEST_CASE("Logger without sinks is silent and counts nothing", "[utils][logger]") {
    Logger logger{"quiet", LogLevel::Trace};
    CHECK_FALSE(logger.shouldLog(LogLevel::Fatal));
    logger.fatal("nobody listens");
    CHECK(logger.count(LogLevel::Fatal) == 0);
    CHECK(logger.sinkCount() == 0);
}

TEST_CASE("Logger filters by its level", "[utils][logger]") {
    Logger logger{"core", LogLevel::Warn};
    auto sink = plainStringSink();
    logger.addSink(sink);
    logger.debug("hidden");
    logger.info("hidden");
    logger.warn("shown warn");
    logger.error("shown error");
    const auto lines = sink->lines();
    REQUIRE(lines.size() == 2);
    CHECK(lines[0] == "[WARN] [core] shown warn");
    CHECK(lines[1] == "[ERROR] [core] shown error");
    CHECK(logger.count(LogLevel::Info) == 0);
    CHECK(logger.count(LogLevel::Warn) == 1);

    logger.setLevel(LogLevel::Off);
    logger.fatal("off");
    CHECK(sink->lines().size() == 2);
}

TEST_CASE("Sinks apply their own level filter independently", "[utils][logger]") {
    Logger logger{"multi", LogLevel::Trace};
    auto all = plainStringSink();
    auto errorsOnly = plainStringSink();
    errorsOnly->setLevel(LogLevel::Error);
    logger.addSink(all);
    logger.addSink(errorsOnly);
    logger.trace("t");
    logger.info("i");
    logger.error("e");
    CHECK(all->lines().size() == 3);
    REQUIRE(errorsOnly->lines().size() == 1);
    CHECK_THAT(errorsOnly->lines()[0], ContainsSubstring("e"));
    CHECK(errorsOnly->recordsWritten() == 1);
}

TEST_CASE("OStreamSink writes newline-terminated lines to an ostringstream", "[utils][logger]") {
    std::ostringstream oss;
    Logger logger{"", LogLevel::Info};
    auto sink = std::make_shared<OStreamSink>(oss);
    PatternFormatter::Options opts;
    opts.timestamp = false;
    opts.level = false;
    sink->setFormatter(std::make_shared<PatternFormatter>(opts));
    logger.addSink(sink);
    logger.info("alpha");
    logger.logArgs(LogLevel::Info, "x=", 42, ", y=", 1.5);
    logger.flush();
    CHECK(oss.str() == "alpha\nx=42, y=1.5\n");
}

TEST_CASE("PatternFormatter includes timestamp, thread and location when enabled", "[utils][logger]") {
    PatternFormatter::Options opts;
    opts.threadId = true;
    opts.location = true;
    const PatternFormatter fmt{opts};
    LogRecord record;
    record.level = LogLevel::Info;
    record.loggerName = "fmt";
    record.message = "hello";
    record.timestamp = std::chrono::system_clock::time_point{std::chrono::seconds{86'400}};
    record.location = std::source_location::current();
    const std::string line = fmt.format(record);
    CHECK_THAT(line, StartsWith("1970-01-02T00:00:00.000Z [INFO] [fmt] [tid "));
    CHECK_THAT(line, ContainsSubstring("(LoggerTests.cpp:"));
    CHECK_THAT(line, ContainsSubstring("hello"));
}

TEST_CASE("JsonFormatter escapes special characters", "[utils][logger]") {
    CHECK(JsonFormatter::escape("a\"b\\c\n\x01") == "a\\\"b\\\\c\\n\\u0001");
    LogRecord record;
    record.level = LogLevel::Error;
    record.loggerName = "json";
    record.message = "say \"hi\"";
    const std::string line = JsonFormatter{}.format(record);
    CHECK_THAT(line, StartsWith("{\"timestamp\":\""));
    CHECK_THAT(line, ContainsSubstring("\"level\":\"ERROR\""));
    CHECK_THAT(line, ContainsSubstring("\"message\":\"say \\\"hi\\\"\""));
    CHECK(line.back() == '}');
}

TEST_CASE("CPPVERSEHUB_LOG evaluates its arguments only when enabled", "[utils][logger]") {
    Logger logger{"lazy", LogLevel::Info};
    auto sink = plainStringSink();
    logger.addSink(sink);
    int evaluations = 0;
    auto expensive = [&evaluations] {
        ++evaluations;
        return 7;
    };
    CPPVERSEHUB_LOG(logger, LogLevel::Debug, "value ", expensive());
    CHECK(evaluations == 0);
    CPPVERSEHUB_LOG(logger, LogLevel::Info, "value ", expensive());
    CHECK(evaluations == 1);
    REQUIRE(sink->lines().size() == 1);
    CHECK(sink->lines()[0] == "[INFO] [lazy] value 7");
}

TEST_CASE("RingBufferSink keeps only the newest lines", "[utils][logger]") {
    Logger logger{"ring", LogLevel::Info};
    auto ring = std::make_shared<RingBufferSink>(3);
    PatternFormatter::Options opts;
    opts.timestamp = opts.level = opts.loggerName = false;
    ring->setFormatter(std::make_shared<PatternFormatter>(opts));
    logger.addSink(ring);
    for (int i = 1; i <= 5; ++i) {
        logger.logArgs(LogLevel::Info, i);
    }
    CHECK(ring->lines() == std::vector<std::string>{"3", "4", "5"});
    CHECK(ring->capacity() == 3);
    CHECK(RingBufferSink{0}.capacity() == 1);
}

TEST_CASE("CallbackSink forwards records and removeSink/clearSinks detach", "[utils][logger]") {
    Logger logger{"cb", LogLevel::Info};
    std::vector<LogLevel> seen;
    auto sink = std::make_shared<CallbackSink>(
        [&seen](std::string_view, const LogRecord& r) { seen.push_back(r.level); });
    logger.addSink(sink);
    logger.addSink(nullptr); // ignored
    CHECK(logger.sinkCount() == 1);
    logger.info("one");
    logger.warn("two");
    CHECK(seen == std::vector<LogLevel>{LogLevel::Info, LogLevel::Warn});
    CHECK(logger.removeSink(sink));
    CHECK_FALSE(logger.removeSink(sink));
    logger.error("three");
    CHECK(seen.size() == 2);
    logger.addSink(sink);
    logger.clearSinks();
    CHECK(logger.sinkCount() == 0);
}

TEST_CASE("Logger swallows and counts sink exceptions", "[utils][logger]") {
    Logger logger{"faulty", LogLevel::Info};
    auto bad = std::make_shared<CallbackSink>(
        [](std::string_view, const LogRecord&) { throw std::runtime_error("x"); });
    auto good = plainStringSink();
    logger.addSink(bad);
    logger.addSink(good);
    REQUIRE_NOTHROW(logger.info("still works"));
    CHECK(logger.sinkErrors() == 1);
    CHECK(good->lines().size() == 1);
}

TEST_CASE("Logger is thread-safe under concurrent logging", "[utils][logger][threads]") {
    Logger logger{"mt", LogLevel::Info};
    auto sink = plainStringSink();
    logger.addSink(sink);
    constexpr int kThreads = 8;
    constexpr int kPerThread = 200;
    std::vector<std::thread> threads;
    for (int t = 0; t < kThreads; ++t) {
        threads.emplace_back([&logger, t] {
            for (int i = 0; i < kPerThread; ++i) {
                logger.logArgs(LogLevel::Info, "t", t, "-", i);
            }
        });
    }
    for (auto& th : threads) {
        th.join();
    }
    const auto lines = sink->lines();
    CHECK(lines.size() == static_cast<std::size_t>(kThreads * kPerThread));
    CHECK(logger.count(LogLevel::Info) == static_cast<std::size_t>(kThreads * kPerThread));
    for (const auto& line : lines) {
        CHECK_THAT(line, StartsWith("[INFO] [mt] t"));
    }
}

TEST_CASE("AsyncSink delivers every record before flush returns", "[utils][logger][threads]") {
    auto target = plainStringSink();
    {
        Logger logger{"async", LogLevel::Info};
        auto async = std::make_shared<AsyncSink>(target);
        logger.addSink(async);
        std::vector<std::thread> threads;
        for (int t = 0; t < 4; ++t) {
            threads.emplace_back([&logger] {
                for (int i = 0; i < 100; ++i) {
                    logger.info("m");
                }
            });
        }
        for (auto& th : threads) {
            th.join();
        }
        logger.flush();
        CHECK(target->lines().size() == 400);
        CHECK(async->dropped() == 0);
    }
    CHECK(target->lines().size() == 400);
    CHECK_THROWS_AS(AsyncSink(nullptr), std::invalid_argument);
}

TEST_CASE("Bounded AsyncSink accounts for every record as delivered or dropped", "[utils][logger][threads]") {
    auto target = plainStringSink();
    Logger logger{"bounded", LogLevel::Info};
    auto async = std::make_shared<AsyncSink>(target, 1);
    logger.addSink(async);
    for (int i = 0; i < 1000; ++i) {
        logger.info("burst");
    }
    logger.flush(); // waits until the worker is idle and the queue is empty
    CHECK(target->lines().size() + async->dropped() == 1000);
    CHECK(target->lines().size() >= 1);
    logger.clearSinks();
    async.reset(); // joins the worker
    CHECK(target->lines().size() >= 1);
}

TEST_CASE("FileSink appends to a file and rotates by size", "[utils][logger][file]") {
    const auto dir = std::filesystem::temp_directory_path() / "cppversehub_utils_filesink_test";
    std::filesystem::remove_all(dir);
    const auto path = dir / "app.log";
    {
        Logger logger{"file", LogLevel::Info};
        auto sink = std::make_shared<FileSink>(path, 200, 2);
        PatternFormatter::Options opts;
        opts.timestamp = false;
        sink->setFormatter(std::make_shared<PatternFormatter>(opts));
        logger.addSink(sink);
        for (int i = 0; i < 30; ++i) {
            logger.logArgs(LogLevel::Info, "line number ", i);
        }
        logger.flush();
        CHECK(sink->rotations() > 0);
        CHECK(sink->path() == path);
    }
    CHECK(std::filesystem::exists(path));
    CHECK(std::filesystem::exists(dir / "app.log.1"));
    CHECK(std::filesystem::exists(dir / "app.log.2"));
    CHECK_FALSE(std::filesystem::exists(dir / "app.log.3"));
    std::ifstream in(path);
    std::string last;
    for (std::string line; std::getline(in, line);) {
        last = line;
    }
    CHECK(last == "[INFO] [file] line number 29");
    in.close();
    std::filesystem::remove_all(dir);
}

TEST_CASE("LoggerRegistry hands out shared named loggers", "[utils][logger]") {
    auto& registry = LoggerRegistry::instance();
    registry.clear();
    auto a = registry.get("net");
    auto b = registry.get("net");
    CHECK(a == b);
    CHECK(registry.contains("net"));
    CHECK(registry.size() == 1);
    CHECK(a->sinkCount() == 0); // silent by default
    registry.setGlobalLevel(LogLevel::Error);
    CHECK(a->level() == LogLevel::Error);
    CHECK(registry.get("other")->level() == LogLevel::Error);
    CHECK(registry.remove("net"));
    CHECK_FALSE(registry.remove("net"));
    CHECK(a->name() == "net"); // handle outlives removal
    registry.setGlobalLevel(LogLevel::Info);
    registry.clear();
    CHECK(registry.size() == 0);
}

TEST_CASE("ScopedLogTimer reports scope duration", "[utils][logger]") {
    Logger logger{"timer", LogLevel::Debug};
    auto sink = plainStringSink();
    logger.addSink(sink);
    { const ScopedLogTimer timer{logger, "compute"}; }
    REQUIRE(sink->lines().size() == 1);
    CHECK_THAT(sink->lines()[0], StartsWith("[DEBUG] [timer] compute took "));
    CHECK_THAT(sink->lines()[0], ContainsSubstring(" us"));
}
