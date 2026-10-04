/**
 * @file Exceptions.hpp
 * @brief Domain exception hierarchy for the core simulation.
 *
 * Demonstrates a small, catchable-by-category exception hierarchy rooted in `std::runtime_error`:
 * callers can catch `CoreException` for "anything the simulation rejected" or a specific subclass
 * when they can recover (e.g. `InsufficientResourcesException` carries the shortfall details).
 * Exceptions are used only for contract violations and recoverable domain errors, never for
 * ordinary control flow inside the simulation loop.
 */
#pragma once

#include <cstdint>
#include <stdexcept>
#include <string>

#include "core/Identifiers.hpp"

namespace CppVerseHub::Core {

/// @brief Root of every exception thrown by the core module.
class CoreException : public std::runtime_error {
public:
    /// @brief Construct with a message. @param message Human-readable description.
    explicit CoreException(const std::string& message) : std::runtime_error(message) {}
};

/// @brief A caller supplied an argument that violates a precondition (empty name, negative amount, ...).
class InvalidArgumentException : public CoreException {
public:
    /// @brief Construct with a message. @param message Description of the violated precondition.
    explicit InvalidArgumentException(const std::string& message)
        : CoreException("invalid argument: " + message) {}
};

/// @brief An operation was attempted in a state that does not allow it (e.g. starting a finished mission).
class InvalidStateException : public CoreException {
public:
    /// @brief Construct with a message. @param message Description of the illegal transition.
    explicit InvalidStateException(const std::string& message) : CoreException("invalid state: " + message) {}
};

/// @brief A lookup by EntityId found nothing (or found an entity of the wrong kind).
class EntityNotFoundException : public CoreException {
public:
    /**
     * @brief Construct for a missing id.
     * @param id The id that was looked up.
     * @param detail Optional extra context.
     */
    explicit EntityNotFoundException(EntityId id, const std::string& detail = {})
        : CoreException("entity #" + std::to_string(id.value()) + " not found" +
                        (detail.empty() ? std::string{} : ": " + detail)),
          id_(id) {}

    /// @brief The id that was looked up. @return Entity id.
    [[nodiscard]] EntityId id() const noexcept { return id_; }

private:
    EntityId id_;
};

/// @brief A withdrawal or transfer requested more of a resource than an account holds.
class InsufficientResourcesException : public CoreException {
public:
    /**
     * @brief Construct with the shortfall details.
     * @param account Account that was debited.
     * @param resourceName Name of the resource type.
     * @param requested Amount requested.
     * @param available Amount actually available.
     */
    InsufficientResourcesException(EntityId account, const std::string& resourceName, std::int64_t requested,
                                   std::int64_t available)
        : CoreException("insufficient " + resourceName + " in account #" + std::to_string(account.value()) +
                        ": requested " + std::to_string(requested) + ", available " +
                        std::to_string(available)),
          requested_(requested), available_(available) {}

    /// @brief Requested amount. @return Amount.
    [[nodiscard]] std::int64_t requested() const noexcept { return requested_; }
    /// @brief Available amount. @return Amount.
    [[nodiscard]] std::int64_t available() const noexcept { return available_; }

private:
    std::int64_t requested_;
    std::int64_t available_;
};

/// @brief A factory was asked for an unknown product key, or a key was registered twice.
class FactoryException : public CoreException {
public:
    /// @brief Construct with a message. @param message Description.
    explicit FactoryException(const std::string& message) : CoreException("factory: " + message) {}
};

/// @brief A scenario document could not be parsed or failed validation.
class SerializationException : public CoreException {
public:
    /// @brief Construct with a message. @param message Description.
    explicit SerializationException(const std::string& message) : CoreException("serialization: " + message) {}
};

} // namespace CppVerseHub::Core
