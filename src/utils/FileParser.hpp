/**
 * @file FileParser.hpp
 * @brief Hand-written recursive-descent parsers for JSON, CSV (RFC 4180) and a practical XML subset.
 *
 * Demonstrates how to model a recursive document type with `std::variant` (`JsonValue` holds a
 * vector of itself, which the standard permits for incomplete element types), `std::visit`-based
 * serialisation, error reporting with line/column positions via an exception hierarchy, a depth
 * limit that turns stack exhaustion into a clean error, and a small state machine for CSV quoting.
 * Each parser is independent, allocation-conscious (`std::string_view` input) and round-trips its
 * own output. Use nlohmann/json in production; these exist to show the techniques.
 */
#pragma once

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <initializer_list>
#include <iostream>
#include <optional>
#include <ostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

namespace CppVerseHub::Utils {

// ===================================================================================================
// Errors
// ===================================================================================================

/// @brief Base class for all parse errors; carries a 1-based line and column.
class ParseException : public std::runtime_error {
public:
    /**
     * @brief Creates an error.
     * @param message Description (position is appended automatically when line > 0).
     * @param line 1-based line (0 = unknown).
     * @param column 1-based column (0 = unknown).
     */
    ParseException(const std::string& message, std::size_t line = 0, std::size_t column = 0);

    /// @brief Line of the error. @return 1-based line, or 0 if unknown.
    [[nodiscard]] std::size_t line() const noexcept { return line_; }
    /// @brief Column of the error. @return 1-based column, or 0 if unknown.
    [[nodiscard]] std::size_t column() const noexcept { return column_; }

private:
    std::size_t line_;
    std::size_t column_;
};

/// @brief A file could not be opened or read.
class FileNotFoundException : public ParseException {
public:
    /// @brief Creates the error. @param path Offending path.
    explicit FileNotFoundException(const std::filesystem::path& path);
};

/// @brief Malformed JSON.
class JsonParseException : public ParseException {
public:
    using ParseException::ParseException;
};

/// @brief Malformed CSV.
class CsvParseException : public ParseException {
public:
    using ParseException::ParseException;
};

/// @brief Malformed XML.
class XmlParseException : public ParseException {
public:
    using ParseException::ParseException;
};

// ===================================================================================================
// JSON
// ===================================================================================================

/**
 * @brief A JSON document node with value semantics.
 *
 * Objects preserve insertion order (stored as a vector of key/value pairs), which keeps output
 * deterministic and avoids instantiating associative containers with an incomplete type.
 * Numbers are stored as `double` (integers are exact up to 2^53).
 */
class JsonValue {
public:
    /// @brief Kind of value.
    enum class Type : std::uint8_t { Null, Boolean, Number, String, Array, Object };

    using Array = std::vector<JsonValue>;             ///< Array storage.
    using Member = std::pair<std::string, JsonValue>; ///< Object member.
    using Object = std::vector<Member>;               ///< Object storage.

    /// @brief Null value.
    JsonValue() noexcept = default;
    /// @brief Null value. @param n nullptr.
    JsonValue(std::nullptr_t n) noexcept : value_(n) {} // NOLINT(google-explicit-constructor)
    /// @brief Boolean value. @param b Value.
    JsonValue(bool b) noexcept : value_(b) {} // NOLINT(google-explicit-constructor)
    /// @brief Number value. @param d Value.
    JsonValue(double d) noexcept : value_(d) {} // NOLINT(google-explicit-constructor)
    /// @brief Number value from any integer type. @param i Value.
    template <std::integral I>
        requires(!std::same_as<I, bool>)
    JsonValue(I i) noexcept : value_(static_cast<double>(i)) {} // NOLINT(google-explicit-constructor)
    /// @brief String value. @param s Value.
    JsonValue(std::string s) noexcept : value_(std::move(s)) {} // NOLINT(google-explicit-constructor)
    /// @brief String value. @param s Value.
    JsonValue(const char* s) : value_(std::string{s}) {} // NOLINT(google-explicit-constructor)
    /// @brief String value. @param s Value.
    JsonValue(std::string_view s) : value_(std::string{s}) {} // NOLINT(google-explicit-constructor)
    /// @brief Array value. @param a Elements.
    JsonValue(Array a) noexcept : value_(std::move(a)) {} // NOLINT(google-explicit-constructor)
    /// @brief Object value. @param o Members.
    JsonValue(Object o) noexcept : value_(std::move(o)) {} // NOLINT(google-explicit-constructor)

