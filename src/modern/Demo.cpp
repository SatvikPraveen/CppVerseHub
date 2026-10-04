/**
 * @file Demo.cpp
 * @brief Implementation of `CppVerseHub::Modern::runDemo`.
 */
#include "modern/Demo.hpp"

#include <exception>

#include "modern/ConceptsAdvanced.hpp"
#include "modern/ConstexprProgramming.hpp"
#include "modern/LambdaExpressions.hpp"
#include "modern/ModulesDemo.hpp"
#include "modern/MoveSemantics.hpp"
#include "modern/RangesDemo.hpp"
#include "modern/StructuredBindings.hpp"

namespace CppVerseHub::Modern {

void runDemo(std::ostream& out) {
    try {
        out << "===== CppVerseHub: Modern C++ showcase =====\n";
        Concepts::demonstrateConcepts(out);
        ConstexprProgramming::demonstrateConstexpr(out);
        LambdaExpressions::demonstrateAllLambdas(out);
        MoveSemantics::demonstrateAllMoveSemantics(out);
        Ranges::demonstrateAllRanges(out);
        StructuredBindings::demonstrateStructuredBindings(out);
        Modules::demonstrateModules(out);
        out << "\n===== Modern C++ showcase complete =====\n";
    } catch (const std::exception& e) {
        out << "\n[modern] demo aborted by exception: " << e.what() << '\n';
    } catch (...) {
        out << "\n[modern] demo aborted by unknown exception\n";
    }
}

}  // namespace CppVerseHub::Modern
