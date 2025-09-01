// File: src/utils/ConfigManager.cpp
// Application Configuration Management Implementation

#include "ConfigManager.hpp"
#include "FileParser.hpp"
#include <filesystem>
#include <cstdlib>
#include <algorithm>
#include <sstream>

namespace CppVerseHub::Utils {

// Static member definitions
std::unique_ptr<ConfigManager> ConfigManager::instance_ = nullptr;
std::mutex ConfigManager::instance_mutex_;

// ===== CONFIG MANAGER IMPLEMENTATION =====

void ConfigManager::initializeDefaults() {
    // Application settings
    auto& app_section = getSection("Application");
    app_section.set("name", std::string("CppVerseHub Space Game"), "Application name", true);
    app_section.set("version", std::string("1.0.0"), "Application version", true);
    app_section.set("build_date", std::string(__DATE__), "Build date", true);
    app_section.set("debug_mode", false, "Enable debug mode");
    app_section.set("log_level", std::string("INFO"), "Logging level");
    
    // Graphics settings
    auto& graphics_section = getSection("Graphics");
    graphics_section.set("resolution_width", 1920, "Screen width in pixels");
    graphics_section.set("resolution_height", 1080, "Screen height in pixels");
    graphics_section.set("fullscreen", false, "Enable fullscreen mode");
    graphics_section.set("vsync", true, "Enable vertical sync");
    graphics_section.set("max_fps", 60, "Maximum frames per second");
    graphics_section.set("quality_level", 3, "Graphics quality (1-5)");
    graphics_section.set("render_scale", 1.0, "Render scale multiplier");
    
    // Audio settings
    auto& audio_section = getSection("Audio");
    audio_section.set("master_volume", 0.8, "Master volume (0.0-1.0)");
    audio_section.set("music_volume", 0.7, "Music volume (0.0-1.0)");
    audio_section.set("effects_volume", 0.9, "Sound effects volume (0.0-1.0)");
    audio_section.set("voice_volume", 1.0, "Voice volume (0.0-1.0)");
    audio_section.set("surround_sound", false, "Enable surround sound");
    
    // Gameplay settings
    auto& gameplay_section = getSection("Gameplay");
    gameplay_section.set("difficulty", std::string("Normal"), "Game difficulty");
    gameplay_section.set("auto_save", true, "Enable automatic saving");
    gameplay_section.set("save_frequency", 5, "Auto-save frequency in minutes");
    gameplay_section.set("pause_on_focus_loss", true, "Pause when window loses focus");
    
    // Network settings
    auto& network_section = getSection("Network");
    network_section.set("server_address", std::string("localhost"), "Server address");
    network_section.set("server_port", 9999, "Server port");
    network_section.set("max_players", 8, "Maximum players in multiplayer");
    network_section.set("timeout_seconds", 30, "Network timeout in seconds");
    network_section.set("use_compression", true, "Enable network compression");
    
    // Input settings
    auto& input_section = getSection("Input");
    input_section.set("mouse_sensitivity", 1.0, "Mouse sensitivity multiplier");
    input_section.set("invert_y_axis", false, "Invert Y-axis for mouse/controller");
    input_section.set("key_repeat_delay", 250, "Key repeat delay in milliseconds");
    input_section.set("key_repeat_rate", 50, "Key repeat rate in milliseconds");
}

void ConfigManager::notifyChange(const std::string& section_key, const ConfigValue& value) {
    auto it = change_listeners_.find(section_key);
    if (it != change_listeners_.end()) {
        for (const auto& listener : it->second) {
            try {
                listener(section_key, value);
            } catch (const std::exception& e) {
                std::cerr << "Error in config change listener: " << e.what() << std::endl;
            }
        }
    }
}

bool ConfigManager::loadFromFile(const std::string& filename) {
    try {
        if (!std::filesystem::exists(filename)) {
            return false;
        }
        
        // Determine file format
        std::string extension = filename.substr(filename.find_last_of('.') + 1);
        std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
        
        if (extension == "json") {
            JsonValue root = JsonParser::parseFromFile(filename);
            return loadFromJsonValue(root);
        } else if (extension == "ini" || extension == "cfg" || extension == "conf") {
            return loadFromIniFile(filename);
        } else {
            // Try JSON first, then INI
            try {
                JsonValue root = JsonParser::parseFromFile(filename);
                return loadFromJsonValue(root);
            } catch (const JsonParseException&) {
                return loadFromIniFile(filename);
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Error loading config file '" << filename << "': " << e.what() << std::endl;
        return false;
    }
}

bool ConfigManager::loadFromJsonValue(const JsonValue& root) {
    if (!root.isObject()) {
        return false;
    }
    
    const auto& root_obj = root.asObject();
    for (const auto& section_pair : root_obj) {
        const std::string& section_name = section_pair.first;
        const JsonValue& section_value = section_pair.second;
        
        if (!section_value.isObject()) continue;
        
        auto& section = getSection(section_name);
        const auto& section_obj = section_value.asObject();
        
        for (const auto& value_pair : section_obj) {
            const std::string& key = value_pair.first;
            const JsonValue& value = value_pair.second;
            
            try {
                if (value.isBool()) {
                    section.set(key, value.asBool());
                } else if (value.isNumber()) {
                    double num = value.asNumber();
                    if (num == static_cast<int>(num)) {
                        section.set(key, static_cast<int>(num));
                    } else {
                        section.set(key, num);
                    }
                } else if (value.isString()) {
                    section.set(key, value.asString());
                } else if (value.isArray()) {
                    std::vector<std::string> string_array;
                    for (size_t i = 0; i < value.size(); ++i) {
                        if (value[i].isString()) {
                            string_array.push_back(value[i].asString());
                        }
                    }
                    if (!string_array.empty()) {
                        section.set(key, string_array);
                    }
                }
            } catch (const std::exception& e) {
                std::cerr << "Error loading config value [" << section_name << "]." << key << ": " << e.what() << std::endl;
            }
        }
    }
    
    return true;
}

bool ConfigManager::loadFromIniFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        return false;
    }
    
    std::string line;
    std::string current_section = "General";
    size_t line_number = 0;
    
    while (std::getline(file, line)) {
        ++line_number;
        
        // Trim whitespace
        line.erase(0, line.find_first_not_of(" \t"));
        line.erase(line.find_last_not_of(" \t") + 1);
        
        // Skip empty lines and comments
        if (line.empty() || line[0] == '#' || line[0] == ';') {
            continue;
        }
        
        // Section header
        if (line[0] == '[' && line.back() == ']') {
            current_section = line.substr(1, line.length() - 2);
            continue;
        }
        
        // Key-value pair
        size_t equals_pos = line.find('=');
        if (equals_pos != std::string::npos) {
            std::string key = line.substr(0, equals_pos);
            std::string value = line.substr(equals_pos + 1);
            
            // Trim key and value
            key.erase(0, key.find_first_not_of(" \t"));
            key.erase(key.find_last_not_of(" \t") + 1);
            value.erase(0, value.find_first_not_of(" \t"));
            value.erase(value.find_last_not_of(" \t") + 1);
            
            // Remove quotes if present
            if (value.length() >= 2 && value[0] == '"' && value.back() == '"') {
                value = value.substr(1, value.length() - 2);
            }
            
            try {
                auto& section = getSection(current_section);
                
                // Try to parse as different types
                if (value == "true" || value == "false") {
                    section.set(key, value == "true");
                } else if (value.find('.') != std::string::npos) {
                    // Try as double
                    try {
                        double d = std::stod(value);
                        section.set(key, d);
                    } catch (const std::exception&) {
                        section.set(key, value); // Store as string
                    }
                } else {
                    // Try as int
                    try {
                        int i = std::stoi(value);
                        section.set(key, i);
                    } catch (const std::exception&) {
                        section.set(key, value); // Store as string
                    }
                }
            } catch (const std::exception& e) {
                std::cerr << "Error parsing INI file line " << line_number << ": " << e.what() << std::endl;
            }
        }
    }
    
    return true;
}

bool ConfigManager::saveToFile(const std::string& filename) const {
    try {
        // Determine format from extension
        std::string extension = filename.substr(filename.find_last_of('.') + 1);
        std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
        
        if (extension == "json") {
            std::ofstream file(filename);
            if (!file.is_open()) {
                return false;
            }
            file << saveToJson();
            return true;
        } else {
            return saveToIniFile(filename);
        }
    } catch (const std::exception& e) {
        std::cerr << "Error saving config file '" << filename << "': " << e.what() << std::endl;
        return false;
    }
}

std::string ConfigManager::saveToJson() const {
    JsonValue root(JsonValue::JsonObject{});
    
    std::lock_guard<std::mutex> lock(mutex_);
    for (const auto& section_pair : sections_) {
        const std::string& section_name = section_pair.first;
        const auto& section = section_pair.second;
        
        JsonValue section_obj(JsonValue::JsonObject{});
        
        for (const auto& key : section->getKeys()) {
            auto config_value = section->getConfigValue(key);
            if (!config_value) continue;
            
            try {
                if (config_value->isBool()) {
                    section_obj[key] = config_value->get<bool>();
                } else if (config_value->isInt()) {
                    section_obj[key] = config_value->get<int>();
                } else if (config_value->isDouble()) {
                    section_obj[key] = config_value->get<double>();
                } else if (config_value->isString()) {
                    section_obj[key] = config_value->get<std::string>();
                } else if (config_value->isStringArray()) {
                    JsonValue array_val(JsonValue::JsonArray{});
                    auto string_array = config_value->get<std::vector<std::string>>();
                    for (const auto& str : string_array) {
                        array_val.asArray().push_back(JsonValue(str));
                    }
                    section_obj[key] = array_val;
                }
            } catch (const std::exception& e) {
                std::cerr << "Error serializing config value [" << section_name << "]." << key << ": " << e.what() << std::endl;
            }
        }
        
        root[section_name] = section_obj;
    }
    
    return root.toString(true);
}

bool ConfigManager::saveToIniFile(const std::string& filename) const {
    std::ofstream file(filename);
    if (!file.is_open()) {
        return false;
    }
    
    file << "# CppVerseHub Configuration File" << std::endl;
    file << "# Generated on " << std::chrono::system_clock::now() << std::endl;
    file << std::endl;
    
    std::lock_guard<std::mutex> lock(mutex_);
    for (const auto& section_pair : sections_) {
        const std::string& section_name = section_pair.first;
        const auto& section = section_pair.second;
        
        if (section->empty()) continue;
        
        file << "[" << section_name << "]" << std::endl;
        
        for (const auto& key : section->getKeys()) {
            auto config_value = section->getConfigValue(key);
            if (!config_value) continue;
            
            std::string description = config_value->getDescription();
            if (!description.empty()) {
                file << "# " << description << std::endl;
            }
            
            file << key << " = ";
            
            try {
                if (config_value->isString()) {
                    std::string str_val = config_value->get<std::string>();
                    if (str_val.find(' ') != std::string::npos || str_val.empty()) {
                        file << "\"" << str_val << "\"";
                    } else {
                        file << str_val;
                    }
                } else {
                    file << config_value->toString();
                }
            } catch (const std::exception& e) {
                file << "# ERROR: " << e.what();
            }
            
            file << std::endl;
        }
        
        file << std::endl;
    }
    
    return true;
}

bool ConfigManager::loadFromJson(const std::string& json_content) {
    try {
        JsonValue root = JsonParser::parseFromString(json_content);
        return loadFromJsonValue(root);
    } catch (const std::exception& e) {
        std::cerr << "Error loading JSON config: " << e.what() << std::endl;
        return false;
    }
}

void ConfigManager::loadFromEnvironment(const std::string& prefix) {
    // Get all environment variables with the given prefix
    for (char** env = environ; *env != nullptr; ++env) {
        std::string env_var(*env);
        size_t equals_pos = env_var.find('=');
        
        if (equals_pos != std::string::npos) {
            std::string name = env_var.substr(0, equals_pos);
            std::string value = env_var.substr(equals_pos + 1);
            
            if (name.substr(0, prefix.length()) == prefix) {
                std::string config_key = name.substr(prefix.length());
                
                // Convert SECTION_KEY to section.key format
                std::transform(config_key.begin(), config_key.end(), config_key.begin(), ::tolower);
                size_t underscore_pos = config_key.find('_');
                
                if (underscore_pos != std::string::npos) {
                    std::string section_name = config_key.substr(0, underscore_pos);
                    std::string key_name = config_key.substr(underscore_pos + 1);
                    
                    // Replace remaining underscores with dots in key name
                    std::replace(key_name.begin(), key_name.end(), '_', '.');
                    
                    auto& section = getSection(section_name);
                    section.set(key_name, value, "From environment variable: " + name);
                }
            }
        }
    }
}

void ConfigManager::loadFromCommandLine(int argc, char* argv[], const std::string& prefix) {
    for (int i = 1; i < argc; ++i) {
        std::string arg(argv[i]);
        
        if (arg.substr(0, prefix.length()) == prefix) {
            std::string config_part = arg.substr(prefix.length());
            size_t equals_pos = config_part.find('=');
            
            if (equals_pos != std::string::npos) {
                std::string key = config_part.substr(0, equals_pos);
                std::string value = config_part.substr(equals_pos + 1);
                
                // Parse section.key format
                size_t dot_pos = key.find('.');
                if (dot_pos != std::string::npos) {
                    std::string section_name = key.substr(0, dot_pos);
                    std::string key_name = key.substr(dot_pos + 1);
                    
                    auto& section = getSection(section_name);
                    section.set(key_name, value, "From command line: " + arg);
                }
            }
        }
    }
}

void ConfigManager::printConfiguration(std::ostream& os) const {
    os << "=== Configuration Report ===" << std::endl;
    
    std::lock_guard<std::mutex> lock(mutex_);
    for (const auto& section_pair : sections_) {
        const std::string& section_name = section_pair.first;
        const auto& section = section_pair.second;
        
        if (section->empty()) continue;
        
        os << std::endl << "[" << section_name << "]";
        if (!section->getDescription().empty()) {
            os << " - " << section->getDescription();
        }
        os << std::endl;
        
        for (const auto& key : section->getKeys()) {
            auto config_value = section->getConfigValue(key);
            if (!config_value) continue;
            
            os << "  " << key << " = " << config_value->toString();
            os << " (" << config_value->getTypeName() << ")";
            
            if (config_value->isReadOnly()) {
                os << " [READ-ONLY]";
            }
            
            if (!config_value->getDescription().empty()) {
                os << " # " << config_value->getDescription();
            }
            
            os << std::endl;
        }
    }
    
    os << std::endl << "Total sections: " << sections_.size() << std::endl;
}

std::string ConfigManager::getConfigurationReport() const {
    std::ostringstream oss;
    printConfiguration(oss);
    return oss.str();
}

void ConfigManager::mergeFrom(const ConfigManager& other, bool overwrite) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::lock_guard<std::mutex> other_lock(other.mutex_);
    
