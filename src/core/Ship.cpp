/**
 * @file Ship.cpp
 * @brief ShipType string conversions.
 */
#include "core/Ship.hpp"

namespace CppVerseHub::Core {

namespace {
constexpr std::array<std::string_view, kShipTypeCount> kNames{"scout",      "fighter",   "cruiser",
                                                              "battleship", "transport", "colonizer"};
} // namespace

std::string_view toString(ShipType type) noexcept {
    const auto i = static_cast<std::size_t>(type);
    return i < kNames.size() ? kNames[i] : std::string_view{"unknown"};
}

std::optional<ShipType> parseShipType(std::string_view name) noexcept {
    for (std::size_t i = 0; i < kNames.size(); ++i) {
        if (kNames[i] == name) {
            return static_cast<ShipType>(i);
        }
    }
    return std::nullopt;
}

} // namespace CppVerseHub::Core
