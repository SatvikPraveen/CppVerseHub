/**
 * @file Demo.cpp
 * @brief Implementation of the STL showcase entry point.
 */
#include "stl_showcase/Demo.hpp"

#include <exception>

#include "stl_showcase/Algorithms.hpp"
#include "stl_showcase/Containers.hpp"
#include "stl_showcase/Functors.hpp"
#include "stl_showcase/Iterators.hpp"
#include "stl_showcase/STLUtilities.hpp"

namespace CppVerseHub::STL {

void runDemo(std::ostream& out) {
    try {
        out << "##### CppVerseHub STL Showcase #####\n";
        runContainersDemo(out);
        runAlgorithmsDemo(out);
        runIteratorsDemo(out);
        runFunctorsDemo(out);
        runSTLUtilitiesDemo(out);
        out << "\n##### STL Showcase complete #####\n";
    } catch (const std::exception& e) {
        out << "STL showcase aborted: " << e.what() << '\n';
    } catch (...) {
        out << "STL showcase aborted: unknown exception\n";
    }
}

}  // namespace CppVerseHub::STL
