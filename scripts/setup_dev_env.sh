#!/bin/bash
# File: scripts/setup_dev_env.sh
# Development environment setup script for CppVerseHub
# Installs dependencies, configures tools, and prepares development environment

set -e  # Exit on any error

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
PURPLE='\033[0;35m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

# Setup configuration
PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
EXTERNAL_DIR="$PROJECT_ROOT/external"
TOOLS_DIR="$PROJECT_ROOT/tools"
INSTALL_SYSTEM_DEPS=true
INSTALL_EXTERNAL_LIBS=true
SETUP_IDE_CONFIG=true
SETUP_GIT_HOOKS=true
INSTALL_OPTIONAL_TOOLS=true
VERBOSE=false
SKIP_CONFIRMATION=false
OFFLINE_MODE=false
OS_TYPE=""
PACKAGE_MANAGER=""

# Version requirements
MIN_CMAKE_VERSION="3.20"
MIN_GCC_VERSION="10"
MIN_CLANG_VERSION="12"
REQUIRED_CPP_STANDARD="20"

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

print_setup() {
    echo -e "${PURPLE}[SETUP]${NC} $1"
}

print_debug() {
    if [ "$VERBOSE" = true ]; then
        echo -e "${CYAN}[DEBUG]${NC} $1"
    fi
}

# Function to show usage information
show_usage() {
    cat << EOF
CppVerseHub Development Environment Setup
=========================================

Usage: $0 [OPTIONS]

Options:
    -h, --help              Show this help message
    -v, --verbose           Enable verbose output
    -y, --yes               Skip confirmation prompts
    --offline               Skip online dependency installations
    --no-system             Skip system dependency installation
    --no-external           Skip external library installation
    --no-ide                Skip IDE configuration setup
    --no-git-hooks          Skip Git hooks installation
    --no-optional           Skip optional tool installation

System Requirements:
    - CMake $MIN_CMAKE_VERSION or later
    - GCC $MIN_GCC_VERSION+ or Clang $MIN_CLANG_VERSION+
    - C++$REQUIRED_CPP_STANDARD support
    - Git (for version control)
    - Python 3.6+ (for scripts)

Components Installed:
    System Dependencies     - Compilers, build tools, libraries
    External Libraries      - JSON, Catch2, Google Benchmark
    Development Tools       - Formatters, static analyzers, documentation
    IDE Configuration       - VS Code, CLion settings
    Git Hooks              - Pre-commit checks and formatting

Supported Platforms:
    - Ubuntu 20.04+ (apt)
    - Debian 10+ (apt)  
    - CentOS 8+ / RHEL 8+ (dnf/yum)
    - Fedora 34+ (dnf)
    - macOS 11+ (brew)
    - Arch Linux (pacman)
    - openSUSE (zypper)

Examples:
    $0                     # Full setup with confirmations
    $0 -y                  # Automated setup
    $0 --no-optional       # Essential components only
    $0 --offline           # Setup without internet downloads

EOF
}

# Function to detect operating system and package manager
detect_system() {
    print_setup "Detecting operating system and package manager..."
    
    if [[ "$OSTYPE" == "linux-gnu"* ]]; then
        if [ -f /etc/os-release ]; then
            . /etc/os-release
            case "$ID" in
                ubuntu|debian)
                    OS_TYPE="debian"
                    PACKAGE_MANAGER="apt"
                    ;;
                centos|rhel|rocky|almalinux)
                    OS_TYPE="redhat"
                    PACKAGE_MANAGER="yum"
                    if command -v dnf >/dev/null 2>&1; then
                        PACKAGE_MANAGER="dnf"
                    fi
                    ;;
                fedora)
                    OS_TYPE="redhat"
                    PACKAGE_MANAGER="dnf"
                    ;;
                arch|manjaro)
                    OS_TYPE="arch"
                    PACKAGE_MANAGER="pacman"
                    ;;
                opensuse*|sled|sles)
                    OS_TYPE="opensuse"
                    PACKAGE_MANAGER="zypper"
                    ;;
                *)
                    OS_TYPE="linux"
                    PACKAGE_MANAGER="unknown"
                    ;;
            esac
        else
            OS_TYPE="linux"
            PACKAGE_MANAGER="unknown"
        fi
    elif [[ "$OSTYPE" == "darwin"* ]]; then
        OS_TYPE="macos"
        PACKAGE_MANAGER="brew"
    elif [[ "$OSTYPE" == "cygwin" ]] || [[ "$OSTYPE" == "msys" ]] || [[ "$OSTYPE" == "win32" ]]; then
        OS_TYPE="windows"
        PACKAGE_MANAGER="unknown"
    else
        OS_TYPE="unknown"
        PACKAGE_MANAGER="unknown"
    fi
    
    print_info "Detected OS: $OS_TYPE"
    print_info "Package manager: $PACKAGE_MANAGER"
    
    if [ "$PACKAGE_MANAGER" = "unknown" ]; then
        print_warning "Unknown package manager. Manual installation may be required."
    fi
}

# Function to check if command exists
command_exists() {
    command -v "$1" >/dev/null 2>&1
}

# Function to compare versions
version_greater_equal() {
    local version1="$1"
    local version2="$2"
    
    # Use sort -V for version comparison
    if command_exists sort; then
        [ "$version1" = "$(echo -e "$version1\n$version2" | sort -V | tail -n1)" ]
    else
        # Fallback: simple string comparison (not always accurate)
        [ "$version1" \> "$version2" ] || [ "$version1" = "$version2" ]
    fi
}

# Function to install system dependencies
install_system_dependencies() {
    if [ "$INSTALL_SYSTEM_DEPS" != true ]; then
        print_info "Skipping system dependencies installation"
        return 0
    fi
    
    print_setup "Installing system dependencies..."
    
    case "$PACKAGE_MANAGER" in
        apt)
            install_debian_dependencies
            ;;
        dnf|yum)
            install_redhat_dependencies
            ;;
        pacman)
            install_arch_dependencies
            ;;
        zypper)
            install_opensuse_dependencies
            ;;
        brew)
            install_macos_dependencies
            ;;
        *)
            print_warning "Unknown package manager. Please install dependencies manually:"
            show_manual_dependencies
            ;;
    esac
}

