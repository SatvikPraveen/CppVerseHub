// File: src/utils/FileParser.hpp
// JSON, CSV, XML Parsing Utilities for Space Game

#pragma once

#include <string>
#include <vector>
#include <map>
#include <unordered_map>
#include <variant>
#include <optional>
#include <fstream>
#include <sstream>
#include <memory>
#include <stdexcept>
#include <algorithm>
#include <regex>

namespace CppVerseHub::Utils {

// ===== FORWARD DECLARATIONS =====

class JsonValue;
class CsvParser;
class XmlParser;

// ===== PARSING EXCEPTIONS =====

class ParseException : public std::runtime_error {
public:
    explicit ParseException(const std::string& message) : std::runtime_error(message) {}
};

class FileNotFoundException : public ParseException {
public:
    explicit FileNotFoundException(const std::string& filename) 
        : ParseException("File not found: " + filename) {}
};

class JsonParseException : public ParseException {
public:
    JsonParseException(const std::string& message, size_t line = 0, size_t column = 0)
        : ParseException(formatMessage(message, line, column)) {}
        
private:
    static std::string formatMessage(const std::string& message, size_t line, size_t column) {
        std::ostringstream oss;
        oss << "JSON Parse Error";
        if (line > 0 || column > 0) {
            oss << " at line " << line << ", column " << column;
        }
        oss << ": " << message;
        return oss.str();
    }
};

class CsvParseException : public ParseException {
public:
    CsvParseException(const std::string& message, size_t row = 0)
        : ParseException("CSV Parse Error" + (row > 0 ? " at row " + std::to_string(row) : "") + ": " + message) {}
};

class XmlParseException : public ParseException {
public:
    XmlParseException(const std::string& message, size_t line = 0)
        : ParseException("XML Parse Error" + (line > 0 ? " at line " + std::to_string(line) : "") + ": " + message) {}
};

// ===== JSON VALUE IMPLEMENTATION =====

class JsonValue {
public:
    enum class Type {
        Null,
        Boolean,
        Number,
        String,
        Array,
        Object
    };
    
private:
    using JsonObject = std::unordered_map<std::string, JsonValue>;
    using JsonArray = std::vector<JsonValue>;
    using ValueType = std::variant<std::nullptr_t, bool, double, std::string, JsonArray, JsonObject>;
    
    ValueType value_;
    Type type_;
    
public:
    // Constructors
    JsonValue() : value_(nullptr), type_(Type::Null) {}
    JsonValue(std::nullptr_t) : value_(nullptr), type_(Type::Null) {}
    JsonValue(bool val) : value_(val), type_(Type::Boolean) {}
    JsonValue(int val) : value_(static_cast<double>(val)), type_(Type::Number) {}
    JsonValue(double val) : value_(val), type_(Type::Number) {}
    JsonValue(const std::string& val) : value_(val), type_(Type::String) {}
    JsonValue(const char* val) : value_(std::string(val)), type_(Type::String) {}
    JsonValue(const JsonArray& val) : value_(val), type_(Type::Array) {}
    JsonValue(const JsonObject& val) : value_(val), type_(Type::Object) {}
    
    // Type checking
    Type getType() const { return type_; }
    bool isNull() const { return type_ == Type::Null; }
    bool isBool() const { return type_ == Type::Boolean; }
    bool isNumber() const { return type_ == Type::Number; }
    bool isString() const { return type_ == Type::String; }
    bool isArray() const { return type_ == Type::Array; }
    bool isObject() const { return type_ == Type::Object; }
    
    // Value access
    bool asBool() const {
        if (type_ != Type::Boolean) throw std::runtime_error("JsonValue is not a boolean");
        return std::get<bool>(value_);
    }
    
    double asNumber() const {
        if (type_ != Type::Number) throw std::runtime_error("JsonValue is not a number");
        return std::get<double>(value_);
    }
    
    int asInt() const {
        return static_cast<int>(asNumber());
    }
    
    const std::string& asString() const {
        if (type_ != Type::String) throw std::runtime_error("JsonValue is not a string");
        return std::get<std::string>(value_);
    }
    
    const JsonArray& asArray() const {
        if (type_ != Type::Array) throw std::runtime_error("JsonValue is not an array");
        return std::get<JsonArray>(value_);
    }
    
    JsonArray& asArray() {
        if (type_ != Type::Array) throw std::runtime_error("JsonValue is not an array");
        return std::get<JsonArray>(value_);
    }
    
    const JsonObject& asObject() const {
        if (type_ != Type::Object) throw std::runtime_error("JsonValue is not an object");
        return std::get<JsonObject>(value_);
    }
    
