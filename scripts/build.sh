#!/bin/bash
# File: scripts/build.sh
# Build automation script for CppVerseHub
# Supports Debug, Release, and Testing builds with comprehensive options

set -e  # Exit on any error

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
PURPLE='\033[0;35m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

# Build configuration
PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="$PROJECT_ROOT/build"
SOURCE_DIR="$PROJECT_ROOT"
BUILD_TYPE="Release"
CLEAN_BUILD=false
VERBOSE=false
PARALLEL_JOBS=$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo "4")
BUILD_TESTS=true
BUILD_EXAMPLES=true
BUILD_DOCS=false
ENABLE_SANITIZERS=false
ENABLE_COVERAGE=false
ENABLE_LTO=true
CXX_STANDARD="20"

# Function to print colored output
print_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

print_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

print_build() {
    echo -e "${PURPLE}[BUILD]${NC} $1"
}

print_debug() {
    if [ "$VERBOSE" = true ]; then
        echo -e "${CYAN}[DEBUG]${NC} $1"
    fi
}

# Function to show usage information
show_usage() {
    cat << EOF
CppVerseHub Build Script
========================

Usage: $0 [OPTIONS]

Options:
    -h, --help              Show this help message
    -c, --clean             Clean build (remove build directory first)
    -d, --debug             Build in Debug mode (default: Release)
    -r, --release           Build in Release mode
    -v, --verbose           Enable verbose output
    -j, --jobs N            Number of parallel jobs (default: $PARALLEL_JOBS)
    --no-tests              Skip building tests
    --no-examples           Skip building examples
    --docs                  Build documentation (requires Doxygen)
    --sanitizers            Enable AddressSanitizer and UBSan
    --coverage              Enable code coverage analysis
    --no-lto                Disable Link Time Optimization
    --std=VERSION           C++ standard version (default: $CXX_STANDARD)
    --compiler=COMPILER     Specify compiler (gcc, clang, or path)
    --generator=GEN         CMake generator (default: auto-detect)

Examples:
    $0                      # Release build with all features
    $0 -d -c               # Clean debug build
    $0 --sanitizers -j8    # Release build with sanitizers using 8 cores
    $0 -d --coverage       # Debug build with coverage analysis
    $0 --docs              # Build with documentation generation

Build Types:
    Debug     - No optimization, debug symbols, assertions enabled
    Release   - Full optimization, minimal debug info, NDEBUG defined

Features:
    - Automatic compiler detection and optimization
    - Parallel compilation support
    - Optional sanitizers and coverage analysis
    - Integrated testing and documentation build
    - Cross-platform compatibility (Linux, macOS, Windows)

EOF
}

# Function to detect compiler and set appropriate flags
detect_compiler() {
    local compiler_cmd="$1"
    
    if [ -n "$compiler_cmd" ]; then
        export CXX="$compiler_cmd"
        print_info "Using specified compiler: $compiler_cmd"
        return
    fi
    
    # Try to detect available compilers in order of preference
    if command -v g++ >/dev/null 2>&1; then
        export CXX="g++"
        print_info "Using GCC compiler: $(g++ --version | head -n1)"
    elif command -v clang++ >/dev/null 2>&1; then
        export CXX="clang++"
        print_info "Using Clang compiler: $(clang++ --version | head -n1)"
    else
        print_error "No suitable C++ compiler found!"
        print_error "Please install GCC or Clang, or specify compiler with --compiler option"
        exit 1
    fi
}

# Function to detect CMake generator
detect_generator() {
    local generator="$1"
    
    if [ -n "$generator" ]; then
        CMAKE_GENERATOR="-G $generator"
        print_info "Using specified generator: $generator"
        return
    fi
    
    # Auto-detect best generator
    if command -v ninja >/dev/null 2>&1; then
        CMAKE_GENERATOR="-G Ninja"
        print_info "Using Ninja generator (faster builds)"
    elif [[ "$OSTYPE" == "msys" || "$OSTYPE" == "cygwin" ]]; then
        CMAKE_GENERATOR="-G \"Visual Studio 16 2019\""
        print_info "Using Visual Studio generator"
    else
        CMAKE_GENERATOR="-G \"Unix Makefiles\""
        print_info "Using Unix Makefiles generator"
    fi
}

# Function to check dependencies
check_dependencies() {
    print_info "Checking build dependencies..."
    
    # Check CMake
    if ! command -v cmake >/dev/null 2>&1; then
        print_error "CMake is not installed!"
        print_error "Please install CMake 3.20 or later"
        exit 1
    fi
    
    local cmake_version=$(cmake --version | head -n1 | grep -o '[0-9]\+\.[0-9]\+')
    print_debug "CMake version: $cmake_version"
    
    # Check compiler
    if ! command -v "$CXX" >/dev/null 2>&1; then
        print_error "C++ compiler '$CXX' not found!"
        exit 1
    fi
    
    # Check for optional tools
    if [ "$BUILD_DOCS" = true ] && ! command -v doxygen >/dev/null 2>&1; then
        print_warning "Doxygen not found - documentation will not be generated"
        BUILD_DOCS=false
    fi
    
    if [ "$ENABLE_COVERAGE" = true ] && ! command -v gcov >/dev/null 2>&1; then
        print_warning "gcov not found - coverage analysis disabled"
        ENABLE_COVERAGE=false
    fi
    
    print_success "All required dependencies are available"
}

