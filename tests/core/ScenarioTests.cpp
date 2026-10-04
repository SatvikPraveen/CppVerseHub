#include <catch2/catch_test_macros.hpp>

#include <filesystem>
#include <fstream>
#include <string>

#include "core/Scenario.hpp"

using namespace CppVerseHub::Core;

namespace {

/// Unique temp file removed on scope exit.
struct TempFile {
    std::filesystem::path path;
    explicit TempFile(const std::string& name)
        : path(std::filesystem::temp_directory_path() / ("cppversehub_core_" + name + ".json")) {}
    ~TempFile() {
        std::error_code ec;
        std::filesystem::remove(path, ec);
    }
    TempFile(const TempFile&) = delete;
    TempFile& operator=(const TempFile&) = delete;
};

SampleScenarioOptions opts() {
    SampleScenarioOptions o;
    o.seed = 99;
    o.planets = 7;
    o.fleets = 5;
    return o;
}

} // namespace

TEST_CASE("Scenario save -> load -> save is a fixed point", "[core][scenario][json]") {
    auto engine = makeSampleScenario(opts());
    engine->runSteps(37); // mid-flight: missions active, carries non-zero
    const nlohmann::json saved = saveScenario(*engine);
    auto restored = loadScenario(saved);
    REQUIRE(saveScenario(*restored) == saved);
    REQUIRE(restored->stateDigest() == engine->stateDigest());
    REQUIRE(restored->tick() == engine->tick());
    REQUIRE(restored->stats() == engine->stats());
    REQUIRE(restored->galaxy().entityCount() == engine->galaxy().entityCount());
    REQUIRE(restored->galaxy().missionCount() == engine->galaxy().missionCount());
}

TEST_CASE("A restored scenario continues bit-identically", "[core][scenario][determinism]") {
    auto engine = makeSampleScenario(opts());
    engine->runSteps(25);
    auto restored = loadScenario(saveScenario(*engine));
    for (int i = 0; i < 8; ++i) {
        engine->runSteps(50);
        restored->runSteps(50);
        REQUIRE(restored->stateDigest() == engine->stateDigest());
    }
    REQUIRE(restored->galaxy().resources().checkConservation());
}

TEST_CASE("Scenario file round trip through a temporary directory", "[core][scenario][io]") {
    TempFile file("roundtrip");
    auto engine = makeSampleScenario(opts());
    engine->runSteps(10);
    saveScenarioFile(*engine, file.path);
    REQUIRE(std::filesystem::exists(file.path));
    REQUIRE(std::filesystem::file_size(file.path) > 100);
    auto loaded = loadScenarioFile(file.path);
    REQUIRE(loaded->stateDigest() == engine->stateDigest());
}

TEST_CASE("Malformed scenarios raise SerializationException", "[core][scenario][errors]") {
    REQUIRE_THROWS_AS(loadScenario(nlohmann::json::object()), SerializationException);
    REQUIRE_THROWS_AS(loadScenario(nlohmann::json{{"format", "something-else"}}), SerializationException);

    auto engine = makeSampleScenario(opts());
    nlohmann::json doc = saveScenario(*engine);
    nlohmann::json badVersion = doc;
    badVersion["version"] = 99;
    REQUIRE_THROWS_AS(loadScenario(badVersion), SerializationException);
    nlohmann::json badKind = doc;
    badKind["galaxy"]["entities"][0]["kind"] = "blackhole";
    REQUIRE_THROWS_AS(loadScenario(badKind), SerializationException);
    nlohmann::json badRng = doc;
    badRng["rngState"] = "garbage";
    REQUIRE_THROWS_AS(loadScenario(badRng), SerializationException);
    nlohmann::json missingAccount = doc;
    missingAccount["galaxy"]["resources"]["accounts"].erase(0);
    REQUIRE_THROWS_AS(loadScenario(missingAccount), SerializationException);
    nlohmann::json badStep = doc;
    badStep["config"]["timeStep"] = -1.0;
    REQUIRE_THROWS_AS(loadScenario(badStep), SerializationException);

    TempFile garbage("garbage");
    {
        std::ofstream out(garbage.path);
        out << "{ not json";
    }
    REQUIRE_THROWS_AS(loadScenarioFile(garbage.path), SerializationException);
    REQUIRE_THROWS_AS(loadScenarioFile(garbage.path.parent_path() / "cppversehub_core_missing_file.json"),
                      SerializationException);
}

TEST_CASE("Sample generator is reproducible and validates options", "[core][scenario]") {
    auto a = makeSampleGalaxy(opts());
    auto b = makeSampleGalaxy(opts());
    nlohmann::json ja;
    nlohmann::json jb;
    a->toJson(ja);
    b->toJson(jb);
    REQUIRE(ja == jb);
    REQUIRE(a->all<Planet>().size() == 7);
    REQUIRE(a->all<Fleet>().size() == 5);
    SampleScenarioOptions none = opts();
    none.planets = 0;
    REQUIRE_THROWS_AS(makeSampleGalaxy(none), InvalidArgumentException);
}
