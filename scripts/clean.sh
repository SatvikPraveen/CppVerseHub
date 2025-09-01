#!/bin/bash
# File: scripts/clean.sh
# Clean build artifacts and temporary files for CppVerseHub
# Provides comprehensive cleanup with different levels and safety checks

set -e  # Exit on any error

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
PURPLE='\033[0;35m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

# Clean configuration
PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="$PROJECT_ROOT/build"
DOCS_OUTPUT_DIR="$PROJECT_ROOT/docs/generated"
TEST_RESULTS_DIR="$PROJECT_ROOT/test_results"
COVERAGE_DIR="$TEST_RESULTS_DIR/coverage"
CLEAN_LEVEL="standard"
INTERACTIVE_MODE=true
VERBOSE=false
DRY_RUN=false
FORCE_CLEAN=false

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

print_clean() {
    echo -e "${PURPLE}[CLEAN]${NC} $1"
}

print_debug() {
    if [ "$VERBOSE" = true ]; then
        echo -e "${CYAN}[DEBUG]${NC} $1"
    fi
}

# Function to show usage information
show_usage() {
    cat << EOF
CppVerseHub Clean Script
========================

Usage: $0 [OPTIONS]

Options:
    -h, --help              Show this help message
    -v, --verbose           Enable verbose output
    -n, --dry-run           Show what would be cleaned without actually doing it
    -f, --force             Force clean without interactive prompts
    -y, --yes               Answer yes to all prompts (non-interactive mode)
    --level=LEVEL           Set clean level (minimal, standard, deep, nuclear)

Clean Levels:
    minimal                 Clean only build artifacts (build/, *.o, *.a)
    standard               Clean build artifacts + generated docs (default)
    deep                   Clean everything + test results + logs
    nuclear                Clean everything including external dependencies

Directories cleaned:
    build/                 - All build artifacts and binaries
    docs/generated/        - Generated documentation
    test_results/          - Test outputs and coverage reports
    src/data/logs/         - Application log files
    *.tmp, *.log           - Temporary and log files
    .cache/                - Compiler and tool caches
    
Safety Features:
    - Interactive confirmation for destructive operations
    - Dry run mode to preview actions
    - Different clean levels to avoid over-cleaning
    - Backup important files before cleaning
    - Restoration hints for accidentally cleaned files

Examples:
    $0                     # Standard clean with confirmation
    $0 --level=minimal     # Clean only build artifacts
    $0 --level=deep -f     # Deep clean without prompts
    $0 --dry-run           # Preview what would be cleaned
    $0 --level=nuclear     # Complete reset (careful!)

EOF
}

# Function to confirm action
confirm_action() {
    local message="$1"
    local default_answer="$2"
    
    if [ "$FORCE_CLEAN" = true ] || [ "$INTERACTIVE_MODE" = false ]; then
        return 0
    fi
    
    echo -e "${YELLOW}[CONFIRM]${NC} $message"
    if [ "$default_answer" = "y" ]; then
        echo -n "Continue? [Y/n]: "
    else
        echo -n "Continue? [y/N]: "
    fi
    
    read -r response
    response=${response:-$default_answer}
    
    case "$response" in
        [yY]|[yY][eE][sS])
            return 0
            ;;
        *)
            return 1
            ;;
    esac
}

# Function to safely remove directory
safe_remove_dir() {
    local dir_path="$1"
    local description="$2"
    
    if [ ! -d "$dir_path" ]; then
        print_debug "Directory does not exist: $dir_path"
        return 0
    fi
    
    local dir_size=$(du -sh "$dir_path" 2>/dev/null | cut -f1 || echo "unknown")
    
    print_clean "Removing $description ($dir_size): $dir_path"
    
    if [ "$DRY_RUN" = true ]; then
        print_info "[DRY RUN] Would remove: $dir_path"
        return 0
    fi
    
    if confirm_action "Remove $description directory ($dir_size)?" "y"; then
        rm -rf "$dir_path"
        print_success "Removed: $dir_path"
    else
        print_warning "Skipped: $dir_path"
    fi
}

