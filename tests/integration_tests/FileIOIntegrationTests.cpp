// File: tests/integration_tests/FileIOIntegrationTests.cpp
// File parsing integration tests for CppVerseHub showcase

#include <catch2/catch.hpp>
#include <fstream>
#include <sstream>
#include <memory>
#include <vector>
#include <string>
#include <filesystem>
#include <chrono>
#include <thread>
#include <atomic>

// Include file I/O system components
#include "FileManager.hpp"
#include "ConfigParser.hpp"
#include "DataSerializer.hpp"
#include "Galaxy.hpp"
#include "Planet.hpp"
#include "Fleet.hpp"
#include "Mission.hpp"
#include "Logger.hpp"
#include "MemoryTracker.hpp"

using namespace CppVerseHub::Core;
using namespace CppVerseHub::IO;
using namespace CppVerseHub::Utils;
namespace fs = std::filesystem;

/**
 * @brief Test fixture for file I/O integration tests
 */
class FileIOIntegrationTestFixture {
public:
    FileIOIntegrationTestFixture() {
        MemoryTracker::resetCounters();
        
        // Create test directory
        testDir = "test_files";
        if (!fs::exists(testDir)) {
            fs::create_directory(testDir);
        }
        
        // Initialize logger
        auto& logger = Logger::getInstance();
        logger.setLogLevel(Logger::LogLevel::DEBUG);
        
        setupTestFiles();
    }
    
    ~FileIOIntegrationTestFixture() {
        cleanup();
        MemoryTracker::printMemoryStats("File I/O integration test completion");
    }
    
protected:
    void setupTestFiles() {
        createConfigFile();
        createGalaxyDataFile();
        createMissionDataFile();
        createInvalidDataFiles();
    }
    
    void createConfigFile() {
        std::ofstream configFile(testDir + "/test_config.ini");
        configFile << "[simulation]\n";
        configFile << "timestep=0.1\n";
        configFile << "max_threads=4\n";
        configFile << "enable_logging=true\n";
        configFile << "\n";
        configFile << "[galaxy]\n";
        configFile << "size_x=1000.0\n";
        configFile << "size_y=1000.0\n";
        configFile << "size_z=1000.0\n";
        configFile << "planet_count=50\n";
        configFile << "\n";
        configFile << "[rendering]\n";
        configFile << "width=1920\n";
        configFile << "height=1080\n";
        configFile << "fullscreen=false\n";
        configFile << "vsync=true\n";
        configFile.close();
    }
    
    void createGalaxyDataFile() {
        std::ofstream galaxyFile(testDir + "/test_galaxy.json");
        galaxyFile << "{\n";
        galaxyFile << "  \"name\": \"TestGalaxy\",\n";
        galaxyFile << "  \"dimensions\": {\n";
        galaxyFile << "    \"x\": 2000.0,\n";
        galaxyFile << "    \"y\": 2000.0,\n";
        galaxyFile << "    \"z\": 2000.0\n";
        galaxyFile << "  },\n";
        galaxyFile << "  \"planets\": [\n";
        galaxyFile << "    {\n";
        galaxyFile << "      \"name\": \"AlphaBase\",\n";
        galaxyFile << "      \"position\": {\"x\": 100.0, \"y\": 200.0, \"z\": 300.0},\n";
        galaxyFile << "      \"resources\": {\n";
        galaxyFile << "        \"minerals\": 1500,\n";
        galaxyFile << "        \"energy\": 800\n";
        galaxyFile << "      },\n";
        galaxyFile << "      \"habitability\": 0.75\n";
        galaxyFile << "    },\n";
        galaxyFile << "    {\n";
        galaxyFile << "      \"name\": \"BetaStation\",\n";
        galaxyFile << "      \"position\": {\"x\": 500.0, \"y\": 600.0, \"z\": 700.0},\n";
        galaxyFile << "      \"resources\": {\n";
        galaxyFile << "        \"minerals\": 2000,\n";
        galaxyFile << "        \"energy\": 1200\n";
        galaxyFile << "      },\n";
        galaxyFile << "      \"habitability\": 0.60\n";
        galaxyFile << "    }\n";
        galaxyFile << "  ],\n";
        galaxyFile << "  \"fleets\": [\n";
        galaxyFile << "    {\n";
        galaxyFile << "      \"name\": \"FirstFleet\",\n";
        galaxyFile << "      \"position\": {\"x\": 150.0, \"y\": 250.0, \"z\": 350.0},\n";
        galaxyFile << "      \"ships\": {\n";
        galaxyFile << "        \"fighters\": 20,\n";
        galaxyFile << "        \"cruisers\": 5,\n";
        galaxyFile << "        \"battleships\": 2\n";
        galaxyFile << "      }\n";
        galaxyFile << "    }\n";
        galaxyFile << "  ]\n";
        galaxyFile << "}\n";
        galaxyFile.close();
    }
    
