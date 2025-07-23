#!/bin/bash

# Snes9x EX Plus ARM64 Optimized Build Script
# Usage: ./build-snes9x-arm64.sh [clean|debug]

set -e

# Color codes for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Configuration
NDK_VERSION=${NDK_VERSION:-"r27-beta2"}
BUILD_TYPE=${1:-"release"}
CLEAN_BUILD=${2:-"false"}

# Android SDK setup
export ANDROID_HOME=${ANDROID_HOME:-"/workspace/android-sdk"}
export PATH="$ANDROID_HOME/cmdline-tools/latest/bin:$ANDROID_HOME/platform-tools:$PATH"

# Paths
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
WORKSPACE_DIR="$SCRIPT_DIR"
NDK_PATH="$WORKSPACE_DIR/android-ndk-$NDK_VERSION"
IMAGINE_PATH="$WORKSPACE_DIR/imagine"
EMUFRAMEWORK_PATH="$WORKSPACE_DIR/EmuFramework"
IMAGINE_SDK_PATH="$WORKSPACE_DIR/imagine-sdk"
SNES9X_PATH="$WORKSPACE_DIR/Snes9x"

# Functions
print_header() {
    echo -e "${BLUE}================================${NC}"
    echo -e "${BLUE} Snes9x EX Plus ARM64 Builder ${NC}"
    echo -e "${BLUE}================================${NC}"
}

print_step() {
    echo -e "${GREEN}[STEP]${NC} $1"
}

