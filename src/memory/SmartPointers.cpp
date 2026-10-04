/**
 * @file SmartPointers.cpp
 * @brief Implementation of the smart-pointer domain classes and showcase.
 */

#include "memory/SmartPointers.hpp"

#include <algorithm>
#include <sstream>
#include <thread>

namespace CppVerseHub::Memory {

    // ------------------------------------------------------------------------ Resource

    Resource::Resource(std::string name) : name_(std::move(name)), id_(next_id_.fetch_add(1)) {
        live_.fetch_add(1, std::memory_order_relaxed);
    }

    Resource::~Resource() { live_.fetch_sub(1, std::memory_order_relaxed); }

    // -------------------------------------------------------------------- SpaceStation

    SpaceStation::SpaceStation(std::string name, int capacity)
        : Resource(std::move(name)), capacity_(std::max(0, capacity)) {}

    void SpaceStation::process() { static_cast<void>(add_population(1)); }

    int SpaceStation::add_population(int count) noexcept {
        if (count <= 0) {
            return 0;
        }
        const int admitted = std::min(count, capacity_ - population_);
        population_ += admitted;
        return admitted;
    }

    // ---------------------------------------------------------------------- Spacecraft

    Spacecraft::Spacecraft(std::string name, double fuel_capacity, double low_fuel_ratio)
        : Resource(std::move(name)),
          capacity_(std::max(0.0, fuel_capacity)),
          fuel_(capacity_),
          threshold_(capacity_ * std::clamp(low_fuel_ratio, 0.0, 1.0)) {}

    void Spacecraft::process() { consume_fuel(capacity_ * 0.1); }

    void Spacecraft::consume_fuel(double amount) {
        if (amount <= 0.0) {
            return;
        }
        fuel_ = std::max(0.0, fuel_ - amount);
        notify_if_low();
    }

    void Spacecraft::refuel(double amount) noexcept {
        if (amount > 0.0) {
            fuel_ = std::min(capacity_, fuel_ + amount);
        }
    }

    void Spacecraft::add_observer(std::weak_ptr<FuelObserver> observer) { observers_.push_back(std::move(observer)); }

    std::size_t Spacecraft::live_observer_count() {
        std::erase_if(observers_, [](const std::weak_ptr<FuelObserver>& w) { return w.expired(); });
        return observers_.size();
    }

    void Spacecraft::notify_if_low() {
        if (fuel_ >= threshold_) {
            return;
        }
        // Lock each observer for the duration of the call; drop the ones that died.
        std::erase_if(observers_, [this](const std::weak_ptr<FuelObserver>& w) {
            if (auto observer = w.lock()) {
                observer->on_low_fuel(name(), fuel_);
                return false;
            }
            return true;
        });
    }

    // ------------------------------------------------------------------ MissionControl

    void MissionControl::on_low_fuel(const std::string& craft, double fuel) {
        std::ostringstream entry;
        entry << craft << ':' << fuel;
        alerts_.push_back(entry.str());
    }

    // ----------------------------------------------------------------- ResourceFactory

    std::unique_ptr<Resource> ResourceFactory::create(std::string_view kind, std::string name) {
        if (kind == "station") {
            return std::make_unique<SpaceStation>(std::move(name), 100);
        }
        if (kind == "spacecraft") {
            return std::make_unique<Spacecraft>(std::move(name), 1000.0);
        }
        return nullptr;
    }

    std::unique_ptr<Resource, ResourceFactory::CountingDeleter>
    ResourceFactory::create_counted(std::string_view kind, std::string name, int& deletions) {
        CountingDeleter deleter = [&deletions](Resource* r) {
            ++deletions;
            std::default_delete<Resource>{}(r);
        };
        // Create first with a default deleter so nothing leaks if building the deleter throws.
        std::unique_ptr<Resource> owned = create(kind, std::move(name));
        return std::unique_ptr<Resource, CountingDeleter>(owned.release(), std::move(deleter));
    }

    // ------------------------------------------------------------------- ResourceCache

    std::shared_ptr<Resource> ResourceCache::get_or_create(const std::string& name, const Factory& factory) {
        auto it = entries_.find(name);
        if (it != entries_.end()) {
            if (auto alive = it->second.lock()) {
                ++hits_;
                return alive;
            }
        }
        ++misses_;
        std::shared_ptr<Resource> created = factory();
        entries_.insert_or_assign(name, created);
        return created;
    }

    std::shared_ptr<Resource> ResourceCache::find(const std::string& name) const {
        const auto it = entries_.find(name);
        return it == entries_.end() ? nullptr : it->second.lock();
    }

    std::size_t ResourceCache::purge_expired() {
        return static_cast<std::size_t>(
            std::erase_if(entries_, [](const auto& entry) { return entry.second.expired(); }));
    }

    // ------------------------------------------------------------------------ TreeNode

    TreeNode::TreeNode(std::string label) : label_(std::move(label)) { live_.fetch_add(1); }

    TreeNode::~TreeNode() { live_.fetch_sub(1); }

    std::shared_ptr<TreeNode> TreeNode::create(std::string label) {
        return std::shared_ptr<TreeNode>(new TreeNode(std::move(label)));
    }

    std::shared_ptr<TreeNode> TreeNode::add_child(std::string label) {
        auto child = create(std::move(label));
        child->parent_ = weak_from_this();
        children_.push_back(child);
        return child;
    }

    std::string TreeNode::path() const {
        std::vector<std::string> parts{label_};
        for (auto p = parent(); p; p = p->parent()) {
            parts.push_back(p->label());
        }
        std::string result;
        for (auto it = parts.rbegin(); it != parts.rend(); ++it) {
            if (!result.empty()) {
                result += '/';
            }
            result += *it;
        }
        return result;
    }

