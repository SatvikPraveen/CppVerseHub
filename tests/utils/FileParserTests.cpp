// Tests for utils/FileParser.hpp.
#include "utils/FileParser.hpp"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include <filesystem>
#include <string>
#include <variant>

using namespace CppVerseHub::Utils;
using Catch::Approx;
using Catch::Matchers::ContainsSubstring;

TEST_CASE("JsonParser parses scalars", "[utils][json]") {
    CHECK(JsonParser::parse("null").isNull());
    CHECK(JsonParser::parse("true").asBool());
    CHECK_FALSE(JsonParser::parse(" false ").asBool());
    CHECK(JsonParser::parse("-12.5e1").asNumber() == -125.0);
    CHECK(JsonParser::parse("0").asInt() == 0);
    CHECK(JsonParser::parse("\"hi\"").asString() == "hi");
}

TEST_CASE("JsonParser parses nested documents", "[utils][json]") {
    const auto doc = JsonParser::parse(R"({"name": "Vega", "crew": [1, 2, {"x": null}], "ok": true, "e": {}})");
    REQUIRE(doc.isObject());
    CHECK(doc.size() == 4);
    CHECK(doc.at("name").asString() == "Vega");
    CHECK(doc.at("crew").size() == 3);
    CHECK(doc.at("crew").at(1).asInt() == 2);
    CHECK(doc.at("crew").at(2).at("x").isNull());
    CHECK(doc.at("e").isObject());
    CHECK(doc.at("e").size() == 0);
    CHECK(doc.contains("ok"));
    CHECK_FALSE(doc.contains("missing"));
    CHECK(doc.find("missing") == nullptr);
    CHECK_THROWS_AS(doc.at("missing"), std::out_of_range);
    CHECK_THROWS_AS(doc.at("crew").at(9), std::out_of_range);
    CHECK_THROWS_AS(doc.at("name").asNumber(), std::bad_variant_access);
}

TEST_CASE("JsonParser decodes string escapes including surrogate pairs", "[utils][json]") {
    CHECK(JsonParser::parse(R"("a\"b\\c\/d\n\t")").asString() == "a\"b\\c/d\n\t");
    CHECK(JsonParser::parse(R"("\u00e9")").asString() == "\xC3\xA9");
    CHECK(JsonParser::parse(R"("\ud83d\ude80")").asString() == "\xF0\x9F\x9A\x80");
}

TEST_CASE("JsonParser rejects malformed input with positions", "[utils][json]") {
    const auto bad = GENERATE(as<std::string>{}, "", "{", "[1,]", "{\"a\" 1}", "01", "1.", "-", "tru", "\"abc",
                              "\"\\x\"", "[1 2]", "{\"a\":1,}", "\"\\ud800\"", "nul", "1 2", "\"tab\tin\"", "1e");
    CHECK_THROWS_AS(JsonParser::parse(bad), JsonParseException);
}

TEST_CASE("JsonParseException reports line and column", "[utils][json]") {
    try {
        (void)JsonParser::parse("{\n  \"a\": 1,\n  \"b\": ?\n}");
        FAIL("expected an exception");
    } catch (const JsonParseException& e) {
        CHECK(e.line() == 3);
        CHECK(e.column() == 8);
        CHECK_THAT(e.what(), ContainsSubstring("line 3"));
    }
}

TEST_CASE("JsonParser enforces a nesting limit", "[utils][json]") {
    const std::string deep = std::string(JsonParser::kMaxDepth + 10, '[') + std::string(JsonParser::kMaxDepth + 10, ']');
    CHECK_THROWS_AS(JsonParser::parse(deep), JsonParseException);
    const std::string ok = std::string(100, '[') + std::string(100, ']');
    CHECK_NOTHROW(JsonParser::parse(ok));
}

TEST_CASE("JsonValue dump round-trips and formats numbers", "[utils][json]") {
    const std::string text = R"({"a":[1,2.5,-3e-7,true,null],"b":{"c":"x\ny"},"big":1e+300})";
    const auto doc = JsonParser::parse(text);
    const std::string compact = doc.dump();
    CHECK(JsonParser::parse(compact) == doc);
    CHECK(JsonParser::parse(doc.dump(4)) == doc);
    CHECK(JsonValue{42}.dump() == "42");
    CHECK(JsonValue{0.1}.dump() == "0.1");
    CHECK(JsonValue{"q\"uote"}.dump() == R"("q\"uote")");
    CHECK(JsonValue::array().dump(2) == "[]");
    CHECK(JsonValue::object({{"k", 1}}).dump(2) == "{\n  \"k\": 1\n}");
}

