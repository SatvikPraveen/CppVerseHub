/**
 * @file StringUtils.hpp
 * @brief String processing toolkit built on `std::string_view`, `<charconv>` and C++20 ranges idioms.
 *
 * Demonstrates: non-owning `std::string_view` parameters to avoid copies; locale-independent number
 * parsing with `std::from_chars`; `constexpr` hashing (FNV-1a) usable in `switch` labels; classic
 * dynamic-programming text algorithms (Levenshtein distance, wildcard matching); binary-to-text
 * encodings (Base64, percent-encoding); UTF-8 validation; and small value types (`StringBuilder`,
 * `StringTemplate`, `StringPool`) that show RAII buffer management and string interning.
 *
 * All case conversions are ASCII-only by design (locale-independent and deterministic).
 */
#pragma once

#include <charconv>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <map>
#include <mutex>
#include <optional>
#include <ostream>
#include <sstream>
#include <string>
#include <string_view>
#include <system_error>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace CppVerseHub::Utils::String {

// ===================================================================================================
// Trimming, splitting, joining
// ===================================================================================================

/// @brief Characters treated as whitespace by the trimming functions.
inline constexpr std::string_view kWhitespace = " \t\n\r\f\v";

/**
 * @brief Returns a view of `text` without leading whitespace.
 * @param text Input.
 * @return Trimmed view (aliases `text`).
 */
[[nodiscard]] constexpr std::string_view trimLeftView(std::string_view text) noexcept {
    const auto pos = text.find_first_not_of(kWhitespace);
    return pos == std::string_view::npos ? std::string_view{} : text.substr(pos);
}

/**
 * @brief Returns a view of `text` without trailing whitespace.
 * @param text Input.
 * @return Trimmed view (aliases `text`).
 */
[[nodiscard]] constexpr std::string_view trimRightView(std::string_view text) noexcept {
    const auto pos = text.find_last_not_of(kWhitespace);
    return pos == std::string_view::npos ? std::string_view{} : text.substr(0, pos + 1);
}

/**
 * @brief Returns a view of `text` without surrounding whitespace.
 * @param text Input.
 * @return Trimmed view (aliases `text`).
 */
[[nodiscard]] constexpr std::string_view trimView(std::string_view text) noexcept {
    return trimRightView(trimLeftView(text));
}

/**
 * @brief Owning variant of `trimView`.
 * @param text Input.
 * @return Trimmed copy.
 */
[[nodiscard]] std::string trim(std::string_view text);

/**
 * @brief Splits on a single character.
 * @param text Input.
 * @param delimiter Separator.
 * @param skipEmpty Whether to drop empty fields.
 * @return Fields.
 */
[[nodiscard]] std::vector<std::string> split(std::string_view text, char delimiter, bool skipEmpty = false);

/**
 * @brief Splits on a (multi-character) delimiter.
 * @param text Input.
 * @param delimiter Separator (an empty delimiter returns `{text}`).
 * @param skipEmpty Whether to drop empty fields.
 * @return Fields.
 */
[[nodiscard]] std::vector<std::string> split(std::string_view text, std::string_view delimiter,
                                             bool skipEmpty = false);

/**
 * @brief Splits on runs of whitespace (never yields empty fields).
 * @param text Input.
 * @return Words.
 */
[[nodiscard]] std::vector<std::string> splitWhitespace(std::string_view text);

/**
 * @brief Joins a range of string-like values.
 * @tparam Range Any range whose elements convert to `std::string_view`.
 * @param parts Elements.
 * @param separator Separator inserted between elements.
 * @return Joined string.
 */
template <typename Range>
[[nodiscard]] std::string join(const Range& parts, std::string_view separator) {
    std::string out;
    bool first = true;
    for (const auto& part : parts) {
        if (!first) {
            out += separator;
        }
        out += std::string_view{part};
        first = false;
    }
    return out;
}

/**
 * @brief Tokenises a command line, honouring single/double quotes and backslash escapes.
 *
 * `move "Fleet Alpha" --to 'Mars Base'` -> {move, Fleet Alpha, --to, Mars Base}.
 * @param text Input.
 * @return Tokens.
 * @throws std::invalid_argument on an unterminated quote.
 */
