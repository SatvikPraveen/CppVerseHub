/**
 * @file Fleet.cpp
 * @brief Fleet implementation.
 */
#include "core/Fleet.hpp"

#include "core/Exceptions.hpp"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <cmath>
#include <limits>
#include <sstream>

namespace CppVerseHub::Core {

Fleet::Fleet(EntityId id, std::string name, const Vector3D& position)
    : Entity(id, std::move(name), position) {}

Fleet::Fleet(EntityId id, const nlohmann::json& params) : Entity(id, params) {
    try {
        if (params.contains("ships")) {
            for (const auto& entry : params["ships"]) {
                const auto type = parseShipType(entry.at("type").get<std::string>());
                if (!type) {
                    throw InvalidArgumentException("unknown ship type");
                }
                if (entry.contains("hull")) {
                    addShip(Ship{*type, entry["hull"].get<double>()});
                } else {
                    addShips(*type, entry.value("count", std::size_t{1}));
                }
            }
        }
        if (params.contains("destination") && !params["destination"].is_null()) {
            setDestination(vectorFromJson(params["destination"]));
        }
        lowPower_ = params.value("lowPower", false);
    } catch (const nlohmann::json::exception& e) {
        throw SerializationException(std::string("fleet: ") + e.what());
    }
    refreshHealth(); // a fleet's health is derived from its hull, overriding any stored "health"
}

void Fleet::addShips(ShipType type, std::size_t count) {
    if (!isAlive()) {
        throw InvalidStateException("cannot add ships to a destroyed fleet");
    }
    ships_.insert(ships_.end(), count, Ship::make(type));
    refreshHealth();
}

void Fleet::addShip(const Ship& ship) {
    if (!isAlive()) {
        throw InvalidStateException("cannot add ships to a destroyed fleet");
    }
    if (!(ship.hull > 0.0 && ship.hull <= specOf(ship.type).maxHull)) {
        throw InvalidArgumentException("ship hull must be in (0, maxHull]");
    }
    ships_.push_back(ship);
    refreshHealth();
}

std::size_t Fleet::removeShips(ShipType type, std::size_t count) {
    std::size_t removed = 0;
    for (auto it = ships_.end(); it != ships_.begin() && removed < count;) {
        --it;
        if (it->type == type) {
            it = ships_.erase(it);
            ++removed;
        }
    }
    refreshHealth();
    return removed;
}

std::size_t Fleet::shipCount(ShipType type) const noexcept {
    return static_cast<std::size_t>(
        std::count_if(ships_.begin(), ships_.end(), [type](const Ship& s) { return s.type == type; }));
}

double Fleet::speed() const noexcept {
    if (ships_.empty() || !isAlive()) {
        return 0.0;
    }
    double slowest = std::numeric_limits<double>::infinity();
    for (const Ship& s : ships_) {
        slowest = std::min(slowest, specOf(s.type).speed);
    }
    return lowPower_ ? slowest * 0.5 : slowest;
}

double Fleet::attackPower() const noexcept {
    double total = 0.0;
    for (const Ship& s : ships_) {
        total += s.effectiveAttack();
    }
    return total;
}

double Fleet::totalHull() const noexcept {
    double total = 0.0;
    for (const Ship& s : ships_) {
        total += s.hull;
    }
    return total;
}

double Fleet::maxHull() const noexcept {
    double total = 0.0;
    for (const Ship& s : ships_) {
        total += specOf(s.type).maxHull;
    }
    return total;
}

void Fleet::refreshHealth() noexcept {
    const double max = maxHull();
    setHealthInternal(max > 0.0 ? kMaxHealth * totalHull() / max : (isAlive() ? kMaxHealth : 0.0));
}

void Fleet::setDestination(const Vector3D& destination) {
    if (!std::isfinite(destination.x) || !std::isfinite(destination.y) || !std::isfinite(destination.z)) {
        throw InvalidArgumentException("destination must be finite");
    }
    destination_ = destination;
}

double Fleet::etaTo(const Vector3D& target) const noexcept {
    const double s = speed();
    const double d = position().distanceTo(target);
    if (d == 0.0) {
        return 0.0;
    }
    return s > 0.0 ? d / s : std::numeric_limits<double>::infinity();
}

void Fleet::update(double dt) {
    if (!isAlive() || !destination_) {
        return;
    }
    const Vector3D delta = *destination_ - position();
    const double distance = delta.length();
    const double step = speed() * dt;
    if (distance <= step) {
        setPosition(*destination_);
        destination_.reset();
    } else if (step > 0.0) {
        setPosition(position() + delta * (step / distance));
    }
}

void Fleet::takeDamage(double amount) {
    if (!std::isfinite(amount) || amount < 0.0) {
        throw InvalidArgumentException("damage must be finite and non-negative");
    }
    if (!isAlive()) {
        return;
    }
    double remaining = amount;
    auto it = ships_.begin();
    while (remaining > 0.0 && it != ships_.end()) {
        if (it->hull <= remaining) {
            remaining -= it->hull;
            it = ships_.erase(it);
        } else {
            it->hull -= remaining;
            remaining = 0.0;
        }
    }
    if (ships_.empty()) {
        setStatus(EntityStatus::Destroyed);
    } else {
        refreshHealth();
    }
}

ResourceFlows Fleet::resourceFlows() const {
    ResourceFlows flows;
    if (isAlive()) {
        double upkeep = 0.0;
        for (const Ship& s : ships_) {
            upkeep += specOf(s.type).energyUpkeep;
        }
        flows.consumption[ResourceType::Energy] = upkeep;
    }
    return flows;
}

void Fleet::onResourceTick(const ResourceAmounts& shortfall) {
    lowPower_ = shortfall[ResourceType::Energy] > 0;
}

std::string Fleet::describe() const {
    std::ostringstream os;
    os << Entity::describe() << " ships " << ships_.size() << " attack " << attackPower() << " speed "
       << speed();
    if (destination_) {
        os << " -> " << *destination_;
    }
    return os.str();
}

void Fleet::toJson(nlohmann::json& out) const {
    Entity::toJson(out);
    nlohmann::json ships = nlohmann::json::array();
    for (const Ship& s : ships_) {
        ships.push_back({{"type", std::string(toString(s.type))}, {"hull", s.hull}});
    }
    out["ships"] = std::move(ships);
    if (destination_) {
        nlohmann::json d;
        vectorToJson(*destination_, d);
        out["destination"] = std::move(d);
    } else {
        out["destination"] = nullptr;
    }
    out["lowPower"] = lowPower_;
}

} // namespace CppVerseHub::Core
