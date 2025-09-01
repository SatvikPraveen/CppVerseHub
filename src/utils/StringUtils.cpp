// File: src/utils/StringUtils.cpp
// String Manipulation Utilities Implementation

#include "StringUtils.hpp"
#include <iostream>
#include <chrono>
#include <ctime>
#include <iomanip>

namespace CppVerseHub::Utils::String {

// ===== STRING CONSTANTS =====

namespace Constants {
    const std::string WHITESPACE_CHARS = " \t\n\r\f\v";
    const std::string ALPHANUMERIC_CHARS = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    const std::string ALPHABETIC_CHARS = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
    const std::string NUMERIC_CHARS = "0123456789";
    const std::string HEX_CHARS = "0123456789ABCDEFabcdef";
    const std::string SPECIAL_CHARS = "!@#$%^&*()_+-=[]{}|;:,.<>?";
    const std::string VOWELS = "aeiouAEIOU";
    const std::string CONSONANTS = "bcdfghjklmnpqrstvwxyzBCDFGHJKLMNPQRSTVWXYZ";
}

// ===== BASIC STRING OPERATIONS =====

std::string trim(const std::string& str) {
    size_t start = str.find_first_not_of(Constants::WHITESPACE_CHARS);
    if (start == std::string::npos) return "";
    
    size_t end = str.find_last_not_of(Constants::WHITESPACE_CHARS);
    return str.substr(start, end - start + 1);
}

std::string trimLeft(const std::string& str) {
    size_t start = str.find_first_not_of(Constants::WHITESPACE_CHARS);
    return (start == std::string::npos) ? "" : str.substr(start);
}

std::string trimRight(const std::string& str) {
    size_t end = str.find_last_not_of(Constants::WHITESPACE_CHARS);
    return (end == std::string::npos) ? "" : str.substr(0, end + 1);
}

std::string trimChars(const std::string& str, const std::string& chars) {
    size_t start = str.find_first_not_of(chars);
    if (start == std::string::npos) return "";
    
    size_t end = str.find_last_not_of(chars);
    return str.substr(start, end - start + 1);
}

std::string toUpper(const std::string& str) {
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(), ::toupper);
    return result;
}

std::string toLower(const std::string& str) {
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(), ::tolower);
    return result;
}

std::string toTitle(const std::string& str) {
    std::string result = str;
    bool capitalize_next = true;
    
    for (char& c : result) {
        if (std::isalpha(c)) {
            c = capitalize_next ? std::toupper(c) : std::tolower(c);
            capitalize_next = false;
        } else {
            capitalize_next = true;
        }
    }
    
    return result;
}

std::string toCamelCase(const std::string& str) {
    std::vector<std::string> words = splitByWhitespace(str);
    if (words.empty()) return "";
    
    std::string result = toLower(words[0]);
    for (size_t i = 1; i < words.size(); ++i) {
        if (!words[i].empty()) {
            result += static_cast<char>(std::toupper(words[i][0]));
            result += toLower(words[i].substr(1));
        }
    }
    
    return result;
}

std::string toPascalCase(const std::string& str) {
    std::vector<std::string> words = splitByWhitespace(str);
    std::string result;
    
    for (const auto& word : words) {
        if (!word.empty()) {
            result += static_cast<char>(std::toupper(word[0]));
            result += toLower(word.substr(1));
        }
    }
    
    return result;
}

std::string toSnakeCase(const std::string& str) {
    std::string result;
    for (size_t i = 0; i < str.length(); ++i) {
        if (std::isupper(str[i])) {
            if (i > 0 && std::islower(str[i-1])) {
                result += '_';
            }
            result += static_cast<char>(std::tolower(str[i]));
        } else if (std::isspace(str[i])) {
            result += '_';
        } else {
            result += str[i];
        }
    }
    
    return result;
}

std::string toKebabCase(const std::string& str) {
    return replaceAll(toSnakeCase(str), "_", "-");
}

