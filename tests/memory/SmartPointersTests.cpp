/**
 * @file SmartPointersTests.cpp
 * @brief Tests for the smart-pointer showcase classes.
 */

#include "memory/SmartPointers.hpp"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <sstream>
#include <string>
#include <thread>
#include <vector>

using namespace CppVerseHub::Memory;

TEST_CASE("Resource live counter and unique ids", "[memory][smart][resource]") {
    const int before = Resource::live_count();
    {
        SpaceStation a("A", 10);
        Spacecraft b("B", 100.0);
        CHECK(Resource::live_count() == before + 2);
        CHECK(a.id() != b.id());
        CHECK(a.kind() == "station");
        CHECK(b.kind() == "spacecraft");
        CHECK(a.name() == "A");
    }
    CHECK(Resource::live_count() == before);
}

TEST_CASE("SpaceStation population is clamped", "[memory][smart][resource]") {
    SpaceStation s("Gateway", 5);
    CHECK(s.add_population(3) == 3);
    CHECK(s.add_population(10) == 2);
    CHECK(s.add_population(-4) == 0);
    CHECK(s.population() == 5);
    s.process();
    CHECK(s.population() == s.capacity());
}

TEST_CASE("Spacecraft fuel bookkeeping", "[memory][smart][resource]") {
    Spacecraft c("Odyssey", 200.0);
    CHECK(c.fuel() == Catch::Approx(200.0));
    c.consume_fuel(50.0);
    CHECK(c.fuel() == Catch::Approx(150.0));
    c.process();
    CHECK(c.fuel() == Catch::Approx(130.0));
    c.refuel(1000.0);
    CHECK(c.fuel() == Catch::Approx(c.fuel_capacity()));
    c.consume_fuel(-5.0);
    c.consume_fuel(1e9);
    CHECK(c.fuel() == Catch::Approx(0.0));
}

TEST_CASE("unique_ptr factory and polymorphic deletion", "[memory][smart][unique]") {
    const int before = Resource::live_count();
    {
        std::vector<std::unique_ptr<Resource>> fleet;
        fleet.push_back(ResourceFactory::create("station", "S"));
        fleet.push_back(ResourceFactory::create("spacecraft", "C"));
        CHECK(ResourceFactory::create("asteroid", "X") == nullptr);
        CHECK(fleet[0]->kind() == "station");
        CHECK(fleet[1]->kind() == "spacecraft");
        CHECK(Resource::live_count() == before + 2);
        auto moved = std::move(fleet[0]);
        CHECK(fleet[0] == nullptr);
        CHECK(moved->name() == "S");
    }
    CHECK(Resource::live_count() == before);
}

TEST_CASE("unique_ptr with counting custom deleter", "[memory][smart][unique]") {
    int deletions = 0;
    {
        auto r = ResourceFactory::create_counted("station", "Counted", deletions);
        REQUIRE(r);
        CHECK(deletions == 0);
        auto shared = SmartPtrUtils::to_shared(std::move(r));
        CHECK(r == nullptr);
        auto copy = shared;
        CHECK(shared.use_count() == 2);
    }
    CHECK(deletions == 1);
    {
        auto unknown = ResourceFactory::create_counted("unknown", "none", deletions);
        CHECK_FALSE(unknown);
    }
    CHECK(deletions == 1); // deleter not invoked for null
}

TEST_CASE("dynamic_unique_cast transfers ownership only on success", "[memory][smart][unique]") {
    std::unique_ptr<Resource> station = std::make_unique<SpaceStation>("Hub", 3);
    std::unique_ptr<Resource> craft = std::make_unique<Spacecraft>("Ship", 10.0);

    auto as_station = SmartPtrUtils::dynamic_unique_cast<SpaceStation>(station);
    REQUIRE(as_station);
    CHECK(station == nullptr);
    CHECK(as_station->capacity() == 3);

    auto wrong = SmartPtrUtils::dynamic_unique_cast<SpaceStation>(craft);
    CHECK(wrong == nullptr);
    REQUIRE(craft != nullptr);
    CHECK(craft->name() == "Ship");

    std::unique_ptr<Resource> empty;
    CHECK(SmartPtrUtils::dynamic_unique_cast<Spacecraft>(empty) == nullptr);
}

TEST_CASE("shared_ptr aliasing constructor keeps the owner alive", "[memory][smart][shared]") {
    struct Pair {
        int first = 1;
        std::string second = "two";
    };
    auto owner = std::make_shared<Pair>();
    std::weak_ptr<Pair> watch = owner;
    auto member = SmartPtrUtils::member_alias(owner, &Pair::second);
    CHECK(owner.use_count() == 2);
    owner.reset();
    CHECK_FALSE(watch.expired());
    CHECK(*member == "two");
    member.reset();
    CHECK(watch.expired());
}

TEST_CASE("shared_ptr reference count is thread safe", "[memory][smart][shared][threads]") {
    auto shared = std::make_shared<Spacecraft>("Shared", 1.0);
    std::vector<std::thread> threads;
    for (int t = 0; t < 4; ++t) {
        threads.emplace_back([shared] {
            std::vector<std::shared_ptr<Spacecraft>> copies;
            for (int i = 0; i < 500; ++i) {
                copies.push_back(shared);
            }
        });
    }
    for (auto& th : threads) {
        th.join();
    }
    CHECK(shared.use_count() == 1);
}