# Function to safely remove files by pattern
safe_remove_files() {
    local pattern="$1"
    local description="$2"
    local base_dir="${3:-$PROJECT_ROOT}"
    
    print_clean "Looking for $description files: $pattern"
    
    # Find files matching pattern
    local files=()
    while IFS= read -r -d '' file; do
        files+=("$file")
    done < <(find "$base_dir" -name "$pattern" -type f -print0 2>/dev/null)
    
    if [ ${#files[@]} -eq 0 ]; then
        print_debug "No $description files found"
        return 0
    fi
    
    local total_size=0
    for file in "${files[@]}"; do
        local size=$(stat -f%z "$file" 2>/dev/null || stat -c%s "$file" 2>/dev/null || echo 0)
        total_size=$((total_size + size))
    done
    
    local human_size=$(numfmt --to=iec "$total_size" 2>/dev/null || echo "$total_size bytes")
    
    print_info "Found ${#files[@]} $description files ($human_size)"
    
    if [ "$DRY_RUN" = true ]; then
        for file in "${files[@]}"; do
            print_info "[DRY RUN] Would remove: $file"
        done
        return 0
    fi
    
    if [ "$VERBOSE" = true ]; then
        echo "Files to be removed:"
        printf '%s\n' "${files[@]}" | sed 's/^/  /'
    fi
    
    if confirm_action "Remove ${#files[@]} $description files ($human_size)?" "y"; then
        for file in "${files[@]}"; do
            rm -f "$file"
            print_debug "Removed file: $file"
        done
        print_success "Removed ${#files[@]} $description files"
    else
        print_warning "Skipped $description files"
    fi
}

# Function to clean build artifacts
clean_build_artifacts() {
    print_clean "Cleaning build artifacts..."
    
    # Remove build directory
    safe_remove_dir "$BUILD_DIR" "build artifacts"
    
    # Remove object files
    safe_remove_files "*.o" "object files"
    safe_remove_files "*.obj" "object files (Windows)"
    
    # Remove static libraries
    safe_remove_files "*.a" "static library files"
    safe_remove_files "*.lib" "static library files (Windows)"
    
    # Remove shared libraries
    safe_remove_files "*.so" "shared library files"
    safe_remove_files "*.dylib" "shared library files (macOS)"
    safe_remove_files "*.dll" "shared library files (Windows)"
    
    # Remove executable files (be careful with this)
    if confirm_action "Remove executable files in source directories?" "n"; then
        find "$PROJECT_ROOT/src" -type f -executable -not -name "*.sh" -not -name "*.py" 2>/dev/null | while read -r exe; do
            if [ "$DRY_RUN" = true ]; then
                print_info "[DRY RUN] Would remove executable: $exe"
            else
                rm -f "$exe"
                print_debug "Removed executable: $exe"
            fi
        done
    fi
    
    # Remove CMake cache files
    safe_remove_files "CMakeCache.txt" "CMake cache files"
    safe_remove_files "cmake_install.cmake" "CMake install files"
    safe_remove_dir "$PROJECT_ROOT/CMakeFiles" "CMake files"
    
    # Remove compilation database
    safe_remove_files "compile_commands.json" "compilation database"
    
    print_success "Build artifacts cleaning completed"
}

# Function to clean documentation
clean_documentation() {
    if [ "$CLEAN_LEVEL" = "minimal" ]; then
        return 0
    fi
    
    print_clean "Cleaning generated documentation..."
    
    # Remove generated docs
    safe_remove_dir "$DOCS_OUTPUT_DIR" "generated documentation"
    
    # Remove Doxygen temporary files
    safe_remove_files "Doxyfile" "Doxygen configuration files"
    safe_remove_files "doxygen.log" "Doxygen log files"
    
    # Remove LaTeX temporary files
    safe_remove_files "*.aux" "LaTeX auxiliary files"
    safe_remove_files "*.log" "LaTeX log files" "$PROJECT_ROOT/docs"
    safe_remove_files "*.toc" "LaTeX table of contents files" "$PROJECT_ROOT/docs"
    safe_remove_files "*.out" "LaTeX output files" "$PROJECT_ROOT/docs"
    
    print_success "Documentation cleaning completed"
}

# Function to clean test results
clean_test_results() {
    if [ "$CLEAN_LEVEL" = "minimal" ] || [ "$CLEAN_LEVEL" = "standard" ]; then
        return 0
    fi
    
    print_clean "Cleaning test results and coverage data..."
    
    # Remove test results directory
    safe_remove_dir "$TEST_RESULTS_DIR" "test results"
    
    # Remove coverage files
    safe_remove_files "*.gcov" "coverage files"
    safe_remove_files "*.gcda" "coverage data files"
    safe_remove_files "*.gcno" "coverage notes files"
    safe_remove_files "coverage.info" "coverage info files"
    safe_remove_files "*.profdata" "profiling data files"
    safe_remove_files "*.profraw" "raw profiling files"
    
    # Remove test log files
    safe_remove_files "*_test.log" "test log files"
    safe_remove_files "test_*.xml" "test result XML files"
    safe_remove_files "test_*.json" "test result JSON files"
    
    # Remove benchmark results
    safe_remove_files "*_benchmark_results.*" "benchmark result files"
    
    # Remove Valgrind output files
    safe_remove_files "valgrind_*.log" "Valgrind log files"
    safe_remove_files "massif.out.*" "Massif output files"
    safe_remove_files "callgrind.out.*" "Callgrind output files"
    
    print_success "Test results cleaning completed"
}

# Function to clean logs
clean_logs() {
    if [ "$CLEAN_LEVEL" = "minimal" ]; then
        return 0
    fi
    
    print_clean "Cleaning log files..."
    
    # Remove application logs
    local logs_dir="$PROJECT_ROOT/src/data/logs"
    if [ -d "$logs_dir" ]; then
        safe_remove_files "*.log" "application log files" "$logs_dir"
    fi
    
    # Remove system logs in project
    safe_remove_files "*.log" "log files"
    safe_remove_files "*.log.*" "rotated log files"
    
    # Remove temporary files
    safe_remove_files "*.tmp" "temporary files"
    safe_remove_files "*.temp" "temporary files"
    safe_remove_files "*~" "backup files"
    safe_remove_files ".#*" "lock files"
    safe_remove_files "#*#" "Emacs backup files"
    
    # Remove core dumps
    safe_remove_files "core" "core dump files"
    safe_remove_files "core.*" "core dump files"
    safe_remove_files "vgcore.*" "Valgrind core files"
    
    print_success "Logs cleaning completed"
}

# Function to clean caches
clean_caches() {
    if [ "$CLEAN_LEVEL" != "deep" ] && [ "$CLEAN_LEVEL" != "nuclear" ]; then
        return 0
    fi
    
    print_clean "Cleaning compiler and tool caches..."
    
    # Remove compiler caches
    safe_remove_dir "$PROJECT_ROOT/.cache" "compiler cache"
    safe_remove_dir "$PROJECT_ROOT/.ccache" "ccache directory"
    
    # Remove package manager caches
    safe_remove_dir "$PROJECT_ROOT/.conan" "Conan cache"
    safe_remove_dir "$PROJECT_ROOT/.vcpkg" "vcpkg cache"
    
    # Remove IDE caches
    safe_remove_dir "$PROJECT_ROOT/.vscode" "VS Code settings"
    safe_remove_dir "$PROJECT_ROOT/.idea" "IntelliJ IDEA settings"
    safe_remove_files "*.swp" "Vim swap files"
    safe_remove_files "*.swo" "Vim swap files"
    safe_remove_files ".DS_Store" "macOS metadata files"
    safe_remove_files "Thumbs.db" "Windows thumbnail files"
    
    print_success "Caches cleaning completed"
}

# Function to clean external dependencies
clean_external_dependencies() {
    if [ "$CLEAN_LEVEL" != "nuclear" ]; then
        return 0
    fi
    
    print_clean "Cleaning external dependencies..."
    print_warning "This will remove all external libraries and require rebuilding!"
    
    if ! confirm_action "Remove ALL external dependencies? This cannot be easily undone!" "n"; then
        print_info "Skipping external dependencies cleaning"
        return 0
    fi
    
    # Remove external directory
    safe_remove_dir "$PROJECT_ROOT/external" "external dependencies"
    
    # Remove package manager files
    safe_remove_files "conanfile.lock" "Conan lock files"
    safe_remove_files "vcpkg.json" "vcpkg manifest files"
    safe_remove_files "Pipfile.lock" "Python lock files"
    safe_remove_files "package-lock.json" "npm lock files"
    
    print_warning "External dependencies removed. Run setup script to reinstall."
    print_success "External dependencies cleaning completed"
}

# Function to create backup of important files
create_backup() {
    if [ "$CLEAN_LEVEL" = "nuclear" ]; then
        print_clean "Creating backup of important files..."
        
        local backup_dir="$PROJECT_ROOT/.cleanup_backup_$(date +%Y%m%d_%H%M%S)"
        mkdir -p "$backup_dir"
        
        # Backup configuration files
        for config_file in CMakeLists.txt .gitignore README.md LICENSE; do
            if [ -f "$PROJECT_ROOT/$config_file" ]; then
                cp "$PROJECT_ROOT/$config_file" "$backup_dir/"
                print_debug "Backed up: $config_file"
            fi
        done
        
        # Backup source data files
        if [ -d "$PROJECT_ROOT/src/data/config" ]; then
            cp -r "$PROJECT_ROOT/src/data/config" "$backup_dir/"
            print_debug "Backed up: src/data/config"
        fi
        
        print_info "Backup created at: $backup_dir"
        print_info "Restore with: cp -r $backup_dir/* $PROJECT_ROOT/"
    fi
}

# Function to show cleaning summary
show_cleaning_summary() {
    local start_time="$1"
    local end_time="$2"
    local duration=$((end_time - start_time))
    
    print_success "Cleaning Summary"
    echo "================"
    echo "Clean Level:      $CLEAN_LEVEL"
    echo "Total Time:       ${duration}s"
    echo "Mode:             $([ "$DRY_RUN" = true ] && echo "DRY RUN" || echo "ACTUAL")"
    echo "Interactive:      $INTERACTIVE_MODE"
    echo ""
    
    # Show current disk usage
    if command -v du >/dev/null 2>&1; then
        echo "Current Project Size:"
        du -sh "$PROJECT_ROOT" 2>/dev/null | sed 's/^/  /'
    fi
    
    echo ""
    echo "Cleaned Components:"
    echo "  ✓ Build artifacts"
    [ "$CLEAN_LEVEL" != "minimal" ] && echo "  ✓ Generated documentation"
    [ "$CLEAN_LEVEL" = "deep" ] || [ "$CLEAN_LEVEL" = "nuclear" ] && echo "  ✓ Test results and coverage"
    [ "$CLEAN_LEVEL" != "minimal" ] && echo "  ✓ Log files"
    [ "$CLEAN_LEVEL" = "deep" ] || [ "$CLEAN_LEVEL" = "nuclear" ] && echo "  ✓ Compiler caches"
    [ "$CLEAN_LEVEL" = "nuclear" ] && echo "  ✓ External dependencies"
    
    if [ "$DRY_RUN" = true ]; then
        echo ""
        print_info "This was a dry run. No files were actually removed."
        print_info "Run without --dry-run to perform actual cleaning."
    else
        echo ""
        print_info "Next steps:"
        echo "  - Run './scripts/build.sh' to rebuild the project"
        [ "$CLEAN_LEVEL" = "nuclear" ] && echo "  - Run './scripts/setup_dev_env.sh' to reinstall dependencies"
        echo "  - Run './scripts/generate_docs.sh' to regenerate documentation"
    fi
}

# Function to estimate cleaning impact
estimate_cleaning_impact() {
    print_info "Estimating cleaning impact for level: $CLEAN_LEVEL"
    
    local total_size=0
    local file_count=0
    
    # Function to add directory size
    add_dir_size() {
        local dir="$1"
        if [ -d "$dir" ]; then
            local size=$(du -s "$dir" 2>/dev/null | cut -f1 || echo 0)
            local files=$(find "$dir" -type f 2>/dev/null | wc -l || echo 0)
            total_size=$((total_size + size))
            file_count=$((file_count + files))
            print_debug "Directory $dir: $size KB, $files files"
        fi
    }
    
    # Add directories based on clean level
    add_dir_size "$BUILD_DIR"
    
    if [ "$CLEAN_LEVEL" != "minimal" ]; then
        add_dir_size "$DOCS_OUTPUT_DIR"
    fi
    
    if [ "$CLEAN_LEVEL" = "deep" ] || [ "$CLEAN_LEVEL" = "nuclear" ]; then
        add_dir_size "$TEST_RESULTS_DIR"
        add_dir_size "$PROJECT_ROOT/.cache"
        add_dir_size "$PROJECT_ROOT/.ccache"
    fi
    
    if [ "$CLEAN_LEVEL" = "nuclear" ]; then
        add_dir_size "$PROJECT_ROOT/external"
    fi
    
    # Convert KB to human readable
    local human_size
    if command -v numfmt >/dev/null 2>&1; then
        human_size=$(echo "$total_size * 1024" | bc | numfmt --to=iec 2>/dev/null || echo "$total_size KB")
    else
        human_size="$total_size KB"
    fi
    
    echo ""
    echo "Estimated Impact:"
    echo "  Files to remove: ~$file_count"
    echo "  Space to free:   ~$human_size"
    echo ""
}

# Parse command line arguments
parse_arguments() {
    while [[ $# -gt 0 ]]; do
        case $1 in
            -h|--help)
                show_usage
                exit 0
                ;;
            -v|--verbose)
                VERBOSE=true
                shift
                ;;
            -n|--dry-run)
                DRY_RUN=true
                shift
                ;;
            -f|--force)
                FORCE_CLEAN=true
                shift
                ;;
            -y|--yes)
                INTERACTIVE_MODE=false
                shift
                ;;
            --level=*)
                CLEAN_LEVEL="${1#*=}"
                case "$CLEAN_LEVEL" in
                    minimal|standard|deep|nuclear)
                        ;;
                    *)
                        print_error "Invalid clean level: $CLEAN_LEVEL"
                        print_error "Valid levels: minimal, standard, deep, nuclear"
                        exit 1
                        ;;
                esac
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