// ===== STRING PREDICATES =====

bool startsWith(const std::string& str, const std::string& prefix) {
    return str.length() >= prefix.length() && 
           str.compare(0, prefix.length(), prefix) == 0;
}

bool endsWith(const std::string& str, const std::string& suffix) {
    return str.length() >= suffix.length() && 
           str.compare(str.length() - suffix.length(), suffix.length(), suffix) == 0;
}

bool contains(const std::string& str, const std::string& substring) {
    return str.find(substring) != std::string::npos;
}

bool containsIgnoreCase(const std::string& str, const std::string& substring) {
    return contains(toLower(str), toLower(substring));
}

bool isAlpha(const std::string& str) {
    return !str.empty() && std::all_of(str.begin(), str.end(), [](char c) { return std::isalpha(c); });
}

bool isNumeric(const std::string& str) {
    if (str.empty()) return false;
    
    size_t start = 0;
    if (str[0] == '-' || str[0] == '+') start = 1;
    
    bool has_dot = false;
    for (size_t i = start; i < str.length(); ++i) {
        if (str[i] == '.' && !has_dot) {
            has_dot = true;
        } else if (!std::isdigit(str[i])) {
            return false;
        }
    }
    
    return start < str.length();
}

bool isAlphaNumeric(const std::string& str) {
    return !str.empty() && std::all_of(str.begin(), str.end(), [](char c) { return std::isalnum(c); });
}

bool isEmpty(const std::string& str) {
    return str.empty();
}

bool isWhitespace(const std::string& str) {
    return !str.empty() && std::all_of(str.begin(), str.end(), [](char c) { return std::isspace(c); });
}

// ===== SPLITTING AND JOINING =====

std::vector<std::string> split(const std::string& str, char delimiter) {
    std::vector<std::string> tokens;
    std::stringstream ss(str);
    std::string token;
    
    while (std::getline(ss, token, delimiter)) {
        tokens.push_back(token);
    }
    
    return tokens;
}

std::vector<std::string> split(const std::string& str, const std::string& delimiter) {
    std::vector<std::string> tokens;
    
    if (delimiter.empty()) {
        tokens.push_back(str);
        return tokens;
    }
    
    size_t start = 0;
    size_t found = str.find(delimiter);
    
    while (found != std::string::npos) {
        tokens.push_back(str.substr(start, found - start));
        start = found + delimiter.length();
        found = str.find(delimiter, start);
    }
    
    tokens.push_back(str.substr(start));
    return tokens;
}

std::vector<std::string> splitByWhitespace(const std::string& str) {
    std::vector<std::string> tokens;
    std::istringstream iss(str);
    std::string token;
    
    while (iss >> token) {
        tokens.push_back(token);
    }
    
    return tokens;
}

std::vector<std::string> splitLines(const std::string& str) {
    std::vector<std::string> lines;
    std::stringstream ss(str);
    std::string line;
    
    while (std::getline(ss, line)) {
        lines.push_back(line);
    }
    
    return lines;
}

std::vector<std::string> tokenize(const std::string& str, const std::string& delimiters) {
    std::vector<std::string> tokens;
    size_t start = str.find_first_not_of(delimiters);
    
    while (start != std::string::npos) {
        size_t end = str.find_first_of(delimiters, start);
        tokens.push_back(str.substr(start, end - start));
        start = str.find_first_not_of(delimiters, end);
    }
    
    return tokens;
}

std::string join(const std::vector<std::string>& strings, const std::string& separator) {
    if (strings.empty()) return "";
    
    std::string result = strings[0];
    for (size_t i = 1; i < strings.size(); ++i) {
        result += separator + strings[i];
    }
    
    return result;
}

std::string join(const std::vector<std::string>& strings, char separator) {
    return join(strings, std::string(1, separator));
}

// ===== SEARCH AND REPLACE =====

std::string replace(const std::string& str, const std::string& from, const std::string& to) {
    return replaceFirst(str, from, to);
}

