/**
 * @file ConfigManager.hpp
 * @brief Typed, thread-safe, observable configuration store with INI/JSON import and validation.
 *
 * Demonstrates: a closed set of value types modelled with `std::variant` and converted with
 * `if constexpr`; reader/writer locking with `std::shared_mutex` (many concurrent readers, exclusive
 * writers); the Observer pattern with callbacks invoked *outside* the lock to avoid re-entrancy
 * deadlocks; per-key validators; dependency injection of the environment lookup so overrides are
 * testable; and a fluent Builder. Keys are hierarchical, "section.key" ("graphics.width").
 */
#pragma once

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <ostream>
#include <shared_mutex>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

namespace CppVerseHub::Utils {

class JsonValue;

/// @brief Error raised for missing keys, type mismatches, validation failures and malformed input.
class ConfigError : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

/**
 * @brief A single configuration value: bool, integer, floating point, string or list of strings.
 */
class ConfigValue {
public:
    using List = std::vector<std::string>;                                       ///< String list.
    using Storage = std::variant<bool, std::int64_t, double, std::string, List>; ///< Underlying storage.

    /// @brief Kind of value.
    enum class Type : std::uint8_t { Bool, Int, Double, String, List };

    /// @brief Empty string value.
    ConfigValue() = default;
    /// @brief Boolean. @param b Value.
    ConfigValue(bool b) : value_(b) {} // NOLINT(google-explicit-constructor)
    /// @brief Integer from any integral type. @param i Value.
    template <std::integral I>
        requires(!std::same_as<I, bool>)
    ConfigValue(I i) : value_(static_cast<std::int64_t>(i)) {} // NOLINT(google-explicit-constructor)
    /// @brief Floating point. @param d Value.
    ConfigValue(double d) : value_(d) {} // NOLINT(google-explicit-constructor)
    /// @brief String. @param s Value.
    ConfigValue(std::string s) : value_(std::move(s)) {} // NOLINT(google-explicit-constructor)
    /// @brief String. @param s Value.
    ConfigValue(const char* s) : value_(std::string{s}) {} // NOLINT(google-explicit-constructor)
    /// @brief String. @param s Value.
    ConfigValue(std::string_view s) : value_(std::string{s}) {} // NOLINT(google-explicit-constructor)
    /// @brief List. @param l Value.
    ConfigValue(List l) : value_(std::move(l)) {} // NOLINT(google-explicit-constructor)

    /**
     * @brief Infers a typed value from text: "true"/"false"/"yes"/"no"/"on"/"off", integers, decimals,
     * "[a, b, c]" lists; anything else (or text in double quotes) is a string.
     * @param text Raw text.
     * @return Typed value.
     */
    [[nodiscard]] static ConfigValue parse(std::string_view text);

    /// @brief Kind of value. @return Type.
    [[nodiscard]] Type type() const noexcept { return static_cast<Type>(value_.index()); }

    /// @brief Name of the type. @return "bool", "int", "double", "string" or "list".
    [[nodiscard]] std::string_view typeName() const noexcept;

    /**
     * @brief Exact-type test.
     * @tparam T One of the storage types (or any integral type for Int).
     * @return True if the value holds T.
     */
    template <typename T>
    [[nodiscard]] bool is() const noexcept {
        if constexpr (std::same_as<T, bool>) {
            return std::holds_alternative<bool>(value_);
        } else if constexpr (std::integral<T>) {
            return std::holds_alternative<std::int64_t>(value_);
        } else if constexpr (std::floating_point<T>) {
            return std::holds_alternative<double>(value_);
        } else if constexpr (std::same_as<T, std::string>) {
            return std::holds_alternative<std::string>(value_);
        } else if constexpr (std::same_as<T, List>) {
            return std::holds_alternative<List>(value_);
        } else {
            return false;
        }
    }

    /**
     * @brief Converting accessor.
     *
     * Integers widen to floating point; floating values convert to integers only if integral; strings
     * are parsed for numeric/bool targets; every value converts to `std::string`; a string converts to
     * a one-element list.
     * @tparam T bool, an integral type, a floating-point type, std::string or List.
     * @return The converted value, or `std::nullopt` if conversion is impossible or out of range.
     */
    template <typename T>
    [[nodiscard]] std::optional<T> as() const;

