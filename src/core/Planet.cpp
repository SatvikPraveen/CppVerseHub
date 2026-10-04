/**
 * @file Planet.cpp
 * @brief Planet implementation.
 */
#include "core/Planet.hpp"

#include <array>
#include <cmath>
#include <sstream>

#include <nlohmann/json.hpp>

#include "core/Exceptions.hpp"

namespace CppVerseHub::Core {

namespace {

constexpr std::array<std::string_view, 6> kNames{"terrestrial", "desert", "ocean", "gas_giant", "ice", "volcanic"};

void requireNonNegativeFinite(double v, const char* what) {
    if (!std::isfinite(v) || v < 0.0) {
        throw InvalidArgumentException(std::string(what) + " must be finite and non-negative");
    }
}

} // namespace

std::string_view toString(PlanetType type) noexcept {
    const auto i = static_cast<std::size_t>(type);
    return i < kNames.size() ? kNames[i] : std::string_view{"unknown"};
}

std::optional<PlanetType> parsePlanetType(std::string_view name) noexcept {
    for (std::size_t i = 0; i < kNames.size(); ++i) {
        if (kNames[i] == name) {
            return static_cast<PlanetType>(i);
        }
    }
    return std::nullopt;
}

Planet::Planet(EntityId id, std::string name, const Vector3D& position, PlanetType type, double habitability)
    : Entity(id, std::move(name), position), type_(type) {
    setHabitability(habitability);
}

Planet::Planet(EntityId id, const nlohmann::json& params) : Entity(id, params), type_(PlanetType::Terrestrial) {
    try {
        if (params.contains("planetType")) {
            const auto t = parsePlanetType(params["planetType"].get<std::string>());
            if (!t) {
                throw InvalidArgumentException("unknown planet type");
            }
            type_ = *t;
        }
        setHabitability(params.value("habitability", 0.5));
        setPopulation(params.value("population", 0.0));
        setDefense(params.value("defense", 0.0));
        explored_ = params.value("explored", false);
        if (params.contains("production")) {
            for (const auto& [key, value] : params["production"].items()) {
                const auto r = parseResourceType(key);
                if (!r) {
                    throw InvalidArgumentException("unknown resource type '" + key + "'");
                }
                setProductionRate(*r, value.get<double>());
            }
        }
    } catch (const nlohmann::json::exception& e) {
        throw SerializationException(std::string("planet: ") + e.what());
    }
}

void Planet::setHabitability(double value) {
    if (!(value >= 0.0 && value <= 1.0)) {
        throw InvalidArgumentException("habitability must be in [0, 1]");
    }
    habitability_ = value;
}

void Planet::setPopulation(double value) {
    requireNonNegativeFinite(value, "population");
    population_ = value;
}

void Planet::setDefense(double value) {
    requireNonNegativeFinite(value, "defense");
    defense_ = value;
}

void Planet::setProductionRate(ResourceType type, double unitsPerSecond) {
    requireNonNegativeFinite(unitsPerSecond, "production rate");
    production_[type] = unitsPerSecond;
}

void Planet::update(double dt) {
    if (!isAlive() || population_ <= 0.0) {
        return;
    }
    const double capacity = populationCapacity();
    if (capacity <= 0.0) {
        return;
    }
    const double growth = kGrowthRate * population_ * (1.0 - population_ / capacity) * dt;
    population_ = population_ + growth > 0.0 ? population_ + growth : 0.0;
}

ResourceFlows Planet::resourceFlows() const {
    ResourceFlows flows;
    if (isAlive()) {
        flows.production = production_;
        flows.consumption[ResourceType::Food] = population_ * kFoodPerCapita;
    }
    return flows;
}

void Planet::onResourceTick(const ResourceAmounts& shortfall) {
    const ResourceAmount missing = shortfall[ResourceType::Food];
    if (missing > 0) {
        const double lost = static_cast<double>(missing) * kStarvationPerUnit;
        population_ = population_ > lost ? population_ - lost : 0.0;
    }
}

std::string Planet::describe() const {
    std::ostringstream os;
    os << Entity::describe() << " type " << toString(type_) << " habitability " << habitability_ << " population "
       << static_cast<std::int64_t>(population_) << " defense " << defense_ << (explored_ ? " explored" : "");
    return os.str();
}

void Planet::toJson(nlohmann::json& out) const {
    Entity::toJson(out);
    nlohmann::json production = nlohmann::json::object();
    for (ResourceType t : kAllResourceTypes) {
        if (production_[t] != 0.0) {
            production[std::string(toString(t))] = production_[t];
        }
    }
    out["planetType"] = std::string(toString(type_));
    out["habitability"] = habitability_;
    out["population"] = population_;
    out["defense"] = defense_;
    out["explored"] = explored_;
    out["production"] = std::move(production);
}

} // namespace CppVerseHub::Core
