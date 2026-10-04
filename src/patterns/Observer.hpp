/**
 * @file Observer.hpp
 * @brief Observer pattern in two flavours: the classic GoF Subject/Observer and a modern
 *        type-safe Signal with RAII connections.
 *
 * The classic form (`Subject<Event>` + `IObserver<Event>`) stores observers as `std::weak_ptr`, so the
 * subject never extends an observer's lifetime and an observer that is destroyed simply stops
 * receiving events (dangling-observer bugs are impossible). The modern form (`Signal<Args...>`)
 * returns a `Connection` handle from `connect()`; wrapping it in a `ScopedConnection` unsubscribes
 * automatically when the handle goes out of scope. Both are thread-safe and allow observers to
 * (un)subscribe while a notification is in progress, because notification iterates over a snapshot.
 *
 * The domain example is an `ObservablePlanet` whose resource, population and defence changes are
 * monitored by `ResourceMonitor`, `DefenseMonitor` and `EventLogger`.
 */

#pragma once

#include <algorithm>
#include <atomic>
#include <cstddef>
#include <exception>
#include <functional>
#include <iostream>
#include <memory>
#include <mutex>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace CppVerseHub::Patterns {

// ============================================================================
// Classic GoF observer with weak_ptr-based registration
// ============================================================================

/**
 * @brief Observer interface for events of type @p Event.
 * @tparam Event The notification payload type.
 */
template <typename Event>
class IObserver {
public:
    virtual ~IObserver() = default;

    /**
     * @brief Called by a Subject for every published event.
     * @param event The event being published.
     */
    virtual void onNotify(const Event& event) = 0;

protected:
    IObserver() = default;
    IObserver(const IObserver&) = default;
    IObserver& operator=(const IObserver&) = default;
    IObserver(IObserver&&) = default;
    IObserver& operator=(IObserver&&) = default;
};

/**
 * @brief Subject that notifies non-owning (weak) observers.
 *
 * Observers are held via `std::weak_ptr`; expired observers are pruned lazily during
 * notification. If one or more observers throw, every remaining observer is still notified and the
 * first exception is rethrown afterwards.
 *
 * @tparam Event The notification payload type.
 */
template <typename Event>
class Subject {
public:
    using ObserverPtr = std::shared_ptr<IObserver<Event>>;

    Subject() = default;
    Subject(const Subject&) = delete;
    Subject& operator=(const Subject&) = delete;
    Subject(Subject&&) = delete;
    Subject& operator=(Subject&&) = delete;
    ~Subject() = default;

    /**
     * @brief Register an observer.
     * @param observer Observer to register (not owned).
     * @return false if @p observer is null or already registered.
     */
    bool attach(const ObserverPtr& observer) {
        if (!observer) {
            return false;
        }
        std::scoped_lock lock(mutex_);
        const bool duplicate = std::any_of(observers_.begin(), observers_.end(),
                                           [&](const auto& weak) { return weak.lock() == observer; });
        if (duplicate) {
            return false;
        }
        observers_.push_back(observer);
        return true;
    }

    /**
     * @brief Unregister an observer.
     * @param observer Raw identity of the observer to remove.
     * @return true if the observer was registered.
     */
    bool detach(const IObserver<Event>* observer) {
        if (observer == nullptr) {
            return false;
        }
        std::scoped_lock lock(mutex_);
        bool found = false;
        std::erase_if(observers_, [&](const auto& weak) {
            const auto strong = weak.lock();
            if (strong.get() == observer) {
                found = true;
                return true;
            }
            return !strong; // prune expired entries while we are here
        });
        return found;
    }

    /**
     * @brief Publish @p event to every live observer.
     * @param event Event to deliver.
     * @return Number of observers that received the event.
     */
    std::size_t notify(const Event& event) {
        std::vector<ObserverPtr> live;
        {
            std::scoped_lock lock(mutex_);
            live.reserve(observers_.size());
            std::erase_if(observers_, [&](const auto& weak) {
                auto strong = weak.lock();
                if (!strong) {
                    return true;
                }
                live.push_back(std::move(strong));
                return false;
            });
        }
        std::exception_ptr firstError;
        std::size_t delivered = 0;
        for (const auto& observer : live) {
            try {
                observer->onNotify(event);
                ++delivered;
            } catch (...) {
                if (!firstError) {
                    firstError = std::current_exception();
                }
            }
        }
        if (firstError) {
            std::rethrow_exception(firstError);
        }
        return delivered;
    }