TEST_CASE("JsonValue builder API and findPath", "[utils][json]") {
    JsonValue v;
    v["fleet"]["name"] = "Andromeda";
    v["fleet"]["ships"].push_back(JsonValue::object({{"name", "Vega"}}));
    v["fleet"]["ships"].push_back(JsonValue::object({{"name", "Rigel"}}));
    v["fleet"]["name"] = "Andromeda II"; // overwrite keeps position
    CHECK(v.dump() == R"({"fleet":{"name":"Andromeda II","ships":[{"name":"Vega"},{"name":"Rigel"}]}})");
    REQUIRE(v.findPath("fleet.ships.1.name") != nullptr);
    CHECK(v.findPath("fleet.ships.1.name")->asString() == "Rigel");
    CHECK(v.findPath("fleet.ships.5") == nullptr);
    CHECK(v.findPath("fleet.missing") == nullptr);
    CHECK(v.findPath("fleet.ships.x") == nullptr);
    JsonValue scalar{1};
    CHECK_THROWS_AS(scalar["x"], std::logic_error);
    CHECK_THROWS_AS(scalar.push_back(1), std::logic_error);
    CHECK_THROWS_AS(JsonValue{1.5}.asInt(), std::domain_error);
}

TEST_CASE("CsvParser parses RFC 4180 quoting", "[utils][csv]") {
    const std::string text = "name,notes\r\n\"Doe, Jane\",\"said \"\"hi\"\"\"\r\nBob,\"multi\nline\"\r\n";
    const auto data = CsvParser{}.parse(text);
    CHECK(data.headers() == CsvData::Row{"name", "notes"});
    REQUIRE(data.rowCount() == 2);
    CHECK(data.cell(0, "name") == "Doe, Jane");
    CHECK(data.cell(0, "notes") == "said \"hi\"");
    CHECK(data.cell(1, "notes") == "multi\nline");
    CHECK(data.column("name") == std::vector<std::string>{"Doe, Jane", "Bob"});
    CHECK(data.columnIndex("notes") == 1u);
    CHECK_FALSE(data.columnIndex("zzz").has_value());
    CHECK_THROWS_AS(data.column("zzz"), std::out_of_range);
    CHECK_THROWS_AS(data.cell(5, "name"), std::out_of_range);
}

TEST_CASE("CsvParser options: delimiter, no header, trimming, blank lines", "[utils][csv]") {
    CsvParser::Options opts;
    opts.delimiter = ';';
    opts.hasHeader = false;
    opts.trimFields = true;
    const auto data = CsvParser{opts}.parse(" a ; b \n\n c;\" d \"\n");
    REQUIRE(data.rowCount() == 2);
    CHECK(data.headers().empty());
    CHECK(data.rows()[0] == CsvData::Row{"a", "b"});
    CHECK(data.rows()[1] == CsvData::Row{"c", " d "});
    CHECK(data.columnCount() == 2);
    opts.skipEmptyLines = false;
    CHECK(CsvParser{opts}.parse("a\n\nb").rowCount() == 3);
}

TEST_CASE("CsvParser errors", "[utils][csv]") {
    CHECK_THROWS_AS(CsvParser{}.parse("a,b\n\"open,1\n"), CsvParseException);
    CHECK_THROWS_AS(CsvParser{}.parse("a\n\"x\"y\n"), CsvParseException);
    CsvParser::Options strict;
    strict.strictColumns = true;
    CHECK_THROWS_AS(CsvParser{strict}.parse("a,b\n1,2\n3\n"), CsvParseException);
    CHECK_NOTHROW(CsvParser{strict}.parse("a,b\n1,2\n3,4"));
    CHECK(CsvParser{}.parse("").rowCount() == 0);
}

TEST_CASE("CsvData serialisation round-trips", "[utils][csv]") {
    CsvData data{{"id", "text"}, {{"1", "plain"}, {"2", "has,comma"}, {"3", "has \"quote\""}, {"4", "two\nlines"}}};
    data.addRow({"5", " padded "});
    const std::string csv = data.toString();
    const auto parsed = CsvParser{}.parse(csv);
    CHECK(parsed.headers() == data.headers());
    CHECK(parsed.rows() == data.rows());
    CHECK(CsvParser::escapeField("a,b") == "\"a,b\"");
    CHECK(CsvParser::escapeField("x") == "x");
    CHECK(CsvParser::escapeField("tab\there", '\t') == "\"tab\there\"");
}

