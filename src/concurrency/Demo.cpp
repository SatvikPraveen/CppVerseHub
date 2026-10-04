/**
 * @file Demo.cpp
 * @brief Implementation of the concurrency module's `runDemo()`.
 * @details File location: src/concurrency/Demo.cpp
 */

#include "concurrency/Demo.hpp"

#include <exception>

#include "concurrency/AsyncComms.hpp"
#include "concurrency/AsyncMissions.hpp"
#include "concurrency/Atomics.hpp"
#include "concurrency/ConditionalVariables.hpp"
#include "concurrency/CoroutinesDemo.hpp"
#include "concurrency/MutexExamples.hpp"
#include "concurrency/ThreadPool.hpp"

namespace CppVerseHub::Concurrency {

    void runDemo(std::ostream& out) {
        try {
            out << "##### CppVerseHub concurrency showcase #####\n\n";
            demonstrate_thread_pools(out);
            demonstrate_mutexes(out);
            demonstrate_condition_variables(out);
            demonstrate_atomics(out);
            demonstrate_async_missions(out);
            demonstrate_async_comms(out);
            demonstrate_coroutines(out);
        } catch (const std::exception& e) {
            out << "Concurrency demo aborted: " << e.what() << '\n';
        } catch (...) {
            out << "Concurrency demo aborted: unknown exception\n";
        }
    }

}  // namespace CppVerseHub::Concurrency