[[nodiscard]] std::vector<std::string> tokenize(std::string_view text);

// ===================================================================================================
// Case and predicates
// ===================================================================================================

/**
 * @brief ASCII upper-case conversion.
 * @param text Input.
 * @return Upper-cased copy.
 */
[[nodiscard]] std::string toUpper(std::string_view text);

/**
 * @brief ASCII lower-case conversion.
 * @param text Input.
 * @return Lower-cased copy.
 */
[[nodiscard]] std::string toLower(std::string_view text);

/**
 * @brief Upper-cases the first letter of every word, lower-cases the rest.
 * @param text Input.
 * @return Title-cased copy.
 */
[[nodiscard]] std::string toTitleCase(std::string_view text);

/**
 * @brief Converts "hello_world", "Hello World" or "hello-world" to "helloWorld".
 * @param text Input.
 * @return camelCase copy.
 */
[[nodiscard]] std::string toCamelCase(std::string_view text);

/**
 * @brief Converts "helloWorld", "Hello World" or "hello-world" to "hello_world".
 * @param text Input.
 * @return snake_case copy.
 */
[[nodiscard]] std::string toSnakeCase(std::string_view text);

/**
 * @brief Converts to "hello-world" form.
 * @param text Input.
 * @return kebab-case copy.
 */
[[nodiscard]] std::string toKebabCase(std::string_view text);

/**
 * @brief Case-insensitive (ASCII) equality.
 * @param a First string.
 * @param b Second string.
 * @return True if equal ignoring case.
 */
[[nodiscard]] bool equalsIgnoreCase(std::string_view a, std::string_view b) noexcept;

/**
 * @brief Substring test.
 * @param text Haystack.
 * @param needle Needle.
 * @return True if `needle` occurs in `text`.
 */
[[nodiscard]] constexpr bool contains(std::string_view text, std::string_view needle) noexcept {
    return text.find(needle) != std::string_view::npos;
}

/**
 * @brief Counts non-overlapping occurrences.
 * @param text Haystack.
 * @param needle Needle (empty needle returns 0).
 * @return Occurrence count.
 */
[[nodiscard]] std::size_t countOccurrences(std::string_view text, std::string_view needle) noexcept;

/**
 * @brief Palindrome test ignoring case and non-alphanumeric characters.
 * @param text Input.
 * @return True for palindromes ("A man, a plan, a canal: Panama").
 */
[[nodiscard]] bool isPalindrome(std::string_view text) noexcept;

/**
 * @brief Glob-style matching with `*` (any run) and `?` (any single char). Linear-time greedy algorithm.
 * @param text Input.
 * @param pattern Pattern.
 * @return True if the whole of `text` matches.
 */
[[nodiscard]] bool wildcardMatch(std::string_view text, std::string_view pattern) noexcept;

// ===================================================================================================
// Transformations
// ===================================================================================================

/**
 * @brief Replaces every non-overlapping occurrence of `from` with `to`.
 * @param text Input.
 * @param from Pattern (empty pattern returns `text` unchanged).
 * @param to Replacement.
 * @return Modified copy.
 */
[[nodiscard]] std::string replaceAll(std::string_view text, std::string_view from, std::string_view to);

/**
 * @brief Repeats a string.
 * @param text Input.
 * @param times Repetition count.
 * @return Concatenation of `times` copies.
 */
[[nodiscard]] std::string repeat(std::string_view text, std::size_t times);

/**
 * @brief Left-pads to `width` characters.
 * @param text Input.
 * @param width Target width.
 * @param fill Fill character.
 * @return Padded copy (unchanged if already wide enough).
 */
[[nodiscard]] std::string padLeft(std::string_view text, std::size_t width, char fill = ' ');

/**
 * @brief Right-pads to `width` characters.
 * @param text Input.
 * @param width Target width.
 * @param fill Fill character.
 * @return Padded copy.
 */