std::string replaceAll(const std::string& str, const std::string& from, const std::string& to) {
    if (from.empty()) return str;
    
    std::string result = str;
    size_t pos = 0;
    
    while ((pos = result.find(from, pos)) != std::string::npos) {
        result.replace(pos, from.length(), to);
        pos += to.length();
    }
    
    return result;
}

std::string replaceFirst(const std::string& str, const std::string& from, const std::string& to) {
    size_t pos = str.find(from);
    if (pos != std::string::npos) {
        std::string result = str;
        result.replace(pos, from.length(), to);
        return result;
    }
    return str;
}

std::string replaceLast(const std::string& str, const std::string& from, const std::string& to) {
    size_t pos = str.rfind(from);
    if (pos != std::string::npos) {
        std::string result = str;
        result.replace(pos, from.length(), to);
        return result;
    }
    return str;
}

size_t findNth(const std::string& str, const std::string& substring, size_t n) {
    size_t pos = 0;
    
    for (size_t i = 0; i < n; ++i) {
        pos = str.find(substring, pos);
        if (pos == std::string::npos) break;
        if (i < n - 1) pos += substring.length();
    }
    
    return pos;
}

std::vector<size_t> findAll(const std::string& str, const std::string& substring) {
    std::vector<size_t> positions;
    size_t pos = str.find(substring);
    
    while (pos != std::string::npos) {
        positions.push_back(pos);
        pos = str.find(substring, pos + 1);
    }
    
    return positions;
}

size_t countOccurrences(const std::string& str, const std::string& substring) {
    if (substring.empty()) return 0;
    
    size_t count = 0;
    size_t pos = str.find(substring);
    
    while (pos != std::string::npos) {
        ++count;
        pos = str.find(substring, pos + substring.length());
    }
    
    return count;
}

size_t countOccurrences(const std::string& str, char character) {
    return std::count(str.begin(), str.end(), character);
}

// ===== PADDING AND ALIGNMENT =====

std::string padLeft(const std::string& str, size_t width, char pad_char) {
    if (str.length() >= width) return str;
    return std::string(width - str.length(), pad_char) + str;
}

std::string padRight(const std::string& str, size_t width, char pad_char) {
    if (str.length() >= width) return str;
    return str + std::string(width - str.length(), pad_char);
}

std::string padCenter(const std::string& str, size_t width, char pad_char) {
    if (str.length() >= width) return str;
    
    size_t pad_total = width - str.length();
    size_t pad_left = pad_total / 2;
    size_t pad_right = pad_total - pad_left;
    
    return std::string(pad_left, pad_char) + str + std::string(pad_right, pad_char);
}

std::string repeat(const std::string& str, size_t count) {
    std::string result;
    result.reserve(str.length() * count);
    
    for (size_t i = 0; i < count; ++i) {
        result += str;
    }
    
    return result;
}

std::string repeat(char character, size_t count) {
    return std::string(count, character);
}

// ===== FORMATTING UTILITIES =====

std::string formatNumber(double number, int precision) {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(precision) << number;
    return oss.str();
}

std::string formatInteger(long long number, bool use_thousands_separator) {
    std::ostringstream oss;
    
    if (use_thousands_separator) {
        oss.imbue(std::locale(""));
    }
    
    oss << number;
    return oss.str();
}

std::string formatBytes(size_t bytes) {
    const char* units[] = {"B", "KB", "MB", "GB", "TB", "PB"};
    int unit_index = 0;
    double size = static_cast<double>(bytes);
    
    while (size >= 1024.0 && unit_index < 5) {
        size /= 1024.0;
        ++unit_index;
    }
    
    std::ostringstream oss;
    if (unit_index == 0) {
        oss << static_cast<size_t>(size) << " " << units[unit_index];
    } else {
        oss << std::fixed << std::setprecision(1) << size << " " << units[unit_index];
    }
    
    return oss.str();
}