# Function to install Debian/Ubuntu dependencies
install_debian_dependencies() {
    print_info "Installing dependencies for Debian/Ubuntu..."
    
    # Update package lists
    sudo apt update
    
    # Essential build tools
    sudo apt install -y \
        build-essential \
        cmake \
        ninja-build \
        git \
        pkg-config \
        curl \
        wget
    
    # Compilers
    sudo apt install -y \
        gcc-10 \
        g++-10 \
        clang-12 \
        clang++-12 \
        libc++-dev \
        libc++abi-dev
    
    # Development libraries
    sudo apt install -y \
        libc6-dev \
        libstdc++-10-dev \
        libtbb-dev \
        libboost-dev
    
    # Documentation tools
    if [ "$INSTALL_OPTIONAL_TOOLS" = true ]; then
        sudo apt install -y \
            doxygen \
            graphviz \
            plantuml \
            pandoc \
            texlive-latex-recommended
    fi
    
    # Analysis and debugging tools
    if [ "$INSTALL_OPTIONAL_TOOLS" = true ]; then
        sudo apt install -y \
            valgrind \
            gdb \
            lldb \
            clang-tidy-12 \
            clang-format-12 \
            cppcheck \
            lcov
    fi
    
    # Python for scripts
    sudo apt install -y \
        python3 \
        python3-pip \
        python3-dev
}

# Function to install RedHat/CentOS/Fedora dependencies
install_redhat_dependencies() {
    print_info "Installing dependencies for RedHat/CentOS/Fedora..."
    
    # Development tools
    sudo $PACKAGE_MANAGER groupinstall -y "Development Tools"
    sudo $PACKAGE_MANAGER install -y \
        cmake \
        ninja-build \
        git \
        pkg-config \
        curl \
        wget
    
    # Compilers
    sudo $PACKAGE_MANAGER install -y \
        gcc \
        gcc-c++ \
        clang \
        clang++ \
        libcxx-devel \
        libcxxabi-devel
    
    # Development libraries
    sudo $PACKAGE_MANAGER install -y \
        glibc-devel \
        libstdc++-devel \
        tbb-devel \
        boost-devel
    
    # Documentation tools
    if [ "$INSTALL_OPTIONAL_TOOLS" = true ]; then
        sudo $PACKAGE_MANAGER install -y \
            doxygen \
            graphviz \
            pandoc
        
        # LaTeX (if available)
        sudo $PACKAGE_MANAGER install -y texlive-latex || true
    fi
    
    # Analysis tools
    if [ "$INSTALL_OPTIONAL_TOOLS" = true ]; then
        sudo $PACKAGE_MANAGER install -y \
            valgrind \
            gdb \
            lldb \
            clang-tools-extra \
            cppcheck || true
    fi
    
    # Python
    sudo $PACKAGE_MANAGER install -y \
        python3 \
        python3-pip \
        python3-devel
}

# Function to install Arch Linux dependencies
install_arch_dependencies() {
    print_info "Installing dependencies for Arch Linux..."
    
    # Update package database
    sudo pacman -Syu --noconfirm
    
    # Essential tools
    sudo pacman -S --noconfirm \
        base-devel \
        cmake \
        ninja \
        git \
        pkg-config \
        curl \
        wget
    
    # Compilers
    sudo pacman -S --noconfirm \
        gcc \
        clang \
        libc++ \
        libc++abi
    
    # Development libraries
    sudo pacman -S --noconfirm \
        glibc \
        tbb \
        boost
    
    # Documentation tools
    if [ "$INSTALL_OPTIONAL_TOOLS" = true ]; then
        sudo pacman -S --noconfirm \
            doxygen \
            graphviz \
            plantuml \
            pandoc \
            texlive-core || true
    fi
    
    # Analysis tools
    if [ "$INSTALL_OPTIONAL_TOOLS" = true ]; then
        sudo pacman -S --noconfirm \
            valgrind \
            gdb \
            lldb \
            clang \
            cppcheck
    fi
    
    # Python
    sudo pacman -S --noconfirm \
        python \
        python-pip
}

# Function to install openSUSE dependencies
install_opensuse_dependencies() {
    print_info "Installing dependencies for openSUSE..."
    
    # Development pattern
    sudo zypper install -y -t pattern devel_basis
    
    # Essential tools
    sudo zypper install -y \
        cmake \
        ninja \
        git \
        pkg-config \
        curl \
        wget
    
    # Compilers
    sudo zypper install -y \
        gcc-c++ \
        clang \
        clang++
    
    # Development libraries
    sudo zypper install -y \
        glibc-devel \
        libstdc++-devel \
        tbb-devel
    
    # Documentation tools
    if [ "$INSTALL_OPTIONAL_TOOLS" = true ]; then
        sudo zypper install -y \
            doxygen \
            graphviz \
            pandoc
    fi
    
    # Analysis tools
    if [ "$INSTALL_OPTIONAL_TOOLS" = true ]; then
        sudo zypper install -y \
            valgrind \
            gdb \
            clang-tools \
            cppcheck || true
    fi
    
    # Python
    sudo zypper install -y \
        python3 \
        python3-pip \
        python3-devel
}

# Function to install macOS dependencies
install_macos_dependencies() {
    print_info "Installing dependencies for macOS..."
    
    # Check if Homebrew is installed
    if ! command_exists brew; then
        print_info "Installing Homebrew..."
        /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
        
        # Add Homebrew to PATH
        echo 'eval "$(/opt/homebrew/bin/brew shellenv)"' >> ~/.zprofile
        eval "$(/opt/homebrew/bin/brew shellenv)"
    fi
    
    # Update Homebrew
    brew update
    
    # Essential tools
    brew install \
        cmake \
        ninja \
        git \
        pkg-config \
        curl \
        wget
    
    # Compilers
    brew install \
        gcc \
        llvm
    
    # Development libraries
    brew install \
        tbb \
        boost
    
    # Documentation tools
    if [ "$INSTALL_OPTIONAL_TOOLS" = true ]; then
        brew install \
            doxygen \
            graphviz \
            plantuml \
            pandoc
        
        # LaTeX (optional, large download)
        brew install --cask mactex-no-gui || true
    fi
    
    # Analysis tools
    if [ "$INSTALL_OPTIONAL_TOOLS" = true ]; then
        brew install \
            valgrind || true \
            lldb \
            clang-format \
            cppcheck
    fi
    
    # Python (usually pre-installed, but ensure pip)
    brew install python3
}

# Function to show manual dependency installation instructions
show_manual_dependencies() {
    cat << 'EOF'
Manual Installation Required:
============================

Essential Dependencies:
- CMake 3.20+
- GCC 10+ or Clang 12+
- Git
- Python 3.6+

Optional Dependencies:
- Doxygen (documentation)
- Graphviz (diagrams)
- Valgrind (memory checking)
- Clang-format (code formatting)
- Cppcheck (static analysis)

Please install these using your system's package manager.
EOF
}

