/**
 * @file FileParser.cpp
 * @brief Implementation of the JSON, CSV and XML parsers declared in FileParser.hpp.
 */
#include "utils/FileParser.hpp"

#include "utils/StringUtils.hpp"

#include <algorithm>
#include <charconv>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <locale>
#include <sstream>
#include <system_error>

namespace CppVerseHub::Utils {

namespace {

/// Computes the 1-based line/column of a byte offset.
std::pair<std::size_t, std::size_t> position(std::string_view text, std::size_t offset) noexcept {
    std::size_t line = 1;
    std::size_t column = 1;
    const std::size_t end = std::min(offset, text.size());
    for (std::size_t i = 0; i < end; ++i) {
        if (text[i] == '\n') {
            ++line;
            column = 1;
        } else {
            ++column;
        }
    }
    return {line, column};
}

std::string positionSuffix(std::size_t line, std::size_t column) {
    if (line == 0) {
        return {};
    }
    return " (line " + std::to_string(line) + ", column " + std::to_string(column) + ")";
}

void appendIndent(std::string& out, int indent, int depth) {
    if (indent >= 0) {
        out.push_back('\n');
        out.append(static_cast<std::size_t>(indent) * static_cast<std::size_t>(depth), ' ');
    }
}

void appendJsonString(std::string& out, std::string_view s) {
    static constexpr char kHex[] = "0123456789abcdef";
    out.push_back('"');
    for (const char c : s) {
        switch (c) {
        case '"': out += "\\\""; break;
        case '\\': out += "\\\\"; break;
        case '\b': out += "\\b"; break;
        case '\f': out += "\\f"; break;
        case '\n': out += "\\n"; break;
        case '\r': out += "\\r"; break;
        case '\t': out += "\\t"; break;
        default: {
            const auto u = static_cast<unsigned char>(c);
            if (u < 0x20U) {
                out += "\\u00";
                out.push_back(kHex[u >> 4U]);
                out.push_back(kHex[u & 0x0FU]);
            } else {
                out.push_back(c);
            }
        }
        }
    }
    out.push_back('"');
}

void appendJsonNumber(std::string& out, double d) {
    if (!std::isfinite(d)) {
        out += "null"; // JSON has no NaN/Infinity
        return;
    }
    if (d == std::trunc(d) && std::abs(d) < 1e15) {
        out += std::to_string(static_cast<long long>(d));
        return;
    }
#if defined(__cpp_lib_to_chars) && __cpp_lib_to_chars >= 201611L
    char buffer[32];
    const auto [ptr, ec] = std::to_chars(buffer, buffer + sizeof(buffer), d);
    if (ec == std::errc{}) {
        out.append(buffer, ptr);
        return;
    }
#endif
    // Portable fallback: the shortest of 15..17 significant digits that reads back exactly.
    std::string text;
    for (int precision = 15; precision <= 17; ++precision) {
        std::ostringstream oss;
        oss.imbue(std::locale::classic());
        oss << std::setprecision(precision) << d;
        text = oss.str();
        std::istringstream iss{text};
        iss.imbue(std::locale::classic());
        double back = 0.0;
        iss >> back;
        if (back == d) {
            break;
        }
    }
    out += text;
}

// ---------------------------------------------------------------------------------------------------
// JSON recursive-descent parser
// ---------------------------------------------------------------------------------------------------

class JsonReader {
public:
    explicit JsonReader(std::string_view text) noexcept : text_(text) {}

    JsonValue parseDocument() {
        skipWhitespace();
        JsonValue v = parseValue(0);
        skipWhitespace();
        if (pos_ != text_.size()) {
            fail("unexpected trailing characters");
        }
        return v;
    }

private:
    [[noreturn]] void fail(const std::string& message) const { failAt(message, pos_); }

    [[noreturn]] void failAt(const std::string& message, std::size_t offset) const {
        const auto [line, column] = position(text_, offset);
        throw JsonParseException("JSON: " + message, line, column);
    }

    [[nodiscard]] bool done() const noexcept { return pos_ >= text_.size(); }
    [[nodiscard]] char peek() const noexcept { return done() ? '\0' : text_[pos_]; }

    void skipWhitespace() noexcept {
        while (!done() && (text_[pos_] == ' ' || text_[pos_] == '\t' || text_[pos_] == '\n' || text_[pos_] == '\r')) {
            ++pos_;
        }
    }

    void expectLiteral(std::string_view literal) {
        if (text_.substr(pos_, literal.size()) != literal) {
            fail("invalid literal");
        }
        pos_ += literal.size();
    }

