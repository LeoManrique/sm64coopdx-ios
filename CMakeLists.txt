cmake_minimum_required(VERSION 3.21)
project(sm64coopdx C CXX OBJC)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# ---- Options ----
option(TARGET_IOS "Build for iOS" ON)

# ---- Root of the game source tree ----
set(GAME_ROOT ${CMAKE_SOURCE_DIR})
set(BUILD_PC_DIR ${GAME_ROOT}/build/us_pc)

# ---- Preprocessor defines (mirrors Makefile + Android.mk) ----
add_compile_definitions(
    NON_MATCHING
    AVOID_UB
    VERSION_US
    F3DEX_GBI_2E
    F3DEX_GBI_SHARED
    _LANGUAGE_C
    NO_SEGMENTED_MEMORY
    STDC_HEADERS
    WIDESCREEN
    TEXTURE_FIX
    BETTERCAMERA
    EXT_OPTIONS_MENU
    IMMEDIATELOAD
    DYNOS
    COOP
    USE_GLES
    RAPI_GL=1
    WAPI_SDL2=1
    AAPI_SDL2=1
    CAPI_SDL2=1
    HAVE_SDL2=1
    TARGET_IOS
    HANDHELD
    TOUCH_CONTROLS
)

# ---- Compiler flags ----
add_compile_options(
    -fsigned-char
    -fno-strict-aliasing
    -fwrapv
    -Wall
    -Wno-unused-variable
    -Wno-unused-function
    -Wno-unused-but-set-variable
    -Wno-unused-label
    -Wno-sign-compare
    -Wno-sometimes-uninitialized
    -Wno-missing-braces
    -Wno-int-conversion
    -Wno-incompatible-pointer-types
    -Wno-pointer-sign
    -Wno-macro-redefined
    -Wno-tautological-compare
    -Wno-implicit-function-declaration
    -Wno-format
    -O2
)

# ---- Include directories ----
include_directories(
    ${GAME_ROOT}/include
    ${GAME_ROOT}/src
    ${GAME_ROOT}
    ${GAME_ROOT}/sound
    ${GAME_ROOT}/lib/lua/include
    ${GAME_ROOT}/lib/coopnet/include
    ${BUILD_PC_DIR}
    ${BUILD_PC_DIR}/include
)

# ---- Collect source files ----

# Main src/ tree (recursive, with exclusions)
file(GLOB_RECURSE SRC_C_FILES
    ${GAME_ROOT}/src/engine/*.c
    ${GAME_ROOT}/src/game/*.c
    ${GAME_ROOT}/src/audio/*.c
    ${GAME_ROOT}/src/menu/*.c
    ${GAME_ROOT}/src/buffers/*.c
    ${GAME_ROOT}/src/goddard/*.c
    ${GAME_ROOT}/src/pc/*.c
)
# CPP files needed for cross-platform
file(GLOB SRC_CPP_FILES
    ${GAME_ROOT}/src/pc/mods/mod_fs.cpp
    ${GAME_ROOT}/src/pc/mods/mod_storage.cpp
    ${GAME_ROOT}/src/pc/rom_checker.cpp
)

# Exclude files not applicable to iOS
list(FILTER SRC_C_FILES EXCLUDE REGEX "src/game/main\\.c$")
list(FILTER SRC_C_FILES EXCLUDE REGEX "src/pc/discord/")
list(FILTER SRC_C_FILES EXCLUDE REGEX "src/pc/gfx/gfx_sdl1\\.")
list(FILTER SRC_C_FILES EXCLUDE REGEX "src/pc/audio/audio_sdl1\\.")
list(FILTER SRC_C_FILES EXCLUDE REGEX "src/pc/controller/controller_sdl1\\.")
# .inc.c files are #included by other files, not compiled standalone
list(FILTER SRC_C_FILES EXCLUDE REGEX "\\.inc\\.c$")

# actors/ (top-level .c only, not subdirs)
file(GLOB ACTORS_C_FILES ${GAME_ROOT}/actors/*.c)

# levels/ top-level
file(GLOB LEVELS_TOP_C_FILES ${GAME_ROOT}/levels/entry.c ${GAME_ROOT}/levels/scripts.c)

# levels/ subdirectories (specific files per Makefile pattern)
file(GLOB LEVEL_C_FILES
    ${GAME_ROOT}/levels/*/leveldata.c
    ${GAME_ROOT}/levels/*/script.c
    ${GAME_ROOT}/levels/*/geo.c
)