TEST_CASE("XmlParser parses elements, attributes, entities and CDATA", "[utils][xml]") {
    const auto root = XmlParser::parse(R"(<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE galaxy [ <!ELEMENT galaxy ANY> ]>
<!-- comment -->
<galaxy name='Milky Way' arms="4">
  <star name="Sol" type="G2V">Our &amp; only &#x2605; &#65;</star>
  <star name="Sirius"/>
  <nebula><![CDATA[<raw & unescaped>]]></nebula>
</galaxy>)");
    CHECK(root.name() == "galaxy");
    CHECK(root.attribute("name") == "Milky Way");
    CHECK(root.attribute("arms") == "4");
    CHECK_FALSE(root.attribute("missing").has_value());
    CHECK(root.children().size() == 3);
    const auto stars = root.childrenNamed("star");
    REQUIRE(stars.size() == 2);
    CHECK(stars[0]->text() == "Our & only \xE2\x98\x85 A");
    CHECK(stars[1]->attribute("name") == "Sirius");
    CHECK(stars[1]->children().empty());
    REQUIRE(root.child("nebula") != nullptr);
    CHECK(root.child("nebula")->text() == "<raw & unescaped>");
    CHECK(root.child("planet") == nullptr);
}

TEST_CASE("XmlParser rejects malformed documents", "[utils][xml]") {
    const auto bad = GENERATE(as<std::string>{}, "", "text", "<a>", "<a></b>", "<a x=1/>", "<a x=\"1\" x=\"2\"/>",
                              "<a>&bogus;</a>", "<a/><b/>", "<a><!-- unterminated </a>", "<a x=\"<\"/>", "<1a/>",
                              "<a>&#xZZ;</a>");
    CHECK_THROWS_AS(XmlParser::parse(bad), XmlParseException);
}

TEST_CASE("XmlNode serialisation round-trips", "[utils][xml]") {
    XmlNode root{"fleet"};
    root.setAttribute("name", "A & B");
    XmlNode ship{"ship"};
    ship.setAttribute("id", "1");
    ship.setText("Vega <flagship>");
    root.addChild(ship);
    root.addChild(XmlNode{"ship"}).setAttribute("id", "2");
    root.setAttribute("name", "A & B \"quoted\""); // replace
    CHECK(root.attributes().size() == 1);
    const std::string compact = root.toString(-1);
    CHECK(compact ==
          R"(<fleet name="A &amp; B &quot;quoted&quot;"><ship id="1">Vega &lt;flagship&gt;</ship><ship id="2"/></fleet>)");
    CHECK(XmlParser::parse(compact) == root);
    CHECK(XmlParser::parse(root.toString(2)) == root);
    CHECK(XmlParser::escape("<'&'>") == "&lt;&apos;&amp;&apos;&gt;");
}

TEST_CASE("Format detection by extension and content", "[utils][parser]") {
    using namespace FileParserUtils;
    CHECK(detectFormat("a/b/config.JSON") == FileFormat::Json);
    CHECK(detectFormat("data.csv") == FileFormat::Csv);
    CHECK(detectFormat("map.xml") == FileFormat::Xml);
    CHECK(detectFormat("game.ini") == FileFormat::Ini);
    CHECK(detectFormat("README") == FileFormat::Unknown);
    CHECK(detectFormatFromContent("  {\"a\": 1}") == FileFormat::Json);
    CHECK(detectFormatFromContent("[1, 2]") == FileFormat::Json);
    CHECK(detectFormatFromContent("[graphics]\nwidth=1") == FileFormat::Ini);
    CHECK(detectFormatFromContent("<root/>") == FileFormat::Xml);
    CHECK(detectFormatFromContent("a,b,c\n1,2,3") == FileFormat::Csv);
    CHECK(detectFormatFromContent("key = value") == FileFormat::Ini);
    CHECK(detectFormatFromContent("") == FileFormat::Unknown);
    CHECK(toString(FileFormat::Xml) == "XML");
}

TEST_CASE("File helpers read and write files in a temp directory", "[utils][parser][file]") {
    const auto dir = std::filesystem::temp_directory_path() / "cppversehub_utils_parser_test";
    std::filesystem::remove_all(dir);
    const auto jsonPath = dir / "nested" / "doc.json";
    FileParserUtils::writeTextFile(jsonPath, R"({"x": [1, 2, 3]})");
    CHECK(FileParserUtils::readTextFile(jsonPath) == R"({"x": [1, 2, 3]})");
    CHECK(JsonParser::parseFile(jsonPath).at("x").size() == 3);

    const auto csvPath = dir / "t.csv";
    FileParserUtils::writeTextFile(csvPath, "a,b\n1,2\n");
    CHECK(CsvParser{}.parseFile(csvPath).cell(0, "b") == "2");

    const auto xmlPath = dir / "t.xml";
    FileParserUtils::writeTextFile(xmlPath, "<r><c/></r>");
    CHECK(XmlParser::parseFile(xmlPath).children().size() == 1);

    CHECK_THROWS_AS(FileParserUtils::readTextFile(dir / "missing.txt"), FileNotFoundException);
    CHECK_THROWS_AS(JsonParser::parseFile(dir / "missing.json"), ParseException);
    std::filesystem::remove_all(dir);
    CHECK_FALSE(std::filesystem::exists(dir));
}