    /**
     * @brief Number of registered observers that are still alive.
     * @return Live observer count.
     */
    [[nodiscard]] std::size_t observerCount() const {
        std::scoped_lock lock(mutex_);
        return static_cast<std::size_t>(std::count_if(observers_.begin(), observers_.end(),
                                                      [](const auto& weak) { return !weak.expired(); }));
    }

private:
    mutable std::mutex mutex_;
    std::vector<std::weak_ptr<IObserver<Event>>> observers_;
};

// ============================================================================
// Modern Signal/slot with RAII connections
// ============================================================================

namespace detail {
/// Type-erased control block shared between a Signal and its Connections.
struct SignalCore {
    std::mutex mutex;
    virtual ~SignalCore() = default;
    virtual void remove(const void* slot) noexcept = 0;

    SignalCore() = default;
    SignalCore(const SignalCore&) = delete;
    SignalCore& operator=(const SignalCore&) = delete;
    SignalCore(SignalCore&&) = delete;
    SignalCore& operator=(SignalCore&&) = delete;
};
} // namespace detail

/**
 * @brief Non-owning handle to a Signal subscription.
 *
 * Copyable. Disconnecting is idempotent and safe even after the Signal has been destroyed,
 * because the handle only holds weak references.
 */
class Connection {
public:
    Connection() = default;

    /**
     * @brief Remove the subscription (no-op if already disconnected or the signal is gone).
     */
    void disconnect() noexcept {
        if (auto core = core_.lock()) {
            core->remove(slot_);
        }
        core_.reset();
        active_.reset();
        slot_ = nullptr;
    }

    /**
     * @brief Whether the subscription is still registered with a live signal.
     * @return true while connected.
     */
    [[nodiscard]] bool connected() const noexcept {
        if (core_.expired()) {
            return false;
        }
        const auto flag = active_.lock();
        return flag != nullptr && flag->load(std::memory_order_acquire);
    }

private:
    template <typename...>
    friend class Signal;

    Connection(std::weak_ptr<detail::SignalCore> core, const void* slot,
               std::weak_ptr<const std::atomic<bool>> active) noexcept
        : core_(std::move(core)), slot_(slot), active_(std::move(active)) {}

    std::weak_ptr<detail::SignalCore> core_;
    const void* slot_ = nullptr;
    std::weak_ptr<const std::atomic<bool>> active_;
};

/**
 * @brief Move-only RAII wrapper that disconnects its Connection on destruction.
 */
class ScopedConnection {
public:
    ScopedConnection() = default;

    /**
     * @brief Take ownership of a connection.
     * @param connection Connection to manage.
     */
    explicit ScopedConnection(Connection connection) noexcept : connection_(std::move(connection)) {}

    ScopedConnection(const ScopedConnection&) = delete;
    ScopedConnection& operator=(const ScopedConnection&) = delete;
    ScopedConnection(ScopedConnection&& other) noexcept : connection_(std::exchange(other.connection_, {})) {}
    ScopedConnection& operator=(ScopedConnection&& other) noexcept {
        if (this != &other) {
            connection_.disconnect();
            connection_ = std::exchange(other.connection_, {});
        }
        return *this;
    }
    ~ScopedConnection() { connection_.disconnect(); }

    /// @brief Disconnect now. @return void
    void reset() noexcept { connection_.disconnect(); }

    /// @brief Release the connection without disconnecting. @return The released connection.
    [[nodiscard]] Connection release() noexcept { return std::exchange(connection_, {}); }

    /// @brief Whether the managed connection is live. @return true while connected.
    [[nodiscard]] bool connected() const noexcept { return connection_.connected(); }

private:
    Connection connection_;
};

/**
 * @brief Thread-safe multicast signal.
 *
 * `emit()` takes a snapshot of the slots under the lock and invokes them without holding it, so
 * slots may connect/disconnect (including themselves) re-entrantly. A slot disconnected during an
 * emission is not called afterwards by that emission.
 *
 * @tparam Args Argument types passed to every slot.
 */
