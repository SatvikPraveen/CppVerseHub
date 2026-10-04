/**
 * @file EventSystem.cpp
 * @brief Non-template parts of the event bus and subscription handle.
 */
#include "core/EventSystem.hpp"

#include <algorithm>

namespace CppVerseHub::Core {

bool detail::EventBusState::remove(std::type_index type, std::uint64_t token) {
    const std::lock_guard lock(mutex);
    const auto it = handlers.find(type);
    if (it == handlers.end()) {
        return false;
    }
    auto& list = it->second;
    const auto pos = std::find_if(list.begin(), list.end(),
                                  [token](const Entry& e) { return e.token == token; });
    if (pos == list.end()) {
        return false;
    }
    list.erase(pos);
    if (list.empty()) {
        handlers.erase(it);
    }
    return true;
}

Subscription::~Subscription() {
    reset();
}

Subscription::Subscription(Subscription&& other) noexcept
    : state_(std::move(other.state_)), type_(other.type_), token_(std::exchange(other.token_, 0)) {}

Subscription& Subscription::operator=(Subscription&& other) noexcept {
    if (this != &other) {
        reset();
        state_ = std::move(other.state_);
        type_ = other.type_;
        token_ = std::exchange(other.token_, 0);
    }
    return *this;
}

void Subscription::reset() noexcept {
    if (token_ != 0) {
        if (auto state = state_.lock()) {
            try {
                static_cast<void>(state->remove(type_, token_));
                // NOLINTNEXTLINE(bugprone-empty-catch): std::mutex::lock may throw; reset() is noexcept
            } catch (...) {}
        }
        token_ = 0;
    }
    state_.reset();
}

bool Subscription::active() const noexcept {
    if (token_ == 0) {
        return false;
    }
    const auto state = state_.lock();
    if (!state) {
        return false;
    }
    try {
        const std::lock_guard lock(state->mutex);
        const auto it = state->handlers.find(type_);
        return it != state->handlers.end() &&
               std::any_of(it->second.begin(), it->second.end(),
                           [this](const detail::EventBusState::Entry& e) { return e.token == token_; });
    } catch (...) {
        return false;
    }
}

std::size_t EventBus::totalSubscribers() const {
    const std::lock_guard lock(state_->mutex);
    std::size_t n = 0;
    for (const auto& entry : state_->handlers) {
        n += entry.second.size();
    }
    return n;
}

std::uint64_t EventBus::publishedCount() const {
    const std::lock_guard lock(state_->mutex);
    return state_->published;
}

} // namespace CppVerseHub::Core