    JsonValue parseValue(std::size_t depth) {
        if (depth > JsonParser::kMaxDepth) {
            fail("maximum nesting depth exceeded");
        }
        switch (peek()) {
        case '{': return parseObject(depth);
        case '[': return parseArray(depth);
        case '"': return JsonValue{parseString()};
        case 't': expectLiteral("true"); return JsonValue{true};
        case 'f': expectLiteral("false"); return JsonValue{false};
        case 'n': expectLiteral("null"); return JsonValue{nullptr};
        case '\0':
            if (done()) {
                fail("unexpected end of input");
            }
            fail("unexpected character");
        default:
            if (peek() == '-' || (peek() >= '0' && peek() <= '9')) {
                return JsonValue{parseNumber()};
            }
            fail(std::string{"unexpected character '"} + peek() + "'");
        }
    }

    JsonValue parseObject(std::size_t depth) {
        ++pos_; // '{'
        JsonValue::Object members;
        skipWhitespace();
        if (peek() == '}') {
            ++pos_;
            return JsonValue{std::move(members)};
        }
        while (true) {
            skipWhitespace();
            if (peek() != '"') {
                fail("expected string key");
            }
            std::string key = parseString();
            skipWhitespace();
            if (peek() != ':') {
                fail("expected ':' after object key");
            }
            ++pos_;
            skipWhitespace();
            JsonValue value = parseValue(depth + 1);
            // Duplicate keys: last one wins (RFC 8259 leaves this implementation-defined).
            const auto it = std::find_if(members.begin(), members.end(), [&key](const auto& m) { return m.first == key; });
            if (it != members.end()) {
                it->second = std::move(value);
            } else {
                members.emplace_back(std::move(key), std::move(value));
            }
            skipWhitespace();
            if (peek() == ',') {
                ++pos_;
                continue;
            }
            if (peek() == '}') {
                ++pos_;
                return JsonValue{std::move(members)};
            }
            fail("expected ',' or '}' in object");
        }
    }

    JsonValue parseArray(std::size_t depth) {
        ++pos_; // '['
        JsonValue::Array items;
        skipWhitespace();
        if (peek() == ']') {
            ++pos_;
            return JsonValue{std::move(items)};
        }
        while (true) {
            skipWhitespace();
            items.push_back(parseValue(depth + 1));
            skipWhitespace();
            if (peek() == ',') {
                ++pos_;
                continue;
            }
            if (peek() == ']') {
                ++pos_;
                return JsonValue{std::move(items)};
            }
            fail("expected ',' or ']' in array");
        }
    }

    unsigned parseHex4() {
        if (pos_ + 4 > text_.size()) {
            fail("truncated \\u escape");
        }
        unsigned value = 0;
        for (int i = 0; i < 4; ++i) {
            const char c = text_[pos_++];
            value <<= 4U;
            if (c >= '0' && c <= '9') {
                value |= static_cast<unsigned>(c - '0');
            } else if (c >= 'a' && c <= 'f') {
                value |= static_cast<unsigned>(c - 'a' + 10);
            } else if (c >= 'A' && c <= 'F') {
                value |= static_cast<unsigned>(c - 'A' + 10);
            } else {
                fail("invalid hex digit in \\u escape");
            }
        }
        return value;
    }

    std::string parseString() {
        ++pos_; // opening quote
        std::string out;
        while (true) {
            if (done()) {
                fail("unterminated string");
            }
            const char c = text_[pos_++];
            if (c == '"') {
                return out;
            }
            if (static_cast<unsigned char>(c) < 0x20U) {
                failAt("control character in string", pos_ - 1);
            }
            if (c != '\\') {
                out.push_back(c);
                continue;
            }
            if (done()) {
                fail("unterminated escape");
            }
            const char e = text_[pos_++];
            switch (e) {
            case '"': out.push_back('"'); break;
            case '\\': out.push_back('\\'); break;
            case '/': out.push_back('/'); break;
            case 'b': out.push_back('\b'); break;
            case 'f': out.push_back('\f'); break;
            case 'n': out.push_back('\n'); break;
            case 'r': out.push_back('\r'); break;
            case 't': out.push_back('\t'); break;
            case 'u': {
                char32_t cp = parseHex4();
                if (cp >= 0xD800 && cp <= 0xDBFF) {
                    if (text_.substr(pos_, 2) != "\\u") {
                        fail("unpaired high surrogate");
                    }
                    pos_ += 2;
                    const unsigned low = parseHex4();
                    if (low < 0xDC00 || low > 0xDFFF) {
                        fail("invalid low surrogate");
                    }
                    cp = 0x10000 + ((cp - 0xD800) << 10U) + (low - 0xDC00);
                } else if (cp >= 0xDC00 && cp <= 0xDFFF) {
                    fail("unpaired low surrogate");
                }
                out += String::Unicode::encodeUtf8(cp);
                break;
            }
            default: failAt("invalid escape sequence", pos_ - 1);
            }
        }
    }