    /**
     * @brief Throwing variant of `as()`.
     * @tparam T Target type.
     * @return Converted value.
     * @throws ConfigError if the conversion is impossible.
     */
    template <typename T>
    [[nodiscard]] T get() const {
        if (auto v = as<T>()) {
            return std::move(*v);
        }
        throw ConfigError("ConfigValue: cannot convert " + std::string{typeName()} + " '" + toString() +
                          "' to requested type");
    }

    /**
     * @brief Canonical text form (lists as "[a, b]"); `parse(toString())` round-trips except for
     * strings that look like other types.
     * @return Text.
     */
    [[nodiscard]] std::string toString() const;

    /// @brief Underlying variant. @return Storage.
    [[nodiscard]] const Storage& storage() const noexcept { return value_; }

    /// @brief Equality. @param a Lhs. @param b Rhs. @return True if same type and value.
    friend bool operator==(const ConfigValue& a, const ConfigValue& b) = default;

    /// @brief Streams `toString()`. @param os Stream. @param v Value. @return os.
    friend std::ostream& operator<<(std::ostream& os, const ConfigValue& v) { return os << v.toString(); }

private:
    [[nodiscard]] std::optional<std::int64_t> toInt() const noexcept;
    [[nodiscard]] std::optional<double> toDouble() const noexcept;
    [[nodiscard]] std::optional<bool> toBool() const noexcept;

    Storage value_{std::string{}};
};

template <typename T>
std::optional<T> ConfigValue::as() const {
    if constexpr (std::same_as<T, bool>) {
        return toBool();
    } else if constexpr (std::integral<T>) {
        const auto i = toInt();
        if (!i || !std::in_range<T>(*i)) {
            return std::nullopt;
        }
        return static_cast<T>(*i);
    } else if constexpr (std::floating_point<T>) {
        const auto d = toDouble();
        if (!d) {
            return std::nullopt;
        }
        return static_cast<T>(*d);
    } else if constexpr (std::same_as<T, std::string>) {
        return toString();
    } else if constexpr (std::same_as<T, List>) {
        if (const auto* l = std::get_if<List>(&value_)) {
            return *l;
        }
        return List{toString()};
    } else {
        static_assert(sizeof(T) == 0, "ConfigValue::as: unsupported target type");
    }
}

/**
 * @brief Hierarchical key/value configuration with thread-safe access and change notification.
 *
 * Copying copies values and validators but not listeners (observers belong to one instance).
 */
class ConfigManager {
public:
    /// @brief Description of one change delivered to listeners.
    struct Change {
        std::string key;                    ///< Affected key.
        std::optional<ConfigValue> before;  ///< Previous value (nullopt if the key was new).
        std::optional<ConfigValue> after;   ///< New value (nullopt if the key was removed).
    };
    using Listener = std::function<void(const Change&)>;            ///< Observer callback.
    using ListenerId = std::uint64_t;                               ///< Handle for unsubscribing.
    using Validator = std::function<bool(const ConfigValue&)>;      ///< Validation predicate.
    using EnvLookup = std::function<std::optional<std::string>(const std::string&)>; ///< Env lookup.

    ConfigManager() = default;
    ~ConfigManager() = default;
    /// @brief Copies values and validators. @param other Source.
    ConfigManager(const ConfigManager& other);
    /// @brief Moves values and validators. @param other Source.
    ConfigManager(ConfigManager&& other) noexcept;
    /// @brief Copy-assigns values and validators. @param other Source. @return `*this`.
    ConfigManager& operator=(const ConfigManager& other);
    /// @brief Move-assigns values and validators. @param other Source. @return `*this`.
    ConfigManager& operator=(ConfigManager&& other) noexcept;

    // ----- access ---------------------------------------------------------------------------------

    /**
     * @brief Stores a value after running the key's validator, then notifies listeners.
     * @param key "section.key" (non-empty).
     * @param value Value.
     * @throws ConfigError if the key is empty or validation fails.
     */
    void set(const std::string& key, ConfigValue value);

    /**
     * @brief Returns whether a key exists.
     * @param key Key.
     * @return True if present.
     */
    [[nodiscard]] bool has(const std::string& key) const;