[[nodiscard]] std::string padRight(std::string_view text, std::size_t width, char fill = ' ');

/**
 * @brief Centres within `width` characters (extra padding goes right).
 * @param text Input.
 * @param width Target width.
 * @param fill Fill character.
 * @return Centred copy.
 */
[[nodiscard]] std::string center(std::string_view text, std::size_t width, char fill = ' ');

/**
 * @brief Reverses byte order.
 * @param text Input.
 * @return Reversed copy.
 */
[[nodiscard]] std::string reverse(std::string_view text);

/**
 * @brief Greedy word wrap.
 * @param text Input (whitespace is normalised).
 * @param width Maximum line width (words longer than `width` occupy their own line).
 * @return Lines.
 */
[[nodiscard]] std::vector<std::string> wordWrap(std::string_view text, std::size_t width);

/**
 * @brief Truncates to `maxLength` characters, appending `ellipsis` if truncated.
 * @param text Input.
 * @param maxLength Maximum total length including the ellipsis.
 * @param ellipsis Suffix to append.
 * @return Possibly truncated copy.
 */
[[nodiscard]] std::string truncate(std::string_view text, std::size_t maxLength,
                                   std::string_view ellipsis = "...");

// ===================================================================================================
// Algorithms
// ===================================================================================================

/**
 * @brief Levenshtein edit distance using an O(min(n, m)) rolling row.
 * @param a First string.
 * @param b Second string.
 * @return Minimum number of single-character insertions, deletions and substitutions.
 */
[[nodiscard]] std::size_t levenshteinDistance(std::string_view a, std::string_view b);

/**
 * @brief Normalised similarity in [0, 1] derived from Levenshtein distance.
 * @param a First string.
 * @param b Second string.
 * @return 1 for identical strings, 0 for completely different ones.
 */
[[nodiscard]] double similarity(std::string_view a, std::string_view b);

/**
 * @brief Longest common subsequence via dynamic programming.
 * @param a First string.
 * @param b Second string.
 * @return One longest common subsequence.
 */
[[nodiscard]] std::string longestCommonSubsequence(std::string_view a, std::string_view b);

/**
 * @brief Counts lower-cased alphanumeric words.
 * @param text Input.
 * @return Word -> frequency, ordered by word.
 */
[[nodiscard]] std::map<std::string, std::size_t> wordFrequency(std::string_view text);

/**
 * @brief 64-bit FNV-1a hash, usable at compile time (e.g. `switch (fnv1a(s)) { case fnv1a("x"): }`).
 * @param text Input.
 * @return Hash value.
 */
[[nodiscard]] constexpr std::uint64_t fnv1a(std::string_view text) noexcept {
    std::uint64_t hash = 14695981039346656037ULL;
    for (const char c : text) {
        hash ^= static_cast<std::uint8_t>(c);
        hash *= 1099511628211ULL;
    }
    return hash;
}

// ===================================================================================================
// Numbers
// ===================================================================================================

/**
 * @brief Locale-independent, whole-string number parsing (surrounding whitespace allowed).
 * @tparam T Integral or floating-point type.
 * @param text Input such as "42", "-7", "3.25", "1e-3".
 * @return The value, or `std::nullopt` on malformed input or overflow.
 */
template <typename T>
    requires(std::integral<T> || std::floating_point<T>) && (!std::same_as<T, bool>)
[[nodiscard]] std::optional<T> parseNumber(std::string_view text) noexcept;

/// @cond INTERNAL
namespace detail {
[[nodiscard]] std::optional<double> parseDouble(std::string_view text) noexcept;
} // namespace detail
/// @endcond

template <typename T>
    requires(std::integral<T> || std::floating_point<T>) && (!std::same_as<T, bool>)
std::optional<T> parseNumber(std::string_view text) noexcept {
    text = trimView(text);
    if (text.empty()) {
        return std::nullopt;
    }
    if constexpr (std::integral<T>) {
        if (text.front() == '+') {
            text.remove_prefix(1);
        }
        T value{};
        const auto [ptr, ec] = std::from_chars(text.data(), text.data() + text.size(), value);
        if (ec != std::errc{} || ptr != text.data() + text.size()) {
            return std::nullopt;
        }
        return value;
    } else {
        const auto d = detail::parseDouble(text);
        if (!d) {
            return std::nullopt;
        }
        return static_cast<T>(*d);
    }
}

