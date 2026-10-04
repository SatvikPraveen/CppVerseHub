# Changelog

All notable changes to this project are documented here.
The format follows [Keep a Changelog](https://keepachangelog.com/en/1.1.0/) and the project uses [Semantic Versioning](https://semver.org/).

## [2.0.0] - 2026-10-04

### Added
- Modular CMake build: one exported library per module plus the `CppVerseHub::CppVerseHub` umbrella target.
- CMake presets for Release, Debug, ASan/UBSan, TSan, coverage, strict warnings and clang-tidy.
- Catch2 v3 test suites and Google Benchmark suites for every module.
- Deterministic, seeded space-fleet simulation core with JSON scenario persistence.
- Continuous integration across GCC 13/14, Clang 18, Apple Clang 16 and MSVC 2022, with sanitizer, coverage, static-analysis and documentation jobs.
- API documentation published to GitHub Pages.
- Zero-finding clang-tidy and cppcheck baselines, enforced in CI, plus an enforced clang-format check.
- Warnings are errors on every CI toolchain, including MSVC.
- `CITATION.cff`, contribution guide, code of conduct and security policy.

### Changed
- All modules rewritten to compile warning-free under a strict warning set on all supported compilers.
- Headers are included relative to `src/` (for example `"concurrency/ThreadPool.hpp"`).
- Library code no longer prints; demonstrations write to a caller-supplied `std::ostream`.

## [1.0.0] - 2025

### Added
- Initial project scaffold.
