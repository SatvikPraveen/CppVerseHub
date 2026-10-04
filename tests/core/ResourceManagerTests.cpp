#include "core/Exceptions.hpp"
#include "core/Random.hpp"
#include "core/ResourceManager.hpp"

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <nlohmann/json.hpp>

#include <thread>
#include <vector>

using namespace CppVerseHub::Core;

namespace {

ResourceAmount grandTotal(const ResourceManager& rm) {
    ResourceAmount t = 0;
    for (ResourceType r : kAllResourceTypes) {
        t += rm.total(r);
    }
    return t;
}

} // namespace

TEST_CASE("ResourceManager accounts open, close and reject misuse", "[core][resources]") {
    ResourceManager rm;
    rm.openAccount(EntityId{1});
    REQUIRE(rm.hasAccount(EntityId{1}));
    REQUIRE(rm.accountCount() == 1);
    REQUIRE_THROWS_AS(rm.openAccount(EntityId{1}), InvalidStateException);
    REQUIRE_THROWS_AS(rm.openAccount(EntityId{}), InvalidArgumentException);
    REQUIRE_THROWS_AS(rm.balance(EntityId{9}, ResourceType::Food), EntityNotFoundException);
    REQUIRE_THROWS_AS(rm.deposit(EntityId{1}, ResourceType::Food, -1), InvalidArgumentException);
    REQUIRE(rm.closeAccount(EntityId{1}));
    REQUIRE_FALSE(rm.closeAccount(EntityId{1}));
    REQUIRE(rm.accounts().empty());
}

TEST_CASE("ResourceManager deposit and withdraw track minted and burned totals", "[core][resources]") {
    ResourceManager rm;
    rm.openAccount(EntityId{1});
    rm.deposit(EntityId{1}, ResourceType::Energy, 500);
    rm.withdraw(EntityId{1}, ResourceType::Energy, 120);
    REQUIRE(rm.balance(EntityId{1}, ResourceType::Energy) == 380);
    REQUIRE(rm.totalMinted(ResourceType::Energy) == 500);
    REQUIRE(rm.totalBurned(ResourceType::Energy) == 120);
    REQUIRE(rm.checkConservation());
    REQUIRE_FALSE(rm.tryWithdraw(EntityId{1}, ResourceType::Energy, 381));
    REQUIRE(rm.tryWithdraw(EntityId{1}, ResourceType::Energy, 380));
    REQUIRE(rm.balance(EntityId{1}, ResourceType::Energy) == 0);
}

TEST_CASE("ResourceManager transfers conserve totals exactly", "[core][resources][invariant]") {
    ResourceManager rm;
    constexpr std::uint64_t kAccounts = 8;
    for (std::uint64_t i = 1; i <= kAccounts; ++i) {
        rm.openAccount(EntityId{i});
        for (ResourceType t : kAllResourceTypes) {
            rm.deposit(EntityId{i}, t, static_cast<ResourceAmount>(100 * i));
        }
    }
    const ResourceAmount before = grandTotal(rm);
    DeterministicRng rng(GENERATE(1U, 2U, 3U));
    std::size_t rejected = 0;
    for (int i = 0; i < 5000; ++i) {
        const EntityId from{1 + rng.below(kAccounts)};
        const EntityId to{1 + rng.below(kAccounts)};
        const auto type = kAllResourceTypes[rng.below(kResourceTypeCount)];
        const auto amount = rng.uniformInt(0, 150);
        try {
            rm.transfer(from, to, type, amount);
        } catch (const InsufficientResourcesException&) {
            ++rejected;
        }
    }
    REQUIRE(grandTotal(rm) == before);
    REQUIRE(rm.checkConservation());
    REQUIRE(rejected > 0); // the failure path was exercised and left no trace
}

TEST_CASE("ResourceManager failed transfer has the strong guarantee", "[core][resources]") {
    ResourceManager rm;
    rm.openAccount(EntityId{1});
    rm.openAccount(EntityId{2});
    rm.deposit(EntityId{1}, ResourceType::Water, 10);
    try {
        rm.transfer(EntityId{1}, EntityId{2}, ResourceType::Water, 11);
        FAIL("expected InsufficientResourcesException");
    } catch (const InsufficientResourcesException& e) {
        REQUIRE(e.requested() == 11);
        REQUIRE(e.available() == 10);
    }
    REQUIRE_THROWS_AS(rm.transfer(EntityId{1}, EntityId{3}, ResourceType::Water, 1), EntityNotFoundException);
    REQUIRE(rm.balance(EntityId{1}, ResourceType::Water) == 10);
    REQUIRE(rm.balance(EntityId{2}, ResourceType::Water) == 0);
    const ResourceAmounts moved = rm.transferAll(EntityId{1}, EntityId{2});
    REQUIRE(moved[ResourceType::Water] == 10);
    REQUIRE(rm.balance(EntityId{2}, ResourceType::Water) == 10);
    REQUIRE(rm.checkConservation());
}

