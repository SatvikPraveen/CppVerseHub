// File: src/utils/ConfigManager.hpp
// Application Configuration Management System

#pragma once

#include <string>
#include <map>
#include <unordered_map>
#include <variant>
#include <optional>
#include <vector>
#include <memory>
#include <mutex>
#include <fstream>
#include <iostream>
#include <type_traits>
#include <functional>

namespace CppVerseHub::Utils {

// Forward declaration
class JsonValue;

// ===== CONFIGURATION VALUE TYPE =====

class ConfigValue {
public:
    using ValueType = std::variant<bool, int, double, std::string, std::vector<std::string>>;
    
private:
    ValueType value_;
    std::string description_;
    bool is_readonly_;
    
public:
    // Constructors
    ConfigValue() : value_(std::string("")), is_readonly_(false) {}
    
    template<typename T>
    ConfigValue(T&& value, const std::string& desc = "", bool readonly = false)
        : value_(std::forward<T>(value)), description_(desc), is_readonly_(readonly) {}
    
    // Type checking
    bool isBool() const { return std::holds_alternative<bool>(value_); }
    bool isInt() const { return std::holds_alternative<int>(value_); }
    bool isDouble() const { return std::holds_alternative<double>(value_); }
    bool isString() const { return std::holds_alternative<std::string>(value_); }
    bool isStringArray() const { return std::holds_alternative<std::vector<std::string>>(value_); }
    
    // Value access with type safety
    template<typename T>
    T get() const {
        if constexpr (std::is_same_v<T, bool>) {
            if (isBool()) return std::get<bool>(value_);
            if (isString()) {
                const std::string& str = std::get<std::string>(value_);
                std::string lower = str;
                std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
                return lower == "true" || lower == "1" || lower == "yes" || lower == "on";
            }
            if (isInt()) return std::get<int>(value_) != 0;
            if (isDouble()) return std::get<double>(value_) != 0.0;
        } else if constexpr (std::is_same_v<T, int>) {
            if (isInt()) return std::get<int>(value_);
            if (isDouble()) return static_cast<int>(std::get<double>(value_));
            if (isString()) return std::stoi(std::get<std::string>(value_));
            if (isBool()) return std::get<bool>(value_) ? 1 : 0;
        } else if constexpr (std::is_same_v<T, double>) {
            if (isDouble()) return std::get<double>(value_);
            if (isInt()) return static_cast<double>(std::get<int>(value_));
            if (isString()) return std::stod(std::get<std::string>(value_));
            if (isBool()) return std::get<bool>(value_) ? 1.0 : 0.0;
        } else if constexpr (std::is_same_v<T, std::string>) {
            if (isString()) return std::get<std::string>(value_);
            if (isBool()) return std::get<bool>(value_) ? "true" : "false";
            if (isInt()) return std::to_string(std::get<int>(value_));
            if (isDouble()) return std::to_string(std::get<double>(value_));
        } else if constexpr (std::is_same_v<T, std::vector<std::string>>) {
            if (isStringArray()) return std::get<std::vector<std::string>>(value_);
        }
        
        throw std::runtime_error("Cannot convert config value to requested type");
    }
    
    // Optional value access
    template<typename T>
    std::optional<T> tryGet() const {
        try {
            return get<T>();
        } catch (const std::exception&) {
            return std::nullopt;
        }
    }
    
    // Value assignment
    template<typename T>
    void set(T&& new_value) {
        if (is_readonly_) {
            throw std::runtime_error("Cannot modify read-only configuration value");
        }
        value_ = std::forward<T>(new_value);
    }
    
    // Properties
    const std::string& getDescription() const { return description_; }
    void setDescription(const std::string& desc) { description_ = desc; }
    
    bool isReadOnly() const { return is_readonly_; }
    void setReadOnly(bool readonly) { is_readonly_ = readonly; }
    
    // String representation
    std::string toString() const {
        return std::visit([](const auto& val) -> std::string {
            using T = std::decay_t<decltype(val)>;
            if constexpr (std::is_same_v<T, bool>) {
                return val ? "true" : "false";
            } else if constexpr (std::is_same_v<T, std::string>) {
                return val;
            } else if constexpr (std::is_same_v<T, std::vector<std::string>>) {
                std::string result = "[";
                for (size_t i = 0; i < val.size(); ++i) {
                    if (i > 0) result += ", ";
                    result += "\"" + val[i] + "\"";
                }
                result += "]";
                return result;
            } else {
                return std::to_string(val);
            }
        }, value_);
    }
    