    double parseNumber() {
        const std::size_t start = pos_;
        auto digits = [this] {
            const std::size_t s = pos_;
            while (!done() && text_[pos_] >= '0' && text_[pos_] <= '9') {
                ++pos_;
            }
            return pos_ - s;
        };
        if (peek() == '-') {
            ++pos_;
        }
        if (peek() == '0') {
            ++pos_;
        } else if (digits() == 0) {
            fail("invalid number");
        }
        if (peek() == '.') {
            ++pos_;
            if (digits() == 0) {
                fail("expected digits after decimal point");
            }
        }
        if (peek() == 'e' || peek() == 'E') {
            ++pos_;
            if (peek() == '+' || peek() == '-') {
                ++pos_;
            }
            if (digits() == 0) {
                fail("expected digits in exponent");
            }
        }
        const auto value = String::parseNumber<double>(text_.substr(start, pos_ - start));
        if (!value || !std::isfinite(*value)) {
            failAt("number out of range", start);
        }
        return *value;
    }

    std::string_view text_;
    std::size_t pos_ = 0;
};

// ---------------------------------------------------------------------------------------------------
// XML parser
// ---------------------------------------------------------------------------------------------------

class XmlReader {
public:
    explicit XmlReader(std::string_view text) noexcept : text_(text) {}

    XmlNode parseDocument() {
        skipMisc();
        if (!startsWith("<")) {
            fail("expected root element");
        }
        XmlNode root = parseElement(0);
        skipMisc();
        if (!done()) {
            fail("unexpected content after root element");
        }
        return root;
    }

private:
    [[noreturn]] void fail(const std::string& message) const {
        const auto [line, column] = position(text_, pos_);
        throw XmlParseException("XML: " + message, line, column);
    }

    [[nodiscard]] bool done() const noexcept { return pos_ >= text_.size(); }
    [[nodiscard]] bool startsWith(std::string_view s) const noexcept { return text_.substr(pos_, s.size()) == s; }

    static bool isSpace(char c) noexcept { return c == ' ' || c == '\t' || c == '\n' || c == '\r'; }
    static bool isNameStart(char c) noexcept {
        return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || c == '_' || c == ':' ||
               static_cast<unsigned char>(c) >= 0x80U;
    }
    static bool isNameChar(char c) noexcept {
        return isNameStart(c) || (c >= '0' && c <= '9') || c == '-' || c == '.';
    }

    void skipSpace() noexcept {
        while (!done() && isSpace(text_[pos_])) {
            ++pos_;
        }
    }

    void skipPast(std::string_view terminator, const char* what) {
        const auto end = text_.find(terminator, pos_);
        if (end == std::string_view::npos) {
            fail(std::string{"unterminated "} + what);
        }
        pos_ = end + terminator.size();
    }

    /// Skips whitespace, comments, processing instructions and DOCTYPE declarations.
    void skipMisc() {
        while (true) {
            skipSpace();
            if (startsWith("<?")) {
                skipPast("?>", "processing instruction");
            } else if (startsWith("<!--")) {
                skipPast("-->", "comment");
            } else if (startsWith("<!DOCTYPE")) {
                skipDoctype();
            } else {
                return;
            }
        }
    }

    void skipDoctype() {
        int bracketDepth = 0;
        while (!done()) {
            const char c = text_[pos_++];
            if (c == '[') {
                ++bracketDepth;
            } else if (c == ']') {
                --bracketDepth;
            } else if (c == '>' && bracketDepth <= 0) {
                return;
            }
        }
        fail("unterminated DOCTYPE");
    }

    std::string parseName() {
        if (done() || !isNameStart(text_[pos_])) {
            fail("expected name");
        }
        const std::size_t start = pos_;
        while (!done() && isNameChar(text_[pos_])) {
            ++pos_;
        }
        return std::string{text_.substr(start, pos_ - start)};
    }

