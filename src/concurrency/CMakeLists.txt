# concurrency module: thread pools, synchronisation primitives, atomics and
# lock-free structures, futures-based orchestration, message passing, coroutines.
cppversehub_add_module(
    NAME  concurrency
    ALIAS Concurrency
    SOURCES
    AsyncComms.cpp
    AsyncMissions.cpp
    Atomics.cpp
    ConditionalVariables.cpp
    CoroutinesDemo.cpp
    Demo.cpp
    MutexExamples.cpp
    ThreadPool.cpp
)
