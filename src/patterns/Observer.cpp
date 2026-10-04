/**
 * @file Observer.cpp
 * @brief Domain observers and the observer showcase.
 */

#include "patterns/Observer.hpp"

#include <sstream>

namespace CppVerseHub::Patterns {

std::string_view toString(PlanetEventKind kind) noexcept {
    switch (kind) {
        case PlanetEventKind::ResourceChanged:
            return "ResourceChanged";
        case PlanetEventKind::PopulationChanged:
            return "PopulationChanged";
        case PlanetEventKind::DefenseChanged:
            return "DefenseChanged";
        case PlanetEventKind::UnderAttack:
            return "UnderAttack";
    }
    return "Unknown";
}

std::string describe(const PlanetEvent& event) {
    std::ostringstream os;
    os << event.planet << ": " << toString(event.kind);
    if (!event.attribute.empty()) {
        os << ' ' << event.attribute;
    }
    os << ' ' << event.oldValue << " -> " << event.newValue;
    return os.str();
}

ObservablePlanet::ObservablePlanet(std::string name) : name_(std::move(name)) {}

void ObservablePlanet::setResource(const std::string& resource, double amount) {
    amount = std::max(0.0, amount);
    auto it = std::find_if(resources_.begin(), resources_.end(),
                           [&](const auto& entry) { return entry.first == resource; });
    double old = 0.0;
    if (it == resources_.end()) {
        resources_.emplace_back(resource, amount);
    } else {
        old = it->second;
        it->second = amount;
    }
    if (old != amount) {
        publish({name_, PlanetEventKind::ResourceChanged, resource, old, amount});
    }
}

double ObservablePlanet::resource(const std::string& resource) const {
    auto it = std::find_if(resources_.begin(), resources_.end(),
                           [&](const auto& entry) { return entry.first == resource; });
    return it == resources_.end() ? 0.0 : it->second;
}

void ObservablePlanet::setPopulation(double population) {
    population = std::max(0.0, population);
    const double old = std::exchange(population_, population);
    if (old != population) {
        publish({name_, PlanetEventKind::PopulationChanged, {}, old, population});
    }
}

void ObservablePlanet::setDefense(double rating) {
    rating = std::max(0.0, rating);
    const double old = std::exchange(defense_, rating);
    if (old != rating) {
        publish({name_, PlanetEventKind::DefenseChanged, {}, old, rating});
    }
}

void ObservablePlanet::attack(double damage) {
    damage = std::clamp(damage, 0.0, defense_);
    const double old = defense_;
    defense_ -= damage;
    publish({name_, PlanetEventKind::UnderAttack, {}, old, defense_});
}

void ObservablePlanet::publish(const PlanetEvent& event) {
    // Signal first: it never throws on its own; Subject may rethrow an observer's exception.
    changed_.emit(event);
    subject_.notify(event);
}

void ResourceMonitor::onNotify(const PlanetEvent& event) {
    if (event.kind == PlanetEventKind::ResourceChanged && event.newValue < threshold_ &&
        event.oldValue >= threshold_) {
        alerts_.push_back("LOW " + event.attribute + " on " + event.planet);
    }
}

void DefenseMonitor::onNotify(const PlanetEvent& event) {
    if (event.kind == PlanetEventKind::UnderAttack) {
        ++attacks_;
        damage_ += event.oldValue - event.newValue;
    }
    if ((event.kind == PlanetEventKind::UnderAttack || event.kind == PlanetEventKind::DefenseChanged) &&
        event.newValue <= critical_) {
        critical_hit_ = true;
    }
}

void EventLogger::onNotify(const PlanetEvent& event) {
    entries_.push_back(describe(event));
}

void demonstrateObserver(std::ostream& out) {
    out << "=== Observer pattern ===\n";
    ObservablePlanet terra("Terra");

    // Classic GoF: observers are owned by the caller; the subject only holds weak_ptrs.
    auto resources = std::make_shared<ResourceMonitor>(50.0);
    auto defense = std::make_shared<DefenseMonitor>(20.0);
    terra.subject().attach(resources);
    terra.subject().attach(defense);
    {
        auto logger = std::make_shared<EventLogger>();
        terra.subject().attach(logger);
        terra.setResource("minerals", 100.0);
        out << "  logger saw: " << logger->entries().back() << '\n';
    } // logger destroyed: the subject prunes it automatically
    out << "  live classic observers after logger expired: " << terra.subject().observerCount() << '\n';

    // Modern: RAII connections unsubscribe when they leave scope.
    std::size_t signalEvents = 0;
    {
        auto conn = terra.changed().connectScoped([&](const PlanetEvent&) { ++signalEvents; });
        terra.setResource("minerals", 40.0);
        terra.setDefense(60.0);
    }
    terra.attack(45.0); // no longer counted by the expired scoped connection
    out << "  signal events seen while connected: " << signalEvents << '\n';

    for (const auto& alert : resources->alerts()) {
        out << "  alert: " << alert << '\n';
    }
    out << "  attacks observed: " << defense->attacksObserved() << ", damage " << defense->totalDamage()
        << (defense->critical() ? " (CRITICAL)" : "") << '\n';
}

} // namespace CppVerseHub::Patterns
