/**
 * @file ConfigManager.cpp
 * @brief Implementation of the configuration store declared in ConfigManager.hpp.
 */
#include "utils/ConfigManager.hpp"

#include "utils/FileParser.hpp"
#include "utils/StringUtils.hpp"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <limits>
#include <set>

namespace CppVerseHub::Utils {

// ===================================================================================================
// ConfigValue
// ===================================================================================================

namespace {

std::optional<bool> parseBool(std::string_view text) noexcept {
    const auto t = String::trimView(text);
    if (String::equalsIgnoreCase(t, "true") || String::equalsIgnoreCase(t, "yes") ||
        String::equalsIgnoreCase(t, "on") || t == "1") {
        return true;
    }
    if (String::equalsIgnoreCase(t, "false") || String::equalsIgnoreCase(t, "no") ||
        String::equalsIgnoreCase(t, "off") || t == "0") {
        return false;
    }
    return std::nullopt;
}

std::string unquote(std::string_view text) {
    if (text.size() >= 2 &&
        ((text.front() == '"' && text.back() == '"') || (text.front() == '\'' && text.back() == '\''))) {
        std::string out;
        const auto inner = text.substr(1, text.size() - 2);
        for (std::size_t i = 0; i < inner.size(); ++i) {
            if (inner[i] == '\\' && i + 1 < inner.size()) {
                const char e = inner[++i];
                out.push_back(e == 'n' ? '\n' : e == 't' ? '\t' : e);
            } else {
                out.push_back(inner[i]);
            }
        }
        return out;
    }
    return std::string{text};
}

bool isQuoted(std::string_view text) noexcept {
    return text.size() >= 2 &&
           ((text.front() == '"' && text.back() == '"') || (text.front() == '\'' && text.back() == '\''));
}

} // namespace

ConfigValue ConfigValue::parse(std::string_view text) {
    const auto t = String::trimView(text);
    if (isQuoted(t)) {
        return ConfigValue{unquote(t)};
    }
    if (String::equalsIgnoreCase(t, "true") || String::equalsIgnoreCase(t, "yes") ||
        String::equalsIgnoreCase(t, "on")) {
        return ConfigValue{true};
    }
    if (String::equalsIgnoreCase(t, "false") || String::equalsIgnoreCase(t, "no") ||
        String::equalsIgnoreCase(t, "off")) {
        return ConfigValue{false};
    }
    if (const auto i = String::parseNumber<std::int64_t>(t)) {
        return ConfigValue{*i};
    }
    if (const auto d = String::parseNumber<double>(t); d && std::isfinite(*d)) {
        return ConfigValue{*d};
    }
    if (t.size() >= 2 && t.front() == '[' && t.back() == ']') {
        List items;
        for (const auto& part : String::split(t.substr(1, t.size() - 2), ',')) {
            const auto item = String::trimView(part);
            if (!item.empty()) {
                items.push_back(unquote(item));
            }
        }
        return ConfigValue{std::move(items)};
    }
    return ConfigValue{std::string{t}};
}

std::string_view ConfigValue::typeName() const noexcept {
    switch (type()) {
        case Type::Bool:
            return "bool";
        case Type::Int:
            return "int";
        case Type::Double:
            return "double";
        case Type::String:
            return "string";
        case Type::List:
            return "list";
    }
    return "unknown";
}

std::optional<std::int64_t> ConfigValue::toInt() const noexcept {
    return std::visit(
        [](const auto& v) -> std::optional<std::int64_t> {
            using T = std::decay_t<decltype(v)>;
            if constexpr (std::same_as<T, std::int64_t>) {
                return v;
            } else if constexpr (std::same_as<T, bool>) {
                return v ? 1 : 0;
            } else if constexpr (std::same_as<T, double>) {
                // 2^63 is exactly representable; values in [-2^63, 2^63) fit.
                if (std::isfinite(v) && v == std::trunc(v) && v >= -9.2233720368547758e18 &&
                    v < 9.2233720368547758e18) {
                    return static_cast<std::int64_t>(v);
                }
                return std::nullopt;
            } else if constexpr (std::same_as<T, std::string>) {
                return String::parseNumber<std::int64_t>(v);
            } else {
                return std::nullopt;
            }
        },
        value_);
}

std::optional<double> ConfigValue::toDouble() const noexcept {
    return std::visit(
        [](const auto& v) -> std::optional<double> {
            using T = std::decay_t<decltype(v)>;
            if constexpr (std::same_as<T, double>) {
                return v;
            } else if constexpr (std::same_as<T, std::int64_t>) {
                return static_cast<double>(v);
            } else if constexpr (std::same_as<T, std::string>) {
                return String::parseNumber<double>(v);
            } else {
                return std::nullopt;
            }
        },
        value_);
}

std::optional<bool> ConfigValue::toBool() const noexcept {
    return std::visit(
        [](const auto& v) -> std::optional<bool> {
            using T = std::decay_t<decltype(v)>;
            if constexpr (std::same_as<T, bool>) {
                return v;
            } else if constexpr (std::same_as<T, std::int64_t>) {
                return v != 0;
            } else if constexpr (std::same_as<T, std::string>) {
                return parseBool(v);
            } else {
                return std::nullopt;
            }
        },
        value_);
}

std::string ConfigValue::toString() const {
    return std::visit(
        [](const auto& v) -> std::string {
            using T = std::decay_t<decltype(v)>;
            if constexpr (std::same_as<T, bool>) {
                return v ? "true" : "false";
            } else if constexpr (std::same_as<T, std::int64_t>) {
                return std::to_string(v);
            } else if constexpr (std::same_as<T, double>) {
                // Shortest representation that still reads back as a double (keeps a '.').
                JsonValue j{v};
                std::string s = j.dump();
                if (s.find_first_of(".eE") == std::string::npos && s != "null") {
                    s += ".0";
                }
                return s;
            } else if constexpr (std::same_as<T, std::string>) {
                return v;
            } else {
                return "[" + String::join(v, ", ") + "]";
            }
        },
        value_);
}

// ===================================================================================================
// ConfigManager: special members
// ===================================================================================================

ConfigManager::ConfigManager(const ConfigManager& other) {
    const std::shared_lock lock(other.mutex_);
    values_ = other.values_;
    validators_ = other.validators_;
}

ConfigManager::ConfigManager(ConfigManager&& other) noexcept {
    const std::unique_lock lock(other.mutex_);
    values_ = std::move(other.values_);
    validators_ = std::move(other.validators_);
}

ConfigManager& ConfigManager::operator=(const ConfigManager& other) {
    if (this != &other) {
        std::unique_lock mine(mutex_, std::defer_lock);
        std::shared_lock theirs(other.mutex_, std::defer_lock);
        std::lock(mine, theirs);
        values_ = other.values_;
        validators_ = other.validators_;
    }
    return *this;
}

ConfigManager& ConfigManager::operator=(ConfigManager&& other) noexcept {
    if (this != &other) {
        std::unique_lock mine(mutex_, std::defer_lock);
        std::unique_lock theirs(other.mutex_, std::defer_lock);
        std::lock(mine, theirs);
        values_ = std::move(other.values_);
        validators_ = std::move(other.validators_);
    }
    return *this;
}

// ===================================================================================================
// ConfigManager: access
// ===================================================================================================

void ConfigManager::set(const std::string& key, ConfigValue value) {
    if (String::trimView(key).empty()) {
        throw ConfigError("ConfigManager: key must not be empty");
    }
    Change change{key, std::nullopt, std::nullopt};
    {
        const std::unique_lock lock(mutex_);
        if (const auto v = validators_.find(key); v != validators_.end() && !v->second.check(value)) {
            throw ConfigError("ConfigManager: validation failed for '" + key + "' = '" + value.toString() +
                              "': " + v->second.description);
        }
        auto it = values_.find(key);
        if (it != values_.end()) {
            if (it->second == value) {
                return; // no change, no notification
            }
            change.before = std::move(it->second);
            it->second = value;
        } else {
            values_.emplace(key, value);
        }
        change.after = std::move(value);
    }
    notify(change);
}

bool ConfigManager::has(const std::string& key) const {
    const std::shared_lock lock(mutex_);
    return values_.find(key) != values_.end();
}

std::optional<ConfigValue> ConfigManager::value(const std::string& key) const {
    const std::shared_lock lock(mutex_);
    const auto it = values_.find(key);
    if (it == values_.end()) {
        return std::nullopt;
    }
    return it->second;
}

bool ConfigManager::remove(const std::string& key) {
    Change change{key, std::nullopt, std::nullopt};
    {
        const std::unique_lock lock(mutex_);
        const auto it = values_.find(key);
        if (it == values_.end()) {
            return false;
        }
        change.before = std::move(it->second);
        values_.erase(it);
    }
    notify(change);
    return true;
}

void ConfigManager::clear() {
    const std::unique_lock lock(mutex_);
    values_.clear();
}

std::size_t ConfigManager::size() const {
    const std::shared_lock lock(mutex_);
    return values_.size();
}

std::vector<std::string> ConfigManager::keys(std::string_view prefix) const {
    const std::shared_lock lock(mutex_);
    std::vector<std::string> result;
    for (auto it = values_.lower_bound(std::string{prefix}); it != values_.end(); ++it) {
        if (!std::string_view{it->first}.starts_with(prefix)) {
            break; // map is sorted: all keys with the prefix are contiguous
        }
        result.push_back(it->first);
    }
    return result;
}

std::vector<std::string> ConfigManager::sections() const {
    const std::shared_lock lock(mutex_);
    std::set<std::string> names;
    for (const auto& [key, v] : values_) {
        const auto dot = key.find('.');
        if (dot != std::string::npos) {
            names.insert(key.substr(0, dot));
        }
    }
    return {names.begin(), names.end()};
}

std::map<std::string, ConfigValue> ConfigManager::section(std::string_view sectionName) const {
    const std::string prefix = std::string{sectionName} + ".";
    const std::shared_lock lock(mutex_);
    std::map<std::string, ConfigValue> result;
    for (auto it = values_.lower_bound(prefix); it != values_.end() && it->first.starts_with(prefix); ++it) {
        result.emplace(it->first.substr(prefix.size()), it->second);
    }
    return result;
}

std::map<std::string, ConfigValue> ConfigManager::snapshot() const {
    const std::shared_lock lock(mutex_);
    return values_;
}

// ===================================================================================================
// ConfigManager: import / export
// ===================================================================================================

void ConfigManager::loadIni(std::string_view text) {
    std::string currentSection;
    std::size_t lineNumber = 0;
    for (const auto& rawLine : String::split(text, '\n')) {
        ++lineNumber;
        const auto line = String::trimView(rawLine);
        if (line.empty() || line.front() == ';' || line.front() == '#') {
            continue;
        }
        if (line.front() == '[') {
            if (line.back() != ']' || line.size() < 3) {
                throw ConfigError("ConfigManager::loadIni: malformed section header on line " +
                                  std::to_string(lineNumber));
            }
            currentSection = String::trim(line.substr(1, line.size() - 2));
            continue;
        }
        const auto eq = line.find('=');
        if (eq == std::string_view::npos) {
            throw ConfigError("ConfigManager::loadIni: expected 'key = value' on line " +
                              std::to_string(lineNumber));
        }
        const auto key = String::trimView(line.substr(0, eq));
        if (key.empty()) {
            throw ConfigError("ConfigManager::loadIni: empty key on line " + std::to_string(lineNumber));
        }
        auto rawValue = String::trimView(line.substr(eq + 1));
        // Strip inline comments (" ; ..." or " # ...") outside quotes.
        if (!isQuoted(rawValue)) {
            for (std::size_t i = 1; i < rawValue.size(); ++i) {
                if ((rawValue[i] == ';' || rawValue[i] == '#') &&
                    (rawValue[i - 1] == ' ' || rawValue[i - 1] == '\t')) {
                    rawValue = String::trimView(rawValue.substr(0, i));
                    break;
                }
            }
        }
        const std::string fullKey = currentSection.empty() ? std::string{key}
                                                           : currentSection + "." + std::string{key};
        try {
            set(fullKey, ConfigValue::parse(rawValue));
        } catch (const ConfigError& e) {
            throw ConfigError(std::string{e.what()} + " (line " + std::to_string(lineNumber) + ")");
        }
    }
}

std::string ConfigManager::toIni() const {
    const auto entries = snapshot();
    std::string out;
    auto formatValue = [](const ConfigValue& v) {
        if (v.type() == ConfigValue::Type::String) {
            const std::string s = v.toString();
            // Quote strings that would otherwise be re-parsed as another type or lose whitespace.
            if (ConfigValue::parse(s) != v || s.find_first_of(";#") != std::string::npos) {
                return "\"" + String::replaceAll(String::replaceAll(s, "\\", "\\\\"), "\"", "\\\"") + "\"";
            }
            return s;
        }
        return v.toString();
    };
    // Unsectioned keys first.
    for (const auto& [key, v] : entries) {
        if (key.find('.') == std::string::npos) {
            out += key + " = " + formatValue(v) + "\n";
        }
    }
    std::string current;
    bool first = out.empty();
    for (const auto& [key, v] : entries) {
        const auto dot = key.find('.');
        if (dot == std::string::npos) {
            continue;
        }
        const std::string sectionName = key.substr(0, dot);
        if (sectionName != current) {
            if (!first) {
                out += "\n";
            }
            out += "[" + sectionName + "]\n";
            current = sectionName;
            first = false;
        }
        out += key.substr(dot + 1) + " = " + formatValue(v) + "\n";
    }
    return out;
}

namespace {
void flattenJson(ConfigManager& config, const JsonValue& json, const std::string& prefix) {
    for (const auto& [name, child] : json.asObject()) {
        const std::string key = prefix.empty() ? name : prefix + "." + name;
        switch (child.type()) {
            case JsonValue::Type::Null:
                break;
            case JsonValue::Type::Boolean:
                config.set(key, child.asBool());
                break;
            case JsonValue::Type::Number: {
                const double d = child.asNumber();
                if (d == std::trunc(d) && std::abs(d) < 9.007199254740992e15) {
                    config.set(key, static_cast<std::int64_t>(d));
                } else {
                    config.set(key, d);
                }
                break;
            }
            case JsonValue::Type::String:
                config.set(key, child.asString());
                break;
            case JsonValue::Type::Array: {
                ConfigValue::List items;
                for (const auto& item : child.asArray()) {
                    items.push_back(item.isString() ? item.asString() : item.dump());
                }
                config.set(key, std::move(items));
                break;
            }
            case JsonValue::Type::Object:
                flattenJson(config, child, key);
                break;
        }
    }
}
} // namespace

void ConfigManager::loadJson(const JsonValue& json) {
    if (!json.isObject()) {
        throw ConfigError("ConfigManager::loadJson: top-level JSON value must be an object");
    }
    flattenJson(*this, json, "");
}

void ConfigManager::loadFile(const std::filesystem::path& path) {
    const std::string content = FileParserUtils::readTextFile(path);
    if (FileParserUtils::detectFormat(path) == FileParserUtils::FileFormat::Json) {
        loadJson(JsonParser::parse(content));
    } else {
        loadIni(content);
    }
}

void ConfigManager::saveIni(const std::filesystem::path& path) const {
    FileParserUtils::writeTextFile(path, toIni());
}

std::size_t ConfigManager::merge(const ConfigManager& other, bool overwrite) {
    if (&other == this) {
        return 0;
    }
    std::size_t written = 0;
    for (const auto& [key, v] : other.snapshot()) {
        if (overwrite || !has(key)) {
            set(key, v);
            ++written;
        }
    }
    return written;
}

std::string ConfigManager::environmentName(std::string_view prefix, std::string_view key) {
    std::string name;
    if (!prefix.empty()) {
        name = String::toUpper(prefix) + "_";
    }
    for (const char c : key) {
        const bool alnum = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9');
        name.push_back(!alnum ? '_' : (c >= 'a' && c <= 'z') ? static_cast<char>(c - 'a' + 'A') : c);
    }
    return name;
}

std::optional<std::string> ConfigManager::systemEnvironment(const std::string& name) {
#if defined(_MSC_VER)
    char* buffer = nullptr;
    std::size_t length = 0;
    if (_dupenv_s(&buffer, &length, name.c_str()) != 0 || buffer == nullptr) {
        return std::nullopt;
    }
    const std::unique_ptr<char, decltype(&std::free)> owner(buffer, &std::free);
    return std::string{buffer};
#else
    const char* value = std::getenv(name.c_str()); // NOLINT(concurrency-mt-unsafe): read-only lookup
    if (value == nullptr) {
        return std::nullopt;
    }
    return std::string{value};
#endif
}

std::size_t ConfigManager::applyEnvironmentOverrides(std::string_view prefix, const EnvLookup& lookup) {
    std::size_t applied = 0;
    for (const auto& key : keys()) {
        if (const auto env = lookup(environmentName(prefix, key))) {
            set(key, ConfigValue::parse(*env));
            ++applied;
        }
    }
    return applied;
}

// ===================================================================================================
// ConfigManager: validation & observers
// ===================================================================================================

void ConfigManager::addValidator(const std::string& key, Validator validator, std::string description) {
    if (!validator) {
        throw ConfigError("ConfigManager::addValidator: validator must be callable");
    }
    const std::unique_lock lock(mutex_);
    if (const auto it = values_.find(key); it != values_.end() && !validator(it->second)) {
        throw ConfigError("ConfigManager: existing value of '" + key + "' fails validation: " + description);
    }
    validators_.insert_or_assign(key, ValidatorEntry{std::move(validator), std::move(description)});
}

std::vector<std::string> ConfigManager::validate() const {
    const std::shared_lock lock(mutex_);
    std::vector<std::string> failures;
    for (const auto& [key, entry] : validators_) {
        const auto it = values_.find(key);
        if (it != values_.end() && !entry.check(it->second)) {
            failures.push_back(key + ": " + entry.description);
        }
    }
    return failures;
}

std::vector<std::string> ConfigManager::missingKeys(const std::vector<std::string>& required) const {
    const std::shared_lock lock(mutex_);
    std::vector<std::string> missing;
    for (const auto& key : required) {
        if (values_.find(key) == values_.end()) {
            missing.push_back(key);
        }
    }
    return missing;
}

ConfigManager::ListenerId ConfigManager::addListener(Listener listener) {
    const std::lock_guard lock(listenersMutex_);
    const ListenerId id = nextListenerId_++;
    listeners_.emplace(id, std::make_shared<const Listener>(std::move(listener)));
    return id;
}

bool ConfigManager::removeListener(ListenerId id) {
    const std::lock_guard lock(listenersMutex_);
    return listeners_.erase(id) > 0;
}

void ConfigManager::notify(const Change& change) const {
    std::vector<std::shared_ptr<const Listener>> callbacks;
    {
        const std::lock_guard lock(listenersMutex_);
        callbacks.reserve(listeners_.size());
        for (const auto& [id, cb] : listeners_) {
            callbacks.push_back(cb);
        }
    }
    // No lock held here: listeners may freely read or modify the configuration.
    for (const auto& cb : callbacks) {
        if (*cb) {
            (*cb)(change);
        }
    }
}

// ===================================================================================================
// ConfigBuilder
// ===================================================================================================

ConfigBuilder& ConfigBuilder::withDefault(std::string key, ConfigValue value) {
    defaults_.emplace_back(std::move(key), std::move(value));
    return *this;
}

ConfigBuilder& ConfigBuilder::withIni(std::string text) {
    iniSources_.push_back(std::move(text));
    return *this;
}

ConfigBuilder& ConfigBuilder::withOverride(std::string key, ConfigValue value) {
    overrides_.emplace_back(std::move(key), std::move(value));
    return *this;
}

ConfigBuilder& ConfigBuilder::withValidator(std::string key, ConfigManager::Validator validator,
                                            std::string description) {
    validators_.push_back({std::move(key), std::move(validator), std::move(description)});
    return *this;
}

ConfigBuilder& ConfigBuilder::require(std::string key) {
    required_.push_back(std::move(key));
    return *this;
}

ConfigManager ConfigBuilder::build() const {
    ConfigManager config;
    for (const auto& v : validators_) {
        config.addValidator(v.key, v.check, v.description);
    }
    for (const auto& [key, value] : defaults_) {
        config.set(key, value);
    }
    for (const auto& ini : iniSources_) {
        config.loadIni(ini);
    }
    for (const auto& [key, value] : overrides_) {
        config.set(key, value);
    }
    if (const auto missing = config.missingKeys(required_); !missing.empty()) {
        throw ConfigError("ConfigBuilder: missing required keys: " + String::join(missing, ", "));
    }
    return config;
}

// ===================================================================================================
// Demo
// ===================================================================================================

void demonstrateConfig(std::ostream& out) {
    out << "=== Configuration ===\n";
    const std::string ini = R"(
; CppVerseHub sample configuration
[graphics]
width = 1920
height = 1080
fullscreen = yes
gamma = 2.2
title = "Space Explorer"

[fleet]
ships = [Vega, Rigel, Altair]
max_speed = 0.8   ; fraction of c
)";