std::string formatPercent(double value, int precision) {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(precision) << (value * 100.0) << "%";
    return oss.str();
}

std::string formatDuration(double seconds) {
    int days = static_cast<int>(seconds / 86400);
    seconds = std::fmod(seconds, 86400);
    
    int hours = static_cast<int>(seconds / 3600);
    seconds = std::fmod(seconds, 3600);
    
    int minutes = static_cast<int>(seconds / 60);
    seconds = std::fmod(seconds, 60);
    
    std::ostringstream oss;
    
    if (days > 0) {
        oss << days << "d ";
    }
    if (hours > 0 || days > 0) {
        oss << hours << "h ";
    }
    if (minutes > 0 || hours > 0 || days > 0) {
        oss << minutes << "m ";
    }
    
    oss << std::fixed << std::setprecision(1) << seconds << "s";
    
    return oss.str();
}

std::string formatFileSize(size_t bytes) {
    return formatBytes(bytes);
}

// ===== VALIDATION AND PARSING =====

bool isValidEmail(const std::string& email) {
    std::regex email_regex(R"([a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\.[a-zA-Z]{2,})");
    return std::regex_match(email, email_regex);
}

bool isValidUrl(const std::string& url) {
    std::regex url_regex(R"(https?://[^\s/$.?#].[^\s]*)");
    return std::regex_match(url, url_regex);
}

bool isValidIPv4(const std::string& ip) {
    std::regex ipv4_regex(R"(^((25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\.){3}(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)$)");
    return std::regex_match(ip, ipv4_regex);
}

bool isValidIPv6(const std::string& ip) {
    // Simplified IPv6 validation
    std::regex ipv6_regex(R"(^([0-9a-fA-F]{1,4}:){7}[0-9a-fA-F]{1,4}$)");
    return std::regex_match(ip, ipv6_regex);
}

bool isValidHexColor(const std::string& color) {
    std::regex color_regex(R"(^#[0-9a-fA-F]{6}$)");
    return std::regex_match(color, color_regex);
}

bool isValidUUID(const std::string& uuid) {
    std::regex uuid_regex(R"(^[0-9a-fA-F]{8}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{12}$)");
    return std::regex_match(uuid, uuid_regex);
}

std::optional<int> parseInt(const std::string& str) {
    try {
        return std::stoi(str);
    } catch (const std::exception&) {
        return std::nullopt;
    }
}

std::optional<double> parseDouble(const std::string& str) {
    try {
        return std::stod(str);
    } catch (const std::exception&) {
        return std::nullopt;
    }
}

std::optional<bool> parseBool(const std::string& str) {
    std::string lower = toLower(trim(str));
    
    if (lower == "true" || lower == "1" || lower == "yes" || lower == "on") {
        return true;
    } else if (lower == "false" || lower == "0" || lower == "no" || lower == "off") {
        return false;
    }
    
    return std::nullopt;
}

// ===== ENCODING AND ESCAPING =====

std::string urlEncode(const std::string& str) {
    std::ostringstream encoded;
    encoded.fill('0');
    encoded << std::hex;
    
    for (char c : str) {
        if (std::isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
            encoded << c;
        } else {
            encoded << std::uppercase << '%' << std::setw(2) << static_cast<int>(static_cast<unsigned char>(c));
        }
    }
    
    return encoded.str();
}

std::string urlDecode(const std::string& str) {
    std::string decoded;
    
    for (size_t i = 0; i < str.length(); ++i) {
        if (str[i] == '%' && i + 2 < str.length()) {
            std::string hex = str.substr(i + 1, 2);
            char c = static_cast<char>(std::stoi(hex, nullptr, 16));
            decoded += c;
            i += 2;
        } else if (str[i] == '+') {
            decoded += ' ';
        } else {
            decoded += str[i];
        }
    }
    
    return decoded;
}