    for (const auto& section_pair : other.sections_) {
        const std::string& section_name = section_pair.first;
        const auto& other_section = section_pair.second;
        
        auto& our_section = getSection(section_name);
        our_section.merge(*other_section, overwrite);
    }
}

void ConfigManager::setupSpaceGameDefaults() {
    // Space-specific game settings
    auto& space_section = getSection("SpaceGame");
    space_section.set("max_planets", 1000, "Maximum number of planets in the universe");
    space_section.set("max_fleets", 100, "Maximum number of fleets per player");
    space_section.set("max_missions", 50, "Maximum concurrent missions");
    space_section.set("galaxy_size", 10000.0, "Size of the galaxy in light years");
    space_section.set("time_scale", 1.0, "Game time scale multiplier");
    space_section.set("resource_regeneration", true, "Enable resource regeneration");
    space_section.set("random_events", true, "Enable random events");
    
    // Physics settings
    auto& physics_section = getSection("Physics");
    physics_section.set("gravity_constant", 6.67430e-11, "Gravitational constant");
    physics_section.set("light_speed", 299792458.0, "Speed of light in m/s");
    physics_section.set("time_step", 0.016667, "Physics simulation time step");
    physics_section.set("collision_detection", true, "Enable collision detection");
    
    // AI settings
    auto& ai_section = getSection("AI");
    ai_section.set("difficulty_modifier", 1.0, "AI difficulty multiplier");
    ai_section.set("update_frequency", 1.0, "AI update frequency in Hz");
    ai_section.set("pathfinding_algorithm", std::string("A*"), "Pathfinding algorithm");
    ai_section.set("decision_tree_depth", 5, "Maximum AI decision tree depth");
    
    // Add validators for space game settings
    addValidator("SpaceGame", "max_planets", [](const ConfigValue& val) {
        int planets = val.get<int>();
        return planets > 0 && planets <= 100000;
    });
    
    addValidator("SpaceGame", "galaxy_size", [](const ConfigValue& val) {
        double size = val.get<double>();
        return size > 0.0 && size <= 1000000.0;
    });
    
    addValidator("Physics", "time_step", [](const ConfigValue& val) {
        double step = val.get<double>();
        return step > 0.0 && step <= 1.0;
    });
}

