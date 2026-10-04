/**
 * @file MoveSemantics.hpp
 * @brief Move semantics, value categories and perfect forwarding.
 *
 * Move semantics let resources be *transferred* instead of duplicated. This module demonstrates:
 *  - the rule of five with instrumented special members (`TrackedResource` counts every
 *    construction, copy, move and destruction into an `OperationCounts` observer);
 *  - move-only types (`Spacecraft`, owning its log through `std::unique_ptr`);
 *  - value categories and why `std::forward` is needed (`categoryOf`, `relayForwarded` vs.
 *    `relayWithoutForward`);
 *  - a hand-written growable array (`MoveAwareVector`) that, like `std::vector`, uses
 *    `std::move_if_noexcept` when reallocating so it keeps the strong exception guarantee;
 *  - guaranteed copy elision of prvalues (C++17) and the "sink argument" idiom.
 *
 * Library classes are silent: instrumentation is reported through counters, never through printing.
 */
#pragma once

#include <cstddef>
#include <functional>
#include <initializer_list>
#include <iostream>
#include <memory>
#include <new>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

namespace CppVerseHub::Modern::MoveSemantics {

// ===== INSTRUMENTATION =====

/// @brief Tally of special-member invocations observed by `TrackedResource` (not thread-safe).
struct OperationCounts {
    std::size_t constructions = 0; ///< Value constructions.
    std::size_t copies = 0;        ///< Copy constructions + copy assignments.
    std::size_t moves = 0;         ///< Move constructions + move assignments.
    std::size_t destructions = 0;  ///< Destructor calls.

    /// @brief Resets every counter to zero.
    constexpr void reset() noexcept { *this = OperationCounts{}; }
    /// @brief Memberwise equality.
    friend constexpr bool operator==(const OperationCounts&, const OperationCounts&) = default;
};

/// @brief A resource-owning type that implements the rule of five and reports each special member.
///        After being moved from it is empty (`isMovedFrom()`), but remains valid and assignable.
class TrackedResource {
public:
    /// @brief Creates a named resource holding `payloadSize` integers 0..payloadSize-1.
    /// @param name Resource name. @param payloadSize Number of integers. @param counts Optional observer.
    TrackedResource(std::string name, std::size_t payloadSize, OperationCounts* counts = nullptr);

    /// @brief Deep copy. @param other Source.
    TrackedResource(const TrackedResource& other);
    /// @brief Steals `other`'s state; `other` becomes empty. @param other Source.
    TrackedResource(TrackedResource&& other) noexcept;
    /// @brief Copy assignment with the strong exception guarantee (copy-and-swap). @param other Source.
    /// @return *this.
    TrackedResource& operator=(const TrackedResource& other);
    /// @brief Move assignment; `other` becomes empty. @param other Source. @return *this.
    TrackedResource& operator=(TrackedResource&& other) noexcept;
    /// @brief Releases the payload and records a destruction.
    ~TrackedResource();

    /// @brief Resource name (empty when moved-from). @return Name.
    [[nodiscard]] const std::string& name() const noexcept { return name_; }
    /// @brief Number of integers in the payload. @return Size (0 when moved-from).
    [[nodiscard]] std::size_t size() const noexcept { return payload_ ? payload_->size() : 0; }
    /// @brief Sum of the payload. @return Sum.
    [[nodiscard]] long long checksum() const noexcept;
    /// @brief Whether the state was moved out. @return True if empty.
    [[nodiscard]] bool isMovedFrom() const noexcept { return !payload_; }
    /// @brief Appends a value (re-creating the payload if moved-from). @param value Value.
    void append(int value);
    /// @brief Rebinds the observer. @param counts New observer (may be null).
    void setObserver(OperationCounts* counts) noexcept { counts_ = counts; }

    /// @brief Member-wise swap (never throws). @param a First. @param b Second.
    friend void swap(TrackedResource& a, TrackedResource& b) noexcept {
        using std::swap;
        swap(a.name_, b.name_);
        swap(a.payload_, b.payload_);
        swap(a.counts_, b.counts_);
    }

private:
    std::string name_;
    std::unique_ptr<std::vector<int>> payload_;
    OperationCounts* counts_ = nullptr; // non-owning observer
};

static_assert(std::is_nothrow_move_constructible_v<TrackedResource>);
static_assert(std::is_nothrow_move_assignable_v<TrackedResource>);

// ===== MOVE-ONLY TYPE =====

/// @brief A spacecraft that uniquely owns its mission log and cargo: copyable is meaningless, so the
///        copy operations are deleted and only moves are allowed.
class Spacecraft {
public:
    /// @brief Creates a spacecraft. @param id Identifier. @param name Name (sink argument).
    Spacecraft(int id, std::string name);

