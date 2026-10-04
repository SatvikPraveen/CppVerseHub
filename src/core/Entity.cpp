/**
 * @file Entity.cpp
 * @brief Entity base class implementation and enum conversions.
 */
#include "core/Entity.hpp"

#include <array>
#include <cmath>
#include <sstream>

#include <nlohmann/json.hpp>

#include "core/Exceptions.hpp"

namespace CppVerseHub::Core {

namespace {

constexpr std::array<std::string_view, 2> kKindNames{"planet", "fleet"};
constexpr std::array<std::string_view, 3> kStatusNames{"active", "inactive", "destroyed"};

bool isFinite(const Vector3D& v) noexcept { return std::isfinite(v.x) && std::isfinite(v.y) && std::isfinite(v.z); }

std::string validatedName(std::string name) {
    if (name.empty()) {
        throw InvalidArgumentException("entity name must not be empty");
    }
    return name;
}

} // namespace

std::string_view toString(EntityKind kind) noexcept {
    const auto i = static_cast<std::size_t>(kind);
    return i < kKindNames.size() ? kKindNames[i] : std::string_view{"unknown"};
}

std::optional<EntityKind> parseEntityKind(std::string_view name) noexcept {
    for (std::size_t i = 0; i < kKindNames.size(); ++i) {
        if (kKindNames[i] == name) {
            return static_cast<EntityKind>(i);
        }
    }
    return std::nullopt;
}

std::string_view toString(EntityStatus status) noexcept {
    const auto i = static_cast<std::size_t>(status);
    return i < kStatusNames.size() ? kStatusNames[i] : std::string_view{"unknown"};
}

std::optional<EntityStatus> parseEntityStatus(std::string_view name) noexcept {
    for (std::size_t i = 0; i < kStatusNames.size(); ++i) {
        if (kStatusNames[i] == name) {
            return static_cast<EntityStatus>(i);
        }
    }
    return std::nullopt;
}

void vectorToJson(const Vector3D& v, nlohmann::json& out) { out = nlohmann::json::array({v.x, v.y, v.z}); }

Vector3D vectorFromJson(const nlohmann::json& in) {
    if (!in.is_array() || in.size() != 3) {
        throw SerializationException("a vector must be an array of three numbers");
    }
    try {
        return {in[0].get<double>(), in[1].get<double>(), in[2].get<double>()};
    } catch (const nlohmann::json::exception& e) {
        throw SerializationException(std::string("vector: ") + e.what());
    }
}

Entity::Entity(EntityId id, std::string name, const Vector3D& position)
    : id_(id), name_(validatedName(std::move(name))), position_(position) {
    if (!id_.isValid()) {
        throw InvalidArgumentException("entity id must be valid (non-zero)");
    }
    if (!isFinite(position_)) {
        throw InvalidArgumentException("entity position must be finite");
    }
}

Entity::Entity(EntityId id, const nlohmann::json& params)
    : Entity(id, params.contains("name") && params["name"].is_string() ? params["name"].get<std::string>() : "",
             params.contains("position") ? vectorFromJson(params["position"]) : Vector3D{}) {
    if (params.contains("health")) {
        const double h = params["health"].get<double>();
        if (!std::isfinite(h)) {
            throw InvalidArgumentException("health must be finite");
        }
        setHealthInternal(h);
    }
    if (params.contains("status")) {
        const auto status = parseEntityStatus(params["status"].get<std::string>());
        if (!status) {
            throw SerializationException("unknown entity status");
        }
        status_ = *status;
    }
}

void Entity::setName(std::string name) { name_ = validatedName(std::move(name)); }

void Entity::setPosition(const Vector3D& position) {
    if (!isFinite(position)) {
        throw InvalidArgumentException("entity position must be finite");
    }
    position_ = position;
}

void Entity::setStatus(EntityStatus status) {
    if (status_ == EntityStatus::Destroyed && status != EntityStatus::Destroyed) {
        throw InvalidStateException("a destroyed entity cannot be revived");
    }
    status_ = status;
    if (status_ == EntityStatus::Destroyed) {
        health_ = 0.0;
    }
}

void Entity::takeDamage(double amount) {
    if (!std::isfinite(amount) || amount < 0.0) {
        throw InvalidArgumentException("damage must be finite and non-negative");
    }
    if (!isAlive()) {
        return;
    }
    setHealthInternal(health_ - amount);
    if (health_ <= 0.0) {
        status_ = EntityStatus::Destroyed;
    }
}

void Entity::heal(double amount) {
    if (!std::isfinite(amount) || amount < 0.0) {
        throw InvalidArgumentException("heal amount must be finite and non-negative");
    }
    if (isAlive()) {
        setHealthInternal(health_ + amount);
    }
}

void Entity::setHealthInternal(double health) noexcept {
    health_ = health < 0.0 ? 0.0 : (health > kMaxHealth ? kMaxHealth : health);
}

std::string Entity::describe() const {
    std::ostringstream os;
    os << toString(kind()) << ' ' << id_ << " '" << name_ << "' at " << position_ << " health " << health_ << " ["
       << toString(status_) << ']';
    return os.str();
}

void Entity::toJson(nlohmann::json& out) const {
    nlohmann::json pos;
    vectorToJson(position_, pos);
    out = {{"id", id_.value()},
           {"kind", std::string(toString(kind()))},
           {"name", name_},
           {"position", std::move(pos)},
           {"health", health_},
           {"status", std::string(toString(status_))}};
}

} // namespace CppVerseHub::Core
