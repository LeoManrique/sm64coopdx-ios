#!/bin/bash
set -e

# SM64CoopDX iOS Build Script
# Requires: macOS, Xcode + iOS SDK, CMake, make

BUILD_DIR="build-ios"
CONFIG="Release"
IPA_NAME="sm64coopdx.ipa"

# CFBundleShortVersionString for the IPA, resolved in priority order:
#   1. APP_VERSION env var, if set   (e.g. APP_VERSION=1.2.3 ./build_ios.sh)
#   2. latest version in apps.json   (single source of truth for the published version)
# apps.json's top version is what AltStore/SideStore install, so deriving the IPA
# version from it keeps the bundle and the source feed in lockstep automatically.
APP_VERSION="${APP_VERSION:-$(python3 -c "import json;print(json.load(open('apps.json'))['apps'][0]['versions'][0]['version'])" 2>/dev/null)}"
[ -z "$APP_VERSION" ] && APP_VERSION="1.0"

# ---- Prerequisites check ----
check_prerequisites() {
    if ! command -v cmake &> /dev/null; then
        echo "Error: cmake not found. Install with: brew install cmake"
        exit 1
    fi

    if ! command -v xcrun &> /dev/null; then
        echo "Error: Xcode command line tools not found. Install with: xcode-select --install"
        exit 1
    fi

    if ! xcrun --sdk iphoneos --show-sdk-path &> /dev/null; then
        echo "Error: iOS SDK not found. Install Xcode from the App Store."
        exit 1
    fi

    if ! command -v make &> /dev/null; then
        echo "Error: make not found."
        exit 1
    fi

    if [ ! -f "baserom.us.z64" ]; then
        echo "Error: baserom.us.z64 not found in repo root."
        echo "Place your base ROM as baserom.us.z64 before building."
        exit 1
    fi
}

# ---- Desktop build (generates assets) ----
desktop() {
    echo "==> Building desktop version..."
    # Build tools first (Makefile's two-pass restart can miss them)
    make -C tools
    # Pre-create all build output directories (Makefile's mkdir can miss them on fresh builds)
    for dir in src actors levels bin data sound textures text lib include; do
        find "$dir" -type d 2>/dev/null | while read d; do mkdir -p "build/us_pc/$d"; done
    done
    mkdir -p build/us_pc/assets build/us_pc/rsp build/us_pc/include build/us_pc/res
    # Pre-generate texture .inc.c files (Makefile can't resolve these deps on fresh builds)
    find actors levels textures -name "*.png" | while read png; do
        inc="build/us_pc/${png%.png}.inc.c"
        if [ ! -f "$inc" ]; then
            fmt=$(echo "$png" | sed 's/.*\.\(.*\)\.png/\1/')
            mkdir -p "$(dirname "$inc")"
            tools/n64graphics -s u8 -i "$inc" -g "$png" -f "$fmt" 2>/dev/null || true
        fi
    done
    # Pre-copy coopnet dylibs into build dir (Makefile's COOPNET_LIBS rule collides with BUILD_DIR target)
    if [ "$(uname -m)" = "arm64" ]; then
        COOPNET_SRC="lib/coopnet/mac_arm"
    else
        COOPNET_SRC="lib/coopnet/mac_intel"
    fi
    cp -f "$COOPNET_SRC/libcoopnet.dylib" "$COOPNET_SRC/libjuice.1.6.2.dylib" build/us_pc/
    make -j$(sysctl -n hw.ncpu)
    # Post-fix GLEW in app bundle (Makefile hardcodes libGLEW.2.2.0 but brew may ship 2.3+)
    APP_MACOS_DIR="build/us_pc/sm64coopdx.app/Contents/MacOS"
    if [ -d "$APP_MACOS_DIR" ] && [ ! -f "$APP_MACOS_DIR/libGLEW.dylib" ]; then
        GLEW_REF=$(otool -L "$APP_MACOS_DIR/sm64coopdx" 2>/dev/null | awk '/libGLEW/{print $1; exit}')
        if [ -n "$GLEW_REF" ] && [ -e "$GLEW_REF" ]; then
            cp "$GLEW_REF" "$APP_MACOS_DIR/libGLEW.dylib"
            install_name_tool -change "$GLEW_REF" @executable_path/libGLEW.dylib "$APP_MACOS_DIR/sm64coopdx" >/dev/null 2>&1
            install_name_tool -id @executable_path/libGLEW.dylib "$APP_MACOS_DIR/libGLEW.dylib" >/dev/null 2>&1
            codesign --force --deep --sign - "$APP_MACOS_DIR/libGLEW.dylib" >/dev/null 2>&1
        fi
    fi
    echo "==> Desktop build complete."
}

