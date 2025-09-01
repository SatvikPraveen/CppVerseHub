// File: src/utils/FileParser.cpp
// JSON, CSV, XML Parsing Implementation

#include "FileParser.hpp"
#include <iostream>
#include <iomanip>

namespace CppVerseHub::Utils {

// ===== JSON VALUE IMPLEMENTATION =====

std::string JsonValue::toString(bool pretty, int indent) const {
    std::ostringstream oss;
    std::string indent_str = pretty ? std::string(indent * 2, ' ') : "";
    std::string newline = pretty ? "\n" : "";
    
    switch (type_) {
        case Type::Null:
            oss << "null";
            break;
            
        case Type::Boolean:
            oss << (std::get<bool>(value_) ? "true" : "false");
            break;
            
        case Type::Number: {
            double num = std::get<double>(value_);
            if (num == static_cast<int>(num)) {
                oss << static_cast<int>(num);
            } else {
                oss << std::fixed << std::setprecision(6) << num;
                // Remove trailing zeros
                std::string str = oss.str();
                str.erase(str.find_last_not_of('0') + 1);
                str.erase(str.find_last_not_of('.') + 1);
                oss.str("");
                oss << str;
            }
            break;
        }
        
        case Type::String:
            oss << "\"";
            for (char c : std::get<std::string>(value_)) {
                switch (c) {
                    case '"': oss << "\\\""; break;
                    case '\\': oss << "\\\\"; break;
                    case '\b': oss << "\\b"; break;
                    case '\f': oss << "\\f"; break;
                    case '\n': oss << "\\n"; break;
                    case '\r': oss << "\\r"; break;
                    case '\t': oss << "\\t"; break;
                    default:
                        if (c < 0x20) {
                            oss << "\\u" << std::hex << std::setw(4) << std::setfill('0') << static_cast<int>(c);
                        } else {
                            oss << c;
                        }
                        break;
                }
            }
            oss << "\"";
            break;
            
        case Type::Array: {
            const auto& arr = std::get<JsonArray>(value_);
            oss << "[";
            
            if (pretty && !arr.empty()) {
                oss << newline;
            }
            
            for (size_t i = 0; i < arr.size(); ++i) {
                if (i > 0) {
                    oss << ",";
                    if (pretty) oss << newline;
                }
                
                if (pretty) {
                    oss << std::string((indent + 1) * 2, ' ');
                }
                
                oss << arr[i].toString(pretty, indent + 1);
            }
            
            if (pretty && !arr.empty()) {
                oss << newline << indent_str;
            }
            
            oss << "]";
            break;
        }
        
        case Type::Object: {
            const auto& obj = std::get<JsonObject>(value_);
            oss << "{";
            
            if (pretty && !obj.empty()) {
                oss << newline;
            }
            
            bool first = true;
            for (const auto& pair : obj) {
                if (!first) {
                    oss << ",";
                    if (pretty) oss << newline;
                } else {
                    first = false;
                }
                
                if (pretty) {
                    oss << std::string((indent + 1) * 2, ' ');
                }
                
                oss << "\"" << pair.first << "\":" << (pretty ? " " : "");
                oss << pair.second.toString(pretty, indent + 1);
            }
            
            if (pretty && !obj.empty()) {
                oss << newline << indent_str;
            }
            
            oss << "}";
            break;
        }
    }
    
    return oss.str();
}

// ===== JSON PARSER IMPLEMENTATION =====

JsonValue JsonParser::parseValue() {
    skipWhitespace();
    
    char c = peek();
    switch (c) {
        case 'n':
            if (json_.substr(pos_, 4) == "null") {
                pos_ += 4;
                return JsonValue(nullptr);
            }
            throw JsonParseException("Invalid literal", line_, column_);
            
        case 't':
            if (json_.substr(pos_, 4) == "true") {
                pos_ += 4;
                return JsonValue(true);
            }
            throw JsonParseException("Invalid literal", line_, column_);
            
        case 'f':
            if (json_.substr(pos_, 5) == "false") {
                pos_ += 5;
                return JsonValue(false);
            }
            throw JsonParseException("Invalid literal", line_, column_);
            
        case '"':
            return JsonValue(parseString());
            
        case '[':
            return parseArray();
            
        case '{':
            return parseObject();
            
        case '-':
        case '0': case '1': case '2': case '3': case '4':
        case '5': case '6': case '7': case '8': case '9':
            return JsonValue(parseNumber());
            
        default:
            throw JsonParseException("Unexpected character: " + std::string(1, c), line_, column_);
    }
}

JsonValue JsonParser::parseArray() {
    if (consume() != '[') {
        throw JsonParseException("Expected '['", line_, column_);
    }
    
    JsonValue::JsonArray array;
    skipWhitespace();
    
    // Handle empty array
    if (peek() == ']') {
        consume();
        return JsonValue(array);
    }
    
    while (true) {
        array.push_back(parseValue());
        
        skipWhitespace();
        char c = peek();
        
        if (c == ']') {
            consume();
            break;
        } else if (c == ',') {
            consume();
            skipWhitespace();
        } else {
            throw JsonParseException("Expected ',' or ']' in array", line_, column_);
        }
    }
    
    return JsonValue(array);
}

JsonValue JsonParser::parseObject() {
    if (consume() != '{') {
        throw JsonParseException("Expected '{'", line_, column_);
    }
    
    JsonValue::JsonObject object;
    skipWhitespace();
    
    // Handle empty object
    if (peek() == '}') {
        consume();
        return JsonValue(object);
    }
    
    while (true) {
        skipWhitespace();
        if (peek() != '"') {
            throw JsonParseException("Expected string key in object", line_, column_);
        }
        
        std::string key = parseString();
        
        skipWhitespace();
        if (consume() != ':') {
            throw JsonParseException("Expected ':' after key in object", line_, column_);
        }
        
        JsonValue value = parseValue();
        object[key] = value;
        
        skipWhitespace();
        char c = peek();
        
        if (c == '}') {
            consume();
            break;
        } else if (c == ',') {
            consume();
        } else {
            throw JsonParseException("Expected ',' or '}' in object", line_, column_);
        }
    }
    
    return JsonValue(object);
}

// ===== DEMONSTRATION FUNCTIONS =====

void demonstrateJsonParser() {
    std::cout << "\n=== JSON Parser Demonstration ===" << std::endl;
    
    // Create sample JSON data
    std::string json_data = R"({
        "game_info": {
            "name": "CppVerseHub Space Game",
            "version": "1.0.0",
            "max_players": 8
        },
        "planets": [
            {
                "id": 1,
                "name": "Earth",
                "habitable": true,
                "population": 8000000000,
                "resources": ["Water", "Oxygen", "Iron"]
            },
            {
                "id": 2,
                "name": "Mars",
                "habitable": false,
                "population": 0,
                "resources": ["Iron", "Silicon", "Ice"]
            }
        ],
        "settings": {
            "difficulty": "Normal",
            "auto_save": true,
            "graphics_quality": 0.85
        }
    })";
    
    try {
        // Parse JSON
        JsonValue root = JsonParser::parseFromString(json_data);
        
        // Access nested values
        std::cout << "Game Name: " << root["game_info"]["name"].asString() << std::endl;
        std::cout << "Version: " << root["game_info"]["version"].asString() << std::endl;
        std::cout << "Max Players: " << root["game_info"]["max_players"].asInt() << std::endl;
        
        // Access array
        const auto& planets = root["planets"];
        std::cout << "\nPlanets (" << planets.size() << "):" << std::endl;
        
        for (size_t i = 0; i < planets.size(); ++i) {
            const auto& planet = planets[i];
            std::cout << "  - " << planet["name"].asString() 
                      << " (ID: " << planet["id"].asInt() << ")"
                      << " Habitable: " << (planet["habitable"].asBool() ? "Yes" : "No")
                      << " Population: " << static_cast<long long>(planet["population"].asNumber()) << std::endl;
            
            // Access resources array
            const auto& resources = planet["resources"];
            std::cout << "    Resources: ";
            for (size_t j = 0; j < resources.size(); ++j) {
                if (j > 0) std::cout << ", ";
                std::cout << resources[j].asString();
            }
            std::cout << std::endl;
        }
        
        // Access settings
        std::cout << "\nSettings:" << std::endl;
        std::cout << "  Difficulty: " << root["settings"]["difficulty"].asString() << std::endl;
        std::cout << "  Auto Save: " << (root["settings"]["auto_save"].asBool() ? "Enabled" : "Disabled") << std::endl;
        std::cout << "  Graphics Quality: " << root["settings"]["graphics_quality"].asNumber() << std::endl;
        
        // Demonstrate pretty printing
        std::cout << "\nPretty printed JSON:" << std::endl;
        std::cout << root.toString(true) << std::endl;
        
    } catch (const JsonParseException& e) {
        std::cerr << "JSON Parse Error: " << e.what() << std::endl;
    }
}

