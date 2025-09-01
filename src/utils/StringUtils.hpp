// File: src/utils/StringUtils.hpp
// String Manipulation Utilities for Space Game

#pragma once

#include <string>
#include <vector>
#include <sstream>
#include <algorithm>
#include <regex>
#include <unordered_map>
#include <cctype>
#include <locale>
#include <iomanip>
#include <random>

namespace CppVerseHub::Utils::String {

// ===== BASIC STRING OPERATIONS =====

// Trimming functions
std::string trim(const std::string& str);
std::string trimLeft(const std::string& str);
std::string trimRight(const std::string& str);
std::string trimChars(const std::string& str, const std::string& chars);

// Case conversion
std::string toUpper(const std::string& str);
std::string toLower(const std::string& str);
std::string toTitle(const std::string& str);
std::string toCamelCase(const std::string& str);
std::string toPascalCase(const std::string& str);
std::string toSnakeCase(const std::string& str);
std::string toKebabCase(const std::string& str);

// String predicates
bool startsWith(const std::string& str, const std::string& prefix);
bool endsWith(const std::string& str, const std::string& suffix);
bool contains(const std::string& str, const std::string& substring);
bool containsIgnoreCase(const std::string& str, const std::string& substring);
bool isAlpha(const std::string& str);
bool isNumeric(const std::string& str);
bool isAlphaNumeric(const std::string& str);
bool isEmpty(const std::string& str);
bool isWhitespace(const std::string& str);

// ===== SPLITTING AND JOINING =====

std::vector<std::string> split(const std::string& str, char delimiter);
std::vector<std::string> split(const std::string& str, const std::string& delimiter);
std::vector<std::string> splitByWhitespace(const std::string& str);
std::vector<std::string> splitLines(const std::string& str);
std::vector<std::string> tokenize(const std::string& str, const std::string& delimiters);

std::string join(const std::vector<std::string>& strings, const std::string& separator);
std::string join(const std::vector<std::string>& strings, char separator);

// ===== SEARCH AND REPLACE =====

std::string replace(const std::string& str, const std::string& from, const std::string& to);
std::string replaceAll(const std::string& str, const std::string& from, const std::string& to);
std::string replaceFirst(const std::string& str, const std::string& from, const std::string& to);
std::string replaceLast(const std::string& str, const std::string& from, const std::string& to);

size_t findNth(const std::string& str, const std::string& substring, size_t n);
std::vector<size_t> findAll(const std::string& str, const std::string& substring);
size_t countOccurrences(const std::string& str, const std::string& substring);
size_t countOccurrences(const std::string& str, char character);

// ===== PADDING AND ALIGNMENT =====

std::string padLeft(const std::string& str, size_t width, char pad_char = ' ');
std::string padRight(const std::string& str, size_t width, char pad_char = ' ');
std::string padCenter(const std::string& str, size_t width, char pad_char = ' ');
std::string repeat(const std::string& str, size_t count);
std::string repeat(char character, size_t count);

// ===== FORMATTING UTILITIES =====

template<typename... Args>
std::string format(const std::string& format_str, Args&&... args) {
    std::ostringstream oss;
    formatImpl(oss, format_str, std::forward<Args>(args)...);
    return oss.str();
}

// Specialized formatters
std::string formatNumber(double number, int precision = 2);
std::string formatInteger(long long number, bool use_thousands_separator = false);
std::string formatBytes(size_t bytes);
std::string formatPercent(double value, int precision = 1);
std::string formatDuration(double seconds);
std::string formatFileSize(size_t bytes);

// ===== VALIDATION AND PARSING =====

bool isValidEmail(const std::string& email);
bool isValidUrl(const std::string& url);
bool isValidIPv4(const std::string& ip);
bool isValidIPv6(const std::string& ip);
bool isValidHexColor(const std::string& color);
bool isValidUUID(const std::string& uuid);

// Parsing functions
std::optional<int> parseInt(const std::string& str);
std::optional<double> parseDouble(const std::string& str);
std::optional<bool> parseBool(const std::string& str);

// ===== ENCODING AND ESCAPING =====

std::string urlEncode(const std::string& str);
std::string urlDecode(const std::string& str);
std::string htmlEncode(const std::string& str);
std::string htmlDecode(const std::string& str);
std::string jsonEscape(const std::string& str);
std::string csvEscape(const std::string& str);
std::string base64Encode(const std::string& str);
std::string base64Decode(const std::string& str);

// ===== HASHING AND CHECKSUMS =====

std::string md5Hash(const std::string& str);
std::string sha256Hash(const std::string& str);
uint32_t crc32(const std::string& str);
uint32_t simpleHash(const std::string& str);
uint64_t fnv1aHash(const std::string& str);

// ===== ADVANCED STRING OPERATIONS =====

double similarityScore(const std::string& str1, const std::string& str2);
int levenshteinDistance(const std::string& str1, const std::string& str2);
double jaccardSimilarity(const std::string& str1, const std::string& str2);
std::string longestCommonSubstring(const std::string& str1, const std::string& str2);

// ===== RANDOM STRING GENERATION =====

class StringGenerator {
private:
    std::mt19937 generator_;
    
public:
    explicit StringGenerator(unsigned seed = std::random_device{}());
    