    std::string decodeEntities(std::string_view raw) {
        std::string out;
        out.reserve(raw.size());
        for (std::size_t i = 0; i < raw.size(); ++i) {
            if (raw[i] != '&') {
                out.push_back(raw[i]);
                continue;
            }
            const auto semi = raw.find(';', i);
            if (semi == std::string_view::npos) {
                fail("unterminated entity reference");
            }
            const auto entity = raw.substr(i + 1, semi - i - 1);
            if (entity == "lt") {
                out.push_back('<');
            } else if (entity == "gt") {
                out.push_back('>');
            } else if (entity == "amp") {
                out.push_back('&');
            } else if (entity == "quot") {
                out.push_back('"');
            } else if (entity == "apos") {
                out.push_back('\'');
            } else if (entity.size() > 1 && entity[0] == '#') {
                std::uint32_t cp = 0;
                const bool hex = entity[1] == 'x' || entity[1] == 'X';
                const auto digits = entity.substr(hex ? 2 : 1);
                const auto [ptr, ec] = std::from_chars(digits.data(), digits.data() + digits.size(), cp, hex ? 16 : 10);
                if (digits.empty() || ec != std::errc{} || ptr != digits.data() + digits.size()) {
                    fail("invalid character reference");
                }
                const std::string encoded = String::Unicode::encodeUtf8(static_cast<char32_t>(cp));
                if (encoded.empty() && cp != 0) {
                    fail("invalid code point in character reference");
                }
                out += encoded;
            } else {
                fail("unknown entity '&" + std::string{entity} + ";'");
            }
            i = semi;
        }
        return out;
    }

    XmlNode parseElement(std::size_t depth) {
        if (depth > XmlParser::kMaxDepth) {
            fail("maximum nesting depth exceeded");
        }
        ++pos_; // '<'
        XmlNode node{parseName()};
        // Attributes
        while (true) {
            const std::size_t before = pos_;
            skipSpace();
            if (done()) {
                fail("unterminated start tag");
            }
            if (startsWith("/>")) {
                pos_ += 2;
                return node;
            }
            if (text_[pos_] == '>') {
                ++pos_;
                break;
            }
            if (pos_ == before) {
                fail("expected whitespace before attribute");
            }
            std::string attrName = parseName();
            skipSpace();
            if (done() || text_[pos_] != '=') {
                fail("expected '=' after attribute name");
            }
            ++pos_;
            skipSpace();
            if (done() || (text_[pos_] != '"' && text_[pos_] != '\'')) {
                fail("expected quoted attribute value");
            }
            const char quote = text_[pos_++];
            const auto end = text_.find(quote, pos_);
            if (end == std::string_view::npos) {
                fail("unterminated attribute value");
            }
            const auto raw = text_.substr(pos_, end - pos_);
            if (raw.find('<') != std::string_view::npos) {
                fail("'<' not allowed in attribute value");
            }
            if (node.attribute(attrName)) {
                fail("duplicate attribute '" + attrName + "'");
            }
            node.setAttribute(std::move(attrName), decodeEntities(raw));
            pos_ = end + 1;
        }
        // Content
        std::string text;
        while (true) {
            if (done()) {
                fail("unterminated element <" + node.name() + ">");
            }
            if (startsWith("</")) {
                pos_ += 2;
                const std::string closing = parseName();
                if (closing != node.name()) {
                    fail("mismatched closing tag </" + closing + "> for <" + node.name() + ">");
                }
                skipSpace();
                if (done() || text_[pos_] != '>') {
                    fail("expected '>' in closing tag");
                }
                ++pos_;
                break;
            }
            if (startsWith("<!--")) {
                skipPast("-->", "comment");
            } else if (startsWith("<![CDATA[")) {
                pos_ += 9;
                const auto end = text_.find("]]>", pos_);
                if (end == std::string_view::npos) {
                    fail("unterminated CDATA section");
                }
                text += text_.substr(pos_, end - pos_);
                pos_ = end + 3;
            } else if (startsWith("<?")) {
                skipPast("?>", "processing instruction");
            } else if (text_[pos_] == '<') {
                node.addChild(parseElement(depth + 1));
            } else {
                const auto end = text_.find('<', pos_);
                const auto raw = text_.substr(pos_, end == std::string_view::npos ? std::string_view::npos : end - pos_);
                text += decodeEntities(raw);
                pos_ += raw.size();
            }
        }
        node.setText(String::trim(text));
        return node;
    }

