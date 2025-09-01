#!/bin/bash
# File: tools/static_analysis/run_analysis.sh
# Static analysis runner for CppVerseHub

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
BLUE='\033[0;34m'
PURPLE='\033[0;35m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

# Configuration
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/../.." && pwd)"
BUILD_DIR="${PROJECT_ROOT}/build/debug"
SRC_DIR="${PROJECT_ROOT}/src"
CLANG_TIDY_CONFIG="${SCRIPT_DIR}/.clang-tidy"
OUTPUT_DIR="${PROJECT_ROOT}/build/analysis"
COMPILATION_DATABASE="${BUILD_DIR}/compile_commands.json"

# Default values
VERBOSE=false
PARALLEL_JOBS=$(nproc)
FORMAT_ONLY=false
TIDY_ONLY=false
CPPCHECK_ONLY=false
INCLUDE_TESTS=false
EXCLUDE_EXTERNAL=true
FAIL_ON_ERRORS=true
OUTPUT_FORMAT="text"

# Function to print usage
print_usage() {
    echo -e "${CYAN}Usage: $0 [OPTIONS]${NC}"
    echo ""
    echo "Static analysis runner for CppVerseHub"
    echo ""
    echo "Options:"
    echo "  -v, --verbose           Enable verbose output"
    echo "  -j, --jobs N            Number of parallel jobs (default: $(nproc))"
    echo "  -f, --format-only       Run only clang-format checks"
    echo "  -t, --tidy-only         Run only clang-tidy analysis"
    echo "  -c, --cppcheck-only     Run only cppcheck analysis"
    echo "  --include-tests         Include test files in analysis"
    echo "  --include-external      Include external dependencies"
    echo "  --no-fail               Don't fail on errors/warnings"
    echo "  --output-format FORMAT  Output format: text, json, xml (default: text)"
    echo "  --build-dir DIR         Build directory (default: build/debug)"
    echo "  -h, --help              Show this help message"
    echo ""
    echo "Examples:"
    echo "  $0                      # Run all analysis tools"
    echo "  $0 -t -j 8             # Run only clang-tidy with 8 jobs"
    echo "  $0 -f                   # Check only formatting"
    echo "  $0 --include-tests      # Include test files in analysis"
}

# Function to log with timestamp
log() {
    echo -e "[$(date +'%H:%M:%S')] $*"
}

# Function to log info
log_info() {
    log "${BLUE}INFO:${NC} $*"
}

# Function to log warning
log_warning() {
    log "${YELLOW}WARNING:${NC} $*"
}

# Function to log error
log_error() {
    log "${RED}ERROR:${NC} $*"
}

# Function to log success
log_success() {
    log "${GREEN}SUCCESS:${NC} $*"
}

# Function to check if command exists
command_exists() {
    command -v "$1" >/dev/null 2>&1
}

# Function to find source files
find_source_files() {
    local files=()
    local extensions=("*.cpp" "*.cxx" "*.cc" "*.c++" "*.hpp" "*.hxx" "*.hh" "*.h++")
    
    for ext in "${extensions[@]}"; do
        while IFS= read -r -d '' file; do
            # Exclude external dependencies unless requested
            if [[ "$EXCLUDE_EXTERNAL" == "true" && "$file" =~ external/ ]]; then
                continue
            fi
            
            # Exclude tests unless requested
            if [[ "$INCLUDE_TESTS" == "false" && "$file" =~ tests/ ]]; then
                continue
            fi
            
            files+=("$file")
        done < <(find "$SRC_DIR" -name "$ext" -type f -print0 2>/dev/null)
        
        # Include tests if requested
        if [[ "$INCLUDE_TESTS" == "true" ]]; then
            while IFS= read -r -d '' file; do
                if [[ "$EXCLUDE_EXTERNAL" == "true" && "$file" =~ external/ ]]; then
                    continue
                fi
                files+=("$file")
            done < <(find "${PROJECT_ROOT}/tests" -name "$ext" -type f -print0 2>/dev/null)
        fi
    done
    
    printf '%s\n' "${files[@]}" | sort -u
}

