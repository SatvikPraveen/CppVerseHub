/**
 * @file Resources.hpp
 * @brief Resource types and a fixed-size, enum-indexed vector of integer resource amounts.
 *
 * Resources are counted in integer units (`ResourceAmount` = `std::int64_t`) rather than doubles so
 * that conservation laws in the ResourceManager hold *exactly*: a transfer never creates or destroys
 * a fraction of a unit through rounding. `ResourceAmounts` shows how to make an `std::array` safely
 * indexable by a scoped enum.
 */
#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string_view>

namespace CppVerseHub::Core {

/// @brief Integer quantity of a resource.
using ResourceAmount = std::int64_t;

/// @brief The kinds of resource tracked by the economy.
enum class ResourceType : std::uint8_t { Minerals, Energy, Food, Water, Technology };

/// @brief Number of ResourceType enumerators.
inline constexpr std::size_t kResourceTypeCount = 5;

/// @brief All resource types in declaration order (useful for range-for loops).
inline constexpr std::array<ResourceType, kResourceTypeCount> kAllResourceTypes{
    ResourceType::Minerals, ResourceType::Energy, ResourceType::Food, ResourceType::Water,
    ResourceType::Technology};

/// @brief Array index of a resource type. @param type Resource type. @return Index in [0,
/// kResourceTypeCount).
[[nodiscard]] constexpr std::size_t indexOf(ResourceType type) noexcept {
    return static_cast<std::size_t>(type);
}

/// @brief Lower-case name of a resource type. @param type Resource type. @return Name such as "minerals".
[[nodiscard]] std::string_view toString(ResourceType type) noexcept;

/// @brief Parse a lower-case resource name. @param name Name. @return The type, or std::nullopt.
[[nodiscard]] std::optional<ResourceType> parseResourceType(std::string_view name) noexcept;

/// @brief A value per ResourceType, indexable by the enum.
/// @tparam T Element type (ResourceAmount for stock, double for rates).
template <typename T>
struct ResourceArray {
    std::array<T, kResourceTypeCount> values{}; ///< Storage, indexed by indexOf(type).

    /// @brief Mutable access. @param type Resource. @return Reference to the element.
    [[nodiscard]] constexpr T& operator[](ResourceType type) noexcept { return values[indexOf(type)]; }
    /// @brief Const access. @param type Resource. @return Element value.
    [[nodiscard]] constexpr const T& operator[](ResourceType type) const noexcept {
        return values[indexOf(type)];
    }

    /// @brief Sum of all elements. @return Total.
    [[nodiscard]] constexpr T sum() const noexcept {
        T total{};
        for (const T& v : values) {
            total += v;
        }
        return total;
    }

    /// @brief Whether every element is zero. @return true if all zero.
    [[nodiscard]] constexpr bool isZero() const noexcept {
        for (const T& v : values) {
            if (v != T{}) {
                return false;
            }
        }
        return true;
    }

    /// @brief Element-wise addition. @param o Other. @return `*this`.
    constexpr ResourceArray& operator+=(const ResourceArray& o) noexcept {
        for (std::size_t i = 0; i < kResourceTypeCount; ++i) {
            values[i] += o.values[i];
        }
        return *this;
    }

    /// @brief Element-wise equality. @return true if equal.
    friend constexpr bool operator==(const ResourceArray&, const ResourceArray&) noexcept = default;
};

/// @brief Integer stock of every resource.
using ResourceAmounts = ResourceArray<ResourceAmount>;
/// @brief Per-second rate of every resource.
using ResourceRates = ResourceArray<double>;

} // namespace CppVerseHub::Core
