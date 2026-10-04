/**
 * @file StringUtils.cpp
 * @brief Implementation of the string utilities declared in StringUtils.hpp.
 */
#include "utils/StringUtils.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <iomanip>
#include <limits>
#include <locale>
#include <stdexcept>

namespace CppVerseHub::Utils::String {

namespace {

constexpr bool isSpace(char c) noexcept {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\f' || c == '\v';
}
constexpr bool isUpper(char c) noexcept {
    return c >= 'A' && c <= 'Z';
}
constexpr bool isLower(char c) noexcept {
    return c >= 'a' && c <= 'z';
}
constexpr bool isDigit(char c) noexcept {
    return c >= '0' && c <= '9';
}
constexpr bool isAlpha(char c) noexcept {
    return isUpper(c) || isLower(c);
}
constexpr bool isAlnum(char c) noexcept {
    return isAlpha(c) || isDigit(c);
}
constexpr char upper(char c) noexcept {
    return isLower(c) ? static_cast<char>(c - 'a' + 'A') : c;
}
constexpr char lower(char c) noexcept {
    return isUpper(c) ? static_cast<char>(c - 'A' + 'a') : c;
}

/// Splits identifiers into lower-cased words at separators and lower->upper transitions.
std::vector<std::string> identifierWords(std::string_view text) {
    std::vector<std::string> words;
    std::string current;
    auto flush = [&] {
        if (!current.empty()) {
            words.push_back(std::move(current));
            current.clear();
        }
    };
    for (std::size_t i = 0; i < text.size(); ++i) {
        const char c = text[i];
        if (!isAlnum(c)) {
            flush();
            continue;
        }
        if (isUpper(c) && !current.empty()) {
            const bool prevLower = isLower(text[i - 1]) || isDigit(text[i - 1]);
            const bool nextLower = i + 1 < text.size() && isLower(text[i + 1]);
            // "parseHTTPResponse" -> parse, http, response
            if (prevLower || (isUpper(text[i - 1]) && nextLower)) {
                flush();
            }
        }
        current.push_back(lower(c));
    }
    flush();
    return words;
}

constexpr std::string_view kBase64Alphabet =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

constexpr int base64Value(char c) noexcept {
    if (isUpper(c)) {
        return c - 'A';
    }
    if (isLower(c)) {
        return c - 'a' + 26;
    }
    if (isDigit(c)) {
        return c - '0' + 52;
    }
    if (c == '+') {
        return 62;
    }
    if (c == '/') {
        return 63;
    }
    return -1;
}

constexpr int hexValue(char c) noexcept {
    if (isDigit(c)) {
        return c - '0';
    }
    if (c >= 'a' && c <= 'f') {
        return c - 'a' + 10;
    }
    if (c >= 'A' && c <= 'F') {
        return c - 'A' + 10;
    }
    return -1;
}

constexpr char kHexDigits[] = "0123456789abcdef";

/// Decodes one UTF-8 sequence starting at `i`; returns code point and advances `i`, or nullopt.
std::optional<char32_t> decodeOne(std::string_view text, std::size_t& i) noexcept {
    const auto b0 = static_cast<unsigned char>(text[i]);
    std::size_t length = 0;
    char32_t cp = 0;
    char32_t minimum = 0;
    if (b0 < 0x80U) {
        ++i;
        return static_cast<char32_t>(b0);
    }
    if ((b0 & 0xE0U) == 0xC0U) {
        length = 2;
        cp = b0 & 0x1FU;
        minimum = 0x80;
    } else if ((b0 & 0xF0U) == 0xE0U) {
        length = 3;
        cp = b0 & 0x0FU;
        minimum = 0x800;
    } else if ((b0 & 0xF8U) == 0xF0U) {
        length = 4;
        cp = b0 & 0x07U;
        minimum = 0x10000;
    } else {
        return std::nullopt;
    }
    if (i + length > text.size()) {
        return std::nullopt;
    }
    for (std::size_t k = 1; k < length; ++k) {
        const auto b = static_cast<unsigned char>(text[i + k]);
        if ((b & 0xC0U) != 0x80U) {
            return std::nullopt;
        }
        cp = (cp << 6U) | (b & 0x3FU);
    }
    if (cp < minimum || cp > 0x10FFFF || (cp >= 0xD800 && cp <= 0xDFFF)) {
        return std::nullopt;
    }
    i += length;
    return cp;
}

} // namespace

// ===================================================================================================
// Trimming, splitting, joining
// ===================================================================================================

std::string trim(std::string_view text) {
    return std::string{trimView(text)};
}

std::vector<std::string> split(std::string_view text, char delimiter, bool skipEmpty) {
    std::vector<std::string> parts;
    std::size_t start = 0;
    while (true) {
        const auto pos = text.find(delimiter, start);
        const auto field = text.substr(start,
                                       pos == std::string_view::npos ? std::string_view::npos : pos - start);
        if (!skipEmpty || !field.empty()) {
            parts.emplace_back(field);
        }
        if (pos == std::string_view::npos) {
            break;
        }
        start = pos + 1;
    }
    return parts;
}

std::vector<std::string> split(std::string_view text, std::string_view delimiter, bool skipEmpty) {
    if (delimiter.empty()) {
        return {std::string{text}};
    }
    std::vector<std::string> parts;
    std::size_t start = 0;
    while (true) {
        const auto pos = text.find(delimiter, start);
        const auto field = text.substr(start,
                                       pos == std::string_view::npos ? std::string_view::npos : pos - start);
        if (!skipEmpty || !field.empty()) {
            parts.emplace_back(field);
        }
        if (pos == std::string_view::npos) {
            break;
        }
        start = pos + delimiter.size();
    }
    return parts;
}

std::vector<std::string> splitWhitespace(std::string_view text) {
    std::vector<std::string> words;
    std::size_t i = 0;
    while (i < text.size()) {
        while (i < text.size() && isSpace(text[i])) {
            ++i;
        }
        const std::size_t start = i;
        while (i < text.size() && !isSpace(text[i])) {
            ++i;
        }
        if (i > start) {
            words.emplace_back(text.substr(start, i - start));
        }
    }
    return words;
}

std::vector<std::string> tokenize(std::string_view text) {
    std::vector<std::string> tokens;
    std::string current;
    bool inToken = false;
    char quote = '\0';
    for (std::size_t i = 0; i < text.size(); ++i) {
        const char c = text[i];
        if (quote != '\0') {
            if (c == quote) {
                quote = '\0';
            } else if (c == '\\' && quote == '"' && i + 1 < text.size()) {
                current.push_back(text[++i]);
            } else {
                current.push_back(c);
            }
        } else if (c == '"' || c == '\'') {
            quote = c;
            inToken = true;
        } else if (c == '\\' && i + 1 < text.size()) {
            current.push_back(text[++i]);
            inToken = true;
        } else if (isSpace(c)) {
            if (inToken) {
                tokens.push_back(std::move(current));
                current.clear();
                inToken = false;
            }
        } else {
            current.push_back(c);
            inToken = true;
        }
    }
    if (quote != '\0') {
        throw std::invalid_argument("tokenize: unterminated quote");
    }
    if (inToken) {
        tokens.push_back(std::move(current));
    }
    return tokens;
}

// ===================================================================================================
// Case and predicates
// ===================================================================================================

std::string toUpper(std::string_view text) {
    std::string out{text};
    std::transform(out.begin(), out.end(), out.begin(), upper);
    return out;
}

std::string toLower(std::string_view text) {
    std::string out{text};
    std::transform(out.begin(), out.end(), out.begin(), lower);
    return out;
}

std::string toTitleCase(std::string_view text) {
    std::string out{text};
    bool startOfWord = true;
    for (char& c : out) {
        if (isAlnum(c)) {
            c = startOfWord ? upper(c) : lower(c);
            startOfWord = false;
        } else {
            startOfWord = true;
        }
    }
    return out;
}

std::string toCamelCase(std::string_view text) {
    const auto words = identifierWords(text);
    std::string out;
    for (std::size_t i = 0; i < words.size(); ++i) {
        std::string word = words[i];
        if (i > 0 && !word.empty()) {
            word[0] = upper(word[0]);
        }
        out += word;
    }
    return out;
}

std::string toSnakeCase(std::string_view text) {
    return join(identifierWords(text), "_");
}

std::string toKebabCase(std::string_view text) {
    return join(identifierWords(text), "-");
}

bool equalsIgnoreCase(std::string_view a, std::string_view b) noexcept {
    return a.size() == b.size() &&
           std::equal(a.begin(), a.end(), b.begin(), [](char x, char y) { return lower(x) == lower(y); });
}

std::size_t countOccurrences(std::string_view text, std::string_view needle) noexcept {
    if (needle.empty()) {
        return 0;
    }
    std::size_t count = 0;
    for (auto pos = text.find(needle); pos != std::string_view::npos;
         pos = text.find(needle, pos + needle.size())) {
        ++count;
    }
    return count;
}

bool isPalindrome(std::string_view text) noexcept {
    if (text.empty()) {
        return true;
    }
    std::size_t i = 0;
    std::size_t j = text.size() - 1;
    while (i < j) {
        if (!isAlnum(text[i])) {
            ++i;
        } else if (!isAlnum(text[j])) {
            --j;
        } else {
            if (lower(text[i]) != lower(text[j])) {
                return false;
            }
            ++i;
            --j;
        }
    }
    return true;
}

bool wildcardMatch(std::string_view text, std::string_view pattern) noexcept {
    std::size_t t = 0;
    std::size_t p = 0;
    std::size_t starP = std::string_view::npos;
    std::size_t starT = 0;
    while (t < text.size()) {
        if (p < pattern.size() && (pattern[p] == '?' || pattern[p] == text[t])) {
            ++t;
            ++p;
        } else if (p < pattern.size() && pattern[p] == '*') {
            starP = p++;
            starT = t;
        } else if (starP != std::string_view::npos) {
            p = starP + 1;
            t = ++starT;
        } else {
            return false;
        }
    }
    while (p < pattern.size() && pattern[p] == '*') {
        ++p;
    }
    return p == pattern.size();
}

// ===================================================================================================
// Transformations
// ===================================================================================================

std::string replaceAll(std::string_view text, std::string_view from, std::string_view to) {
    if (from.empty()) {
        return std::string{text};
    }
    std::string out;
    out.reserve(text.size());
    std::size_t start = 0;
    for (auto pos = text.find(from); pos != std::string_view::npos; pos = text.find(from, start)) {
        out += text.substr(start, pos - start);
        out += to;
        start = pos + from.size();
    }
    out += text.substr(start);
    return out;
}

std::string repeat(std::string_view text, std::size_t times) {
    std::string out;
    out.reserve(text.size() * times);
    for (std::size_t i = 0; i < times; ++i) {
        out += text;
    }
    return out;
}

std::string padLeft(std::string_view text, std::size_t width, char fill) {
    if (text.size() >= width) {
        return std::string{text};
    }
    return std::string(width - text.size(), fill) + std::string{text};
}

std::string padRight(std::string_view text, std::size_t width, char fill) {
    std::string out{text};
    if (out.size() < width) {
        out.append(width - out.size(), fill);
    }
    return out;
}

std::string center(std::string_view text, std::size_t width, char fill) {
    if (text.size() >= width) {
        return std::string{text};
    }
    const std::size_t total = width - text.size();
    const std::size_t left = total / 2;
    return std::string(left, fill) + std::string{text} + std::string(total - left, fill);
}

std::string reverse(std::string_view text) {
    return {text.rbegin(), text.rend()};
}

std::vector<std::string> wordWrap(std::string_view text, std::size_t width) {
    std::vector<std::string> lines;
    std::string line;
    for (const auto& word : splitWhitespace(text)) {
        if (!line.empty() && line.size() + 1 + word.size() > width) {
            lines.push_back(std::move(line));
            line.clear();
        }
        if (!line.empty()) {
            line.push_back(' ');
        }
        line += word;
    }
    if (!line.empty()) {
        lines.push_back(std::move(line));
    }
    return lines;
}

std::string truncate(std::string_view text, std::size_t maxLength, std::string_view ellipsis) {
    if (text.size() <= maxLength) {
        return std::string{text};
    }
    if (maxLength <= ellipsis.size()) {
        return std::string{ellipsis.substr(0, maxLength)};
    }
    return std::string{text.substr(0, maxLength - ellipsis.size())} + std::string{ellipsis};
}

// ===================================================================================================
// Algorithms
// ===================================================================================================

std::size_t levenshteinDistance(std::string_view a, std::string_view b) {
    if (a.size() < b.size()) {
        std::swap(a, b);
    }
    // b is the shorter string: keep one row of length |b| + 1.
    std::vector<std::size_t> row(b.size() + 1);
    for (std::size_t j = 0; j <= b.size(); ++j) {
        row[j] = j;
    }
    for (std::size_t i = 1; i <= a.size(); ++i) {
        std::size_t diagonal = row[0];
        row[0] = i;
        for (std::size_t j = 1; j <= b.size(); ++j) {
            const std::size_t above = row[j];
            const std::size_t cost = a[i - 1] == b[j - 1] ? 0 : 1;
            row[j] = std::min({row[j] + 1, row[j - 1] + 1, diagonal + cost});
            diagonal = above;
        }
    }
    return row[b.size()];
}

double similarity(std::string_view a, std::string_view b) {
    const std::size_t longest = std::max(a.size(), b.size());
    if (longest == 0) {
        return 1.0;
    }
    return 1.0 - static_cast<double>(levenshteinDistance(a, b)) / static_cast<double>(longest);
}

std::string longestCommonSubsequence(std::string_view a, std::string_view b) {
    const std::size_t n = a.size();
    const std::size_t m = b.size();
    std::vector<std::size_t> table((n + 1) * (m + 1), 0);
    auto at = [m, &table](std::size_t i, std::size_t j) -> std::size_t& { return table[i * (m + 1) + j]; };
    for (std::size_t i = 1; i <= n; ++i) {
        for (std::size_t j = 1; j <= m; ++j) {
            at(i, j) = a[i - 1] == b[j - 1] ? at(i - 1, j - 1) + 1 : std::max(at(i - 1, j), at(i, j - 1));
        }
    }
    std::string result;
    std::size_t i = n;
    std::size_t j = m;
    while (i > 0 && j > 0) {
        if (a[i - 1] == b[j - 1]) {
            result.push_back(a[i - 1]);
            --i;
            --j;
        } else if (at(i - 1, j) >= at(i, j - 1)) {
            --i;
        } else {
            --j;
        }
    }
    std::reverse(result.begin(), result.end());
    return result;
}

std::map<std::string, std::size_t> wordFrequency(std::string_view text) {
    std::map<std::string, std::size_t> freq;
    std::string word;
    for (const char c : text) {
        if (isAlnum(c) || c == '\'') {
            word.push_back(lower(c));
        } else if (!word.empty()) {
            ++freq[word];
            word.clear();
        }
    }
    if (!word.empty()) {
        ++freq[word];
    }
    return freq;
}

// ===================================================================================================
// Numbers
// ===================================================================================================

std::optional<double> detail::parseDouble(std::string_view text) noexcept {
    if (!text.empty() && text.front() == '+') {
        text.remove_prefix(1);
    }
    if (text.empty()) {
        return std::nullopt;
    }
#if defined(__cpp_lib_to_chars) && __cpp_lib_to_chars >= 201611L
    double value = 0.0;
    const auto [ptr, ec] = std::from_chars(text.data(), text.data() + text.size(), value);
    if (ec != std::errc{} || ptr != text.data() + text.size()) {
        return std::nullopt;
    }
    return value;
#else
    // Fallback for standard libraries without floating-point from_chars: a classic-locale stream.
    try {
        for (const char c : text) {
            if (!(isDigit(c) || c == '.' || c == '-' || c == 'e' || c == 'E' || c == '+')) {
                return std::nullopt;
            }
        }
        std::istringstream iss{std::string{text}};
        iss.imbue(std::locale::classic());
        double value = 0.0;
        iss >> value;
        if (iss.fail() || iss.peek() != std::char_traits<char>::eof()) {
            return std::nullopt;
        }
        return value;
    } catch (...) {
        return std::nullopt;
    }
#endif
}

std::string formatFixed(double value, int precision) {
    std::ostringstream oss;
    oss.imbue(std::locale::classic());
    oss << std::fixed << std::setprecision(std::max(precision, 0)) << value;
    return oss.str();
}

std::string withThousandsSeparator(long long value, char separator) {
    const bool negative = value < 0;
    // Work with the unsigned magnitude so LLONG_MIN is handled.
    const unsigned long long magnitude = negative ? 0ULL - static_cast<unsigned long long>(value)
                                                  : static_cast<unsigned long long>(value);
    const std::string digits = std::to_string(magnitude);
    std::string out;
    out.reserve(digits.size() + digits.size() / 3 + 1);
    const std::size_t lead = digits.size() % 3;
    for (std::size_t i = 0; i < digits.size(); ++i) {
        if (i != 0 && i >= lead && (i - lead) % 3 == 0) {
            out.push_back(separator);
        }
        out.push_back(digits[i]);
    }
    return negative ? "-" + out : out;
}

std::string formatBytes(std::uint64_t bytes) {
    static constexpr std::array<std::string_view, 7> kUnits = {"B", "KiB", "MiB", "GiB", "TiB", "PiB", "EiB"};
    if (bytes < 1024) {
        return std::to_string(bytes) + " B";
    }
    auto value = static_cast<double>(bytes);
    std::size_t unit = 0;
    while (value >= 1024.0 && unit + 1 < kUnits.size()) {
        value /= 1024.0;
        ++unit;
    }
    return formatFixed(value, 2) + " " + std::string{kUnits[unit]};
}

// ===================================================================================================
// Encodings
// ===================================================================================================

std::string base64Encode(std::string_view data) {
    std::string out;
    out.reserve((data.size() + 2) / 3 * 4);
    std::size_t i = 0;
    for (; i + 2 < data.size(); i += 3) {
        const std::uint32_t n = (static_cast<std::uint32_t>(static_cast<unsigned char>(data[i])) << 16U) |
                                (static_cast<std::uint32_t>(static_cast<unsigned char>(data[i + 1])) << 8U) |
                                static_cast<std::uint32_t>(static_cast<unsigned char>(data[i + 2]));
        out.push_back(kBase64Alphabet[(n >> 18U) & 0x3FU]);
        out.push_back(kBase64Alphabet[(n >> 12U) & 0x3FU]);
        out.push_back(kBase64Alphabet[(n >> 6U) & 0x3FU]);
        out.push_back(kBase64Alphabet[n & 0x3FU]);
    }
    const std::size_t rest = data.size() - i;
    if (rest > 0) {
        std::uint32_t n = static_cast<std::uint32_t>(static_cast<unsigned char>(data[i])) << 16U;
        if (rest == 2) {
            n |= static_cast<std::uint32_t>(static_cast<unsigned char>(data[i + 1])) << 8U;
        }
        out.push_back(kBase64Alphabet[(n >> 18U) & 0x3FU]);
        out.push_back(kBase64Alphabet[(n >> 12U) & 0x3FU]);
        out.push_back(rest == 2 ? kBase64Alphabet[(n >> 6U) & 0x3FU] : '=');
        out.push_back('=');
    }
    return out;
}

std::optional<std::string> base64Decode(std::string_view text) {
    std::string clean;
    clean.reserve(text.size());
    for (const char c : text) {
        if (!isSpace(c)) {
            clean.push_back(c);
        }
    }
    if (clean.size() % 4 != 0) {
        return std::nullopt;
    }
    std::string out;
    out.reserve(clean.size() / 4 * 3);
    for (std::size_t i = 0; i < clean.size(); i += 4) {
        std::uint32_t n = 0;
        int padding = 0;
        for (std::size_t k = 0; k < 4; ++k) {
            const char c = clean[i + k];
            int v = 0;
            if (c == '=') {
                // Padding is only allowed in the last two positions of the final quantum.
                if (i + 4 != clean.size() || k < 2) {
                    return std::nullopt;
                }
                ++padding;
            } else {
                if (padding > 0) {
                    return std::nullopt;
                }
                v = base64Value(c);
                if (v < 0) {
                    return std::nullopt;
                }
            }
            n = (n << 6U) | static_cast<std::uint32_t>(v);
        }
        out.push_back(static_cast<char>((n >> 16U) & 0xFFU));
        if (padding < 2) {
            out.push_back(static_cast<char>((n >> 8U) & 0xFFU));
        }
        if (padding < 1) {
            out.push_back(static_cast<char>(n & 0xFFU));
        }
    }
    return out;
}

std::string urlEncode(std::string_view text) {
    std::string out;
    out.reserve(text.size());
    for (const char c : text) {
        if (isAlnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
            out.push_back(c);
        } else {
            const auto u = static_cast<unsigned char>(c);
            out.push_back('%');
            out.push_back(upper(kHexDigits[u >> 4U]));
            out.push_back(upper(kHexDigits[u & 0x0FU]));
        }
    }
    return out;
}

std::optional<std::string> urlDecode(std::string_view text) {
    std::string out;
    out.reserve(text.size());
    for (std::size_t i = 0; i < text.size(); ++i) {
        const char c = text[i];
        if (c == '%') {
            if (i + 2 >= text.size()) {
                return std::nullopt;
            }
            const int hi = hexValue(text[i + 1]);
            const int lo = hexValue(text[i + 2]);
            if (hi < 0 || lo < 0) {
                return std::nullopt;
            }
            out.push_back(static_cast<char>(hi * 16 + lo));
            i += 2;
        } else if (c == '+') {
            out.push_back(' ');
        } else {
            out.push_back(c);
        }
    }
    return out;
}

std::string escapeHtml(std::string_view text) {
    std::string out;
    out.reserve(text.size());
    for (const char c : text) {
        switch (c) {
            case '&':
                out += "&amp;";
                break;
            case '<':
                out += "&lt;";
                break;
            case '>':
                out += "&gt;";
                break;
            case '"':
                out += "&quot;";
                break;
            case '\'':
                out += "&#39;";
                break;
            default:
                out.push_back(c);
        }
    }
    return out;
}

std::string toHex(std::string_view data) {
    std::string out;
    out.reserve(data.size() * 2);
    for (const char c : data) {
        const auto u = static_cast<unsigned char>(c);
        out.push_back(kHexDigits[u >> 4U]);
        out.push_back(kHexDigits[u & 0x0FU]);
    }
    return out;
}

// ===================================================================================================
// Unicode
// ===================================================================================================

bool Unicode::isValidUtf8(std::string_view text) noexcept {
    return codePointCount(text).has_value();
}

std::optional<std::size_t> Unicode::codePointCount(std::string_view text) noexcept {
    std::size_t count = 0;
    std::size_t i = 0;
    while (i < text.size()) {
        if (!decodeOne(text, i)) {
            return std::nullopt;
        }
        ++count;
    }
    return count;
}

std::string Unicode::encodeUtf8(char32_t cp) {
    std::string out;
    if (cp < 0x80) {
        out.push_back(static_cast<char>(cp));
    } else if (cp < 0x800) {
        out.push_back(static_cast<char>(0xC0U | (cp >> 6U)));
        out.push_back(static_cast<char>(0x80U | (cp & 0x3FU)));
    } else if (cp >= 0xD800 && cp <= 0xDFFF) {
        return {};
    } else if (cp < 0x10000) {
        out.push_back(static_cast<char>(0xE0U | (cp >> 12U)));
        out.push_back(static_cast<char>(0x80U | ((cp >> 6U) & 0x3FU)));
        out.push_back(static_cast<char>(0x80U | (cp & 0x3FU)));
    } else if (cp <= 0x10FFFF) {
        out.push_back(static_cast<char>(0xF0U | (cp >> 18U)));
        out.push_back(static_cast<char>(0x80U | ((cp >> 12U) & 0x3FU)));
        out.push_back(static_cast<char>(0x80U | ((cp >> 6U) & 0x3FU)));
        out.push_back(static_cast<char>(0x80U | (cp & 0x3FU)));
    }
    return out;
}

std::optional<std::u32string> Unicode::decodeUtf8(std::string_view text) {
    std::u32string out;
    std::size_t i = 0;
    while (i < text.size()) {
        const auto cp = decodeOne(text, i);
        if (!cp) {
            return std::nullopt;
        }
        out.push_back(*cp);
    }
    return out;
}

// ===================================================================================================
// StringTemplate / StringPool
// ===================================================================================================

StringTemplate::StringTemplate(std::string_view text) {
    std::size_t pos = 0;
    while (pos < text.size()) {
        const auto open = text.find("{{", pos);
        if (open == std::string_view::npos) {
            parts_.push_back({false, std::string{text.substr(pos)}});
            break;
        }
        if (open > pos) {
            parts_.push_back({false, std::string{text.substr(pos, open - pos)}});
        }
        const auto close = text.find("}}", open + 2);
        if (close == std::string_view::npos) {
            throw std::invalid_argument("StringTemplate: unterminated placeholder");
        }
        const auto name = trimView(text.substr(open + 2, close - open - 2));
        if (name.empty()) {
            throw std::invalid_argument("StringTemplate: empty placeholder");
        }
        parts_.push_back({true, std::string{name}});
        pos = close + 2;
    }
}

std::string StringTemplate::render(const std::unordered_map<std::string, std::string>& values,
                                   bool strict) const {
    std::string out;
    for (const auto& part : parts_) {
        if (!part.isPlaceholder) {
            out += part.text;
            continue;
        }
        const auto it = values.find(part.text);
        if (it != values.end()) {
            out += it->second;
        } else if (strict) {
            throw std::out_of_range("StringTemplate: missing value for '" + part.text + "'");
        } else {
            out += "{{" + part.text + "}}";
        }
    }
    return out;
}

std::vector<std::string> StringTemplate::placeholders() const {
    std::vector<std::string> names;
    for (const auto& part : parts_) {
        if (part.isPlaceholder && std::find(names.begin(), names.end(), part.text) == names.end()) {
            names.push_back(part.text);
        }
    }
    return names;
}

std::string_view StringPool::intern(std::string_view text) {
    const std::lock_guard lock(mutex_);
    return *strings_.emplace(text).first;
}

bool StringPool::contains(std::string_view text) const {
    const std::lock_guard lock(mutex_);
    return strings_.find(std::string{text}) != strings_.end();
}

std::size_t StringPool::size() const {
    const std::lock_guard lock(mutex_);
    return strings_.size();
}

// ===================================================================================================
// Demo
// ===================================================================================================

void demonstrateStrings(std::ostream& out) {
    out << "=== String utilities ===\n";
    out << "trim(\"  warp drive  \")       = '" << trim("  warp drive  ") << "'\n";
    out << "split(\"a,b,,c\", ',')         = [" << join(split("a,b,,c", ','), "|") << "]\n";
    out << "tokenize(move \"Fleet A\" 'x') = [" << join(tokenize(R"(move "Fleet A" 'x y')"), "|") << "]\n";
    out << "toSnakeCase(parseHTTPReply) = " << toSnakeCase("parseHTTPReply") << '\n';
    out << "toCamelCase(warp_core_temp) = " << toCamelCase("warp_core_temp") << '\n';
    out << "levenshtein(kitten,sitting) = " << levenshteinDistance("kitten", "sitting") << '\n';
    out << "LCS(ABCBDAB, BDCABA)        = " << longestCommonSubsequence("ABCBDAB", "BDCABA") << '\n';
    out << "wildcard(nebula.log,*.log)  = " << std::boolalpha << wildcardMatch("nebula.log", "*.log") << '\n';
    out << "isPalindrome(Racecar)       = " << isPalindrome("Racecar") << '\n';
    out << "base64(\"Hello, Mars!\")     = " << base64Encode("Hello, Mars!") << '\n';
    out << "urlEncode(\"a b&c\")         = " << urlEncode("a b&c") << '\n';
    out << "withThousandsSeparator      = " << withThousandsSeparator(299792458) << '\n';
    out << "formatBytes(5'000'000)      = " << formatBytes(5'000'000) << '\n';
    out << "parseNumber<int>(\" 42 \")    = " << parseNumber<int>(" 42 ").value_or(-1) << '\n';
    out << R"(UTF-8 code points in "h\u00e9llo" = )" << Unicode::codePointCount("h\xC3\xA9llo").value_or(0)
        << '\n';

    switch (fnv1a("launch")) {
        case fnv1a("launch"):
            out << "constexpr fnv1a switch dispatched 'launch'\n";
            break;
        default:
            out << "unexpected\n";
            break;
    }

    const StringTemplate tmpl{"Captain {{name}} commands the {{ship}}."};
    out << tmpl.render({{"name", "Vega"}, {"ship", "Endeavour"}}) << '\n';

    StringBuilder sb;
    sb << "Fleet size: " << 12 << ", morale: " << 0.75;
    out << sb.view() << '\n';

    for (const auto& line :
         wordWrap("The quick brown fox jumps over the lazy dog near the orbital station", 24)) {
        out << "  | " << line << '\n';
    }

    StringPool pool;
    const auto a = pool.intern("Andromeda");
    const auto b = pool.intern(std::string{"Andro"} + "meda");
    out << "StringPool shares storage: " << (a.data() == b.data()) << ", unique=" << pool.size() << '\n';
    out << std::noboolalpha;
}

} // namespace CppVerseHub::Utils::String
