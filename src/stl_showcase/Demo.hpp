/**
 * @file Demo.hpp
 * @brief Single entry point that runs the whole STL showcase (containers, algorithms, iterators,
 *        functors and vocabulary utilities) deterministically.
 */
#pragma once

#include <iostream>

namespace CppVerseHub::STL {

/**
 * @brief Run every STL showcase end to end, writing only to @p out. Never throws.
 * @param out Destination stream.
 */
void runDemo(std::ostream& out = std::cout);

} // namespace CppVerseHub::STL
