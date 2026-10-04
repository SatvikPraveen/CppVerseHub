/**
 * @file Demo.cpp
 * @brief Implementation of the patterns module demo entry point.
 */

#include "patterns/Demo.hpp"

#include "patterns/Adapter.hpp"
#include "patterns/Builder.hpp"
#include "patterns/Command.hpp"
#include "patterns/Decorator.hpp"
#include "patterns/Observer.hpp"
#include "patterns/Singleton.hpp"
#include "patterns/State.hpp"
#include "patterns/Strategy.hpp"

#include <exception>

namespace CppVerseHub::Patterns {

void runDemo(std::ostream& out) {
    using Showcase = void (*)(std::ostream&);
    constexpr Showcase showcases[] = {&demonstrateAdapter,   &demonstrateBuilder,  &demonstrateCommand,
                                      &demonstrateDecorator, &demonstrateObserver, &demonstrateSingleton,
                                      &demonstrateState,     &demonstrateStrategy};
    for (const Showcase showcase : showcases) {
        try {
            showcase(out);
        } catch (const std::exception& e) {
            out << "  [showcase failed: " << e.what() << "]\n";
        } catch (...) {
            out << "  [showcase failed: unknown exception]\n";
        }
        out << '\n';
    }
}

} // namespace CppVerseHub::Patterns