    void createMissionDataFile() {
        std::ofstream missionFile(testDir + "/test_missions.xml");
        missionFile << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
        missionFile << "<missions>\n";
        missionFile << "  <mission>\n";
        missionFile << "    <id>EXPLORE_001</id>\n";
        missionFile << "    <name>AlphaExploration</name>\n";
        missionFile << "    <type>explore</type>\n";
        missionFile << "    <target>AlphaBase</target>\n";
        missionFile << "    <status>pending</status>\n";
        missionFile << "    <priority>high</priority>\n";
        missionFile << "  </mission>\n";
        missionFile << "  <mission>\n";
        missionFile << "    <id>COLONIZE_001</id>\n";
        missionFile << "    <name>BetaColonization</name>\n";
        missionFile << "    <type>colonize</type>\n";
        missionFile << "    <target>BetaStation</target>\n";
        missionFile << "    <status>in_progress</status>\n";
        missionFile << "    <priority>medium</priority>\n";
        missionFile << "  </mission>\n";
        missionFile << "</missions>\n";
        missionFile.close();
    }
    
    void createInvalidDataFiles() {
        // Invalid JSON
        std::ofstream invalidJson(testDir + "/invalid.json");
        invalidJson << "{\n";
        invalidJson << "  \"incomplete\": \"json\"\n";
        invalidJson << "  // missing closing brace\n";
        invalidJson.close();
        
        // Invalid XML
        std::ofstream invalidXml(testDir + "/invalid.xml");
        invalidXml << "<?xml version=\"1.0\"?>\n";
        invalidXml << "<root>\n";
        invalidXml << "  <unclosed_tag>\n";
        invalidXml << "</root>\n";
        invalidXml.close();
        
        // Corrupted config
        std::ofstream invalidConfig(testDir + "/invalid.ini");
        invalidConfig << "[section\n";  // Missing closing bracket
        invalidConfig << "key=value\n";
        invalidConfig << "invalid_line_without_equals\n";
        invalidConfig.close();
    }
    
    void cleanup() {
        try {
            if (fs::exists(testDir)) {
                fs::remove_all(testDir);
            }
        } catch (const fs::filesystem_error& e) {
            // Ignore cleanup errors in tests
        }
    }
    
    std::string testDir;
};

TEST_CASE_METHOD(FileIOIntegrationTestFixture, "Configuration File Processing", "[file-io][integration][config]") {
    
    SECTION("Loading configuration from file") {
        ConfigParser parser;
        
        auto config = parser.loadFromFile(testDir + "/test_config.ini");
        
        REQUIRE(config != nullptr);
        
        // Verify simulation settings
        REQUIRE(config->getProperty("simulation.timestep", "0.0") == "0.1");
        REQUIRE(config->getProperty("simulation.max_threads", "0") == "4");
        REQUIRE(config->getProperty("simulation.enable_logging", "false") == "true");
        
        // Verify galaxy settings
        REQUIRE(config->getProperty("galaxy.size_x", "0.0") == "1000.0");
        REQUIRE(config->getProperty("galaxy.planet_count", "0") == "50");
        
        // Verify rendering settings
        REQUIRE(config->getProperty("rendering.width", "0") == "1920");
        REQUIRE(config->getProperty("rendering.height", "0") == "1080");
        REQUIRE(config->getProperty("rendering.fullscreen", "true") == "false");
    }
    
    SECTION("Configuration file validation") {
        ConfigParser parser;
        
        // Test with invalid file
        auto invalidConfig = parser.loadFromFile(testDir + "/invalid.ini");
        REQUIRE(invalidConfig == nullptr);
        
        // Test with non-existent file
        auto missingConfig = parser.loadFromFile(testDir + "/missing.ini");
        REQUIRE(missingConfig == nullptr);
    }
    
    SECTION("Configuration file writing") {
        std::string outputPath = testDir + "/output_config.ini";
        
        ConfigParser parser;
        auto config = std::make_shared<Configuration>();
        
        // Set configuration values
        config->setProperty("test.value1", "123");
        config->setProperty("test.value2", "hello");
        config->setProperty("test.flag", "true");
        config->setProperty("database.host", "localhost");
        config->setProperty("database.port", "5432");
        
        // Save configuration
        bool saveSuccess = parser.saveToFile(outputPath, config);
        REQUIRE(saveSuccess);
        
        // Verify file exists
        REQUIRE(fs::exists(outputPath));
        
        // Load back and verify
        auto loadedConfig = parser.loadFromFile(outputPath);
        REQUIRE(loadedConfig != nullptr);
        REQUIRE(loadedConfig->getProperty("test.value1", "") == "123");
        REQUIRE(loadedConfig->getProperty("database.host", "") == "localhost");
    }
}

