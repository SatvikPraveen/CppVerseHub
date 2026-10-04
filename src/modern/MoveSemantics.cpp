/**
 * @file MoveSemantics.cpp
 * @brief Implementation of the move-semantics showcase (see MoveSemantics.hpp).
 */
#include "modern/MoveSemantics.hpp"

#include <numeric>

namespace CppVerseHub::Modern::MoveSemantics {

// ===== TrackedResource =====

TrackedResource::TrackedResource(std::string name, std::size_t payloadSize, OperationCounts* counts)
    : name_(std::move(name)), payload_(std::make_unique<std::vector<int>>(payloadSize)), counts_(counts) {
    std::iota(payload_->begin(), payload_->end(), 0);
    if (counts_ != nullptr) {
        ++counts_->constructions;
    }
}

TrackedResource::TrackedResource(const TrackedResource& other)
    : name_(other.name_),
      payload_(other.payload_ ? std::make_unique<std::vector<int>>(*other.payload_) : nullptr),
      counts_(other.counts_) {
    if (counts_ != nullptr) {
        ++counts_->copies;
    }
}

TrackedResource::TrackedResource(TrackedResource&& other) noexcept
    : name_(std::exchange(other.name_, {})), payload_(std::move(other.payload_)), counts_(other.counts_) {
    if (counts_ != nullptr) {
        ++counts_->moves;
    }
}

TrackedResource& TrackedResource::operator=(const TrackedResource& other) {
    if (this != &other) {
        // Copy-and-swap: the copy may throw and leave *this untouched. Recorded as one copy, plus
        // one destruction when the temporary holding the old state dies.
        TrackedResource copy(other);
        swap(*this, copy);
    }
    return *this;
}

TrackedResource& TrackedResource::operator=(TrackedResource&& other) noexcept {
    if (this != &other) {
        name_ = std::exchange(other.name_, {});
        payload_ = std::move(other.payload_);
        counts_ = other.counts_;
        if (counts_ != nullptr) {
            ++counts_->moves;
        }
    }
    return *this;
}

TrackedResource::~TrackedResource() {
    if (counts_ != nullptr) {
        ++counts_->destructions;
    }
}

long long TrackedResource::checksum() const noexcept {
    return payload_ ? std::accumulate(payload_->begin(), payload_->end(), 0LL) : 0LL;
}

void TrackedResource::append(int value) {
    if (!payload_) {
        payload_ = std::make_unique<std::vector<int>>();
    }
    payload_->push_back(value);
}

// ===== Spacecraft =====

Spacecraft::Spacecraft(int id, std::string name)
    : id_(id), name_(std::move(name)), log_(std::make_unique<std::vector<std::string>>()) {}

Spacecraft::Spacecraft(Spacecraft&& other) noexcept
    : id_(std::exchange(other.id_, -1)),
      name_(std::move(other.name_)),
      cargo_(std::move(other.cargo_)),
      log_(std::move(other.log_)) {}

Spacecraft& Spacecraft::operator=(Spacecraft&& other) noexcept {
    if (this != &other) {
        id_ = std::exchange(other.id_, -1);
        name_ = std::move(other.name_);
        cargo_ = std::move(other.cargo_);
        log_ = std::move(other.log_);
    }
    return *this;
}

void Spacecraft::loadCargo(TrackedResource item) {
    cargo_.push_back(std::move(item));
}

TrackedResource Spacecraft::unloadCargo() {
    if (cargo_.empty()) {
        throw std::out_of_range("Spacecraft::unloadCargo: no cargo");
    }
    TrackedResource item = std::move(cargo_.back());
    cargo_.pop_back();
    return item;
}

void Spacecraft::log(std::string entry) {
    if (!log_) {
        log_ = std::make_unique<std::vector<std::string>>();
    }
    log_->push_back(std::move(entry));
}

// ===== Elision / sinks =====

TrackedResource makeResource(OperationCounts* counts) {
    return TrackedResource("factory-made", 16, counts);  // prvalue: constructed directly in the caller
}

std::string makeCallSign(std::string name) {
    name += "-01";
    return name;  // implicitly moved (or NRVO'd)
}

// ===== SHOWCASES =====

namespace {

void printCounts(std::ostream& out, std::string_view label, const OperationCounts& c) {
    out << "  " << label << ": constructions=" << c.constructions << " copies=" << c.copies
        << " moves=" << c.moves << " destructions=" << c.destructions << '\n';
}

/// A type whose move constructor may throw, so containers must copy it on reallocation.
struct ThrowingMove {
    explicit ThrowingMove(OperationCounts* c) : counts(c) {}
    ThrowingMove(const ThrowingMove& o) : counts(o.counts) { ++counts->copies; }
    ThrowingMove(ThrowingMove&& o) noexcept(false) : counts(o.counts) { ++counts->moves; }
    ThrowingMove& operator=(const ThrowingMove&) = default;
    ThrowingMove& operator=(ThrowingMove&&) = default;
    ~ThrowingMove() = default;
    OperationCounts* counts;
};

}  // namespace

void demonstrateBasicMoveSemantics(std::ostream& out) {
    out << "\n--- Copy vs move ---\n";
    OperationCounts counts;
    {
        TrackedResource original("ore", 1000, &counts);
        TrackedResource copy = original;             // deep copy
        TrackedResource moved = std::move(original); // pointer steal
        out << "  original moved-from: " << std::boolalpha << original.isMovedFrom()  // NOLINT(bugprone-use-after-move)
            << ", copy checksum " << copy.checksum() << ", moved checksum " << moved.checksum() << std::noboolalpha
            << '\n';
        original = copy;  // a moved-from object can be assigned again
        out << "  reassigned moved-from object, size " << original.size() << '\n';
    }
    printCounts(out, "after scope", counts);
}

void demonstratePerfectForwarding(std::ostream& out) {
    out << "\n--- Value categories and perfect forwarding ---\n";
    std::string name = "Enterprise";
    const std::string constName = "Defiant";
    out << "  categoryOf(name)            = " << toString(categoryOf(name)) << '\n';
    out << "  categoryOf(constName)       = " << toString(categoryOf(constName)) << '\n';
    out << "  categoryOf(std::move(name)) = " << toString(categoryOf(std::move(name))) << '\n';
    out << "  relayForwarded(lvalue)        -> " << relayForwarded(name) << '\n';
    out << "  relayForwarded(rvalue)        -> " << relayForwarded(std::string("temp")) << '\n';
    out << "  relayWithoutForward(rvalue)   -> " << relayWithoutForward(std::string("temp"))
        << " (named rvalue references are lvalues)\n";
    auto ship = makeUniqueForwarded<Spacecraft>(7, std::string("Forwarded"));
    out << "  makeUniqueForwarded<Spacecraft> -> " << ship->name() << '\n';
    out << "  forwardTo(std::plus<>, 2, 40) = " << forwardTo(std::plus<>{}, 2, 40) << '\n';
}

void demonstrateMoveOnlyTypes(std::ostream& out) {
    out << "\n--- Move-only types ---\n";
    OperationCounts counts;
    Spacecraft scout(1, "Scout");
    scout.log("launched");
    TrackedResource fuel("fuel", 64, &counts);
    scout.loadCargo(fuel);                                         // copy into the sink
    scout.loadCargo(TrackedResource("water", 32, &counts));        // prvalue: moved, not copied
    scout.emplaceCargo("medkits", std::size_t{8}, &counts);        // constructed in place
    Spacecraft flagship = std::move(scout);
    out << "  after move: flagship '" << flagship.name() << "' has " << flagship.cargoCount() << " cargo, "
        << flagship.logSize() << " log entries; scout valid=" << std::boolalpha << scout.isValid()  // NOLINT
        << std::noboolalpha << " id=" << scout.id() << '\n';
    const TrackedResource unloaded = flagship.unloadCargo();
    out << "  unloaded '" << unloaded.name() << "'\n";
    printCounts(out, "cargo operations", counts);
}

void demonstrateMoveAwareContainer(std::ostream& out) {
    out << "\n--- MoveAwareVector and move_if_noexcept ---\n";
    OperationCounts nothrowCounts;
    {
        MoveAwareVector<TrackedResource> v;
        for (int i = 0; i < 8; ++i) {
            v.emplace_back("crate-" + std::to_string(i), std::size_t{4}, &nothrowCounts);
        }
        out << "  8 noexcept-movable elements: size=" << v.size() << " capacity=" << v.capacity()
            << " reallocations=" << v.reallocations() << '\n';
    }
    printCounts(out, "noexcept move (elements moved on growth)", nothrowCounts);

    OperationCounts throwingCounts;
    {
        MoveAwareVector<ThrowingMove> v;
        for (int i = 0; i < 8; ++i) {
            v.emplace_back(&throwingCounts);
        }
    }
    printCounts(out, "potentially-throwing move (elements copied on growth)", throwingCounts);
}

void demonstrateOptimizationPatterns(std::ostream& out) {
    out << "\n--- Copy elision and sink arguments ---\n";
    OperationCounts counts;
    {
        TrackedResource r = makeResource(&counts);  // guaranteed elision: exactly one construction
        out << "  makeResource() -> '" << r.name() << "'\n";
    }
    printCounts(out, "prvalue return", counts);
    std::string base = "Falcon";
    out << "  makeCallSign(lvalue) = " << makeCallSign(base) << " (base still '" << base << "')\n";
    out << "  makeCallSign(std::move(base)) = " << makeCallSign(std::move(base)) << '\n';
}

void demonstrateAllMoveSemantics(std::ostream& out) {
    out << "\n=== Move Semantics & Perfect Forwarding ===\n";
    demonstrateBasicMoveSemantics(out);
    demonstratePerfectForwarding(out);
    demonstrateMoveOnlyTypes(out);
    demonstrateMoveAwareContainer(out);
    demonstrateOptimizationPatterns(out);
}

}  // namespace CppVerseHub::Modern::MoveSemantics