print_info() {
    echo -e "${YELLOW}[INFO]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

check_dependencies() {
    print_step "Checking dependencies..."
    
    local missing_deps=()
    
    for cmd in make cmake autoconf automake libtool pkg-config wget unzip; do
        if ! command -v $cmd &> /dev/null; then
            missing_deps+=($cmd)
        fi
    done
    
    if [ ${#missing_deps[@]} -ne 0 ]; then
        print_error "Missing dependencies: ${missing_deps[*]}"
        print_info "Install them with: sudo apt-get install ${missing_deps[*]}"
        exit 1
    fi
    
    print_info "All dependencies found ✓"
}

setup_environment() {
    print_step "Setting up build environment..."
    
    export ANDROID_NDK_PATH="$NDK_PATH"
    export EMUFRAMEWORK_PATH="$EMUFRAMEWORK_PATH"
    export IMAGINE_PATH="$IMAGINE_PATH"
    export IMAGINE_SDK_PATH="$IMAGINE_SDK_PATH"
    export android_arch=arm64
    export ANDROID_ABI=arm64-v8a
    
    # Build optimizations
    export MAKEFLAGS="-j$(nproc)"
    export CFLAGS="-O3 -march=armv8-a -mtune=cortex-a73 -flto"
    export CXXFLAGS="-O3 -march=armv8-a -mtune=cortex-a73 -flto"
    export LDFLAGS="-flto -Wl,--gc-sections"
    
    print_info "Environment configured for ARM64 build"
}

download_ndk() {
    if [ ! -d "$NDK_PATH" ]; then
        print_step "Downloading Android NDK $NDK_VERSION..."
        
        local ndk_url="https://dl.google.com/android/repository/android-ndk-$NDK_VERSION-linux.zip"
        local ndk_zip="android-ndk-$NDK_VERSION-linux.zip"
        
        wget -q --show-progress "$ndk_url" -O "$ndk_zip"
        unzip -q "$ndk_zip"
        rm "$ndk_zip"
        
        print_info "Android NDK downloaded and extracted"
    else
        print_info "Android NDK already exists"
    fi
}

build_imagine_sdk() {
    print_step "Building Imagine SDK (ARM64 only)..."
    
    mkdir -p "$IMAGINE_SDK_PATH"
    cd "$IMAGINE_PATH/bundle/all"
    
    if [ ! -x "./makeAll-android-arm64.sh" ]; then
        print_error "makeAll-android-arm64.sh not found or not executable"
        exit 1
    fi
    
    chmod +x ./runMakefiles.sh
    ./makeAll-android-arm64.sh install -j$(nproc)
    
    print_info "Imagine SDK built successfully"
}

build_emuframework() {
    print_step "Building EmuFramework (ARM64)..."
    
    cd "$EMUFRAMEWORK_PATH"
    
    make -f "../imagine/make/shortcut/common-builds/android-arm64-release.mk" install V=1 -j$(nproc)
    
    print_info "EmuFramework built successfully"
}

build_snes9x() {
    print_step "Building Snes9x EX Plus APK (ARM64)..."
    
    cd "$SNES9X_PATH"
    
    # Clean build if requested
    if [ "$CLEAN_BUILD" = "true" ] || [ "$1" = "clean" ]; then
        print_info "Performing clean build..."
        make -f android-release.mk clean android_arch=arm64 || true
        rm -rf target/ || true
    fi
    
    # Select makefile based on build type
    local makefile="android-release.mk"
    if [ -f "android-arm64-optimized.mk" ]; then
        makefile="android-arm64-optimized.mk"
        print_info "Using optimized ARM64 makefile"
    fi
    
    # Build APK using common-builds makefile
    local build_cmd="make -f ../imagine/make/shortcut/common-builds/android-arm64-release.mk android-apk V=1 -j$(nproc)"
    
    if [ "$BUILD_TYPE" = "debug" ]; then
        build_cmd="$build_cmd config=debug"
        print_info "Building debug APK..."
    else
        print_info "Building release APK..."
    fi
    
    eval $build_cmd
    
    # Verify and copy APK - check multiple possible paths
    local apk_paths=(
        "target/android-arm64-release/build/outputs/apk/release/Snes9xEXPlus-release.apk"
        "target/android-release/build/outputs/apk/release/Snes9xEXPlus-release.apk"
        "build/outputs/apk/release/Snes9xEXPlus-release.apk"
    )
    
    if [ "$BUILD_TYPE" = "debug" ]; then
        apk_paths=(
            "target/android-arm64-release/build/outputs/apk/debug/Snes9xEXPlus-debug.apk"
            "target/android-release/build/outputs/apk/debug/Snes9xEXPlus-debug.apk"
            "build/outputs/apk/debug/Snes9xEXPlus-debug.apk"
        )
    fi
    
    local apk_path=""
    for path in "${apk_paths[@]}"; do
        if [ -f "$path" ]; then
            apk_path="$path"
            break
        fi
    done
    
    if [ -n "$apk_path" ] && [ -f "$apk_path" ]; then
        local output_dir="$WORKSPACE_DIR/output"
        mkdir -p "$output_dir"
        
        local timestamp=$(date +%Y%m%d_%H%M%S)
        local output_name="Snes9xEXPlus-arm64-${BUILD_TYPE}-${timestamp}.apk"
        
        cp "$apk_path" "$output_dir/$output_name"
        
        print_info "APK built successfully:"
        print_info "  File: $output_name"
        print_info "  Size: $(du -h "$output_dir/$output_name" | cut -f1)"
        print_info "  Path: $output_dir/$output_name"
        
        return 0
    else
        print_error "APK build failed - no APK file found"
        echo "Searched paths:"
        for path in "${apk_paths[@]}"; do
            echo "  $path"
        done
        echo "Available APK files:"
        find . -name "*.apk" -type f 2>/dev/null || echo "  No APK files found"
        return 1
    fi
}

show_usage() {
    echo "Usage: $0 [OPTIONS]"
    echo ""
    echo "OPTIONS:"
    echo "  clean     Perform clean build"
    echo "  debug     Build debug APK instead of release"
    echo "  help      Show this help message"
    echo ""
    echo "Environment variables:"
    echo "  NDK_VERSION    Android NDK version (default: r27-beta2)"
    echo ""
    echo "Examples:"
    echo "  $0                 # Build release APK"
    echo "  $0 debug           # Build debug APK"
    echo "  $0 clean           # Clean build release APK"
    echo "  NDK_VERSION=r26d $0  # Use specific NDK version"
}

# Main execution
main() {
    if [ "$1" = "help" ] || [ "$1" = "-h" ] || [ "$1" = "--help" ]; then
        show_usage
        exit 0
    fi
    
    print_header
    
    # Parse arguments
    if [ "$1" = "clean" ]; then
        CLEAN_BUILD="true"
        BUILD_TYPE="release"
    elif [ "$1" = "debug" ]; then
        BUILD_TYPE="debug"
    fi
    
    print_info "Build configuration:"
    print_info "  Type: $BUILD_TYPE"
    print_info "  Clean: $CLEAN_BUILD"
    print_info "  NDK: $NDK_VERSION"
    print_info "  Threads: $(nproc)"
    echo ""
    
    # Execute build steps
    check_dependencies
    setup_environment
    download_ndk
    build_imagine_sdk
    build_emuframework
    
    if build_snes9x; then
        echo ""
        print_step "Build completed successfully! 🎉"
        print_info "Your ARM64 APK is ready in the output/ directory"
    else
        echo ""
        print_error "Build failed! ❌"
        exit 1
    fi
}

# Run main function with all arguments
main "$@"