std::string htmlEncode(const std::string& str) {
    std::string encoded;
    encoded.reserve(str.length() * 1.1);  // Rough estimate
    
    for (char c : str) {
        switch (c) {
            case '<': encoded += "&lt;"; break;
            case '>': encoded += "&gt;"; break;
            case '&': encoded += "&amp;"; break;
            case '"': encoded += "&quot;"; break;
            case '\'': encoded += "&apos;"; break;
            default: encoded += c; break;
        }
    }
    
    return encoded;
}

std::string htmlDecode(const std::string& str) {
    std::string decoded = str;
    decoded = replaceAll(decoded, "&lt;", "<");
    decoded = replaceAll(decoded, "&gt;", ">");
    decoded = replaceAll(decoded, "&amp;", "&");
    decoded = replaceAll(decoded, "&quot;", "\"");
    decoded = replaceAll(decoded, "&apos;", "'");
    return decoded;
}

std::string jsonEscape(const std::string& str) {
    std::string escaped;
    escaped.reserve(str.length() * 1.1);
    
    for (char c : str) {
        switch (c) {
            case '"': escaped += "\\\""; break;
            case '\\': escaped += "\\\\"; break;
            case '\b': escaped += "\\b"; break;
            case '\f': escaped += "\\f"; break;
            case '\n': escaped += "\\n"; break;
            case '\r': escaped += "\\r"; break;
            case '\t': escaped += "\\t"; break;
            default:
                if (c < 0x20) {
                    escaped += "\\u";
                    escaped += "0000";
                    // Convert to hex
                    char hex[3];
                    sprintf(hex, "%02x", static_cast<unsigned char>(c));
                    escaped[escaped.length()-2] = hex[0];
                    escaped[escaped.length()-1] = hex[1];
                } else {
                    escaped += c;
                }
                break;
        }
    }
    
    return escaped;
}

std::string csvEscape(const std::string& str) {
    bool needs_quoting = contains(str, ",") || contains(str, "\"") || contains(str, "\n");
    
    if (!needs_quoting) return str;
    
    std::string escaped = "\"" + replaceAll(str, "\"", "\"\"") + "\"";
    return escaped;
}

// Simplified base64 implementation
std::string base64Encode(const std::string& str) {
    static const std::string chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string encoded;
    int padding = str.length() % 3;
    
    for (size_t i = 0; i < str.length(); i += 3) {
        uint32_t temp = 0;
        
        for (int j = 0; j < 3; ++j) {
            temp <<= 8;
            if (i + j < str.length()) {
                temp |= static_cast<unsigned char>(str[i + j]);
            }
        }
        
        for (int j = 3; j >= 0; --j) {
            if (i * 4 / 3 + j < (str.length() * 4 + 2) / 3) {
                encoded += chars[(temp >> (j * 6)) & 0x3F];
            }
        }
    }
    
    if (padding > 0) {
        encoded += std::string(3 - padding, '=');
    }
    
    return encoded;
}

std::string base64Decode(const std::string& str) {
    // Simplified implementation - in production use a more robust library
    static const int decode_table[256] = {
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,62,-1,-1,-1,63,
        52,53,54,55,56,57,58,59,60,61,-1,-1,-1,-1,-1,-1,
        -1, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,
        15,16,17,18,19,20,21,22,23,24,25,-1,-1,-1,-1,-1,
        -1,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,
        41,42,43,44,45,46,47,48,49,50,51,-1,-1,-1,-1,-1
    };
    
    std::string decoded;
    uint32_t temp = 0;
    int bits = 0;
    
    for (char c : str) {
        if (c == '=') break;
        
        int val = decode_table[static_cast<unsigned char>(c)];
        if (val == -1) continue;
        
        temp = (temp << 6) | val;
        bits += 6;
        
        if (bits >= 8) {
            decoded += static_cast<char>((temp >> (bits - 8)) & 0xFF);
            bits -= 8;
        }
    }
    
    return decoded;
}

// ===== ADVANCED STRING OPERATIONS =====

std::string reverse(const std::string& str) {
    std::string reversed = str;
    std::reverse(reversed.begin(), reversed.end());
    return reversed;
}