void demonstrateCsvParser() {
    std::cout << "\n=== CSV Parser Demonstration ===" << std::endl;
    
    // Create sample CSV data
    std::string csv_data = R"(fleet_id,commander,ship_count,fuel_level,mission_type,current_location
101,"Admiral Zhang",25,85.5,Exploration,"Alpha Centauri"
102,"Commander Rodriguez",12,92.0,Combat,"Sol System"
103,"Captain Singh",8,45.2,Colonization,"Kepler-442"
104,"Admiral Thompson",30,76.8,Trade,"Proxima Centauri"
105,"Commander Chen",15,20.1,Rescue,"Wolf 359"
)";
    
    try {
        // Parse CSV
        CsvData csv = CsvParser::parseFromString(csv_data);
        
        std::cout << "CSV Data Loaded:" << std::endl;
        std::cout << "Headers: ";
        for (const auto& header : csv.getHeaders()) {
            std::cout << header << " ";
        }
        std::cout << std::endl;
        std::cout << "Rows: " << csv.rowCount() << std::endl;
        std::cout << "Columns: " << csv.columnCount() << std::endl;
        
        // Display data
        std::cout << "\nFleet Data:" << std::endl;
        for (size_t i = 0; i < csv.rowCount(); ++i) {
            const auto& row = csv[i];
            std::cout << "  Fleet " << row.getFieldAs<int>(0) 
                      << ": " << row.getField(csv.getHeaders(), "commander")
                      << " (" << row.getField(csv.getHeaders(), "ship_count") << " ships)"
                      << " Fuel: " << row.getField(csv.getHeaders(), "fuel_level") << "%"
                      << " Mission: " << row.getField(csv.getHeaders(), "mission_type")
                      << " Location: " << row.getField(csv.getHeaders(), "current_location") << std::endl;
        }
        
        // Find specific data
        auto exploration_fleets = csv.findRows("mission_type", "Exploration");
        std::cout << "\nExploration fleets: " << exploration_fleets.size() << std::endl;
        for (const auto* fleet : exploration_fleets) {
            std::cout << "  - " << fleet->getField(csv.getHeaders(), "commander") << std::endl;
        }
        
        // Get column statistics
        auto fuel_levels = csv.getColumnAs<double>("fuel_level");
        if (!fuel_levels.empty()) {
            double avg_fuel = std::accumulate(fuel_levels.begin(), fuel_levels.end(), 0.0) / fuel_levels.size();
            std::cout << "\nAverage fuel level: " << std::fixed << std::setprecision(1) << avg_fuel << "%" << std::endl;
        }
        
        // Convert to JSON
        std::cout << "\nConverted to JSON:" << std::endl;
        std::cout << FileParserUtils::csvToJson(csv) << std::endl;
        
    } catch (const CsvParseException& e) {
        std::cerr << "CSV Parse Error: " << e.what() << std::endl;
    }
}