# Function to configure build
configure_build() {
    print_build "Configuring build..."
    
    local build_dir="$BUILD_DIR/$(echo $BUILD_TYPE | tr '[:upper:]' '[:lower:]')"
    
    # Clean build if requested
    if [ "$CLEAN_BUILD" = true ]; then
        print_info "Cleaning previous build..."
        rm -rf "$build_dir"
    fi
    
    # Create build directory
    mkdir -p "$build_dir"
    cd "$build_dir"
    
    # Prepare CMake arguments
    local cmake_args=(
        "$CMAKE_GENERATOR"
        "-DCMAKE_BUILD_TYPE=$BUILD_TYPE"
        "-DCMAKE_CXX_STANDARD=$CXX_STANDARD"
        "-DCMAKE_CXX_STANDARD_REQUIRED=ON"
        "-DCMAKE_CXX_EXTENSIONS=OFF"
        "-DCMAKE_EXPORT_COMPILE_COMMANDS=ON"
    )
    
    # Add optional features
    if [ "$BUILD_TESTS" = true ]; then
        cmake_args+=("-DBUILD_TESTING=ON")
    else
        cmake_args+=("-DBUILD_TESTING=OFF")
    fi
    
    if [ "$BUILD_EXAMPLES" = true ]; then
        cmake_args+=("-DBUILD_EXAMPLES=ON")
    else
        cmake_args+=("-DBUILD_EXAMPLES=OFF")
    fi
    
    if [ "$BUILD_DOCS" = true ]; then
        cmake_args+=("-DBUILD_DOCUMENTATION=ON")
    else
        cmake_args+=("-DBUILD_DOCUMENTATION=OFF")
    fi
    
    if [ "$ENABLE_SANITIZERS" = true ]; then
        cmake_args+=("-DENABLE_SANITIZERS=ON")
        print_info "Enabling AddressSanitizer and UndefinedBehaviorSanitizer"
    fi
    
    if [ "$ENABLE_COVERAGE" = true ]; then
        cmake_args+=("-DENABLE_COVERAGE=ON")
        print_info "Enabling code coverage analysis"
    fi
    
    if [ "$ENABLE_LTO" = true ] && [ "$BUILD_TYPE" = "Release" ]; then
        cmake_args+=("-DCMAKE_INTERPROCEDURAL_OPTIMIZATION=ON")
        print_info "Enabling Link Time Optimization"
    fi
    
    # Set compiler-specific flags
    if [[ "$CXX" == *"g++"* ]]; then
        cmake_args+=("-DCMAKE_CXX_FLAGS=-Wall -Wextra -Wpedantic")
        if [ "$BUILD_TYPE" = "Release" ]; then
            cmake_args+=("-DCMAKE_CXX_FLAGS_RELEASE=-O3 -DNDEBUG -march=native")
        fi
    elif [[ "$CXX" == *"clang++"* ]]; then
        cmake_args+=("-DCMAKE_CXX_FLAGS=-Wall -Wextra -Wpedantic")
        if [ "$BUILD_TYPE" = "Release" ]; then
            cmake_args+=("-DCMAKE_CXX_FLAGS_RELEASE=-O3 -DNDEBUG -march=native")
        fi
    fi
    
    # Add verbose output if requested
    if [ "$VERBOSE" = true ]; then
        cmake_args+=("-DCMAKE_VERBOSE_MAKEFILE=ON")
    fi
    
    print_debug "CMake configuration: ${cmake_args[*]}"
    
    # Run CMake configuration
    eval cmake "${cmake_args[@]}" "$SOURCE_DIR"
    
    if [ $? -eq 0 ]; then
        print_success "Configuration completed successfully"
    else
        print_error "Configuration failed!"
        exit 1
    fi
}

# Function to build project
build_project() {
    print_build "Building CppVerseHub ($BUILD_TYPE mode)..."
    
    local build_args=()
    
    # Add parallel jobs
    if command -v cmake >/dev/null 2>&1; then
        # Use modern cmake --build syntax
        build_args+=("--build" ".")
        build_args+=("--config" "$BUILD_TYPE")
        build_args+=("--parallel" "$PARALLEL_JOBS")
        
        if [ "$VERBOSE" = true ]; then
            build_args+=("--verbose")
        fi
        
        print_debug "Build command: cmake ${build_args[*]}"
        cmake "${build_args[@]}"
    else
        # Fallback to make
        make -j"$PARALLEL_JOBS"
    fi
    
    if [ $? -eq 0 ]; then
        print_success "Build completed successfully!"
    else
        print_error "Build failed!"
        exit 1
    fi
}