# Function to verify system requirements
verify_system_requirements() {
    print_setup "Verifying system requirements..."
    
    local errors=0
    
    # Check CMake
    if command_exists cmake; then
        local cmake_version=$(cmake --version | head -n1 | grep -oE '[0-9]+\.[0-9]+' | head -n1)
        if version_greater_equal "$cmake_version" "$MIN_CMAKE_VERSION"; then
            print_success "CMake $cmake_version (>= $MIN_CMAKE_VERSION) ✓"
        else
            print_error "CMake $cmake_version is too old (need >= $MIN_CMAKE_VERSION)"
            errors=$((errors + 1))
        fi
    else
        print_error "CMake not found"
        errors=$((errors + 1))
    fi
    
    # Check C++ compiler
    local cpp_compiler_found=false
    
    if command_exists g++; then
        local gcc_version=$(g++ --version | head -n1 | grep -oE '[0-9]+\.[0-9]+' | head -n1)
        if version_greater_equal "$gcc_version" "$MIN_GCC_VERSION"; then
            print_success "GCC $gcc_version (>= $MIN_GCC_VERSION) ✓"
            cpp_compiler_found=true
        else
            print_warning "GCC $gcc_version is too old (recommend >= $MIN_GCC_VERSION)"
        fi
    fi
    
    if command_exists clang++; then
        local clang_version=$(clang++ --version | head -n1 | grep -oE '[0-9]+\.[0-9]+' | head -n1)
        if version_greater_equal "$clang_version" "$MIN_CLANG_VERSION"; then
            print_success "Clang $clang_version (>= $MIN_CLANG_VERSION) ✓"
            cpp_compiler_found=true
        else
            print_warning "Clang $clang_version is too old (recommend >= $MIN_CLANG_VERSION)"
        fi
    fi
    
    if [ "$cpp_compiler_found" = false ]; then
        print_error "No suitable C++ compiler found"
        errors=$((errors + 1))
    fi
    
    # Check Git
    if command_exists git; then
        local git_version=$(git --version | grep -oE '[0-9]+\.[0-9]+' | head -n1)
        print_success "Git $git_version ✓"
    else
        print_error "Git not found"
        errors=$((errors + 1))
    fi
    
    # Check Python
    if command_exists python3; then
        local python_version=$(python3 --version | grep -oE '[0-9]+\.[0-9]+' | head -n1)
        print_success "Python $python_version ✓"
    elif command_exists python; then
        local python_version=$(python --version | grep -oE '[0-9]+\.[0-9]+' | head -n1)
        if version_greater_equal "$python_version" "3.6"; then
            print_success "Python $python_version ✓"
        else
            print_warning "Python $python_version may be too old"
        fi
    else
        print_warning "Python not found (optional for some scripts)"
    fi
    
    # Test C++20 support
    print_info "Testing C++$REQUIRED_CPP_STANDARD support..."
    local test_cpp_file="/tmp/cpp20_test.cpp"
    cat > "$test_cpp_file" << 'EOF'
#include <concepts>
#include <ranges>
#include <iostream>

template<std::integral T>
constexpr T add(T a, T b) {
    return a + b;
}

int main() {
    constexpr auto result = add(5, 3);
    std::cout << "C++20 concepts work: " << result << std::endl;
    return 0;
}
EOF
    
    local cpp20_supported=false
    
    if command_exists g++; then
        if g++ -std=c++20 -o /tmp/cpp20_test "$test_cpp_file" >/dev/null 2>&1; then
            print_success "GCC supports C++$REQUIRED_CPP_STANDARD ✓"
            cpp20_supported=true
        fi
    fi
    
    if command_exists clang++; then
        if clang++ -std=c++20 -o /tmp/cpp20_test "$test_cpp_file" >/dev/null 2>&1; then
            print_success "Clang supports C++$REQUIRED_CPP_STANDARD ✓"
            cpp20_supported=true
        fi
    fi
    
    if [ "$cpp20_supported" = false ]; then
        print_warning "C++$REQUIRED_CPP_STANDARD support not detected (some features may not compile)"
    fi
    
    # Cleanup test files
    rm -f /tmp/cpp20_test /tmp/cpp20_test.cpp
    
    if [ $errors -gt 0 ]; then
        print_error "$errors critical dependencies missing"
        return 1
    else
        print_success "All system requirements satisfied"
        return 0
    fi
}

# Function to install external libraries
install_external_libraries() {
    if [ "$INSTALL_EXTERNAL_LIBS" != true ] || [ "$OFFLINE_MODE" = true ]; then
        print_info "Skipping external libraries installation"
        return 0
    fi
    
    print_setup "Installing external libraries..."
    
    mkdir -p "$EXTERNAL_DIR"
    cd "$EXTERNAL_DIR"
    
    # Install nlohmann/json
    install_nlohmann_json
    
    # Install Catch2
    install_catch2
    
    # Install Google Benchmark
    install_google_benchmark
    
    print_success "External libraries installed"
}

# Function to install nlohmann/json
install_nlohmann_json() {
    print_info "Installing nlohmann/json..."
    
    local json_dir="$EXTERNAL_DIR/json"
    
    if [ -d "$json_dir" ]; then
        print_info "nlohmann/json already exists, updating..."
        cd "$json_dir"
        git pull
    else
        git clone --depth 1 --branch v3.11.2 \
            https://github.com/nlohmann/json.git "$json_dir"
    fi
    
    print_success "nlohmann/json installed"
}

# Function to install Catch2
install_catch2() {
    print_info "Installing Catch2..."
    
    local catch2_dir="$EXTERNAL_DIR/catch2"
    
    if [ -d "$catch2_dir" ]; then
        print_info "Catch2 already exists, updating..."
        cd "$catch2_dir"
        git pull
    else
        git clone --depth 1 --branch v3.4.0 \
            https://github.com/catchorg/Catch2.git "$catch2_dir"
    fi
    
    # Build and install Catch2
    cd "$catch2_dir"
    mkdir -p build
    cd build
    
    cmake -DBUILD_TESTING=OFF -DCMAKE_INSTALL_PREFIX="$catch2_dir/install" ..
    make -j$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo "4")
    make install
    
    print_success "Catch2 installed"
}