    std::string_view text_;
    std::size_t pos_ = 0;
};

} // namespace

// ===================================================================================================
// Exceptions
// ===================================================================================================

ParseException::ParseException(const std::string& message, std::size_t line, std::size_t column)
    : std::runtime_error(message + positionSuffix(line, column)), line_(line), column_(column) {}

FileNotFoundException::FileNotFoundException(const std::filesystem::path& path)
    : ParseException("cannot open file '" + path.string() + "'") {}

// ===================================================================================================
// JsonValue
// ===================================================================================================

std::int64_t JsonValue::asInt() const {
    const double d = asNumber();
    if (d != std::trunc(d) || std::abs(d) > 9.007199254740992e15) {
        throw std::domain_error("JsonValue::asInt: number is not an exactly representable integer");
    }
    return static_cast<std::int64_t>(d);
}

std::size_t JsonValue::size() const noexcept {
    if (const auto* a = std::get_if<Array>(&value_)) {
        return a->size();
    }
    if (const auto* o = std::get_if<Object>(&value_)) {
        return o->size();
    }
    return 0;
}

const JsonValue* JsonValue::find(std::string_view key) const noexcept {
    const auto* o = std::get_if<Object>(&value_);
    if (o == nullptr) {
        return nullptr;
    }
    for (const auto& [k, v] : *o) {
        if (k == key) {
            return &v;
        }
    }
    return nullptr;
}

const JsonValue& JsonValue::at(std::string_view key) const {
    const JsonValue* v = find(key);
    if (v == nullptr) {
        throw std::out_of_range("JsonValue::at: no member '" + std::string{key} + "'");
    }
    return *v;
}

const JsonValue& JsonValue::at(std::size_t index) const {
    const auto* a = std::get_if<Array>(&value_);
    if (a == nullptr || index >= a->size()) {
        throw std::out_of_range("JsonValue::at: index out of range");
    }
    return (*a)[index];
}

JsonValue& JsonValue::operator[](std::string_view key) {
    if (isNull()) {
        value_ = Object{};
    }
    auto* o = std::get_if<Object>(&value_);
    if (o == nullptr) {
        throw std::logic_error("JsonValue::operator[]: value is not an object");
    }
    for (auto& [k, v] : *o) {
        if (k == key) {
            return v;
        }
    }
    o->emplace_back(std::string{key}, JsonValue{});
    return o->back().second;
}

void JsonValue::push_back(JsonValue value) {
    if (isNull()) {
        value_ = Array{};
    }
    auto* a = std::get_if<Array>(&value_);
    if (a == nullptr) {
        throw std::logic_error("JsonValue::push_back: value is not an array");
    }
    a->push_back(std::move(value));
}

const JsonValue* JsonValue::findPath(std::string_view path) const noexcept {
    const JsonValue* current = this;
    std::size_t start = 0;
    while (current != nullptr && start <= path.size()) {
        const auto dot = path.find('.', start);
        const auto segment = path.substr(start, dot == std::string_view::npos ? std::string_view::npos : dot - start);
        if (current->isArray()) {
            std::size_t index = 0;
            const auto [ptr, ec] = std::from_chars(segment.data(), segment.data() + segment.size(), index);
            if (segment.empty() || ec != std::errc{} || ptr != segment.data() + segment.size() ||
                index >= current->size()) {
                return nullptr;
            }
            current = &std::get<Array>(current->value_)[index];
        } else {
            current = current->find(segment);
        }
        if (dot == std::string_view::npos) {
            break;
        }
        start = dot + 1;
    }
    return current;
}

void JsonValue::dumpTo(std::string& out, int indent, int depth) const {
    switch (type()) {
    case Type::Null: out += "null"; break;
    case Type::Boolean: out += std::get<bool>(value_) ? "true" : "false"; break;
    case Type::Number: appendJsonNumber(out, std::get<double>(value_)); break;
    case Type::String: appendJsonString(out, std::get<std::string>(value_)); break;
    case Type::Array: {
        const auto& a = std::get<Array>(value_);
        out.push_back('[');
        for (std::size_t i = 0; i < a.size(); ++i) {
            if (i > 0) {
                out.push_back(',');
            }
            appendIndent(out, indent, depth + 1);
            a[i].dumpTo(out, indent, depth + 1);
        }
        if (!a.empty()) {
            appendIndent(out, indent, depth);
        }
        out.push_back(']');
        break;
    }
    case Type::Object: {
        const auto& o = std::get<Object>(value_);
        out.push_back('{');
        for (std::size_t i = 0; i < o.size(); ++i) {
            if (i > 0) {
                out.push_back(',');
            }
            appendIndent(out, indent, depth + 1);
            appendJsonString(out, o[i].first);
            out += indent >= 0 ? ": " : ":";
            o[i].second.dumpTo(out, indent, depth + 1);
        }
        if (!o.empty()) {
            appendIndent(out, indent, depth);
        }
        out.push_back('}');
        break;
    }
    }
}

std::string JsonValue::dump(int indent) const {
    std::string out;
    dumpTo(out, indent, 0);
    return out;
}

JsonValue JsonParser::parse(std::string_view text) { return JsonReader{text}.parseDocument(); }

JsonValue JsonParser::parseFile(const std::filesystem::path& path) {
    return parse(FileParserUtils::readTextFile(path));
}

// ===================================================================================================
// CSV
// ===================================================================================================

std::size_t CsvData::columnCount() const noexcept {
    if (!headers_.empty()) {
        return headers_.size();
    }
    return rows_.empty() ? 0 : rows_.front().size();
}

std::optional<std::size_t> CsvData::columnIndex(std::string_view name) const noexcept {
    const auto it = std::find(headers_.begin(), headers_.end(), name);
    if (it == headers_.end()) {
        return std::nullopt;
    }
    return static_cast<std::size_t>(it - headers_.begin());
}

std::vector<std::string> CsvData::column(std::string_view name) const {
    const auto index = columnIndex(name);
    if (!index) {
        throw std::out_of_range("CsvData::column: no column '" + std::string{name} + "'");
    }
    std::vector<std::string> values;
    values.reserve(rows_.size());
    for (const auto& row : rows_) {
        values.push_back(*index < row.size() ? row[*index] : std::string{});
    }
    return values;
}

const std::string& CsvData::cell(std::size_t row, std::string_view name) const {
    const auto index = columnIndex(name);
    if (!index || row >= rows_.size() || *index >= rows_[row].size()) {
        throw std::out_of_range("CsvData::cell: no such cell");
    }
    return rows_[row][*index];
}

std::string CsvData::toString(char delimiter) const {
    std::string out;
    auto writeRow = [&out, delimiter](const Row& row) {
        for (std::size_t i = 0; i < row.size(); ++i) {
            if (i > 0) {
                out.push_back(delimiter);
            }
            out += CsvParser::escapeField(row[i], delimiter);
        }
        out.push_back('\n');
    };
    if (!headers_.empty()) {
        writeRow(headers_);
    }
    for (const auto& row : rows_) {
        writeRow(row);
    }
    return out;
}

std::string CsvParser::escapeField(std::string_view field, char delimiter) {
    const bool needsQuotes = field.find_first_of(std::string{delimiter} + "\"\r\n") != std::string_view::npos ||
                             (!field.empty() && (field.front() == ' ' || field.back() == ' '));
    if (!needsQuotes) {
        return std::string{field};
    }
    std::string out = "\"";
    for (const char c : field) {
        if (c == '"') {
            out += "\"\"";
        } else {
            out.push_back(c);
        }
    }
    out.push_back('"');
    return out;
}

CsvData CsvParser::parse(std::string_view text) const {
    std::vector<CsvData::Row> records;
    CsvData::Row record;
    std::string field;
    bool fieldQuoted = false;
    bool inQuotes = false;
    bool afterQuote = false;
    std::size_t quoteStart = 0;

    auto endField = [&] {
        record.push_back(options_.trimFields && !fieldQuoted ? String::trim(field) : std::move(field));
        field.clear();
        fieldQuoted = false;
        afterQuote = false;
    };
    auto endRecord = [&] {
        endField();
        const bool blank = record.size() == 1 && record.front().empty();
        if (!(blank && options_.skipEmptyLines)) {
            records.push_back(std::move(record));
        }
        record.clear();
    };

    for (std::size_t i = 0; i < text.size(); ++i) {
        const char c = text[i];
        if (inQuotes) {
            if (c == options_.quote) {
                if (i + 1 < text.size() && text[i + 1] == options_.quote) {
                    field.push_back(c);
                    ++i;
                } else {
                    inQuotes = false;
                    afterQuote = true;
                }
            } else {
                field.push_back(c);
            }
            continue;
        }
        if (c == options_.delimiter) {
            endField();
        } else if (c == '\n' || c == '\r') {
            if (c == '\r' && i + 1 < text.size() && text[i + 1] == '\n') {
                ++i;
            }
            endRecord();
        } else if (afterQuote) {
            if (c == ' ' || c == '\t') {
                continue; // tolerate padding after a closing quote
            }
            const auto [line, column] = position(text, i);
            throw CsvParseException("CSV: unexpected character after closing quote", line, column);
        } else if (c == options_.quote && field.find_first_not_of(" \t") == std::string::npos) {
            field.clear(); // leading padding before an opening quote is ignored
            inQuotes = true;
            fieldQuoted = true;
            quoteStart = i;
        } else {
            field.push_back(c);
        }
    }
    if (inQuotes) {
        const auto [line, column] = position(text, quoteStart);
        throw CsvParseException("CSV: unterminated quoted field", line, column);
    }
    if (!field.empty() || fieldQuoted || !record.empty()) {
        endRecord();
    }

    CsvData::Row headers;
    std::size_t first = 0;
    if (options_.hasHeader && !records.empty()) {
        headers = std::move(records.front());
        first = 1;
    }
    std::vector<CsvData::Row> rows(std::make_move_iterator(records.begin() + static_cast<std::ptrdiff_t>(first)),
                                   std::make_move_iterator(records.end()));
    if (options_.strictColumns) {
        const std::size_t width = !headers.empty() ? headers.size() : (rows.empty() ? 0 : rows.front().size());
        for (std::size_t r = 0; r < rows.size(); ++r) {
            if (rows[r].size() != width) {
                throw CsvParseException("CSV: row " + std::to_string(r + 1) + " has " + std::to_string(rows[r].size()) +
                                        " fields, expected " + std::to_string(width));
            }
        }
    }
    return CsvData{std::move(headers), std::move(rows)};
}

CsvData CsvParser::parseFile(const std::filesystem::path& path) const {
    return parse(FileParserUtils::readTextFile(path));
}

// ===================================================================================================
// XML
// ===================================================================================================

std::optional<std::string> XmlNode::attribute(std::string_view name) const {
    for (const auto& [k, v] : attributes_) {
        if (k == name) {
            return v;
        }
    }
    return std::nullopt;
}

void XmlNode::setAttribute(std::string name, std::string value) {
    for (auto& [k, v] : attributes_) {
        if (k == name) {
            v = std::move(value);
            return;
        }
    }
    attributes_.emplace_back(std::move(name), std::move(value));
}

XmlNode& XmlNode::addChild(XmlNode child) {
    children_.push_back(std::move(child));
    return children_.back();
}

const XmlNode* XmlNode::child(std::string_view name) const noexcept {
    for (const auto& c : children_) {
        if (c.name_ == name) {
            return &c;
        }
    }
    return nullptr;
}

std::vector<const XmlNode*> XmlNode::childrenNamed(std::string_view name) const {
    std::vector<const XmlNode*> result;
    for (const auto& c : children_) {
        if (c.name_ == name) {
            result.push_back(&c);
        }
    }
    return result;
}

void XmlNode::write(std::string& out, int indent, int depth) const {
    out.push_back('<');
    out += name_;
    for (const auto& [k, v] : attributes_) {
        out += ' ' + k + "=\"" + XmlParser::escape(v) + '"';
    }
    if (text_.empty() && children_.empty()) {
        out += "/>";
        return;
    }
    out.push_back('>');
    out += XmlParser::escape(text_);
    for (const auto& c : children_) {
        appendIndent(out, indent, depth + 1);
        c.write(out, indent, depth + 1);
    }
    if (!children_.empty()) {
        appendIndent(out, indent, depth);
    }
    out += "</" + name_ + '>';
}

std::string XmlNode::toString(int indent) const {
    std::string out;
    write(out, indent, 0);
    return out;
}

XmlNode XmlParser::parse(std::string_view text) { return XmlReader{text}.parseDocument(); }

XmlNode XmlParser::parseFile(const std::filesystem::path& path) { return parse(FileParserUtils::readTextFile(path)); }

std::string XmlParser::escape(std::string_view text) {
    std::string out;
    out.reserve(text.size());
    for (const char c : text) {
        switch (c) {
        case '&': out += "&amp;"; break;
        case '<': out += "&lt;"; break;
        case '>': out += "&gt;"; break;
        case '"': out += "&quot;"; break;
        case '\'': out += "&apos;"; break;
        default: out.push_back(c);
        }
    }
    return out;
}

// ===================================================================================================
// File helpers
// ===================================================================================================

FileParserUtils::FileFormat FileParserUtils::detectFormat(const std::filesystem::path& path) {
    const std::string ext = String::toLower(path.extension().string());
    if (ext == ".json") return FileFormat::Json;
    if (ext == ".csv" || ext == ".tsv") return FileFormat::Csv;
    if (ext == ".xml") return FileFormat::Xml;
    if (ext == ".ini" || ext == ".cfg" || ext == ".conf") return FileFormat::Ini;
    return FileFormat::Unknown;
}

FileParserUtils::FileFormat FileParserUtils::detectFormatFromContent(std::string_view content) noexcept {
    const auto trimmed = String::trimView(content);
    if (trimmed.empty()) {
        return FileFormat::Unknown;
    }
    const char first = trimmed.front();
    if (first == '{' || first == '[') {
        // "[section]" on its own line is INI; JSON arrays rarely look like that.
        if (first == '[') {
            const auto eol = trimmed.find('\n');
            const auto line = String::trimView(trimmed.substr(0, eol));
            const auto close = line.find(']');
            const bool iniHeader = line.size() > 2 && close == line.size() - 1 &&
                                   line.find_first_of("\",{[0123456789", 1) == std::string_view::npos;
            if (iniHeader) {
                return FileFormat::Ini;
            }
        }
        return FileFormat::Json;
    }
    if (first == '<') {
        return FileFormat::Xml;
    }
    const auto eol = trimmed.find('\n');
    const auto firstLine = trimmed.substr(0, eol);
    if (firstLine.find('=') != std::string_view::npos || first == ';' || first == '#') {
        return FileFormat::Ini;
    }
    if (firstLine.find(',') != std::string_view::npos || firstLine.find('\t') != std::string_view::npos) {
        return FileFormat::Csv;
    }
    return FileFormat::Unknown;
}

std::string_view FileParserUtils::toString(FileFormat format) noexcept {
    switch (format) {
    case FileFormat::Json: return "JSON";
    case FileFormat::Csv: return "CSV";
    case FileFormat::Xml: return "XML";
    case FileFormat::Ini: return "INI";
    case FileFormat::Unknown: break;
    }
    return "Unknown";
}

std::string FileParserUtils::readTextFile(const std::filesystem::path& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        throw FileNotFoundException(path);
    }
    std::ostringstream ss;
    ss << in.rdbuf();
    return ss.str();
}