    JsonObject& asObject() {
        if (type_ != Type::Object) throw std::runtime_error("JsonValue is not an object");
        return std::get<JsonObject>(value_);
    }
    
    // Array/Object access operators
    JsonValue& operator[](size_t index) {
        if (type_ != Type::Array) throw std::runtime_error("JsonValue is not an array");
        auto& arr = std::get<JsonArray>(value_);
        if (index >= arr.size()) {
            arr.resize(index + 1);
        }
        return arr[index];
    }
    
    const JsonValue& operator[](size_t index) const {
        if (type_ != Type::Array) throw std::runtime_error("JsonValue is not an array");
        const auto& arr = std::get<JsonArray>(value_);
        if (index >= arr.size()) {
            throw std::out_of_range("Array index out of range");
        }
        return arr[index];
    }
    
    JsonValue& operator[](const std::string& key) {
        if (type_ == Type::Null) {
            *this = JsonValue(JsonObject{});
            type_ = Type::Object;
        }
        if (type_ != Type::Object) throw std::runtime_error("JsonValue is not an object");
        return std::get<JsonObject>(value_)[key];
    }
    
    const JsonValue& operator[](const std::string& key) const {
        if (type_ != Type::Object) throw std::runtime_error("JsonValue is not an object");
        const auto& obj = std::get<JsonObject>(value_);
        auto it = obj.find(key);
        if (it == obj.end()) {
            throw std::out_of_range("Object key not found: " + key);
        }
        return it->second;
    }
    
    // Utility methods
    bool contains(const std::string& key) const {
        if (type_ != Type::Object) return false;
        const auto& obj = std::get<JsonObject>(value_);
        return obj.find(key) != obj.end();
    }
    
    size_t size() const {
        switch (type_) {
            case Type::Array: return std::get<JsonArray>(value_).size();
            case Type::Object: return std::get<JsonObject>(value_).size();
            case Type::String: return std::get<std::string>(value_).size();
            default: return 0;
        }
    }
    
    bool empty() const {
        return size() == 0;
    }
    
    // Serialization
    std::string toString(bool pretty = false, int indent = 0) const;
    
    // Optional value access
    template<typename T>
    std::optional<T> get() const {
        try {
            if constexpr (std::is_same_v<T, bool>) {
                return asBool();
            } else if constexpr (std::is_same_v<T, int>) {
                return asInt();
            } else if constexpr (std::is_same_v<T, double>) {
                return asNumber();
            } else if constexpr (std::is_same_v<T, std::string>) {
                return asString();
            }
        } catch (const std::exception&) {
            return std::nullopt;
        }
        return std::nullopt;
    }
};

// ===== JSON PARSER =====

class JsonParser {
private:
    std::string json_;
    size_t pos_;
    size_t line_;
    size_t column_;
    
    void skipWhitespace() {
        while (pos_ < json_.length() && std::isspace(json_[pos_])) {
            if (json_[pos_] == '\n') {
                line_++;
                column_ = 1;
            } else {
                column_++;
            }
            pos_++;
        }
    }
    
    char peek() const {
        return pos_ < json_.length() ? json_[pos_] : '\0';
    }
    
    char consume() {
        if (pos_ >= json_.length()) return '\0';
        char c = json_[pos_++];
        column_++;
        return c;
    }
    
    std::string parseString() {
        if (consume() != '"') {
            throw JsonParseException("Expected '\"'", line_, column_);
        }
        
        std::string result;
        while (pos_ < json_.length()) {
            char c = peek();
            if (c == '"') {
                consume(); // consume closing quote
                return result;
            } else if (c == '\\') {
                consume(); // consume backslash
                char escaped = consume();
                switch (escaped) {
                    case '"': result += '"'; break;
                    case '\\': result += '\\'; break;
                    case '/': result += '/'; break;
                    case 'b': result += '\b'; break;
                    case 'f': result += '\f'; break;
                    case 'n': result += '\n'; break;
                    case 'r': result += '\r'; break;
                    case 't': result += '\t'; break;
                    case 'u': {
                        // Unicode escape sequence
                        std::string hex;
                        for (int i = 0; i < 4; ++i) {
                            char hexChar = consume();
                            if (!std::isxdigit(hexChar)) {
                                throw JsonParseException("Invalid unicode escape", line_, column_);
                            }
                            hex += hexChar;
                        }
                        // Convert hex to Unicode character (simplified)
                        int codepoint = std::stoi(hex, nullptr, 16);
                        if (codepoint <= 0x7F) {
                            result += static_cast<char>(codepoint);
                        } else {
                            result += '?'; // Placeholder for non-ASCII
                        }
                        break;
                    }
                    default:
                        throw JsonParseException("Invalid escape sequence", line_, column_);
                }
            } else if (c == '\0') {
                throw JsonParseException("Unterminated string", line_, column_);
            } else {
                result += consume();
            }
        }
        
        throw JsonParseException("Unterminated string", line_, column_);
    }
    
