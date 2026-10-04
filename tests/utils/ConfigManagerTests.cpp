// Tests for utils/ConfigManager.hpp.
#include "utils/ConfigManager.hpp"
#include "utils/FileParser.hpp"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

#include <atomic>
#include <filesystem>
#include <thread>
#include <vector>

using namespace CppVerseHub::Utils;
using Catch::Approx;
using Strings = std::vector<std::string>;

TEST_CASE("ConfigValue::parse infers types", "[utils][config]") {
    CHECK(ConfigValue::parse("true").type() == ConfigValue::Type::Bool);
    CHECK(ConfigValue::parse("OFF") == ConfigValue{false});
    CHECK(ConfigValue::parse(" 42 ") == ConfigValue{42});
    CHECK(ConfigValue::parse("-7").type() == ConfigValue::Type::Int);
    CHECK(ConfigValue::parse("2.5") == ConfigValue{2.5});
    CHECK(ConfigValue::parse("1e3").type() == ConfigValue::Type::Double);
    CHECK(ConfigValue::parse("[a, \"b c\", d]") == ConfigValue{ConfigValue::List{"a", "b c", "d"}});
    CHECK(ConfigValue::parse("[]") == ConfigValue{ConfigValue::List{}});
    CHECK(ConfigValue::parse("\"42\"") == ConfigValue{"42"});
    CHECK(ConfigValue::parse("hello world") == ConfigValue{"hello world"});
    CHECK(ConfigValue::parse("'single'") == ConfigValue{"single"});
    CHECK(ConfigValue::parse("\"esc\\\"aped\"") == ConfigValue{"esc\"aped"});
}

TEST_CASE("ConfigValue conversions follow documented rules", "[utils][config]") {
    const ConfigValue i{42};
    CHECK(i.as<int>() == 42);
    CHECK(i.as<double>() == 42.0);
    CHECK(i.as<bool>() == true);
    CHECK(i.as<std::string>() == "42");
    CHECK_FALSE(ConfigValue{300}.as<std::uint8_t>().has_value());
    CHECK_FALSE(ConfigValue{-1}.as<unsigned>().has_value());

    const ConfigValue d{2.5};
    CHECK_FALSE(d.as<int>().has_value());
    CHECK(ConfigValue{3.0}.as<int>() == 3);
    CHECK(d.as<std::string>() == "2.5");
    CHECK(ConfigValue{3.0}.toString() == "3.0");
    CHECK_FALSE(d.as<bool>().has_value());

    const ConfigValue s{"123"};
    CHECK(s.as<int>() == 123);
    CHECK(ConfigValue{"yes"}.as<bool>() == true);
    CHECK_FALSE(ConfigValue{"abc"}.as<int>().has_value());
    CHECK(s.as<ConfigValue::List>() == ConfigValue::List{"123"});

    const ConfigValue l{ConfigValue::List{"x", "y"}};
    CHECK(l.toString() == "[x, y]");
    CHECK_FALSE(l.as<int>().has_value());
    CHECK(l.is<ConfigValue::List>());
    CHECK(i.is<long>());
    CHECK_FALSE(i.is<double>());
    CHECK(i.typeName() == "int");
    CHECK_THROWS_AS(l.get<double>(), ConfigError);
}

TEST_CASE("ConfigManager set/get/has/remove", "[utils][config]") {
    ConfigManager config;
    config.set("graphics.width", 1920);
    config.set("graphics.title", "Explorer");
    CHECK(config.has("graphics.width"));
    CHECK(config.get<int>("graphics.width") == 1920);
    CHECK(config.get<std::string>("graphics.title") == "Explorer");
    CHECK(config.tryGet<int>("graphics.title") == std::nullopt);
    CHECK(config.getOr("missing.key", 5) == 5);
    CHECK(config.getOr("missing.key", "dflt") == "dflt");
    CHECK(config.getOr("graphics.width", 0) == 1920);
    CHECK_THROWS_AS(config.get<int>("missing.key"), ConfigError);
    CHECK_THROWS_AS(config.get<int>("graphics.title"), ConfigError);
    CHECK_THROWS_AS(config.set("  ", 1), ConfigError);
    CHECK(config.size() == 2);
    CHECK(config.remove("graphics.title"));
    CHECK_FALSE(config.remove("graphics.title"));
    CHECK(config.size() == 1);
    config.clear();
    CHECK(config.size() == 0);
}

TEST_CASE("ConfigManager keys, sections and section views", "[utils][config]") {
    ConfigManager config;
    config.set("b.y", 2);
    config.set("a.x", 1);
    config.set("b.z", 3);
    config.set("root", true);
    config.set("bb.q", 4);
    CHECK(config.keys() == Strings{"a.x", "b.y", "b.z", "bb.q", "root"});
    CHECK(config.keys("b.") == Strings{"b.y", "b.z"});
    CHECK(config.sections() == Strings{"a", "b", "bb"});
    const auto b = config.section("b");
    REQUIRE(b.size() == 2);
    CHECK(b.at("y") == ConfigValue{2});
    CHECK(config.snapshot().size() == 5);
}