    Spacecraft(const Spacecraft&) = delete;
    Spacecraft& operator=(const Spacecraft&) = delete;
    /// @brief Transfers ownership. @param other Source (left with id -1 and no log).
    Spacecraft(Spacecraft&& other) noexcept;
    /// @brief Transfers ownership. @param other Source. @return *this.
    Spacecraft& operator=(Spacecraft&& other) noexcept;
    ~Spacecraft() = default;

    /// @brief Adds cargo by value: callers choose copy (lvalue) or move (rvalue). @param item Cargo.
    void loadCargo(TrackedResource item);
    /// @brief Constructs cargo in place. @param args `TrackedResource` constructor arguments.
    /// @return Reference to the new cargo item.
    template <typename... Args>
    TrackedResource& emplaceCargo(Args&&... args) {
        return cargo_.emplace_back(std::forward<Args>(args)...);
    }
    /// @brief Removes and returns the last cargo item. @return The item. @throws std::out_of_range if empty.
    [[nodiscard]] TrackedResource unloadCargo();
    /// @brief Appends a log line. @param entry Text.
    void log(std::string entry);

    /// @brief Id (-1 once moved-from). @return Id.
    [[nodiscard]] int id() const noexcept { return id_; }
    /// @brief Name. @return Name.
    [[nodiscard]] const std::string& name() const noexcept { return name_; }
    /// @brief Cargo count. @return Number of cargo items.
    [[nodiscard]] std::size_t cargoCount() const noexcept { return cargo_.size(); }
    /// @brief Log size (0 when moved-from). @return Number of log entries.
    [[nodiscard]] std::size_t logSize() const noexcept { return log_ ? log_->size() : 0; }
    /// @brief Whether this object still owns its state. @return True if valid.
    [[nodiscard]] bool isValid() const noexcept { return log_ != nullptr; }

private:
    int id_;
    std::string name_;
    std::vector<TrackedResource> cargo_;
    std::unique_ptr<std::vector<std::string>> log_;
};

static_assert(!std::is_copy_constructible_v<Spacecraft> && std::is_nothrow_move_constructible_v<Spacecraft>);

// ===== VALUE CATEGORIES AND PERFECT FORWARDING =====

/// @brief The value category (and constness) an expression had at a call site.
enum class ValueCategory { LValue, ConstLValue, RValue };

/// @brief Human-readable name. @param c Category. @return Name.
[[nodiscard]] constexpr std::string_view toString(ValueCategory c) noexcept {
    switch (c) {
        case ValueCategory::LValue:
            return "lvalue";
        case ValueCategory::ConstLValue:
            return "const lvalue";
        case ValueCategory::RValue:
            return "rvalue";
    }
    return "unknown";
}

/// @brief Classifies its argument via forwarding-reference deduction (`T` is `U&` for lvalues).
/// @return The category of the argument expression.
template <typename T>
[[nodiscard]] constexpr ValueCategory categoryOf(T&& /*unused*/) noexcept {
    if constexpr (std::is_lvalue_reference_v<T>) {
        return std::is_const_v<std::remove_reference_t<T>> ? ValueCategory::ConstLValue
                                                           : ValueCategory::LValue;
    } else {
        return ValueCategory::RValue;
    }
}

/// @brief Overload chosen for lvalues. @return "copy".
[[nodiscard]] inline std::string_view accept(const std::string& /*unused*/) noexcept {
    return "copy";
}
/// @brief Overload chosen for rvalues. @return "move".
[[nodiscard]] inline std::string_view accept(std::string&& /*unused*/) noexcept {
    return "move";
}

/// @brief Relays its argument with `std::forward`, preserving the caller's value category.
/// @param value Argument. @return Which `accept` overload was chosen.
template <typename T>
[[nodiscard]] std::string_view relayForwarded(T&& value) noexcept {
    return accept(std::forward<T>(value));
}

/// @brief Relays its argument *without* forwarding: a named parameter is always an lvalue.
/// @param value Argument. @return Which `accept` overload was chosen (always "copy").
template <typename T>
[[nodiscard]] std::string_view relayWithoutForward(T&& value) noexcept {
    return accept(value);
}

/// @brief Perfect-forwarding invoker. @param f Callable. @param args Arguments. @return f(args...).
template <typename F, typename... Args>
constexpr decltype(auto) forwardTo(F&& f, Args&&... args) noexcept(std::is_nothrow_invocable_v<F, Args...>) {
    return std::invoke(std::forward<F>(f), std::forward<Args>(args)...);
}

/// @brief Forwarding factory (what `std::make_unique` does). @param args Constructor arguments.
/// @return Owning pointer to a new `T`.
template <typename T, typename... Args>
[[nodiscard]] std::unique_ptr<T> makeUniqueForwarded(Args&&... args) {
    return std::make_unique<T>(std::forward<Args>(args)...);
}

// ===== COPY ELISION AND SINKS =====

/// @brief Returns a prvalue: C++17 guarantees no copy or move happens at the call site.
/// @param counts Observer. @return A freshly constructed resource.
[[nodiscard]] TrackedResource makeResource(OperationCounts* counts);

/// @brief Sink-argument idiom: take by value and move into place (1 copy for lvalues, 0 for rvalues).
/// @param name Name to store. @return A string built from `name`.
[[nodiscard]] std::string makeCallSign(std::string name);

// ===== MOVE-AWARE CONTAINER =====

/// @brief A minimal growable array showing how containers use move semantics internally.
///
/// On reallocation elements are transferred with `std::move_if_noexcept`: types whose move constructor
/// is `noexcept` are moved, others are copied so a throwing transfer leaves the original intact
/// (strong exception guarantee for `push_back`/`emplace_back`/`reserve`). Raw storage is managed by
/// `std::allocator<T>` and fully encapsulated (rule of five).
/// @tparam T Element type (must be nothrow destructible).
template <typename T>
class MoveAwareVector {
    static_assert(std::is_nothrow_destructible_v<T>, "MoveAwareVector requires nothrow destructible T");

public:
    using value_type = T;            ///< Element type.
    using size_type = std::size_t;   ///< Size type.
    using iterator = T*;             ///< Mutable iterator.
    using const_iterator = const T*; ///< Const iterator.