    // Type name
    std::string getTypeName() const {
        return std::visit([](const auto& val) -> std::string {
            using T = std::decay_t<decltype(val)>;
            if constexpr (std::is_same_v<T, bool>) return "bool";
            else if constexpr (std::is_same_v<T, int>) return "int";
            else if constexpr (std::is_same_v<T, double>) return "double";
            else if constexpr (std::is_same_v<T, std::string>) return "string";
            else if constexpr (std::is_same_v<T, std::vector<std::string>>) return "string_array";
            else return "unknown";
        }, value_);
    }
};

// ===== CONFIGURATION SECTION =====

class ConfigSection {
private:
    std::unordered_map<std::string, ConfigValue> values_;
    std::string name_;
    std::string description_;
    mutable std::mutex mutex_;
    
public:
    explicit ConfigSection(const std::string& section_name = "", const std::string& desc = "")
        : name_(section_name), description_(desc) {}
    
    // Value access
    template<typename T>
    T get(const std::string& key, const T& default_value = T{}) const {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = values_.find(key);
        if (it != values_.end()) {
            auto opt_val = it->second.template tryGet<T>();
            if (opt_val.has_value()) {
                return opt_val.value();
            }
        }
        return default_value;
    }
    
    template<typename T>
    void set(const std::string& key, T&& value, const std::string& description = "", bool readonly = false) {
        std::lock_guard<std::mutex> lock(mutex_);
        values_[key] = ConfigValue(std::forward<T>(value), description, readonly);
    }
    
    // Check if key exists
    bool has(const std::string& key) const {
        std::lock_guard<std::mutex> lock(mutex_);
        return values_.find(key) != values_.end();
    }
    
    // Remove key
    bool remove(const std::string& key) {
        std::lock_guard<std::mutex> lock(mutex_);
        return values_.erase(key) > 0;
    }
    
    // Get all keys
    std::vector<std::string> getKeys() const {
        std::lock_guard<std::mutex> lock(mutex_);
        std::vector<std::string> keys;
        keys.reserve(values_.size());
        for (const auto& pair : values_) {
            keys.push_back(pair.first);
        }
        return keys;
    }
    
    // Get config value (for metadata access)
    std::optional<ConfigValue> getConfigValue(const std::string& key) const {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = values_.find(key);
        if (it != values_.end()) {
            return it->second;
        }
        return std::nullopt;
    }
    
    // Section properties
    const std::string& getName() const { return name_; }
    const std::string& getDescription() const { return description_; }
    void setDescription(const std::string& desc) { description_ = desc; }
    
    size_t size() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return values_.size();
    }
    
    bool empty() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return values_.empty();
    }
    
    // Clear all values
    void clear() {
        std::lock_guard<std::mutex> lock(mutex_);
        values_.clear();
    }
    
    // Merge another section
    void merge(const ConfigSection& other, bool overwrite = true) {
        std::lock_guard<std::mutex> lock(mutex_);
        std::lock_guard<std::mutex> other_lock(other.mutex_);
        
        for (const auto& pair : other.values_) {
            if (overwrite || values_.find(pair.first) == values_.end()) {
                values_[pair.first] = pair.second;
            }
        }
    }
    
    // Iterator support
    auto begin() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return values_.begin();
    }
    
    auto end() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return values_.end();
    }
};

// ===== CONFIGURATION MANAGER =====

class ConfigManager {
private:
    std::unordered_map<std::string, std::unique_ptr<ConfigSection>> sections_;
    std::vector<std::string> config_file_paths_;
    mutable std::mutex mutex_;
    bool auto_save_;
    std::string current_config_file_;
    
    // Validation callbacks
    std::unordered_map<std::string, std::function<bool(const ConfigValue&)>> validators_;
    
    // Change notifications
    std::unordered_map<std::string, std::vector<std::function<void(const std::string&, const ConfigValue&)>>> change_listeners_;
    
    // Singleton instance
    static std::unique_ptr<ConfigManager> instance_;
    static std::mutex instance_mutex_;
    
    ConfigManager() : auto_save_(false) {
        initializeDefaults();
    }
    
    void initializeDefaults();
    void notifyChange(const std::string& section_key, const ConfigValue& value);
    
public:
    // Singleton access
    static ConfigManager& getInstance() {
        std::lock_guard<std::mutex> lock(instance_mutex_);
        if (!instance_) {
            instance_ = std::unique_ptr<ConfigManager>(new ConfigManager());
        }
        return *instance_;
    }
    
    // Delete copy constructor and assignment
    ConfigManager(const ConfigManager&) = delete;
    ConfigManager& operator=(const ConfigManager&) = delete;
    