TEST_CASE("ConfigManager loads INI with comments, quotes and inline comments", "[utils][config][ini]") {
    ConfigManager config;
    config.loadIni(R"(
top = 1
; full-line comment
# another
[graphics]
width = 1280      ; inline comment
title = "Hello; World"
ratio=1.5
[ fleet ]
ships = [Vega, Rigel]
url = http://x#y
)");
    CHECK(config.get<int>("top") == 1);
    CHECK(config.get<int>("graphics.width") == 1280);
    CHECK(config.get<std::string>("graphics.title") == "Hello; World");
    CHECK(config.get<double>("graphics.ratio") == Approx(1.5));
    CHECK(config.get<ConfigValue::List>("fleet.ships") == ConfigValue::List{"Vega", "Rigel"});
    CHECK(config.get<std::string>("fleet.url") == "http://x#y");
}

TEST_CASE("ConfigManager INI errors carry line numbers", "[utils][config][ini]") {
    ConfigManager config;
    try {
        config.loadIni("[ok]\na = 1\nbroken line\n");
        FAIL("expected ConfigError");
    } catch (const ConfigError& e) {
        CHECK(std::string{e.what()}.find("line 3") != std::string::npos);
    }
    CHECK_THROWS_AS(config.loadIni("[unterminated\n"), ConfigError);
    CHECK_THROWS_AS(config.loadIni(" = value\n"), ConfigError);
}

TEST_CASE("ConfigManager INI serialisation round-trips", "[utils][config][ini]") {
    ConfigManager config;
    config.set("name", "plain");
    config.set("graphics.width", 800);
    config.set("graphics.title", "Has; semicolon");
    config.set("graphics.numberish", "42");
    config.set("graphics.flag", false);
    config.set("audio.gain", 0.25);
    config.set("fleet.ships", ConfigValue::List{"A", "B"});
    ConfigManager reloaded;
    reloaded.loadIni(config.toIni());
    CHECK(reloaded.snapshot() == config.snapshot());
}

TEST_CASE("ConfigManager imports flattened JSON", "[utils][config][json]") {
    ConfigManager config;
    config.loadJson(JsonParser::parse(
        R"({"server": {"host": "h", "port": 8080, "tls": {"enabled": true}}, "ratio": 0.5, "tags": ["a", 1], "n": null})"));
    CHECK(config.get<std::string>("server.host") == "h");
    CHECK(config.get<int>("server.port") == 8080);
    CHECK(config.value("server.port")->type() == ConfigValue::Type::Int);
    CHECK(config.get<bool>("server.tls.enabled"));
    CHECK(config.get<double>("ratio") == 0.5);
    CHECK(config.get<ConfigValue::List>("tags") == ConfigValue::List{"a", "1"});
    CHECK_FALSE(config.has("n"));
    CHECK_THROWS_AS(config.loadJson(JsonValue::array()), ConfigError);
}

TEST_CASE("ConfigManager file load/save in a temp directory", "[utils][config][file]") {
    const auto dir = std::filesystem::temp_directory_path() / "cppversehub_utils_config_test";
    std::filesystem::remove_all(dir);
    ConfigManager config;
    config.set("game.level", 3);
    config.set("game.name", "Odyssey");
    const auto ini = dir / "game.ini";
    config.saveIni(ini);
    ConfigManager loaded;
    loaded.loadFile(ini);
    CHECK(loaded.snapshot() == config.snapshot());

    const auto json = dir / "game.json";
    FileParserUtils::writeTextFile(json, R"({"game": {"level": 9}})");
    loaded.loadFile(json);
    CHECK(loaded.get<int>("game.level") == 9);
    CHECK_THROWS(loaded.loadFile(dir / "missing.ini"));
    std::filesystem::remove_all(dir);
}

TEST_CASE("ConfigManager validators reject bad values", "[utils][config][validation]") {
    ConfigManager config;
    config.set("audio.volume", 50);
    const auto inRange = [](const ConfigValue& v) {
        const auto i = v.as<int>();
        return i && *i >= 0 && *i <= 100;
    };
    config.addValidator("audio.volume", inRange, "0..100");
    CHECK_NOTHROW(config.set("audio.volume", 100));
    CHECK_THROWS_AS(config.set("audio.volume", 101), ConfigError);
    CHECK(config.get<int>("audio.volume") == 100);
    CHECK(config.validate().empty());
    config.set("x.y", "bad");
    CHECK_THROWS_AS(config.addValidator("x.y", inRange, "number"), ConfigError);
    CHECK_THROWS_AS(config.addValidator("x.z", ConfigManager::Validator{}, "null"), ConfigError);
    CHECK(config.missingKeys({"audio.volume", "net.port"}) == Strings{"net.port"});
}