# Function to install Google Benchmark
install_google_benchmark() {
    print_info "Installing Google Benchmark..."
    
    local benchmark_dir="$EXTERNAL_DIR/benchmark"
    
    if [ -d "$benchmark_dir" ]; then
        print_info "Google Benchmark already exists, updating..."
        cd "$benchmark_dir"
        git pull
    else
        git clone --depth 1 --branch v1.8.3 \
            https://github.com/google/benchmark.git "$benchmark_dir"
    fi
    
    # Build and install Google Benchmark
    cd "$benchmark_dir"
    mkdir -p build
    cd build
    
    cmake -DBENCHMARK_ENABLE_TESTING=OFF \
          -DCMAKE_INSTALL_PREFIX="$benchmark_dir/install" \
          -DCMAKE_BUILD_TYPE=Release ..
    make -j$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo "4")
    make install
    
    print_success "Google Benchmark installed"
}

# Function to setup IDE configurations
setup_ide_configuration() {
    if [ "$SETUP_IDE_CONFIG" != true ]; then
        print_info "Skipping IDE configuration"
        return 0
    fi
    
    print_setup "Setting up IDE configurations..."
    
    # Setup VS Code configuration
    setup_vscode_config
    
    # Setup CLion configuration
    setup_clion_config
    
    # Setup Vim/Neovim configuration
    setup_vim_config
    
    print_success "IDE configurations setup completed"
}

# Function to setup VS Code configuration
setup_vscode_config() {
    print_info "Setting up VS Code configuration..."
    
    local vscode_dir="$PROJECT_ROOT/.vscode"
    mkdir -p "$vscode_dir"
    
    # Create settings.json
    cat > "$vscode_dir/settings.json" << 'EOF'
{
    "C_Cpp.default.cppStandard": "c++20",
    "C_Cpp.default.compilerPath": "/usr/bin/g++",
    "C_Cpp.default.includePath": [
        "${workspaceFolder}/src/**",
        "${workspaceFolder}/external/**"
    ],
    "C_Cpp.default.defines": [],
    "C_Cpp.default.compileCommands": "${workspaceFolder}/build/compile_commands.json",
    "files.associations": {
        "*.hpp": "cpp",
        "*.cpp": "cpp",
        "*.h": "c",
        "*.c": "c"
    },
    "editor.formatOnSave": true,
    "editor.tabSize": 4,
    "editor.insertSpaces": true,
    "clang-format.executable": "/usr/bin/clang-format"
}
EOF
    
    # Create launch.json for debugging
    cat > "$vscode_dir/launch.json" << 'EOF'
{
    "version": "0.2.0",
    "configurations": [
        {
            "name": "Debug CppVerseHub",
            "type": "cppdbg",
            "request": "launch",
            "program": "${workspaceFolder}/build/debug/src/CppVerseHub",
            "args": [],
            "stopAtEntry": false,
            "cwd": "${workspaceFolder}",
            "environment": [],
            "externalConsole": false,
            "MIMode": "gdb",
            "setupCommands": [
                {
                    "description": "Enable pretty-printing for gdb",
                    "text": "-enable-pretty-printing",
                    "ignoreFailures": true
                }
            ],
            "preLaunchTask": "Build Debug",
            "miDebuggerPath": "/usr/bin/gdb",
            "internalConsoleOptions": "openOnSessionStart"
        }
    ]
}
EOF
    
    # Create tasks.json
    cat > "$vscode_dir/tasks.json" << 'EOF'
{
    "version": "2.0.0",
    "tasks": [
        {
            "label": "Build Debug",
            "type": "shell",
            "command": "${workspaceFolder}/scripts/build.sh",
            "args": ["--debug"],
            "group": {
                "kind": "build",
                "isDefault": true
            },
            "presentation": {
                "echo": true,
                "reveal": "always",
                "focus": false,
                "panel": "shared"
            },
            "problemMatcher": ["$gcc"]
        },
        {
            "label": "Build Release",
            "type": "shell",
            "command": "${workspaceFolder}/scripts/build.sh",
            "args": ["--release"],
            "group": "build",
            "presentation": {
                "echo": true,
                "reveal": "always",
                "focus": false,
                "panel": "shared"
            },
            "problemMatcher": ["$gcc"]
        },
        {
            "label": "Run Tests",
            "type": "shell",
            "command": "${workspaceFolder}/scripts/run_tests.sh",
            "group": "test",
            "presentation": {
                "echo": true,
                "reveal": "always",
                "focus": false,
                "panel": "shared"
            }
        },
        {
            "label": "Clean",
            "type": "shell",
            "command": "${workspaceFolder}/scripts/clean.sh",
            "args": ["--force"],
            "group": "build"
        }
    ]
}
EOF
    
    # Create extensions.json with recommended extensions
    cat > "$vscode_dir/extensions.json" << 'EOF'
{
    "recommendations": [
        "ms-vscode.cpptools",
        "ms-vscode.cpptools-extension-pack",
        "ms-vscode.cmake-tools",
        "twxs.cmake",
        "xaver.clang-format",
        "ms-python.python",
        "streetsidesoftware.code-spell-checker",
        "gruntfuggly.todo-tree",
        "eamodio.gitlens"
    ]
}
EOF
    
    print_debug "VS Code configuration created in $vscode_dir"
}

# Function to setup CLion configuration
setup_clion_config() {
    print_info "Setting up CLion configuration..."
    
    local clion_dir="$PROJECT_ROOT/.idea"
    mkdir -p "$clion_dir"
    
    # Create minimal CLion project configuration
    cat > "$clion_dir/misc.xml" << 'EOF'
<?xml version="1.0" encoding="UTF-8"?>
<project version="4">
  <component name="CMakeWorkspace" PROJECT_DIR="$PROJECT_DIR$" />
</project>
EOF
    
    cat > "$clion_dir/modules.xml" << 'EOF'
<?xml version="1.0" encoding="UTF-8"?>
<project version="4">
  <component name="ProjectModuleManager">
    <modules>
      <module fileurl="file://$PROJECT_DIR$/.idea/CppVerseHub.iml" filepath="$PROJECT_DIR$/.idea/CppVerseHub.iml" />
    </modules>
  </component>
</project>
EOF
    
    cat > "$clion_dir/CppVerseHub.iml" << 'EOF'
<?xml version="1.0" encoding="UTF-8"?>
<module classpath="CMake" type="CPP_MODULE" version="4" />
EOF
    
    print_debug "CLion configuration created in $clion_dir"
}

