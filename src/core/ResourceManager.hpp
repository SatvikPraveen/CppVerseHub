/**
 * @file ResourceManager.hpp
 * @brief A thread-safe resource ledger with producers, consumers and exact conservation invariants.
 *
 * Every entity owns an account holding integer amounts of each ResourceType. Resources enter the
 * economy only through `deposit` / production ("minting") and leave only through `withdraw` /
 * consumption / account closure ("burning"); `transfer` moves resources between accounts and never
 * changes totals. The manager tracks minted and burned totals so the invariant
 *
 *     sum(balances[type]) == minted[type] - burned[type]
 *
 * can be checked at any time (see `checkConservation`). Continuous per-second production and
 * consumption rates are integrated by `tick(dt)`; fractional output is carried between ticks so that
 * integer accounting stays exact. All public operations lock an internal mutex and provide the strong
 * exception guarantee. The design deliberately avoids a singleton: each Galaxy owns its own manager,
 * which keeps simulations independent and testable.
 */
#pragma once

#include "core/Identifiers.hpp"
#include "core/Resources.hpp"

#include <nlohmann/json_fwd.hpp>

#include <cstddef>
#include <map>
#include <mutex>
#include <vector>

namespace CppVerseHub::Core {

/// @brief One unmet consumption demand reported by ResourceManager::tick.
struct ResourceShortfall {
    EntityId account;         ///< Account that could not consume what it needed.
    ResourceType type;        ///< Resource that ran short.
    ResourceAmount shortfall; ///< Units demanded but not available.
};

/// @brief Summary of one ResourceManager::tick.
struct ResourceTickReport {
    ResourceAmounts produced;                  ///< Units minted by producers this tick.
    ResourceAmounts consumed;                  ///< Units burned by consumers this tick.
    std::vector<ResourceShortfall> shortfalls; ///< Unmet demands, ordered by account id then type.
};

/// @brief Thread-safe ledger of per-entity resource accounts. See the file comment for invariants.
class ResourceManager {
public:
    /// @brief Construct an empty ledger.
    ResourceManager() = default;
    ResourceManager(const ResourceManager&) = delete;            ///< Non-copyable (owns a mutex).
    ResourceManager& operator=(const ResourceManager&) = delete; ///< Non-copyable.
    ResourceManager(ResourceManager&&) = delete;                 ///< Non-movable.
    ResourceManager& operator=(ResourceManager&&) = delete;      ///< Non-movable.
    ~ResourceManager() = default;                                ///< Destructor.

    /// @brief Open an empty account. @param id Owner; must be valid and not already open.
    void openAccount(EntityId id);

    /**
     * @brief Close an account; any remaining balance is burned (counted as consumed).
     * @param id Owner.
     * @return true if an account was closed.
     */
    bool closeAccount(EntityId id);

    /// @brief Whether an account exists. @param id Owner. @return true if open.
    [[nodiscard]] bool hasAccount(EntityId id) const;

    /// @brief Number of open accounts. @return Count.
    [[nodiscard]] std::size_t accountCount() const;

    /// @brief Ids of all open accounts in ascending order. @return Ids.
    [[nodiscard]] std::vector<EntityId> accounts() const;

    /**
     * @brief Balance of one resource.
     * @param id Owner (must exist).
     * @param type Resource.
     * @return Units held.
     */
    [[nodiscard]] ResourceAmount balance(EntityId id, ResourceType type) const;

    /// @brief All balances of an account. @param id Owner (must exist). @return Balances.
    [[nodiscard]] ResourceAmounts balances(EntityId id) const;

    /**
     * @brief Mint resources into an account.
     * @param id Owner (must exist).
     * @param type Resource.
     * @param amount Non-negative amount.
     */
    void deposit(EntityId id, ResourceType type, ResourceAmount amount);

    /**
     * @brief Burn resources from an account.
     * @param id Owner (must exist).
     * @param type Resource.
     * @param amount Non-negative amount; throws InsufficientResourcesException if above the balance.
     */
    void withdraw(EntityId id, ResourceType type, ResourceAmount amount);

