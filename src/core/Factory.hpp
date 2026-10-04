/**
 * @file Factory.hpp
 * @brief A generic, type-safe factory registry and the default entity/mission factories.
 *
 * `Factory<Base, Key, Args...>` maps keys to creator callables returning `std::unique_ptr<Base>`.
 * It demonstrates the registry form of the Factory Method pattern: new products are added at run
 * time without touching existing code, creators are type-erased with `std::function`, and the
 * convenience `registerType<Derived>()` is constrained with concepts so registering a type that is
 * not a `Base` or cannot be built from `Args...` fails at compile time. The entity and mission
 * factories use the JSON object of a serialised entity/mission as their parameter bag, so
 * "create from parameters" and "load from a saved scenario" are the same code path.
 */
#pragma once

#include "core/Exceptions.hpp"
#include "core/Identifiers.hpp"

#include <nlohmann/json_fwd.hpp>

#include <concepts>
#include <cstddef>
#include <functional>
#include <map>
#include <memory>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace CppVerseHub::Core {

class Entity;
class Mission;

/**
 * @brief Registry of creators producing `std::unique_ptr<Base>` from `Args...`, keyed by `Key`.
 * @tparam Base Product base class.
 * @tparam Key Ordered key type (must be streamable for error messages).
 * @tparam Args Creator argument types.
 */
template <typename Base, typename Key, typename... Args>
class Factory {
public:
    /// @brief Type-erased creator.
    using Creator = std::function<std::unique_ptr<Base>(Args...)>;

    /**
     * @brief Register a creator.
     * @param key Unique key; throws FactoryException if already registered.
     * @param creator Non-empty creator.
     */
    void registerCreator(Key key, Creator creator) {
        if (!creator) {
            throw FactoryException("empty creator for key '" + keyString(key) + "'");
        }
        const std::string text = keyString(key);
        if (!creators_.emplace(std::move(key), std::move(creator)).second) {
            throw FactoryException("key '" + text + "' already registered");
        }
    }

    /**
     * @brief Register a concrete type constructed directly from `Args...`.
     * @tparam Derived Product type derived from Base.
     * @param key Unique key.
     */
    template <typename Derived>
        requires std::derived_from<Derived, Base> && std::constructible_from<Derived, Args...>
    void registerType(Key key) {
        registerCreator(std::move(key), [](Args... args) -> std::unique_ptr<Base> {
            return std::make_unique<Derived>(std::forward<Args>(args)...);
        });
    }

    /// @brief Remove a key. @param key Key. @return true if it was registered.
    bool unregister(const Key& key) { return creators_.erase(key) > 0; }

    /// @brief Whether a key is registered. @param key Key. @return true if registered.
    [[nodiscard]] bool contains(const Key& key) const { return creators_.contains(key); }

    /// @brief Number of registered keys. @return Count.
    [[nodiscard]] std::size_t size() const noexcept { return creators_.size(); }

    /// @brief Registered keys in ascending order. @return Keys.
    [[nodiscard]] std::vector<Key> keys() const {
        std::vector<Key> out;
        out.reserve(creators_.size());
        for (const auto& entry : creators_) {
            out.push_back(entry.first);
        }
        return out;
    }

    /**
     * @brief Create a product.
     * @param key Registered key; throws FactoryException otherwise.
     * @param args Creator arguments.
     * @return Newly created product (never null; a creator returning null raises FactoryException).
     */
    [[nodiscard]] std::unique_ptr<Base> create(const Key& key, Args... args) const {
        const auto it = creators_.find(key);
        if (it == creators_.end()) {
            throw FactoryException("unknown key '" + keyString(key) + "'");
        }
        auto product = it->second(std::forward<Args>(args)...);
        if (!product) {
            throw FactoryException("creator for '" + keyString(key) + "' returned null");
        }
        return product;
    }

private:
    static std::string keyString(const Key& key) {
        std::ostringstream os;
        os << key;
        return os.str();
    }

    std::map<Key, Creator> creators_;
};

/// @brief Creates entities from (id, JSON parameters), keyed by kind name ("planet", "fleet", ...).
using EntityFactory = Factory<Entity, std::string, EntityId, const nlohmann::json&>;
/// @brief Creates missions from (id, JSON parameters), keyed by type name ("exploration", ...).
using MissionFactory = Factory<Mission, std::string, MissionId, const nlohmann::json&>;

/// @brief Entity factory with "planet" and "fleet" registered. @return Factory.
[[nodiscard]] EntityFactory makeDefaultEntityFactory();
/// @brief Mission factory with "exploration", "combat" and "colonization" registered. @return Factory.
[[nodiscard]] MissionFactory makeDefaultMissionFactory();

} // namespace CppVerseHub::Core