void ConfigManager::validateSpaceGameConfig() {
    bool valid = true;
    std::ostringstream errors;
    
    // Validate graphics settings
    int width = get<int>("Graphics", "resolution_width", 1920);
    int height = get<int>("Graphics", "resolution_height", 1080);
    if (width < 640 || height < 480) {
        errors << "Invalid resolution: " << width << "x" << height << std::endl;
        valid = false;
    }
    
    // Validate audio settings
    double master_vol = get<double>("Audio", "master_volume", 0.8);
    if (master_vol < 0.0 || master_vol > 1.0) {
        errors << "Invalid master volume: " << master_vol << std::endl;
        valid = false;
    }
    
    // Validate space game settings
    int max_planets = get<int>("SpaceGame", "max_planets", 1000);
    if (max_planets <= 0 || max_planets > 100000) {
        errors << "Invalid max_planets: " << max_planets << std::endl;
        valid = false;
    }
    
    if (!valid) {
        throw std::runtime_error("Configuration validation failed:\n" + errors.str());
    }
}

// ===== CONFIG PRESETS IMPLEMENTATION =====

namespace ConfigPresets {
    
    void PresetManager::initializeDefaultPresets() {
        // Graphics presets
        graphics_presets_ = {
            {"Low", 1280, 720, false, 1, false, 30, 0.8},
            {"Medium", 1920, 1080, false, 3, true, 60, 1.0},
            {"High", 2560, 1440, false, 4, true, 75, 1.0},
            {"Ultra", 3840, 2160, true, 5, true, 120, 1.2}
        };
        
        // Audio presets
        audio_presets_ = {
            {"Quiet", 0.3, 0.2, 0.4, 0.5, false, "default"},
            {"Normal", 0.8, 0.7, 0.9, 1.0, false, "default"},
            {"Loud", 1.0, 0.9, 1.0, 1.0, true, "default"}
        };
        
        // Gameplay presets
        gameplay_presets_ = {
            {"Easy", "Easy", true, 2, true, {}},
            {"Normal", "Normal", true, 5, true, {}},
            {"Hard", "Hard", true, 10, false, {}},
            {"Expert", "Expert", false, 0, false, {}}
        };
        
        // Network presets
        network_presets_ = {
            {"Local", "localhost", 9999, 4, 10, false, "none"},
            {"LAN", "192.168.1.100", 9999, 8, 30, true, "basic"},
            {"Internet", "server.example.com", 9999, 16, 60, true, "full"}
        };
    }
    
