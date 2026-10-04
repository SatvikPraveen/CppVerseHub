/**
 * @file Identifiers.hpp
 * @brief Strongly typed identifiers (`EntityId`, `MissionId`) built from a single tag-dispatched template.
 *
 * Demonstrates the "strong typedef" idiom: two ids with the same representation (`std::uint64_t`) are
 * distinct types, so passing a `MissionId` where an `EntityId` is expected is a compile error rather
 * than a silent bug. Comparison is defaulted via `operator<=>`, and `std::hash` is specialised so ids
 * work as keys in both ordered and unordered containers.
 */
#pragma once

#include <compare>
#include <cstdint>
#include <functional>
#include <ostream>

namespace CppVerseHub::Core {

/**
 * @brief A strongly typed integral identifier. The value 0 is reserved as "invalid".
 * @tparam Tag Empty tag type that makes each instantiation a distinct type.
 */
template <typename Tag>
class StrongId {
public:
    using ValueType = std::uint64_t; ///< Underlying representation.

    /// @brief Construct the invalid id (value 0).
    constexpr StrongId() noexcept = default;

    /// @brief Construct from a raw value. @param value Raw id value.
    constexpr explicit StrongId(ValueType value) noexcept : value_(value) {}

    /// @brief Raw value. @return The underlying integer.
    [[nodiscard]] constexpr ValueType value() const noexcept { return value_; }

    /// @brief Whether this id refers to anything. @return true if the value is non-zero.
    [[nodiscard]] constexpr bool isValid() const noexcept { return value_ != 0; }

    /// @brief Total ordering and equality. @return Comparison result.
    friend constexpr auto operator<=>(const StrongId&, const StrongId&) noexcept = default;

    /**
     * @brief Stream as "#<value>".
     * @param os Output stream.
     * @param id Id to print.
     * @return The stream.
     */
    friend std::ostream& operator<<(std::ostream& os, const StrongId& id) { return os << '#' << id.value_; }

private:
    ValueType value_{0};
};

/// @brief Tag for entity identifiers.
struct EntityIdTag {};
/// @brief Tag for mission identifiers.
struct MissionIdTag {};

/// @brief Identifier of an Entity (Planet, Fleet, ...) within a Galaxy.
using EntityId = StrongId<EntityIdTag>;
/// @brief Identifier of a Mission within a Galaxy.
using MissionId = StrongId<MissionIdTag>;

} // namespace CppVerseHub::Core

namespace std {
/// @brief Hash support so StrongId can key unordered containers.
template <typename Tag>
struct hash<CppVerseHub::Core::StrongId<Tag>> {
    /// @brief Hash the underlying value. @param id Id. @return Hash value.
    [[nodiscard]] size_t operator()(const CppVerseHub::Core::StrongId<Tag>& id) const noexcept {
        return hash<uint64_t>{}(id.value());
    }
};
} // namespace std