template <typename... Args>
class Signal {
    struct Slot {
        std::function<void(Args...)> fn;
        std::atomic<bool> active{true}; // cleared on disconnect so in-flight emissions skip the slot
    };

    struct Core final : detail::SignalCore {
        std::vector<std::shared_ptr<Slot>> slots;
        void remove(const void* slot) noexcept override {
            std::scoped_lock lock(mutex);
            std::erase_if(slots, [&](const auto& s) {
                if (s.get() == slot) {
                    s->active.store(false, std::memory_order_release);
                    return true;
                }
                return false;
            });
        }
    };

public:
    using Slot_t = std::function<void(Args...)>;

    Signal() = default;
    Signal(const Signal&) = delete;
    Signal& operator=(const Signal&) = delete;
    Signal(Signal&&) = delete;
    Signal& operator=(Signal&&) = delete;
    ~Signal() = default;

    /**
     * @brief Subscribe a callable.
     * @param fn Callable invoked with the emitted arguments.
     * @return A handle that can disconnect the subscription.
     */
    [[nodiscard]] Connection connect(Slot_t fn) {
        auto slot = std::make_shared<Slot>();
        slot->fn = std::move(fn);
        // Aliasing constructor: shares ownership of the slot but points at its active flag.
        std::weak_ptr<const std::atomic<bool>> alive =
            std::shared_ptr<const std::atomic<bool>>(slot, &slot->active);
        const void* id = slot.get();
        {
            std::scoped_lock lock(core_->mutex);
            core_->slots.push_back(std::move(slot));
        }
        return Connection(core_, id, std::move(alive));
    }

    /**
     * @brief Subscribe and wrap the handle in a ScopedConnection.
     * @param fn Callable invoked with the emitted arguments.
     * @return RAII connection that unsubscribes when destroyed.
     */
    [[nodiscard]] ScopedConnection connectScoped(Slot_t fn) {
        return ScopedConnection(connect(std::move(fn)));
    }

    /**
     * @brief Invoke every connected slot.
     * @param args Arguments forwarded (by const reference) to each slot.
     * @return Number of slots invoked.
     */
    std::size_t emit(const Args&... args) const {
        // Copy-construct the snapshot under the lock (no lock held while slots run).
        const std::vector<std::shared_ptr<Slot>> snapshot = [this] {
            std::scoped_lock lock(core_->mutex);
            return core_->slots;
        }();
        std::size_t invoked = 0;
        for (const auto& slot : snapshot) {
            if (slot->active.load(std::memory_order_acquire)) {
                slot->fn(args...);
                ++invoked;
            }
        }
        return invoked;
    }

    /**
     * @brief Number of connected slots.
     * @return Slot count.
     */
    [[nodiscard]] std::size_t slotCount() const {
        std::scoped_lock lock(core_->mutex);
        return core_->slots.size();
    }

    /// @brief Disconnect every slot. @return void
    void disconnectAll() {
        std::scoped_lock lock(core_->mutex);
        for (const auto& slot : core_->slots) {
            slot->active.store(false, std::memory_order_release);
        }
        core_->slots.clear();
    }

private:
    std::shared_ptr<Core> core_ = std::make_shared<Core>();
};

// ============================================================================
// Domain: planet monitoring
// ============================================================================

/// @brief Kind of change published by an ObservablePlanet.
enum class PlanetEventKind { ResourceChanged, PopulationChanged, DefenseChanged, UnderAttack };

/// @brief Human-readable name of a PlanetEventKind. @param kind The kind. @return Its name.
[[nodiscard]] std::string_view toString(PlanetEventKind kind) noexcept;

/**
 * @brief Payload published by an ObservablePlanet.
 */
struct PlanetEvent {
    std::string planet;     ///< Name of the planet that changed.
    PlanetEventKind kind{}; ///< What changed.
    std::string attribute;  ///< Resource name, or empty for population/defence.
    double oldValue = 0.0;  ///< Value before the change.
    double newValue = 0.0;  ///< Value after the change.
};

/**
 * @brief A planet that publishes changes to both a classic Subject and a modern Signal.
 */
class ObservablePlanet {
public:
    /**
     * @brief Construct a planet.
     * @param name Planet name.
     */
    explicit ObservablePlanet(std::string name);

