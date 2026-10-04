/**
 * @file SmartPointers.hpp
 * @brief Ownership semantics with unique_ptr, shared_ptr and weak_ptr.
 *
 * Demonstrates, in a small space-simulation domain:
 *  - Exclusive ownership and polymorphic deletion (`std::unique_ptr<Resource>`), factories
 *    returning owning pointers, and custom deleters.
 *  - Shared ownership (`std::shared_ptr`), the aliasing constructor, and
 *    `enable_shared_from_this`.
 *  - Non-owning observation (`std::weak_ptr`): an observer list that tolerates dead
 *    observers, a cache that does not keep entries alive, and parent links that break
 *    reference cycles in a tree.
 *  - The pimpl idiom with `std::unique_ptr<Impl>` and value semantics.
 *  - Safe ownership-transferring casts (`dynamic_unique_cast`).
 *
 * Why: smart pointers encode ownership in the type system, eliminating leaks, double frees
 * and dangling pointers, while weak_ptr makes non-owning back references safe.
 */

#ifndef CPPVERSEHUB_MEMORY_SMART_POINTERS_HPP
#define CPPVERSEHUB_MEMORY_SMART_POINTERS_HPP

#include <atomic>
#include <cstddef>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

namespace CppVerseHub::Memory {

    /**
     * @class Resource
     * @brief Polymorphic base class with a process-wide live-instance counter.
     */
    class Resource {
    public:
        /**
         * @brief Construct a named resource.
         * @param name Human-readable name.
         */
        explicit Resource(std::string name);
        Resource(const Resource&) = delete;
        Resource& operator=(const Resource&) = delete;
        Resource(Resource&&) = delete;
        Resource& operator=(Resource&&) = delete;
        /** @brief Virtual destructor: deletion through Resource* is well defined. */
        virtual ~Resource();

        /** @brief @return Resource name. */
        [[nodiscard]] const std::string& name() const noexcept { return name_; }
        /** @brief @return Unique id assigned at construction. */
        [[nodiscard]] int id() const noexcept { return id_; }
        /** @brief @return Short type tag, e.g. "station". */
        [[nodiscard]] virtual std::string_view kind() const noexcept = 0;
        /** @brief Advance the resource by one simulation step. */
        virtual void process() = 0;
        /** @brief @return Number of Resource objects currently alive. */
        [[nodiscard]] static int live_count() noexcept { return live_.load(); }

    private:
        std::string name_;
        int id_;
        inline static std::atomic<int> live_{0};
        inline static std::atomic<int> next_id_{1};
    };

    /**
     * @class SpaceStation
     * @brief Resource with a bounded population.
     */
    class SpaceStation final : public Resource {
    public:
        /**
         * @brief Construct a station.
         * @param name Station name.
         * @param capacity Maximum population.
         */
        SpaceStation(std::string name, int capacity);

        /** @brief @return "station". */
        [[nodiscard]] std::string_view kind() const noexcept override { return "station"; }
        /** @brief One step: population grows by one up to capacity. */
        void process() override;

        /**
         * @brief Add crew, clamped to capacity.
         * @param count People arriving (negative values are ignored).
         * @return Number actually admitted.
         */
        int add_population(int count) noexcept;

        /** @brief @return Current population. */
        [[nodiscard]] int population() const noexcept { return population_; }
        /** @brief @return Capacity. */
        [[nodiscard]] int capacity() const noexcept { return capacity_; }

    private:
        int capacity_;
        int population_ = 0;
    };

    /**
     * @class FuelObserver
     * @brief Interface for receivers of low-fuel notifications.
     */
    class FuelObserver {
    public:
        FuelObserver() = default;
        FuelObserver(const FuelObserver&) = default;
        FuelObserver& operator=(const FuelObserver&) = default;
        FuelObserver(FuelObserver&&) = default;
        FuelObserver& operator=(FuelObserver&&) = default;
        /** @brief Virtual destructor. */
        virtual ~FuelObserver() = default;

        /**
         * @brief Called when a craft's fuel falls below its threshold.
         * @param craft Craft name.
         * @param fuel Remaining fuel.
         */
        virtual void on_low_fuel(const std::string& craft, double fuel) = 0;
    };

    /**
     * @class Spacecraft
     * @brief Resource that notifies weakly-held observers about low fuel.
     */
    class Spacecraft final : public Resource {
    public:
        /**
         * @brief Construct a craft with a full tank.
         * @param name Craft name.
         * @param fuel_capacity Tank size (> 0).
         * @param low_fuel_ratio Fraction of capacity below which observers are notified.
         */
        Spacecraft(std::string name, double fuel_capacity, double low_fuel_ratio = 0.2);

        /** @brief @return "spacecraft". */
        [[nodiscard]] std::string_view kind() const noexcept override { return "spacecraft"; }
        /** @brief One step: burn 10% of capacity. */
        void process() override;