    /// @brief Creates an array. @param items Elements. @return The array.
    [[nodiscard]] static JsonValue array(std::initializer_list<JsonValue> items = {}) { return Array(items); }
    /// @brief Creates an object. @param members Members. @return The object.
    [[nodiscard]] static JsonValue object(std::initializer_list<Member> members = {}) {
        return Object(members);
    }

    /// @brief Kind of value. @return Type.
    [[nodiscard]] Type type() const noexcept { return static_cast<Type>(value_.index()); }
    /// @brief Is null. @return True if null.
    [[nodiscard]] bool isNull() const noexcept { return type() == Type::Null; }
    /// @brief Is boolean. @return True if boolean.
    [[nodiscard]] bool isBool() const noexcept { return type() == Type::Boolean; }
    /// @brief Is number. @return True if number.
    [[nodiscard]] bool isNumber() const noexcept { return type() == Type::Number; }
    /// @brief Is string. @return True if string.
    [[nodiscard]] bool isString() const noexcept { return type() == Type::String; }
    /// @brief Is array. @return True if array.
    [[nodiscard]] bool isArray() const noexcept { return type() == Type::Array; }
    /// @brief Is object. @return True if object.
    [[nodiscard]] bool isObject() const noexcept { return type() == Type::Object; }

    /// @brief Boolean access. @return Value. @throws std::bad_variant_access on type mismatch.
    [[nodiscard]] bool asBool() const { return std::get<bool>(value_); }
    /// @brief Number access. @return Value. @throws std::bad_variant_access on type mismatch.
    [[nodiscard]] double asNumber() const { return std::get<double>(value_); }
    /**
     * @brief Integer access.
     * @return Value.
     * @throws std::bad_variant_access if not a number; std::domain_error if not integral.
     */
    [[nodiscard]] std::int64_t asInt() const;
    /// @brief String access. @return Value. @throws std::bad_variant_access on type mismatch.
    [[nodiscard]] const std::string& asString() const { return std::get<std::string>(value_); }
    /// @brief Array access. @return Elements. @throws std::bad_variant_access on type mismatch.
    [[nodiscard]] const Array& asArray() const { return std::get<Array>(value_); }
    /// @brief Mutable array access. @return Elements. @throws std::bad_variant_access on type mismatch.
    [[nodiscard]] Array& asArray() { return std::get<Array>(value_); }
    /// @brief Object access. @return Members. @throws std::bad_variant_access on type mismatch.
    [[nodiscard]] const Object& asObject() const { return std::get<Object>(value_); }
    /// @brief Mutable object access. @return Members. @throws std::bad_variant_access on type mismatch.
    [[nodiscard]] Object& asObject() { return std::get<Object>(value_); }

    /**
     * @brief Element count for arrays/objects.
     * @return Number of elements or members; 0 for scalars.
     */
    [[nodiscard]] std::size_t size() const noexcept;

    /**
     * @brief Looks up an object member.
     * @param key Member name.
     * @return Pointer to the value, or nullptr if absent or not an object.
     */
    [[nodiscard]] const JsonValue* find(std::string_view key) const noexcept;

    /**
     * @brief Returns whether an object has a member.
     * @param key Member name.
     * @return True if present.
     */
    [[nodiscard]] bool contains(std::string_view key) const noexcept { return find(key) != nullptr; }

