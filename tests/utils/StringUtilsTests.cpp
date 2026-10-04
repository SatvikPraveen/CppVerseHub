// Tests for utils/StringUtils.hpp.
#include "utils/StringUtils.hpp"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

#include <limits>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

using namespace CppVerseHub::Utils::String;
using Catch::Approx;
using Strings = std::vector<std::string>;

TEST_CASE("trim variants remove surrounding whitespace", "[utils][string]") {
    static_assert(trimView("  a b \t") == "a b");
    static_assert(trimLeftView("  x ") == "x ");
    static_assert(trimRightView("  x ") == "  x");
    CHECK(trim("\n\t hello \r\n") == "hello");
    CHECK(trim("   ").empty());
    CHECK(trim("").empty());
}

TEST_CASE("split handles empty fields and multi-character delimiters", "[utils][string]") {
    CHECK(split("a,b,,c", ',') == Strings{"a", "b", "", "c"});
    CHECK(split("a,b,,c", ',', true) == Strings{"a", "b", "c"});
    CHECK(split("", ',') == Strings{""});
    CHECK(split(",", ',') == Strings{"", ""});
    CHECK(split("one::two::three", "::") == Strings{"one", "two", "three"});
    CHECK(split("abc", "") == Strings{"abc"});
    CHECK(splitWhitespace("  the \t quick\nfox  ") == Strings{"the", "quick", "fox"});
}

TEST_CASE("join concatenates any range of string-likes", "[utils][string]") {
    CHECK(join(Strings{"a", "b", "c"}, ", ") == "a, b, c");
    CHECK(join(Strings{}, ",").empty());
    const std::vector<std::string_view> views{"x", "y"};
    CHECK(join(views, "") == "xy");
}

TEST_CASE("tokenize honours quotes and escapes", "[utils][string]") {
    CHECK(tokenize(R"(move "Fleet Alpha" --to 'Mars Base')") == Strings{"move", "Fleet Alpha", "--to", "Mars Base"});
    CHECK(tokenize(R"(say "he said \"hi\"" x\ y)") == Strings{"say", "he said \"hi\"", "x y"});
    CHECK(tokenize(R"(empty "" arg)") == Strings{"empty", "", "arg"});
    CHECK(tokenize("   ").empty());
    CHECK_THROWS_AS(tokenize("\"open"), std::invalid_argument);
}

TEST_CASE("Case conversions", "[utils][string]") {
    CHECK(toUpper("Warp 9!") == "WARP 9!");
    CHECK(toLower("Warp 9!") == "warp 9!");
    CHECK(toTitleCase("the QUICK brown-fox") == "The Quick Brown-Fox");
    CHECK(toCamelCase("warp_core_temp") == "warpCoreTemp");
    CHECK(toCamelCase("Hello World") == "helloWorld");
    CHECK(toSnakeCase("parseHTTPResponse") == "parse_http_response");
    CHECK(toSnakeCase("warpCore2Temp") == "warp_core2_temp");
    CHECK(toKebabCase("Hello World_again") == "hello-world-again");
    CHECK(equalsIgnoreCase("NeBuLa", "nebula"));
    CHECK_FALSE(equalsIgnoreCase("nebula", "nebulae"));
}

TEST_CASE("Predicates: contains, countOccurrences, isPalindrome", "[utils][string]") {
    static_assert(contains("starship", "ship"));
    CHECK(countOccurrences("aaaa", "aa") == 2);
    CHECK(countOccurrences("abcabc", "c") == 2);
    CHECK(countOccurrences("abc", "") == 0);
    CHECK(isPalindrome("A man, a plan, a canal: Panama"));
    CHECK(isPalindrome(""));
    CHECK_FALSE(isPalindrome("rocket"));
}

TEST_CASE("wildcardMatch supports * and ?", "[utils][string]") {
    CHECK(wildcardMatch("nebula.log", "*.log"));
    CHECK(wildcardMatch("abc", "a?c"));
    CHECK(wildcardMatch("", "*"));
    CHECK(wildcardMatch("abcde", "a*c*e"));
    CHECK(wildcardMatch("mississippi", "m*iss*ppi"));
    CHECK_FALSE(wildcardMatch("abc", "a?"));
    CHECK_FALSE(wildcardMatch("abc", "*d"));
    CHECK_FALSE(wildcardMatch("", "?"));
}

TEST_CASE("Transformations: replaceAll, repeat, padding, reverse, truncate", "[utils][string]") {
    CHECK(replaceAll("a-b-c", "-", "+") == "a+b+c");
    CHECK(replaceAll("aaa", "a", "aa") == "aaaaaa");
    CHECK(replaceAll("abc", "", "x") == "abc");
    CHECK(repeat("ab", 3) == "ababab");
    CHECK(repeat("ab", 0).empty());
    CHECK(padLeft("7", 3, '0') == "007");
    CHECK(padRight("ab", 4, '.') == "ab..");
    CHECK(padLeft("long", 2) == "long");
    CHECK(center("ab", 7, '*') == "**ab***");
    CHECK(reverse("abc") == "cba");
    CHECK(truncate("Hello, galaxy", 8) == "Hello...");
    CHECK(truncate("short", 10) == "short");
    CHECK(truncate("abcdef", 2) == "..");
}

