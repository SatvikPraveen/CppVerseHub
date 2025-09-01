#!/bin/bash
# File: scripts/run_tests.sh
# Test execution script for CppVerseHub
# Supports unit tests, integration tests, benchmarks, and coverage analysis

set -e  # Exit on any error

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
PURPLE='\033[0;35m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

# Test configuration
PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="$PROJECT_ROOT/build"
TEST_RESULTS_DIR="$PROJECT_ROOT/test_results"
BUILD_TYPE="Release"
RUN_UNIT_TESTS=true
RUN_INTEGRATION_TESTS=true
RUN_BENCHMARK_TESTS=false
GENERATE_COVERAGE=false
VERBOSE=false
PARALLEL_TESTS=true
TEST_FILTER=""
OUTPUT_FORMAT="console"
REPEAT_TESTS=1
TIMEOUT=300
GENERATE_REPORT=true
VALGRIND_CHECK=false

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

print_test() {
    echo -e "${PURPLE}[TEST]${NC} $1"
}

print_debug() {
    if [ "$VERBOSE" = true ]; then
        echo -e "${CYAN}[DEBUG]${NC} $1"
    fi
}

# Function to show usage information
show_usage() {
    cat << EOF
CppVerseHub Test Runner
=======================

Usage: $0 [OPTIONS]

Options:
    -h, --help              Show this help message
    -d, --debug             Run tests in Debug build (default: Release)
    -v, --verbose           Enable verbose test output
    --unit-only             Run only unit tests
    --integration-only      Run only integration tests  
    --benchmarks            Run benchmark tests
    --coverage              Generate code coverage report
    --no-parallel           Disable parallel test execution
    --filter=PATTERN        Run tests matching pattern
    --format=FORMAT         Output format (console, xml, json)
    --repeat=N              Repeat tests N times (default: 1)
    --timeout=SECONDS       Test timeout in seconds (default: 300)
    --no-report             Skip generating test report
    --valgrind              Run tests with Valgrind memory checking
    --list-tests            List available tests and exit
    --clean                 Clean test results before running

Test Categories:
    Unit Tests      - Fast, isolated component tests
    Integration     - Multi-component system tests
    Benchmarks      - Performance measurement tests

Output Formats:
    console         - Human-readable console output (default)
    xml             - JUnit XML format for CI integration
    json            - JSON format for programmatic parsing

Examples:
    $0                      # Run all tests in Release mode
    $0 -d --coverage        # Debug tests with coverage analysis
    $0 --benchmarks         # Run performance benchmarks
    $0 --filter="*Core*"    # Run tests matching pattern
    $0 --format=xml         # Generate XML test report
    $0 --valgrind           # Run with memory leak detection

EOF
}

# Function to check test dependencies
check_test_dependencies() {
    print_info "Checking test dependencies..."
    
    # Check if tests were built
    local build_dir="$BUILD_DIR/$(echo $BUILD_TYPE | tr '[:upper:]' '[:lower:]')"
    local test_executable="$build_dir/tests/CppVerseHub_tests"
    
    if [ ! -f "$test_executable" ]; then
        print_error "Test executable not found: $test_executable"
        print_error "Please run './scripts/build.sh' first to build tests"
        exit 1
    fi
    
    # Check for optional tools
    if [ "$GENERATE_COVERAGE" = true ]; then
        if ! command -v gcov >/dev/null 2>&1; then
            print_warning "gcov not found - coverage analysis disabled"
            GENERATE_COVERAGE=false
        fi
        
        if ! command -v lcov >/dev/null 2>&1; then
            print_warning "lcov not found - HTML coverage report disabled"
        fi
    fi
    
    if [ "$VALGRIND_CHECK" = true ] && ! command -v valgrind >/dev/null 2>&1; then
        print_warning "Valgrind not found - memory checking disabled"
        VALGRIND_CHECK=false
    fi
    
    # Create test results directory
    mkdir -p "$TEST_RESULTS_DIR"
    
    print_success "Test dependencies verified"
}