    /**
     * @brief Checked object member access.
     * @param key Member name.
     * @return Value.
     * @throws std::out_of_range if absent or not an object.
     */
    [[nodiscard]] const JsonValue& at(std::string_view key) const;

    /**
     * @brief Checked array element access.
     * @param index Index.
     * @return Element.
     * @throws std::out_of_range if out of range or not an array.
     */
    [[nodiscard]] const JsonValue& at(std::size_t index) const;

    /**
     * @brief Object member access, inserting null if absent. Converts a null value into an object.
     * @param key Member name.
     * @return Reference to the member.
     * @throws std::logic_error if the value is neither null nor an object.
     */
    JsonValue& operator[](std::string_view key);

    /**
     * @brief Appends to an array. Converts a null value into an array.
     * @param value Element.
     * @throws std::logic_error if the value is neither null nor an array.
     */
    void push_back(JsonValue value);

    /**
     * @brief Resolves a JSON-Pointer-like dotted path, e.g. "fleet.ships.0.name".
     * @param path Dot-separated keys and array indices.
     * @return Pointer to the value, or nullptr if any segment is missing.
     */
    [[nodiscard]] const JsonValue* findPath(std::string_view path) const noexcept;

    /**
     * @brief Serialises to JSON text.
     * @param indent Spaces per level; negative produces compact single-line output.
     * @return JSON text.
     */
    [[nodiscard]] std::string dump(int indent = -1) const;

    /// @brief Deep equality. @param a Lhs. @param b Rhs. @return True if equal.
    friend bool operator==(const JsonValue& a, const JsonValue& b) { return a.value_ == b.value_; }

private:
    void dumpTo(std::string& out, int indent, int depth) const;

    std::variant<std::nullptr_t, bool, double, std::string, Array, Object> value_{nullptr};
};

/**
 * @brief Strict RFC 8259 JSON parser.
 */
class JsonParser {
public:
    /// @brief Maximum nesting depth accepted before failing (prevents stack exhaustion).
    static constexpr std::size_t kMaxDepth = 256;

    /**
     * @brief Parses a JSON document.
     * @param text JSON text.
     * @return Parsed value.
     * @throws JsonParseException on malformed input (with line/column).
     */
    [[nodiscard]] static JsonValue parse(std::string_view text);

    /**
     * @brief Parses a JSON file.
     * @param path File path.
     * @return Parsed value.
     * @throws FileNotFoundException if unreadable; JsonParseException if malformed.
     */
    [[nodiscard]] static JsonValue parseFile(const std::filesystem::path& path);
};

// ===================================================================================================
// CSV
// ===================================================================================================

/// @brief Parsed CSV table: optional header row plus data rows.
class CsvData {
public:
    using Row = std::vector<std::string>; ///< One record.

    /// @brief Empty table.
    CsvData() = default;

    /**
     * @brief Creates a table.
     * @param headers Header names (may be empty).
     * @param rows Data rows.
     */
    CsvData(Row headers, std::vector<Row> rows) : headers_(std::move(headers)), rows_(std::move(rows)) {}

    /// @brief Header names. @return Headers (empty if none).
    [[nodiscard]] const Row& headers() const noexcept { return headers_; }
    /// @brief Data rows. @return Rows.
    [[nodiscard]] const std::vector<Row>& rows() const noexcept { return rows_; }
    /// @brief Number of data rows. @return Count.
    [[nodiscard]] std::size_t rowCount() const noexcept { return rows_.size(); }
    /// @brief Number of columns (header width, else first row width). @return Count.
    [[nodiscard]] std::size_t columnCount() const noexcept;

    /**
     * @brief Index of a header.
     * @param name Header name.
     * @return Index, or `std::nullopt` if absent.
     */
    [[nodiscard]] std::optional<std::size_t> columnIndex(std::string_view name) const noexcept;

