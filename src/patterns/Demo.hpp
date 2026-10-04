/**
 * @file Demo.hpp
 * @brief Single entry point that runs every design-pattern showcase of the patterns module.
 */

#pragma once

#include <iostream>

namespace CppVerseHub::Patterns {

/**
 * @brief Run every pattern showcase (Adapter, Builder, Command, Decorator, Observer, Singleton,
 *        State, Strategy) deterministically. Never throws.
 * @param out Stream receiving the narration.
 */
void runDemo(std::ostream& out = std::cout);

} // namespace CppVerseHub::Patterns
