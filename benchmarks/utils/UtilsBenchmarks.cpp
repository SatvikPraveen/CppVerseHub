// Google Benchmark micro-benchmarks for the utils module.
#include "utils/ConfigManager.hpp"
#include "utils/FileParser.hpp"
#include "utils/Logger.hpp"
#include "utils/MathUtils.hpp"
#include "utils/StringUtils.hpp"
#include "utils/TimeUtils.hpp"

#include <benchmark/benchmark.h>

#include <memory>
#include <string>
#include <vector>

using namespace CppVerseHub::Utils;

namespace {

std::string makeJsonDocument(int items) {
    JsonValue root;
    for (int i = 0; i < items; ++i) {
        root["ships"].push_back(JsonValue::object(
            {{"id", i}, {"name", "ship-" + std::to_string(i)}, {"mass", 1.5 * i}, {"active", i % 2 == 0}}));
    }
    return root.dump();
}

std::string makeCsv(int rows) {
    std::string csv = "id,name,notes\n";
    for (int i = 0; i < rows; ++i) {
        csv += std::to_string(i) + ",ship-" + std::to_string(i) + ",\"note, with \"\"quotes\"\"\"\n";
    }
    return csv;
}

} // namespace

static void BM_LoggerDisabledLevel(benchmark::State& state) {
    Logger logger{"bench", LogLevel::Error};
    logger.addSink(std::make_shared<StringSink>());
    for (auto _ : state) {
        CPPVERSEHUB_LOG(logger, LogLevel::Debug, "value ", 42);
    }
}
BENCHMARK(BM_LoggerDisabledLevel);

static void BM_LoggerRingBufferSink(benchmark::State& state) {
    Logger logger{"bench", LogLevel::Info};
    auto sink = std::make_shared<RingBufferSink>(64);
    PatternFormatter::Options opts;
    opts.timestamp = false;
    sink->setFormatter(std::make_shared<PatternFormatter>(opts));
    logger.addSink(sink);
    for (auto _ : state) {
        logger.info("telemetry packet received");
    }
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_LoggerRingBufferSink);

static void BM_JsonParse(benchmark::State& state) {
    const std::string doc = makeJsonDocument(static_cast<int>(state.range(0)));
    for (auto _ : state) {
        benchmark::DoNotOptimize(JsonParser::parse(doc));
    }
    state.SetBytesProcessed(state.iterations() * static_cast<std::int64_t>(doc.size()));
}
BENCHMARK(BM_JsonParse)->Arg(10)->Arg(1000);

static void BM_JsonDump(benchmark::State& state) {
    const JsonValue doc = JsonParser::parse(makeJsonDocument(1000));
    for (auto _ : state) {
        benchmark::DoNotOptimize(doc.dump());
    }
}
BENCHMARK(BM_JsonDump);

static void BM_CsvParse(benchmark::State& state) {
    const std::string csv = makeCsv(static_cast<int>(state.range(0)));
    for (auto _ : state) {
        benchmark::DoNotOptimize(CsvParser{}.parse(csv));
    }
    state.SetBytesProcessed(state.iterations() * static_cast<std::int64_t>(csv.size()));
}
BENCHMARK(BM_CsvParse)->Arg(100)->Arg(5000);

static void BM_Levenshtein(benchmark::State& state) {
    const std::string a(static_cast<std::size_t>(state.range(0)), 'a');
    std::string b = a;
    for (std::size_t i = 0; i < b.size(); i += 3) {
        b[i] = 'b';
    }
    for (auto _ : state) {
        benchmark::DoNotOptimize(String::levenshteinDistance(a, b));
    }
    state.SetComplexityN(state.range(0));
}
BENCHMARK(BM_Levenshtein)->RangeMultiplier(4)->Range(16, 1024)->Complexity(benchmark::oNSquared);

static void BM_Base64RoundTrip(benchmark::State& state) {
    const std::string data(4096, '\x5A');
    for (auto _ : state) {
        benchmark::DoNotOptimize(String::base64Decode(String::base64Encode(data)));
    }
    state.SetBytesProcessed(state.iterations() * 4096);
}
BENCHMARK(BM_Base64RoundTrip);

static void BM_MatrixMultiply(benchmark::State& state) {
    const auto n = static_cast<std::size_t>(state.range(0));
    Math::RandomGenerator rng{1};
    Math::Matrix a(n, n);
    Math::Matrix b(n, n);
    for (std::size_t r = 0; r < n; ++r) {
        for (std::size_t c = 0; c < n; ++c) {
            a(r, c) = rng.uniformReal();
            b(r, c) = rng.uniformReal();
        }
    }
    for (auto _ : state) {
        benchmark::DoNotOptimize(a * b);
    }
}
BENCHMARK(BM_MatrixMultiply)->Arg(16)->Arg(64);

static void BM_MatrixInverse(benchmark::State& state) {
    const auto n = static_cast<std::size_t>(state.range(0));
    Math::Matrix a = Math::Matrix::identity(n) * static_cast<double>(n);
    Math::RandomGenerator rng{2};
    for (std::size_t r = 0; r < n; ++r) {
        for (std::size_t c = 0; c < n; ++c) {
            a(r, c) += rng.uniformReal();
        }
    }
    for (auto _ : state) {
        benchmark::DoNotOptimize(a.inverse());
    }
}
BENCHMARK(BM_MatrixInverse)->Arg(32);

static void BM_PerlinFractal(benchmark::State& state) {
    const Math::PerlinNoise noise{3};
    double x = 0.0;
    for (auto _ : state) {
        benchmark::DoNotOptimize(noise.fractal(x, x * 0.5, 0.25, 6));
        x += 0.01;
    }
}
BENCHMARK(BM_PerlinFractal);

static void BM_NBodyStep(benchmark::State& state) {
    Math::Space::NBodySimulator sim{1.0, 0.01};
    Math::RandomGenerator rng{4};
    for (int i = 0; i < state.range(0); ++i) {
        sim.addBody({rng.unitVector() * 10.0, rng.unitVector() * 0.1, 1.0});
    }
    for (auto _ : state) {
        sim.step(0.001);
    }
}
BENCHMARK(BM_NBodyStep)->Arg(32)->Arg(128);

static void BM_ConfigRead(benchmark::State& state) {
    ConfigManager config;
    for (int i = 0; i < 100; ++i) {
        config.set("section" + std::to_string(i % 10) + ".key" + std::to_string(i), i);
    }
    const std::string key = "section5.key55";
    for (auto _ : state) {
        benchmark::DoNotOptimize(config.get<int>(key));
    }
}
BENCHMARK(BM_ConfigRead);

static void BM_Iso8601FormatParse(benchmark::State& state) {
    const auto now = Time::SystemClock::time_point{std::chrono::seconds{1'700'000'000}};
    for (auto _ : state) {
        benchmark::DoNotOptimize(Time::parseIso8601(Time::formatIso8601(now)));
    }
}
BENCHMARK(BM_Iso8601FormatParse);
