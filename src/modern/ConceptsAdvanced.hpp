/**
 * @file ConceptsAdvanced.hpp
 * @brief C++20 concepts: named constraints, requires-expressions, subsumption and constrained templates.
 *
 * Concepts turn the implicit "duck typing" contracts of templates into named, checkable predicates.
 * This header demonstrates:
 *  - basic concepts built from standard ones (`Numeric`, `Printable`, `Comparable`);
 *  - requires-expressions that check nested types, expressions and return-type constraints
 *    (`Container`, `RandomAccessContainer`, domain concepts such as `Entity` and `Movable`);
 *  - concept composition and *subsumption*: the most constrained overload of `classify` wins
 *    without any SFINAE or tag dispatch;
 *  - constrained class templates, constrained member functions and constrained aliases;
 *  - concept-driven reflection (`profileOf<T>()`) that is evaluated entirely at compile time.
 *
 * Every concept below is verified with `static_assert` against positive and negative examples, so
 * the header doubles as a compile-time test suite.
 */
#pragma once

#include <concepts>
#include <cstddef>
#include <iostream>
#include <iterator>
#include <list>
#include <map>
#include <memory>
#include <ranges>
#include <sstream>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

namespace CppVerseHub::Modern::Concepts {

// ===== BASIC CONCEPTS =====

/// @brief Any built-in arithmetic type except `bool` (which is integral but rarely meant as a number).
template <typename T>
concept Numeric = (std::integral<T> || std::floating_point<T>) && !std::same_as<std::remove_cv_t<T>, bool>;

/// @brief Types that can be streamed to a `std::ostream`.
template <typename T>
concept Printable = requires(std::ostream& os, const T& value) {
    { os << value } -> std::same_as<std::ostream&>;
};

/// @brief Types with a full, consistent set of relational operators (a strict weak order).
template <typename T>
concept Comparable = std::totally_ordered<T>;

/// @brief Types that can be hashed with `std::hash`.
template <typename T>
concept Hashable = requires(const T& value) {
    { std::hash<T>{}(value) } -> std::convertible_to<std::size_t>;
};

// ===== CONTAINER CONCEPTS =====

/// @brief A minimal standard-library-style container: nested types plus begin/end/size/empty.
template <typename T>
concept Container = requires(T& c, const T& cc) {
    typename T::value_type;
    typename T::iterator;
    typename T::const_iterator;
    typename T::size_type;
    { c.begin() } -> std::same_as<typename T::iterator>;
    { c.end() } -> std::same_as<typename T::iterator>;
    { cc.begin() } -> std::same_as<typename T::const_iterator>;
    { cc.size() } -> std::convertible_to<typename T::size_type>;
    { cc.empty() } -> std::convertible_to<bool>;
};

/// @brief Anything usable in a range-for loop (alias for `std::ranges::range`).
template <typename T>
concept Iterable = std::ranges::range<T>;

/// @brief A container offering O(1) indexed access (refines `Container`).
template <typename T>
concept RandomAccessContainer = Container<T> && std::random_access_iterator<typename T::iterator> &&
                                requires(T& c, typename T::size_type n) {
                                    { c[n] } -> std::same_as<typename T::value_type&>;
                                };

/// @brief A container whose elements are `Numeric` (refines `Container`).
template <typename T>
concept NumericContainer = Container<T> && Numeric<typename T::value_type>;

// ===== CALLABLE CONCEPTS =====

/// @brief `F(Args...)` is invocable and its result converts to `R`.
template <typename F, typename R, typename... Args>
concept InvocableReturning = std::invocable<F, Args...> &&
                             std::convertible_to<std::invoke_result_t<F, Args...>, R>;

/// @brief A callable `T -> T` (closed under the element type).
template <typename F, typename T>
concept UnaryOperation = InvocableReturning<F, T, const T&>;

/// @brief A callable `(T, T) -> T`, suitable for folds.
template <typename F, typename T>
concept BinaryOperation = InvocableReturning<F, T, const T&, const T&>;

// ===== DOMAIN CONCEPTS (space game) =====

/// @brief An identifiable game entity.
template <typename T>
concept Entity = requires(const T& e) {
    typename T::EntityType;
    { e.getId() } -> std::convertible_to<int>;
    { e.getName() } -> std::convertible_to<std::string>;
    { e.isActive() } -> std::convertible_to<bool>;
};

/// @brief Something that has a 3D position which can be read and written.
template <typename T>
concept Positionable = requires(T& p, const T& cp) {
    { cp.getX() } -> std::floating_point;
    { cp.getY() } -> std::floating_point;
    { cp.getZ() } -> std::floating_point;
    { p.setPosition(0.0, 0.0, 0.0) } -> std::same_as<void>;
};

/// @brief A `Positionable` that also has a velocity and can be advanced in time (refines `Positionable`).
template <typename T>
concept Movable = Positionable<T> && requires(T& m, const T& cm) {
    { cm.getVelocityX() } -> std::floating_point;
    { cm.getVelocityY() } -> std::floating_point;
    { cm.getVelocityZ() } -> std::floating_point;
    { m.move(0.0) } -> std::same_as<void>;
};

/// @brief A quantified, typed resource.
template <typename T>
concept Resource = requires(const T& r) {
    { r.getAmount() } -> Numeric;
    { r.getType() } -> std::convertible_to<std::string>;
    { r.isRenewable() } -> std::convertible_to<bool>;
};

/// @brief An entity that lives somewhere in space.
template <typename T>
concept SpaceEntity = Entity<T> && Positionable<T>;

/// @brief A space entity that can move (most refined domain concept, subsumes `SpaceEntity`).
template <typename T>
concept MovableSpaceEntity = SpaceEntity<T> && Movable<T>;

/// @brief A container of resources.
template <typename T>
concept ResourceContainer = Container<T> && Resource<typename T::value_type>;

// ===== CONSTRAINED FUNCTIONS =====

/// @brief Adds two numbers; rejected at compile time for non-numeric types.
/// @param a First operand. @param b Second operand.
/// @return `a + b`.
template <Numeric T>
[[nodiscard]] constexpr T add(T a, T b) noexcept {
    return a + b;
}

/// @brief Returns the larger of two totally ordered values (first one on ties).
/// @param a First value. @param b Second value.
/// @return The maximum of `a` and `b`.
template <Comparable T>
[[nodiscard]] constexpr const T& maxOf(const T& a, const T& b) noexcept(noexcept(a < b)) {
    return (a < b) ? b : a;
}

/// @brief Sums all elements of a numeric container.
/// @param container The container to sum.
/// @return The sum, starting from a value-initialised accumulator.
template <NumericContainer C>
[[nodiscard]] constexpr typename C::value_type sumContainer(const C& container) {
    typename C::value_type sum{};
    for (const auto& item : container) {
        sum += item;
    }
    return sum;
}

/// @brief Applies a closed unary operation to every element in place.
/// @param range Range of elements (modified in place).
/// @param op Operation mapping an element to a new value of the same type.
template <Iterable R, UnaryOperation<std::ranges::range_value_t<R>> Op>
constexpr void transformInPlace(R& range, Op op) {
    for (auto& item : range) {
        item = op(item);
    }
}

/// @brief Left fold of a range with a binary operation.
/// @param range Input range. @param init Initial accumulator. @param op Combining operation.
/// @return The folded value.
template <Iterable R, typename T, BinaryOperation<T> Op>
[[nodiscard]] constexpr T foldLeft(const R& range, T init, Op op) {
    for (const auto& item : range) {
        init = op(init, static_cast<T>(item));
    }
    return init;
}

/// @brief Joins the elements of a range of printable values into a string.
/// @param range Input range. @param separator Text inserted between elements.
/// @return The joined string.
template <Iterable R>
    requires Printable<std::ranges::range_value_t<R>>
[[nodiscard]] std::string joinPrintable(const R& range, std::string_view separator = ", ") {
    std::ostringstream os;
    bool first = true;
    for (const auto& item : range) {
        if (!first) {
            os << separator;
        }
        os << item;
        first = false;
    }
    return os.str();
}

// ===== SUBSUMPTION: the most constrained overload is selected =====

/// @brief Fallback classification for unconstrained types.
/// @return "generic".
template <typename T>
[[nodiscard]] constexpr std::string_view classify(const T& /*unused*/) noexcept {
    return "generic";
}

/// @brief Classification for any numeric type.
/// @return "numeric".
template <Numeric T>
[[nodiscard]] constexpr std::string_view classify(const T& /*unused*/) noexcept {
    return "numeric";
}

/// @brief Classification for integers; `std::integral<T> && Numeric<T>` subsumes `Numeric<T>`.
/// @return "integral".
template <Numeric T>
    requires std::integral<T>
[[nodiscard]] constexpr std::string_view classify(const T& /*unused*/) noexcept {
    return "integral";
}

/// @brief Classification for containers.
/// @return "container".
template <Container T>
[[nodiscard]] constexpr std::string_view classify(const T& /*unused*/) noexcept {
    return "container";
}

/// @brief Classification for random-access containers (subsumes the `Container` overload).
/// @return "random-access container".
template <RandomAccessContainer T>
[[nodiscard]] constexpr std::string_view classify(const T& /*unused*/) noexcept {
    return "random-access container";
}

/// @brief Classification for space entities.
/// @return "space entity".
template <SpaceEntity T>
[[nodiscard]] constexpr std::string_view classify(const T& /*unused*/) noexcept {
    return "space entity";
}

/// @brief Classification for movable space entities (subsumes `SpaceEntity`).
/// @return "movable space entity".
template <MovableSpaceEntity T>
[[nodiscard]] constexpr std::string_view classify(const T& /*unused*/) noexcept {
    return "movable space entity";
}

// ===== COMPILE-TIME CONCEPT PROFILE =====

/// @brief Which of the module's concepts a type satisfies, computed at compile time.
struct ConceptProfile {
    bool numeric = false;    ///< Satisfies `Numeric`.
    bool printable = false;  ///< Satisfies `Printable`.
    bool comparable = false; ///< Satisfies `Comparable`.
    bool hashable = false;   ///< Satisfies `Hashable`.
    bool container = false;  ///< Satisfies `Container`.
    bool iterable = false;   ///< Satisfies `Iterable`.

