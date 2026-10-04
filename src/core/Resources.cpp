/**
 * @file Resources.cpp
 * @brief String conversions for ResourceType.
 */
#include "core/Resources.hpp"

namespace CppVerseHub::Core {

namespace {
constexpr std::array<std::string_view, kResourceTypeCount> kNames{"minerals", "energy", "food", "water",
                                                                   "technology"};
} // namespace

std::string_view toString(ResourceType type) noexcept {
    const std::size_t i = indexOf(type);
    return i < kNames.size() ? kNames[i] : std::string_view{"unknown"};
}

std::optional<ResourceType> parseResourceType(std::string_view name) noexcept {
    for (ResourceType t : kAllResourceTypes) {
        if (kNames[indexOf(t)] == name) {
            return t;
        }
    }
    return std::nullopt;
}

} // namespace CppVerseHub::Core