    // ---------------------------------------------------------------------- Pimpl

    class PimplExample::Impl {
    public:
        int value = 0;
        std::vector<int> history;
    };

    PimplExample::PimplExample() : impl_(std::make_unique<Impl>()) {}
    PimplExample::PimplExample(const PimplExample& other)
        : impl_(other.impl_ ? std::make_unique<Impl>(*other.impl_) : nullptr) {}
    PimplExample& PimplExample::operator=(const PimplExample& other) {
        if (this != &other) {
            PimplExample copy(other);
            impl_ = std::move(copy.impl_);
        }
        return *this;
    }
    PimplExample::PimplExample(PimplExample&&) noexcept = default;
    PimplExample& PimplExample::operator=(PimplExample&&) noexcept = default;
    PimplExample::~PimplExample() = default;

    void PimplExample::set_value(int value) {
        if (!impl_) {
            impl_ = std::make_unique<Impl>();
        }
        impl_->history.push_back(value);
        impl_->value = value;
    }

    int PimplExample::value() const noexcept { return impl_ ? impl_->value : 0; }

    std::size_t PimplExample::history_size() const noexcept { return impl_ ? impl_->history.size() : 0; }

    // ------------------------------------------------------------------------ Showcase

    void demonstrateSmartPointers(std::ostream& out) {
        out << "=== Smart Pointers ===\n";
        const std::ios_base::fmtflags saved_flags = out.flags();
        struct FlagRestorer {
            std::ostream& stream;
            std::ios_base::fmtflags flags;
            ~FlagRestorer() { stream.flags(flags); }
        } const restorer{out, saved_flags};
        const int live_before = Resource::live_count();

        // 1. unique_ptr: factory, polymorphism, containers, transfer.
        {
            std::vector<std::unique_ptr<Resource>> fleet;
            fleet.push_back(ResourceFactory::create("station", "Gateway"));
            fleet.push_back(ResourceFactory::create("spacecraft", "Odyssey"));
            fleet.push_back(std::make_unique<SpaceStation>("Tiangong", 3));
            for (auto& r : fleet) {
                r->process();
                out << "unique_ptr<Resource> -> " << r->kind() << " '" << r->name() << "'\n";
            }
            auto station = SmartPtrUtils::dynamic_unique_cast<SpaceStation>(fleet[0]);
            out << "dynamic_unique_cast succeeded: " << std::boolalpha << (station != nullptr)
                << ", source now empty: " << (fleet[0] == nullptr) << "\n";
            auto failed = SmartPtrUtils::dynamic_unique_cast<SpaceStation>(fleet[1]);
            out << "Failed cast keeps ownership: " << (fleet[1] != nullptr && failed == nullptr) << "\n";

            int deletions = 0;
            {
                auto counted = ResourceFactory::create_counted("spacecraft", "Probe", deletions);
            }
            out << "Custom deleter invoked " << deletions << " time(s)\n";
        }

        // 2. shared_ptr: shared ownership, aliasing, thread-safe reference counting.
        {
            auto craft = std::make_shared<Spacecraft>("Endurance", 500.0);
            std::shared_ptr<Resource> as_base = craft;
            out << "shared_ptr use_count = " << craft.use_count() << "\n";

            struct Telemetry {
                double speed = 7.8;
                double altitude = 408.0;
            };
            auto telemetry = std::make_shared<Telemetry>();
            const auto altitude = SmartPtrUtils::member_alias(telemetry, &Telemetry::altitude);
            telemetry.reset();
            out << "Aliased member keeps owner alive: altitude = " << *altitude << "\n";

            std::vector<std::thread> threads;
            for (int t = 0; t < 4; ++t) {
                threads.emplace_back([craft] {
                    for (int i = 0; i < 1000; ++i) {
                        const std::shared_ptr<Spacecraft> copy = craft; // atomic refcount
                        static_cast<void>(copy);
                    }
                });
            }
            for (auto& th : threads) {
                th.join();
            }
            out << "After 4 threads x 1000 copies, use_count = " << craft.use_count() << "\n";
        }

        // 3. weak_ptr: observers, caches, cycle-free trees.
        {
            Spacecraft craft("Voyager", 100.0, 0.5);
            auto control = MissionControl::create();
            control->watch(craft);
            {
                auto temporary = MissionControl::create();
                temporary->watch(craft);
            }
            craft.consume_fuel(60.0);
            out << "MissionControl alerts: " << control->alerts().size() << ", live observers "
                << craft.live_observer_count() << "\n";

            ResourceCache cache;
            auto make = [] { return std::shared_ptr<Resource>(ResourceFactory::create("station", "Cached")); };
            auto first = cache.get_or_create("alpha", make);
            auto second = cache.get_or_create("alpha", make);
            out << "ResourceCache hits " << cache.hits() << ", misses " << cache.misses()
                << ", same object: " << (first == second) << "\n";
            first.reset();
            second.reset();
            out << "Expired entries purged: " << cache.purge_expired() << "\n";

            auto root = TreeNode::create("sol");
            auto earth = root->add_child("earth");
            auto moon = earth->add_child("moon");
            out << "TreeNode path: " << moon->path() << "\n";
        }
        out << "TreeNodes alive after scope (no cycles): " << TreeNode::live_count() << "\n";

        // 4. pimpl with value semantics.
        PimplExample a;
        a.set_value(7);
        PimplExample b = a;
        b.set_value(9);
        out << "Pimpl: a = " << a.value() << " (history " << a.history_size() << "), b = " << b.value()
            << " (history " << b.history_size() << ")\n";

        out << "Resources leaked by this demo: " << (Resource::live_count() - live_before) << "\n";
    }

} // namespace CppVerseHub::Memory