std::string removeChars(const std::string& str, const std::string& chars_to_remove) {
    std::string result;
    result.reserve(str.length());
    
    for (char c : str) {
        if (chars_to_remove.find(c) == std::string::npos) {
            result += c;
        }
    }
    
    return result;
}

std::string keepChars(const std::string& str, const std::string& chars_to_keep) {
    std::string result;
    result.reserve(str.length());
    
    for (char c : str) {
        if (chars_to_keep.find(c) != std::string::npos) {
            result += c;
        }
    }
    
    return result;
}

std::string insertAt(const std::string& str, size_t position, const std::string& insertion) {
    if (position >= str.length()) {
        return str + insertion;
    }
    
    return str.substr(0, position) + insertion + str.substr(position);
}

std::string removeAt(const std::string& str, size_t position, size_t length) {
    if (position >= str.length()) {
        return str;
    }
    
    return str.substr(0, position) + str.substr(position + length);
}

std::string substring(const std::string& str, size_t start, size_t length) {
    if (start >= str.length()) {
        return "";
    }
    
    return str.substr(start, length);
}

std::string left(const std::string& str, size_t count) {
    return str.substr(0, count);
}

std::string right(const std::string& str, size_t count) {
    if (count >= str.length()) {
        return str;
    }
    
    return str.substr(str.length() - count);
}

std::string mid(const std::string& str, size_t start, size_t length) {
    return substring(str, start, length);
}

// ===== TEXT ANALYSIS =====

size_t wordCount(const std::string& str) {
    auto words = splitByWhitespace(str);
    return words.size();
}

size_t lineCount(const std::string& str) {
    return countOccurrences(str, '\n') + 1;
}

size_t characterCount(const std::string& str, bool include_spaces) {
    if (include_spaces) {
        return str.length();
    }
    
    return str.length() - countOccurrences(str, ' ');
}

std::map<char, size_t> characterFrequency(const std::string& str) {
    std::map<char, size_t> frequency;
    
    for (char c : str) {
        frequency[c]++;
    }
    
    return frequency;
}

double readabilityScore(const std::string& text) {
    // Simple Flesch Reading Ease approximation
    auto sentences = split(text, '.');
    auto words = splitByWhitespace(text);
    
    if (sentences.empty() || words.empty()) {
        return 0.0;
    }
    
    size_t total_syllables = 0;
    for (const auto& word : words) {
        // Simple syllable counting heuristic
        size_t syllables = std::max(1ul, countOccurrences(toLower(word), "aeiou"));
        total_syllables += syllables;
    }
    
    double avg_sentence_length = static_cast<double>(words.size()) / sentences.size();
    double avg_syllables_per_word = static_cast<double>(total_syllables) / words.size();
    
    return 206.835 - (1.015 * avg_sentence_length) - (84.6 * avg_syllables_per_word);
}

// ===== STRING GENERATION =====

std::string generateRandom(size_t length, const std::string& charset) {
    static std::random_device rd;
    static std::mt19937 generator(rd());
    
    std::uniform_int_distribution<> distribution(0, charset.size() - 1);
    std::string result;
    result.reserve(length);
    
    for (size_t i = 0; i < length; ++i) {
        result += charset[distribution(generator)];
    }
    
    return result;
}

std::string generateUUID() {
    static std::random_device rd;
    static std::mt19937 generator(rd());
    static std::uniform_int_distribution<> hex_dist(0, 15);
    
    std::string uuid;
    uuid.reserve(36);
    
    for (int i = 0; i < 36; ++i) {
        if (i == 8 || i == 13 || i == 18 || i == 23) {
            uuid += '-';
        } else if (i == 14) {
            uuid += '4';  // Version 4
        } else if (i == 19) {
            uuid += "89ab"[hex_dist(generator) % 4];  // Variant bits
        } else {
            uuid += "0123456789abcdef"[hex_dist(generator)];
        }
    }
    
    return uuid;
}