    /// @brief Memberwise equality.
    friend constexpr bool operator==(const ConceptProfile&, const ConceptProfile&) = default;
};

/// @brief Evaluates every basic concept for `T`.
/// @return The `ConceptProfile` of `T`.
template <typename T>
[[nodiscard]] consteval ConceptProfile profileOf() noexcept {
    return ConceptProfile{Numeric<T>, Printable<T>, Comparable<T>, Hashable<T>, Container<T>, Iterable<T>};
}

/// @brief Writes a human-readable concept profile.
/// @param out Destination stream. @param typeName Display name of the type. @param profile The profile.
inline void printProfile(std::ostream& out, std::string_view typeName, const ConceptProfile& profile) {
    auto yn = [](bool b) { return b ? "yes" : "no"; };
    out << "  " << typeName << ": numeric=" << yn(profile.numeric) << " printable=" << yn(profile.printable)
        << " comparable=" << yn(profile.comparable) << " hashable=" << yn(profile.hashable)
        << " container=" << yn(profile.container) << " iterable=" << yn(profile.iterable) << '\n';
}

// ===== CONSTRAINED ALIASES AND CLASS TEMPLATES =====

/// @brief A vector restricted to numeric element types.
template <Numeric T>
using NumericVector = std::vector<T>;

/// @brief Unique ownership of an entity.
template <Entity T>
using EntityPtr = std::unique_ptr<T>;

/// @brief Factory whose operations are individually constrained on constructibility.
/// @tparam T Product type; must be an object type.
template <typename T>
    requires std::is_object_v<T>
class ConceptFactory {
public:
    /// @brief Constructs a `T` by value from the given arguments.
    /// @param args Constructor arguments (perfect-forwarded).
    /// @return The constructed object.
    template <typename... Args>
        requires std::constructible_from<T, Args...>
    [[nodiscard]] static T create(Args&&... args) {
        return T(std::forward<Args>(args)...);
    }

