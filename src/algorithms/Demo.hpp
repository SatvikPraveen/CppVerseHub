/**
 * @file Demo.hpp
 * @brief Entry point that runs every showcase of the algorithms module.
 *
 * The algorithms module demonstrates generic, concept-constrained algorithms (sorting, searching,
 * graphs) and classic data structures. `runDemo` exercises all of them deterministically.
 */

#ifndef CPPVERSEHUB_ALGORITHMS_DEMO_HPP
#define CPPVERSEHUB_ALGORITHMS_DEMO_HPP

#include <iostream>

namespace CppVerseHub::Algorithms {

/**
 * @brief Runs the sorting, searching, graph and data-structure showcases in sequence.
 * @param out Stream that receives all output. Never throws (errors are reported to `out`).
 */
void runDemo(std::ostream& out = std::cout);

}  // namespace CppVerseHub::Algorithms

#endif  // CPPVERSEHUB_ALGORITHMS_DEMO_HPP