TEST_CASE("Spacecraft notifies live observers and prunes dead ones", "[memory][smart][weak]") {
    Spacecraft craft("Voyager", 100.0, 0.5);
    auto control = MissionControl::create();
    control->watch(craft);
    {
        auto transient = MissionControl::create();
        transient->watch(craft);
        CHECK(craft.live_observer_count() == 2);
    }
    CHECK(craft.live_observer_count() == 1);
    craft.consume_fuel(40.0); // 60 left, above threshold 50
    CHECK(control->alerts().empty());
    craft.consume_fuel(20.0); // 40 left
    REQUIRE(control->alerts().size() == 1);
    CHECK(control->alerts().front() == "Voyager:40");
    CHECK(control.use_count() == 1); // observing did not take ownership
}

TEST_CASE("MissionControl registers itself through enable_shared_from_this", "[memory][smart][weak]") {
    auto control = MissionControl::create();
    CHECK(control->weak_from_this().lock() == control);
    Spacecraft a("A", 10.0, 0.9);
    Spacecraft b("B", 10.0, 0.9);
    control->watch(a);
    control->watch(b);
    a.consume_fuel(5.0);
    b.consume_fuel(5.0);
    CHECK(control->alerts() == std::vector<std::string>{"A:5", "B:5"});
    control.reset();
    a.consume_fuel(1.0); // observer gone: no crash, pruned
    CHECK(a.live_observer_count() == 0);
}

TEST_CASE("ResourceCache does not extend lifetimes", "[memory][smart][weak][cache]") {
    ResourceCache cache;
    int created = 0;
    auto factory = [&created] {
        ++created;
        return std::shared_ptr<Resource>(std::make_shared<SpaceStation>("Cached", 1));
    };
    auto a = cache.get_or_create("k", factory);
    auto b = cache.get_or_create("k", factory);
    CHECK(a == b);
    CHECK(created == 1);
    CHECK(cache.hits() == 1);
    CHECK(cache.misses() == 1);
    CHECK(cache.find("k") == a);
    CHECK(cache.find("missing") == nullptr);
    a.reset();
    b.reset();
    CHECK(cache.find("k") == nullptr);
    CHECK(cache.size() == 1);
    auto c = cache.get_or_create("k", factory); // expired -> recreated
    CHECK(created == 2);
    CHECK(cache.purge_expired() == 0);
    c.reset();
    CHECK(cache.purge_expired() == 1);
    CHECK(cache.size() == 0);
}

TEST_CASE("TreeNode parent links do not create reference cycles", "[memory][smart][weak][tree]") {
    const int before = TreeNode::live_count();
    std::weak_ptr<TreeNode> leaf_watch;
    {
        auto root = TreeNode::create("root");
        auto mid = root->add_child("mid");
        auto leaf = mid->add_child("leaf");
        static_cast<void>(root->add_child("sibling"));
        leaf_watch = leaf;
        CHECK(leaf->path() == "root/mid/leaf");
        CHECK(leaf->parent() == mid);
        CHECK(root->parent() == nullptr);
        CHECK(root->children().size() == 2);
        CHECK(TreeNode::live_count() == before + 4);
        CHECK(root.use_count() == 1); // children hold only weak references upward
    }
    CHECK(leaf_watch.expired());
    CHECK(TreeNode::live_count() == before);
}

TEST_CASE("TreeNode survives losing its parent", "[memory][smart][weak][tree]") {
    std::shared_ptr<TreeNode> orphan;
    {
        auto root = TreeNode::create("root");
        orphan = root->add_child("orphan");
    }
    CHECK(orphan->parent() == nullptr);
    CHECK(orphan->path() == "orphan");
}

TEST_CASE("PimplExample deep copies and moves", "[memory][smart][pimpl]") {
    PimplExample a;
    a.set_value(1);
    a.set_value(2);
    PimplExample b = a;
    b.set_value(3);
    CHECK(a.value() == 2);
    CHECK(a.history_size() == 2);
    CHECK(b.value() == 3);
    CHECK(b.history_size() == 3);

    PimplExample c = std::move(b);
    CHECK_FALSE(b.valid()); // NOLINT(bugprone-use-after-move)
    CHECK(b.value() == 0);
    CHECK(c.value() == 3);
    b.set_value(10); // moved-from object is reusable
    CHECK(b.value() == 10);

    a = c;
    CHECK(a.history_size() == 3);
    const PimplExample& alias = a;
    a = alias; // self-assignment safe
    CHECK(a.value() == 3);
    PimplExample d;
    d = std::move(c);
    CHECK(d.value() == 3);
}

TEST_CASE("demonstrateSmartPointers leaks nothing", "[memory][smart][demo]") {
    const int resources = Resource::live_count();
    const int nodes = TreeNode::live_count();
    std::ostringstream out;
    demonstrateSmartPointers(out);
    const auto text = out.str();
    CHECK(text.find("Resources leaked by this demo: 0") != std::string::npos);
    CHECK(text.find("TreeNode path: sol/earth/moon") != std::string::npos);
    CHECK(text.find("1000 copies, use_count = 2") != std::string::npos); // craft + as_base
    CHECK(Resource::live_count() == resources);
    CHECK(TreeNode::live_count() == nodes);
}
