/**
 * @file Demo.cpp
 * @brief Implementation of the algorithms module demo entry point.
 */

#include "algorithms/Demo.hpp"

#include "algorithms/DataStructures.hpp"
#include "algorithms/GraphAlgorithms.hpp"
#include "algorithms/SearchAlgorithms.hpp"
#include "algorithms/SortingAlgorithms.hpp"

#include <exception>

namespace CppVerseHub::Algorithms {

void runDemo(std::ostream& out) {
    try {
        demonstrate_sorting(out);
        out << '\n';
        demonstrate_searching(out);
        out << '\n';
        demonstrate_graphs(out);
        out << '\n';
        demonstrate_data_structures(out);
    } catch (const std::exception& e) {
        out << "algorithms demo failed: " << e.what() << '\n';
    } catch (...) {
        out << "algorithms demo failed: unknown exception\n";
    }
}

}  // namespace CppVerseHub::Algorithms
