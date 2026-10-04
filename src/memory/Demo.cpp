/**
 * @file Demo.cpp
 * @brief Implementation of CppVerseHub::Memory::runDemo.
 */

#include "memory/Demo.hpp"

#include "memory/CustomAllocators.hpp"
#include "memory/MemoryPools.hpp"
#include "memory/RAII_Examples.hpp"
#include "memory/SmartPointers.hpp"

#include <exception>

namespace CppVerseHub::Memory {

    void runDemo(std::ostream& out) {
        using Showcase = void (*)(std::ostream&);
        constexpr Showcase showcases[] = {demonstrateCustomAllocators, demonstrateMemoryPools, demonstrateRAII,
                                          demonstrateSmartPointers};
        for (const Showcase showcase : showcases) {
            try {
                showcase(out);
            } catch (const std::exception& e) {
                out << "[memory demo error] " << e.what() << "\n";
            } catch (...) {
                out << "[memory demo error] unknown exception\n";
            }
            out << "\n";
        }
    }

} // namespace CppVerseHub::Memory
