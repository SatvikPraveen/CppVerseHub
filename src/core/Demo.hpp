/**
 * @file Demo.hpp
 * @brief Entry point that showcases the whole core module end to end.
 */
#pragma once

#include <iostream>
#include <ostream>

namespace CppVerseHub::Core {

/**
 * @brief Run every core showcase (Vector3D, strong ids, entities, factory, event bus, resource
 *        conservation, missions, deterministic simulation, save/load round trip), writing only to `out`.
 *
 * Deterministic, fast (well under a second) and never throws.
 * @param out Destination stream.
 */
void runDemo(std::ostream& out = std::cout);

} // namespace CppVerseHub::Core
