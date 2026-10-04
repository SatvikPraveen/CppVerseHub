/**
 * @file ResourceManager.cpp
 * @brief Implementation of the conserving resource ledger.
 */
#include "core/ResourceManager.hpp"

#include <cmath>
#include <string>

#include <nlohmann/json.hpp>

#include "core/Exceptions.hpp"

namespace CppVerseHub::Core {

namespace {

void requireNonNegative(ResourceAmount amount, const char* what) {
    if (amount < 0) {
        throw InvalidArgumentException(std::string(what) + " amount must be non-negative");
    }
}

void requireRate(double rate) {
    if (!std::isfinite(rate) || rate < 0.0) {
        throw InvalidArgumentException("resource rate must be finite and non-negative");
    }
}

/// Advance a fractional carry and return the whole units that became due.
ResourceAmount integrate(double& carry, double rate, double dt) {
    carry += rate * dt;
    const double whole = std::floor(carry);
    carry -= whole;
    return static_cast<ResourceAmount>(whole);
}

nlohmann::json amountsToJson(const ResourceAmounts& a) {
    nlohmann::json j = nlohmann::json::object();
    for (ResourceType t : kAllResourceTypes) {
        if (a[t] != 0) {
            j[std::string(toString(t))] = a[t];
        }
    }
    return j;
}

nlohmann::json ratesToJson(const ResourceRates& r) {
    nlohmann::json j = nlohmann::json::object();
    for (ResourceType t : kAllResourceTypes) {
        if (r[t] != 0.0) {
            j[std::string(toString(t))] = r[t];
        }
    }
    return j;
}

template <typename T>
ResourceArray<T> arrayFromJson(const nlohmann::json& j) {
    ResourceArray<T> out;
    if (j.is_null()) {
        return out;
    }
    if (!j.is_object()) {
        throw SerializationException("resource map must be an object");
    }
    for (const auto& [key, value] : j.items()) {
        const auto type = parseResourceType(key);
        if (!type) {
            throw SerializationException("unknown resource type '" + key + "'");
        }
        out[*type] = value.get<T>();
    }
    return out;
}

} // namespace

ResourceManager::Account& ResourceManager::accountLocked(EntityId id) {
    const auto it = accounts_.find(id);
    if (it == accounts_.end()) {
        throw EntityNotFoundException(id, "no resource account");
    }
    return it->second;
}

const ResourceManager::Account& ResourceManager::accountLocked(EntityId id) const {
    const auto it = accounts_.find(id);
    if (it == accounts_.end()) {
        throw EntityNotFoundException(id, "no resource account");
    }
    return it->second;
}

void ResourceManager::openAccount(EntityId id) {
    if (!id.isValid()) {
        throw InvalidArgumentException("cannot open an account for the invalid id");
    }
    const std::lock_guard lock(mutex_);
    if (!accounts_.try_emplace(id).second) {
        throw InvalidStateException("account #" + std::to_string(id.value()) + " already open");
    }
}

bool ResourceManager::closeAccount(EntityId id) {
    const std::lock_guard lock(mutex_);
    const auto it = accounts_.find(id);
    if (it == accounts_.end()) {
        return false;
    }
    burned_ += it->second.balance;
    accounts_.erase(it);
    return true;
}

bool ResourceManager::hasAccount(EntityId id) const {
    const std::lock_guard lock(mutex_);
    return accounts_.contains(id);
}

std::size_t ResourceManager::accountCount() const {
    const std::lock_guard lock(mutex_);
    return accounts_.size();
}

std::vector<EntityId> ResourceManager::accounts() const {
    const std::lock_guard lock(mutex_);
    std::vector<EntityId> ids;
    ids.reserve(accounts_.size());
    for (const auto& entry : accounts_) {
        ids.push_back(entry.first);
    }
    return ids;
}

ResourceAmount ResourceManager::balance(EntityId id, ResourceType type) const {
    const std::lock_guard lock(mutex_);
    return accountLocked(id).balance[type];
}

ResourceAmounts ResourceManager::balances(EntityId id) const {
    const std::lock_guard lock(mutex_);
    return accountLocked(id).balance;
}

void ResourceManager::deposit(EntityId id, ResourceType type, ResourceAmount amount) {
    requireNonNegative(amount, "deposit");
    const std::lock_guard lock(mutex_);
    accountLocked(id).balance[type] += amount;
    minted_[type] += amount;
}

void ResourceManager::withdraw(EntityId id, ResourceType type, ResourceAmount amount) {
    requireNonNegative(amount, "withdraw");
    const std::lock_guard lock(mutex_);
    Account& account = accountLocked(id);
    if (account.balance[type] < amount) {
        throw InsufficientResourcesException(id, std::string(toString(type)), amount, account.balance[type]);
    }
    account.balance[type] -= amount;
    burned_[type] += amount;
}

bool ResourceManager::tryWithdraw(EntityId id, ResourceType type, ResourceAmount amount) {
    requireNonNegative(amount, "withdraw");
    const std::lock_guard lock(mutex_);
    Account& account = accountLocked(id);
    if (account.balance[type] < amount) {
        return false;
    }
    account.balance[type] -= amount;
    burned_[type] += amount;
    return true;
}

void ResourceManager::transfer(EntityId from, EntityId to, ResourceType type, ResourceAmount amount) {
    requireNonNegative(amount, "transfer");
    const std::lock_guard lock(mutex_);
    Account& source = accountLocked(from);
    Account& target = accountLocked(to); // looked up before mutating: strong guarantee
    if (source.balance[type] < amount) {
        throw InsufficientResourcesException(from, std::string(toString(type)), amount, source.balance[type]);
    }
    source.balance[type] -= amount;
    target.balance[type] += amount;
}

ResourceAmounts ResourceManager::transferAll(EntityId from, EntityId to) {
    const std::lock_guard lock(mutex_);
    Account& source = accountLocked(from);
    Account& target = accountLocked(to);
    const ResourceAmounts moved = source.balance;
    if (from != to) {
        target.balance += moved;
        source.balance = ResourceAmounts{};
    }
    return moved;
}

void ResourceManager::setProductionRate(EntityId id, ResourceType type, double unitsPerSecond) {
    requireRate(unitsPerSecond);
    const std::lock_guard lock(mutex_);
    accountLocked(id).production[type] = unitsPerSecond;
}

void ResourceManager::setConsumptionRate(EntityId id, ResourceType type, double unitsPerSecond) {
    requireRate(unitsPerSecond);
    const std::lock_guard lock(mutex_);
    accountLocked(id).consumption[type] = unitsPerSecond;
}

double ResourceManager::productionRate(EntityId id, ResourceType type) const {
    const std::lock_guard lock(mutex_);
    return accountLocked(id).production[type];
}

double ResourceManager::consumptionRate(EntityId id, ResourceType type) const {
    const std::lock_guard lock(mutex_);
    return accountLocked(id).consumption[type];
}

std::size_t ResourceManager::producerCount() const {
    const std::lock_guard lock(mutex_);
    std::size_t n = 0;
    for (const auto& entry : accounts_) {
        for (double r : entry.second.production.values) {
            n += r > 0.0 ? 1U : 0U;
        }
    }
    return n;
}

std::size_t ResourceManager::consumerCount() const {
    const std::lock_guard lock(mutex_);
    std::size_t n = 0;
    for (const auto& entry : accounts_) {
        for (double r : entry.second.consumption.values) {
            n += r > 0.0 ? 1U : 0U;
        }
    }
    return n;
}

ResourceTickReport ResourceManager::tick(double dt) {
    if (!std::isfinite(dt) || dt < 0.0) {
        throw InvalidArgumentException("tick dt must be finite and non-negative");
    }
    const std::lock_guard lock(mutex_);
    ResourceTickReport report;
    for (auto& [id, account] : accounts_) {
        for (ResourceType t : kAllResourceTypes) {
            if (account.production[t] > 0.0) {
                const ResourceAmount made = integrate(account.productionCarry[t], account.production[t], dt);
                account.balance[t] += made;
                minted_[t] += made;
                report.produced[t] += made;
            }
            if (account.consumption[t] > 0.0) {
                const ResourceAmount demand = integrate(account.consumptionCarry[t], account.consumption[t], dt);
                const ResourceAmount used = demand < account.balance[t] ? demand : account.balance[t];
                account.balance[t] -= used;
                burned_[t] += used;
                report.consumed[t] += used;
                if (used < demand) {
                    report.shortfalls.push_back(ResourceShortfall{id, t, demand - used});
                }
            }
        }
    }
    return report;
}

ResourceAmount ResourceManager::total(ResourceType type) const {
    const std::lock_guard lock(mutex_);
    ResourceAmount sum = 0;
    for (const auto& entry : accounts_) {
        sum += entry.second.balance[type];
    }
    return sum;
}

ResourceAmount ResourceManager::totalMinted(ResourceType type) const {
    const std::lock_guard lock(mutex_);
    return minted_[type];
}

ResourceAmount ResourceManager::totalBurned(ResourceType type) const {
    const std::lock_guard lock(mutex_);
    return burned_[type];
}

bool ResourceManager::checkConservation() const {
    const std::lock_guard lock(mutex_);
    ResourceAmounts sums;
    for (const auto& entry : accounts_) {
        for (ResourceType t : kAllResourceTypes) {
            if (entry.second.balance[t] < 0) {
                return false;
            }
            sums[t] += entry.second.balance[t];
        }
    }
    for (ResourceType t : kAllResourceTypes) {
        if (sums[t] != minted_[t] - burned_[t]) {
            return false;
        }
    }
    return true;
}

void ResourceManager::clear() {
    const std::lock_guard lock(mutex_);
    accounts_.clear();
    minted_ = ResourceAmounts{};
    burned_ = ResourceAmounts{};
}

void ResourceManager::toJson(nlohmann::json& out) const {
    const std::lock_guard lock(mutex_);
    nlohmann::json list = nlohmann::json::array();
    for (const auto& [id, a] : accounts_) {
        list.push_back({{"id", id.value()},
                        {"balance", amountsToJson(a.balance)},
                        {"production", ratesToJson(a.production)},
                        {"consumption", ratesToJson(a.consumption)},
                        {"productionCarry", ratesToJson(a.productionCarry)},
                        {"consumptionCarry", ratesToJson(a.consumptionCarry)}});
    }
    out = {{"accounts", std::move(list)}, {"minted", amountsToJson(minted_)}, {"burned", amountsToJson(burned_)}};
}

void ResourceManager::fromJson(const nlohmann::json& in) {
    std::map<EntityId, Account> accounts;
    ResourceAmounts minted;
    ResourceAmounts burned;
    try {
        for (const auto& entry : in.at("accounts")) {
            const EntityId id{entry.at("id").get<std::uint64_t>()};
            if (!id.isValid()) {
                throw SerializationException("account with invalid id");
            }
            Account a;
            a.balance = arrayFromJson<ResourceAmount>(entry.value("balance", nlohmann::json{}));
            a.production = arrayFromJson<double>(entry.value("production", nlohmann::json{}));
            a.consumption = arrayFromJson<double>(entry.value("consumption", nlohmann::json{}));
            a.productionCarry = arrayFromJson<double>(entry.value("productionCarry", nlohmann::json{}));
            a.consumptionCarry = arrayFromJson<double>(entry.value("consumptionCarry", nlohmann::json{}));
            if (!accounts.emplace(id, a).second) {
                throw SerializationException("duplicate account id " + std::to_string(id.value()));
            }
        }
        minted = arrayFromJson<ResourceAmount>(in.value("minted", nlohmann::json{}));
        burned = arrayFromJson<ResourceAmount>(in.value("burned", nlohmann::json{}));
    } catch (const nlohmann::json::exception& e) {
        throw SerializationException(std::string("resource ledger: ") + e.what());
    }
    const std::lock_guard lock(mutex_);
    accounts_ = std::move(accounts);
    minted_ = minted;
    burned_ = burned;
}

} // namespace CppVerseHub::Core