    /// @brief Constructs a `T` owned by a `std::unique_ptr`.
    /// @param args Constructor arguments (perfect-forwarded).
    /// @return Owning pointer to the new object.
    template <typename... Args>
        requires std::constructible_from<T, Args...>
    [[nodiscard]] static std::unique_ptr<T> createUnique(Args&&... args) {
        return std::make_unique<T>(std::forward<Args>(args)...);
    }

    /// @brief Constructs a `T` owned by a `std::shared_ptr`.
    /// @param args Constructor arguments (perfect-forwarded).
    /// @return Shared owning pointer to the new object.
    template <typename... Args>
        requires std::constructible_from<T, Args...>
    [[nodiscard]] static std::shared_ptr<T> createShared(Args&&... args) {
        return std::make_shared<T>(std::forward<Args>(args)...);
    }
};

/// @brief A statistics accumulator only instantiable for numeric types.
/// @tparam T Numeric sample type.
template <Numeric T>
class RunningStats {
public:
    /// @brief Adds a sample.
    /// @param value The sample.
    constexpr void add(T value) noexcept {
        if (count_ == 0 || value < min_) {
            min_ = value;
        }
        if (count_ == 0 || value > max_) {
            max_ = value;
        }
        sum_ += static_cast<double>(value);
        ++count_;
    }
    /// @brief Number of samples.
    /// @return Count of samples added.
    [[nodiscard]] constexpr std::size_t count() const noexcept { return count_; }
    /// @brief Arithmetic mean (0 for no samples).
    /// @return Mean of the samples.
    [[nodiscard]] constexpr double mean() const noexcept {
        return count_ == 0 ? 0.0 : sum_ / static_cast<double>(count_);
    }
    /// @brief Smallest sample (value-initialised if empty).
    /// @return Minimum sample.
    [[nodiscard]] constexpr T min() const noexcept { return min_; }
    /// @brief Largest sample (value-initialised if empty).
    /// @return Maximum sample.
    [[nodiscard]] constexpr T max() const noexcept { return max_; }

private:
    std::size_t count_ = 0;
    double sum_ = 0.0;
    T min_{};
    T max_{};
};

// ===== DEMO TYPES MODELLING THE DOMAIN CONCEPTS =====

/// @brief A stationary entity: models `SpaceEntity` but not `Movable`.
class DemoEntity {
public:
    using EntityType = int; ///< Required by `Entity`.

