/**
 * @file EventSystem.hpp
 * @brief Type-safe publish/subscribe event bus with RAII subscription handles.
 *
 * Demonstrates type erasure keyed on `std::type_index`: subscribers register a callable for a concrete
 * event type `E`, and `publish(const E&)` dispatches only to handlers of exactly that type, with no
 * casts visible to users and no common event base class required. `subscribe` returns a move-only
 * `Subscription` whose destructor unsubscribes (RAII), so a handler can never outlive the object it
 * captures by accident. The bus state lives in a `std::shared_ptr`, and subscriptions hold a
 * `std::weak_ptr` to it, so destroying the bus before its subscriptions is safe.
 *
 * Thread safety: subscribe/unsubscribe/publish may be called concurrently. Handlers are invoked
 * outside the lock on a snapshot of the subscriber list, so handlers may themselves publish,
 * subscribe or unsubscribe (re-entrancy). A handler removed during a publish may still receive that
 * one in-flight event. Dispatch order is subscription order.
 */
#pragma once

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <type_traits>
#include <typeindex>
#include <unordered_map>
#include <utility>
#include <vector>

namespace CppVerseHub::Core {

namespace detail {

/// @brief Shared state of an EventBus (kept alive by the bus, observed by subscriptions).
struct EventBusState {
    using ErasedHandler = std::function<void(const void*)>; ///< Handler receiving a pointer to the event.

    /// @brief One registered handler.
    struct Entry {
        std::uint64_t token;                          ///< Unique registration token.
        std::shared_ptr<const ErasedHandler> handler; ///< Shared so snapshots are cheap.
    };

    std::mutex mutex;                                                 ///< Guards all members below.
    std::unordered_map<std::type_index, std::vector<Entry>> handlers; ///< Handlers per event type.
    std::uint64_t nextToken{1};                                       ///< Next registration token.
    std::uint64_t published{0};                                       ///< Total events published.

    /// @brief Remove a registration. @param type Event type. @param token Token. @return true if removed.
    bool remove(std::type_index type, std::uint64_t token);
};

} // namespace detail

/**
 * @brief Move-only RAII handle; unsubscribes its handler on destruction or reset().
 */
class Subscription {
public:
    /// @brief An empty (inactive) subscription.
    Subscription() noexcept = default;
    ~Subscription(); ///< Unsubscribes if still active.

    Subscription(const Subscription&) = delete;            ///< Non-copyable.
    Subscription& operator=(const Subscription&) = delete; ///< Non-copyable.

    /// @brief Move constructor. @param other Source; becomes inactive.
    Subscription(Subscription&& other) noexcept;
    /// @brief Move assignment; unsubscribes the current handler first. @param other Source. @return `*this`.
    Subscription& operator=(Subscription&& other) noexcept;

    /// @brief Unsubscribe now (idempotent).
    void reset() noexcept;

    /// @brief Whether the handler is still registered on a live bus. @return true if active.
    [[nodiscard]] bool active() const noexcept;

    /// @brief Same as active(). @return true if active.
    explicit operator bool() const noexcept { return active(); }

private:
    friend class EventBus;
    Subscription(std::weak_ptr<detail::EventBusState> state, std::type_index type,
                 std::uint64_t token) noexcept
        : state_(std::move(state)), type_(type), token_(token) {}

    std::weak_ptr<detail::EventBusState> state_;
    std::type_index type_{typeid(void)};
    std::uint64_t token_{0};
};

/// @brief Event types must be complete, non-reference, non-cv object types.
template <typename E>
concept EventType = std::is_object_v<E> && !std::is_const_v<E> && !std::is_volatile_v<E>;

/**
 * @brief Synchronous, type-safe event bus.
 */
class EventBus {
public:
    /// @brief Construct an empty bus.
    EventBus() : state_(std::make_shared<detail::EventBusState>()) {}
    EventBus(const EventBus&) = delete;            ///< Non-copyable: subscriptions refer to one bus.
    EventBus& operator=(const EventBus&) = delete; ///< Non-copyable.
    EventBus(EventBus&&) = delete;                 ///< Non-movable: a moved-from bus would have no state.
    EventBus& operator=(EventBus&&) = delete;      ///< Non-movable.
    ~EventBus() = default;                         ///< Outstanding subscriptions become inactive.

    /**
     * @brief Register a handler for events of type E.
     * @tparam E Event type.
     * @tparam F Callable with signature compatible with `void(const E&)`.
     * @param handler Handler to invoke on every published E.
     * @return RAII subscription; keep it alive for as long as the handler should run.
     */
    template <EventType E, std::invocable<const E&> F>
    [[nodiscard]] Subscription subscribe(F&& handler) {
        auto erased = std::make_shared<const detail::EventBusState::ErasedHandler>(
            [h = std::forward<F>(handler)](const void* event) mutable { h(*static_cast<const E*>(event)); });
        const std::lock_guard lock(state_->mutex);
        const std::uint64_t token = state_->nextToken++;
        state_->handlers[std::type_index(typeid(E))].push_back({token, std::move(erased)});
        return {state_, std::type_index(typeid(E)), token};
    }

    /**
     * @brief Deliver an event to every current subscriber of its exact type.
     * @tparam E Event type.
     * @param event The event.
     * @return Number of handlers invoked.
     */
    template <EventType E>
    std::size_t publish(const E& event) {
        std::vector<std::shared_ptr<const detail::EventBusState::ErasedHandler>> snapshot;
        {
            const std::lock_guard lock(state_->mutex);
            ++state_->published;
            const auto it = state_->handlers.find(std::type_index(typeid(E)));
            if (it == state_->handlers.end()) {
                return 0;
            }
            snapshot.reserve(it->second.size());
            for (const auto& entry : it->second) {
                snapshot.push_back(entry.handler);
            }
        }
        for (const auto& handler : snapshot) {
            (*handler)(static_cast<const void*>(&event));
        }
        return snapshot.size();
    }

    /// @brief Number of handlers subscribed to E. @tparam E Event type. @return Count.
    template <EventType E>
    [[nodiscard]] std::size_t subscriberCount() const {
        const std::lock_guard lock(state_->mutex);
        const auto it = state_->handlers.find(std::type_index(typeid(E)));
        return it == state_->handlers.end() ? 0U : it->second.size();
    }

    /// @brief Number of handlers across all event types. @return Count.
    [[nodiscard]] std::size_t totalSubscribers() const;

    /// @brief Number of publish() calls so far (with or without subscribers). @return Count.
    [[nodiscard]] std::uint64_t publishedCount() const;

private:
    std::shared_ptr<detail::EventBusState> state_;
};

} // namespace CppVerseHub::Core