    void PresetManager::applyGraphicsPreset(const std::string& preset_name, ConfigManager& config) {
        auto it = std::find_if(graphics_presets_.begin(), graphics_presets_.end(),
                              [&preset_name](const GraphicsPreset& preset) {
                                  return preset.name == preset_name;
                              });
        
        if (it != graphics_presets_.end()) {
            auto& graphics = config.getSection("Graphics");
            graphics.set("resolution_width", it->resolution_width);
            graphics.set("resolution_height", it->resolution_height);
            graphics.set("fullscreen", it->fullscreen);
            graphics.set("quality_level", it->quality_level);
            graphics.set("vsync", it->vsync);
            graphics.set("max_fps", it->max_fps);
            graphics.set("render_scale", it->render_scale);
        }
    }
    
    void PresetManager::applyAudioPreset(const std::string& preset_name, ConfigManager& config) {
        auto it = std::find_if(audio_presets_.begin(), audio_presets_.end(),
                              [&preset_name](const AudioPreset& preset) {
                                  return preset.name == preset_name;
                              });
        
        if (it != audio_presets_.end()) {
            auto& audio = config.getSection("Audio");
            audio.set("master_volume", it->master_volume);
            audio.set("music_volume", it->music_volume);
            audio.set("effects_volume", it->effects_volume);
            audio.set("voice_volume", it->voice_volume);
            audio.set("surround_sound", it->surround_sound);
            audio.set("audio_device", it->audio_device);
        }
    }
    