    double parseNumber() {
        std::string number;
        
        // Handle negative sign
        if (peek() == '-') {
            number += consume();
        }
        
        // Parse integer part
        if (peek() == '0') {
            number += consume();
        } else if (std::isdigit(peek())) {
            while (std::isdigit(peek())) {
                number += consume();
            }
        } else {
            throw JsonParseException("Invalid number format", line_, column_);
        }
        
        // Parse decimal part
        if (peek() == '.') {
            number += consume();
            if (!std::isdigit(peek())) {
                throw JsonParseException("Invalid number format", line_, column_);
            }
            while (std::isdigit(peek())) {
                number += consume();
            }
        }
        
        // Parse exponent part
        if (peek() == 'e' || peek() == 'E') {
            number += consume();
            if (peek() == '+' || peek() == '-') {
                number += consume();
            }
            if (!std::isdigit(peek())) {
                throw JsonParseException("Invalid number format", line_, column_);
            }
            while (std::isdigit(peek())) {
                number += consume();
            }
        }
        
        try {
            return std::stod(number);
        } catch (const std::exception&) {
            throw JsonParseException("Invalid number: " + number, line_, column_);
        }
    }
    
    JsonValue parseValue();
    JsonValue parseArray();
    JsonValue parseObject();
    
public:
    JsonValue parse(const std::string& json) {
        json_ = json;
        pos_ = 0;
        line_ = 1;
        column_ = 1;
        
        skipWhitespace();
        JsonValue result = parseValue();
        skipWhitespace();
        
        if (pos_ < json_.length()) {
            throw JsonParseException("Unexpected characters after JSON", line_, column_);
        }
        
        return result;
    }
    
    static JsonValue parseFromFile(const std::string& filename) {
        std::ifstream file(filename);
        if (!file.is_open()) {
            throw FileNotFoundException(filename);
        }
        
        std::string content((std::istreambuf_iterator<char>(file)),
                           std::istreambuf_iterator<char>());
        
        JsonParser parser;
        return parser.parse(content);
    }
    
    static JsonValue parseFromString(const std::string& json) {
        JsonParser parser;
        return parser.parse(json);
    }
};

// ===== CSV DATA STRUCTURES =====

struct CsvRow {
    std::vector<std::string> fields;
    
    const std::string& operator[](size_t index) const {
        if (index >= fields.size()) {
            throw std::out_of_range("CSV column index out of range");
        }
        return fields[index];
    }
    
    std::string& operator[](size_t index) {
        if (index >= fields.size()) {
            fields.resize(index + 1);
        }
        return fields[index];
    }
    
    size_t size() const { return fields.size(); }
    bool empty() const { return fields.empty(); }
    
    // Get field by header name
    std::string getField(const std::vector<std::string>& headers, const std::string& header_name) const {
        auto it = std::find(headers.begin(), headers.end(), header_name);
        if (it != headers.end()) {
            size_t index = std::distance(headers.begin(), it);
            if (index < fields.size()) {
                return fields[index];
            }
        }
        return "";
    }
    
    // Convert field to typed value
    template<typename T>
    T getFieldAs(size_t index) const {
        if (index >= fields.size()) {
            throw std::out_of_range("CSV column index out of range");
        }
        
        const std::string& field = fields[index];
        
        if constexpr (std::is_same_v<T, std::string>) {
            return field;
        } else if constexpr (std::is_same_v<T, int>) {
            return std::stoi(field);
        } else if constexpr (std::is_same_v<T, double>) {
            return std::stod(field);
        } else if constexpr (std::is_same_v<T, bool>) {
            std::string lower = field;
            std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
            return lower == "true" || lower == "1" || lower == "yes";
        }
    }
};

class CsvData {
private:
    std::vector<std::string> headers_;
    std::vector<CsvRow> rows_;
    
public:
    CsvData() = default;
    
    void setHeaders(const std::vector<std::string>& headers) {
        headers_ = headers;
    }
    
    void addRow(const CsvRow& row) {
        rows_.push_back(row);
    }
    
    void addRow(CsvRow&& row) {
        rows_.push_back(std::move(row));
    }
    
    const std::vector<std::string>& getHeaders() const { return headers_; }
    const std::vector<CsvRow>& getRows() const { return rows_; }
    std::vector<CsvRow>& getRows() { return rows_; }
    
