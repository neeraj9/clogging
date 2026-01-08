#!/bin/bash
# CMake Build and Test Script for clogging
# This script provides a convenient way to build and test the library

set -e  # Exit on error

# Color codes
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Defaults
BUILD_DIR="build"
INSTALL_PREFIX=""
BUILD_TYPE="Release"
BUILD_TESTS=ON
PARALLEL_JOBS=4

# Functions
print_usage() {
    cat << EOF
Usage: $0 [OPTIONS]

Options:
    -h, --help              Show this help message
    -p, --prefix PREFIX     Installation prefix (default: /usr/local)
    -b, --build-dir DIR     Build directory (default: build)
    -t, --type TYPE         Build type: Debug or Release (default: Release)
    --no-tests              Disable building tests
    -j, --jobs N            Number of parallel jobs (default: 4)
    -c, --clean             Clean build directory before configuring
    --install               Install after building (requires sudo or proper permissions)
    --test-only             Only run tests (skip configuration and build)
    --config-only           Only configure, don't build

Examples:
    # Basic build
    $0

    # Install to /opt/clogging
    $0 --prefix /opt/clogging --install

    # Debug build with tests
    $0 --type Debug

    # Clean build
    $0 --clean

    # Run tests only
    $0 --test-only

EOF
    exit 0
}

# Parse arguments
CLEAN=0
INSTALL=0
CONFIG_ONLY=0
TEST_ONLY=0

while [[ $# -gt 0 ]]; do
    case $1 in
        -h|--help)
            print_usage
            ;;
        -p|--prefix)
            INSTALL_PREFIX="$2"
            shift 2
            ;;
        -b|--build-dir)
            BUILD_DIR="$2"
            shift 2
            ;;
        -t|--type)
            BUILD_TYPE="$2"
            shift 2
            ;;
        --no-tests)
            BUILD_TESTS=OFF
            shift
            ;;
        -j|--jobs)
            PARALLEL_JOBS="$2"
            shift 2
            ;;
        -c|--clean)
            CLEAN=1
            shift
            ;;
        --install)
            INSTALL=1
            shift
            ;;
        --test-only)
            TEST_ONLY=1
            shift
            ;;
        --config-only)
            CONFIG_ONLY=1
            shift
            ;;
        *)
            echo "Unknown option: $1"
            print_usage
            ;;
    esac
done

# Check if cmake is available
if ! command -v cmake &> /dev/null; then
    echo -e "${RED}Error: cmake is not installed${NC}"
    echo "Please install cmake 3.10 or later"
    exit 1
fi

echo -e "${GREEN}=== clogging CMake Build Script ===${NC}"
echo ""

# Get CMake version
CMAKE_VERSION=$(cmake --version | head -1)
echo "Using: $CMAKE_VERSION"
echo ""

# Test only mode
if [ $TEST_ONLY -eq 1 ]; then
    if [ ! -d "$BUILD_DIR" ]; then
        echo -e "${RED}Error: Build directory '$BUILD_DIR' not found${NC}"
        echo "Please run build first: $0"
        exit 1
    fi
    
    echo -e "${YELLOW}Running tests...${NC}"
    cd "$BUILD_DIR"
    ctest --output-on-failure
    cd ..
    echo -e "${GREEN}Tests completed${NC}"
    exit 0
fi

# Clean if requested
if [ $CLEAN -eq 1 ]; then
    echo -e "${YELLOW}Cleaning build directory...${NC}"
    rm -rf "$BUILD_DIR"
fi

# Create build directory
echo -e "${YELLOW}Creating build directory: $BUILD_DIR${NC}"
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# Configure
echo ""
echo -e "${YELLOW}Configuring project...${NC}"
CMAKE_ARGS="-DCMAKE_BUILD_TYPE=$BUILD_TYPE -DBUILD_TESTS=$BUILD_TESTS"

if [ -n "$INSTALL_PREFIX" ]; then
    CMAKE_ARGS="$CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=$INSTALL_PREFIX"
fi

if cmake .. $CMAKE_ARGS; then
    echo -e "${GREEN}Configuration successful${NC}"
else
    echo -e "${RED}Configuration failed${NC}"
    exit 1
fi

# Check if config-only was requested
if [ $CONFIG_ONLY -eq 1 ]; then
    echo -e "${GREEN}Configuration complete. Build files written to $BUILD_DIR/${NC}"
    exit 0
fi

echo ""

# Build
echo -e "${YELLOW}Building project...${NC}"
if cmake --build . --parallel "$PARALLEL_JOBS"; then
    echo -e "${GREEN}Build successful${NC}"
else
    echo -e "${RED}Build failed${NC}"
    exit 1
fi

echo ""

# Run tests
if [ "$BUILD_TESTS" = "ON" ]; then
    echo -e "${YELLOW}Running tests...${NC}"
    if ctest --output-on-failure; then
        echo -e "${GREEN}All tests passed${NC}"
    else
        echo -e "${RED}Some tests failed${NC}"
        exit 1
    fi
    echo ""
fi

# Install
if [ $INSTALL -eq 1 ]; then
    echo -e "${YELLOW}Installing...${NC}"
    if cmake --install .; then
        echo -e "${GREEN}Installation successful${NC}"
        if [ -n "$INSTALL_PREFIX" ]; then
            echo "Installed to: $INSTALL_PREFIX"
        fi
    else
        echo -e "${RED}Installation failed${NC}"
        exit 1
    fi
fi

cd ..

echo ""
echo -e "${GREEN}=== Build Complete ===${NC}"
echo ""
echo "Next steps:"
if [ $INSTALL -eq 0 ]; then
    echo "  1. Review the build: cmake --build $BUILD_DIR"
    echo "  2. Install the library:"
    if [ -n "$INSTALL_PREFIX" ]; then
        echo "     cmake --install $BUILD_DIR --prefix $INSTALL_PREFIX"
    else
        echo "     sudo cmake --install $BUILD_DIR"
    fi
fi
echo "  3. Use in your project:"
echo "     find_package(clogging REQUIRED)"
echo "     target_link_libraries(myapp PRIVATE clogging::clogging)"
echo ""
echo "For more information, see CMAKE_GUIDE.md or CMAKE_INSTALL_GUIDE.md"
