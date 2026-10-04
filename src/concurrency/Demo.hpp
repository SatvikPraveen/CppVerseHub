/**
 * @file Demo.hpp
 * @brief Single entry point that runs every concurrency showcase in this module.
 * @details File location: src/concurrency/Demo.hpp
 *
 * The CLI calls `runDemo()` to exercise thread pools, mutexes, condition variables,
 * atomics and lock-free structures, futures-based orchestration, message passing and
 * C++20 coroutines end to end. All output goes to the supplied stream and is
 * deterministic (results are printed only after the concurrent work has been joined).
 */

#ifndef CPPVERSEHUB_CONCURRENCY_DEMO_HPP
#define CPPVERSEHUB_CONCURRENCY_DEMO_HPP

#include <iostream>

namespace CppVerseHub::Concurrency {

/**
 * @brief Runs every showcase of the concurrency module.
 *
 * Completes in well under a second, needs no input and never throws (any unexpected
 * exception is reported to `out`).
 * @param out Destination stream.
 */
void runDemo(std::ostream& out = std::cout);

} // namespace CppVerseHub::Concurrency

#endif // CPPVERSEHUB_CONCURRENCY_DEMO_HPP
