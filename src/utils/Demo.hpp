/**
 * @file Demo.hpp
 * @brief Single entry point that runs every utils showcase (logging, configuration, parsing, math,
 * strings and time) deterministically, writing only to the supplied stream.
 */
#pragma once

#include <iostream>
#include <ostream>

namespace CppVerseHub::Utils {

/**
 * @brief Runs all utils demonstrations end-to-end. Never throws; any unexpected error is reported to `out`.
 * @param out Destination stream.
 */
void runDemo(std::ostream& out = std::cout);

} // namespace CppVerseHub::Utils