    MoveAwareVector() noexcept = default;

    /// @brief Constructs from a list of values. @param init Values to copy.
    MoveAwareVector(std::initializer_list<T> init) {
        reserve(init.size());
        for (const T& v : init) {
            emplace_back(v);
        }
    }

    /// @brief Deep copy. @param other Source.
    MoveAwareVector(const MoveAwareVector& other) {
        reserve(other.size_);
        for (const T& v : other) {
            emplace_back(v);
        }
    }

    /// @brief Steals `other`'s buffer in O(1). @param other Source (left empty).
    MoveAwareVector(MoveAwareVector&& other) noexcept
        : data_(std::exchange(other.data_, nullptr))
        , size_(std::exchange(other.size_, 0))
        , capacity_(std::exchange(other.capacity_, 0)) {}

    /// @brief Copy-and-swap assignment (strong guarantee). @param other Source. @return *this.
    MoveAwareVector& operator=(const MoveAwareVector& other) {
        if (this != &other) {
            MoveAwareVector tmp(other);
            swap(tmp);
        }
        return *this;
    }

    /// @brief Move assignment. @param other Source (left empty). @return *this.
    MoveAwareVector& operator=(MoveAwareVector&& other) noexcept {
        if (this != &other) {
            MoveAwareVector tmp(std::move(other));
            swap(tmp);
        }
        return *this;
    }

    /// @brief Destroys all elements and releases storage.
    ~MoveAwareVector() { release(); }

    /// @brief Appends a copy. @param value Value.
    void push_back(const T& value) { emplace_back(value); }
    /// @brief Appends by moving. @param value Value.
    void push_back(T&& value) { emplace_back(std::move(value)); }

    /// @brief Constructs an element at the end. Safe even if an argument aliases an element.
    /// @param args Constructor arguments. @return Reference to the new element.
    template <typename... Args>
    T& emplace_back(Args&&... args) {
        if (size_ < capacity_) {
            T* slot = std::construct_at(data_ + size_, std::forward<Args>(args)...);
            ++size_;
            return *slot;
        }
        const size_type newCapacity = capacity_ == 0 ? 1 : capacity_ * 2;
        T* newData = allocate(newCapacity);
        T* slot = nullptr;
        try {
            // Construct the new element first: `args` may refer into the old buffer.
            slot = std::construct_at(newData + size_, std::forward<Args>(args)...);
        } catch (...) {
            deallocate(newData, newCapacity);
            throw;
        }
        try {
            transferInto(newData);
        } catch (...) {
            std::destroy_at(slot);
            deallocate(newData, newCapacity);
            throw;
        }
        destroyAndDeallocate();
        data_ = newData;
        capacity_ = newCapacity;
        ++size_;
        ++reallocations_;
        return *slot;
    }

    /// @brief Removes the last element. @throws std::out_of_range if empty.
    void pop_back() {
        if (size_ == 0) {
            throw std::out_of_range("MoveAwareVector::pop_back on empty vector");
        }
        --size_;
        std::destroy_at(data_ + size_);
    }

