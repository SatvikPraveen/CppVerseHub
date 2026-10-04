/**
 * @file STLUtilities.cpp
 * @brief Implementation of the vocabulary-type showcase.
 */
#include "stl_showcase/STLUtilities.hpp"

#include <algorithm>
#include <array>
#include <charconv>
#include <cmath>
#include <limits>
#include <locale>
#include <numeric>
#include <system_error>

namespace CppVerseHub::STL {

double NavigationCoordinate::distanceTo(const NavigationCoordinate& other) const noexcept {
    const double dx = other.x - x;
    const double dy = other.y - y;
    const double dz = other.z - z;
    return std::sqrt(dx * dx + dy * dy + dz * dz);
}

// --------------------------------------------------------------------------------- pair/tuple

std::optional<std::pair<std::size_t, std::size_t>> closestPairIndices(std::span<const NavigationCoordinate> points) {
    if (points.size() < 2) {
        return std::nullopt;
    }
    std::pair<std::size_t, std::size_t> best{0, 1};
    double best_distance = points[0].distanceTo(points[1]);
    for (std::size_t i = 0; i < points.size(); ++i) {
        for (std::size_t j = i + 1; j < points.size(); ++j) {
            if (const double d = points[i].distanceTo(points[j]); d < best_distance) {
                best_distance = d;
                best = {i, j};
            }
        }
    }
    return best;
}

std::optional<std::pair<double, double>> distanceRangeFromOrigin(std::span<const NavigationCoordinate> points) {
    if (points.empty()) {
        return std::nullopt;
    }
    constexpr NavigationCoordinate origin{};
    const auto [nearest, farthest] = std::ranges::minmax_element(
        points, {}, [&origin](const NavigationCoordinate& p) { return p.distanceTo(origin); });
    return std::pair{nearest->distanceTo(origin), farthest->distanceTo(origin)};
}

std::optional<CoordinateStats> coordinateStatistics(std::span<const NavigationCoordinate> points) {
    if (points.empty()) {
        return std::nullopt;
    }
    const auto sum = std::accumulate(points.begin(), points.end(), NavigationCoordinate{},
                                     [](NavigationCoordinate acc, const NavigationCoordinate& p) {
                                         return NavigationCoordinate{acc.x + p.x, acc.y + p.y, acc.z + p.z};
                                     });
    const auto n = static_cast<double>(points.size());
    const NavigationCoordinate centroid{sum.x / n, sum.y / n, sum.z / n};
    double spread = 0.0;
    for (const auto& p : points) {
        spread = std::max(spread, p.distanceTo(centroid));
    }
    return CoordinateStats{centroid, spread, points.size()};
}

void sortVesselRecords(std::vector<VesselRecord>& records) {
    std::ranges::sort(records, [](const VesselRecord& lhs, const VesselRecord& rhs) {
        // rhs.priority on the left side yields descending priority within each status.
        return std::tie(lhs.status, rhs.priority, lhs.name) < std::tie(rhs.status, lhs.priority, rhs.name);
    });
}

// ----------------------------------------------------------------------------------- optional

std::optional<int> parseInt(std::string_view text) noexcept {
    if (text.empty()) {
        return std::nullopt;
    }
    const char* first = text.data();
    const char* last = text.data() + text.size();
    if (*first == '+') {  // from_chars rejects a leading '+'; accept it for friendliness
        ++first;
        if (first == last || *first == '-') {
            return std::nullopt;
        }
    }
    int value = 0;
    const auto [ptr, ec] = std::from_chars(first, last, value);
    if (ec != std::errc{} || ptr != last) {
        return std::nullopt;
    }
    return value;
}

std::optional<VesselStatus> findVesselStatus(const std::map<std::string, VesselStatus, std::less<>>& registry,
                                             std::string_view name) {
    const auto it = registry.find(name);
    if (it == registry.end()) {
        return std::nullopt;
    }
    return it->second;
}

// ------------------------------------------------------------------------------------ variant

std::string describeCommand(const Command& command) {
    return std::visit(
        Overloaded{
            [](const MoveCommand& c) {
                std::ostringstream os;
                os << "Move to (" << c.destination.x << ", " << c.destination.y << ", " << c.destination.z << ")";
                return os.str();
            },
            [](const AttackCommand& c) {
                return "Attack " + c.target + " at intensity " + std::to_string(c.intensity);
            },
            [](const ScanCommand& c) {
                std::ostringstream os;
                os << "Scan radius " << c.radius;
                return os.str();
            },
            [](const DockCommand& c) { return "Dock at " + c.station; },
        },
        command);
}

VesselStatus statusAfter(const Command& command) noexcept {
    // std::visit throws std::bad_variant_access only for a valueless variant, excluded here.
    if (command.valueless_by_exception()) {
        return VesselStatus::Maintenance;
    }
    return std::visit(Overloaded{
                          [](const MoveCommand&) noexcept { return VesselStatus::InTransit; },
                          [](const AttackCommand&) noexcept { return VesselStatus::Combat; },
                          [](const ScanCommand&) noexcept { return VesselStatus::Exploring; },
                          [](const DockCommand&) noexcept { return VesselStatus::Docked; },
                      },
                      command);
}

namespace {

std::optional<double> parseDouble(std::string_view token) {
    // Stream-based parsing is portable; floating-point std::from_chars is not universally available.
    std::istringstream stream{std::string(token)};
    stream.imbue(std::locale::classic());
    double value = 0.0;
    if (!(stream >> value) || !stream.eof() || !std::isfinite(value)) {
        return std::nullopt;
    }
    return value;
}

std::vector<std::string_view> tokenize(std::string_view text) {
    std::vector<std::string_view> tokens;
    std::size_t pos = 0;
    while (pos < text.size()) {
        const auto start = text.find_first_not_of(" \t", pos);
        if (start == std::string_view::npos) {
            break;
        }
        const auto end = text.find_first_of(" \t", start);
        tokens.push_back(text.substr(start, end == std::string_view::npos ? std::string_view::npos : end - start));
        pos = end == std::string_view::npos ? text.size() : end;
    }
    return tokens;
}

}  // namespace

CommandParseResult parseCommand(std::string_view text) {
    const auto tokens = tokenize(text);
    if (tokens.empty()) {
        return ParseError{"empty command", 0};
    }
    const auto expect = [&tokens](std::size_t count) -> std::optional<ParseError> {
        if (tokens.size() != count) {
            return ParseError{"expected " + std::to_string(count - 1) + " argument(s)",
                              std::min(tokens.size(), count)};
        }
        return std::nullopt;
    };
    const std::string_view verb = tokens[0];
    if (verb == "move") {
        if (auto error = expect(4)) {
            return *std::move(error);
        }
        std::array<double, 3> xyz{};
        for (std::size_t i = 0; i < 3; ++i) {
            const auto value = parseDouble(tokens[i + 1]);
            if (!value) {
                return ParseError{"invalid coordinate", i + 1};
            }
            xyz[i] = *value;
        }
        return Command{MoveCommand{{xyz[0], xyz[1], xyz[2]}}};
    }
    if (verb == "attack") {
        if (auto error = expect(3)) {
            return *std::move(error);
        }
        const auto intensity = parseInt(tokens[2]);
        if (!intensity || *intensity < 1 || *intensity > 10) {
            return ParseError{"intensity must be an integer in [1, 10]", 2};
        }
        return Command{AttackCommand{std::string(tokens[1]), *intensity}};
    }
    if (verb == "scan") {
        if (auto error = expect(2)) {
            return *std::move(error);
        }
        const auto radius = parseDouble(tokens[1]);
        if (!radius || *radius <= 0.0) {
            return ParseError{"radius must be a positive number", 1};
        }
        return Command{ScanCommand{*radius}};
    }
    if (verb == "dock") {
        if (auto error = expect(2)) {
            return *std::move(error);
        }
        return Command{DockCommand{std::string(tokens[1])}};
    }
    return ParseError{"unknown command '" + std::string(verb) + "'", 0};
}

// ---------------------------------------------------------------------------------------- any

bool PropertyBag::erase(std::string_view key) {
    const auto it = properties_.find(key);
    if (it == properties_.end()) {
        return false;
    }
    properties_.erase(it);
    return true;
}

std::vector<std::string> PropertyBag::keys() const {
    std::vector<std::string> result;
    result.reserve(properties_.size());
    for (const auto& [key, value] : properties_) {
        result.push_back(key);
    }
    return result;
}

// --------------------------------------------------------------------------- string_view/span

std::vector<std::string_view> splitView(std::string_view text, char delimiter) {
    std::vector<std::string_view> fields;
    std::size_t start = 0;
    while (true) {
        const auto end = text.find(delimiter, start);
        if (end == std::string_view::npos) {
            fields.push_back(text.substr(start));
            return fields;
        }
        fields.push_back(text.substr(start, end - start));
        start = end + 1;
    }
}

std::optional<double> mean(std::span<const double> values) noexcept {
    if (values.empty()) {
        return std::nullopt;
    }
    return std::accumulate(values.begin(), values.end(), 0.0) / static_cast<double>(values.size());
}

// ------------------------------------------------------------------------------ demonstrations

void demonstratePairs(std::ostream& out) {
    out << "\n=== std::pair ===\n";
    const std::pair<std::string, VesselStatus> vessel{"Rocinante", VesselStatus::InTransit};
    const auto& [name, status] = vessel;
    out << "structured binding: " << name << " is " << toString(status) << '\n';

    const std::vector<NavigationCoordinate> beacons{{0, 0, 0}, {10, 0, 0}, {10, 1, 0}, {-5, -5, -5}};
    if (const auto closest = closestPairIndices(beacons)) {
        out << "closest beacons: #" << closest->first << " and #" << closest->second << '\n';
    }
    if (const auto range = distanceRangeFromOrigin(beacons)) {
        out << "distance range from origin: [" << range->first << ", " << range->second << "]\n";
    }
    std::map<std::string, int> docking_bays;
    const auto [where, inserted] = docking_bays.insert({"Bay 1", 4});
    const auto [again, inserted_again] = docking_bays.insert({"Bay 1", 9});
    out << std::boolalpha << "map::insert returns pair<iterator,bool>: " << inserted << " then " << inserted_again
        << " (value stays " << again->second << ", same node: " << (where == again) << ")\n";
}

void demonstrateTuples(std::ostream& out) {
    out << "\n=== std::tuple ===\n";
    const std::vector<NavigationCoordinate> cloud{{1, 1, 1}, {3, 1, 1}, {2, 4, 1}};
    if (const auto stats = coordinateStatistics(cloud)) {
        const auto& [centroid, spread, count] = *stats;
        out << "centroid (" << centroid.x << ", " << centroid.y << ", " << centroid.z << "), spread " << spread
            << ", " << count << " points\n";
    }
    const auto record = std::make_tuple(std::string("Tycho"), 42, 3.5);
    out << "formatTuple via std::apply: " << formatTuple(record) << '\n';
    const auto doubled = transformTuple(std::make_tuple(1, 2.5, 4L), [](auto v) { return v * 2; });
    out << "transformTuple: " << formatTuple(doubled) << '\n';
    std::size_t elements = 0;
    forEachElement(record, [&elements](const auto&) { ++elements; });
    out << "forEachElement visited " << elements << " elements; tuple_size = "
        << std::tuple_size_v<decltype(record)> << '\n';

    std::vector<VesselRecord> fleet{{"Canterbury", VesselStatus::InTransit, 2},
                                    {"Donnager", VesselStatus::Combat, 9},
                                    {"Razorback", VesselStatus::InTransit, 7},
                                    {"Agatha King", VesselStatus::Docked, 5}};
    sortVesselRecords(fleet);
    out << "std::tie multi-key sort:";
    for (const auto& v : fleet) {
        out << ' ' << v.name;
    }
    out << '\n';
}

void demonstrateOptional(std::ostream& out) {
    out << "\n=== std::optional ===\n";
    for (const std::string_view input : {"42", "-7", "12abc", ""}) {
        const auto parsed = parseInt(input);
        out << "parseInt(\"" << input << "\") -> " << (parsed ? std::to_string(*parsed) : std::string("nullopt"))
            << '\n';
    }
    out << "safeDivide(1, 0).value_or(-1) = " << safeDivide(1.0, 0.0).value_or(-1.0) << '\n';
    const auto fuel_per_jump = andThen(parseInt("120"), [](int fuel) { return safeDivide(fuel, 8.0); });
    out << "andThen(parseInt, safeDivide) = " << fuel_per_jump.value_or(0.0) << '\n';
    const auto label = transformOptional(parseInt("3"), [](int n) { return "warp " + std::to_string(n); });
    out << "transformOptional -> " << label.value_or("none") << '\n';
    const std::map<std::string, VesselStatus, std::less<>> registry{{"Nauvoo", VesselStatus::Maintenance}};
    const auto status = findVesselStatus(registry, "Nauvoo");
    out << "registry lookup: " << (status ? toString(*status) : std::string_view{"unknown"}) << '\n';
}

void demonstrateVariant(std::ostream& out) {
    out << "\n=== std::variant ===\n";
    const std::vector<Command> orders{MoveCommand{{1, 2, 3}}, AttackCommand{"Pirate", 7}, ScanCommand{250.0},
                                      DockCommand{"Tycho Station"}};
    for (const auto& order : orders) {
        out << "visit: " << describeCommand(order) << " -> status " << toString(statusAfter(order)) << '\n';
    }
    for (const std::string_view text : {"scan 12.5", "attack Drone 11", "warp 9"}) {
        const auto result = parseCommand(text);
        if (const auto* command = std::get_if<Command>(&result)) {
            out << "parsed \"" << text << "\": " << describeCommand(*command) << '\n';
        } else {
            const auto& error = std::get<ParseError>(result);
            out << "error in \"" << text << "\" at token " << error.token << ": " << error.message << '\n';
        }
    }
    out << "holds_alternative<ScanCommand>(orders[2]) = " << std::boolalpha
        << std::holds_alternative<ScanCommand>(orders[2]) << ", index = " << orders[2].index() << '\n';
}

void demonstrateAny(std::ostream& out) {
    out << "\n=== std::any ===\n";
    PropertyBag config;
    config.set("callsign", std::string("Roci"));
    config.set("max_warp", 9.2);
    config.set("crew", 4);
    out << "PropertyBag holds " << config.size() << " properties\n";
    out << std::boolalpha << "get<int>(\"crew\") = " << config.get<int>("crew").value_or(-1)
        << ", get<long>(\"crew\") engaged? " << config.get<long>("crew").has_value() << '\n';
    out << "holds<double>(\"max_warp\") = " << config.holds<double>("max_warp") << '\n';
    std::any scratch = 5;
    scratch = std::string("now a string");
    try {
        (void)std::any_cast<int>(scratch);
    } catch (const std::bad_any_cast&) {
        out << "any_cast<int> on a string threw std::bad_any_cast as expected\n";
    }
}

void demonstrateViews(std::ostream& out) {
    out << "\n=== std::string_view and std::span ===\n";
    constexpr std::string_view manifest = "fuel,water,,ammo";
    const auto fields = splitView(manifest, ',');
    out << "splitView produced " << fields.size() << " non-owning fields:";
    for (const auto field : fields) {
        out << " [" << field << ']';
    }
    out << '\n';
    const std::array<double, 4> readings{2.0, 4.0, 6.0, 8.0};
    out << "mean over std::span of first 3 readings: " << mean(std::span(readings).first(3)).value_or(0.0) << '\n';
}

void runSTLUtilitiesDemo(std::ostream& out) {
    demonstratePairs(out);
    demonstrateTuples(out);
    demonstrateOptional(out);
    demonstrateVariant(out);
    demonstrateAny(out);
    demonstrateViews(out);
}

}  // namespace CppVerseHub::STL