    DemoEntity() = default;
    /// @brief Constructs an entity with an id and name.
    /// @param id Entity id. @param name Display name.
    DemoEntity(int id, std::string name) : id_(id), name_(std::move(name)) {}

    /// @brief Entity id. @return The id.
    [[nodiscard]] int getId() const noexcept { return id_; }
    /// @brief Entity name. @return The name.
    [[nodiscard]] std::string getName() const { return name_; }
    /// @brief Whether the entity is active. @return Active flag.
    [[nodiscard]] bool isActive() const noexcept { return active_; }
    /// @brief X coordinate. @return X.
    [[nodiscard]] double getX() const noexcept { return x_; }
    /// @brief Y coordinate. @return Y.
    [[nodiscard]] double getY() const noexcept { return y_; }
    /// @brief Z coordinate. @return Z.
    [[nodiscard]] double getZ() const noexcept { return z_; }
    /// @brief Sets the position. @param x X. @param y Y. @param z Z.
    void setPosition(double x, double y, double z) noexcept {
        x_ = x;
        y_ = y;
        z_ = z;
    }

private:
    int id_ = 1;
    std::string name_ = "DemoEntity";
    bool active_ = true;
    double x_ = 0.0;
    double y_ = 0.0;
    double z_ = 0.0;
};

/// @brief A ship: models `MovableSpaceEntity`.
class DemoShip : public DemoEntity {
public:
    DemoShip() = default;
    /// @brief Constructs a ship with an id, name and velocity.
    /// @param id Id. @param name Name. @param vx X velocity. @param vy Y velocity. @param vz Z velocity.
    DemoShip(int id, std::string name, double vx, double vy, double vz)
        : DemoEntity(id, std::move(name)), vx_(vx), vy_(vy), vz_(vz) {}