    /// @brief Planet name. @return The name.
    [[nodiscard]] const std::string& name() const noexcept { return name_; }

    /**
     * @brief Set a resource amount and publish a ResourceChanged event if it changed.
     * @param resource Resource name.
     * @param amount New amount (negative values are clamped to 0).
     */
    void setResource(const std::string& resource, double amount);

    /**
     * @brief Current amount of a resource.
     * @param resource Resource name.
     * @return Amount, or 0 if unknown.
     */
    [[nodiscard]] double resource(const std::string& resource) const;

    /// @brief Set the population and publish if changed. @param population New population.
    void setPopulation(double population);
    /// @brief Current population. @return Population.
    [[nodiscard]] double population() const noexcept { return population_; }

    /// @brief Set the defence rating and publish if changed. @param rating New rating.
    void setDefense(double rating);
    /// @brief Current defence rating. @return Rating.
    [[nodiscard]] double defense() const noexcept { return defense_; }

    /**
     * @brief Apply an attack: reduces defence by @p damage and publishes UnderAttack.
     * @param damage Damage dealt (clamped at the current defence).
     */
    void attack(double damage);

    /// @brief Classic subject for GoF observers. @return Subject reference.
    [[nodiscard]] Subject<PlanetEvent>& subject() noexcept { return subject_; }
    /// @brief Modern signal for callable subscribers. @return Signal reference.
    [[nodiscard]] Signal<PlanetEvent>& changed() noexcept { return changed_; }

private:
    void publish(const PlanetEvent& event);

    std::string name_;
    std::vector<std::pair<std::string, double>> resources_;
    double population_ = 0.0;
    double defense_ = 0.0;
    Subject<PlanetEvent> subject_;
    Signal<PlanetEvent> changed_;
};

/**
 * @brief Raises alerts when any resource drops below a threshold.
 */
class ResourceMonitor final : public IObserver<PlanetEvent> {
public:
    /// @brief Construct a monitor. @param threshold Alert threshold.
    explicit ResourceMonitor(double threshold) noexcept : threshold_(threshold) {}
    /// @brief Handle an event. @param event The event.
    void onNotify(const PlanetEvent& event) override;
    /// @brief Alerts raised so far. @return Alert messages.
    [[nodiscard]] const std::vector<std::string>& alerts() const noexcept { return alerts_; }

private:
    double threshold_;
    std::vector<std::string> alerts_;
};

/**
 * @brief Tracks attacks and flags planets whose defence becomes critical.
 */
class DefenseMonitor final : public IObserver<PlanetEvent> {
public:
    /// @brief Construct a monitor. @param criticalLevel Defence level considered critical.
    explicit DefenseMonitor(double criticalLevel) noexcept : critical_(criticalLevel) {}
    /// @brief Handle an event. @param event The event.
    void onNotify(const PlanetEvent& event) override;
    /// @brief Number of attacks observed. @return Count.
    [[nodiscard]] std::size_t attacksObserved() const noexcept { return attacks_; }
    /// @brief Total damage observed. @return Damage.
    [[nodiscard]] double totalDamage() const noexcept { return damage_; }
    /// @brief Whether any planet's defence fell to/below the critical level. @return Flag.
    [[nodiscard]] bool critical() const noexcept { return critical_hit_; }

private:
    double critical_;
    std::size_t attacks_ = 0;
    double damage_ = 0.0;
    bool critical_hit_ = false;
};

/**
 * @brief Records a one-line description of every event.
 */
class EventLogger final : public IObserver<PlanetEvent> {
public:
    /// @brief Handle an event. @param event The event.
    void onNotify(const PlanetEvent& event) override;
    /// @brief Logged lines. @return Lines in arrival order.
    [[nodiscard]] const std::vector<std::string>& entries() const noexcept { return entries_; }

private:
    std::vector<std::string> entries_;
};

/**
 * @brief Format an event as a single line.
 * @param event The event.
 * @return Description such as "Terra: ResourceChanged minerals 100 -> 40".
 */
[[nodiscard]] std::string describe(const PlanetEvent& event);

/**
 * @brief Showcase both observer flavours.
 * @param out Stream receiving the narration.
 */
void demonstrateObserver(std::ostream& out = std::cout);

} // namespace CppVerseHub::Patterns
