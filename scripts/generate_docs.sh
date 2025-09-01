#!/bin/bash
# File: scripts/generate_docs.sh
# Documentation generation script for CppVerseHub
# Generates comprehensive documentation using Doxygen, Sphinx, and custom tools

set -e  # Exit on any error

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
PURPLE='\033[0;35m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

# Documentation configuration
PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
DOCS_DIR="$PROJECT_ROOT/docs"
OUTPUT_DIR="$DOCS_DIR/generated"
SOURCE_DIR="$PROJECT_ROOT/src"
DOXYGEN_OUTPUT_DIR="$OUTPUT_DIR/api"
DIAGRAMS_DIR="$DOCS_DIR/UML_diagrams"
GENERATE_API_DOCS=true
GENERATE_UML_DIAGRAMS=true
GENERATE_CHEAT_SHEETS=true
GENERATE_TUTORIALS=true
GENERATE_PDF=false
CLEAN_OUTPUT=false
VERBOSE=false
OPEN_BROWSER=true
CUSTOM_THEME=true
INCLUDE_PRIVATE=false
INCLUDE_SOURCE=true

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

print_docs() {
    echo -e "${PURPLE}[DOCS]${NC} $1"
}

print_debug() {
    if [ "$VERBOSE" = true ]; then
        echo -e "${CYAN}[DEBUG]${NC} $1"
    fi
}

# Function to show usage information
show_usage() {
    cat << EOF
CppVerseHub Documentation Generator
===================================

Usage: $0 [OPTIONS]

Options:
    -h, --help              Show this help message
    -c, --clean             Clean output directory before generation
    -v, --verbose           Enable verbose output
    --no-api                Skip API documentation generation
    --no-diagrams           Skip UML diagram generation
    --no-cheatsheets        Skip cheat sheet generation
    --no-tutorials          Skip tutorial generation
    --pdf                   Generate PDF documentation
    --no-browser            Don't open browser after generation
    --no-theme              Use default Doxygen theme
    --include-private       Include private members in documentation
    --no-source             Don't include source code in documentation
    --output=DIR            Specify custom output directory

Documentation Types:
    API Documentation       - Doxygen-generated API reference
    UML Diagrams           - Class and system architecture diagrams
    Cheat Sheets           - Quick reference guides
    Tutorials              - Step-by-step learning guides
    Design Documents       - Architecture and design explanations

Tools Required:
    doxygen                - For API documentation generation
    dot (graphviz)         - For UML diagram generation
    plantuml               - For advanced diagrams (optional)
    pandoc                 - For PDF generation (optional)
    sphinx                 - For advanced documentation (optional)

Examples:
    $0                     # Generate all documentation
    $0 --clean --pdf       # Clean rebuild with PDF output
    $0 --no-diagrams       # Generate docs without UML diagrams
    $0 --include-private   # Include private members in API docs

EOF
}