    const CsvRow& operator[](size_t index) const {
        if (index >= rows_.size()) {
            throw std::out_of_range("CSV row index out of range");
        }
        return rows_[index];
    }
    
    CsvRow& operator[](size_t index) {
        if (index >= rows_.size()) {
            throw std::out_of_range("CSV row index out of range");
        }
        return rows_[index];
    }
    
    size_t rowCount() const { return rows_.size(); }
    size_t columnCount() const { return headers_.size(); }
    bool empty() const { return rows_.empty(); }
    
    // Find rows by criteria
    std::vector<const CsvRow*> findRows(const std::string& header, const std::string& value) const {
        std::vector<const CsvRow*> results;
        for (const auto& row : rows_) {
            if (row.getField(headers_, header) == value) {
                results.push_back(&row);
            }
        }
        return results;
    }
    
    // Get column data
    std::vector<std::string> getColumn(const std::string& header) const {
        std::vector<std::string> column;
        for (const auto& row : rows_) {
            column.push_back(row.getField(headers_, header));
        }
        return column;
    }
    
    // Statistics
    template<typename T>
    std::vector<T> getColumnAs(const std::string& header) const {
        std::vector<T> column;
        for (const auto& row : rows_) {
            try {
                std::string field = row.getField(headers_, header);
                if constexpr (std::is_same_v<T, int>) {
                    column.push_back(std::stoi(field));
                } else if constexpr (std::is_same_v<T, double>) {
                    column.push_back(std::stod(field));
                }
            } catch (const std::exception&) {
                // Skip invalid values
            }
        }
        return column;
    }
};

// ===== CSV PARSER =====

class CsvParser {
private:
    char delimiter_;
    char quote_char_;
    bool has_header_;
    
    std::vector<std::string> parseLine(const std::string& line) const {
        std::vector<std::string> fields;
        std::string current_field;
        bool in_quotes = false;
        bool quote_in_quoted_field = false;
        
        for (size_t i = 0; i < line.length(); ++i) {
            char c = line[i];
            
            if (c == quote_char_ && !quote_in_quoted_field) {
                if (in_quotes) {
                    if (i + 1 < line.length() && line[i + 1] == quote_char_) {
                        // Escaped quote
                        current_field += quote_char_;
                        ++i; // Skip next quote
                        quote_in_quoted_field = true;
                    } else {
                        // End of quoted field
                        in_quotes = false;
                    }
                } else {
                    // Start of quoted field
                    in_quotes = true;
                }
            } else if (c == delimiter_ && !in_quotes) {
                // Field delimiter
                fields.push_back(current_field);
                current_field.clear();
                quote_in_quoted_field = false;
            } else {
                current_field += c;
                quote_in_quoted_field = false;
            }
        }
        
        // Add the last field
        fields.push_back(current_field);
        
        return fields;
    }
    
    std::string trim(const std::string& str) const {
        size_t start = str.find_first_not_of(" \t\r\n");
        if (start == std::string::npos) return "";
        
        size_t end = str.find_last_not_of(" \t\r\n");
        return str.substr(start, end - start + 1);
    }
    
public:
    explicit CsvParser(char delimiter = ',', char quote_char = '"', bool has_header = true)
        : delimiter_(delimiter), quote_char_(quote_char), has_header_(has_header) {}
    
    CsvData parse(const std::string& content) const {
        CsvData data;
        std::istringstream iss(content);
        std::string line;
        size_t row_number = 0;
        
        while (std::getline(iss, line)) {
            ++row_number;
            
            // Skip empty lines
            if (trim(line).empty()) continue;
            
            try {
                std::vector<std::string> fields = parseLine(line);
                
                // Trim fields
                for (auto& field : fields) {
                    field = trim(field);
                }
                
                if (row_number == 1 && has_header_) {
                    data.setHeaders(fields);
                } else {
                    CsvRow row;
                    row.fields = std::move(fields);
                    data.addRow(std::move(row));
                }
            } catch (const std::exception& e) {
                throw CsvParseException(e.what(), row_number);
            }
        }
        
        return data;
    }
    
    static CsvData parseFromFile(const std::string& filename, char delimiter = ',', 
                                char quote_char = '"', bool has_header = true) {
        std::ifstream file(filename);
        if (!file.is_open()) {
            throw FileNotFoundException(filename);
        }
        
        std::string content((std::istreambuf_iterator<char>(file)),
                           std::istreambuf_iterator<char>());
        
        CsvParser parser(delimiter, quote_char, has_header);
        return parser.parse(content);
    }
    