void demonstrateXmlParser() {
    std::cout << "\n=== XML Parser Demonstration ===" << std::endl;
    
    // Create sample XML data
    std::string xml_data = R"(<?xml version="1.0" encoding="UTF-8"?>
<missions>
    <mission id="1001" type="Exploration" priority="2">
        <title>Explore Alpha Centauri</title>
        <description>Survey the Alpha Centauri system for habitable planets and resources.</description>
        <objectives>
            <objective>Scan all planets in the system</objective>
            <objective>Identify potential colonization targets</objective>
            <objective>Map asteroid fields and resource deposits</objective>
        </objectives>
        <parameters>
            <parameter name="duration">30</parameter>
            <parameter name="required_ships">5</parameter>
            <parameter name="minimum_fuel">80</parameter>
        </parameters>
    </mission>
    <mission id="1002" type="Combat" priority="5">
        <title>Defend Earth</title>
        <description>Protect Earth from incoming hostile forces.</description>
        <objectives>
            <objective>Eliminate all hostile ships</objective>
            <objective>Prevent damage to Earth's orbital stations</objective>
        </objectives>
        <parameters>
            <parameter name="duration">10</parameter>
            <parameter name="required_ships">20</parameter>
            <parameter name="minimum_fuel">90</parameter>
        </parameters>
    </mission>
</missions>)";
    
    try {
        // Parse XML
        auto root = XmlParser::parseFromString(xml_data);
        
        std::cout << "XML Root Element: " << root->name << std::endl;
        std::cout << "Child Elements: " << root->childCount() << std::endl;
        
        // Process missions
        std::cout << "\nMissions:" << std::endl;
        for (const auto& mission_node : root->children) {
            if (mission_node->name != "mission") continue;
            
            std::string id = mission_node->getAttribute("id");
            std::string type = mission_node->getAttribute("type");
            std::string priority = mission_node->getAttribute("priority");
            
            std::cout << "  Mission " << id << " (" << type << ", Priority: " << priority << ")" << std::endl;
            
            // Get mission details
            auto title_node = mission_node->findChild("title");
            if (title_node) {
                std::cout << "    Title: " << title_node->content << std::endl;
            }
            
            auto desc_node = mission_node->findChild("description");
            if (desc_node) {
                std::cout << "    Description: " << desc_node->content << std::endl;
            }
            
            // Get objectives
            auto objectives_node = mission_node->findChild("objectives");
            if (objectives_node) {
                std::cout << "    Objectives:" << std::endl;
                for (const auto& obj_node : objectives_node->children) {
                    if (obj_node->name == "objective") {
                        std::cout << "      - " << obj_node->content << std::endl;
                    }
                }
            }
            
            // Get parameters
            auto params_node = mission_node->findChild("parameters");
            if (params_node) {
                std::cout << "    Parameters:" << std::endl;
                for (const auto& param_node : params_node->children) {
                    if (param_node->name == "parameter") {
                        std::string name = param_node->getAttribute("name");
                        std::cout << "      " << name << ": " << param_node->content << std::endl;
                    }
                }
            }
            
            std::cout << std::endl;
        }
        
        // Demonstrate XML serialization
        std::cout << "Reconstructed XML:" << std::endl;
        std::cout << root->toString() << std::endl;
        
    } catch (const XmlParseException& e) {
        std::cerr << "XML Parse Error: " << e.what() << std::endl;
    }
}