# bin/ (top-level .c only)
file(GLOB BIN_C_FILES ${GAME_ROOT}/bin/*.c)

# data/
file(GLOB DATA_C_FILES ${GAME_ROOT}/data/*.c)
file(GLOB DATA_CPP_FILES ${GAME_ROOT}/data/*.cpp)

# sound/
set(SOUND_C_FILES
    ${GAME_ROOT}/sound/sound_data.c
    ${GAME_ROOT}/sound/samples_assets.c
    ${GAME_ROOT}/sound/sequences_assets.c
)

# lib/src/ (specific ultra files for PC target)
set(ULTRA_C_FILES
    ${GAME_ROOT}/lib/src/alBnkfNew.c
    ${GAME_ROOT}/lib/src/guLookAtRef.c
    ${GAME_ROOT}/lib/src/guMtxF2L.c
    ${GAME_ROOT}/lib/src/guNormalize.c
    ${GAME_ROOT}/lib/src/guOrthoF.c
    ${GAME_ROOT}/lib/src/guPerspectiveF.c
    ${GAME_ROOT}/lib/src/guRotateF.c
    ${GAME_ROOT}/lib/src/guScaleF.c
    ${GAME_ROOT}/lib/src/guTranslateF.c
    ${GAME_ROOT}/lib/src/ldiv.c
)

# Generated files from desktop build
set(GENERATED_C_FILES
    ${BUILD_PC_DIR}/assets/mario_anim_data.c
    ${BUILD_PC_DIR}/assets/demo_data.c
)

# All sources combined
set(ALL_SOURCES
    ${SRC_C_FILES}
    ${SRC_CPP_FILES}
    ${ACTORS_C_FILES}
    ${LEVELS_TOP_C_FILES}
    ${LEVEL_C_FILES}
    ${BIN_C_FILES}
    ${DATA_C_FILES}
    ${DATA_CPP_FILES}
    ${SOUND_C_FILES}
    ${ULTRA_C_FILES}
    ${GENERATED_C_FILES}
)

# ---- SDL2 (build from source for iOS) ----
set(SDL_SHARED OFF CACHE BOOL "" FORCE)
set(SDL_STATIC ON CACHE BOOL "" FORCE)
set(SDL_TEST OFF CACHE BOOL "" FORCE)
set(SDL2_DISABLE_INSTALL ON CACHE BOOL "" FORCE)
# Apply iOS gamepad fix patches to SDL2 before building
execute_process(
    COMMAND git apply --check ${GAME_ROOT}/patches/sdl2-ios-gamepad-fix.patch
    WORKING_DIRECTORY ${GAME_ROOT}/lib/SDL2-source
    RESULT_VARIABLE PATCH_CHECK_RESULT
    ERROR_QUIET
)
if(PATCH_CHECK_RESULT EQUAL 0)
    execute_process(
        COMMAND git apply ${GAME_ROOT}/patches/sdl2-ios-gamepad-fix.patch
        WORKING_DIRECTORY ${GAME_ROOT}/lib/SDL2-source
    )
    message(STATUS "Applied SDL2 iOS gamepad fix patch")
else()
    message(STATUS "SDL2 iOS gamepad fix patch already applied or not needed")
endif()

add_subdirectory(${GAME_ROOT}/lib/SDL2-source SDL2 EXCLUDE_FROM_ALL)
include_directories(${GAME_ROOT}/lib/SDL2-source/include)

# ---- Lua 5.3 (build from source for iOS) ----
set(LUA_SRC_DIR ${GAME_ROOT}/lib/lua-5.3.6/src)
file(GLOB LUA_SOURCES ${LUA_SRC_DIR}/*.c)
# Remove lua.c and luac.c (standalone interpreter/compiler)
list(FILTER LUA_SOURCES EXCLUDE REGEX "lua\\.c$")
list(FILTER LUA_SOURCES EXCLUDE REGEX "luac\\.c$")
add_library(lua53 STATIC ${LUA_SOURCES})
target_include_directories(lua53 PUBLIC ${LUA_SRC_DIR})
target_compile_definitions(lua53 PRIVATE LUA_USE_POSIX LUA_USE_IOS)

# ---- iOS App Bundle ----
# iOS Objective-C sources
set(IOS_OBJC_FILES
    ${GAME_ROOT}/src/pc/platform_ios.m
)

add_executable(sm64coopdx MACOSX_BUNDLE ${ALL_SOURCES} ${IOS_OBJC_FILES})

# Files that include miniaudio.h need ObjC on iOS (AVFoundation headers)
set_source_files_properties(
    ${GAME_ROOT}/src/pc/lua/utils/smlua_audio_utils.c
    PROPERTIES LANGUAGE OBJC
)

target_link_libraries(sm64coopdx
    SDL2main
    SDL2-static
    lua53
    z
)

# iOS app bundle properties
set_target_properties(sm64coopdx PROPERTIES
    MACOSX_BUNDLE TRUE
    MACOSX_BUNDLE_GUI_IDENTIFIER "com.sm64coopdx.ios"
    MACOSX_BUNDLE_BUNDLE_NAME "SM64CoopDX"
    MACOSX_BUNDLE_BUNDLE_VERSION "1.0"
    MACOSX_BUNDLE_SHORT_VERSION_STRING "1.0"
    MACOSX_BUNDLE_INFO_PLIST ${GAME_ROOT}/platform/ios/Info.plist
    XCODE_ATTRIBUTE_TARGETED_DEVICE_FAMILY "1,2"
    XCODE_ATTRIBUTE_CODE_SIGN_STYLE "Automatic"
    XCODE_ATTRIBUTE_IPHONEOS_DEPLOYMENT_TARGET "15.0"
    XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "com.sm64coopdx.ios"
    XCODE_ATTRIBUTE_DEVELOPMENT_TEAM ""
)

# iOS frameworks
target_link_libraries(sm64coopdx
    "-framework OpenGLES"
    "-framework Foundation"
    "-framework UIKit"
    "-framework AudioToolbox"
    "-framework CoreAudio"
    "-framework GameController"
    "-framework CoreMotion"
    "-framework CoreHaptics"
    "-framework Metal"
    "-framework QuartzCore"
    "-framework AVFoundation"
    "-framework CoreGraphics"
    "-framework CoreBluetooth"
)

# Bundle the ROM file into the app
# TODO: Replace with a file picker UI so users can import their own ROM
set(ROM_FILE ${GAME_ROOT}/baserom.us.z64)
if(EXISTS ${ROM_FILE})
    target_sources(sm64coopdx PRIVATE ${ROM_FILE})
    set_source_files_properties(${ROM_FILE} PROPERTIES
        MACOSX_PACKAGE_LOCATION Resources
    )
endif()

# Copy game data files into the app bundle
set(GAME_DATA_DIRS
    ${BUILD_PC_DIR}/lang
    ${BUILD_PC_DIR}/dynos
)
foreach(DATA_DIR ${GAME_DATA_DIRS})
    if(EXISTS ${DATA_DIR})
        file(GLOB DATA_CONTENTS "${DATA_DIR}/*")
        get_filename_component(DIR_NAME ${DATA_DIR} NAME)
        foreach(ITEM ${DATA_CONTENTS})
            target_sources(sm64coopdx PRIVATE ${ITEM})
            set_source_files_properties(${ITEM} PROPERTIES
                MACOSX_PACKAGE_LOCATION "Resources/${DIR_NAME}"
            )
        endforeach()
    endif()
endforeach()
