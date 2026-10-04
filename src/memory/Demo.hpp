/**
 * @file Demo.hpp
 * @brief Single entry point that runs every memory-management showcase.
 *
 * Runs the custom allocator, memory pool, RAII and smart pointer demonstrations in
 * sequence. Deterministic, quick and non-throwing, so a CLI can call it directly.
 */

#ifndef CPPVERSEHUB_MEMORY_DEMO_HPP
#define CPPVERSEHUB_MEMORY_DEMO_HPP

#include <iostream>

namespace CppVerseHub::Memory {

    /**
     * @brief Run every memory showcase end-to-end.
     * @param out Stream receiving the narration (exceptions are caught and reported to it).
     */
    void runDemo(std::ostream& out = std::cout);

} // namespace CppVerseHub::Memory

#endif // CPPVERSEHUB_MEMORY_DEMO_HPP