void demonstrateSpaceGameParsers() {
    std::cout << "\n=== Space Game Specific Parsers ===" << std::endl;
    
    // Demonstrate planet config parsing
    try {
        // Create sample planet configuration JSON
        std::string planets_json = R"([
            {
                "id": 1,
                "name": "Earth",
                "distance_from_star": 1.0,
                "population": 8000000000,
                "habitable": true,
                "resources": ["Water", "Oxygen", "Iron", "Silicon"],
                "orbital_parameters": {
                    "orbital_period": 365.25,
                    "orbital_velocity": 29.78,
                    "eccentricity": 0.0167
                }
            },
            {
                "id": 2,
                "name": "Mars",
                "distance_from_star": 1.52,
                "population": 0,
                "habitable": false,
                "resources": ["Iron", "Silicon", "Ice", "Carbon_Dioxide"],
                "orbital_parameters": {
                    "orbital_period": 686.98,
                    "orbital_velocity": 24.07,
                    "eccentricity": 0.0934
                }
            }
        ])";
        
        // Write to temporary file and parse
        std::ofstream temp_file("temp_planets.json");
        temp_file << planets_json;
        temp_file.close();
        
        auto planets = SpaceGameParsers::PlanetConfigParser::parseFromJson("temp_planets.json");
        
        std::cout << "Loaded " << planets.size() << " planet configurations:" << std::endl;
        for (const auto& planet : planets) {
            std::cout << "  " << planet.name << " (ID: " << planet.id << ")" << std::endl;
            std::cout << "    Distance: " << planet.distance_from_star << " AU" << std::endl;
            std::cout << "    Population: " << planet.population << std::endl;
            std::cout << "    Habitable: " << (planet.habitable ? "Yes" : "No") << std::endl;
            std::cout << "    Resources: ";
            for (size_t i = 0; i < planet.resources.size(); ++i) {
                if (i > 0) std::cout << ", ";
                std::cout << planet.resources[i];
            }
            std::cout << std::endl;
            
            if (!planet.orbital_parameters.empty()) {
                std::cout << "    Orbital Parameters:" << std::endl;
                for (const auto& param : planet.orbital_parameters) {
                    std::cout << "      " << param.first << ": " << param.second << std::endl;
                }
            }
            std::cout << std::endl;
        }
        
        // Clean up
        std::remove("temp_planets.json");
        
    } catch (const std::exception& e) {
        std::cerr << "Planet config parse error: " << e.what() << std::endl;
    }
}