    // Section management
    ConfigSection& getSection(const std::string& section_name) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = sections_.find(section_name);
        if (it == sections_.end()) {
            sections_[section_name] = std::make_unique<ConfigSection>(section_name);
        }
        return *sections_[section_name];
    }
    
    bool hasSection(const std::string& section_name) const {
        std::lock_guard<std::mutex> lock(mutex_);
        return sections_.find(section_name) != sections_.end();
    }
    
    bool removeSection(const std::string& section_name) {
        std::lock_guard<std::mutex> lock(mutex_);
        return sections_.erase(section_name) > 0;
    }
    
    std::vector<std::string> getSectionNames() const {
        std::lock_guard<std::mutex> lock(mutex_);
        std::vector<std::string> names;
        names.reserve(sections_.size());
        for (const auto& pair : sections_) {
            names.push_back(pair.first);
        }
        return names;
    }
    
    // Convenience methods for direct value access
    template<typename T>
    T get(const std::string& section_name, const std::string& key, const T& default_value = T{}) {
        return getSection(section_name).template get<T>(key, default_value);
    }
    
    template<typename T>
    void set(const std::string& section_name, const std::string& key, T&& value, 
             const std::string& description = "", bool readonly = false) {
        getSection(section_name).set(key, std::forward<T>(value), description, readonly);
        
        // Validate if validator exists
        std::string full_key = section_name + "." + key;
        auto validator_it = validators_.find(full_key);
        if (validator_it != validators_.end()) {
            auto config_val = getSection(section_name).getConfigValue(key);
            if (config_val && !validator_it->second(*config_val)) {
                throw std::runtime_error("Configuration value validation failed for: " + full_key);
            }
        }
        
        // Notify change
        if (auto config_val = getSection(section_name).getConfigValue(key)) {
            notifyChange(full_key, *config_val);
        }
        
        // Auto-save if enabled
        if (auto_save_ && !current_config_file_.empty()) {
            saveToFile(current_config_file_);
        }
    }
    
    // File operations
    bool loadFromFile(const std::string& filename);
    bool saveToFile(const std::string& filename) const;
    bool loadFromJson(const std::string& json_content);
    std::string saveToJson() const;
    
    // Environment variable loading
    void loadFromEnvironment(const std::string& prefix = "CPPVERSEHUB_");
    
    // Command line arguments
    void loadFromCommandLine(int argc, char* argv[], const std::string& prefix = "--config-");
    
    // Auto-save functionality
    void setAutoSave(bool enabled, const std::string& filename = "") {
        auto_save_ = enabled;
        if (!filename.empty()) {
            current_config_file_ = filename;
        }
    }
    
    bool isAutoSaveEnabled() const { return auto_save_; }
    const std::string& getCurrentConfigFile() const { return current_config_file_; }
    
    // Validation
    void addValidator(const std::string& section_name, const std::string& key,
                     std::function<bool(const ConfigValue&)> validator) {
        std::string full_key = section_name + "." + key;
        validators_[full_key] = std::move(validator);
    }
    
    // Change notifications
    void addChangeListener(const std::string& section_name, const std::string& key,
                          std::function<void(const std::string&, const ConfigValue&)> listener) {
        std::string full_key = section_name + "." + key;
        change_listeners_[full_key].push_back(std::move(listener));
    }
    
    // Configuration paths
    void addConfigPath(const std::string& path) {
        config_file_paths_.push_back(path);
    }
    
    bool loadFromPaths() {
        for (const auto& path : config_file_paths_) {
            if (loadFromFile(path)) {
                current_config_file_ = path;
                return true;
            }
        }
        return false;
    }
    
    // Utility methods
    void clear() {
        std::lock_guard<std::mutex> lock(mutex_);
        sections_.clear();
        validators_.clear();
        change_listeners_.clear();
    }
    
    size_t getSectionCount() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return sections_.size();
    }
    
    // Debug and introspection
    void printConfiguration(std::ostream& os = std::cout) const;
    std::string getConfigurationReport() const;
    
    // Backup and restore
    std::string createBackup() const { return saveToJson(); }
    void restoreFromBackup(const std::string& backup_json) { loadFromJson(backup_json); }
    
    // Configuration merging
    void mergeFrom(const ConfigManager& other, bool overwrite = true);
    
    // Space game specific configurations
    void setupSpaceGameDefaults();
    void validateSpaceGameConfig();
};

// ===== CONFIGURATION PRESETS =====

namespace ConfigPresets {
    
    // Graphics settings preset
    struct GraphicsPreset {
        std::string name;
        int resolution_width;
        int resolution_height;
        bool fullscreen;
        int quality_level; // 1-5
        bool vsync;
        int max_fps;
        double render_scale;
    };
    