# Function to list available tests
list_tests() {
    print_info "Available tests:"
    
    local build_dir="$BUILD_DIR/$(echo $BUILD_TYPE | tr '[:upper:]' '[:lower:]')"
    local test_executable="$build_dir/tests/CppVerseHub_tests"
    
    if [ -f "$test_executable" ]; then
        echo ""
        echo "Unit Tests:"
        "$test_executable" --list-tests | grep -E "^\s*[a-zA-Z]" | sed 's/^/  /'
        
        echo ""
        echo "Test Tags:"
        "$test_executable" --list-tags | grep -E "^\s*\[" | sed 's/^/  /'
    fi
    
    # List integration tests
    local integration_dir="$build_dir/tests/integration_tests"
    if [ -d "$integration_dir" ]; then
        echo ""
        echo "Integration Tests:"
        find "$integration_dir" -name "*_test" -executable | sed 's|.*/||; s/^/  /'
    fi
    
    # List benchmark tests
    local benchmark_dir="$build_dir/tests/benchmark_tests"
    if [ -d "$benchmark_dir" ]; then
        echo ""
        echo "Benchmark Tests:"
        find "$benchmark_dir" -name "*_benchmark" -executable | sed 's|.*/||; s/^/  /'
    fi
}

# Function to run unit tests
run_unit_tests() {
    if [ "$RUN_UNIT_TESTS" != true ]; then
        return 0
    fi
    
    print_test "Running unit tests..."
    
    local build_dir="$BUILD_DIR/$(echo $BUILD_TYPE | tr '[:upper:]' '[:lower:]')"
    local test_executable="$build_dir/tests/CppVerseHub_tests"
    local test_args=()
    
    # Configure test arguments
    if [ "$VERBOSE" = true ]; then
        test_args+=("--verbose")
    fi
    
    if [ "$PARALLEL_TESTS" = true ]; then
        test_args+=("--parallel")
    fi
    
    if [ -n "$TEST_FILTER" ]; then
        test_args+=("--filter" "$TEST_FILTER")
    fi
    
    # Configure output format
    case "$OUTPUT_FORMAT" in
        "xml")
            test_args+=("--reporter" "junit" "--out" "$TEST_RESULTS_DIR/unit_tests.xml")
            ;;
        "json")
            test_args+=("--reporter" "json" "--out" "$TEST_RESULTS_DIR/unit_tests.json")
            ;;
        *)
            test_args+=("--reporter" "console")
            ;;
    esac
    
    # Run tests with optional memory checking
    local test_command=()
    if [ "$VALGRIND_CHECK" = true ]; then
        test_command+=(
            "valgrind"
            "--leak-check=full"
            "--show-leak-kinds=all"
            "--track-origins=yes"
            "--error-exitcode=1"
            "--log-file=$TEST_RESULTS_DIR/valgrind_unit.log"
        )
    fi
    
    test_command+=("$test_executable")
    test_command+=("${test_args[@]}")
    
    print_debug "Unit test command: ${test_command[*]}"
    
    # Run tests with timeout
    local unit_start_time=$(date +%s)
    
    if timeout "$TIMEOUT" "${test_command[@]}"; then
        local unit_end_time=$(date +%s)
        local unit_duration=$((unit_end_time - unit_start_time))
        print_success "Unit tests passed in ${unit_duration}s"
        return 0
    else
        local exit_code=$?
        print_error "Unit tests failed with exit code: $exit_code"
        return $exit_code
    fi
}

# Function to run integration tests
run_integration_tests() {
    if [ "$RUN_INTEGRATION_TESTS" != true ]; then
        return 0
    fi
    
    print_test "Running integration tests..."
    
    local build_dir="$BUILD_DIR/$(echo $BUILD_TYPE | tr '[:upper:]' '[:lower:]')"
    local integration_dir="$build_dir/tests/integration_tests"
    local integration_results=0
    
    if [ ! -d "$integration_dir" ]; then
        print_warning "Integration tests directory not found, skipping..."
        return 0
    fi
    
    local integration_start_time=$(date +%s)
    
    # Find and run integration test executables
    while IFS= read -r -d '' test_exe; do
        local test_name=$(basename "$test_exe")
        print_info "Running integration test: $test_name"
        
        local test_command=()
        if [ "$VALGRIND_CHECK" = true ]; then
            test_command+=(
                "valgrind"
                "--leak-check=full"
                "--error-exitcode=1"
                "--log-file=$TEST_RESULTS_DIR/valgrind_${test_name}.log"
            )
        fi
        
        test_command+=("$test_exe")
        
        if timeout "$TIMEOUT" "${test_command[@]}"; then
            print_success "Integration test '$test_name' passed"
        else
            print_error "Integration test '$test_name' failed"
            integration_results=1
        fi
    done < <(find "$integration_dir" -name "*_test" -executable -print0)
    
    local integration_end_time=$(date +%s)
    local integration_duration=$((integration_end_time - integration_start_time))
    
    if [ $integration_results -eq 0 ]; then
        print_success "All integration tests passed in ${integration_duration}s"
    else
        print_error "Some integration tests failed"
    fi
    
    return $integration_results
}