        /**
         * @brief Burn fuel (clamped at zero) and notify observers if low.
         * @param amount Fuel to burn.
         */
        void consume_fuel(double amount);

        /**
         * @brief Refuel (clamped at capacity).
         * @param amount Fuel to add.
         */
        void refuel(double amount) noexcept;

        /**
         * @brief Register an observer without taking ownership.
         * @param observer Observer; expired observers are pruned automatically.
         */
        void add_observer(std::weak_ptr<FuelObserver> observer);

        /** @brief @return Observers that are still alive (prunes expired ones). */
        std::size_t live_observer_count();

        /** @brief @return Current fuel. */
        [[nodiscard]] double fuel() const noexcept { return fuel_; }
        /** @brief @return Tank capacity. */
        [[nodiscard]] double fuel_capacity() const noexcept { return capacity_; }

    private:
        void notify_if_low();

        double capacity_;
        double fuel_;
        double threshold_;
        std::vector<std::weak_ptr<FuelObserver>> observers_;
    };

    /**
     * @class MissionControl
     * @brief FuelObserver that records alerts; uses enable_shared_from_this to self-register.
     */
    class MissionControl final : public FuelObserver, public std::enable_shared_from_this<MissionControl> {
    public:
        /**
         * @brief Create a MissionControl (must be owned by shared_ptr for watch()).
         * @return Shared owner.
         */
        [[nodiscard]] static std::shared_ptr<MissionControl> create() {
            return std::shared_ptr<MissionControl>(new MissionControl());
        }

        /**
         * @brief Subscribe to @p craft's fuel alerts using a weak reference to *this.
         * @param craft Craft to observe.
         */
        void watch(Spacecraft& craft) { craft.add_observer(weak_from_this()); }

        /**
         * @brief Record an alert.
         * @param craft Craft name.
         * @param fuel Remaining fuel.
         */
        void on_low_fuel(const std::string& craft, double fuel) override;

        /** @brief @return Alerts received, formatted as "name:fuel". */
        [[nodiscard]] const std::vector<std::string>& alerts() const noexcept { return alerts_; }

    private:
        MissionControl() = default;
        std::vector<std::string> alerts_;
    };

    /**
     * @class ResourceFactory
     * @brief Factory functions returning owning smart pointers.
     */
    class ResourceFactory {
    public:
        /**
         * @brief Create a resource by kind.
         * @param kind "station" or "spacecraft".
         * @param name Resource name.
         * @return Owning pointer, or nullptr for an unknown kind.
         */
        [[nodiscard]] static std::unique_ptr<Resource> create(std::string_view kind, std::string name);

        /** @brief Deleter type used by create_counted(). */
        using CountingDeleter = std::function<void(Resource*)>;

        /**
         * @brief Create a resource whose custom deleter increments @p deletions.
         * @param kind "station" or "spacecraft".
         * @param name Resource name.
         * @param deletions Counter incremented when the resource is deleted; must outlive it.
         * @return Owning pointer with custom deleter (empty for an unknown kind).
         */
        [[nodiscard]] static std::unique_ptr<Resource, CountingDeleter>
        create_counted(std::string_view kind, std::string name, int& deletions);
    };

    /**
     * @class ResourceCache
     * @brief Name -> resource cache holding weak references (does not extend lifetimes).
     */
    class ResourceCache {
    public:
        using Factory = std::function<std::shared_ptr<Resource>()>;

        /**
         * @brief Return the cached resource or create it with @p factory.
         * @param name Cache key.
         * @param factory Called on a miss.
         * @return Shared owner (the cache itself only keeps a weak_ptr).
         */
        std::shared_ptr<Resource> get_or_create(const std::string& name, const Factory& factory);

        /**
         * @brief Look up without creating.
         * @param name Cache key.
         * @return Resource if still alive, else nullptr.
         */
        [[nodiscard]] std::shared_ptr<Resource> find(const std::string& name) const;

        /** @brief Remove expired entries. @return Number removed. */
        std::size_t purge_expired();

        /** @brief @return Number of entries (including expired). */
        [[nodiscard]] std::size_t size() const noexcept { return entries_.size(); }
        /** @brief @return Cache hits so far. */
        [[nodiscard]] std::size_t hits() const noexcept { return hits_; }
        /** @brief @return Cache misses so far. */
        [[nodiscard]] std::size_t misses() const noexcept { return misses_; }

    private:
        std::unordered_map<std::string, std::weak_ptr<Resource>> entries_;
        std::size_t hits_ = 0;
        std::size_t misses_ = 0;
    };

    /**
     * @class TreeNode
     * @brief Tree whose children are owned (shared_ptr) and parents observed (weak_ptr).
     */
    class TreeNode : public std::enable_shared_from_this<TreeNode> {
    public:
        /**
         * @brief Create a node.
         * @param label Node label.
         * @return Shared owner.
         */
        [[nodiscard]] static std::shared_ptr<TreeNode> create(std::string label);