    std::vector<std::string> PresetManager::getGraphicsPresetNames() const {
        std::vector<std::string> names;
        for (const auto& preset : graphics_presets_) {
            names.push_back(preset.name);
        }
        return names;
    }
    
    std::vector<std::string> PresetManager::getAudioPresetNames() const {
        std::vector<std::string> names;
        for (const auto& preset : audio_presets_) {
            names.push_back(preset.name);
        }
        return names;
    }
    
} // namespace ConfigPresets

// ===== DEMONSTRATION FUNCTIONS =====

void demonstrateConfigManager() {
    std::cout << "\n=== Configuration Manager Demonstration ===" << std::endl;
    
    auto& config = ConfigManager::getInstance();
    
    // Set up space game defaults
    config.setupSpaceGameDefaults();
    
    // Demonstrate basic usage
    config.set("Graphics", "resolution_width", 2560);
    config.set("Graphics", "resolution_height", 1440);
    config.set("Graphics", "fullscreen", false);
    
    config.set("Audio", "master_volume", 0.85);
    config.set("Gameplay", "difficulty", std::string("Hard"));
    
    // Demonstrate type-safe access
    int width = config.get<int>("Graphics", "resolution_width", 1920);
    bool fullscreen = config.get<bool>("Graphics", "fullscreen", false);
    std::string difficulty = config.get<std::string>("Gameplay", "difficulty", "Normal");
    
    std::cout << "Resolution: " << width << "x" << config.get<int>("Graphics", "resolution_height", 1080) << std::endl;
    std::cout << "Fullscreen: " << (fullscreen ? "Yes" : "No") << std::endl;
    std::cout << "Difficulty: " << difficulty << std::endl;
    
    // Demonstrate validators
    config.addValidator("Graphics", "resolution_width", [](const ConfigValue& val) {
        int w = val.get<int>();
        return w >= 640 && w <= 7680;
    });
    
    try {
        config.set("Graphics", "resolution_width", 100); // Should fail validation
    } catch (const std::exception& e) {
        std::cout << "Validation caught invalid value: " << e.what() << std::endl;
    }
    
    // Demonstrate change listeners
    config.addChangeListener("Audio", "master_volume", 
        [](const std::string& key, const ConfigValue& value) {
            std::cout << "Volume changed to: " << value.toString() << std::endl;
        });
    
    config.set("Audio", "master_volume", 0.9); // Should trigger listener
    
    // Demonstrate configuration builder
    ConfigBuilder(config)
        .section("TestSection")
        .set("test_value", 42, "A test integer value")
        .set("test_string", std::string("Hello World"), "A test string value")
        .validator("test_value", [](const ConfigValue& val) {
            return val.get<int>() > 0;
        });
    
    // Print current configuration
    std::cout << "\nCurrent Configuration:" << std::endl;
    config.printConfiguration();
    
    // Demonstrate save/load
    std::string config_backup = config.saveToJson();
    std::cout << "\nConfiguration backup created (" << config_backup.length() << " characters)" << std::endl;
    
    // Save to file
    config.saveToFile("demo_config.json");
    std::cout << "Configuration saved to demo_config.json" << std::endl;
    
    // Clean up
    std::remove("demo_config.json");
}

} // namespace CppVerseHub::Utils