# Function to run benchmark tests
run_benchmark_tests() {
    if [ "$RUN_BENCHMARK_TESTS" != true ]; then
        return 0
    fi
    
    print_test "Running benchmark tests..."
    
    local build_dir="$BUILD_DIR/$(echo $BUILD_TYPE | tr '[:upper:]' '[:lower:]')"
    local benchmark_dir="$build_dir/tests/benchmark_tests"
    
    if [ ! -d "$benchmark_dir" ]; then
        print_warning "Benchmark tests directory not found, skipping..."
        return 0
    fi
    
    local benchmark_start_time=$(date +%s)
    local benchmark_results=0
    
    # Find and run benchmark executables
    while IFS= read -r -d '' benchmark_exe; do
        local benchmark_name=$(basename "$benchmark_exe")
        print_info "Running benchmark: $benchmark_name"
        
        local benchmark_output="$TEST_RESULTS_DIR/${benchmark_name}_results.json"
        
        if timeout $((TIMEOUT * 2)) "$benchmark_exe" --benchmark_format=json --benchmark_out="$benchmark_output"; then
            print_success "Benchmark '$benchmark_name' completed"
        else
            print_error "Benchmark '$benchmark_name' failed"
            benchmark_results=1
        fi
    done < <(find "$benchmark_dir" -name "*_benchmark" -executable -print0)
    
    local benchmark_end_time=$(date +%s)
    local benchmark_duration=$((benchmark_end_time - benchmark_start_time))
    
    if [ $benchmark_results -eq 0 ]; then
        print_success "All benchmarks completed in ${benchmark_duration}s"
    else
        print_error "Some benchmarks failed"
    fi
    
    return $benchmark_results
}

# Function to generate coverage report
generate_coverage_report() {
    if [ "$GENERATE_COVERAGE" != true ]; then
        return 0
    fi
    
    print_test "Generating code coverage report..."
    
    local build_dir="$BUILD_DIR/$(echo $BUILD_TYPE | tr '[:upper:]' '[:lower:]')"
    local coverage_dir="$TEST_RESULTS_DIR/coverage"
    
    mkdir -p "$coverage_dir"
    
    # Collect coverage data
    if command -v lcov >/dev/null 2>&1; then
        print_info "Collecting coverage data with lcov..."
        
        # Reset coverage counters
        lcov --directory "$build_dir" --zerocounters
        
        # Capture initial state
        lcov --capture --initial --directory "$build_dir" --output-file "$coverage_dir/coverage_base.info"
        
        # Capture coverage data after test run
        lcov --capture --directory "$build_dir" --output-file "$coverage_dir/coverage_test.info"
        
        # Combine coverage data
        lcov --add-tracefile "$coverage_dir/coverage_base.info" \
             --add-tracefile "$coverage_dir/coverage_test.info" \
             --output-file "$coverage_dir/coverage_total.info"
        
        # Filter out system headers and test files
        lcov --remove "$coverage_dir/coverage_total.info" \
             '/usr/*' '/opt/*' '*/external/*' '*/tests/*' \
             --output-file "$coverage_dir/coverage_filtered.info"
        
        # Generate HTML report
        if command -v genhtml >/dev/null 2>&1; then
            genhtml "$coverage_dir/coverage_filtered.info" \
                    --output-directory "$coverage_dir/html" \
                    --title "CppVerseHub Coverage Report" \
                    --show-details --legend
            
            print_success "HTML coverage report generated: $coverage_dir/html/index.html"
        fi
        
        # Generate summary
        lcov --summary "$coverage_dir/coverage_filtered.info" > "$coverage_dir/coverage_summary.txt"
        
    else
        # Fallback to gcov
        print_info "Collecting coverage data with gcov..."
        
        cd "$build_dir"
        find . -name "*.gcno" -exec gcov {} \; > "$coverage_dir/gcov_output.txt"
        
        # Move coverage files
        find . -name "*.gcov" -exec mv {} "$coverage_dir/" \;
    fi
    
    print_success "Coverage report generated in $coverage_dir"
}