    static CsvData parseFromString(const std::string& csv, char delimiter = ',',
                                  char quote_char = '"', bool has_header = true) {
        CsvParser parser(delimiter, quote_char, has_header);
        return parser.parse(csv);
    }
    
    // Write CSV
    static std::string writeCsv(const CsvData& data, char delimiter = ',', char quote_char = '"') {
        std::ostringstream oss;
        
        // Write headers
        const auto& headers = data.getHeaders();
        for (size_t i = 0; i < headers.size(); ++i) {
            if (i > 0) oss << delimiter;
            oss << quote_char << headers[i] << quote_char;
        }
        oss << '\n';
        
        // Write rows
        for (const auto& row : data.getRows()) {
            for (size_t i = 0; i < row.size(); ++i) {
                if (i > 0) oss << delimiter;
                oss << quote_char << row[i] << quote_char;
            }
            oss << '\n';
        }
        
        return oss.str();
    }
    
    static void writeToFile(const CsvData& data, const std::string& filename, 
                           char delimiter = ',', char quote_char = '"') {
        std::ofstream file(filename);
        if (!file.is_open()) {
            throw std::runtime_error("Cannot open file for writing: " + filename);
        }
        
        file << writeCsv(data, delimiter, quote_char);
    }
};

// ===== XML NODE STRUCTURE =====

class XmlNode {
public:
    std::string name;
    std::string content;
    std::map<std::string, std::string> attributes;
    std::vector<std::shared_ptr<XmlNode>> children;
    std::weak_ptr<XmlNode> parent;
    
    XmlNode() = default;
    explicit XmlNode(const std::string& node_name) : name(node_name) {}
    
    // Attribute access
    std::string getAttribute(const std::string& attr_name, const std::string& default_value = "") const {
        auto it = attributes.find(attr_name);
        return it != attributes.end() ? it->second : default_value;
    }
    
    void setAttribute(const std::string& attr_name, const std::string& value) {
        attributes[attr_name] = value;
    }
    
    bool hasAttribute(const std::string& attr_name) const {
        return attributes.find(attr_name) != attributes.end();
    }
    
    // Child node access
    std::shared_ptr<XmlNode> findChild(const std::string& child_name) const {
        for (const auto& child : children) {
            if (child->name == child_name) {
                return child;
            }
        }
        return nullptr;
    }
    
    std::vector<std::shared_ptr<XmlNode>> findChildren(const std::string& child_name) const {
        std::vector<std::shared_ptr<XmlNode>> results;
        for (const auto& child : children) {
            if (child->name == child_name) {
                results.push_back(child);
            }
        }
        return results;
    }
    
    void addChild(std::shared_ptr<XmlNode> child) {
        child->parent = shared_from_this();
        children.push_back(child);
    }
    
    // Content access
    std::string getContent() const { return content; }
    void setContent(const std::string& new_content) { content = new_content; }
    
    // Utility methods
    bool hasChildren() const { return !children.empty(); }
    size_t childCount() const { return children.size(); }
    
    // Convert to string representation
    std::string toString(int indent = 0) const {
        std::ostringstream oss;
        std::string indent_str(indent * 2, ' ');
        
        oss << indent_str << "<" << name;
        
        // Add attributes
        for (const auto& attr : attributes) {
            oss << " " << attr.first << "=\"" << attr.second << "\"";
        }
        
        if (children.empty() && content.empty()) {
            oss << "/>";
        } else {
            oss << ">";
            
            if (!content.empty()) {
                oss << content;
            }
            
            if (!children.empty()) {
                oss << "\n";
                for (const auto& child : children) {
                    oss << child->toString(indent + 1) << "\n";
                }
                oss << indent_str;
            }
            
            oss << "</" << name << ">";
        }
        
        return oss.str();
    }
};

// Enable shared_from_this for XmlNode
class XmlNode : public std::enable_shared_from_this<XmlNode> {
    // Previous XmlNode implementation remains the same
};

// ===== XML PARSER =====

class XmlParser {
private:
    std::string xml_;
    size_t pos_;
    size_t line_;
    
    void skipWhitespace() {
        while (pos_ < xml_.length() && std::isspace(xml_[pos_])) {
            if (xml_[pos_] == '\n') line_++;
            pos_++;
        }
    }
    
    char peek() const {
        return pos_ < xml_.length() ? xml_[pos_] : '\0';
    }
    
    char consume() {
        return pos_ < xml_.length() ? xml_[pos_++] : '\0';
    }
    
    std::string parseTagName() {
        std::string name;
        while (pos_ < xml_.length() && (std::isalnum(xml_[pos_]) || xml_[pos_] == '_' || xml_[pos_] == '-')) {
            name += consume();
        }
        return name;
    }
    
