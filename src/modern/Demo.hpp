/**
 * @file Demo.hpp
 * @brief Single entry point that runs every showcase of the `modern` module (concepts, constexpr,
 *        lambdas, move semantics, ranges, structured bindings and the emulated modules system).
 */
#pragma once

#include <iostream>

namespace CppVerseHub::Modern {

/// @brief Runs every Modern C++ showcase end-to-end, deterministically and without user input.
///        Never throws: any unexpected exception is caught and reported on `out`.
/// @param out Destination stream for all output.
void runDemo(std::ostream& out = std::cout);

}  // namespace CppVerseHub::Modern