# Function to check documentation dependencies
check_doc_dependencies() {
    print_info "Checking documentation dependencies..."
    
    local missing_deps=()
    
    # Check essential tools
    if [ "$GENERATE_API_DOCS" = true ] && ! command -v doxygen >/dev/null 2>&1; then
        missing_deps+=("doxygen")
    fi
    
    if [ "$GENERATE_UML_DIAGRAMS" = true ] && ! command -v dot >/dev/null 2>&1; then
        missing_deps+=("graphviz (dot)")
    fi
    
    if [ "$GENERATE_PDF" = true ] && ! command -v pandoc >/dev/null 2>&1; then
        missing_deps+=("pandoc")
    fi
    
    # Check optional tools
    if ! command -v plantuml >/dev/null 2>&1; then
        print_warning "PlantUML not found - advanced diagrams will be skipped"
    fi
    
    if ! command -v sphinx-build >/dev/null 2>&1; then
        print_warning "Sphinx not found - advanced documentation features disabled"
    fi
    
    # Report missing essential dependencies
    if [ ${#missing_deps[@]} -ne 0 ]; then
        print_error "Missing required dependencies:"
        for dep in "${missing_deps[@]}"; do
            echo "  - $dep"
        done
        print_error "Please install missing dependencies and try again"
        exit 1
    fi
    
    print_success "All required dependencies are available"
}

# Function to setup output directories
setup_output_directories() {
    print_info "Setting up output directories..."
    
    if [ "$CLEAN_OUTPUT" = true ]; then
        print_info "Cleaning previous output..."
        rm -rf "$OUTPUT_DIR"
    fi
    
    # Create directory structure
    mkdir -p "$OUTPUT_DIR"
    mkdir -p "$DOXYGEN_OUTPUT_DIR"
    mkdir -p "$OUTPUT_DIR/cheat_sheets"
    mkdir -p "$OUTPUT_DIR/tutorials"
    mkdir -p "$OUTPUT_DIR/diagrams"
    mkdir -p "$OUTPUT_DIR/design_docs"
    
    print_success "Output directories prepared"
}

# Function to generate Doxygen configuration
generate_doxygen_config() {
    print_docs "Generating Doxygen configuration..."
    
    local doxygen_config="$OUTPUT_DIR/Doxyfile"
    local project_version=$(grep -r "VERSION" "$PROJECT_ROOT/CMakeLists.txt" | head -n1 | sed 's/.*VERSION \([0-9.]*\).*/\1/' || echo "1.0.0")
    
    cat > "$doxygen_config" << EOF
# Doxyfile for CppVerseHub
# Generated on $(date)

#---------------------------------------------------------------------------
# Project related configuration options
#---------------------------------------------------------------------------

DOXYFILE_ENCODING      = UTF-8
PROJECT_NAME           = "CppVerseHub"
PROJECT_NUMBER         = $project_version
PROJECT_BRIEF          = "Comprehensive C++ Learning and Demonstration Platform"
PROJECT_LOGO           = 

OUTPUT_DIRECTORY       = $DOXYGEN_OUTPUT_DIR
CREATE_SUBDIRS         = NO
ALLOW_UNICODE_NAMES    = NO
OUTPUT_LANGUAGE        = English

BRIEF_MEMBER_DESC      = YES
REPEAT_BRIEF           = YES
ABBREVIATE_BRIEF       = "The \$name class" \
                         "The \$name widget" \
                         "The \$name file" \
                         "is" \
                         "provides" \
                         "specifies" \
                         "contains" \
                         "represents" \
                         "a" \
                         "an" \
                         "the"

ALWAYS_DETAILED_SEC    = NO
INLINE_INHERITED_MEMB  = NO
FULL_PATH_NAMES        = YES
STRIP_FROM_PATH        = $PROJECT_ROOT
STRIP_FROM_INC_PATH    = 
SHORT_NAMES            = NO
JAVADOC_AUTOBRIEF      = YES
JAVADOC_BANNER         = NO
QT_AUTOBRIEF           = NO
MULTILINE_CPP_IS_BRIEF = NO
PYTHON_DOCSTRING       = YES
INHERIT_DOCS           = YES
SEPARATE_MEMBER_PAGES  = NO
TAB_SIZE               = 4
ALIASES                = 
OPTIMIZE_OUTPUT_FOR_C  = NO
OPTIMIZE_OUTPUT_JAVA   = NO
OPTIMIZE_FOR_FORTRAN   = NO
OPTIMIZE_OUTPUT_VHDL   = NO
OPTIMIZE_OUTPUT_SLICE  = NO

#---------------------------------------------------------------------------
# Build related configuration options
#---------------------------------------------------------------------------

EXTRACT_ALL            = YES
EXTRACT_PRIVATE        = $([ "$INCLUDE_PRIVATE" = true ] && echo "YES" || echo "NO")
EXTRACT_PRIV_VIRTUAL   = NO
EXTRACT_PACKAGE        = NO
EXTRACT_STATIC         = YES
EXTRACT_LOCAL_CLASSES  = YES
EXTRACT_LOCAL_METHODS  = NO
EXTRACT_ANON_NSPACES   = NO
HIDE_UNDOC_MEMBERS     = NO
HIDE_UNDOC_CLASSES     = NO
HIDE_FRIEND_COMPOUNDS  = NO
HIDE_IN_BODY_DOCS      = NO
INTERNAL_DOCS          = NO
CASE_SENSE_NAMES       = YES
HIDE_SCOPE_NAMES       = NO
HIDE_COMPOUND_REFERENCE= NO
SHOW_INCLUDE_FILES     = YES
SHOW_GROUPED_MEMB_INC  = NO
FORCE_LOCAL_INCLUDES   = NO
INLINE_INFO            = YES
SORT_MEMBER_DOCS       = YES
SORT_BRIEF_DOCS        = NO
SORT_MEMBERS_CTORS_1ST = NO
SORT_GROUP_NAMES       = NO
SORT_BY_SCOPE_NAME     = NO
STRICT_PROTO_MATCHING  = NO
GENERATE_TODOLIST      = YES
GENERATE_TESTLIST      = YES
GENERATE_BUGLIST       = YES
GENERATE_DEPRECATEDLIST= YES
ENABLED_SECTIONS       = 
MAX_INITIALIZER_LINES  = 30
SHOW_USED_FILES        = YES
SHOW_FILES             = YES
SHOW_NAMESPACES        = YES

#---------------------------------------------------------------------------
# Configuration options related to warning and progress messages
#---------------------------------------------------------------------------

QUIET                  = $([ "$VERBOSE" = true ] && echo "NO" || echo "YES")
WARNINGS               = YES
WARN_IF_UNDOCUMENTED   = YES
WARN_IF_DOC_ERROR      = YES
WARN_NO_PARAMDOC       = NO
WARN_AS_ERROR          = NO
WARN_FORMAT            = "\$file:\$line: \$text"
WARN_LOGFILE           = 

#---------------------------------------------------------------------------
# Configuration options related to the input files
#---------------------------------------------------------------------------

INPUT                  = $SOURCE_DIR \\
                         $DOCS_DIR/design_docs

INPUT_ENCODING         = UTF-8
FILE_PATTERNS          = *.c \\
                         *.cc \\
                         *.cxx \\
                         *.cpp \\
                         *.c++ \\
                         *.h \\
                         *.hh \\
                         *.hxx \\
                         *.hpp \\
                         *.h++ \\
                         *.md

RECURSIVE              = YES
EXCLUDE                = $SOURCE_DIR/external \\
                         $SOURCE_DIR/build

EXCLUDE_SYMLINKS       = NO
EXCLUDE_PATTERNS       = */tests/* \\
                         */build/* \\
                         */external/*

EXCLUDE_SYMBOLS        = 
EXAMPLE_PATH           = $PROJECT_ROOT/examples
EXAMPLE_PATTERNS       = *
EXAMPLE_RECURSIVE      = YES
IMAGE_PATH             = $DOCS_DIR/images \\
                         $DIAGRAMS_DIR

INPUT_FILTER           = 
FILTER_PATTERNS        = 
FILTER_SOURCE_FILES    = NO
FILTER_SOURCE_PATTERNS = 
USE_MDFILE_AS_MAINPAGE = $PROJECT_ROOT/README.md

#---------------------------------------------------------------------------
# Configuration options related to source browsing
#---------------------------------------------------------------------------

SOURCE_BROWSER         = $([ "$INCLUDE_SOURCE" = true ] && echo "YES" || echo "NO")
INLINE_SOURCES         = NO
STRIP_CODE_COMMENTS    = YES
REFERENCED_BY_RELATION = NO
REFERENCES_RELATION    = NO
REFERENCES_LINK_SOURCE = YES
SOURCE_TOOLTIPS        = YES
USE_HTAGS              = NO
VERBATIM_HEADERS       = YES
CLANG_ASSISTED_PARSING = NO
CLANG_DATABASE_PATH    = 

#---------------------------------------------------------------------------
# Configuration options related to the alphabetical class index
#---------------------------------------------------------------------------

ALPHABETICAL_INDEX     = YES
IGNORE_PREFIX          = 

#---------------------------------------------------------------------------
# Configuration options related to the HTML output
#---------------------------------------------------------------------------

GENERATE_HTML          = YES
HTML_OUTPUT            = html
HTML_FILE_EXTENSION    = .html
HTML_HEADER            = 
HTML_FOOTER            = 
HTML_STYLESHEET        = 
HTML_EXTRA_STYLESHEET  = $([ "$CUSTOM_THEME" = true ] && echo "$PROJECT_ROOT/tools/doxygen/custom.css" || echo "")
HTML_EXTRA_FILES       = 
HTML_COLORSTYLE_HUE    = 220
HTML_COLORSTYLE_SAT    = 100
HTML_COLORSTYLE_GAMMA  = 80
HTML_TIMESTAMP         = YES
HTML_DYNAMIC_MENUS     = YES
HTML_DYNAMIC_SECTIONS  = NO
HTML_INDEX_NUM_ENTRIES = 100
GENERATE_DOCSET        = NO
GENERATE_HTMLHELP      = NO
GENERATE_QHP           = NO
GENERATE_ECLIPSEHELP   = NO
DISABLE_INDEX          = NO
GENERATE_TREEVIEW      = YES
ENUM_VALUES_PER_LINE   = 4
TREEVIEW_WIDTH         = 250
EXT_LINKS_IN_WINDOW    = NO
HTML_FORMULA_FORMAT    = png
FORMULA_FONTSIZE       = 10
FORMULA_TRANSPARENT    = YES
FORMULA_MACROFILE      = 
USE_MATHJAX            = NO
SEARCHENGINE           = YES
SERVER_BASED_SEARCH    = NO
EXTERNAL_SEARCH        = NO
SEARCHENGINE_URL       = 
SEARCHDATA_FILE        = searchdata.xml
EXTERNAL_SEARCH_ID     = 
EXTRA_SEARCH_MAPPINGS  = 

#---------------------------------------------------------------------------
# Configuration options related to the LaTeX output
#---------------------------------------------------------------------------

GENERATE_LATEX         = $([ "$GENERATE_PDF" = true ] && echo "YES" || echo "NO")
LATEX_OUTPUT           = latex
LATEX_CMD_NAME         = latex
MAKEINDEX_CMD_NAME     = makeindex
LATEX_MAKEINDEX_CMD    = makeindex
COMPACT_LATEX          = NO
PAPER_TYPE             = a4
EXTRA_PACKAGES         = 
LATEX_HEADER           = 
LATEX_FOOTER           = 
LATEX_EXTRA_STYLESHEET = 
LATEX_EXTRA_FILES      = 
PDF_HYPERLINKS         = YES
USE_PDFLATEX           = YES
LATEX_BATCHMODE        = NO
LATEX_HIDE_INDICES     = NO
LATEX_SOURCE_CODE      = NO
LATEX_BIB_STYLE        = plain
LATEX_TIMESTAMP        = NO
LATEX_EMOJI_DIRECTORY  = 

#---------------------------------------------------------------------------
# Configuration options related to the RTF output
#---------------------------------------------------------------------------

GENERATE_RTF           = NO

#---------------------------------------------------------------------------
# Configuration options related to the man page output
#---------------------------------------------------------------------------

GENERATE_MAN           = NO

#---------------------------------------------------------------------------
# Configuration options related to the XML output
#---------------------------------------------------------------------------

GENERATE_XML           = NO

#---------------------------------------------------------------------------
# Configuration options related to the DOCBOOK output
#---------------------------------------------------------------------------

GENERATE_DOCBOOK       = NO

#---------------------------------------------------------------------------
# Configuration options for the AutoGen Definitions output
#---------------------------------------------------------------------------

GENERATE_AUTOGEN_DEF   = NO

#---------------------------------------------------------------------------
# Configuration options related to the Perl module output
#---------------------------------------------------------------------------

GENERATE_PERLMOD       = NO

#---------------------------------------------------------------------------
# Configuration options related to the preprocessor
#---------------------------------------------------------------------------

ENABLE_PREPROCESSING   = YES
MACRO_EXPANSION        = NO
EXPAND_ONLY_PREDEF     = NO
SEARCH_INCLUDES        = YES
INCLUDE_PATH           = $SOURCE_DIR
INCLUDE_FILE_PATTERNS  = 
PREDEFINED             = 
EXPAND_AS_DEFINED      = 
SKIP_FUNCTION_MACROS   = YES

#---------------------------------------------------------------------------
# Configuration options related to external references
#---------------------------------------------------------------------------

TAGFILES               = 
GENERATE_TAGFILE       = 
ALLEXTERNALS           = NO
EXTERNAL_GROUPS        = YES
EXTERNAL_PAGES         = YES

#---------------------------------------------------------------------------
# Configuration options related to the dot tool
#---------------------------------------------------------------------------

CLASS_DIAGRAMS         = YES
DIA_PATH               = 
HIDE_UNDOC_RELATIONS   = YES
HAVE_DOT               = $(command -v dot >/dev/null 2>&1 && echo "YES" || echo "NO")
DOT_NUM_THREADS        = 0
DOT_FONTNAME           = Helvetica
DOT_FONTSIZE           = 10
DOT_FONTPATH           = 
CLASS_GRAPH            = YES
COLLABORATION_GRAPH    = YES
GROUP_GRAPHS           = YES
UML_LOOK               = NO
UML_LIMIT_NUM_FIELDS   = 10
TEMPLATE_RELATIONS     = NO
INCLUDE_GRAPH          = YES
INCLUDED_BY_GRAPH      = YES
CALL_GRAPH             = NO
CALLER_GRAPH           = NO
GRAPHICAL_HIERARCHY    = YES
DIRECTORY_GRAPH        = YES
DOT_IMAGE_FORMAT       = png
INTERACTIVE_SVG        = NO
DOT_PATH               = 
DOTFILE_DIRS           = 
MSCFILE_DIRS           = 
DIAFILE_DIRS           = 
PLANTUML_JAR_PATH      = 
PLANTUML_CFG_FILE      = 
PLANTUML_INCLUDE_PATH  = 
DOT_GRAPH_MAX_NODES    = 50
MAX_DOT_GRAPH_DEPTH    = 0
DOT_TRANSPARENT        = NO
DOT_MULTI_TARGETS      = YES
GENERATE_LEGEND        = YES
DOT_CLEANUP            = YES

EOF

    print_success "Doxygen configuration generated"
}

# Function to generate API documentation
generate_api_documentation() {
    if [ "$GENERATE_API_DOCS" != true ]; then
        return 0
    fi
    
    print_docs "Generating API documentation with Doxygen..."
    
    # Generate Doxygen configuration
    generate_doxygen_config
    
    # Run Doxygen
    local doxygen_config="$OUTPUT_DIR/Doxyfile"
    
    print_debug "Running: doxygen $doxygen_config"
    
    if [ "$VERBOSE" = true ]; then
        doxygen "$doxygen_config"
    else
        doxygen "$doxygen_config" > "$OUTPUT_DIR/doxygen.log" 2>&1
    fi
    
    if [ $? -eq 0 ]; then
        print_success "API documentation generated successfully"
        
        # Generate PDF if requested
        if [ "$GENERATE_PDF" = true ] && [ -d "$DOXYGEN_OUTPUT_DIR/latex" ]; then
            print_info "Generating PDF documentation..."
            cd "$DOXYGEN_OUTPUT_DIR/latex"
            
            if command -v pdflatex >/dev/null 2>&1; then
                make > /dev/null 2>&1
                if [ -f "refman.pdf" ]; then
                    cp "refman.pdf" "$OUTPUT_DIR/CppVerseHub_API_Documentation.pdf"
                    print_success "PDF documentation generated: CppVerseHub_API_Documentation.pdf"
                fi
            else
                print_warning "pdflatex not found - PDF generation skipped"
            fi
            
            cd "$PROJECT_ROOT"
        fi
        
    else
        print_error "API documentation generation failed"
        if [ -f "$OUTPUT_DIR/doxygen.log" ]; then
            print_error "Check $OUTPUT_DIR/doxygen.log for details"
        fi
        return 1
    fi
}

# Function to generate UML diagrams
generate_uml_diagrams() {
    if [ "$GENERATE_UML_DIAGRAMS" != true ]; then
        return 0
    fi
    
    print_docs "Generating UML diagrams..."
    
    # Create class hierarchy diagram
    generate_class_hierarchy_diagram
    
    # Create mission polymorphism diagram
    generate_mission_polymorphism_diagram
    
    # Create design patterns diagram
    generate_design_patterns_diagram
    
    # Create system architecture diagram
    generate_system_architecture_diagram
    
    print_success "UML diagrams generated"
}

# Function to generate class hierarchy diagram
generate_class_hierarchy_diagram() {
    local output_file="$OUTPUT_DIR/diagrams/class_hierarchy.svg"
    
    cat > "${output_file%.svg}.dot" << 'EOF'
digraph ClassHierarchy {
    rankdir=TB;
    node [shape=box, style=filled, fillcolor=lightblue, fontname="Arial"];
    edge [arrowhead=empty];
    
    // Abstract base classes
    Entity [fillcolor=orange, label="Entity\n(Abstract Base)"];
    Mission [fillcolor=orange, label="Mission\n(Abstract Base)"];
    
    // Concrete entity classes
    Planet [label="Planet"];
    Fleet [label="Fleet"];
    
    // Mission types
    ExplorationMission [label="ExplorationMission"];
    ColonizationMission [label="ColonizationMission"];
    CombatMission [label="CombatMission"];
    
    // Singletons
    ResourceManager [fillcolor=yellow, label="ResourceManager\n(Singleton)"];
    GameEngine [fillcolor=yellow, label="GameEngine"];
    
    // Factory
    Factory [fillcolor=lightgreen, label="Factory\n(Abstract Factory)"];
    
    // Relationships
    Entity -> Planet;
    Entity -> Fleet;
    
    Mission -> ExplorationMission;
    Mission -> ColonizationMission;
    Mission -> CombatMission;
    
    GameEngine -> ResourceManager [style=dashed, label="uses"];
    GameEngine -> Factory [style=dashed, label="uses"];
    Factory -> Planet [style=dashed, label="creates"];
    Factory -> Fleet [style=dashed, label="creates"];
    
    // Styling
    { rank=same; Planet; Fleet; }
    { rank=same; ExplorationMission; ColonizationMission; CombatMission; }
}
EOF
    
    if command -v dot >/dev/null 2>&1; then
        dot -Tsvg "${output_file%.svg}.dot" -o "$output_file"
        print_debug "Generated class hierarchy diagram: $output_file"
    else
        print_warning "dot command not found - class hierarchy diagram skipped"
    fi
}

# Function to generate mission polymorphism diagram
generate_mission_polymorphism_diagram() {
    local output_file="$OUTPUT_DIR/diagrams/mission_polymorphism.svg"
    
    cat > "${output_file%.svg}.dot" << 'EOF'
digraph MissionPolymorphism {
    rankdir=TB;
    node [shape=box, style=filled, fillcolor=lightblue, fontname="Arial"];
    edge [arrowhead=empty];
    
    // Base mission class
    Mission [fillcolor=orange, label="Mission\n+ execute() = 0\n+ getStatus()\n+ getDuration()"];
    
    // Derived mission classes
    Exploration [fillcolor=lightgreen, label="ExplorationMission\n+ execute()\n+ scanPlanet()\n+ discoverResources()"];
    Colonization [fillcolor=lightgreen, label="ColonizationMission\n+ execute()\n+ establishColony()\n+ deployResources()"];
    Combat [fillcolor=lightgreen, label="CombatMission\n+ execute()\n+ engageTarget()\n+ calculateDamage()"];
    
    // Mission states
    Pending [shape=ellipse, fillcolor=lightyellow, label="Pending"];
    Active [shape=ellipse, fillcolor=lightcyan, label="Active"];
    Completed [shape=ellipse, fillcolor=lightgreen, label="Completed"];
    Failed [shape=ellipse, fillcolor=salmon, label="Failed"];
    
    // Polymorphic relationships
    Mission -> Exploration;
    Mission -> Colonization;
    Mission -> Combat;
    
    // State transitions
    Pending -> Active [label="start()"];
    Active -> Completed [label="success()"];
    Active -> Failed [label="failure()"];
    
    // Virtual method call illustration
    Client [shape=ellipse, fillcolor=lightgray, label="Client Code"];
    Client -> Mission [style=dashed, label="mission->execute()"];
    
    { rank=same; Exploration; Colonization; Combat; }
    { rank=same; Pending; Active; Completed; Failed; }
}
EOF
    
    if command -v dot >/dev/null 2>&1; then
        dot -Tsvg "${output_file%.svg}.dot" -o "$output_file"
        print_debug "Generated mission polymorphism diagram: $output_file"
    fi
}

# Function to generate design patterns diagram
generate_design_patterns_diagram() {
    local output_file="$OUTPUT_DIR/diagrams/design_patterns.svg"
    
    cat > "${output_file%.svg}.dot" << 'EOF'
digraph DesignPatterns {
    rankdir=TB;
    node [shape=box, style=filled, fontname="Arial"];
    
    // Pattern categories
    subgraph cluster_creational {
        label="Creational Patterns";
        style=filled;
        fillcolor=lightblue;
        
        Singleton [fillcolor=yellow, label="Singleton\nResourceManager"];
        Factory [fillcolor=yellow, label="Abstract Factory\nEntityFactory"];
        Builder [fillcolor=yellow, label="Builder\nFleetBuilder"];
    }
    
    subgraph cluster_structural {
        label="Structural Patterns";
        style=filled;
        fillcolor=lightgreen;
        
        Decorator [fillcolor=lightcyan, label="Decorator\nMissionEnhancer"];
        Adapter [fillcolor=lightcyan, label="Adapter\nLegacySystemAdapter"];
    }
    
    subgraph cluster_behavioral {
        label="Behavioral Patterns";
        style=filled;
        fillcolor=lightyellow;
        
        Strategy [fillcolor=orange, label="Strategy\nRoutingStrategy"];
        Observer [fillcolor=orange, label="Observer\nPlanetMonitor"];
        Command [fillcolor=orange, label="Command\nFleetCommand"];
        State [fillcolor=orange, label="State\nMissionState"];
    }
    
    // Usage relationships
    GameEngine -> Singleton [style=dashed, label="uses"];
    GameEngine -> Factory [style=dashed, label="uses"];
    GameEngine -> Strategy [style=dashed, label="uses"];
    Fleet -> Command [style=dashed, label="uses"];
    Mission -> State [style=dashed, label="uses"];
    Planet -> Observer [style=dashed, label="notifies"];
}
EOF
    
    if command -v dot >/dev/null 2>&1; then
        dot -Tsvg "${output_file%.svg}.dot" -o "$output_file"
        print_debug "Generated design patterns diagram: $output_file"
    fi
}

# Function to generate system architecture diagram
generate_system_architecture_diagram() {
    local output_file="$OUTPUT_DIR/diagrams/system_architecture.svg"
    
    cat > "${output_file%.svg}.dot" << 'EOF'
digraph SystemArchitecture {
    rankdir=TB;
    node [shape=box, style=filled, fontname="Arial"];
    
    // Layer definitions
    subgraph cluster_presentation {
        label="Presentation Layer";
        style=filled;
        fillcolor=lightblue;
        
        CLI [label="Command Line\nInterface"];
        Logger [label="Logging\nSystem"];
    }
    
    subgraph cluster_application {
        label="Application Layer";
        style=filled;
        fillcolor=lightgreen;
        
        GameEngine [label="Game Engine\n(Main Controller)"];
        ConfigManager [label="Configuration\nManager"];
        FileParser [label="File Parser\n(JSON/CSV/XML)"];
    }
    
    subgraph cluster_domain {
        label="Domain Layer";
        style=filled;
        fillcolor=lightyellow;
        
        subgraph cluster_entities {
            label="Entities";
            style=dashed;
            
            Entity [label="Entity\n(Base Class)"];
            Planet [label="Planet"];
            Fleet [label="Fleet"];
        }
        
        subgraph cluster_missions {
            label="Missions";
            style=dashed;
            
            Mission [label="Mission\n(Base Class)"];
            ExplorationMission [label="Exploration\nMission"];
            ColonizationMission [label="Colonization\nMission"];
            CombatMission [label="Combat\nMission"];
        }
        
        ResourceManager [label="Resource Manager\n(Singleton)"];
        Factory [label="Entity Factory"];
    }
    
    subgraph cluster_infrastructure {
        label="Infrastructure Layer";
        style=filled;
        fillcolor=lightcoral;
        
        STL [label="STL Components\n(Containers, Algorithms)"];
        Concurrency [label="Threading\n& Async"];
        Memory [label="Memory\nManagement"];
        Templates [label="Template\nSystem"];
    }
    
    subgraph cluster_patterns {
        label="Design Patterns";
        style=filled;
        fillcolor=lightpink;
        
        Strategy [label="Strategy"];
        Observer [label="Observer"];
        Decorator [label="Decorator"];
        Command [label="Command"];
    }
    
    // Layer relationships
    CLI -> GameEngine;
    GameEngine -> ConfigManager;
    GameEngine -> FileParser;
    GameEngine -> ResourceManager;
    GameEngine -> Factory;
    
    Factory -> Entity;
    Entity -> Planet;
    Entity -> Fleet;
    
    Mission -> ExplorationMission;
    Mission -> ColonizationMission;
    Mission -> CombatMission;
    
    GameEngine -> Strategy;
    Planet -> Observer;
    Mission -> Decorator;
    Fleet -> Command;
    
    // Infrastructure dependencies
    GameEngine -> STL [style=dashed];
    GameEngine -> Concurrency [style=dashed];
    ResourceManager -> Memory [style=dashed];
    Factory -> Templates [style=dashed];
}
EOF
    
    if command -v dot >/dev/null 2>&1; then
        dot -Tsvg "${output_file%.svg}.dot" -o "$output_file"
        print_debug "Generated system architecture diagram: $output_file"
    fi
}

# Function to generate cheat sheets
generate_cheat_sheets() {
    if [ "$GENERATE_CHEAT_SHEETS" != true ]; then
        return 0
    fi
    
    print_docs "Generating enhanced cheat sheets..."
    
    # Process existing cheat sheets and enhance them
    for cheat_sheet in "$DOCS_DIR/cheat_sheets"/*.md; do
        if [ -f "$cheat_sheet" ]; then
            local basename=$(basename "$cheat_sheet")
            local output_file="$OUTPUT_DIR/cheat_sheets/$basename"
            
            # Copy and enhance cheat sheet
            cp "$cheat_sheet" "$output_file"
            
            # Add generation timestamp and styling
            {
                echo ""
                echo "---"
                echo ""
                echo "*Generated on: $(date)*"
                echo ""
                echo "*This cheat sheet is part of the CppVerseHub comprehensive C++ learning platform.*"
                echo ""
                echo "[🏠 Back to Documentation Home]($OUTPUT_DIR/index.html) | [📚 All Cheat Sheets](./)"
            } >> "$output_file"
            
            print_debug "Enhanced cheat sheet: $basename"
        fi
    done
    
    # Generate master cheat sheet index
    generate_cheat_sheet_index
    
    print_success "Cheat sheets generated and enhanced"
}

# Function to generate cheat sheet index
generate_cheat_sheet_index() {
    local index_file="$OUTPUT_DIR/cheat_sheets/index.html"
    
    cat > "$index_file" << EOF
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>CppVerseHub - Cheat Sheets</title>
    <style>
        body { font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif; margin: 40px; background: #f5f5f5; }
        .container { background: white; padding: 30px; border-radius: 8px; box-shadow: 0 2px 10px rgba(0,0,0,0.1); }
        .header { border-bottom: 2px solid #007acc; padding-bottom: 20px; margin-bottom: 30px; }
        h1 { color: #007acc; margin: 0; }
        .cheat-sheet-grid { display: grid; grid-template-columns: repeat(auto-fit, minmax(300px, 1fr)); gap: 20px; margin-top: 30px; }
        .cheat-sheet-card { background: #f8f9fa; padding: 20px; border-radius: 8px; border-left: 4px solid #007acc; }
        .cheat-sheet-card h3 { margin-top: 0; color: #333; }
        .cheat-sheet-card p { color: #666; margin: 10px 0; }
        .cheat-sheet-card a { color: #007acc; text-decoration: none; font-weight: bold; }
        .cheat-sheet-card a:hover { text-decoration: underline; }
        .back-link { margin-top: 30px; padding-top: 20px; border-top: 1px solid #eee; }
        .back-link a { color: #007acc; text-decoration: none; }
        .back-link a:hover { text-decoration: underline; }
    </style>
</head>
<body>
    <div class="container">
        <div class="header">
            <h1>📚 CppVerseHub Cheat Sheets</h1>
            <p>Quick reference guides for mastering C++ concepts</p>
        </div>
        
        <div class="cheat-sheet-grid">
            <div class="cheat-sheet-card">
                <h3>🏗️ Object-Oriented Programming</h3>
                <p>Classes, inheritance, polymorphism, and encapsulation fundamentals</p>
                <a href="OOP_CheatSheet.md">View Cheat Sheet →</a>
            </div>
            
            <div class="cheat-sheet-card">
                <h3>📦 Standard Template Library</h3>
                <p>Containers, algorithms, iterators, and STL best practices</p>
                <a href="STL_CheatSheet.md">View Cheat Sheet →</a>
            </div>
            
            <div class="cheat-sheet-card">
                <h3>🔧 Template Programming</h3>
                <p>Generic programming, template specialization, and metaprogramming</p>
                <a href="Templates_CheatSheet.md">View Cheat Sheet →</a>
            </div>
            
            <div class="cheat-sheet-card">
                <h3>🚀 Concurrency & Threading</h3>
                <p>Multithreading, synchronization, async programming, and parallel algorithms</p>
                <a href="Concurrency_CheatSheet.md">View Cheat Sheet →</a>
            </div>
            
            <div class="cheat-sheet-card">
                <h3>✨ Modern C++ Features</h3>
                <p>C++17/20/23 features, concepts, ranges, and modules</p>
                <a href="ModernCpp_CheatSheet.md">View Cheat Sheet →</a>
            </div>
            
            <div class="cheat-sheet-card">
                <h3>🎨 Design Patterns</h3>
                <p>Common design patterns implementations and usage guidelines</p>
                <a href="DesignPatterns_CheatSheet.md">View Cheat Sheet →</a>
            </div>
            
            <div class="cheat-sheet-card">
                <h3>🧠 Memory Management</h3>
                <p>Smart pointers, RAII, custom allocators, and memory optimization</p>
                <a href="MemoryManagement_CheatSheet.md">View Cheat Sheet →</a>
            </div>
            
            <div class="cheat-sheet-card">
                <h3>⚠️ Exception Handling</h3>
                <p>Exception safety, custom exceptions, and error handling patterns</p>
                <a href="Exceptions_CheatSheet.md">View Cheat Sheet →</a>
            </div>
        </div>
        
        <div class="back-link">
            <a href="../index.html">🏠 Back to Documentation Home</a>
        </div>
    </div>
</body>
</html>
EOF
    
    print_debug "Generated cheat sheet index: $index_file"
}

# Function to generate tutorials
generate_tutorials() {
    if [ "$GENERATE_TUTORIALS" != true ]; then
        return 0
    fi
    
    print_docs "Generating interactive tutorials..."
    
    # Create tutorial structure
    mkdir -p "$OUTPUT_DIR/tutorials"
    
    # Generate beginner tutorial
    generate_beginner_tutorial
    
    # Generate advanced tutorial
    generate_advanced_tutorial
    
    # Generate project walkthrough
    generate_project_walkthrough
    
    print_success "Interactive tutorials generated"
}

# Function to generate beginner tutorial
generate_beginner_tutorial() {
    local tutorial_file="$OUTPUT_DIR/tutorials/beginner_guide.html"
    
    cat > "$tutorial_file" << 'EOF'
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>CppVerseHub - Beginner's Guide</title>
    <style>
        body { font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif; line-height: 1.6; margin: 0; padding: 20px; background: #f5f5f5; }
        .container { max-width: 1000px; margin: 0 auto; background: white; padding: 30px; border-radius: 8px; box-shadow: 0 2px 10px rgba(0,0,0,0.1); }
        .header { text-align: center; border-bottom: 2px solid #007acc; padding-bottom: 20px; margin-bottom: 30px; }
        h1 { color: #007acc; margin: 0; }
        h2 { color: #333; border-left: 4px solid #007acc; padding-left: 15px; }
        .step { background: #f8f9fa; padding: 20px; margin: 20px 0; border-radius: 8px; border-left: 4px solid #28a745; }
        .step h3 { margin-top: 0; color: #28a745; }
        code { background: #e9ecef; padding: 2px 4px; border-radius: 3px; font-family: 'Courier New', monospace; }
        pre { background: #2d3748; color: #e2e8f0; padding: 15px; border-radius: 8px; overflow-x: auto; }
        .navigation { text-align: center; margin-top: 30px; padding-top: 20px; border-top: 1px solid #eee; }
        .nav-button { display: inline-block; background: #007acc; color: white; padding: 10px 20px; margin: 0 10px; text-decoration: none; border-radius: 5px; }
        .nav-button:hover { background: #005a9e; }
        .tip { background: #fff3cd; border: 1px solid #ffeaa7; padding: 15px; border-radius: 8px; margin: 15px 0; }
        .tip::before { content: "💡 "; font-size: 18px; }
    </style>
</head>
<body>
    <div class="container">
        <div class="header">
            <h1>🚀 CppVerseHub Beginner's Guide</h1>
            <p>Your journey into C++ mastery starts here!</p>
        </div>
        
        <h2>Welcome to CppVerseHub!</h2>
        <p>CppVerseHub is a comprehensive C++ learning platform that demonstrates advanced concepts through a space exploration simulation. This guide will help you get started.</p>
        
        <div class="step">
            <h3>Step 1: Understanding the Project Structure</h3>
            <p>CppVerseHub is organized into several key modules:</p>
            <ul>
                <li><strong>Core</strong> - Basic OOP concepts and game entities</li>
                <li><strong>Patterns</strong> - Design pattern implementations</li>
                <li><strong>STL Showcase</strong> - Standard library demonstrations</li>
                <li><strong>Templates</strong> - Generic programming examples</li>
                <li><strong>Concurrency</strong> - Multithreading concepts</li>
                <li><strong>Modern</strong> - C++17/20/23 features</li>
            </ul>
        </div>
        
        <div class="step">
            <h3>Step 2: Building the Project</h3>
            <p>First, let's build CppVerseHub:</p>
            <pre><code># Clone and navigate to the project
cd CppVerseHub

# Build in release mode
./scripts/build.sh

# Or build in debug mode for learning
./scripts/build.sh --debug</code></pre>
            
            <div class="tip">
                Use debug mode when learning to get better error messages and debugging information.
            </div>
        </div>
        
        <div class="step">
            <h3>Step 3: Running Your First Simulation</h3>
            <p>Let's run a basic space exploration simulation:</p>
            <pre><code># Run the main application
./build/release/src/CppVerseHub

# Or with specific parameters
./build/release/src/CppVerseHub --planets 5 --fleets 3</code></pre>
        </div>
        
        <div class="step">
            <h3>Step 4: Exploring the Code</h3>
            <p>Start by examining these key files:</p>
            <ul>
                <li><code>src/core/Entity.hpp</code> - Base class demonstrating inheritance</li>
                <li><code>src/core/Mission.hpp</code> - Polymorphism in action</li>
                <li><code>src/patterns/Singleton.hpp</code> - Design pattern example</li>
                <li><code>src/stl_showcase/Containers.hpp</code> - STL usage</li>
            </ul>
        </div>
        
        <div class="step">
            <h3>Step 5: Understanding Key Concepts</h3>
            <p>CppVerseHub demonstrates these fundamental C++ concepts:</p>
            
            <h4>Object-Oriented Programming</h4>
            <pre><code>class Entity {
public:
    virtual ~Entity() = default;
    virtual void update() = 0;  // Pure virtual function
protected:
    std::string name_;
    Position position_;
};</code></pre>
            
            <h4>Polymorphism</h4>
            <pre><code>// Different mission types, same interface
std::unique_ptr&lt;Mission&gt; mission = 
    std::make_unique&lt;ExplorationMission&gt;();
mission-&gt;execute();  // Calls ExplorationMission::execute()</code></pre>
            
            <div class="tip">
                Notice how we can treat different mission types uniformly through the base Mission interface.
            </div>
        </div>
        
        <div class="step">
            <h3>Step 6: Running Tests</h3>
            <p>Verify your understanding by running the test suite:</p>
            <pre><code># Run all tests
./scripts/run_tests.sh

# Run specific test categories
./scripts/run_tests.sh --unit-only
./scripts/run_tests.sh --coverage</code></pre>
        </div>
        
        <div class="step">
            <h3>Step 7: Next Steps</h3>
            <p>Ready to dive deeper? Here's what to explore next:</p>
            <ul>
                <li>📚 <a href="../cheat_sheets/">Study the cheat sheets</a></li>
                <li>🔧 <a href="advanced_guide.html">Try the advanced tutorial</a></li>
                <li>🎨 <a href="../api/html/index.html">Browse the API documentation</a></li>
                <li>🏗️ <a href="project_walkthrough.html">Take the project walkthrough</a></li>
            </ul>
        </div>
        
        <h2>Learning Path Recommendations</h2>
        <div class="tip">
            <strong>For Beginners:</strong> Start with core concepts → STL → Design Patterns → Modern C++<br>
            <strong>For Intermediate:</strong> Focus on Templates → Concurrency → Advanced Patterns<br>
            <strong>For Advanced:</strong> Study Modern C++ → Performance Optimization → Architecture Design
        </div>
        
        <div class="navigation">
            <a href="../index.html" class="nav-button">🏠 Documentation Home</a>
            <a href="advanced_guide.html" class="nav-button">➡️ Advanced Guide</a>
            <a href="../api/html/index.html" class="nav-button">📖 API Docs</a>
        </div>
    </div>
</body>
</html>
EOF
    
    print_debug "Generated beginner tutorial: $tutorial_file"
}

# Function to generate advanced tutorial
generate_advanced_tutorial() {
    local tutorial_file="$OUTPUT_DIR/tutorials/advanced_guide.html"
    
    cat > "$tutorial_file" << 'EOF'
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>CppVerseHub - Advanced Guide</title>
    <style>
        body { font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif; line-height: 1.6; margin: 0; padding: 20px; background: #f5f5f5; }
        .container { max-width: 1200px; margin: 0 auto; background: white; padding: 30px; border-radius: 8px; box-shadow: 0 2px 10px rgba(0,0,0,0.1); }
        .header { text-align: center; border-bottom: 2px solid #007acc; padding-bottom: 20px; margin-bottom: 30px; }
        h1 { color: #007acc; margin: 0; }
        h2 { color: #333; border-left: 4px solid #007acc; padding-left: 15px; }
        .advanced-topic { background: linear-gradient(135deg, #667eea 0%, #764ba2 100%); color: white; padding: 20px; margin: 20px 0; border-radius: 8px; }
        .advanced-topic h3 { margin-top: 0; }
        code { background: #e9ecef; padding: 2px 4px; border-radius: 3px; font-family: 'Courier New', monospace; color: #333; }
        pre { background: #2d3748; color: #e2e8f0; padding: 15px; border-radius: 8px; overflow-x: auto; }
        .performance-tip { background: #d4edda; border: 1px solid #c3e6cb; padding: 15px; border-radius: 8px; margin: 15px 0; }
        .performance-tip::before { content: "⚡ "; font-size: 18px; }
        .warning { background: #f8d7da; border: 1px solid #f5c6cb; padding: 15px; border-radius: 8px; margin: 15px 0; }
        .warning::before { content: "⚠️ "; font-size: 18px; }
    </style>
</head>
<body>
    <div class="container">
        <div class="header">
            <h1>🧠 CppVerseHub Advanced Guide</h1>
            <p>Master advanced C++ techniques and best practices</p>
        </div>
        
        <div class="advanced-topic">
            <h3>Template Metaprogramming</h3>
            <p>Explore compile-time computations and type manipulation techniques used throughout CppVerseHub.</p>
            <pre><code>template&lt;typename T, size_t N&gt;
constexpr size_t factorial() {
    if constexpr (N == 0) {
        return 1;
    } else {
        return N * factorial&lt;T, N-1&gt;();
    }
}</code></pre>
        </div>
        
        <div class="advanced-topic">
            <h3>Modern Memory Management</h3>
            <p>Learn sophisticated RAII patterns and custom allocators for optimal performance.</p>
            <pre><code>class ResourcePool {
    std::vector&lt;std::unique_ptr&lt;Resource&gt;&gt; pool_;
    std::queue&lt;Resource*&gt; available_;
    std::mutex pool_mutex_;
    
public:
    auto acquire() -&gt; std::unique_ptr&lt;Resource, std::function&lt;void(Resource*)&gt;&gt;;
};</code></pre>
        </div>
        
        <div class="performance-tip">
            Study the memory pool implementations in <code>src/memory/MemoryPools.hpp</code> to understand how CppVerseHub optimizes allocation patterns.
        </div>
        
        <div class="navigation">
            <a href="beginner_guide.html" class="nav-button">⬅️ Beginner Guide</a>
            <a href="project_walkthrough.html" class="nav-button">➡️ Project Walkthrough</a>
        </div>
    </div>
</body>
</html>
EOF
    
    print_debug "Generated advanced tutorial: $tutorial_file"
}

# Function to generate project walkthrough
generate_project_walkthrough() {
    local tutorial_file="$OUTPUT_DIR/tutorials/project_walkthrough.html"
    
    # Generate a comprehensive project walkthrough
    cat > "$tutorial_file" << 'EOF'
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>CppVerseHub - Project Walkthrough</title>
    <style>
        body { font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif; line-height: 1.6; margin: 0; padding: 20px; background: #f5f5f5; }
        .container { max-width: 1200px; margin: 0 auto; background: white; padding: 30px; border-radius: 8px; box-shadow: 0 2px 10px rgba(0,0,0,0.1); }
        .header { text-align: center; border-bottom: 2px solid #007acc; padding-bottom: 20px; margin-bottom: 30px; }
        h1 { color: #007acc; margin: 0; }
        .module-section { background: #f8f9fa; padding: 20px; margin: 20px 0; border-radius: 8px; border-left: 4px solid #007acc; }
        .file-structure { background: #2d3748; color: #e2e8f0; padding: 15px; border-radius: 8px; font-family: monospace; }
    </style>
</head>
<body>
    <div class="container">
        <div class="header">
            <h1>🏗️ CppVerseHub Project Walkthrough</h1>
            <p>Deep dive into the architecture and implementation</p>
        </div>
        
        <div class="module-section">
            <h2>🎯 Project Architecture</h2>
            <p>CppVerseHub follows a layered architecture with clear separation of concerns:</p>
            <div class="file-structure">
src/
├── core/           # Domain entities and business logic
├── patterns/       # Design pattern implementations  
├── stl_showcase/   # STL demonstrations
├── templates/      # Generic programming
├── concurrency/    # Threading and async
├── modern/         # C++17/20/23 features
├── memory/         # Advanced memory management
├── algorithms/     # Custom algorithm implementations
└── utils/          # Cross-cutting utilities
            </div>
        </div>
        
        <p><em>Continue exploring the comprehensive project structure and learn how each component contributes to the overall learning experience...</em></p>
        
        <div class="navigation">
            <a href="advanced_guide.html" class="nav-button">⬅️ Advanced Guide</a>
            <a href="../index.html" class="nav-button">🏠 Documentation Home</a>
        </div>
    </div>
</body>
</html>
EOF
    
    print_debug "Generated project walkthrough: $tutorial_file"
}

# Function to generate main documentation index
generate_main_index() {
    print_docs "Generating main documentation index..."
    
    local index_file="$OUTPUT_DIR/index.html"
    local timestamp=$(date "+%Y-%m-%d %H:%M:%S")
    
    cat > "$index_file" << EOF
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>CppVerseHub - Complete C++ Learning Platform</title>
    <style>
        * { margin: 0; padding: 0; box-sizing: border-box; }
        body { font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif; line-height: 1.6; background: linear-gradient(135deg, #667eea 0%, #764ba2 100%); min-height: 100vh; }
        .container { max-width: 1200px; margin: 0 auto; padding: 20px; }
        .header { text-align: center; color: white; padding: 40px 0; }
        .header h1 { font-size: 3em; margin-bottom: 10px; text-shadow: 2px 2px 4px rgba(0,0,0,0.5); }
        .header p { font-size: 1.2em; opacity: 0.9; }
        .content { background: white; border-radius: 15px; padding: 40px; margin: 20px 0; box-shadow: 0 10px 30px rgba(0,0,0,0.3); }
        .section-grid { display: grid; grid-template-columns: repeat(auto-fit, minmax(300px, 1fr)); gap: 25px; margin: 30px 0; }
        .section-card { background: linear-gradient(135deg, #f5f7fa 0%, #c3cfe2 100%); padding: 25px; border-radius: 10px; transition: transform 0.3s ease, box-shadow 0.3s ease; }
        .section-card:hover { transform: translateY(-5px); box-shadow: 0 10px 25px rgba(0,0,0,0.15); }
        .section-card h3 { color: #333; margin-bottom: 15px; font-size: 1.4em; }
        .section-card p { color: #666; margin-bottom: 15px; }
        .section-card a { display: inline-block; background: #007acc; color: white; padding: 10px 20px; text-decoration: none; border-radius: 5px; transition: background 0.3s ease; }
        .section-card a:hover { background: #005a9e; }
        .quick-start { background: linear-gradient(135deg, #11998e 0%, #38ef7d 100%); color: white; padding: 30px; border-radius: 10px; margin: 30px 0; }
        .quick-start h2 { margin-bottom: 15px; }
        .quick-start pre { background: rgba(0,0,0,0.2); padding: 15px; border-radius: 8px; overflow-x: auto; }
        .stats { display: grid; grid-template-columns: repeat(auto-fit, minmax(150px, 1fr)); gap: 20px; margin: 30px 0; }
        .stat { text-align: center; padding: 20px; background: #f8f9fa; border-radius: 8px; }
        .stat-number { font-size: 2em; font-weight: bold; color: #007acc; }
        .stat-label { color: #666; text-transform: uppercase; font-size: 0.9em; }
        .footer { text-align: center; color: white; padding: 20px 0; opacity: 0.8; }
    </style>
</head>
<body>
    <div class="container">
        <div class="header">
            <h1>🚀 CppVerseHub</h1>
            <p>Your Complete C++ Learning and Mastery Platform</p>
            <p><em>Generated on $timestamp</em></p>
        </div>
        
        <div class="content">
            <h2>Welcome to the Future of C++ Learning</h2>
            <p>CppVerseHub is a comprehensive, hands-on learning platform that teaches advanced C++ concepts through an engaging space exploration simulation. Master everything from basic OOP to cutting-edge C++23 features.</p>
            
            <div class="stats">
                <div class="stat">
                    <div class="stat-number">15+</div>
                    <div class="stat-label">Core Modules</div>
                </div>
                <div class="stat">
                    <div class="stat-number">50+</div>
                    <div class="stat-label">Code Examples</div>
                </div>
                <div class="stat">
                    <div class="stat-number">200+</div>
                    <div class="stat-label">Test Cases</div>
                </div>
                <div class="stat">
                    <div class="stat-number">8</div>
                    <div class="stat-label">Design Patterns</div>
                </div>
            </div>
            
            <div class="quick-start">
                <h2>🏃‍♂️ Quick Start</h2>
                <p>Get up and running in minutes:</p>
                <pre><code># Build the project
./scripts/build.sh

# Run tests
./scripts/run_tests.sh

# Start learning!
./build/release/src/CppVerseHub</code></pre>
            </div>
            
            <div class="section-grid">
                <div class="section-card">
                    <h3>📖 API Documentation</h3>
                    <p>Complete API reference generated from source code with examples and diagrams.</p>
                    <a href="api/html/index.html">Browse API Docs →</a>
                </div>
                
                <div class="section-card">
                    <h3>📚 Cheat Sheets</h3>
                    <p>Quick reference guides covering all major C++ concepts and best practices.</p>
                    <a href="cheat_sheets/">View Cheat Sheets →</a>
                </div>
                
                <div class="section-card">
                    <h3>🎓 Interactive Tutorials</h3>
                    <p>Step-by-step guides from beginner to advanced C++ programming.</p>
                    <a href="tutorials/beginner_guide.html">Start Learning →</a>
                </div>
                
                <div class="section-card">
                    <h3>🔧 UML Diagrams</h3>
                    <p>Visual representations of system architecture and design patterns.</p>
                    <a href="diagrams/">View Diagrams →</a>
                </div>
                
                <div class="section-card">
                    <h3>🏗️ Architecture Guide</h3>
                    <p>Deep dive into the project structure and design decisions.</p>
                    <a href="tutorials/project_walkthrough.html">Explore Architecture →</a>
                </div>
                
                <div class="section-card">
                    <h3>🧪 Test Reports</h3>
                    <p>Comprehensive testing results including coverage analysis.</p>
                    <a href="../test_results/test_report.html">View Test Reports →</a>
                </div>
            </div>
            
            <h2>🎯 Learning Path</h2>
            <div class="section-grid">
                <div class="section-card">
                    <h3>👶 Beginner Path</h3>
                    <p>Start with core concepts → STL → Basic Patterns</p>
                    <a href="tutorials/beginner_guide.html">Begin Journey →</a>
                </div>
                
                <div class="section-card">
                    <h3>🎯 Intermediate Path</h3>
                    <p>Templates → Concurrency → Advanced Patterns</p>
                    <a href="tutorials/advanced_guide.html">Level Up →</a>
                </div>
                
                <div class="section-card">
                    <h3>🚀 Expert Path</h3>
                    <p>Modern C++ → Performance → Architecture</p>
                    <a href="tutorials/project_walkthrough.html">Master C++ →</a>
                </div>
            </div>
            
            <h2>🌟 Key Features</h2>
            <ul style="font-size: 1.1em; line-height: 2;">
                <li>✨ <strong>Modern C++20/23</strong> - Latest language features and best practices</li>
                <li>🎨 <strong>Design Patterns</strong> - Real-world implementations of common patterns</li>
                <li>🧵 <strong>Concurrency</strong> - Multithreading and asynchronous programming</li>
                <li>📦 <strong>STL Mastery</strong> - Complete Standard Template Library coverage</li>
                <li>🔧 <strong>Templates</strong> - Generic programming and metaprogramming</li>
                <li>🧠 <strong>Memory Management</strong> - Smart pointers, RAII, and custom allocators</li>
                <li>⚡ <strong>Performance</strong> - Optimization techniques and benchmarking</li>
                <li>🧪 <strong>Testing</strong> - Comprehensive test suite with coverage analysis</li>
            </ul>
        </div>
        
        <div class="footer">
            <p>🌌 CppVerseHub - Explore the Universe of C++ Programming</p>
            <p>Built with ❤️ for the C++ community</p>
        </div>
    </div>
</body>
</html>
EOF
    
    print_success "Main documentation index generated: $index_file"
}

# Function to open browser if requested
open_documentation() {
    if [ "$OPEN_BROWSER" != true ]; then
        return 0
    fi
    
    local index_file="$OUTPUT_DIR/index.html"
    
    if [ -f "$index_file" ]; then
        print_info "Opening documentation in browser..."
        
        # Try different browsers and commands
        if command -v xdg-open >/dev/null 2>&1; then
            xdg-open "$index_file" >/dev/null 2>&1 &
        elif command -v open >/dev/null 2>&1; then
            open "$index_file" >/dev/null 2>&1 &
        elif command -v start >/dev/null 2>&1; then
            start "$index_file" >/dev/null 2>&1 &
        else
            print_warning "Could not open browser automatically"
            print_info "Please open: file://$index_file"
        fi
    fi
}

# Function to show generation summary
show_generation_summary() {
    local total_time="$1"
    
    print_success "Documentation Generation Summary"
    echo "================================="
    echo "Total Generation Time: ${total_time}s"
    echo "API Documentation:     $GENERATE_API_DOCS"
    echo "UML Diagrams:         $GENERATE_UML_DIAGRAMS"
    echo "Cheat Sheets:         $GENERATE_CHEAT_SHEETS"
    echo "Tutorials:            $GENERATE_TUTORIALS"
    echo "PDF Generation:       $GENERATE_PDF"
    echo ""
    echo "Output Location: $OUTPUT_DIR"
    echo "Main Index:      $OUTPUT_DIR/index.html"
    
    if [ "$GENERATE_API_DOCS" = true ]; then
        echo "API Docs:        $DOXYGEN_OUTPUT_DIR/html/index.html"
    fi
    
    echo ""
    echo "Generated Files:"
    find "$OUTPUT_DIR" -type f | wc -l | xargs echo "  Total Files:"
    find "$OUTPUT_DIR" -name "*.html" | wc -l | xargs echo "  HTML Files: "
    find "$OUTPUT_DIR" -name "*.svg" | wc -l | xargs echo "  SVG Diagrams:"
    
    if [ -f "$OUTPUT_DIR/CppVerseHub_API_Documentation.pdf" ]; then
        echo "  PDF Documentation: Available"
    fi
    
    echo ""
    print_success "Documentation ready! 📚✨"
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
                CLEAN_OUTPUT=true
                shift
                ;;
            -v|--verbose)
                VERBOSE=true
                shift
                ;;
            --no-api)
                GENERATE_API_DOCS=false
                shift
                ;;
            --no-diagrams)
                GENERATE_UML_DIAGRAMS=false
                shift
                ;;
            --no-cheatsheets)
                GENERATE_CHEAT_SHEETS=false
                shift
                ;;
            --no-tutorials)
                GENERATE_TUTORIALS=false
                shift
                ;;
            --pdf)
                GENERATE_PDF=true
                shift
                ;;
            --no-browser)
                OPEN_BROWSER=false
                shift
                ;;
            --no-theme)
                CUSTOM_THEME=false
                shift
                ;;
            --include-private)
                INCLUDE_PRIVATE=true
                shift
                ;;
            --no-source)
                INCLUDE_SOURCE=false
                shift
                ;;
            --output=*)
                OUTPUT_DIR="${1#*=}"
                DOXYGEN_OUTPUT_DIR="$OUTPUT_DIR/api"
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

# Main documentation generation function
main() {
    local start_time=$(date +%s)
    
    print_info "Starting CppVerseHub documentation generation..."
    print_info "Project root: $PROJECT_ROOT"
    
    # Parse command line arguments
    parse_arguments "$@"
    
    # Check dependencies and setup
    check_doc_dependencies
    setup_output_directories
    
    # Generate different types of documentation
    generate_api_documentation
    generate_uml_diagrams
    generate_cheat_sheets
    generate_tutorials
    
    # Generate main index page
    generate_main_index
    
    # Open documentation in browser
    open_documentation
    
    local end_time=$(date +%s)
    local total_time=$((end_time - start_time))
    
    # Show summary
    show_generation_summary "$total_time"
    
    print_success "CppVerseHub documentation generated successfully! 🚀"
}

# Ensure we're in the right directory
cd "$PROJECT_ROOT"

# Run main function with all arguments
main "$@"