    std::string parseAttributeValue() {
        char quote = consume(); // Should be ' or "
        if (quote != '\'' && quote != '"') {
            throw XmlParseException("Expected quote for attribute value", line_);
        }
        
        std::string value;
        while (pos_ < xml_.length() && peek() != quote) {
            char c = consume();
            if (c == '&') {
                // Handle entity references (simplified)
                std::string entity;
                while (pos_ < xml_.length() && peek() != ';') {
                    entity += consume();
                }
                if (consume() != ';') {
                    throw XmlParseException("Invalid entity reference", line_);
                }
                
                if (entity == "amp") value += '&';
                else if (entity == "lt") value += '<';
                else if (entity == "gt") value += '>';
                else if (entity == "quot") value += '"';
                else if (entity == "apos") value += '\'';
                else {
                    // Unknown entity, keep as is
                    value += '&' + entity + ';';
                }
            } else {
                value += c;
            }
        }
        
        if (consume() != quote) {
            throw XmlParseException("Unterminated attribute value", line_);
        }
        
        return value;
    }
    
    std::map<std::string, std::string> parseAttributes() {
        std::map<std::string, std::string> attributes;
        
        skipWhitespace();
        while (pos_ < xml_.length() && peek() != '>' && peek() != '/') {
            std::string attr_name = parseTagName();
            if (attr_name.empty()) {
                throw XmlParseException("Expected attribute name", line_);
            }
            
            skipWhitespace();
            if (consume() != '=') {
                throw XmlParseException("Expected '=' after attribute name", line_);
            }
            
            skipWhitespace();
            std::string attr_value = parseAttributeValue();
            
            attributes[attr_name] = attr_value;
            skipWhitespace();
        }
        
        return attributes;
    }
    
    std::shared_ptr<XmlNode> parseElement() {
        if (consume() != '<') {
            throw XmlParseException("Expected '<'", line_);
        }
        
        std::string tag_name = parseTagName();
        if (tag_name.empty()) {
            throw XmlParseException("Expected tag name", line_);
        }
        
        auto node = std::make_shared<XmlNode>(tag_name);
        node->attributes = parseAttributes();
        
        skipWhitespace();
        char next = peek();
        
        if (next == '/') {
            // Self-closing tag
            consume(); // consume '/'
            if (consume() != '>') {
                throw XmlParseException("Expected '>' after '/'", line_);
            }
            return node;
        } else if (next == '>') {
            consume(); // consume '>'
            
            // Parse content and children
            std::string content;
            while (pos_ < xml_.length()) {
                skipWhitespace();
                if (pos_ < xml_.length() && peek() == '<') {
                    if (pos_ + 1 < xml_.length() && xml_[pos_ + 1] == '/') {
                        // End tag
                        break;
                    } else {
                        // Child element
                        if (!content.empty()) {
                            node->content = content;
                            content.clear();
                        }
                        auto child = parseElement();
                        node->addChild(child);
                    }
                } else if (pos_ < xml_.length()) {
                    // Text content
                    content += consume();
                }
            }
            
            if (!content.empty()) {
                node->content = content;
            }
            
            // Parse end tag
            if (consume() != '<' || consume() != '/') {
                throw XmlParseException("Expected end tag", line_);
            }
            
            std::string end_tag = parseTagName();
            if (end_tag != tag_name) {
                throw XmlParseException("Mismatched end tag: expected " + tag_name + ", got " + end_tag, line_);
            }
            
            skipWhitespace();
            if (consume() != '>') {
                throw XmlParseException("Expected '>' in end tag", line_);
            }
            
            return node;
        } else {
            throw XmlParseException("Expected '>' or '/>'", line_);
        }
    }
    
public:
    std::shared_ptr<XmlNode> parse(const std::string& xml) {
        xml_ = xml;
        pos_ = 0;
        line_ = 1;
        
        skipWhitespace();
        
        // Skip XML declaration if present
        if (pos_ + 5 < xml_.length() && xml_.substr(pos_, 5) == "<?xml") {
            while (pos_ < xml_.length() && xml_.substr(pos_, 2) != "?>") {
                if (xml_[pos_] == '\n') line_++;
                pos_++;
            }
            if (pos_ + 1 < xml_.length()) {
                pos_ += 2; // Skip "?>"
            }
            skipWhitespace();
        }
        
        return parseElement();
    }
    
    static std::shared_ptr<XmlNode> parseFromFile(const std::string& filename) {
        std::ifstream file(filename);
        if (!file.is_open()) {
            throw FileNotFoundException(filename);
        }
        
        std::string content((std::istreambuf_iterator<char>(file)),
                           std::istreambuf_iterator<char>());
        
        XmlParser parser;
        return parser.parse(content);
    }
    