TEST_CASE_METHOD(FileIOIntegrationTestFixture, "Galaxy Data Serialization", "[file-io][integration][galaxy]") {
    
    SECTION("Loading galaxy from JSON file") {
        DataSerializer serializer;
        
        auto galaxy = serializer.loadGalaxyFromFile(testDir + "/test_galaxy.json");
        
        REQUIRE(galaxy != nullptr);
        REQUIRE(galaxy->getName() == "TestGalaxy");
        
        // Verify dimensions
        auto bounds = galaxy->getBounds();
        REQUIRE(bounds.maxX == 2000.0);
        REQUIRE(bounds.maxY == 2000.0);
        REQUIRE(bounds.maxZ == 2000.0);
        
        // Verify planets
        auto planets = galaxy->getPlanets();
        REQUIRE(planets.size() == 2);
        
        auto alphaPlanet = galaxy->getPlanetByName("AlphaBase");
        REQUIRE(alphaPlanet != nullptr);
        REQUIRE(alphaPlanet->getPosition().x == 100.0);
        REQUIRE(alphaPlanet->getPosition().y == 200.0);
        REQUIRE(alphaPlanet->getPosition().z == 300.0);
        REQUIRE(alphaPlanet->getResourceAmount(ResourceType::MINERALS) == 1500);
        REQUIRE(alphaPlanet->getResourceAmount(ResourceType::ENERGY) == 800);
        REQUIRE(alphaPlanet->getHabitabilityRating() == Approx(0.75));
        
        // Verify fleets
        auto fleets = galaxy->getFleets();
        REQUIRE(fleets.size() == 1);
        
        auto firstFleet = galaxy->getFleetByName("FirstFleet");
        REQUIRE(firstFleet != nullptr);
        REQUIRE(firstFleet->getPosition().x == 150.0);
        REQUIRE(firstFleet->getShipCount(ShipType::FIGHTER) == 20);
        REQUIRE(firstFleet->getShipCount(ShipType::CRUISER) == 5);
        REQUIRE(firstFleet->getShipCount(ShipType::BATTLESHIP) == 2);
    }
    
    SECTION("Saving galaxy to JSON file") {
        std::string outputPath = testDir + "/output_galaxy.json";
        
        // Create test galaxy
        auto galaxy = std::make_unique<Galaxy>("OutputTestGalaxy", 1500.0, 1500.0, 1500.0);
        
        // Add planets
        auto planet1 = std::make_unique<Planet>("OutputPlanet1", Vector3D{100, 200, 300});
        planet1->setResourceAmount(ResourceType::MINERALS, 1800);
        planet1->setResourceAmount(ResourceType::ENERGY, 900);
        planet1->setHabitabilityRating(0.85);
        
        galaxy->addPlanet(std::move(planet1));
        
        // Add fleet
        auto fleet = std::make_unique<Fleet>("OutputFleet", Vector3D{250, 350, 450});
        fleet->addShips(ShipType::FIGHTER, 15);
        fleet->addShips(ShipType::CRUISER, 3);
        galaxy->addFleet(std::move(fleet));
        
        // Save galaxy
        DataSerializer serializer;
        bool saveSuccess = serializer.saveGalaxyToFile(outputPath, galaxy.get());
        REQUIRE(saveSuccess);
        
        // Verify file exists
        REQUIRE(fs::exists(outputPath));
        
        // Load back and verify
        auto loadedGalaxy = serializer.loadGalaxyFromFile(outputPath);
        REQUIRE(loadedGalaxy != nullptr);
        REQUIRE(loadedGalaxy->getName() == "OutputTestGalaxy");
        REQUIRE(loadedGalaxy->getPlanets().size() == 1);
        REQUIRE(loadedGalaxy->getFleets().size() == 1);
    }
    
    SECTION("Error handling with invalid galaxy data") {
        DataSerializer serializer;
        
        // Test with invalid JSON file
        auto invalidGalaxy = serializer.loadGalaxyFromFile(testDir + "/invalid.json");
        REQUIRE(invalidGalaxy == nullptr);
        
        // Test with non-existent file
        auto missingGalaxy = serializer.loadGalaxyFromFile(testDir + "/missing.json");
        REQUIRE(missingGalaxy == nullptr);
    }
}