TEST_CASE("wordWrap breaks lines at the requested width", "[utils][string]") {
    const auto lines = wordWrap("the quick brown fox jumps over the lazy dog", 10);
    CHECK(lines == Strings{"the quick", "brown fox", "jumps over", "the lazy", "dog"});
    for (const auto& line : lines) {
        CHECK(line.size() <= 10);
    }
    CHECK(wordWrap("supercalifragilistic word", 5) == Strings{"supercalifragilistic", "word"});
    CHECK(wordWrap("", 5).empty());
}

TEST_CASE("levenshteinDistance and similarity", "[utils][string]") {
    CHECK(levenshteinDistance("kitten", "sitting") == 3);
    CHECK(levenshteinDistance("", "abc") == 3);
    CHECK(levenshteinDistance("abc", "") == 3);
    CHECK(levenshteinDistance("flaw", "lawn") == 2);
    CHECK(levenshteinDistance("same", "same") == 0);
    CHECK(similarity("", "") == 1.0);
    CHECK(similarity("abcd", "abcf") == Approx(0.75));
    CHECK(similarity("abc", "xyz") == 0.0);
}

TEST_CASE("longestCommonSubsequence finds an optimal subsequence", "[utils][string]") {
    const auto lcs = longestCommonSubsequence("ABCBDAB", "BDCABA");
    CHECK(lcs.size() == 4);
    CHECK(longestCommonSubsequence("AGGTAB", "GXTXAYB") == "GTAB");
    CHECK(longestCommonSubsequence("abc", "def").empty());
}

TEST_CASE("wordFrequency counts case-insensitively", "[utils][string]") {
    const auto freq = wordFrequency("The ship, the crew; THE mission! Don't panic.");
    CHECK(freq.at("the") == 3);
    CHECK(freq.at("ship") == 1);
    CHECK(freq.at("don't") == 1);
    CHECK(freq.count("") == 0);
}

TEST_CASE("fnv1a is a stable constexpr hash", "[utils][string]") {
    static_assert(fnv1a("") == 14695981039346656037ULL);
    static_assert(fnv1a("a") == 0xaf63dc4c8601ec8cULL);
    CHECK(fnv1a("launch") != fnv1a("lunch"));
}

TEST_CASE("parseNumber parses integers strictly", "[utils][string]") {
    CHECK(parseNumber<int>("42") == 42);
    CHECK(parseNumber<int>(" -17 ") == -17);
    CHECK(parseNumber<int>("+8") == 8);
    CHECK(parseNumber<long long>("9223372036854775807") == std::numeric_limits<long long>::max());
    CHECK_FALSE(parseNumber<int>("99999999999").has_value());
    CHECK_FALSE(parseNumber<int>("12abc").has_value());
    CHECK_FALSE(parseNumber<int>("").has_value());
    CHECK_FALSE(parseNumber<unsigned>("-1").has_value());
}

TEST_CASE("parseNumber parses floating point locale-independently", "[utils][string]") {
    CHECK(parseNumber<double>("3.25") == 3.25);
    CHECK(parseNumber<double>("-1e-3") == Approx(-0.001));
    CHECK(parseNumber<double>("+2.5") == 2.5);
    CHECK(parseNumber<float>("0.5") == 0.5F);
    CHECK_FALSE(parseNumber<double>("1.2.3").has_value());
    CHECK_FALSE(parseNumber<double>("abc").has_value());
    CHECK_FALSE(parseNumber<double>("1,5").has_value());
}

TEST_CASE("Number formatting helpers", "[utils][string]") {
    CHECK(formatFixed(3.14159, 2) == "3.14");
    CHECK(formatFixed(2.0, 0) == "2");
    CHECK(withThousandsSeparator(1234567) == "1,234,567");
    CHECK(withThousandsSeparator(-1000) == "-1,000");
    CHECK(withThousandsSeparator(999) == "999");
    CHECK(withThousandsSeparator(0) == "0");
    CHECK(withThousandsSeparator(std::numeric_limits<long long>::min()) == "-9,223,372,036,854,775,808");
    CHECK(withThousandsSeparator(1000000, '.') == "1.000.000");
    CHECK(formatBytes(512) == "512 B");
    CHECK(formatBytes(1536) == "1.50 KiB");
    CHECK(formatBytes(5ULL * 1024 * 1024 * 1024) == "5.00 GiB");
}

TEST_CASE("Base64 matches RFC 4648 test vectors and round-trips", "[utils][string]") {
    const auto [plain, encoded] = GENERATE(table<std::string, std::string>({{"", ""},
                                                                             {"f", "Zg=="},
                                                                             {"fo", "Zm8="},
                                                                             {"foo", "Zm9v"},
                                                                             {"foob", "Zm9vYg=="},
                                                                             {"fooba", "Zm9vYmE="},
                                                                             {"foobar", "Zm9vYmFy"}}));
    CHECK(base64Encode(plain) == encoded);
    CHECK(base64Decode(encoded) == plain);
}