# Function to show build summary
show_build_summary() {
    local build_dir="$BUILD_DIR/$(echo $BUILD_TYPE | tr '[:upper:]' '[:lower:]')"
    
    print_success "Build Summary"
    echo "=============="
    echo "Build Type:      $BUILD_TYPE"
    echo "Compiler:        $CXX"
    echo "C++ Standard:    C++$CXX_STANDARD"
    echo "Parallel Jobs:   $PARALLEL_JOBS"
    echo "Build Directory: $build_dir"
    echo "Tests Enabled:   $BUILD_TESTS"
    echo "Examples:        $BUILD_EXAMPLES"
    echo "Documentation:   $BUILD_DOCS"
    echo "Sanitizers:      $ENABLE_SANITIZERS"
    echo "Coverage:        $ENABLE_COVERAGE"
    echo "LTO:             $ENABLE_LTO"
    echo ""
    
    # Show binary locations
    if [ -f "$build_dir/src/CppVerseHub" ]; then
        echo "Main executable: $build_dir/src/CppVerseHub"
    fi
    
    if [ -f "$build_dir/tests/CppVerseHub_tests" ]; then
        echo "Test executable: $build_dir/tests/CppVerseHub_tests"
    fi
    
    # Show next steps
    echo ""
    echo "Next steps:"
    if [ "$BUILD_TESTS" = true ]; then
        echo "  Run tests: ./scripts/run_tests.sh"
    fi
    echo "  Clean build: ./scripts/clean.sh"
    if [ "$BUILD_DOCS" = true ]; then
        echo "  Generate docs: ./scripts/generate_docs.sh"
    fi
}

# Function to create build performance log
log_build_performance() {
    local build_time="$1"
    local log_file="$PROJECT_ROOT/build_performance.log"
    
    {
        echo "$(date '+%Y-%m-%d %H:%M:%S') - $BUILD_TYPE build completed in ${build_time}s"
        echo "  Compiler: $CXX"
        echo "  Parallel Jobs: $PARALLEL_JOBS"
        echo "  Features: Tests=$BUILD_TESTS, Examples=$BUILD_EXAMPLES, Docs=$BUILD_DOCS"
        echo "  Options: Sanitizers=$ENABLE_SANITIZERS, Coverage=$ENABLE_COVERAGE, LTO=$ENABLE_LTO"
        echo ""
    } >> "$log_file"
}

# Parse command line arguments
parse_arguments() {
    while [[ $# -gt 0 ]]; do
        case $1 in
            -h|--help)
                show_usage
                exit 0
                ;;
            -c|--clean)
                CLEAN_BUILD=true
                shift
                ;;
            -d|--debug)
                BUILD_TYPE="Debug"
                ENABLE_LTO=false
                shift
                ;;
            -r|--release)
                BUILD_TYPE="Release"
                shift
                ;;
            -v|--verbose)
                VERBOSE=true
                shift
                ;;
            -j|--jobs)
                PARALLEL_JOBS="$2"
                if ! [[ "$PARALLEL_JOBS" =~ ^[0-9]+$ ]]; then
                    print_error "Invalid number of jobs: $PARALLEL_JOBS"
                    exit 1
                fi
                shift 2
                ;;
            --no-tests)
                BUILD_TESTS=false
                shift
                ;;
            --no-examples)
                BUILD_EXAMPLES=false
                shift
                ;;
            --docs)
                BUILD_DOCS=true
                shift
                ;;
            --sanitizers)
                ENABLE_SANITIZERS=true
                BUILD_TYPE="Debug"
                ENABLE_LTO=false
                shift
                ;;
            --coverage)
                ENABLE_COVERAGE=true
                BUILD_TYPE="Debug"
                ENABLE_LTO=false
                shift
                ;;
            --no-lto)
                ENABLE_LTO=false
                shift
                ;;
            --std=*)
                CXX_STANDARD="${1#*=}"
                shift
                ;;
            --compiler=*)
                USER_COMPILER="${1#*=}"
                shift
                ;;
            --generator=*)
                USER_GENERATOR="${1#*=}"
                shift
                ;;
            *)
                print_error "Unknown option: $1"
                show_usage
                exit 1
                ;;
        esac
    done
}

# Main build function
main() {
    local start_time=$(date +%s)
    
    print_info "Starting CppVerseHub build process..."
    print_info "Project root: $PROJECT_ROOT"
    
    # Parse command line arguments
    parse_arguments "$@"
    
    # Setup build environment
    detect_compiler "$USER_COMPILER"
    detect_generator "$USER_GENERATOR"
    check_dependencies
    
    # Configure and build
    configure_build
    build_project
    
    local end_time=$(date +%s)
    local build_time=$((end_time - start_time))
    
    # Show results
    show_build_summary
    log_build_performance "$build_time"
    
    print_success "CppVerseHub build completed in ${build_time} seconds!"
    print_info "Happy coding! 🚀"
}

# Ensure we're in the right directory
cd "$PROJECT_ROOT"

# Run main function with all arguments
main "$@"