# Function to run clang-format check
run_clang_format() {
    log_info "Running clang-format checks..."
    
    if ! command_exists clang-format; then
        log_error "clang-format not found. Please install clang-format."
        return 1
    fi
    
    local format_config="${PROJECT_ROOT}/tools/clang-format/.clang-format"
    if [[ ! -f "$format_config" ]]; then
        log_error "clang-format configuration not found: $format_config"
        return 1
    fi
    
    local files
    mapfile -t files < <(find_source_files)
    
    if [[ ${#files[@]} -eq 0 ]]; then
        log_warning "No source files found for formatting check."
        return 0
    fi
    
    local format_errors=0
    local output_file="${OUTPUT_DIR}/clang-format-report.txt"
    mkdir -p "$OUTPUT_DIR"
    
    echo "Clang-Format Report - $(date)" > "$output_file"
    echo "=========================" >> "$output_file"
    echo "" >> "$output_file"
    
    for file in "${files[@]}"; do
        if [[ "$VERBOSE" == "true" ]]; then
            log_info "Checking format: $file"
        fi
        
        if ! clang-format --style=file:"$format_config" --dry-run --Werror "$file" 2>/dev/null; then
            echo "FORMAT ISSUE: $file" >> "$output_file"
            ((format_errors++))
        fi
    done
    
    if [[ $format_errors -gt 0 ]]; then
        log_error "Found $format_errors files with formatting issues."
        log_info "Run 'clang-format -i <file>' to fix formatting."
        log_info "Report saved to: $output_file"
        return 1
    else
        log_success "All files are properly formatted."
        echo "All files properly formatted!" >> "$output_file"
    fi
    
    return 0
}

# Function to run clang-tidy
run_clang_tidy() {
    log_info "Running clang-tidy analysis..."
    
    if ! command_exists clang-tidy; then
        log_error "clang-tidy not found. Please install clang-tidy."
        return 1
    fi
    
    if [[ ! -f "$COMPILATION_DATABASE" ]]; then
        log_error "Compilation database not found: $COMPILATION_DATABASE"
        log_info "Please build the project first: cmake --build $BUILD_DIR"
        return 1
    fi
    
    if [[ ! -f "$CLANG_TIDY_CONFIG" ]]; then
        log_error "clang-tidy configuration not found: $CLANG_TIDY_CONFIG"
        return 1
    fi
    
    local files
    mapfile -t files < <(find_source_files | grep -E '\.(cpp|cxx|cc|c\+\+)$')
    
    if [[ ${#files[@]} -eq 0 ]]; then
        log_warning "No C++ source files found for analysis."
        return 0
    fi
    
    local output_file="${OUTPUT_DIR}/clang-tidy-report.txt"
    local json_output="${OUTPUT_DIR}/clang-tidy-report.json"
    mkdir -p "$OUTPUT_DIR"
    
    # Prepare output files
    echo "Clang-Tidy Analysis Report - $(date)" > "$output_file"
    echo "=================================" >> "$output_file"
    echo "" >> "$output_file"
    
    if [[ "$OUTPUT_FORMAT" == "json" ]]; then
        echo "[]" > "$json_output"
    fi
    
    log_info "Analyzing ${#files[@]} files with $PARALLEL_JOBS parallel jobs..."
    
    # Run clang-tidy with parallel processing
    local tidy_errors=0
    local temp_dir
    temp_dir=$(mktemp -d)
    
    # Function to analyze single file
    analyze_file() {
        local file=$1
        local index=$2
        local temp_file="${temp_dir}/result_${index}.txt"
        
        if [[ "$VERBOSE" == "true" ]]; then
            echo "Analyzing: $file" >&2
        fi
        
        # Run clang-tidy
        local result
        result=$(clang-tidy \
            --config-file="$CLANG_TIDY_CONFIG" \
            --format-style=file:"${PROJECT_ROOT}/tools/clang-format/.clang-format" \
            -p="$BUILD_DIR" \
            "$file" 2>&1 || echo "ANALYSIS_FAILED")
        
        if [[ "$result" == "ANALYSIS_FAILED" ]]; then
            echo "FAILED:$file" > "$temp_file"
        elif [[ -n "$result" ]]; then
            echo "ISSUES:$file" > "$temp_file"
            echo "$result" >> "$temp_file"
        else
            echo "CLEAN:$file" > "$temp_file"
        fi
    }
    
    export -f analyze_file
    export temp_dir VERBOSE CLANG_TIDY_CONFIG BUILD_DIR PROJECT_ROOT
    
    # Run analysis in parallel
    printf '%s\n' "${files[@]}" | nl -nln | head -n "${#files[@]}" | \
        xargs -n 2 -P "$PARALLEL_JOBS" bash -c 'analyze_file "$2" "$1"'
    
    # Collect results
    for i in $(seq 0 $((${#files[@]} - 1))); do
        local temp_file="${temp_dir}/result_${i}.txt"
        if [[ -f "$temp_file" ]]; then
            local status
            status=$(head -n 1 "$temp_file" | cut -d: -f1)
            local file
            file=$(head -n 1 "$temp_file" | cut -d: -f2-)
            
            case "$status" in
                "FAILED")
                    echo "ANALYSIS FAILED: $file" >> "$output_file"
                    ((tidy_errors++))
                    ;;
                "ISSUES")
                    echo "ISSUES FOUND: $file" >> "$output_file"
                    tail -n +2 "$temp_file" >> "$output_file"
                    echo "" >> "$output_file"
                    ((tidy_errors++))
                    ;;
                "CLEAN")
                    if [[ "$VERBOSE" == "true" ]]; then
                        echo "CLEAN: $file" >> "$output_file"
                    fi
                    ;;
            esac
        fi
    done
    
    # Cleanup
    rm -rf "$temp_dir"
    
    if [[ $tidy_errors -gt 0 ]]; then
        log_error "Found issues in $tidy_errors files."
        log_info "Report saved to: $output_file"
        return 1
    else
        log_success "No clang-tidy issues found."
        echo "No issues found!" >> "$output_file"
    fi
    
    return 0
}

# Function to run cppcheck
run_cppcheck() {
    log_info "Running cppcheck analysis..."
    
    if ! command_exists cppcheck; then
        log_warning "cppcheck not found. Skipping cppcheck analysis."
        log_info "Install cppcheck for additional static analysis."
        return 0
    fi
    
    local files
    mapfile -t files < <(find_source_files | grep -E '\.(cpp|cxx|cc|c\+\+|h|hpp|hxx|h\+\+)$')
    
    if [[ ${#files[@]} -eq 0 ]]; then
        log_warning "No source files found for cppcheck analysis."
        return 0
    fi
    
    local output_file="${OUTPUT_DIR}/cppcheck-report.txt"
    local xml_output="${OUTPUT_DIR}/cppcheck-report.xml"
    mkdir -p "$OUTPUT_DIR"
    
    log_info "Running cppcheck on ${#files[@]} files..."
    
    # Prepare cppcheck arguments
    local cppcheck_args=(
        "--enable=all"
        "--inconclusive"
        "--std=c++20"
        "--platform=unix64"
        "--check-config"
        "--suppress=missingIncludeSystem"
        "--suppress=unusedFunction"  # Often false positives in template-heavy code
        "--suppress=constParameter"  # Can be noisy for interfaces
        "-j$PARALLEL_JOBS"
        "--force"
    )
    
    if [[ "$EXCLUDE_EXTERNAL" == "true" ]]; then
        cppcheck_args+=("-i${PROJECT_ROOT}/external/")
    fi
    
    if [[ "$OUTPUT_FORMAT" == "xml" ]]; then
        cppcheck_args+=("--xml" "--xml-version=2")
    fi
    
    # Add include directories
    cppcheck_args+=(
        "-I${SRC_DIR}"
        "-I${SRC_DIR}/core"
        "-I${SRC_DIR}/templates"
        "-I${SRC_DIR}/patterns"
        "-I${SRC_DIR}/utils"
    )
    
    # Run cppcheck
    local cppcheck_output
    if [[ "$OUTPUT_FORMAT" == "xml" ]]; then
        cppcheck_output=$(cppcheck "${cppcheck_args[@]}" "${files[@]}" 2>&1)
        echo "$cppcheck_output" > "$xml_output"
        
        # Also create text report from XML
        echo "Cppcheck Analysis Report - $(date)" > "$output_file"
        echo "=============================" >> "$output_file"
        echo "" >> "$output_file"
        
        if command_exists xmllint; then
            xmllint --format "$xml_output" | grep -E "(error|warning|performance|portability)" >> "$output_file" || true
        else
            grep -E "(error|warning|performance|portability)" "$xml_output" >> "$output_file" || true
        fi
    else
        cppcheck_output=$(cppcheck "${cppcheck_args[@]}" "${files[@]}" 2>&1)
        echo "Cppcheck Analysis Report - $(date)" > "$output_file"
        echo "=============================" >> "$output_file"
        echo "" >> "$output_file"
        echo "$cppcheck_output" >> "$output_file"
    fi
    
    # Count issues
    local error_count
    error_count=$(echo "$cppcheck_output" | grep -c "(error)" || echo "0")
    local warning_count
    warning_count=$(echo "$cppcheck_output" | grep -c "(warning)" || echo "0")
    local performance_count
    performance_count=$(echo "$cppcheck_output" | grep -c "(performance)" || echo "0")
    local portability_count
    portability_count=$(echo "$cppcheck_output" | grep -c "(portability)" || echo "0")
    
    local total_issues=$((error_count + warning_count + performance_count + portability_count))
    
    if [[ $total_issues -gt 0 ]]; then
        log_warning "Cppcheck found $total_issues issues:"
        log_warning "  Errors: $error_count"
        log_warning "  Warnings: $warning_count"
        log_warning "  Performance: $performance_count"
        log_warning "  Portability: $portability_count"
        log_info "Report saved to: $output_file"
        
        if [[ "$OUTPUT_FORMAT" == "xml" ]]; then
            log_info "XML report saved to: $xml_output"
        fi
        
        # Only fail on errors, not warnings
        if [[ $error_count -gt 0 ]]; then
            return 1
        fi
    else
        log_success "No cppcheck issues found."
        echo "No issues found!" >> "$output_file"
    fi
    
    return 0
}

# Function to generate summary report
generate_summary() {
    log_info "Generating analysis summary..."
    
    local summary_file="${OUTPUT_DIR}/analysis-summary.txt"
    mkdir -p "$OUTPUT_DIR"
    
    {
        echo "CppVerseHub Static Analysis Summary"
        echo "=================================="
        echo "Generated: $(date)"
        echo ""
        
        echo "Configuration:"
        echo "  Project Root: $PROJECT_ROOT"
        echo "  Build Directory: $BUILD_DIR"
        echo "  Parallel Jobs: $PARALLEL_JOBS"
        echo "  Include Tests: $INCLUDE_TESTS"
        echo "  Exclude External: $EXCLUDE_EXTERNAL"
        echo ""
        
        echo "Tools Run:"
        if [[ "$TIDY_ONLY" == "false" && "$CPPCHECK_ONLY" == "false" ]] || [[ "$FORMAT_ONLY" == "true" ]]; then
            echo "  ✓ clang-format"
        fi
        if [[ "$FORMAT_ONLY" == "false" && "$CPPCHECK_ONLY" == "false" ]] || [[ "$TIDY_ONLY" == "true" ]]; then
            echo "  ✓ clang-tidy"
        fi
        if [[ "$FORMAT_ONLY" == "false" && "$TIDY_ONLY" == "false" ]] || [[ "$CPPCHECK_ONLY" == "true" ]]; then
            echo "  ✓ cppcheck"
        fi
        echo ""
        
        # Include individual report summaries
        for report in "${OUTPUT_DIR}"/*-report.txt; do
            if [[ -f "$report" ]]; then
                local tool_name
                tool_name=$(basename "$report" -report.txt)
                echo "${tool_name^} Results:"
                echo "  Report: $report"
                
                # Extract key metrics from reports
                if [[ -f "$report" ]]; then
                    local issue_count
                    issue_count=$(grep -c -E "(error|warning|ISSUE|FAILED)" "$report" 2>/dev/null || echo "0")
                    echo "  Issues Found: $issue_count"
                fi
                echo ""
            fi
        done
        
        echo "All reports saved to: $OUTPUT_DIR"
        
    } > "$summary_file"
    
    log_success "Analysis summary saved to: $summary_file"
}

# Function to setup build environment
setup_build_env() {
    log_info "Setting up build environment..."
    
    if [[ ! -d "$BUILD_DIR" ]]; then
        log_info "Creating build directory: $BUILD_DIR"
        mkdir -p "$BUILD_DIR"
    fi
    
    # Check if we need to run cmake
    if [[ ! -f "$BUILD_DIR/CMakeCache.txt" ]]; then
        log_info "Running initial cmake configuration..."
        cd "$BUILD_DIR"
        cmake -DCMAKE_BUILD_TYPE=Debug \
              -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
              -DBUILD_TESTING=ON \
              "$PROJECT_ROOT"
        cd - >/dev/null
    fi
    
    # Check if compilation database exists and is recent
    if [[ ! -f "$COMPILATION_DATABASE" ]] || [[ "$BUILD_DIR/CMakeCache.txt" -nt "$COMPILATION_DATABASE" ]]; then
        log_info "Updating compilation database..."
        cmake --build "$BUILD_DIR" --target all -- -j"$PARALLEL_JOBS"
    fi
}

# Function to clean old reports
clean_reports() {
    if [[ -d "$OUTPUT_DIR" ]]; then
        log_info "Cleaning old analysis reports..."
        rm -rf "${OUTPUT_DIR:?}"/*
    fi
    mkdir -p "$OUTPUT_DIR"
}

# Main execution function
main() {
    local start_time
    start_time=$(date +%s)
    
    log_info "Starting CppVerseHub static analysis..."
    log_info "Project root: $PROJECT_ROOT"
    log_info "Build directory: $BUILD_DIR"
    
    # Clean old reports
    clean_reports
    
    # Setup build environment if needed (only for clang-tidy)
    if [[ "$FORMAT_ONLY" == "false" && "$CPPCHECK_ONLY" == "false" ]] || [[ "$TIDY_ONLY" == "true" ]]; then
        setup_build_env
    fi
    
    local exit_code=0
    
    # Run analysis tools
    if [[ "$TIDY_ONLY" == "false" && "$CPPCHECK_ONLY" == "false" ]] || [[ "$FORMAT_ONLY" == "true" ]]; then
        if ! run_clang_format; then
            exit_code=1
        fi
    fi
    
    if [[ "$FORMAT_ONLY" == "false" && "$CPPCHECK_ONLY" == "false" ]] || [[ "$TIDY_ONLY" == "true" ]]; then
        if ! run_clang_tidy; then
            exit_code=1
        fi
    fi
    
    if [[ "$FORMAT_ONLY" == "false" && "$TIDY_ONLY" == "false" ]] || [[ "$CPPCHECK_ONLY" == "true" ]]; then
        if ! run_cppcheck; then
            exit_code=1
        fi
    fi
    
    # Generate summary
    generate_summary
    
    local end_time
    end_time=$(date +%s)
    local duration=$((end_time - start_time))
    
    log_info "Analysis completed in ${duration}s"
    
    if [[ $exit_code -eq 0 ]]; then
        log_success "All static analysis checks passed!"
    else
        log_error "Some static analysis checks failed."
        if [[ "$FAIL_ON_ERRORS" == "false" ]]; then
            log_warning "Ignoring failures due to --no-fail flag."
            exit_code=0
        fi
    fi
    
    return $exit_code
}

# Parse command line arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        -v|--verbose)
            VERBOSE=true
            shift
            ;;
        -j|--jobs)
            if [[ -n "$2" && "$2" =~ ^[0-9]+$ ]]; then
                PARALLEL_JOBS="$2"
                shift 2
            else
                log_error "Invalid number of jobs: $2"
                exit 1
            fi
            ;;
        -f|--format-only)
            FORMAT_ONLY=true
            shift
            ;;
        -t|--tidy-only)
            TIDY_ONLY=true
            shift
            ;;
        -c|--cppcheck-only)
            CPPCHECK_ONLY=true
            shift
            ;;
        --include-tests)
            INCLUDE_TESTS=true
            shift
            ;;
        --include-external)
            EXCLUDE_EXTERNAL=false
            shift
            ;;
        --no-fail)
            FAIL_ON_ERRORS=false
            shift
            ;;
        --output-format)
            if [[ -n "$2" && "$2" =~ ^(text|json|xml)$ ]]; then
                OUTPUT_FORMAT="$2"
                shift 2
            else
                log_error "Invalid output format: $2. Use: text, json, xml"
                exit 1
            fi
            ;;
        --build-dir)
            if [[ -n "$2" ]]; then
                BUILD_DIR="$2"
                COMPILATION_DATABASE="${BUILD_DIR}/compile_commands.json"
                shift 2
            else
                log_error "Build directory not specified"
                exit 1
            fi
            ;;
        -h|--help)
            print_usage
            exit 0
            ;;
        *)
            log_error "Unknown option: $1"
            print_usage
            exit 1
            ;;
    esac
done

# Validate exclusive options
if [[ "$FORMAT_ONLY" == "true" && "$TIDY_ONLY" == "true" ]]; then
    log_error "Cannot specify both --format-only and --tidy-only"
    exit 1
fi

if [[ "$FORMAT_ONLY" == "true" && "$CPPCHECK_ONLY" == "true" ]]; then
    log_error "Cannot specify both --format-only and --cppcheck-only"
    exit 1
fi

if [[ "$TIDY_ONLY" == "true" && "$CPPCHECK_ONLY" == "true" ]]; then
    log_error "Cannot specify both --tidy-only and --cppcheck-only"
    exit 1
fi

# Run main function
if ! main; then
    exit 1
fi

log_success "Static analysis completed successfully!"