TEST_CASE("ConfigManager notifies listeners outside the lock", "[utils][config][observer]") {
    ConfigManager config;
    std::vector<ConfigManager::Change> changes;
    const auto id = config.addListener([&](const ConfigManager::Change& c) {
        changes.push_back(c);
        // Re-entrant read from inside the callback must not deadlock.
        (void)config.has(c.key);
    });
    config.set("a", 1);
    config.set("a", 1); // unchanged: no notification
    config.set("a", 2);
    config.remove("a");
    REQUIRE(changes.size() == 3);
    CHECK_FALSE(changes[0].before.has_value());
    CHECK(changes[0].after == ConfigValue{1});
    CHECK(changes[1].before == ConfigValue{1});
    CHECK(changes[1].after == ConfigValue{2});
    CHECK_FALSE(changes[2].after.has_value());
    CHECK(config.removeListener(id));
    CHECK_FALSE(config.removeListener(id));
    config.set("b", 1);
    CHECK(changes.size() == 3);
}

TEST_CASE("ConfigManager environment overrides use an injectable lookup", "[utils][config][env]") {
    CHECK(ConfigManager::environmentName("app", "graphics.width") == "APP_GRAPHICS_WIDTH");
    CHECK(ConfigManager::environmentName("", "a-b.c") == "A_B_C");
    ConfigManager config;
    config.set("graphics.width", 800);
    config.set("graphics.title", "x");
    const auto lookup = [](const std::string& name) -> std::optional<std::string> {
        if (name == "APP_GRAPHICS_WIDTH") {
            return "1024";
        }
        return std::nullopt;
    };
    CHECK(config.applyEnvironmentOverrides("app", lookup) == 1);
    CHECK(config.get<int>("graphics.width") == 1024);
    CHECK(config.get<std::string>("graphics.title") == "x");
    CHECK_FALSE(ConfigManager::systemEnvironment("CPPVERSEHUB_SURELY_UNSET_VARIABLE_12345").has_value());
}

TEST_CASE("ConfigManager merge, copy and move semantics", "[utils][config]") {
    ConfigManager a;
    a.set("x", 1);
    a.set("y", 2);
    ConfigManager b;
    b.set("y", 20);
    b.set("z", 30);
    ConfigManager keep = a;
    CHECK(keep.merge(b, false) == 1);
    CHECK(keep.get<int>("y") == 2);
    CHECK(keep.get<int>("z") == 30);
    CHECK(a.merge(b) == 2);
    CHECK(a.get<int>("y") == 20);
    CHECK(a.merge(a) == 0);

    ConfigManager copy{a};
    CHECK(copy.snapshot() == a.snapshot());
    ConfigManager moved{std::move(copy)};
    CHECK(moved.size() == 3);
    ConfigManager assigned;
    assigned = moved;
    CHECK(assigned.snapshot() == moved.snapshot());
    ConfigManager moveAssigned;
    moveAssigned = std::move(assigned);
    CHECK(moveAssigned.get<int>("z") == 30);
}

TEST_CASE("ConfigManager supports concurrent readers and writers", "[utils][config][threads]") {
    ConfigManager config;
    config.set("counter", 0);
    std::atomic<int> reads{0};
    std::vector<std::thread> threads;
    for (int t = 0; t < 4; ++t) {
        threads.emplace_back([&config, t] {
            for (int i = 0; i < 200; ++i) {
                config.set("t" + std::to_string(t) + ".k" + std::to_string(i), i);
            }
        });
        threads.emplace_back([&config, &reads] {
            for (int i = 0; i < 200; ++i) {
                if (config.tryGet<int>("counter")) {
                    ++reads;
                }
                (void)config.keys("t");
            }
        });
    }
    for (auto& th : threads) {
        th.join();
    }
    CHECK(config.size() == 801);
    CHECK(reads == 800);
}

TEST_CASE("ConfigBuilder layers defaults, INI and overrides", "[utils][config][builder]") {
    const auto config = ConfigBuilder{}
                            .withDefault("graphics.width", 800)
                            .withDefault("graphics.height", 600)
                            .withIni("[graphics]\nwidth = 1024\n")
                            .withOverride("graphics.height", 768)
                            .require("graphics.width")
                            .build();
    CHECK(config.get<int>("graphics.width") == 1024);
    CHECK(config.get<int>("graphics.height") == 768);
    CHECK_THROWS_AS(ConfigBuilder{}.require("must.exist").build(), ConfigError);
    CHECK_THROWS_AS(ConfigBuilder{}
                        .withValidator(
                            "v", [](const ConfigValue& v) { return v.is<bool>(); }, "bool")
                        .withDefault("v", 3)
                        .build(),
                    ConfigError);
}
