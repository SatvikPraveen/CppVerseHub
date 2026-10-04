# concurrency module
cppversehub_add_module(
    NAME  concurrency
    ALIAS Concurrency
    SOURCES
    AsyncComms.cpp
    AsyncMissions.cpp
    Atomics.cpp
    ConditionalVariables.cpp
    CoroutinesDemo.cpp
    MutexExamples.cpp
    ThreadPool.cpp
)