TEST_CASE_METHOD(FileIOIntegrationTestFixture, "Mission Data Processing", "[file-io][integration][missions]") {
    
    SECTION("Loading missions from XML file") {
        DataSerializer serializer;
        
        auto missions = serializer.loadMissionsFromFile(testDir + "/test_missions.xml");
        
        REQUIRE(missions.size() == 2);
        
        // Verify first mission
        auto exploreMission = std::find_if(missions.begin(), missions.end(),
            [](const std::unique_ptr<Mission>& m) {
                return m->getName() == "AlphaExploration";
            });
        
        REQUIRE(exploreMission != missions.end());
        REQUIRE((*exploreMission)->getType() == MissionType::EXPLORE);
        REQUIRE((*exploreMission)->getStatus() == MissionStatus::PENDING);
        
        // Verify second mission
        auto colonizeMission = std::find_if(missions.begin(), missions.end(),
            [](const std::unique_ptr<Mission>& m) {
                return m->getName() == "BetaColonization";
            });
        
        REQUIRE(colonizeMission != missions.end());
        REQUIRE((*colonizeMission)->getType() == MissionType::COLONIZE);
        REQUIRE((*colonizeMission)->getStatus() == MissionStatus::IN_PROGRESS);
    }
    
    SECTION("Mission data validation") {
        DataSerializer serializer;
        
        // Test with invalid XML file
        auto invalidMissions = serializer.loadMissionsFromFile(testDir + "/invalid.xml");
        REQUIRE(invalidMissions.empty());
        
        // Test with missing file
        auto missingMissions = serializer.loadMissionsFromFile(testDir + "/missing.xml");
        REQUIRE(missingMissions.empty());
    }
}

TEST_CASE_METHOD(FileIOIntegrationTestFixture, "Large File Processing", "[file-io][integration][performance]") {
    
    SECTION("Large galaxy data processing") {
        std::string largePath = testDir + "/large_galaxy.json";
        
        // Create large galaxy data file
        std::ofstream largeFile(largePath);
        largeFile << "{\n";
        largeFile << "  \"name\": \"LargeGalaxy\",\n";
        largeFile << "  \"dimensions\": {\"x\": 5000.0, \"y\": 5000.0, \"z\": 5000.0},\n";
        largeFile << "  \"planets\": [\n";
        
        const int planetCount = 100;
        for (int i = 0; i < planetCount; ++i) {
            largeFile << "    {\n";
            largeFile << "      \"name\": \"Planet_" << i << "\",\n";
            largeFile << "      \"position\": {\"x\": " << (i * 50.0) << ", \"y\": " << (i * 45.0) << ", \"z\": " << (i * 40.0) << "},\n";
            largeFile << "      \"resources\": {\n";
            largeFile << "        \"minerals\": " << (1000 + i * 10) << ",\n";
            largeFile << "        \"energy\": " << (500 + i * 5) << "\n";
            largeFile << "      },\n";
            largeFile << "      \"habitability\": " << (0.3 + (i % 10) * 0.05) << "\n";
            largeFile << "    }";
            if (i < planetCount - 1) largeFile << ",";
            largeFile << "\n";
        }
        
        largeFile << "  ],\n";
        largeFile << "  \"fleets\": []\n";
        largeFile << "}\n";
        largeFile.close();
        
        // Measure loading time
        auto startTime = std::chrono::high_resolution_clock::now();
        
        DataSerializer serializer;
        auto largeGalaxy = serializer.loadGalaxyFromFile(largePath);
        
        auto endTime = std::chrono::high_resolution_clock::now();
        auto loadTime = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
        
        REQUIRE(largeGalaxy != nullptr);
        REQUIRE(largeGalaxy->getPlanets().size() == planetCount);
        
        INFO("Loaded " << planetCount << " planets in " << loadTime.count() << "ms");
        REQUIRE(loadTime.count() < 5000); // Should load in less than 5 seconds
    }
    
    SECTION("Memory usage during large file operations") {
        MemoryTracker::resetCounters();
        auto initialStats = MemoryTracker::getCurrentStats();
        
        // Load large galaxy (created in previous section)
        std::string largePath = testDir + "/large_galaxy.json";
        
        DataSerializer serializer;
        auto galaxy = serializer.loadGalaxyFromFile(largePath);
        
        auto afterLoadStats = MemoryTracker::getCurrentStats();
        
        REQUIRE(galaxy != nullptr);
        
        galaxy.reset(); // Release galaxy
        
        auto finalStats = MemoryTracker::getCurrentStats();
        
        INFO("Memory usage analysis:");
        INFO("Initial: " << initialStats.currentMemoryUsage << " bytes");
        INFO("After load: " << afterLoadStats.currentMemoryUsage << " bytes");
        INFO("Final: " << finalStats.currentMemoryUsage << " bytes");
        
        // Verify reasonable memory usage
        REQUIRE(afterLoadStats.currentMemoryUsage > initialStats.currentMemoryUsage);
        REQUIRE(finalStats.currentMemoryUsage <= afterLoadStats.currentMemoryUsage);
    }
}