# Function to setup Vim configuration
setup_vim_config() {
    print_info "Creating Vim/Neovim configuration hints..."
    
    # Create a .vimrc snippet for C++ development
    cat > "$PROJECT_ROOT/.vimrc_cpp" << 'EOF'
" CppVerseHub Vim Configuration
" Add this to your ~/.vimrc or ~/.config/nvim/init.vim

set number
set relativenumber
set tabstop=4
set shiftwidth=4
set expandtab
set autoindent
set smartindent

" C++ specific settings
autocmd FileType cpp setlocal commentstring=//\ %s
autocmd FileType cpp setlocal cindent
autocmd FileType cpp setlocal cinoptions=g0,:0,N-s,(0

" Syntax highlighting
syntax enable
filetype plugin indent on

" Show matching brackets
set showmatch

" Search settings
set incsearch
set hlsearch
set ignorecase
set smartcase

" Enable mouse support
set mouse=a

" Status line
set laststatus=2
set statusline=%F%m%r%h%w\ [%l,%c]\ [%L]\ %p%%

" Color scheme (if available)
if has('gui_running')
    colorscheme desert
endif
EOF
    
    print_debug "Vim configuration snippet created: $PROJECT_ROOT/.vimrc_cpp"
}

# Function to setup Git hooks
setup_git_hooks() {
    if [ "$SETUP_GIT_HOOKS" != true ]; then
        print_info "Skipping Git hooks setup"
        return 0
    fi
    
    print_setup "Setting up Git hooks..."
    
    local hooks_dir="$PROJECT_ROOT/.git/hooks"
    
    if [ ! -d "$PROJECT_ROOT/.git" ]; then
        print_warning "Not a Git repository, initializing..."
        cd "$PROJECT_ROOT"
        git init
    fi
    
    mkdir -p "$hooks_dir"
    
    # Create pre-commit hook
    create_pre_commit_hook
    
    # Create pre-push hook
    create_pre_push_hook
    
    # Create commit-msg hook
    create_commit_msg_hook
    
    print_success "Git hooks setup completed"
}

# Function to create pre-commit hook
create_pre_commit_hook() {
    local hook_file="$PROJECT_ROOT/.git/hooks/pre-commit"
    
    cat > "$hook_file" << 'EOF'
#!/bin/bash
# File: .git/hooks/pre-commit
# Pre-commit hook for CppVerseHub

echo "Running pre-commit checks..."

# Check for large files
large_files=$(git diff --cached --name-only | xargs -I {} find . -name {} -size +10M 2>/dev/null)
if [ -n "$large_files" ]; then
    echo "Error: Large files detected (>10MB):"
    echo "$large_files"
    echo "Consider using Git LFS for large files"
    exit 1
fi

# Run clang-format on staged C++ files
if command -v clang-format >/dev/null 2>&1; then
    echo "Running clang-format..."
    git diff --cached --name-only --diff-filter=ACM | grep -E '\.(cpp|hpp|c|h) | while read file; do
        if [ -f "$file" ]; then
            clang-format -i "$file"
            git add "$file"
            echo "Formatted: $file"
        fi
    done
fi

# Check for TODO/FIXME comments in staged files
todos=$(git diff --cached | grep -E '^\+.*\b(TODO|FIXME|XXX|HACK)\b' || true)
if [ -n "$todos" ]; then
    echo "Warning: TODO/FIXME comments found in staged changes:"
    echo "$todos"
    echo "Consider addressing these before committing."
fi

# Check for debugging statements
debug_statements=$(git diff --cached | grep -E '^\+.*(std::cout|printf|std::cerr).*<<' | grep -v '// debug-ok' || true)
if [ -n "$debug_statements" ]; then
    echo "Warning: Debug statements found:"
    echo "$debug_statements"
    echo "Add '// debug-ok' at the end of the line to suppress this warning."
fi

# Run basic syntax check on C++ files if compiler is available
if command -v g++ >/dev/null 2>&1; then
    echo "Running basic syntax check..."
    syntax_errors=0
    git diff --cached --name-only --diff-filter=ACM | grep -E '\.(cpp|hpp) | while read file; do
        if [ -f "$file" ]; then
            if ! g++ -std=c++20 -fsyntax-only "$file" 2>/dev/null; then
                echo "Syntax error in: $file"
                syntax_errors=1
            fi
        fi
    done
    
    if [ $syntax_errors -eq 1 ]; then
        echo "Please fix syntax errors before committing"
        exit 1
    fi
fi

echo "Pre-commit checks completed successfully!"
exit 0
EOF
    
    chmod +x "$hook_file"
    print_debug "Pre-commit hook created"
}

# Function to create pre-push hook
create_pre_push_hook() {
    local hook_file="$PROJECT_ROOT/.git/hooks/pre-push"
    
    cat > "$hook_file" << 'EOF'
#!/bin/bash
# File: .git/hooks/pre-push
# Pre-push hook for CppVerseHub

echo "Running pre-push checks..."

# Check if we can build the project
if [ -f "./scripts/build.sh" ]; then
    echo "Building project to ensure it compiles..."
    if ! ./scripts/build.sh --debug; then
        echo "Build failed! Please fix compilation errors before pushing."
        exit 1
    fi
    echo "Build successful!"
fi

# Run tests if they exist
if [ -f "./scripts/run_tests.sh" ]; then
    echo "Running test suite..."
    if ! ./scripts/run_tests.sh --no-coverage; then
        echo "Tests failed! Please ensure all tests pass before pushing."
        exit 1
    fi
    echo "All tests passed!"
fi

echo "Pre-push checks completed successfully!"
exit 0
EOF
    
    chmod +x "$hook_file"
    print_debug "Pre-push hook created"
}

# Function to create commit-msg hook
create_commit_msg_hook() {
    local hook_file="$PROJECT_ROOT/.git/hooks/commit-msg"
    
    cat > "$hook_file" << 'EOF'
#!/bin/bash
# File: .git/hooks/commit-msg
# Commit message hook for CppVerseHub

commit_regex='^(feat|fix|docs|style|refactor|test|chore)(\(.+\))?: .{1,50}'

if ! grep -qE "$commit_regex" "$1"; then
    echo "Invalid commit message format!"
    echo ""
    echo "Format: <type>(<scope>): <description>"
    echo ""
    echo "Types:"
    echo "  feat:     A new feature"
    echo "  fix:      A bug fix"
    echo "  docs:     Documentation changes"
    echo "  style:    Code style changes (formatting, etc.)"
    echo "  refactor: Code refactoring"
    echo "  test:     Adding or modifying tests"
    echo "  chore:    Build process or auxiliary tool changes"
    echo ""
    echo "Example: feat(core): add new mission type"
    echo "Example: fix(memory): resolve memory leak in ResourceManager"
    exit 1
fi

exit 0
EOF
    
    chmod +x "$hook_file"
    print_debug "Commit message hook created"
}

# Function to setup development tools
setup_development_tools() {
    print_setup "Setting up development tools..."
    
    # Setup clang-format configuration
    setup_clang_format_config
    
    # Setup clang-tidy configuration
    setup_clang_tidy_config
    
    # Setup Doxygen configuration
    setup_doxygen_config
    
    # Install Python development tools
    install_python_dev_tools
    
    print_success "Development tools setup completed"
}

# Function to setup clang-format configuration
setup_clang_format_config() {
    print_info "Setting up clang-format configuration..."
    
    local clang_format_file="$PROJECT_ROOT/.clang-format"
    
    cat > "$clang_format_file" << 'EOF'
# CppVerseHub clang-format configuration
BasedOnStyle: Google

# Basic formatting
IndentWidth: 4
TabWidth: 4
UseTab: Never
ColumnLimit: 100

# Indentation
IndentCaseLabels: true
IndentPPDirectives: BeforeHash
ConstructorInitializerIndentWidth: 4
ContinuationIndentWidth: 4

# Alignment
AlignAfterOpenBracket: Align
AlignConsecutiveAssignments: false
AlignConsecutiveDeclarations: false
AlignOperands: true
AlignTrailingComments: true

# Spacing
SpaceAfterCStyleCast: false
SpaceAfterTemplateKeyword: true
SpaceBeforeAssignmentOperators: true
SpaceBeforeParens: ControlStatements
SpaceInEmptyParentheses: false
SpacesBeforeTrailingComments: 2
SpacesInAngles: false
SpacesInParentheses: false
SpacesInSquareBrackets: false

# Line breaks
AllowShortBlocksOnASingleLine: false
AllowShortCaseLabelsOnASingleLine: false
AllowShortFunctionsOnASingleLine: Inline
AllowShortIfStatementsOnASingleLine: false
AllowShortLoopsOnASingleLine: false
AlwaysBreakAfterDefinitionReturnType: None
AlwaysBreakAfterReturnType: None
AlwaysBreakBeforeMultilineStrings: false
AlwaysBreakTemplateDeclarations: Yes
BreakBeforeBinaryOperators: None
BreakBeforeBraces: Attach
BreakBeforeInheritanceComma: false
BreakInheritanceList: BeforeColon
BreakBeforeTernaryOperators: true
BreakConstructorInitializersBeforeComma: false
BreakConstructorInitializers: BeforeColon
BreakStringLiterals: true

# Sorting
SortIncludes: true
SortUsingDeclarations: true

# C++ specific
Cpp11BracedListStyle: true
FixNamespaceComments: true
NamespaceIndentation: None
Standard: Latest

# Penalties (for line breaking decisions)
PenaltyBreakAssignment: 2
PenaltyBreakBeforeFirstCallParameter: 19
PenaltyBreakComment: 300
PenaltyBreakFirstLessLess: 120
PenaltyBreakString: 1000
PenaltyBreakTemplateDeclaration: 10
PenaltyExcessCharacter: 1000000
PenaltyReturnTypeOnItsOwnLine: 200
EOF
    
    print_debug "clang-format configuration created: $clang_format_file"
}

# Function to setup clang-tidy configuration
setup_clang_tidy_config() {
    print_info "Setting up clang-tidy configuration..."
    
    local clang_tidy_file="$PROJECT_ROOT/.clang-tidy"
    
    cat > "$clang_tidy_file" << 'EOF'
# CppVerseHub clang-tidy configuration
Checks: >
  *,
  -abseil-*,
  -android-*,
  -fuchsia-*,
  -google-*,
  -llvm-*,
  -objc-*,
  -zircon-*,
  -altera-*,
  -cert-env33-c,
  -cert-dcl21-cpp,
  -misc-non-private-member-variables-in-classes,
  -modernize-use-trailing-return-type,
  -readability-magic-numbers,
  -cppcoreguidelines-avoid-magic-numbers,
  -readability-named-parameter,
  -cppcoreguidelines-macro-usage

WarningsAsErrors: ''
HeaderFilterRegex: '.*'
AnalyzeTemporaryDtors: false

CheckOptions:
  - key: readability-identifier-naming.NamespaceCase
    value: lower_case
  - key: readability-identifier-naming.ClassCase
    value: CamelCase
  - key: readability-identifier-naming.StructCase
    value: CamelCase
  - key: readability-identifier-naming.TemplateParameterCase
    value: CamelCase
  - key: readability-identifier-naming.FunctionCase
    value: camelBack
  - key: readability-identifier-naming.VariableCase
    value: camelBack
  - key: readability-identifier-naming.ClassMemberCase
    value: camelBack
  - key: readability-identifier-naming.ClassMemberSuffix
    value: _
  - key: readability-identifier-naming.PrivateMemberSuffix
    value: _
  - key: readability-identifier-naming.ProtectedMemberSuffix
    value: _
  - key: readability-identifier-naming.EnumConstantCase
    value: UPPER_CASE
  - key: readability-identifier-naming.ConstantCase
    value: UPPER_CASE
  - key: readability-identifier-naming.StaticConstantCase
    value: UPPER_CASE
  - key: readability-identifier-naming.MacroDefinitionCase
    value: UPPER_CASE
  - key: readability-function-size.LineThreshold
    value: 80
  - key: readability-function-size.StatementThreshold
    value: 800
  - key: readability-function-size.BranchThreshold
    value: 60
EOF
    
    print_debug "clang-tidy configuration created: $clang_tidy_file"
}

# Function to setup Doxygen configuration
setup_doxygen_config() {
    print_info "Setting up Doxygen configuration..."
    
    mkdir -p "$TOOLS_DIR/doxygen"
    
    # Create custom CSS for Doxygen
    cat > "$TOOLS_DIR/doxygen/custom.css" << 'EOF'
/* CppVerseHub Custom Doxygen Styling */

/* Header and navigation */
#top {
    background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
    border-bottom: none;
}

.header {
    background: transparent;
}

.header .headertitle {
    color: white;
    text-shadow: 1px 1px 2px rgba(0,0,0,0.3);
}

/* Main content area */
.contents {
    background: #fafafa;
    border-radius: 8px;
    margin: 10px;
    padding: 20px;
    box-shadow: 0 2px 10px rgba(0,0,0,0.1);
}

/* Code blocks */
pre.fragment {
    background: #2d3748;
    color: #e2e8f0;
    border: none;
    border-radius: 6px;
    padding: 15px;
    font-family: 'Monaco', 'Menlo', 'Ubuntu Mono', monospace;
}

.fragment .keyword {
    color: #f687b3;
    font-weight: bold;
}

.fragment .comment {
    color: #a0aec0;
    font-style: italic;
}

.fragment .preprocessor {
    color: #fbb6ce;
}

/* Tables */
table.doxtable {
    border-collapse: collapse;
    border: none;
    box-shadow: 0 2px 8px rgba(0,0,0,0.1);
    border-radius: 6px;
    overflow: hidden;
}

table.doxtable th {
    background: linear-gradient(135deg, #4299e1 0%, #3182ce 100%);
    color: white;
    padding: 12px;
    border: none;
}

table.doxtable td {
    padding: 10px 12px;
    border-bottom: 1px solid #e2e8f0;
}

table.doxtable tr:nth-child(even) {
    background-color: #f7fafc;
}

/* Links */
a:link, a:visited {
    color: #3182ce;
    text-decoration: none;
}

a:hover {
    color: #2c5282;
    text-decoration: underline;
}

/* Class/function documentation */
.memitem {
    background: white;
    border: 1px solid #e2e8f0;
    border-radius: 6px;
    margin: 10px 0;
    box-shadow: 0 1px 3px rgba(0,0,0,0.1);
}

.memproto {
    background: linear-gradient(135deg, #edf2f7 0%, #e2e8f0 100%);
    border-bottom: 1px solid #cbd5e0;
    padding: 10px;
    border-radius: 6px 6px 0 0;
}

.memdoc {
    padding: 15px;
}

/* Navigation tree */
.directory .levels span {
    color: #4a5568;
}

.directory .levels a {
    color: #3182ce;
    font-weight: 500;
}

/* Search box */
#MSearchBox {
    background: rgba(255,255,255,0.9);
    border-radius: 20px;
    padding: 5px 15px;
}

/* Responsive design */
@media (max-width: 768px) {
    .contents {
        margin: 5px;
        padding: 10px;
    }
    
    pre.fragment {
        font-size: 12px;
        padding: 10px;
    }
    
    table.doxtable {
        font-size: 14px;
    }
}
EOF
    
    print_debug "Doxygen custom styling created: $TOOLS_DIR/doxygen/custom.css"
}

# Function to install Python development tools
install_python_dev_tools() {
    if ! command_exists python3 && ! command_exists python; then
        print_warning "Python not found, skipping Python tools installation"
        return 0
    fi
    
    print_info "Installing Python development tools..."
    
    local pip_cmd="pip3"
    if ! command_exists pip3 && command_exists pip; then
        pip_cmd="pip"
    fi
    
    if ! command_exists "$pip_cmd"; then
        print_warning "pip not found, skipping Python tools installation"
        return 0
    fi
    
    # Install useful Python packages for development
    local python_packages=(
        "cmake-format"      # CMake formatting
        "pre-commit"        # Git hook management
        "cmakelang"         # CMake language support
        "gitpython"         # Git operations in Python
        "jinja2"            # Template engine for code generation
        "pyyaml"            # YAML processing
        "requests"          # HTTP requests for CI scripts
    )
    
    for package in "${python_packages[@]}"; do
        print_info "Installing Python package: $package"
        if ! $pip_cmd install --user "$package"; then
            print_warning "Failed to install $package, continuing..."
        fi
    done
    
    print_success "Python development tools installed"
}

# Function to create project configuration files
create_project_config_files() {
    print_setup "Creating project configuration files..."
    
    # Create EditorConfig
    create_editorconfig
    
    # Create gitattributes
    create_gitattributes
    
    # Create project-specific gitignore entries
    enhance_gitignore
    
    print_success "Project configuration files created"
}

# Function to create EditorConfig
create_editorconfig() {
    local editorconfig_file="$PROJECT_ROOT/.editorconfig"
    
    cat > "$editorconfig_file" << 'EOF'
# EditorConfig configuration for CppVerseHub
# https://editorconfig.org/

root = true

[*]
charset = utf-8
end_of_line = lf
insert_final_newline = true
trim_trailing_whitespace = true
indent_style = space
indent_size = 4

[*.{cpp,hpp,c,h}]
indent_style = space
indent_size = 4
max_line_length = 100

[*.{cmake,CMakeLists.txt}]
indent_style = space
indent_size = 4

[*.{json,yml,yaml}]
indent_style = space
indent_size = 2

[*.{md,txt}]
trim_trailing_whitespace = false

[*.{sh,bash}]
indent_style = space
indent_size = 4

[Makefile]
indent_style = tab
EOF
    
    print_debug "EditorConfig created: $editorconfig_file"
}

# Function to create gitattributes
create_gitattributes() {
    local gitattributes_file="$PROJECT_ROOT/.gitattributes"
    
    cat > "$gitattributes_file" << 'EOF'
# CppVerseHub Git attributes configuration

# Auto detect text files and perform LF normalization
* text=auto

# Source code files
*.cpp text eol=lf
*.hpp text eol=lf
*.c text eol=lf
*.h text eol=lf
*.cmake text eol=lf
CMakeLists.txt text eol=lf

# Scripts
*.sh text eol=lf
*.bash text eol=lf
*.py text eol=lf

# Documentation
*.md text eol=lf
*.txt text eol=lf
*.rst text eol=lf

# Configuration files
*.json text eol=lf
*.xml text eol=lf
*.yml text eol=lf
*.yaml text eol=lf
*.toml text eol=lf
*.ini text eol=lf
*.cfg text eol=lf
*.conf text eol=lf

# Binary files
*.png binary
*.jpg binary
*.jpeg binary
*.gif binary
*.ico binary
*.pdf binary
*.zip binary
*.tar.gz binary
*.7z binary

# Compiled binaries
*.exe binary
*.dll binary
*.so binary
*.dylib binary
*.a binary
*.lib binary
*.o binary
*.obj binary

# Archive files for Git LFS (if used)
*.zip filter=lfs diff=lfs merge=lfs -text
*.tar.gz filter=lfs diff=lfs merge=lfs -text
*.7z filter=lfs diff=lfs merge=lfs -text
*.rar filter=lfs diff=lfs merge=lfs -text

# Large documentation files for Git LFS
*.pdf filter=lfs diff=lfs merge=lfs -text
EOF
    
    print_debug "Git attributes created: $gitattributes_file"
}

# Function to enhance gitignore
enhance_gitignore() {
    local gitignore_file="$PROJECT_ROOT/.gitignore"
    
    if [ ! -f "$gitignore_file" ]; then
        touch "$gitignore_file"
    fi
    
    # Check if our section already exists
    if ! grep -q "# CppVerseHub specific ignores" "$gitignore_file"; then
        cat >> "$gitignore_file" << 'EOF'

# CppVerseHub specific ignores

# Build directories
build/
Build/
BUILD/
out/
cmake-build-*/

# IDE specific files
.vscode/settings.json
.idea/
*.swp
*.swo
*~
.DS_Store
Thumbs.db

# Compiler and tool outputs
*.o
*.obj
*.a
*.lib
*.so
*.dylib
*.dll
*.exe
compile_commands.json

# Coverage and profiling
*.gcov
*.gcda
*.gcno
coverage.info
*.profdata
*.profraw

# Test outputs
test_results/
*.xml
*.json
*.log
valgrind_*.log
callgrind.out.*
massif.out.*

# Documentation output
docs/generated/
docs/html/
docs/latex/
doxygen.log

# Temporary files
*.tmp
*.temp
*.backup
*.bak
core
core.*
vgcore.*

# Package manager files
conan.lock
vcpkg_installed/
.conan/

# Cache directories
.cache/
.ccache/
EOF
    
        print_debug "Enhanced .gitignore file"
    else
        print_debug ".gitignore already contains CppVerseHub specific entries"
    fi
}

# Function to show setup summary
show_setup_summary() {
    local start_time="$1"
    local end_time="$2"
    local duration=$((end_time - start_time))
    
    print_success "Development Environment Setup Summary"
    echo "====================================="
    echo "Total Setup Time:     ${duration}s"
    echo "OS Type:              $OS_TYPE"
    echo "Package Manager:      $PACKAGE_MANAGER"
    echo ""
    echo "Components Installed:"
    [ "$INSTALL_SYSTEM_DEPS" = true ] && echo "  ✓ System dependencies"
    [ "$INSTALL_EXTERNAL_LIBS" = true ] && echo "  ✓ External libraries"
    [ "$SETUP_IDE_CONFIG" = true ] && echo "  ✓ IDE configurations"
    [ "$SETUP_GIT_HOOKS" = true ] && echo "  ✓ Git hooks"
    echo "  ✓ Development tools"
    echo "  ✓ Project configuration"
    echo ""
    echo "IDE Support:"
    echo "  - VS Code:    .vscode/ directory created"
    echo "  - CLion:      .idea/ directory created"  
    echo "  - Vim:        .vimrc_cpp configuration available"
    echo ""
    echo "Next Steps:"
    echo "  1. Build project:     ./scripts/build.sh"
    echo "  2. Run tests:         ./scripts/run_tests.sh"
    echo "  3. Generate docs:     ./scripts/generate_docs.sh"
    echo "  4. Open in your IDE and start coding!"
    echo ""
    
    # Check for any warnings or issues
    local issues=0
    
    if [ "$PACKAGE_MANAGER" = "unknown" ]; then
        echo "⚠️  Unknown package manager - some dependencies may need manual installation"
        issues=$((issues + 1))
    fi
    
    if [ "$OFFLINE_MODE" = true ]; then
        echo "⚠️  Offline mode - external libraries were not downloaded"
        issues=$((issues + 1))
    fi
    
    if [ $issues -eq 0 ]; then
        print_success "Setup completed successfully with no issues! 🎉"
    else
        print_warning "Setup completed with $issues warnings - see above for details"
    fi
}

# Function to confirm action
confirm_action() {
    local message="$1"
    
    if [ "$SKIP_CONFIRMATION" = true ]; then
        return 0
    fi
    
    echo -e "${YELLOW}[CONFIRM]${NC} $message"
    echo -n "Continue? [Y/n]: "
    read -r response
    response=${response:-Y}
    
    case "$response" in
        [yY]|[yY][eE][sS])
            return 0
            ;;
        *)
            return 1
            ;;
    esac
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
            -y|--yes)
                SKIP_CONFIRMATION=true
                shift
                ;;
            --offline)
                OFFLINE_MODE=true
                INSTALL_EXTERNAL_LIBS=false
                shift
                ;;
            --no-system)
                INSTALL_SYSTEM_DEPS=false
                shift
                ;;
            --no-external)
                INSTALL_EXTERNAL_LIBS=false
                shift
                ;;
            --no-ide)
                SETUP_IDE_CONFIG=false
                shift
                ;;
            --no-git-hooks)
                SETUP_GIT_HOOKS=false
                shift
                ;;
            --no-optional)
                INSTALL_OPTIONAL_TOOLS=false
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