    std::string randomString(size_t length, const std::string& charset = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789");
    std::string randomAlphabetic(size_t length);
    std::string randomNumeric(size_t length);
    std::string randomAlphanumeric(size_t length);
    std::string randomHex(size_t length);
    std::string randomPassword(size_t length, bool include_symbols = true);
    std::string randomUUID();
    
    // Space game specific generators
    std::string randomPlanetName();
    std::string randomStarName();
    std::string randomShipName();
    std::string randomCommanderName();
    std::string randomMissionCode();
};

// ===== TEXT PROCESSING =====

class TextProcessor {
public:
    // Word counting and analysis
    static size_t wordCount(const std::string& text);
    static size_t characterCount(const std::string& text, bool include_whitespace = true);
    static size_t lineCount(const std::string& text);
    static std::unordered_map<std::string, size_t> wordFrequency(const std::string& text);
    
    // Text transformation
    static std::string removeExtraWhitespace(const std::string& text);
    static std::string wrapText(const std::string& text, size_t line_width);
    static std::string indent(const std::string& text, size_t spaces);
    static std::string reverseWords(const std::string& text);
    static std::string reverseString(const std::string& text);
    
    // Text extraction
    static std::vector<std::string> extractWords(const std::string& text);
    static std::vector<std::string> extractNumbers(const std::string& text);
    static std::vector<std::string> extractEmails(const std::string& text);
    static std::vector<std::string> extractUrls(const std::string& text);
    
    // Statistics
    static double averageWordLength(const std::string& text);
    static std::string longestWord(const std::string& text);
    static std::string shortestWord(const std::string& text);
    static double readabilityScore(const std::string& text);
};

// ===== TEMPLATE UTILITIES =====

class StringTemplate {
private:
    std::string template_string_;
    std::unordered_map<std::string, std::string> variables_;
    
public:
    explicit StringTemplate(const std::string& template_str);
    
    void setVariable(const std::string& name, const std::string& value);
    void setVariable(const std::string& name, int value);
    void setVariable(const std::string& name, double value);
    void setVariable(const std::string& name, bool value);
    
    std::string render() const;
    void clear();
    
    // Static template methods
    static std::string simpleReplace(const std::string& template_str, 
                                   const std::unordered_map<std::string, std::string>& variables);
};

// ===== SPACE GAME SPECIFIC UTILITIES =====

namespace SpaceGame {
    
    // Coordinate system conversions
    std::string formatCoordinates(double x, double y, double z);
    std::string formatPolarCoordinates(double radius, double theta, double phi);
    std::string formatDistance(double distance_meters);
    std::string formatMass(double mass_kg);
    std::string formatVelocity(double velocity_ms);
    
    // Game object naming
    std::string generateStarSystemName();
    std::string generatePlanetDesignation(const std::string& star_name, int planet_number);
    std::string generateFleetCallsign();
    std::string generateMissionId();
    