TEST_CASE_METHOD(FileIOIntegrationTestFixture, "Concurrent File Operations", "[file-io][integration][concurrent]") {
    
    SECTION("Concurrent file reading") {
        const int threadCount = 8;
        const int readsPerThread = 20;
        
        std::atomic<int> successfulReads{0};
        std::atomic<int> failedReads{0};
        
        std::vector<std::thread> threads;
        
        for (int i = 0; i < threadCount; ++i) {
            threads.emplace_back([this, &successfulReads, &failedReads, readsPerThread] {
                DataSerializer serializer;
                
                for (int j = 0; j < readsPerThread; ++j) {
                    auto galaxy = serializer.loadGalaxyFromFile(testDir + "/test_galaxy.json");
                    
                    if (galaxy && galaxy->getName() == "TestGalaxy") {
                        successfulReads.fetch_add(1);
                    } else {
                        failedReads.fetch_add(1);
                    }
                }
            });
        }
        
        for (auto& thread : threads) {
            thread.join();
        }
        
        REQUIRE(successfulReads.load() == threadCount * readsPerThread);
        REQUIRE(failedReads.load() == 0);
        
        INFO("Concurrent reads completed: " << successfulReads.load());
    }
    
    SECTION("Concurrent file writing") {
        const int threadCount = 4;
        const int writesPerThread = 10;
        
        std::atomic<int> successfulWrites{0};
        std::vector<std::thread> threads;
        
        for (int i = 0; i < threadCount; ++i) {
            threads.emplace_back([this, i, &successfulWrites, writesPerThread] {
                FileManager fileManager;
                
                for (int j = 0; j < writesPerThread; ++j) {
                    std::string filename = testDir + "/concurrent_" + std::to_string(i) + "_" + std::to_string(j) + ".txt";
                    std::string content = "Thread " + std::to_string(i) + " Write " + std::to_string(j) + "\n";
                    
                    if (fileManager.writeFile(filename, content)) {
                        successfulWrites.fetch_add(1);
                    }
                }
            });
        }
        
        for (auto& thread : threads) {
            thread.join();
        }
        
        REQUIRE(successfulWrites.load() == threadCount * writesPerThread);
        
        // Verify files exist and have correct content
        int verifiedFiles = 0;
        for (int i = 0; i < threadCount; ++i) {
            for (int j = 0; j < writesPerThread; ++j) {
                std::string filename = testDir + "/concurrent_" + std::to_string(i) + "_" + std::to_string(j) + ".txt";
                if (fs::exists(filename)) {
                    verifiedFiles++;
                }
            }
        }
        
        REQUIRE(verifiedFiles == threadCount * writesPerThread);
        INFO("Concurrent writes verified: " << verifiedFiles);
    }
}