# ---- Generate Xcode project ----
generate() {
    echo "==> Generating Xcode project (version $APP_VERSION)..."
    cmake -B "$BUILD_DIR" -G Xcode \
        -DCMAKE_SYSTEM_NAME=iOS \
        -DCMAKE_OSX_ARCHITECTURES=arm64 \
        -DCMAKE_OSX_DEPLOYMENT_TARGET=15.0 \
        -DIOS_MARKETING_VERSION="$APP_VERSION"
    echo "==> Xcode project generated at $BUILD_DIR/"
}

# ---- Build ----
build() {
    # Always regenerate: cmake reconfigure is cheap (~0.1s, not a recompile) and ensures
    # version changes and edits to platform/ios/Info.plist propagate into the Xcode project.
    # A stale build-ios/ cache previously pinned an old MARKETING_VERSION and a broken plist.
    generate

    echo "==> Building ($CONFIG, version $APP_VERSION)..."
    cmake --build "$BUILD_DIR" --config "$CONFIG" -- CODE_SIGNING_ALLOWED=NO MARKETING_VERSION="$APP_VERSION"
    echo "==> Build succeeded."
}

# ---- Clean ----
clean() {
    if [ -d "$BUILD_DIR" ]; then
        echo "==> Cleaning..."
        cmake --build "$BUILD_DIR" --config "$CONFIG" --target clean -- CODE_SIGNING_ALLOWED=NO
        echo "==> Clean done."
    fi
}

# ---- Package IPA ----
ipa() {
    APP_PATH="$BUILD_DIR/$CONFIG-iphoneos/sm64coopdx.app"

    if [ ! -d "$APP_PATH" ]; then
        echo "Error: App bundle not found. Run './build_ios.sh build' first."
        exit 1
    fi

    echo "==> Packaging IPA..."
    TMPDIR=$(mktemp -d)
    mkdir -p "$TMPDIR/Payload"
    cp -R "$APP_PATH" "$TMPDIR/Payload/"
    (cd "$TMPDIR" && zip -r -q "$OLDPWD/$IPA_NAME" Payload/)
    rm -rf "$TMPDIR"
    echo "==> Created $IPA_NAME ($(du -h "$IPA_NAME" | cut -f1))"
}

# ---- Full pipeline ----
all() {
    check_prerequisites
    desktop
    clean
    build
    ipa
}

# ---- Usage ----
usage() {
    echo "Usage: ./build_ios.sh <command>"
    echo ""
    echo "Commands:"
    echo "  generate    Generate Xcode project"
    echo "  build       Build the app (Release)"
    echo "  clean       Clean build artifacts"
    echo "  ipa         Package .app into .ipa for sideloading"
    echo "  desktop     Build native desktop (macOS) .app bundle at build/us_pc/sm64coopdx.app"
    echo "  all         Full pipeline: build assets + clean + build iOS + package IPA"
    echo ""
    echo "Prerequisites:"
    echo "  - macOS with Xcode and iOS SDK installed"
    echo "  - CMake (brew install cmake)"
}

# ---- Entry point ----
case "${1:-}" in
    generate)       check_prerequisites; generate ;;
    build)          check_prerequisites; build ;;
    clean)          clean ;;
    ipa)            ipa ;;
    desktop)        check_prerequisites; desktop ;;
    all)            all ;;
    *)              all ;;
esac