/**
 * @brief Formats a value with fixed precision (locale-independent).
 * @param value Number.
 * @param precision Digits after the decimal point.
 * @return Text.
 */
[[nodiscard]] std::string formatFixed(double value, int precision);

/**
 * @brief Inserts thousands separators: 1234567 -> "1,234,567".
 * @param value Number.
 * @param separator Separator character.
 * @return Text.
 */
[[nodiscard]] std::string withThousandsSeparator(long long value, char separator = ',');

/**
 * @brief Formats a byte count using binary prefixes: 1536 -> "1.50 KiB".
 * @param bytes Byte count.
 * @return Text.
 */
[[nodiscard]] std::string formatBytes(std::uint64_t bytes);

// ===================================================================================================
// Encodings
// ===================================================================================================

/**
 * @brief Standard Base64 encoding (RFC 4648) with padding.
 * @param data Bytes.
 * @return Encoded text.
 */
[[nodiscard]] std::string base64Encode(std::string_view data);

/**
 * @brief Base64 decoding. Whitespace is ignored.
 * @param text Encoded text.
 * @return Decoded bytes, or `std::nullopt` on invalid input.
 */
[[nodiscard]] std::optional<std::string> base64Decode(std::string_view text);

/**
 * @brief Percent-encodes everything except RFC 3986 unreserved characters.
 * @param text Input.
 * @return Encoded text.
 */
[[nodiscard]] std::string urlEncode(std::string_view text);

/**
 * @brief Decodes percent-escapes and '+' (as space).
 * @param text Encoded text.
 * @return Decoded text, or `std::nullopt` on a malformed escape.
 */
[[nodiscard]] std::optional<std::string> urlDecode(std::string_view text);

/**
 * @brief Escapes `& < > " '` for HTML/XML text.
 * @param text Input.
 * @return Escaped text.
 */
[[nodiscard]] std::string escapeHtml(std::string_view text);

/**
 * @brief Lower-case hexadecimal dump of bytes.
 * @param data Bytes.
 * @return Two hex digits per byte.
 */
[[nodiscard]] std::string toHex(std::string_view data);

// ===================================================================================================
// Unicode
// ===================================================================================================

namespace Unicode {
/**
 * @brief Validates UTF-8 (rejects overlong forms, surrogates and code points > U+10FFFF).
 * @param text Bytes.
 * @return True if valid.
 */
[[nodiscard]] bool isValidUtf8(std::string_view text) noexcept;

/**
 * @brief Counts code points in valid UTF-8.
 * @param text Bytes.
 * @return Code point count, or `std::nullopt` if invalid.
 */
[[nodiscard]] std::optional<std::size_t> codePointCount(std::string_view text) noexcept;

/**
 * @brief Encodes one code point as UTF-8.
 * @param codePoint Code point.
 * @return Encoded bytes (empty for invalid code points).
 */
[[nodiscard]] std::string encodeUtf8(char32_t codePoint);

/**
 * @brief Decodes valid UTF-8 into code points.
 * @param text Bytes.
 * @return Code points, or `std::nullopt` if invalid.
 */
[[nodiscard]] std::optional<std::u32string> decodeUtf8(std::string_view text);
} // namespace Unicode

// ===================================================================================================
// Value types
// ===================================================================================================

/**
 * @brief Fluent string builder with stream-style insertion and amortised growth.
 */
class StringBuilder {
public:
    StringBuilder() = default;

    /**
     * @brief Pre-allocates capacity.
     * @param capacity Bytes to reserve.
     */
    explicit StringBuilder(std::size_t capacity) { buffer_.reserve(capacity); }

    /**
     * @brief Appends text.
     * @param text Text.
     * @return `*this`.
     */
    StringBuilder& append(std::string_view text) {
        buffer_ += text;
        return *this;
    }

