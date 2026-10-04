# Detect whether std::atomic needs an explicit -latomic.
#
# Clang with libstdc++ lowers std::atomic<T>::is_lock_free() (and atomics wider than the
# native word) to calls into libatomic, which the driver does not link automatically.
# GCC, libc++ and MSVC either inline these or link the runtime themselves.
include(CheckCXXSourceCompiles)

set(_cppversehub_atomic_probe [=[
#include <atomic>
#include <cstdint>
struct Wide { std::uint64_t a; std::uint64_t b; };
int main() {
    std::atomic<int> i{0};
    std::atomic<std::uint64_t> u{0};
    std::atomic<Wide> w{Wide{0, 0}};
    return static_cast<int>(i.is_lock_free()) + static_cast<int>(u.fetch_add(1)) +
           static_cast<int>(w.load().a);
}
]=])

set(CMAKE_REQUIRED_FLAGS "-std=c++20")
if(MSVC)
    set(CMAKE_REQUIRED_FLAGS "/std:c++20")
endif()
check_cxx_source_compiles("${_cppversehub_atomic_probe}" CPPVERSEHUB_ATOMICS_WITHOUT_LIB)

set(CPPVERSEHUB_ATOMIC_LIBRARY "")
if(NOT CPPVERSEHUB_ATOMICS_WITHOUT_LIB)
    set(CMAKE_REQUIRED_LIBRARIES atomic)
    check_cxx_source_compiles("${_cppversehub_atomic_probe}" CPPVERSEHUB_ATOMICS_WITH_LIB)
    unset(CMAKE_REQUIRED_LIBRARIES)
    if(CPPVERSEHUB_ATOMICS_WITH_LIB)
        set(CPPVERSEHUB_ATOMIC_LIBRARY atomic)
        message(STATUS "std::atomic requires libatomic: linking it")
    else()
        message(FATAL_ERROR "std::atomic does not link, with or without libatomic")
    endif()
endif()
unset(CMAKE_REQUIRED_FLAGS)