    // Status and error messages
    std::string formatMissionStatus(const std::string& mission_type, double progress);
    std::string formatFleetStatus(const std::string& fleet_name, int ship_count, double fuel_level);
    std::string formatErrorMessage(const std::string& operation, const std::string& error_details);
    
    // Configuration parsing
    std::pair<std::string, std::string> parseKeyValue(const std::string& line);
    std::vector<std::pair<std::string, std::string>> parseConfigSection(const std::string& section_text);
    
    // Command parsing
    struct Command {
        std::string action;
        std::vector<std::string> arguments;
        std::unordered_map<std::string, std::string> options;
    };
    
    Command parseCommand(const std::string& command_line);
    std::string formatCommandHelp(const std::string& command, const std::string& description, 
                                const std::vector<std::string>& arguments,
                                const std::vector<std::pair<std::string, std::string>>& options);
    
} // namespace SpaceGame

// ===== PERFORMANCE UTILITIES =====

namespace Performance {
    
    class StringPool {
    private:
        std::unordered_map<std::string, std::shared_ptr<std::string>> pool_;
        mutable std::mutex mutex_;
        
    public:
        std::shared_ptr<const std::string> intern(const std::string& str);
        size_t size() const;
        void clear();
    };
    
    class StringBuilder {
    private:
        std::vector<std::string> parts_;
        size_t estimated_length_;
        
    public:
        StringBuilder();
        
        StringBuilder& append(const std::string& str);
        StringBuilder& append(char ch);
        StringBuilder& append(int value);
        StringBuilder& append(double value);
        StringBuilder& appendLine(const std::string& str = "");
        
        std::string toString() const;
        size_t length() const { return estimated_length_; }
        void clear();
        void reserve(size_t capacity);
    };
    
} // namespace Performance

// ===== UNICODE SUPPORT =====

namespace Unicode {
    
    // Basic UTF-8 operations
    size_t utf8Length(const std::string& str);
    std::string utf8Substring(const std::string& str, size_t start, size_t length);
    bool isValidUtf8(const std::string& str);
    
    // Character classification
    bool isUtf8Whitespace(const std::string& str, size_t pos);
    bool isUtf8Alphabetic(const std::string& str, size_t pos);
    bool isUtf8Numeric(const std::string& str, size_t pos);
    
    // Normalization
    std::string normalizeWhitespace(const std::string& str);
    std::string removeAccents(const std::string& str);
    
} // namespace Unicode

// ===== UTILITY FUNCTIONS =====

// Global string utilities
std::string generateUniqueId(const std::string& prefix = "");
std::string getCurrentTimestamp(const std::string& format = "%Y-%m-%d %H:%M:%S");
std::string escapeRegex(const std::string& str);
std::string humanReadableSize(size_t bytes);
std::string pluralize(const std::string& word, size_t count);

// String constants for common use cases
namespace Constants {
    extern const std::string WHITESPACE_CHARS;
    extern const std::string ALPHANUMERIC_CHARS;
    extern const std::string ALPHABETIC_CHARS;
    extern const std::string NUMERIC_CHARS;
    extern const std::string HEX_CHARS;
    extern const std::string SPECIAL_CHARS;
    extern const std::string VOWELS;
    extern const std::string CONSONANTS;
}

// ===== IMPLEMENTATION HELPERS =====

namespace Detail {
    
    template<typename T>
    void formatImpl(std::ostringstream& oss, const std::string& format_str, T&& value) {
        size_t pos = format_str.find("{}");
        if (pos != std::string::npos) {
            oss << format_str.substr(0, pos) << std::forward<T>(value) << format_str.substr(pos + 2);
        } else {
            oss << format_str;
        }
    }
    
    template<typename T, typename... Args>
    void formatImpl(std::ostringstream& oss, const std::string& format_str, T&& value, Args&&... args) {
        size_t pos = format_str.find("{}");
        if (pos != std::string::npos) {
            oss << format_str.substr(0, pos) << std::forward<T>(value);
            formatImpl(oss, format_str.substr(pos + 2), std::forward<Args>(args)...);
        } else {
            oss << format_str;
        }
    }
    
} // namespace Detail

// Make formatImpl available at namespace level
using Detail::formatImpl;

} // namespace CppVerseHub::Utils::String