    // Audio settings preset
    struct AudioPreset {
        std::string name;
        double master_volume;
        double music_volume;
        double effects_volume;
        double voice_volume;
        bool surround_sound;
        std::string audio_device;
    };
    
    // Gameplay settings preset
    struct GameplayPreset {
        std::string name;
        std::string difficulty;
        bool auto_save;
        int save_frequency; // minutes
        bool pause_on_focus_loss;
        std::vector<std::string> enabled_mods;
    };
    
    // Network settings preset
    struct NetworkPreset {
        std::string name;
        std::string server_address;
        int server_port;
        int max_players;
        int timeout_seconds;
        bool use_compression;
        std::string encryption_level;
    };
    
    // Preset manager
    class PresetManager {
    private:
        std::vector<GraphicsPreset> graphics_presets_;
        std::vector<AudioPreset> audio_presets_;
        std::vector<GameplayPreset> gameplay_presets_;
        std::vector<NetworkPreset> network_presets_;
        
    public:
        PresetManager() { initializeDefaultPresets(); }
        
        void initializeDefaultPresets();
        
        // Apply presets to config manager
        void applyGraphicsPreset(const std::string& preset_name, ConfigManager& config);
        void applyAudioPreset(const std::string& preset_name, ConfigManager& config);
        void applyGameplayPreset(const std::string& preset_name, ConfigManager& config);
        void applyNetworkPreset(const std::string& preset_name, ConfigManager& config);
        
        // Get available presets
        std::vector<std::string> getGraphicsPresetNames() const;
        std::vector<std::string> getAudioPresetNames() const;
        std::vector<std::string> getGameplayPresetNames() const;
        std::vector<std::string> getNetworkPresetNames() const;
        
        // Custom preset creation
        void addGraphicsPreset(const GraphicsPreset& preset);
        void addAudioPreset(const AudioPreset& preset);
        void addGameplayPreset(const GameplayPreset& preset);
        void addNetworkPreset(const NetworkPreset& preset);
    };
    
} // namespace ConfigPresets

// ===== CONFIGURATION MACROS FOR CONVENIENCE =====

#define CONFIG_GET(section, key, default_val) \
    CppVerseHub::Utils::ConfigManager::getInstance().get(section, key, default_val)

#define CONFIG_SET(section, key, value) \
    CppVerseHub::Utils::ConfigManager::getInstance().set(section, key, value)

#define CONFIG_GET_BOOL(section, key, default_val) \
    CppVerseHub::Utils::ConfigManager::getInstance().get<bool>(section, key, default_val)

#define CONFIG_GET_INT(section, key, default_val) \
    CppVerseHub::Utils::ConfigManager::getInstance().get<int>(section, key, default_val)

#define CONFIG_GET_DOUBLE(section, key, default_val) \
    CppVerseHub::Utils::ConfigManager::getInstance().get<double>(section, key, default_val)

#define CONFIG_GET_STRING(section, key, default_val) \
    CppVerseHub::Utils::ConfigManager::getInstance().get<std::string>(section, key, default_val)

// ===== CONFIGURATION BUILDER PATTERN =====

class ConfigBuilder {
private:
    ConfigManager& config_;
    std::string current_section_;
    
public:
    explicit ConfigBuilder(ConfigManager& config = ConfigManager::getInstance()) 
        : config_(config) {}
    
    ConfigBuilder& section(const std::string& section_name) {
        current_section_ = section_name;
        return *this;
    }
    
    template<typename T>
    ConfigBuilder& set(const std::string& key, T&& value, const std::string& description = "", bool readonly = false) {
        if (current_section_.empty()) {
            throw std::runtime_error("No section specified in ConfigBuilder");
        }
        config_.set(current_section_, key, std::forward<T>(value), description, readonly);
        return *this;
    }
    
    ConfigBuilder& validator(const std::string& key, std::function<bool(const ConfigValue&)> validator_func) {
        if (current_section_.empty()) {
            throw std::runtime_error("No section specified in ConfigBuilder");
        }
        config_.addValidator(current_section_, key, std::move(validator_func));
        return *this;
    }
    
    ConfigBuilder& onChange(const std::string& key, 
                           std::function<void(const std::string&, const ConfigValue&)> listener) {
        if (current_section_.empty()) {
            throw std::runtime_error("No section specified in ConfigBuilder");
        }
        config_.addChangeListener(current_section_, key, std::move(listener));
        return *this;
    }
    
    ConfigBuilder& loadFile(const std::string& filename) {
        config_.loadFromFile(filename);
        return *this;
    }
    
    ConfigBuilder& autoSave(const std::string& filename) {
        config_.setAutoSave(true, filename);
        return *this;
    }
};

} // namespace CppVerseHub::Utils