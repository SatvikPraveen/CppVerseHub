/**
 * @file Entity.hpp
 * @brief Abstract base class of every simulated object (planets, fleets, ...).
 *
 * Demonstrates classic runtime polymorphism done carefully: a non-copyable identity type with a
 * virtual destructor, a pure virtual `update(double dt)` hook called once per fixed simulation step,
 * protected constructors that validate invariants (non-empty name, valid id, finite position), and
 * small virtual customisation points (`takeDamage`, `resourceFlows`, `onResourceTick`, `toJson`)
 * with sensible defaults. Entities never print and never touch global state.
 */
#pragma once

#include "core/Identifiers.hpp"
#include "core/Resources.hpp"
#include "core/Vector3D.hpp"

#include <nlohmann/json_fwd.hpp>

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace CppVerseHub::Core {

/// @brief Concrete kind of an entity (used for serialisation and factory keys).
enum class EntityKind : std::uint8_t { Planet, Fleet };

/// @brief Lifecycle status of an entity.
enum class EntityStatus : std::uint8_t { Active, Inactive, Destroyed };

/// @brief Lower-case name of an entity kind. @param kind Kind. @return "planet" or "fleet".
[[nodiscard]] std::string_view toString(EntityKind kind) noexcept;
/// @brief Parse an entity kind name. @param name Name. @return Kind or std::nullopt.
[[nodiscard]] std::optional<EntityKind> parseEntityKind(std::string_view name) noexcept;
/// @brief Lower-case name of a status. @param status Status. @return Name.
[[nodiscard]] std::string_view toString(EntityStatus status) noexcept;
/// @brief Parse a status name. @param name Name. @return Status or std::nullopt.
[[nodiscard]] std::optional<EntityStatus> parseEntityStatus(std::string_view name) noexcept;

/// @brief Continuous resource production and consumption an entity requests from the economy.
struct ResourceFlows {
    ResourceRates production;  ///< Units per second produced.
    ResourceRates consumption; ///< Units per second consumed.
};

/**
 * @brief Abstract simulated object with identity, name, position, health and status.
 */
class Entity {
public:
    /// @brief Maximum (and initial) health.
    static constexpr double kMaxHealth = 100.0;

    virtual ~Entity() =
        default; ///< Virtual destructor: entities are owned through `std::unique_ptr<Entity>`.

    Entity(const Entity&) = delete;            ///< Entities have identity; not copyable.
    Entity& operator=(const Entity&) = delete; ///< Not copy-assignable.
    Entity(Entity&&) = delete;                 ///< Not movable (referenced by id and address).
    Entity& operator=(Entity&&) = delete;      ///< Not move-assignable.

    /// @brief Unique id within the owning Galaxy. @return Id.
    [[nodiscard]] EntityId id() const noexcept { return id_; }
    /// @brief Display name. @return Name.
    [[nodiscard]] const std::string& name() const noexcept { return name_; }
    /// @brief Rename. @param name New non-empty name.
    void setName(std::string name);

    /// @brief Current position. @return Position.
    [[nodiscard]] const Vector3D& position() const noexcept { return position_; }
    /// @brief Teleport to a position. @param position New finite position.
    void setPosition(const Vector3D& position);

    /// @brief Distance between two entities. @param other Other entity. @return Euclidean distance.
    [[nodiscard]] double distanceTo(const Entity& other) const noexcept {
        return position_.distanceTo(other.position_);
    }

    /// @brief Health in [0, kMaxHealth]. @return Health.
    [[nodiscard]] double health() const noexcept { return health_; }
    /// @brief Lifecycle status. @return Status.
    [[nodiscard]] EntityStatus status() const noexcept { return status_; }
    /// @brief Change status. Destroyed is terminal. @param status New status.
    void setStatus(EntityStatus status);
    /// @brief Whether the entity is not destroyed. @return true if alive.
    [[nodiscard]] bool isAlive() const noexcept { return status_ != EntityStatus::Destroyed; }

    /**
     * @brief Apply damage. Reaching zero health destroys the entity.
     * @param amount Non-negative finite damage.
     */
    virtual void takeDamage(double amount);

    /// @brief Restore health up to kMaxHealth; no effect on destroyed entities. @param amount Non-negative
    /// amount.
    void heal(double amount);

    /// @brief Concrete kind. @return Kind.
    [[nodiscard]] virtual EntityKind kind() const noexcept = 0;

    /**
     * @brief Advance the entity's own state by one fixed time step.
     * @param dt Step length in seconds (positive).
     */
    virtual void update(double dt) = 0;

    /// @brief Resource rates this entity wants applied this step (default: none). @return Flows.
    [[nodiscard]] virtual ResourceFlows resourceFlows() const { return {}; }

    /// @brief Notification of unmet consumption after the economy tick. @param shortfall Units lacking per
    /// type.
    virtual void onResourceTick(const ResourceAmounts& shortfall) { static_cast<void>(shortfall); }

    /// @brief One-line human-readable summary. @return Description.
    [[nodiscard]] virtual std::string describe() const;

    /**
     * @brief Serialise the entity (including "kind") into a JSON object.
     *
     * The produced object is accepted by the matching constructor/factory, so `toJson` and the
     * factory form a lossless round trip.
     * @param out Destination object.
     */
    virtual void toJson(nlohmann::json& out) const;

protected:
    /**
     * @brief Construct with explicit state.
     * @param id Valid id.
     * @param name Non-empty name.
     * @param position Finite position.
     */
    Entity(EntityId id, std::string name, const Vector3D& position);

    /**
     * @brief Construct from a JSON object with "name", optional "position" [x,y,z], "health", "status".
     * @param id Valid id.
     * @param params Parameters.
     */
    Entity(EntityId id, const nlohmann::json& params);

    /// @brief Set health directly (clamped to [0, kMaxHealth]) for subclasses with derived health.
    /// @param health New health.
    void setHealthInternal(double health) noexcept;

private:
    EntityId id_;
    std::string name_;
    Vector3D position_;
    double health_{kMaxHealth};
    EntityStatus status_{EntityStatus::Active};
};

/// @brief Serialise a vector as a JSON array [x, y, z]. @param v Vector. @param out Destination.
void vectorToJson(const Vector3D& v, nlohmann::json& out);
/// @brief Parse a JSON array [x, y, z]. @param in Source. @return Vector.
[[nodiscard]] Vector3D vectorFromJson(const nlohmann::json& in);

} // namespace CppVerseHub::Core