    /**
     * @brief Raw value lookup.
     * @param key Key.
     * @return Value, or `std::nullopt`.
     */
    [[nodiscard]] std::optional<ConfigValue> value(const std::string& key) const;

    /**
     * @brief Typed lookup.
     * @tparam T Target type (see `ConfigValue::as`).
     * @param key Key.
     * @return Converted value, or `std::nullopt` if missing or not convertible.
     */
    template <typename T>
    [[nodiscard]] std::optional<T> tryGet(const std::string& key) const {
        const auto v = value(key);
        return v ? v->template as<T>() : std::nullopt;
    }

    /**
     * @brief Typed lookup that throws.
     * @tparam T Target type.
     * @param key Key.
     * @return Converted value.
     * @throws ConfigError if missing or not convertible.
     */
    template <typename T>
    [[nodiscard]] T get(const std::string& key) const {
        const auto v = value(key);
        if (!v) {
            throw ConfigError("ConfigManager: missing key '" + key + "'");
        }
        return v->template get<T>();
    }

    /**
     * @brief Typed lookup with a fallback.
     * @tparam T Target type.
     * @param key Key.
     * @param fallback Returned if missing or not convertible.
     * @return Value or fallback.
     */
    template <typename T>
    [[nodiscard]] T getOr(const std::string& key, T fallback) const {
        auto v = tryGet<T>(key);
        return v ? std::move(*v) : std::move(fallback);
    }

    /// @brief `getOr` overload so string literals deduce `std::string`.
    /// @param key Key. @param fallback Fallback. @return Value or fallback.
    [[nodiscard]] std::string getOr(const std::string& key, const char* fallback) const {
        return getOr<std::string>(key, std::string{fallback});
    }

    /**
     * @brief Removes a key and notifies listeners.
     * @param key Key.
     * @return True if the key existed.
     */
    bool remove(const std::string& key);

    /// @brief Removes every key (listeners are not notified per key).
    void clear();

    /// @brief Number of keys. @return Count.
    [[nodiscard]] std::size_t size() const;

    /**
     * @brief Keys starting with a prefix, sorted.
     * @param prefix Prefix (e.g. "graphics." for one section); empty for all keys.
     * @return Keys.
     */
    [[nodiscard]] std::vector<std::string> keys(std::string_view prefix = {}) const;

    /// @brief Distinct section names (text before the first '.'), sorted. @return Sections.
    [[nodiscard]] std::vector<std::string> sections() const;

    /**
     * @brief Copy of all entries in one section, keyed without the section prefix.
     * @param section Section name.
     * @return Entries.
     */
    [[nodiscard]] std::map<std::string, ConfigValue> section(std::string_view section) const;

    /// @brief Copy of every entry. @return Entries.
    [[nodiscard]] std::map<std::string, ConfigValue> snapshot() const;

    // ----- import / export ------------------------------------------------------------------------

    /**
     * @brief Loads INI text: `[section]` headers, `key = value`, `;`/`#` comments, quoted strings.
     * Keys before the first section have no prefix. Existing keys are overwritten.
     * @param text INI text.
     * @throws ConfigError with the line number on malformed lines or validation failure.
     */
    void loadIni(std::string_view text);

    /**
     * @brief Serialises to INI, grouping keys by section (deterministic order).
     * @return INI text.
     */
    [[nodiscard]] std::string toIni() const;

    /**
     * @brief Imports a JSON object, flattening nested objects into dotted keys. Arrays of scalars become
     * lists; numbers become Int when integral, else Double; null is skipped.
     * @param json JSON value (must be an object).
     * @throws ConfigError if `json` is not an object.
     */
    void loadJson(const JsonValue& json);

    /**
     * @brief Loads a file, choosing INI or JSON by extension (.json -> JSON, otherwise INI).
     * @param path File path.
     * @throws ConfigError or a ParseException subtype on failure.
     */
    void loadFile(const std::filesystem::path& path);

    /**
     * @brief Writes `toIni()` to a file.
     * @param path File path.
     * @throws std::runtime_error on I/O failure.
     */
    void saveIni(const std::filesystem::path& path) const;

    /**
     * @brief Copies entries from another manager.
     * @param other Source.
     * @param overwrite Whether to replace keys that already exist.
     * @return Number of keys written.
     */
    std::size_t merge(const ConfigManager& other, bool overwrite = true);