    /// @brief X velocity. @return vx.
    [[nodiscard]] double getVelocityX() const noexcept { return vx_; }
    /// @brief Y velocity. @return vy.
    [[nodiscard]] double getVelocityY() const noexcept { return vy_; }
    /// @brief Z velocity. @return vz.
    [[nodiscard]] double getVelocityZ() const noexcept { return vz_; }
    /// @brief Advances the position by velocity * dt. @param dt Time step.
    void move(double dt) noexcept { setPosition(getX() + vx_ * dt, getY() + vy_ * dt, getZ() + vz_ * dt); }

private:
    double vx_ = 0.0;
    double vy_ = 0.0;
    double vz_ = 0.0;
};

/// @brief A resource: models `Resource`.
class DemoResource {
public:
    DemoResource() = default;
    /// @brief Constructs a resource. @param type Type name. @param amount Quantity. @param renewable Flag.
    DemoResource(std::string type, int amount, bool renewable)
        : amount_(amount), type_(std::move(type)), renewable_(renewable) {}
    /// @brief Quantity. @return Amount.
    [[nodiscard]] int getAmount() const noexcept { return amount_; }
    /// @brief Resource type. @return Type name.
    [[nodiscard]] std::string getType() const { return type_; }
    /// @brief Whether it regenerates. @return Renewable flag.
    [[nodiscard]] bool isRenewable() const noexcept { return renewable_; }

private:
    int amount_ = 100;
    std::string type_ = "Energy";
    bool renewable_ = true;
};

/// @brief Total amount over a container of resources (only accepts `ResourceContainer`s).
/// @param resources The resources.
/// @return Sum of `getAmount()`.
template <ResourceContainer C>
[[nodiscard]] auto totalResourceAmount(const C& resources) {
    using Amount = decltype(std::declval<const typename C::value_type&>().getAmount());
    Amount total{};
    for (const auto& r : resources) {
        total += r.getAmount();
    }
    return total;
}

/// @brief Advances every movable entity in a range by `dt`.
/// @param entities Range of movable space entities. @param dt Time step.
template <std::ranges::range R>
    requires MovableSpaceEntity<std::ranges::range_value_t<R>>
void advanceAll(R& entities, double dt) {
    for (auto& e : entities) {
        e.move(dt);
    }
}

// ===== COMPILE-TIME VERIFICATION =====

static_assert(Numeric<int> && Numeric<double> && !Numeric<bool> && !Numeric<std::string>);
static_assert(Printable<int> && Printable<std::string> && !Printable<std::vector<int>>);
static_assert(Comparable<std::string> && !Comparable<DemoEntity>);
static_assert(Hashable<std::string> && !Hashable<std::vector<int>>);
static_assert(Container<std::vector<int>> && Container<std::list<int>> && !Container<int>);
static_assert(RandomAccessContainer<std::vector<int>> && !RandomAccessContainer<std::list<int>>);
static_assert(NumericContainer<std::vector<double>> && !NumericContainer<std::vector<std::string>>);
static_assert(Iterable<int[3]> && !Container<int[3]>);
static_assert(UnaryOperation<int (*)(const int&), int>);
static_assert(SpaceEntity<DemoEntity> && !Movable<DemoEntity>);
static_assert(MovableSpaceEntity<DemoShip>);
static_assert(Resource<DemoResource> && ResourceContainer<std::vector<DemoResource>>);
static_assert(add(2, 3) == 5);
static_assert(maxOf(4, 9) == 9);
static_assert(profileOf<int>() == ConceptProfile{true, true, true, true, false, false});
static_assert(classify(1) == "integral" && classify(1.0) == "numeric");

/// @brief Showcase of every concept feature in this header.
/// @param out Destination stream.
inline void demonstrateConcepts(std::ostream& out = std::cout) {
    out << "\n=== C++20 Concepts ===\n";
    out << "Concept profiles (computed with consteval):\n";
    printProfile(out, "int", profileOf<int>());
    printProfile(out, "std::string", profileOf<std::string>());
    printProfile(out, "std::vector<int>", profileOf<std::vector<int>>());
    printProfile(out, "DemoEntity", profileOf<DemoEntity>());

    out << "Constrained functions: add(5, 3) = " << add(5, 3) << ", maxOf(10.5, 7.2) = " << maxOf(10.5, 7.2)
        << '\n';
    std::vector<int> numbers{1, 2, 3, 4, 5};
    out << "sumContainer({" << joinPrintable(numbers) << "}) = " << sumContainer(numbers) << '\n';
    transformInPlace(numbers, [](const int& n) { return n * n; });
    out << "transformInPlace(square) -> {" << joinPrintable(numbers) << "}\n";
    out << "foldLeft(product) = " << foldLeft(numbers, 1L, [](const long& a, const long& b) { return a * b; })
        << '\n';

    out << "Overload resolution by subsumption:\n";
    out << "  classify(42)            -> " << classify(42) << '\n';
    out << "  classify(3.14)          -> " << classify(3.14) << '\n';
    out << "  classify(vector<int>)   -> " << classify(numbers) << '\n';
    out << "  classify(list<int>)     -> " << classify(std::list<int>{}) << '\n';
    out << "  classify(DemoEntity)    -> " << classify(DemoEntity{}) << '\n';
    out << "  classify(DemoShip)      -> " << classify(DemoShip{}) << '\n';
    out << "  classify(nullptr)       -> " << classify(nullptr) << '\n';

    std::vector<DemoShip> fleet{{1, "Scout", 1.0, 0.0, 0.0}, {2, "Hauler", 0.0, 2.0, 0.0}};
    advanceAll(fleet, 2.5);
    out << "After advanceAll(2.5): " << fleet[0].getName() << " at x=" << fleet[0].getX() << ", "
        << fleet[1].getName() << " at y=" << fleet[1].getY() << '\n';

    std::vector<DemoResource> cargo{{"Energy", 40, true}, {"Ore", 60, false}};
    out << "totalResourceAmount = " << totalResourceAmount(cargo) << '\n';

    RunningStats<double> stats;
    for (double v : {3.0, 1.0, 4.0, 1.0, 5.0}) {
        stats.add(v);
    }
    out << "RunningStats<double>: n=" << stats.count() << " mean=" << stats.mean() << " min=" << stats.min()
        << " max=" << stats.max() << '\n';

    auto entity = ConceptFactory<DemoEntity>::create(7, "Outpost");
    auto owned = ConceptFactory<DemoEntity>::createUnique(8, "Station");
    out << "ConceptFactory created '" << entity.getName() << "' and '" << owned->getName() << "'\n";
}

} // namespace CppVerseHub::Modern::Concepts