void demonstrateFileUtilities() {
    std::cout << "\n=== File Parser Utilities ===" << std::endl;
    
    // Test format detection
    std::vector<std::pair<std::string, std::string>> test_files = {
        {"test.json", R"({"key": "value"})"},
        {"test.csv", "name,age,city\nJohn,30,New York"},
        {"test.xml", "<root><item>value</item></root>"}
    };
    
    for (const auto& [filename, content] : test_files) {
        // Write test file
        std::ofstream file(filename);
        file << content;
        file.close();
        
        // Detect format
        auto format = FileParserUtils::detectFormat(filename);
        std::string format_name;
        switch (format) {
            case FileParserUtils::FileFormat::JSON: format_name = "JSON"; break;
            case FileParserUtils::FileFormat::CSV: format_name = "CSV"; break;
            case FileParserUtils::FileFormat::XML: format_name = "XML"; break;
            default: format_name = "Unknown"; break;
        }
        
        std::cout << "File: " << filename << " -> Format: " << format_name << std::endl;
        
        // Get file info
        auto info = FileParserUtils::getFileInfo(filename);
        std::cout << "  Size: " << info.file_size << " bytes" << std::endl;
        
        // Validate content
        bool valid = false;
        switch (format) {
            case FileParserUtils::FileFormat::JSON:
                valid = FileParserUtils::isValidJson(content);
                break;
            case FileParserUtils::FileFormat::XML:
                valid = FileParserUtils::isValidXml(content);
                break;
            default:
                valid = true; // Assume CSV is valid
                break;
        }
        
        std::cout << "  Valid: " << (valid ? "Yes" : "No") << std::endl;
        
        // Clean up
        std::remove(filename.c_str());
    }
}

void performanceTest() {
    std::cout << "\n=== File Parser Performance Test ===" << std::endl;
    
    const int num_objects = 1000;
    
    // Generate large JSON
    std::ostringstream large_json;
    large_json << "[";
    for (int i = 0; i < num_objects; ++i) {
        if (i > 0) large_json << ",";
        large_json << R"({
            "id": )" << i << R"(,
            "name": "Object_)" << i << R"(",
            "active": )" << (i % 2 == 0 ? "true" : "false") << R"(,
            "value": )" << (i * 1.5) << R"(,
            "tags": ["tag1", "tag2", "tag3"]
        })";
    }
    large_json << "]";
    
    std::string json_str = large_json.str();
    
    // Test JSON parsing performance
    auto start_time = std::chrono::high_resolution_clock::now();
    
    try {
        JsonValue parsed = JsonParser::parseFromString(json_str);
        
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        
        std::cout << "Parsed " << parsed.size() << " JSON objects in " << duration.count() << " ms" << std::endl;
        std::cout << "JSON string size: " << json_str.length() << " characters" << std::endl;
        std::cout << "Performance: " << (parsed.size() * 1000 / duration.count()) << " objects/second" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Performance test error: " << e.what() << std::endl;
    }
}

} // namespace CppVerseHub::Utils