    static std::shared_ptr<XmlNode> parseFromString(const std::string& xml) {
        XmlParser parser;
        return parser.parse(xml);
    }
};

// ===== SPACE GAME SPECIFIC PARSERS =====

namespace SpaceGameParsers {
    
    // Planet configuration parser
    struct PlanetConfig {
        int id;
        std::string name;
        double distance_from_star;
        long long population;
        bool habitable;
        std::vector<std::string> resources;
        std::map<std::string, double> orbital_parameters;
    };
    
    class PlanetConfigParser {
    public:
        static std::vector<PlanetConfig> parseFromJson(const std::string& filename) {
            JsonValue json = JsonParser::parseFromFile(filename);
            std::vector<PlanetConfig> planets;
            
            if (!json.isArray()) {
                throw ParseException("Expected array of planets in JSON");
            }
            
            for (size_t i = 0; i < json.size(); ++i) {
                const JsonValue& planet_json = json[i];
                PlanetConfig config;
                
                config.id = planet_json["id"].asInt();
                config.name = planet_json["name"].asString();
                config.distance_from_star = planet_json["distance_from_star"].asNumber();
                config.population = static_cast<long long>(planet_json["population"].asNumber());
                config.habitable = planet_json["habitable"].asBool();
                
                // Parse resources array
                if (planet_json.contains("resources") && planet_json["resources"].isArray()) {
                    const auto& resources_json = planet_json["resources"];
                    for (size_t j = 0; j < resources_json.size(); ++j) {
                        config.resources.push_back(resources_json[j].asString());
                    }
                }
                
                // Parse orbital parameters
                if (planet_json.contains("orbital_parameters") && planet_json["orbital_parameters"].isObject()) {
                    const auto& orbital_json = planet_json["orbital_parameters"];
                    const auto& obj = orbital_json.asObject();
                    for (const auto& pair : obj) {
                        config.orbital_parameters[pair.first] = pair.second.asNumber();
                    }
                }
                
                planets.push_back(config);
            }
            
            return planets;
        }
        
        static void writeToJson(const std::vector<PlanetConfig>& planets, const std::string& filename) {
            JsonValue planets_json(JsonValue::JsonArray{});
            
            for (const auto& planet : planets) {
                JsonValue planet_json(JsonValue::JsonObject{});
                planet_json["id"] = planet.id;
                planet_json["name"] = planet.name;
                planet_json["distance_from_star"] = planet.distance_from_star;
                planet_json["population"] = static_cast<double>(planet.population);
                planet_json["habitable"] = planet.habitable;
                
                // Add resources
                JsonValue resources_json(JsonValue::JsonArray{});
                for (const auto& resource : planet.resources) {
                    resources_json.asArray().push_back(JsonValue(resource));
                }
                planet_json["resources"] = resources_json;
                
                // Add orbital parameters
                JsonValue orbital_json(JsonValue::JsonObject{});
                for (const auto& param : planet.orbital_parameters) {
                    orbital_json[param.first] = param.second;
                }
                planet_json["orbital_parameters"] = orbital_json;
                
                planets_json.asArray().push_back(planet_json);
            }
            
            std::ofstream file(filename);
            if (!file.is_open()) {
                throw std::runtime_error("Cannot open file for writing: " + filename);
            }
            
            file << planets_json.toString(true);
        }
    };
    
    // Fleet data CSV parser
    struct FleetData {
        int fleet_id;
        std::string commander;
        int ship_count;
        double fuel_level;
        std::string mission_type;
        std::string current_location;
    };
    
    class FleetDataParser {
    public:
        static std::vector<FleetData> parseFromCsv(const std::string& filename) {
            CsvData csv = CsvParser::parseFromFile(filename);
            std::vector<FleetData> fleets;
            
            const auto& headers = csv.getHeaders();
            
            // Validate required headers
            std::vector<std::string> required_headers = {
                "fleet_id", "commander", "ship_count", "fuel_level", "mission_type", "current_location"
            };
            
            for (const auto& required : required_headers) {
                if (std::find(headers.begin(), headers.end(), required) == headers.end()) {
                    throw ParseException("Missing required header: " + required);
                }
            }
            
            for (const auto& row : csv.getRows()) {
                FleetData fleet;
                fleet.fleet_id = row.getFieldAs<int>(0);
                fleet.commander = row.getField(headers, "commander");
                fleet.ship_count = std::stoi(row.getField(headers, "ship_count"));
                fleet.fuel_level = std::stod(row.getField(headers, "fuel_level"));
                fleet.mission_type = row.getField(headers, "mission_type");
                fleet.current_location = row.getField(headers, "current_location");
                
                fleets.push_back(fleet);
            }
            
            return fleets;
        }
    };
    
