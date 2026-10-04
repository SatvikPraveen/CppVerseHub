/**
 * @file Random.hpp
 * @brief Portable, reproducible random number generation for the simulation.
 *
 * `std::mt19937_64` has a fully specified output sequence, but the standard *distributions*
 * (`std::uniform_real_distribution`, ...) are implementation-defined, so libstdc++, libc++ and MSVC
 * produce different values from the same seed. `DeterministicRng` therefore derives every random
 * quantity directly from the raw engine output using fixed, documented formulas. Together with a
 * fixed timestep this makes simulation runs bit-identical for a given seed and build. The engine
 * state can be serialised to text (standard `operator<<` format) so saved scenarios resume exactly.
 */
#pragma once

#include <cstdint>
#include <limits>
#include <random>
#include <sstream>
#include <string>

#include "core/Exceptions.hpp"

namespace CppVerseHub::Core {

/// @brief Seeded random source with implementation-independent derived distributions.
class DeterministicRng {
public:
    /// @brief Seed used when none is given.
    static constexpr std::uint64_t kDefaultSeed = 0x5EED'C0DE'2026ULL;

    /// @brief Construct from a seed. @param seed Seed for the underlying mt19937_64.
    explicit DeterministicRng(std::uint64_t seed = kDefaultSeed) : engine_(seed), seed_(seed) {}

    /// @brief Seed this generator was constructed (or last reseeded) with. @return Seed.
    [[nodiscard]] std::uint64_t seed() const noexcept { return seed_; }

    /// @brief Reset to the start of the sequence for a seed. @param seed New seed.
    void reseed(std::uint64_t seed) {
        engine_.seed(seed);
        seed_ = seed;
    }

    /// @brief Raw 64-bit engine output. @return Next value of the mt19937_64 sequence.
    [[nodiscard]] std::uint64_t nextU64() { return engine_(); }

    /// @brief Uniform double in [0, 1) using the top 53 bits. @return Random double.
    [[nodiscard]] double uniform01() {
        constexpr double kScale = 1.0 / 9007199254740992.0; // 2^-53
        return static_cast<double>(nextU64() >> 11U) * kScale;
    }

    /**
     * @brief Uniform double in [lo, hi).
     * @param lo Lower bound.
     * @param hi Upper bound (must be >= lo).
     * @return Random double.
     */
    [[nodiscard]] double uniform(double lo, double hi) {
        if (!(hi >= lo)) {
            throw InvalidArgumentException("DeterministicRng::uniform requires lo <= hi");
        }
        return lo + (hi - lo) * uniform01();
    }

    /**
     * @brief Unbiased uniform integer in [0, n) via rejection sampling.
     * @param n Exclusive upper bound, must be positive.
     * @return Random integer.
     */
    [[nodiscard]] std::uint64_t below(std::uint64_t n) {
        if (n == 0) {
            throw InvalidArgumentException("DeterministicRng::below requires n > 0");
        }
        // threshold = 2^64 mod n; rejecting raw values below it leaves a multiple of n outcomes.
        const std::uint64_t threshold = (std::numeric_limits<std::uint64_t>::max() - n + 1U) % n;
        for (;;) {
            const std::uint64_t r = nextU64();
            if (r >= threshold) {
                return r % n;
            }
        }
    }

    /**
     * @brief Unbiased uniform integer in the closed range [lo, hi].
     * @param lo Lower bound.
     * @param hi Upper bound (must be >= lo).
     * @return Random integer.
     */
    [[nodiscard]] std::int64_t uniformInt(std::int64_t lo, std::int64_t hi) {
        if (hi < lo) {
            throw InvalidArgumentException("DeterministicRng::uniformInt requires lo <= hi");
        }
        const std::uint64_t span = static_cast<std::uint64_t>(hi) - static_cast<std::uint64_t>(lo);
        const std::uint64_t offset = span == std::numeric_limits<std::uint64_t>::max() ? nextU64()
                                                                                       : below(span + 1U);
        return static_cast<std::int64_t>(static_cast<std::uint64_t>(lo) + offset);
    }

    /**
     * @brief Bernoulli trial.
     * @param probability Success probability (<= 0 never succeeds, >= 1 always succeeds).
     * @return true with the given probability.
     */
    [[nodiscard]] bool chance(double probability) { return uniform01() < probability; }

    /// @brief Serialise the full engine state (standard textual format). @return State string.
    [[nodiscard]] std::string saveState() const {
        std::ostringstream os;
        os << engine_;
        return os.str();
    }

    /**
     * @brief Restore engine state produced by saveState().
     * @param state State string.
     * @param seed Seed to report from seed() afterwards.
     */
    void loadState(const std::string& state, std::uint64_t seed) {
        std::istringstream is(state);
        std::mt19937_64 restored;
        is >> restored;
        if (!is) {
            throw SerializationException("malformed RNG state");
        }
        engine_ = restored;
        seed_ = seed;
    }

    /// @brief Engines compare equal iff their future sequences are identical. @return Equality.
    friend bool operator==(const DeterministicRng& a, const DeterministicRng& b) { return a.engine_ == b.engine_; }

private:
    std::mt19937_64 engine_;
    std::uint64_t seed_;
};

} // namespace CppVerseHub::Core