void FileParserUtils::writeTextFile(const std::filesystem::path& path, std::string_view content) {
    if (path.has_parent_path()) {
        std::error_code ec;
        std::filesystem::create_directories(path.parent_path(), ec);
    }
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    if (!out) {
        throw std::runtime_error("writeTextFile: cannot open '" + path.string() + "' for writing");
    }
    out.write(content.data(), static_cast<std::streamsize>(content.size()));
    if (!out) {
        throw std::runtime_error("writeTextFile: write to '" + path.string() + "' failed");
    }
}

// ===================================================================================================
// Demo
// ===================================================================================================

void demonstrateParsing(std::ostream& out) {
    out << "=== Parsers ===\n";
    const std::string jsonText = R"({
        "fleet": "Andromeda",
        "ships": [ {"name": "Vega", "crew": 120}, {"name": "Rigel", "crew": 85.5} ],
        "active": true,
        "motto": "Ad astra ★"
    })";
    const JsonValue doc = JsonParser::parse(jsonText);
    out << "JSON compact: " << doc.dump() << '\n';
    if (const JsonValue* name = doc.findPath("ships.1.name")) {
        out << "ships.1.name = " << name->asString() << '\n';
    }
    double crew = 0.0;
    for (const auto& ship : doc.at("ships").asArray()) {
        crew += ship.at("crew").asNumber();
    }
    out << "total crew = " << crew << '\n';

    JsonValue built;
    built["mission"] = "Survey";
    built["waypoints"].push_back(1);
    built["waypoints"].push_back(2.5);
    out << "Built JSON (indent 2):\n" << built.dump(2) << '\n';

    try {
        (void)JsonParser::parse(R"({"broken": [1, 2,]})");
    } catch (const JsonParseException& e) {
        out << "JsonParseException: " << e.what() << '\n';
    }

    const std::string csvText = "planet,mass,notes\nEarth,5.97e24,\"blue, wet\"\nMars,6.42e23,\"the \"\"red\"\" one\"\n";
    const CsvData table = CsvParser{}.parse(csvText);
    out << "CSV rows=" << table.rowCount() << " cols=" << table.columnCount()
        << " notes[1]=" << table.cell(1, "notes") << '\n';
    out << "CSV round trip:\n" << table.toString();

    const std::string xmlText = R"(<?xml version="1.0"?>
        <!-- star chart -->
        <system name="Sol">
          <planet name="Earth" moons="1">Home &amp; hearth</planet>
          <planet name="Mars" moons="2"><![CDATA[<red>]]></planet>
        </system>)";
    const XmlNode root = XmlParser::parse(xmlText);
    out << "XML <" << root.name() << " name=" << root.attribute("name").value_or("?") << "> has "
        << root.childrenNamed("planet").size() << " planets; Mars text = "
        << root.children().at(1).text() << '\n';
    out << root.toString(2) << '\n';

    out << "detectFormat(config.ini) = " << FileParserUtils::toString(FileParserUtils::detectFormat("config.ini"))
        << ", sniff('<a/>') = " << FileParserUtils::toString(FileParserUtils::detectFormatFromContent("<a/>")) << '\n';
}

} // namespace CppVerseHub::Utils
