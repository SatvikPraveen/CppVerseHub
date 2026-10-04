/**
 * @file Demo.hpp
 * @brief Single entry point that runs every showcase of the templates module.
 *
 * The module is header-only, so runDemo is an inline function. It writes only to the supplied
 * stream, is deterministic, takes no input and never throws (exceptions are caught and reported).
 */

#ifndef CPPVERSEHUB_TEMPLATES_DEMO_HPP
#define CPPVERSEHUB_TEMPLATES_DEMO_HPP

#include "templates/ConceptsDemo.hpp"
#include "templates/GenericContainers.hpp"
#include "templates/MetaProgramming.hpp"
#include "templates/SFINAE_Examples.hpp"
#include "templates/TemplateSpecialization.hpp"
#include "templates/VariadicTemplates.hpp"

#include <exception>
#include <iostream>
#include <ostream>

namespace CppVerseHub::Templates {

/**
 * @brief Run all template-programming showcases end to end.
 * @param out destination stream (defaults to std::cout)
 */
inline void runDemo(std::ostream& out = std::cout) {
    try {
        out << "=== CppVerseHub :: Templates ===\n";
        Concepts::demonstrate_concepts(out);
        demonstrate_generic_containers(out);
        Meta::demonstrate_metaprogramming(out);
        SFINAE::demonstrate_sfinae(out);
        Specialization::demonstrate_specializations(out);
        Variadic::demonstrate_variadic_templates(out);
        out << "=== Templates demo complete ===\n";
    } catch (const std::exception& e) {
        out << "templates demo failed: " << e.what() << '\n';
    } catch (...) {
        out << "templates demo failed: unknown exception\n";
    }
}

} // namespace CppVerseHub::Templates

#endif // CPPVERSEHUB_TEMPLATES_DEMO_HPP