        /**
         * @brief Create and attach a child.
         * @param label Child label.
         * @return The new child.
         */
        std::shared_ptr<TreeNode> add_child(std::string label);

        /** @brief @return Parent, or nullptr for a root (or if the parent died). */
        [[nodiscard]] std::shared_ptr<TreeNode> parent() const { return parent_.lock(); }
        /** @brief @return Children. */
        [[nodiscard]] const std::vector<std::shared_ptr<TreeNode>>& children() const noexcept { return children_; }
        /** @brief @return Label. */
        [[nodiscard]] const std::string& label() const noexcept { return label_; }
        /** @brief @return "root/child/..." path built by walking weak parent links. */
        [[nodiscard]] std::string path() const;
        /** @brief @return Number of live TreeNode objects. */
        [[nodiscard]] static int live_count() noexcept { return live_.load(); }

        TreeNode(const TreeNode&) = delete;
        TreeNode& operator=(const TreeNode&) = delete;
        TreeNode(TreeNode&&) = delete;
        TreeNode& operator=(TreeNode&&) = delete;
        /** @brief Destructor (decrements the live counter). */
        ~TreeNode();

    private:
        explicit TreeNode(std::string label);

        std::string label_;
        std::weak_ptr<TreeNode> parent_;
        std::vector<std::shared_ptr<TreeNode>> children_;
        inline static std::atomic<int> live_{0};
    };

    /**
     * @class PimplExample
     * @brief Value type whose implementation is hidden behind `std::unique_ptr<Impl>`.
     *
     * Copy operations deep-copy the implementation; moves are noexcept. The special
     * members are defined in the .cpp where Impl is complete.
     */
    class PimplExample {
    public:
        /** @brief Construct with value 0 and no history. */
        PimplExample();
        /**
         * @brief Deep copy.
         * @param other Source.
         */
        PimplExample(const PimplExample& other);
        /**
         * @brief Deep-copy assignment.
         * @param other Source.
         * @return *this.
         */
        PimplExample& operator=(const PimplExample& other);
        /** @brief Move constructor. */
        PimplExample(PimplExample&&) noexcept;
        /** @brief Move assignment. @return *this. */
        PimplExample& operator=(PimplExample&&) noexcept;
        /** @brief Destructor (defined where Impl is complete). */
        ~PimplExample();

        /**
         * @brief Set the value (recorded in history).
         * @param value New value.
         */
        void set_value(int value);
        /** @brief @return Current value (0 for a moved-from object). */
        [[nodiscard]] int value() const noexcept;
        /** @brief @return Number of set_value calls. */
        [[nodiscard]] std::size_t history_size() const noexcept;
        /** @brief @return False for a moved-from object. */
        [[nodiscard]] bool valid() const noexcept { return impl_ != nullptr; }

    private:
        class Impl;
        std::unique_ptr<Impl> impl_;
    };

    /**
     * @brief Helpers for converting between smart pointer types.
     */
    namespace SmartPtrUtils {

        /**
         * @brief Ownership-transferring dynamic_cast for unique_ptr.
         *
         * On success ownership moves to the result; on failure @p ptr keeps ownership.
         *
         * @param ptr Source pointer.
         * @return Derived pointer, or nullptr if the dynamic type is not Derived.
         */
        template <typename Derived, typename Base>
        [[nodiscard]] std::unique_ptr<Derived> dynamic_unique_cast(std::unique_ptr<Base>& ptr) noexcept {
            if (auto* d = dynamic_cast<Derived*>(ptr.get())) {
                static_cast<void>(ptr.release());
                return std::unique_ptr<Derived>(d);
            }
            return nullptr;
        }

        /**
         * @brief Convert unique ownership into shared ownership.
         * @param ptr Source (left empty).
         * @return Shared owner.
         */
        template <typename T, typename D>
        [[nodiscard]] std::shared_ptr<T> to_shared(std::unique_ptr<T, D>&& ptr) {
            return std::shared_ptr<T>(std::move(ptr));
        }

        /**
         * @brief Aliasing constructor: share ownership of @p owner while pointing at a member.
         * @param owner Owning pointer.
         * @param member Pointer-to-member to expose.
         * @return Pointer to the member that keeps @p owner alive.
         */
        template <typename T, typename M>
        [[nodiscard]] std::shared_ptr<M> member_alias(const std::shared_ptr<T>& owner, M T::*member) noexcept {
            return std::shared_ptr<M>(owner, &((*owner).*member));
        }

    } // namespace SmartPtrUtils

    /**
     * @brief Showcase: unique/shared/weak ownership, observers, caches, trees and pimpl.
     * @param out Stream receiving the narration.
     */
    void demonstrateSmartPointers(std::ostream& out = std::cout);

} // namespace CppVerseHub::Memory

#endif // CPPVERSEHUB_MEMORY_SMART_POINTERS_HPP