    /**
     * @brief All values of a named column (missing cells become empty strings).
     * @param name Header name.
     * @return Column values.
     * @throws std::out_of_range if the header does not exist.
     */
    [[nodiscard]] std::vector<std::string> column(std::string_view name) const;

    /**
     * @brief Cell by row index and header name.
     * @param row Row index.
     * @param name Header name.
     * @return Cell text.
     * @throws std::out_of_range if row or column does not exist.
     */
    [[nodiscard]] const std::string& cell(std::size_t row, std::string_view name) const;

    /**
     * @brief Appends a row.
     * @param row Row values.
     */
    void addRow(Row row) { rows_.push_back(std::move(row)); }

    /**
     * @brief Serialises to CSV, quoting fields that need it.
     * @param delimiter Field separator.
     * @return CSV text with '\n' line endings.
     */
    [[nodiscard]] std::string toString(char delimiter = ',') const;

private:
    Row headers_;
    std::vector<Row> rows_;
};

/**
 * @brief RFC 4180 CSV parser (quoted fields, doubled quotes, embedded delimiters/newlines, CRLF).
 */
class CsvParser {
public:
    /// @brief Parser configuration.
    struct Options {
        char delimiter = ',';       ///< Field separator.
        char quote = '"';           ///< Quote character.
        bool hasHeader = true;      ///< Treat the first record as header names.
        bool trimFields = false;    ///< Trim whitespace around unquoted fields.
        bool skipEmptyLines = true; ///< Ignore blank lines.
        bool strictColumns = false; ///< Throw if a row's width differs from the header/first row.
    };

    /// @brief Parser with default options.
    CsvParser() = default;

    /**
     * @brief Parser with explicit options.
     * @param options Options.
     */
    explicit CsvParser(Options options) noexcept : options_(options) {}

    /**
     * @brief Parses CSV text.
     * @param text CSV text.
     * @return Table.
     * @throws CsvParseException on an unterminated quote, stray quote or (strict) width mismatch.
     */
    [[nodiscard]] CsvData parse(std::string_view text) const;

    /**
     * @brief Parses a CSV file.
     * @param path File path.
     * @return Table.
     * @throws FileNotFoundException or CsvParseException.
     */
    [[nodiscard]] CsvData parseFile(const std::filesystem::path& path) const;

    /**
     * @brief Escapes a field for output if it contains a delimiter, quote or newline.
     * @param field Field text.
     * @param delimiter Field separator.
     * @return Possibly quoted field.
     */
    [[nodiscard]] static std::string escapeField(std::string_view field, char delimiter = ',');

private:
    Options options_{};
};

// ===================================================================================================
// XML
// ===================================================================================================

/**
 * @brief XML element with attributes, text content and child elements (value semantics).
 */
class XmlNode {
public:
    using Attribute = std::pair<std::string, std::string>; ///< name="value".

    /// @brief Unnamed element.
    XmlNode() = default;

    /**
     * @brief Named element.
     * @param name Tag name.
     */
    explicit XmlNode(std::string name) : name_(std::move(name)) {}

    /// @brief Tag name. @return Name.
    [[nodiscard]] const std::string& name() const noexcept { return name_; }
    /// @brief Concatenated, entity-decoded text content (direct children only). @return Text.
    [[nodiscard]] const std::string& text() const noexcept { return text_; }
    /// @brief Sets text content. @param text Text.
    void setText(std::string text) { text_ = std::move(text); }

    /// @brief Attributes in document order. @return Attributes.
    [[nodiscard]] const std::vector<Attribute>& attributes() const noexcept { return attributes_; }

    /**
     * @brief Looks up an attribute.
     * @param name Attribute name.
     * @return Value, or `std::nullopt`.
     */
    [[nodiscard]] std::optional<std::string> attribute(std::string_view name) const;

    /**
     * @brief Sets (or replaces) an attribute.
     * @param name Attribute name.
     * @param value Value.
     */
    void setAttribute(std::string name, std::string value);