# Main setup function
main() {
    local start_time=$(date +%s)
    
    print_info "Starting CppVerseHub development environment setup..."
    print_info "Project root: $PROJECT_ROOT"
    
    # Parse command line arguments
    parse_arguments "$@"
    
    # Detect system and package manager
    detect_system
    
    # Confirm setup if interactive
    if [ "$SKIP_CONFIRMATION" != true ]; then
        echo ""
        echo "CppVerseHub Development Environment Setup"
        echo "========================================"
        echo "This will install development dependencies and configure your environment."
        echo ""
        echo "Components to be set up:"
        [ "$INSTALL_SYSTEM_DEPS" = true ] && echo "  - System dependencies (compilers, tools)"
        [ "$INSTALL_EXTERNAL_LIBS" = true ] && echo "  - External libraries (JSON, Catch2, Benchmark)"
        [ "$SETUP_IDE_CONFIG" = true ] && echo "  - IDE configurations (VS Code, CLion, Vim)"
        [ "$SETUP_GIT_HOOKS" = true ] && echo "  - Git hooks (formatting, testing)"
        echo "  - Development tools and configurations"
        echo ""
        
        if ! confirm_action "Proceed with setup?"; then
            print_info "Setup cancelled by user"
            exit 0
        fi
    fi
    
    # Run setup steps
    install_system_dependencies
    verify_system_requirements || print_warning "Some requirements not met, continuing anyway..."
    install_external_libraries
    setup_ide_configuration
    setup_git_hooks
    setup_development_tools
    create_project_config_files
    
    local end_time=$(date +%s)
    
    # Show summary
    show_setup_summary "$start_time" "$end_time"
    
    print_success "CppVerseHub development environment setup completed! 🚀"
    print_info "Happy coding! 💻✨"
}

# Ensure we're in the right directory
cd "$PROJECT_ROOT"

# Run main function with all arguments
main "$@"