std::string generatePassword(size_t length, bool include_symbols, bool include_numbers) {
    std::string charset = Constants::ALPHABETIC_CHARS;
    
    if (include_numbers) {
        charset += Constants::NUMERIC_CHARS;
    }
    
    if (include_symbols) {
        charset += Constants::SPECIAL_CHARS;
    }
    
    return generateRandom(length, charset);
}

// ===== UTILITY FUNCTIONS =====

std::string escapeRegex(const std::string& str) {
    std::string escaped;
    const std::string special_chars = ".^$*+?()[{\\|";
    
    for (char c : str) {
        if (special_chars.find(c) != std::string::npos) {
            escaped += '\\';
        }
        escaped += c;
    }
    
    return escaped;
}

std::string wrapText(const std::string& text, size_t width, const std::string& indent) {
    if (width == 0) return text;
    
    std::vector<std::string> words = splitByWhitespace(text);
    if (words.empty()) return text;
    
    std::vector<std::string> lines;
    std::string current_line = indent;
    
    for (const auto& word : words) {
        if (current_line.length() + word.length() + 1 > width && current_line != indent) {
            lines.push_back(current_line);
            current_line = indent;
        }
        
        if (current_line != indent) {
            current_line += " ";
        }
        current_line += word;
    }
    
    if (!current_line.empty()) {
        lines.push_back(current_line);
    }
    
    return join(lines, "\n");
}

std::string expandTabs(const std::string& str, size_t tab_size) {
    std::string result;
    size_t column = 0;
    
    for (char c : str) {
        if (c == '\t') {
            size_t spaces = tab_size - (column % tab_size);
            result += std::string(spaces, ' ');
            column += spaces;
        } else if (c == '\n') {
            result += c;
            column = 0;
        } else {
            result += c;
            ++column;
        }
    }
    
    return result;
}

bool isAnagram(const std::string& str1, const std::string& str2) {
    std::string s1 = toLower(removeChars(str1, " "));
    std::string s2 = toLower(removeChars(str2, " "));
    
    if (s1.length() != s2.length()) return false;
    
    std::sort(s1.begin(), s1.end());
    std::sort(s2.begin(), s2.end());
    
    return s1 == s2;
}

bool isPalindrome(const std::string& str, bool ignore_case, bool ignore_spaces) {
    std::string processed = str;
    
    if (ignore_case) {
        processed = toLower(processed);
    }
    
    if (ignore_spaces) {
        processed = removeChars(processed, " ");
    }
    
    return processed == reverse(processed);
}

int levenshteinDistance(const std::string& str1, const std::string& str2) {
    const size_t len1 = str1.length();
    const size_t len2 = str2.length();
    
    std::vector<std::vector<int>> matrix(len1 + 1, std::vector<int>(len2 + 1));
    
    for (size_t i = 0; i <= len1; ++i) {
        matrix[i][0] = i;
    }
    
    for (size_t j = 0; j <= len2; ++j) {
        matrix[0][j] = j;
    }
    
    for (size_t i = 1; i <= len1; ++i) {
        for (size_t j = 1; j <= len2; ++j) {
            int cost = (str1[i - 1] == str2[j - 1]) ? 0 : 1;
            
            matrix[i][j] = std::min({
                matrix[i - 1][j] + 1,      // deletion
                matrix[i][j - 1] + 1,      // insertion
                matrix[i - 1][j - 1] + cost // substitution
            });
        }
    }
    
    return matrix[len1][len2];
}

double similarityRatio(const std::string& str1, const std::string& str2) {
    if (str1.empty() && str2.empty()) return 1.0;
    if (str1.empty() || str2.empty()) return 0.0;
    
    int distance = levenshteinDistance(str1, str2);
    int max_length = std::max(str1.length(), str2.length());
    
    return 1.0 - (static_cast<double>(distance) / max_length);
}

} // namespace CppVerseHub::Utils::String