    /// @brief Ensures capacity for at least `newCapacity` elements. @param newCapacity Requested capacity.
    void reserve(size_type newCapacity) {
        if (newCapacity <= capacity_) {
            return;
        }
        T* newData = allocate(newCapacity);
        try {
            transferInto(newData);
        } catch (...) {
            deallocate(newData, newCapacity);
            throw;
        }
        destroyAndDeallocate();
        data_ = newData;
        capacity_ = newCapacity;
        ++reallocations_;
    }

    /// @brief Destroys all elements (capacity is kept).
    void clear() noexcept {
        std::destroy(data_, data_ + size_);
        size_ = 0;
    }

    /// @brief Bounds-checked access. @param i Index. @return Element. @throws std::out_of_range.
    T& at(size_type i) {
        if (i >= size_) {
            throw std::out_of_range("MoveAwareVector::at index out of range");
        }
        return data_[i];
    }
    /// @brief Bounds-checked access. @param i Index. @return Element. @throws std::out_of_range.
    [[nodiscard]] const T& at(size_type i) const {
        if (i >= size_) {
            throw std::out_of_range("MoveAwareVector::at index out of range");
        }
        return data_[i];
    }
    /// @brief Unchecked access. @param i Index (< size()). @return Element.
    T& operator[](size_type i) noexcept { return data_[i]; }
    /// @brief Unchecked access. @param i Index (< size()). @return Element.
    const T& operator[](size_type i) const noexcept { return data_[i]; }

    /// @brief Element count. @return Size.
    [[nodiscard]] size_type size() const noexcept { return size_; }
    /// @brief Allocated slots. @return Capacity.
    [[nodiscard]] size_type capacity() const noexcept { return capacity_; }
    /// @brief Whether there are no elements. @return True if empty.
    [[nodiscard]] bool empty() const noexcept { return size_ == 0; }
    /// @brief How many times storage was reallocated. @return Reallocation count.
    [[nodiscard]] size_type reallocations() const noexcept { return reallocations_; }

    /// @brief Begin iterator. @return Pointer to the first element.
    iterator begin() noexcept { return data_; }
    /// @brief End iterator. @return One past the last element.
    iterator end() noexcept { return data_ + size_; }
    /// @brief Begin iterator. @return Pointer to the first element.
    [[nodiscard]] const_iterator begin() const noexcept { return data_; }
    /// @brief End iterator. @return One past the last element.
    [[nodiscard]] const_iterator end() const noexcept { return data_ + size_; }

    /// @brief O(1) swap. @param other Vector to swap with.
    void swap(MoveAwareVector& other) noexcept {
        std::swap(data_, other.data_);
        std::swap(size_, other.size_);
        std::swap(capacity_, other.capacity_);
        std::swap(reallocations_, other.reallocations_);
    }

private:
    static T* allocate(size_type n) { return std::allocator<T>{}.allocate(n); }
    static void deallocate(T* p, size_type n) noexcept {
        if (p != nullptr) {
            std::allocator<T>{}.deallocate(p, n);
        }
    }

    /// Moves (if noexcept) or copies the current elements into `dest`; on failure destroys what was
    /// built in `dest` and rethrows, leaving `*this` untouched.
    void transferInto(T* dest) {
        size_type built = 0;
        try {
            for (; built < size_; ++built) {
                std::construct_at(dest + built, std::move_if_noexcept(data_[built]));
            }
        } catch (...) {
            std::destroy(dest, dest + built);
            throw;
        }
    }

    void destroyAndDeallocate() noexcept {
        std::destroy(data_, data_ + size_);
        deallocate(data_, capacity_);
    }

    void release() noexcept {
        destroyAndDeallocate();
        data_ = nullptr;
        size_ = 0;
        capacity_ = 0;
    }

    T* data_ = nullptr; // owned; managed exclusively by this class
    size_type size_ = 0;
    size_type capacity_ = 0;
    size_type reallocations_ = 0;
};

// ===== SHOWCASES =====

/// @brief Copy vs move of a `TrackedResource`, with counters. @param out Destination stream.
void demonstrateBasicMoveSemantics(std::ostream& out = std::cout);
/// @brief Value categories and `std::forward`. @param out Destination stream.
void demonstratePerfectForwarding(std::ostream& out = std::cout);
/// @brief A move-only `Spacecraft`. @param out Destination stream.
void demonstrateMoveOnlyTypes(std::ostream& out = std::cout);
/// @brief `MoveAwareVector` growth with `move_if_noexcept`. @param out Destination stream.
void demonstrateMoveAwareContainer(std::ostream& out = std::cout);
/// @brief Copy elision and the sink idiom. @param out Destination stream.
void demonstrateOptimizationPatterns(std::ostream& out = std::cout);
/// @brief Runs every move-semantics showcase. @param out Destination stream.
void demonstrateAllMoveSemantics(std::ostream& out = std::cout);

} // namespace CppVerseHub::Modern::MoveSemantics