# Function to generate test report
generate_test_report() {
    if [ "$GENERATE_REPORT" != true ]; then
        return 0
    fi
    
    print_test "Generating comprehensive test report..."
    
    local report_file="$TEST_RESULTS_DIR/test_report.html"
    local timestamp=$(date "+%Y-%m-%d %H:%M:%S")
    
    cat > "$report_file" << EOF
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>CppVerseHub Test Report</title>
    <style>
        body { font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif; margin: 40px; background: #f5f5f5; }
        .container { background: white; padding: 30px; border-radius: 8px; box-shadow: 0 2px 10px rgba(0,0,0,0.1); }
        .header { border-bottom: 2px solid #007acc; padding-bottom: 20px; margin-bottom: 30px; }
        h1 { color: #007acc; margin: 0; }
        .timestamp { color: #666; font-size: 14px; }
        .section { margin: 20px 0; }
        .success { color: #28a745; font-weight: bold; }
        .error { color: #dc3545; font-weight: bold; }
        .warning { color: #ffc107; font-weight: bold; }
        .metric { display: inline-block; background: #e9ecef; padding: 10px 15px; margin: 5px; border-radius: 4px; }
        pre { background: #f8f9fa; padding: 15px; border-radius: 4px; overflow-x: auto; }
        .file-link { color: #007acc; text-decoration: none; }
        .file-link:hover { text-decoration: underline; }
    </style>
</head>
<body>
    <div class="container">
        <div class="header">
            <h1>🧪 CppVerseHub Test Report</h1>
            <div class="timestamp">Generated: $timestamp</div>
        </div>
        
        <div class="section">
            <h2>Test Configuration</h2>
            <div class="metric">Build Type: $BUILD_TYPE</div>
            <div class="metric">Unit Tests: $RUN_UNIT_TESTS</div>
            <div class="metric">Integration Tests: $RUN_INTEGRATION_TESTS</div>
            <div class="metric">Benchmarks: $RUN_BENCHMARK_TESTS</div>
            <div class="metric">Coverage: $GENERATE_COVERAGE</div>
            <div class="metric">Valgrind: $VALGRIND_CHECK</div>
        </div>
EOF
    
    # Add test results section
    echo "        <div class=\"section\">" >> "$report_file"
    echo "            <h2>Test Results</h2>" >> "$report_file"
    
    # Check for unit test results
    if [ -f "$TEST_RESULTS_DIR/unit_tests.xml" ]; then
        echo "            <p><a href=\"unit_tests.xml\" class=\"file-link\">Unit Test Results (XML)</a></p>" >> "$report_file"
    fi
    
    if [ -f "$TEST_RESULTS_DIR/unit_tests.json" ]; then
        echo "            <p><a href=\"unit_tests.json\" class=\"file-link\">Unit Test Results (JSON)</a></p>" >> "$report_file"
    fi
    
    # Add coverage results
    if [ -d "$TEST_RESULTS_DIR/coverage/html" ]; then
        echo "            <p><a href=\"coverage/html/index.html\" class=\"file-link\">Coverage Report</a></p>" >> "$report_file"
    fi
    
    # Add benchmark results
    local benchmark_files=$(find "$TEST_RESULTS_DIR" -name "*_benchmark_results.json" 2>/dev/null)
    if [ -n "$benchmark_files" ]; then
        echo "            <h3>Benchmark Results</h3>" >> "$report_file"
        echo "            <ul>" >> "$report_file"
        for file in $benchmark_files; do
            local basename=$(basename "$file")
            echo "                <li><a href=\"$basename\" class=\"file-link\">$basename</a></li>" >> "$report_file"
        done
        echo "            </ul>" >> "$report_file"
    fi
    
    echo "        </div>" >> "$report_file"
    
    # Add Valgrind results if available
    local valgrind_files=$(find "$TEST_RESULTS_DIR" -name "valgrind_*.log" 2>/dev/null)
    if [ -n "$valgrind_files" ]; then
        echo "        <div class=\"section\">" >> "$report_file"
        echo "            <h2>Memory Check Results</h2>" >> "$report_file"
        echo "            <ul>" >> "$report_file"
        for file in $valgrind_files; do
            local basename=$(basename "$file")
            echo "                <li><a href=\"$basename\" class=\"file-link\">$basename</a></li>" >> "$report_file"
        done
        echo "            </ul>" >> "$report_file"
        echo "        </div>" >> "$report_file"
    fi
    
    cat >> "$report_file" << EOF
    </div>
</body>
</html>
EOF
    
    print_success "Test report generated: $report_file"
}

# Function to clean test results
clean_test_results() {
    print_info "Cleaning previous test results..."
    rm -rf "$TEST_RESULTS_DIR"
    mkdir -p "$TEST_RESULTS_DIR"
    print_success "Test results cleaned"
}

# Function to show test summary
show_test_summary() {
    local total_time="$1"
    local test_status="$2"
    
    echo ""
    print_success "Test Execution Summary"
    echo "======================="
    echo "Build Type:           $BUILD_TYPE"
    echo "Total Execution Time: ${total_time}s"
    echo "Unit Tests:           $RUN_UNIT_TESTS"
    echo "Integration Tests:    $RUN_INTEGRATION_TESTS"  
    echo "Benchmark Tests:      $RUN_BENCHMARK_TESTS"
    echo "Coverage Generated:   $GENERATE_COVERAGE"
    echo "Memory Checking:      $VALGRIND_CHECK"
    echo "Overall Status:       $test_status"
    echo ""
    
    # Show results location
    echo "Results Location: $TEST_RESULTS_DIR"
    if [ -f "$TEST_RESULTS_DIR/test_report.html" ]; then
        echo "Test Report:      $TEST_RESULTS_DIR/test_report.html"
    fi
    
    if [ -d "$TEST_RESULTS_DIR/coverage/html" ]; then
        echo "Coverage Report:  $TEST_RESULTS_DIR/coverage/html/index.html"
    fi
    
    echo ""
    echo "Next steps:"
    echo "  View detailed results in the test report"
    if [ "$GENERATE_COVERAGE" = true ]; then
        echo "  Analyze code coverage to identify untested areas"
    fi
    echo "  Run './scripts/build.sh' to rebuild after fixes"
}

# Parse command line arguments
parse_arguments() {
    while [[ $# -gt 0 ]]; do
        case $1 in
            -h|--help)
                show_usage
                exit 0
                ;;
            -d|--debug)
                BUILD_TYPE="Debug"
                shift
                ;;
            -v|--verbose)
                VERBOSE=true
                shift
                ;;
            --unit-only)
                RUN_UNIT_TESTS=true
                RUN_INTEGRATION_TESTS=false
                RUN_BENCHMARK_TESTS=false
                shift
                ;;
            --integration-only)
                RUN_UNIT_TESTS=false
                RUN_INTEGRATION_TESTS=true
                RUN_BENCHMARK_TESTS=false
                shift
                ;;
            --benchmarks)
                RUN_BENCHMARK_TESTS=true
                shift
                ;;
            --coverage)
                GENERATE_COVERAGE=true
                BUILD_TYPE="Debug"
                shift
                ;;
            --no-parallel)
                PARALLEL_TESTS=false
                shift
                ;;
            --filter=*)
                TEST_FILTER="${1#*=}"
                shift
                ;;
            --format=*)
                OUTPUT_FORMAT="${1#*=}"
                shift
                ;;
            --repeat=*)
                REPEAT_TESTS="${1#*=}"
                shift
                ;;
            --timeout=*)
                TIMEOUT="${1#*=}"
                shift
                ;;
            --no-report)
                GENERATE_REPORT=false
                shift
                ;;
            --valgrind)
                VALGRIND_CHECK=true
                shift
                ;;
            --list-tests)
                list_tests
                exit 0
                ;;
            --clean)
                clean_test_results
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