# Main cleaning function
main() {
    local start_time=$(date +%s)
    
    print_info "Starting CppVerseHub cleanup process..."
    print_info "Project root: $PROJECT_ROOT"
    
    # Parse command line arguments
    parse_arguments "$@"
    
    # Show impact estimation
    estimate_cleaning_impact
    
    # Confirm major operations
    if [ "$CLEAN_LEVEL" = "nuclear" ] && [ "$FORCE_CLEAN" != true ]; then
        print_warning "NUCLEAR clean level will remove EVERYTHING including external dependencies!"
        if ! confirm_action "Are you absolutely sure you want to continue?" "n"; then
            print_info "Cleanup cancelled by user"
            exit 0
        fi
    fi
    
    # Create backup for nuclear clean
    create_backup
    
    # Perform cleaning based on level
    clean_build_artifacts
    clean_documentation
    clean_test_results
    clean_logs
    clean_caches
    clean_external_dependencies
    
    local end_time=$(date +%s)
    
    # Show summary
    show_cleaning_summary "$start_time" "$end_time"
    
    if [ "$DRY_RUN" = true ]; then
        print_success "Dry run completed - no files were actually removed"
    else
        print_success "CppVerseHub cleanup completed successfully! 🧹✨"
    fi
}

# Ensure we're in the right directory
cd "$PROJECT_ROOT"

# Run main function with all arguments
main "$@"