    /**
     * @brief Burn resources if available.
     * @param id Owner (must exist).
     * @param type Resource.
     * @param amount Non-negative amount.
     * @return true if withdrawn, false (and nothing changed) if the balance was insufficient.
     */
    [[nodiscard]] bool tryWithdraw(EntityId id, ResourceType type, ResourceAmount amount);

    /**
     * @brief Move resources between accounts; totals are unchanged.
     * @param from Source account (must exist and hold at least `amount`).
     * @param to Destination account (must exist).
     * @param type Resource.
     * @param amount Non-negative amount.
     */
    void transfer(EntityId from, EntityId to, ResourceType type, ResourceAmount amount);

    /**
     * @brief Move every resource from one account to another.
     * @param from Source account.
     * @param to Destination account.
     * @return The amounts moved.
     */
    ResourceAmounts transferAll(EntityId from, EntityId to);

    /**
     * @brief Set the continuous production rate of an account.
     * @param id Owner (must exist).
     * @param type Resource.
     * @param unitsPerSecond Non-negative, finite rate (0 unregisters the producer).
     */
    void setProductionRate(EntityId id, ResourceType type, double unitsPerSecond);

    /**
     * @brief Set the continuous consumption rate of an account.
     * @param id Owner (must exist).
     * @param type Resource.
     * @param unitsPerSecond Non-negative, finite rate (0 unregisters the consumer).
     */
    void setConsumptionRate(EntityId id, ResourceType type, double unitsPerSecond);

    /// @brief Production rate. @param id Owner. @param type Resource. @return Units per second.
    [[nodiscard]] double productionRate(EntityId id, ResourceType type) const;
    /// @brief Consumption rate. @param id Owner. @param type Resource. @return Units per second.
    [[nodiscard]] double consumptionRate(EntityId id, ResourceType type) const;

    /// @brief Number of (account, resource) pairs with a positive production rate. @return Count.
    [[nodiscard]] std::size_t producerCount() const;
    /// @brief Number of (account, resource) pairs with a positive consumption rate. @return Count.
    [[nodiscard]] std::size_t consumerCount() const;

    /**
     * @brief Integrate production then consumption over a time step.
     *
     * Accounts are processed in ascending id order, so the result is deterministic. Consumption is
     * limited by the available balance; unmet demand is reported as a shortfall and not carried.
     * @param dt Step length in seconds (non-negative, finite).
     * @return What was produced, consumed and lacking.
     */
    ResourceTickReport tick(double dt);

    /// @brief Units of a resource held across all accounts. @param type Resource. @return Total.
    [[nodiscard]] ResourceAmount total(ResourceType type) const;
    /// @brief Units ever minted. @param type Resource. @return Total minted.
    [[nodiscard]] ResourceAmount totalMinted(ResourceType type) const;
    /// @brief Units ever burned. @param type Resource. @return Total burned.
    [[nodiscard]] ResourceAmount totalBurned(ResourceType type) const;

    /// @brief Check `total == minted - burned` for every type and no negative balance. @return true if it
    /// holds.
    [[nodiscard]] bool checkConservation() const;

    /// @brief Remove every account and reset the minted/burned counters.
    void clear();

    /// @brief Serialise the full ledger (balances, rates, carries, counters). @param out Destination object.
    void toJson(nlohmann::json& out) const;

    /// @brief Replace the ledger with a serialised one. @param in Document produced by toJson.
    void fromJson(const nlohmann::json& in);

private:
    struct Account {
        ResourceAmounts balance;
        ResourceRates production;
        ResourceRates consumption;
        ResourceRates productionCarry;
        ResourceRates consumptionCarry;
    };

    Account& accountLocked(EntityId id);
    const Account& accountLocked(EntityId id) const;

    mutable std::mutex mutex_;
    std::map<EntityId, Account> accounts_;
    ResourceAmounts minted_;
    ResourceAmounts burned_;
};

} // namespace CppVerseHub::Core