    /**
     * @brief For every existing key `a.b`, checks the variable `PREFIX_A_B` and applies it when set.
     * @param prefix Variable prefix (e.g. "CPPVERSEHUB"); empty means no prefix.
     * @param lookup Environment lookup; defaults to the process environment.
     * @return Number of overridden keys.
     */
    std::size_t applyEnvironmentOverrides(std::string_view prefix, const EnvLookup& lookup = systemEnvironment);

    /**
     * @brief Default lookup using the process environment.
     * @param name Variable name.
     * @return Value, if set.
     */
    [[nodiscard]] static std::optional<std::string> systemEnvironment(const std::string& name);

    /**
     * @brief Environment variable name for a key: ("app", "graphics.width") -> "APP_GRAPHICS_WIDTH".
     * @param prefix Prefix.
     * @param key Key.
     * @return Variable name.
     */
    [[nodiscard]] static std::string environmentName(std::string_view prefix, std::string_view key);

    // ----- validation & observers -----------------------------------------------------------------

    /**
     * @brief Registers a validator for a key; it also runs immediately against any existing value.
     * @param key Key.
     * @param validator Predicate.
     * @param description Message used in errors.
     * @throws ConfigError if the current value fails.
     */
    void addValidator(const std::string& key, Validator validator, std::string description = "invalid value");

    /**
     * @brief Re-validates every key that has a validator.
     * @return Human-readable failure messages (empty if valid).
     */
    [[nodiscard]] std::vector<std::string> validate() const;

    /**
     * @brief Lists required keys that are missing.
     * @param required Keys that must exist.
     * @return Missing keys.
     */
    [[nodiscard]] std::vector<std::string> missingKeys(const std::vector<std::string>& required) const;

    /**
     * @brief Subscribes to changes. Callbacks run on the mutating thread after the lock is released.
     * @param listener Callback.
     * @return Subscription id.
     */
    ListenerId addListener(Listener listener);

    /**
     * @brief Unsubscribes.
     * @param id Subscription id.
     * @return True if removed.
     */
    bool removeListener(ListenerId id);

private:
    struct ValidatorEntry {
        Validator check;
        std::string description;
    };

    void notify(const Change& change) const;

    mutable std::shared_mutex mutex_;
    std::map<std::string, ConfigValue> values_;
    std::map<std::string, ValidatorEntry> validators_;

    mutable std::mutex listenersMutex_;
    std::map<ListenerId, std::shared_ptr<const Listener>> listeners_;
    ListenerId nextListenerId_ = 1;
};

/**
 * @brief Fluent builder for `ConfigManager`: defaults, then INI text, then overrides.
 */
class ConfigBuilder {
public:
    /// @brief Adds a default value. @param key Key. @param value Value. @return `*this`.
    ConfigBuilder& withDefault(std::string key, ConfigValue value);
    /// @brief Adds INI text applied after defaults. @param text INI text. @return `*this`.
    ConfigBuilder& withIni(std::string text);
    /// @brief Adds an override applied last. @param key Key. @param value Value. @return `*this`.
    ConfigBuilder& withOverride(std::string key, ConfigValue value);
    /// @brief Adds a validator. @param key Key. @param validator Predicate. @param description Message.
    /// @return `*this`.
    ConfigBuilder& withValidator(std::string key, ConfigManager::Validator validator, std::string description);
    /// @brief Marks a key as required. @param key Key. @return `*this`.
    ConfigBuilder& require(std::string key);

    /**
     * @brief Builds the configuration.
     * @return Configured manager.
     * @throws ConfigError if a required key is missing or validation fails.
     */
    [[nodiscard]] ConfigManager build() const;

private:
    struct PendingValidator {
        std::string key;
        ConfigManager::Validator check;
        std::string description;
    };
    std::vector<std::pair<std::string, ConfigValue>> defaults_;
    std::vector<std::string> iniSources_;
    std::vector<std::pair<std::string, ConfigValue>> overrides_;
    std::vector<PendingValidator> validators_;
    std::vector<std::string> required_;
};

/**
 * @brief Runs the configuration showcase, writing only to `out`.
 * @param out Destination stream.
 */
void demonstrateConfig(std::ostream& out = std::cout);

} // namespace CppVerseHub::Utils