TEST_CASE("ResourceManager integrates fractional production exactly over many ticks", "[core][resources]") {
    ResourceManager rm;
    rm.openAccount(EntityId{1});
    rm.setProductionRate(EntityId{1}, ResourceType::Minerals, 2.5);
    REQUIRE(rm.producerCount() == 1);
    ResourceAmount produced = 0;
    for (int i = 0; i < 100; ++i) { // 10 seconds at dt = 0.1
        produced += rm.tick(0.1).produced[ResourceType::Minerals];
    }
    // 25 units expected; floor-with-carry may lag by at most one unit due to rounding of 0.1.
    REQUIRE(produced >= 24);
    REQUIRE(produced <= 25);
    REQUIRE(rm.balance(EntityId{1}, ResourceType::Minerals) == produced);
    REQUIRE(rm.totalMinted(ResourceType::Minerals) == produced);
    REQUIRE_THROWS_AS(rm.setProductionRate(EntityId{1}, ResourceType::Minerals, -1.0),
                      InvalidArgumentException);
    REQUIRE_THROWS_AS(rm.tick(-0.1), InvalidArgumentException);
}

TEST_CASE("ResourceManager consumers are limited by stock and report shortfalls", "[core][resources]") {
    ResourceManager rm;
    rm.openAccount(EntityId{1});
    rm.openAccount(EntityId{2});
    rm.deposit(EntityId{1}, ResourceType::Food, 5);
    rm.setConsumptionRate(EntityId{1}, ResourceType::Food, 3.0);
    rm.setConsumptionRate(EntityId{2}, ResourceType::Energy, 1.0);
    REQUIRE(rm.consumerCount() == 2);
    auto r1 = rm.tick(1.0);
    REQUIRE(r1.consumed[ResourceType::Food] == 3);
    REQUIRE(r1.shortfalls.size() == 1);
    REQUIRE(r1.shortfalls[0].account == EntityId{2});
    auto r2 = rm.tick(1.0);
    REQUIRE(r2.consumed[ResourceType::Food] == 2);
    REQUIRE(r2.shortfalls.size() == 2);
    REQUIRE(r2.shortfalls[0].account == EntityId{1}); // ordered by account id
    REQUIRE(r2.shortfalls[0].shortfall == 1);
    REQUIRE(rm.balance(EntityId{1}, ResourceType::Food) == 0);
    REQUIRE(rm.checkConservation());
}

TEST_CASE("ResourceManager conservation holds across production, consumption and closure",
          "[core][resources][invariant]") {
    ResourceManager rm;
    DeterministicRng rng(42);
    for (std::uint64_t i = 1; i <= 6; ++i) {
        rm.openAccount(EntityId{i});
        rm.deposit(EntityId{i}, ResourceType::Energy, 200);
        rm.setProductionRate(EntityId{i}, ResourceType::Energy, rng.uniform(0.0, 5.0));
        rm.setConsumptionRate(EntityId{i}, ResourceType::Energy, rng.uniform(0.0, 8.0));
    }
    for (int step = 0; step < 500; ++step) {
        static_cast<void>(rm.tick(0.05));
        rm.transfer(EntityId{1 + rng.below(6)}, EntityId{1 + rng.below(6)}, ResourceType::Energy, 0);
        REQUIRE(rm.total(ResourceType::Energy) ==
                rm.totalMinted(ResourceType::Energy) - rm.totalBurned(ResourceType::Energy));
    }
    static_cast<void>(rm.closeAccount(EntityId{3}));
    REQUIRE(rm.checkConservation());
}

TEST_CASE("ResourceManager concurrent transfers conserve totals", "[core][resources][threads]") {
    ResourceManager rm;
    constexpr std::uint64_t kAccounts = 4;
    for (std::uint64_t i = 1; i <= kAccounts; ++i) {
        rm.openAccount(EntityId{i});
        rm.deposit(EntityId{i}, ResourceType::Technology, 10'000);
    }
    std::vector<std::thread> workers;
    for (unsigned w = 0; w < 4; ++w) {
        workers.emplace_back([&rm, w] {
            DeterministicRng rng(w + 1U);
            for (int i = 0; i < 5000; ++i) {
                const EntityId from{1 + rng.below(kAccounts)};
                const EntityId to{1 + rng.below(kAccounts)};
                static_cast<void>(rm.tryWithdraw(from, ResourceType::Technology, 0));
                try {
                    rm.transfer(from, to, ResourceType::Technology, rng.uniformInt(1, 50));
                } catch (const InsufficientResourcesException&) {}
            }
        });
    }
    for (auto& t : workers) {
        t.join();
    }
    REQUIRE(rm.total(ResourceType::Technology) == static_cast<ResourceAmount>(kAccounts * 10'000));
    REQUIRE(rm.checkConservation());
}

TEST_CASE("ResourceManager JSON round-trip preserves balances, rates and carries",
          "[core][resources][json]") {
    ResourceManager rm;
    rm.openAccount(EntityId{5});
    rm.openAccount(EntityId{9});
    rm.deposit(EntityId{5}, ResourceType::Minerals, 77);
    rm.setProductionRate(EntityId{9}, ResourceType::Food, 0.3);
    static_cast<void>(rm.tick(1.0)); // leaves a 0.3 carry
    nlohmann::json j;
    rm.toJson(j);
    ResourceManager copy;
    copy.fromJson(j);
    nlohmann::json j2;
    copy.toJson(j2);
    REQUIRE(j == j2);
    for (int i = 0; i < 10; ++i) {
        REQUIRE(rm.tick(0.7).produced == copy.tick(0.7).produced);
    }
    REQUIRE(copy.balances(EntityId{9}) == rm.balances(EntityId{9}));
    REQUIRE_THROWS_AS(copy.fromJson(nlohmann::json{{"accounts", {{{"id", 1}, {"balance", {{"gold", 1}}}}}}}),
                      SerializationException);
    rm.clear();
    REQUIRE(rm.accountCount() == 0);
    REQUIRE(rm.totalMinted(ResourceType::Minerals) == 0);
}