# Main test execution function
main() {
    local start_time=$(date +%s)
    
    print_info "Starting CppVerseHub test execution..."
    print_info "Project root: $PROJECT_ROOT"
    
    # Parse command line arguments
    parse_arguments "$@"
    
    # Check dependencies and setup
    check_test_dependencies
    
    # Execute tests multiple times if requested
    local overall_result=0
    
    for ((i=1; i<=REPEAT_TESTS; i++)); do
        if [ $REPEAT_TESTS -gt 1 ]; then
            print_info "Test run $i of $REPEAT_TESTS"
        fi
        
        # Run unit tests
        if ! run_unit_tests; then
            overall_result=1
        fi
        
        # Run integration tests
        if ! run_integration_tests; then
            overall_result=1
        fi
        
        # Run benchmark tests
        if ! run_benchmark_tests; then
            overall_result=1
        fi
        
        if [ $overall_result -ne 0 ] && [ $REPEAT_TESTS -gt 1 ]; then
            print_warning "Test run $i failed, continuing with remaining runs..."
        fi
    done
    
    # Generate coverage report if requested
    generate_coverage_report
    
    # Generate comprehensive test report
    generate_test_report
    
    local end_time=$(date +%s)
    local total_time=$((end_time - start_time))
    
    # Show summary
    local test_status
    if [ $overall_result -eq 0 ]; then
        test_status="PASSED"
        print_success "All tests passed! 🎉"
    else
        test_status="FAILED"
        print_error "Some tests failed! ❌"
    fi
    
    show_test_summary "$total_time" "$test_status"
    
    # Return appropriate exit code
    exit $overall_result
}

# Ensure we're in the right directory
cd "$PROJECT_ROOT"

# Run main function with all arguments
main "$@"