TEST_CASE("Base64 handles binary data and rejects invalid input", "[utils][string]") {
    std::string binary;
    for (int i = 0; i < 256; ++i) {
        binary.push_back(static_cast<char>(i));
    }
    CHECK(base64Decode(base64Encode(binary)) == binary);
    CHECK(base64Decode("Zm9v\nYmFy") == "foobar");
    CHECK_FALSE(base64Decode("Zm9").has_value());
    CHECK_FALSE(base64Decode("Zm=v").has_value());
    CHECK_FALSE(base64Decode("Z===").has_value());
    CHECK_FALSE(base64Decode("Zm9!").has_value());
}

TEST_CASE("URL and HTML escaping", "[utils][string]") {
    CHECK(urlEncode("a b&c/~") == "a%20b%26c%2F~");
    CHECK(urlDecode("a%20b%26c%2f~") == "a b&c/~");
    CHECK(urlDecode("x+y") == "x y");
    CHECK_FALSE(urlDecode("%4").has_value());
    CHECK_FALSE(urlDecode("%zz").has_value());
    const std::string raw = "caf\xC3\xA9 & <tea>";
    CHECK(urlDecode(urlEncode(raw)) == raw);
    CHECK(escapeHtml("<a href=\"x\">'&'</a>") == "&lt;a href=&quot;x&quot;&gt;&#39;&amp;&#39;&lt;/a&gt;");
    CHECK(toHex("\x01\xAB") == "01ab");
}

TEST_CASE("UTF-8 validation, counting and transcoding", "[utils][string][unicode]") {
    using namespace CppVerseHub::Utils::String::Unicode;
    CHECK(isValidUtf8("plain ascii"));
    CHECK(codePointCount("h\xC3\xA9llo") == 5u);
    CHECK(codePointCount("\xF0\x9F\x9A\x80") == 1u); // rocket emoji
    CHECK_FALSE(isValidUtf8("\xC3"));             // truncated
    CHECK_FALSE(isValidUtf8("\xC0\xAF"));         // overlong
    CHECK_FALSE(isValidUtf8("\xED\xA0\x80"));     // surrogate
    CHECK_FALSE(isValidUtf8("\xF4\x90\x80\x80")); // > U+10FFFF
    CHECK(encodeUtf8(U'é') == "\xC3\xA9");
    CHECK(encodeUtf8(U'\U0001F680') == "\xF0\x9F\x9A\x80");
    CHECK(encodeUtf8(0xD800).empty());
    const auto decoded = decodeUtf8("a\xC3\xA9\xF0\x9F\x9A\x80");
    REQUIRE(decoded.has_value());
    CHECK(*decoded == std::u32string{U'a', U'é', U'\U0001F680'});
    CHECK_FALSE(decodeUtf8("\xFF").has_value());
}

TEST_CASE("StringBuilder appends heterogeneous values", "[utils][string]") {
    StringBuilder sb{16};
    CHECK(sb.empty());
    sb << "ships=" << 3 << ' ' << 2.5 << ' ' << true;
    sb.append(std::string{"!"}).appendLine();
    CHECK(sb.view() == "ships=3 2.5 1!\n");
    CHECK(sb.size() == 15);
    std::string moved = std::move(sb).str();
    CHECK(moved == "ships=3 2.5 1!\n");
    StringBuilder other;
    other.append("x");
    other.clear();
    CHECK(other.str().empty());
}

TEST_CASE("StringTemplate renders placeholders", "[utils][string]") {
    const StringTemplate tmpl{"Hello {{ name }}, welcome to {{place}}. Bye {{name}}!"};
    CHECK(tmpl.placeholders() == Strings{"name", "place"});
    CHECK(tmpl.render({{"name", "Ada"}, {"place", "Mars"}}) == "Hello Ada, welcome to Mars. Bye Ada!");
    CHECK(tmpl.render({{"name", "Ada"}}) == "Hello Ada, welcome to {{place}}. Bye Ada!");
    CHECK_THROWS_AS(tmpl.render({{"name", "Ada"}}, true), std::out_of_range);
    CHECK_THROWS_AS(StringTemplate{"oops {{name"}, std::invalid_argument);
    CHECK_THROWS_AS(StringTemplate{"oops {{  }}"}, std::invalid_argument);
    CHECK(StringTemplate{"no placeholders"}.render({}) == "no placeholders");
}

TEST_CASE("StringPool interns strings with stable storage across threads", "[utils][string][threads]") {
    StringPool pool;
    const auto a = pool.intern("orion");
    std::vector<std::thread> threads;
    for (int t = 0; t < 4; ++t) {
        threads.emplace_back([&pool, t] {
            for (int i = 0; i < 100; ++i) {
                (void)pool.intern("name" + std::to_string((i + t) % 50));
            }
        });
    }
    for (auto& th : threads) {
        th.join();
    }
    const auto b = pool.intern(std::string{"ori"} + "on");
    CHECK(a.data() == b.data());
    CHECK(a == "orion");
    CHECK(pool.contains("name0"));
    CHECK_FALSE(pool.contains("missing"));
    CHECK(pool.size() == 51);
}