    /**
     * @brief Appends a character.
     * @param c Character.
     * @return `*this`.
     */
    StringBuilder& append(char c) {
        buffer_.push_back(c);
        return *this;
    }

    /**
     * @brief Appends any streamable value.
     * @param value Value.
     * @return `*this`.
     */
    template <typename T>
        requires(!std::convertible_to<const T&, std::string_view>) && (!std::same_as<T, char>)
    StringBuilder& append(const T& value) {
        if constexpr (std::integral<T> && !std::same_as<T, bool>) {
            buffer_ += std::to_string(value);
        } else {
            std::ostringstream oss;
            oss << value;
            buffer_ += oss.str();
        }
        return *this;
    }

    /**
     * @brief Appends text followed by '\n'.
     * @param text Text.
     * @return `*this`.
     */
    StringBuilder& appendLine(std::string_view text = {}) {
        buffer_ += text;
        buffer_.push_back('\n');
        return *this;
    }

    /**
     * @brief Stream-style append.
     * @param value Value.
     * @return `*this`.
     */
    template <typename T>
    StringBuilder& operator<<(const T& value) {
        return append(value);
    }

    /// @brief Returns the current length.
    /// @return Length in bytes.
    [[nodiscard]] std::size_t size() const noexcept { return buffer_.size(); }

    /// @brief Returns whether the builder is empty.
    /// @return True if empty.
    [[nodiscard]] bool empty() const noexcept { return buffer_.empty(); }

    /// @brief Clears the content but keeps capacity.
    void clear() noexcept { buffer_.clear(); }

    /// @brief Returns a view of the content.
    /// @return View valid until the next mutation.
    [[nodiscard]] std::string_view view() const noexcept { return buffer_; }

    /// @brief Returns a copy of the content.
    /// @return Content.
    [[nodiscard]] std::string str() const& { return buffer_; }

    /// @brief Moves the content out.
    /// @return Content.
    [[nodiscard]] std::string str() && { return std::move(buffer_); }

private:
    std::string buffer_;
};

/**
 * @brief Minimal "{{name}}" template engine. Templates are parsed once into literal/placeholder parts.
 */
class StringTemplate {
public:
    /**
     * @brief Parses a template.
     * @param text Template text; `{{ name }}` placeholders (whitespace inside braces ignored).
     * @throws std::invalid_argument on an unterminated or empty placeholder.
     */
    explicit StringTemplate(std::string_view text);

    /**
     * @brief Renders the template.
     * @param values Placeholder values.
     * @param strict If true, a missing value throws; otherwise the placeholder is left verbatim.
     * @return Rendered text.
     * @throws std::out_of_range in strict mode when a value is missing.
     */
    [[nodiscard]] std::string render(const std::unordered_map<std::string, std::string>& values,
                                     bool strict = false) const;

    /// @brief Returns placeholder names in order of first appearance (no duplicates).
    /// @return Names.
    [[nodiscard]] std::vector<std::string> placeholders() const;

private:
    struct Part {
        bool isPlaceholder = false;
        std::string text;
    };
    std::vector<Part> parts_;
};

/**
 * @brief Thread-safe string interning: equal strings share one stable allocation.
 *
 * Returned views remain valid for the lifetime of the pool (node-based storage never relocates).
 */
class StringPool {
public:
    /**
     * @brief Interns a string.
     * @param text Text.
     * @return Stable view of the pooled copy.
     */
    [[nodiscard]] std::string_view intern(std::string_view text);

    /**
     * @brief Returns whether a string is pooled.
     * @param text Text.
     * @return True if present.
     */
    [[nodiscard]] bool contains(std::string_view text) const;

    /// @brief Returns the number of unique strings.
    /// @return Count.
    [[nodiscard]] std::size_t size() const;

private:
    mutable std::mutex mutex_;
    std::unordered_set<std::string> strings_;
};

/**
 * @brief Runs the string utilities showcase, writing only to `out`.
 * @param out Destination stream.
 */
void demonstrateStrings(std::ostream& out = std::cout);

} // namespace CppVerseHub::Utils::String