    // Mission XML parser
    struct MissionConfig {
        int mission_id;
        std::string type;
        std::string title;
        std::string description;
        int priority;
        std::vector<std::string> objectives;
        std::map<std::string, std::string> parameters;
    };
    
    class MissionConfigParser {
    public:
        static std::vector<MissionConfig> parseFromXml(const std::string& filename) {
            auto root = XmlParser::parseFromFile(filename);
            std::vector<MissionConfig> missions;
            
            if (root->name != "missions") {
                throw ParseException("Expected 'missions' root element");
            }
            
            for (const auto& mission_node : root->children) {
                if (mission_node->name != "mission") continue;
                
                MissionConfig config;
                config.mission_id = std::stoi(mission_node->getAttribute("id", "0"));
                config.type = mission_node->getAttribute("type", "Unknown");
                config.priority = std::stoi(mission_node->getAttribute("priority", "1"));
                
                for (const auto& child : mission_node->children) {
                    if (child->name == "title") {
                        config.title = child->content;
                    } else if (child->name == "description") {
                        config.description = child->content;
                    } else if (child->name == "objectives") {
                        for (const auto& objective : child->children) {
                            if (objective->name == "objective") {
                                config.objectives.push_back(objective->content);
                            }
                        }
                    } else if (child->name == "parameters") {
                        for (const auto& param : child->children) {
                            if (param->name == "parameter") {
                                std::string key = param->getAttribute("name");
                                config.parameters[key] = param->content;
                            }
                        }
                    }
                }
                
                missions.push_back(config);
            }
            
            return missions;
        }
    };
    
} // namespace SpaceGameParsers

// ===== UTILITY FUNCTIONS =====

namespace FileParserUtils {
    
    // Auto-detect file format and parse accordingly
    enum class FileFormat {
        JSON,
        CSV,
        XML,
        Unknown
    };
    
    FileFormat detectFormat(const std::string& filename) {
        // Check extension first
        std::string extension = filename.substr(filename.find_last_of('.') + 1);
        std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
        
        if (extension == "json") return FileFormat::JSON;
        if (extension == "csv") return FileFormat::CSV;
        if (extension == "xml") return FileFormat::XML;
        
        // Check content
        std::ifstream file(filename);
        if (!file.is_open()) return FileFormat::Unknown;
        
        std::string first_line;
        std::getline(file, first_line);
        
        // Trim whitespace
        first_line.erase(0, first_line.find_first_not_of(" \t\r\n"));
        
        if (!first_line.empty()) {
            if (first_line[0] == '{' || first_line[0] == '[') return FileFormat::JSON;
            if (first_line[0] == '<') return FileFormat::XML;
        }
        
        return FileFormat::CSV; // Default assumption
    }
    
    // Generic file info
    struct FileInfo {
        std::string filename;
        FileFormat format;
        size_t file_size;
        std::chrono::system_clock::time_point last_modified;
    };
    
    FileInfo getFileInfo(const std::string& filename) {
        FileInfo info;
        info.filename = filename;
        info.format = detectFormat(filename);
        
        std::ifstream file(filename, std::ios::ate | std::ios::binary);
        if (file.is_open()) {
            info.file_size = file.tellg();
        } else {
            info.file_size = 0;
        }
        
        // Note: last_modified would require filesystem library for cross-platform compatibility
        info.last_modified = std::chrono::system_clock::now();
        
        return info;
    }
    
    // Validation functions
    bool isValidJson(const std::string& content) {
        try {
            JsonParser::parseFromString(content);
            return true;
        } catch (const JsonParseException&) {
            return false;
        }
    }
    
    bool isValidXml(const std::string& content) {
        try {
            XmlParser::parseFromString(content);
            return true;
        } catch (const XmlParseException&) {
            return false;
        }
    }
    
    // Conversion utilities
    std::string csvToJson(const CsvData& csv_data) {
        JsonValue json_array(JsonValue::JsonArray{});
        
        const auto& headers = csv_data.getHeaders();
        for (const auto& row : csv_data.getRows()) {
            JsonValue json_obj(JsonValue::JsonObject{});
            
            for (size_t i = 0; i < headers.size() && i < row.size(); ++i) {
                json_obj[headers[i]] = row[i];
            }
            
            json_array.asArray().push_back(json_obj);
        }
        
        return json_array.toString(true);
    }
    
} // namespace FileParserUtils

} // namespace CppVerseHub::Utils