    /// @brief Child elements. @return Children.
    [[nodiscard]] const std::vector<XmlNode>& children() const noexcept { return children_; }

    /**
     * @brief Appends a child.
     * @param child Child element.
     * @return Reference to the stored child.
     */
    XmlNode& addChild(XmlNode child);

    /**
     * @brief First child with a tag name.
     * @param name Tag name.
     * @return Pointer to the child, or nullptr.
     */
    [[nodiscard]] const XmlNode* child(std::string_view name) const noexcept;

    /**
     * @brief All children with a tag name.
     * @param name Tag name.
     * @return Pointers to matching children (valid while this node is unmodified).
     */
    [[nodiscard]] std::vector<const XmlNode*> childrenNamed(std::string_view name) const;

    /**
     * @brief Serialises the element.
     * @param indent Spaces per level (negative = compact).
     * @return XML text (no prolog).
     */
    [[nodiscard]] std::string toString(int indent = 2) const;

    /// @brief Deep equality. @param a Lhs. @param b Rhs. @return True if equal.
    friend bool operator==(const XmlNode& a, const XmlNode& b) = default;

private:
    void write(std::string& out, int indent, int depth) const;

    std::string name_;
    std::string text_;
    std::vector<Attribute> attributes_;
    std::vector<XmlNode> children_;
};

/**
 * @brief Non-validating parser for well-formed XML: prolog, comments, CDATA, processing instructions,
 * self-closing tags, attributes in single/double quotes and the predefined/numeric entities.
 * DTDs are skipped (no entity expansion, so no "billion laughs" exposure).
 */
class XmlParser {
public:
    /// @brief Maximum nesting depth accepted before failing.
    static constexpr std::size_t kMaxDepth = 256;

    /**
     * @brief Parses a document.
     * @param text XML text.
     * @return Root element.
     * @throws XmlParseException on malformed input.
     */
    [[nodiscard]] static XmlNode parse(std::string_view text);

    /**
     * @brief Parses a file.
     * @param path File path.
     * @return Root element.
     * @throws FileNotFoundException or XmlParseException.
     */
    [[nodiscard]] static XmlNode parseFile(const std::filesystem::path& path);

    /**
     * @brief Escapes text for XML content or attribute values.
     * @param text Text.
     * @return Escaped text.
     */
    [[nodiscard]] static std::string escape(std::string_view text);
};

// ===================================================================================================
// File helpers
// ===================================================================================================

namespace FileParserUtils {
/// @brief Supported document formats.
enum class FileFormat : std::uint8_t { Unknown, Json, Csv, Xml, Ini };

/**
 * @brief Detects format from the file extension (case-insensitive).
 * @param path File path.
 * @return Format.
 */
[[nodiscard]] FileFormat detectFormat(const std::filesystem::path& path);

/**
 * @brief Detects format by sniffing content.
 * @param content File content.
 * @return Best guess.
 */
[[nodiscard]] FileFormat detectFormatFromContent(std::string_view content) noexcept;

/**
 * @brief Human-readable format name.
 * @param format Format.
 * @return "JSON", "CSV", "XML", "INI" or "Unknown".
 */
[[nodiscard]] std::string_view toString(FileFormat format) noexcept;

/**
 * @brief Reads an entire file in binary mode.
 * @param path File path.
 * @return Content.
 * @throws FileNotFoundException if the file cannot be opened.
 */
[[nodiscard]] std::string readTextFile(const std::filesystem::path& path);

/**
 * @brief Writes (truncates) a file, creating parent directories.
 * @param path File path.
 * @param content Content.
 * @throws std::runtime_error on failure.
 */
void writeTextFile(const std::filesystem::path& path, std::string_view content);
} // namespace FileParserUtils

/**
 * @brief Runs the parser showcase (in-memory documents only), writing to `out`.
 * @param out Destination stream.
 */
void demonstrateParsing(std::ostream& out = std::cout);

} // namespace CppVerseHub::Utils