    ConfigManager config = ConfigBuilder{}
                               .withDefault("audio.volume", 70)
                               .withIni(ini)
                               .withValidator(
                                   "audio.volume",
                                   [](const ConfigValue& v) {
                                       const auto i = v.as<int>();
                                       return i && *i >= 0 && *i <= 100;
                                   },
                                   "volume must be within 0..100")
                               .require("graphics.width")
                               .build();

    out << "graphics.width (int)    = " << config.get<int>("graphics.width") << '\n';
    out << "graphics.gamma (double) = " << config.get<double>("graphics.gamma") << '\n';
    out << "graphics.fullscreen     = " << std::boolalpha << config.get<bool>("graphics.fullscreen") << '\n';
    out << "fleet.ships (list)      = " << config.value("fleet.ships").value_or(ConfigValue{}) << '\n';
    out << "missing with default    = " << config.getOr("network.port", 7777) << '\n';
    out << "sections                = " << String::join(config.sections(), ", ") << '\n';

    std::vector<std::string> changes;
    const auto id = config.addListener([&changes](const ConfigManager::Change& c) {
        changes.push_back(c.key + " -> " + (c.after ? c.after->toString() : std::string{"<removed>"}));
    });
    config.set("audio.volume", 85);
    try {
        config.set("audio.volume", 150);
    } catch (const ConfigError& e) {
        out << "Rejected: " << e.what() << '\n';
    }
    config.remove("fleet.max_speed");
    config.removeListener(id);
    out << "Observed changes: " << String::join(changes, "; ") << '\n';

    // Environment overrides through an injected lookup (deterministic, no real environment access).
    const auto fakeEnv = [](const std::string& name) -> std::optional<std::string> {
        if (name == "CVH_GRAPHICS_WIDTH") {
            return "2560";
        }
        return std::nullopt;
    };
    const auto overridden = config.applyEnvironmentOverrides("cvh", fakeEnv);
    out << "Environment overrides applied: " << overridden
        << " (graphics.width = " << config.get<int>("graphics.width") << ")\n";

    ConfigManager fromJson;
    fromJson.loadJson(
        JsonParser::parse(R"({"server": {"host": "orbital.local", "port": 8080, "tls": true}})"));
    out << "From JSON: server.host = " << fromJson.get<std::string>("server.host")
        << ", server.port = " << fromJson.get<int>("server.port") << '\n';

    out << "Serialised INI:\n" << config.toIni();
    out << std::noboolalpha;
}

} // namespace CppVerseHub::Utils
