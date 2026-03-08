# SM64CoopDX iOS Port

Official iOS platform support for sm64coopdx, targeting sideloaded deployment via Xcode.

## Contribution Strategy

This port follows the patterns established by the existing Android port (`TOUCH_CONTROLS`, `HANDHELD` defines) to minimize divergence from upstream. All iOS-specific code is guarded behind `#ifdef TARGET_IOS` and isolated to platform files where possible.

### PR Structure (Recommended)

1. **PR 1 — Core iOS Build & Platform** (ready now)
   - CMake iOS target, Info.plist, platform files
   - All completed source modifications (Phases 1-5)
   - Game is fully playable with a physical controller

2. **PR 2 — Touch Controls** (in progress)
   - Port Android touch control framework to iOS
   - SDL2 finger event handling
   - DJUI touch settings panel

3. **PR 3 — iOS Polish & UX**
   - ROM file picker (no bundled ROM)
   - Mobile UI cleanup
   - Haptic feedback

---

## Architecture

### Build System

- **CMake** generates an Xcode project (no CocoaPods, no SPM)
- SDL2 built from source (`lib/SDL2-source/`, release-2.30.12) as static lib
- Lua 5.3 built from source (`lib/lua-5.3.6/`)
- System zlib linked; no curl, no Discord SDK, no CoopNet

```bash
# Prerequisites: desktop build must run first to generate build/us_pc/ assets
mkdir build-ios && cd build-ios
cmake -G Xcode \
  -DCMAKE_SYSTEM_NAME=iOS \
  -DCMAKE_OSX_ARCHITECTURES=arm64 \
  -DCMAKE_OSX_DEPLOYMENT_TARGET=15.0 \
  ..
# Open Xcode, select "sm64coopdx" scheme, set signing team, build & run
```

### Preprocessor Defines

| Define | Purpose |
|--------|---------|
| `TARGET_IOS` | iOS-specific code paths |
| `HANDHELD` | Shared mobile behavior (also used by Switch) |
| `USE_GLES` | OpenGL ES rendering |
| `TOUCH_CONTROLS` | Touch input (implemented, matches Android) |

### App Bundle Layout

```
sm64coopdx.app/
  sm64coopdx          (binary)
  baserom.us.z64      (ROM — temporary, will use file picker)
  lang/               (language .ini files)
  dynos/              (DynOS packs)
  Info.plist
```

On iOS, `MACOSX_PACKAGE_LOCATION "Resources/foo"` maps to `app/foo/` (no `Contents/` directory like macOS).

### iOS Frameworks Linked

OpenGLES, UIKit, Foundation, GameController, AudioToolbox, CoreAudio, CoreMotion, CoreHaptics, Metal, QuartzCore, AVFoundation, CoreGraphics, CoreBluetooth

---

## Completed Work

### Phase 1: App Shell

| Item | Details |
|------|---------|
| `platform/ios/Info.plist` | Landscape-only, arm64, fullscreen, status bar hidden, ProMotion 120Hz (`CADisableMinimumFrameDurationOnPhone`) |
| Target | iOS 15.0+, arm64 only |
| Bundle ID | `com.sm64coopdx.ios` |

### Phase 2: CMake Integration

- All game sources compiled via CMake
- SDL2, Lua built from source as static libraries
- Discord, SDL1, CoopNet backends excluded
- `.inc.c` files excluded (included by parent files)
- `src/game/main.c` excluded (N64 entry point)

### Phase 3: Source Modifications

| File | Change |
|------|--------|
| `src/pc/gfx/gfx_sdl2.c` | GLES 3.0 context (iOS needs 3.0 for shader compat), landscape orientation hint |
| `src/pc/configfile.c` | Default fullscreen on iOS; `configDjuiThemeCenter = true` on iOS (HANDHELD defaults to false for Switch overlay, not applicable to iOS) |
| `src/pc/platform.c` | `sys_resource_path()` returns bundle root on iOS (not macOS `Contents/Resources`) |
| `src/pc/platform.h` | Declaration for `platform_ios_get_refresh_rate()` |
| `src/pc/platform_ios.m` | Native UIKit call: `[UIScreen mainScreen].maximumFramesPerSecond` for ProMotion 120Hz |
| `src/pc/pc_main.c` | `get_display_refresh_rate()` uses native iOS API (SDL reports 60Hz on ProMotion displays) |
| `src/pc/rom_checker.cpp` | Scan app bundle for ROM; guard `directory_iterator` against non-existent dirs |
| `src/pc/update_checker.c` | Stubbed on iOS (no curl) |

### Phase 4: Asset Pipeline

- ROM bundled via CMake `MACOSX_PACKAGE_LOCATION`
- `lang/` and `dynos/` directories bundled from `build/us_pc/`
- iOS bundle path handling verified (no `Contents/` prefix)

### Phase 5: Runtime Fixes

| Issue | Fix |
|-------|-----|
| Wrong Xcode scheme | Must select `sm64coopdx` (not `ALL_BUILD`) |
| "No rom detected" | Bundle ROM + scan via `sys_resource_path()` |
| SIGABRT on `directory_iterator` | Added existence guard in `scan_path_for_rom()` |
| Game not filling screen | Fullscreen default + landscape hints + GLES 3.0 |
| Language folder not found | `sys_resource_path()` iOS fix (returns bundle root) |
| Menu left-aligned | `configDjuiThemeCenter` excluded from HANDHELD override on iOS |
| FPS capped at 60 on ProMotion | Native UIKit refresh rate query + Info.plist key |

### Verified Working

- [x] Fullscreen rendering at native resolution
- [x] 120Hz on ProMotion displays (iPhone 13 Pro+)
- [x] Language/translation loading
- [x] Shader compilation (GLES 3.0)
- [x] Texture rendering
- [x] Audio playback
- [x] Physical controller input (MFi / GameController via SDL2)
- [x] DynOS pack loading
- [x] Game fully playable with controller

---

## Remaining Work

### P0 — Required for Merge

#### Touch Controls

Port the Android touch control framework. The Android implementation lives in `controller_touchscreen.c` (~500 lines) with 21 input elements (stick, A/B/X/Y/L/R/Z/Start, C-buttons, D-pad, chat, playerlist, console).

| Task | File(s) | Status |
|------|---------|--------|
| Port touch controller core | `src/pc/controller/controller_touchscreen.c/h` | TODO |
| Port touch button layout | `src/pc/controller/controller_touchscreen_layout.inc` | TODO |
| Port touch button textures | `src/pc/controller/controller_touchscreen_textures.c/h` | TODO |
| Register touch controller | `src/pc/controller/controller_entry_point.c` | TODO |
| SDL2 finger event handling | `src/pc/gfx/gfx_sdl2.c` | TODO |
| Touch callback API | `src/pc/gfx/gfx_window_manager_api.h` | TODO |
| Register touch callbacks | `src/pc/pc_main.c` | TODO |
| Touch config variables | `src/pc/configfile.c/h` | TODO |
| Touch settings DJUI panel | `src/pc/djui/djui_panel_touch_controls.c/h` | TODO |
| Touch layout editor | `src/pc/djui/djui_panel_touch_controls_editor.c/h` | TODO |
| Camera touch integration | `src/game/camera.c`, `bettercamera.inc.h` | TODO |
| CMake: add `TOUCH_CONTROLS` define + sources | `CMakeLists.txt` | TODO |

#### ROM File Picker

Replace the bundled ROM approach with a user-facing file picker so the ROM is not distributed inside the app.

| Task | File(s) | Status |
|------|---------|--------|
| iOS document picker (UIDocumentPickerViewController) | `src/pc/platform_ios.m` | TODO |
| Hook file picker into ROM loading flow | `src/pc/rom_checker.cpp`, `src/pc/loading.c` | TODO |
| Remove ROM from CMake bundle | `CMakeLists.txt` | TODO |
| Change "drag & drop" text to "Import ROM" | `src/pc/loading.c` | TODO |

### P1 — Should Have

| Task | File(s) | Status |
|------|---------|--------|
| Hide "fullscreen" toggle in display settings | `src/pc/djui/djui_panel_display.c` | TODO |
| Hide "mute on focus loss" in sound settings | `src/pc/djui/djui_panel_sound.c` | TODO |
| Disable accelerometer-as-joystick | `src/pc/controller/controller_sdl2.c` | TODO |
| Disable `SDL_StartTextInput()` on mobile | `src/pc/gfx/gfx_sdl2.c` | TODO |
| `pthread_cancel` -> `pthread_kill` (iOS compat) | `src/pc/thread.c` | TODO |
| Add touch controls button to controls menu | `src/pc/djui/djui_panel_controls.c` | TODO |
| GLES 3.0 shader rewrite (see details below) | `src/pc/gfx/gfx_opengl.c` | TODO |
| Disable Mumble (not applicable on iOS) | `src/pc/pc_main.c` | TODO |
| Skip fullscreen config persistence on iOS | `src/pc/configfile.c` | TODO |
| Remove third bind slot for A/B/L/R/Z (avoids invalid button codes on mobile) | `src/pc/configfile.c` | TODO |
| DJUI interactable: add `SCANCODE_BACK` as escape alternative | `src/pc/djui/djui_interactable.c` | TODO |
| DJUI interactable: fix cursor update for touch input | `src/pc/djui/djui_interactable.c` | TODO |
| DJUI interactable: Android-style focus begin hook for keyboard | `src/pc/djui/djui_interactable.c` | TODO |
| Duplicate `controller_sdl2.c` for `TOUCH_CONTROLS` build | `src/pc/controller/controller_sdl2.c` | TODO |

### P1 Details — GLES 3.0 Shader Rewrite

The Android port rewrites `gfx_opengl.c` for GLES 3.0 compatibility. Since iOS also uses GLES 3.0, the same changes are needed. Without them, shaders may fail on some devices or produce rendering artifacts.

| Change | From | To |
|--------|------|----|
| GLSL version | `#version 100` | `#version 300 es` |
| Vertex attributes | `attribute` | `in` |
| Vertex varyings | `varying` (VS) | `out` |
| Fragment varyings | `varying` (FS) | `in` |
| Fragment output | `gl_FragColor` | `out vec4 fragColor` + `fragColor = ...` |
| Texture sampling | `texture2D()` | `texture()` |
| TEX_OFFSET macro | `texture2D(tex, ...)` | `texture(tex, ...)` |
| `sampleTex` param | `filter` (reserved in GLES 3) | `filterMode` |
| VAO init guard | `vmajor >= 3 && !is_es` | `vmajor >= 3` (enable for ES 3.0+) |

The Android port uses `#ifdef USE_GLES` / `#else` blocks with helper macros:
```c
#ifdef USE_GLES
    #define ATTR "in"
    #define VARYING_VS "out"
    #define VARYING_FS "in"
#else
    #define ATTR "attribute"
    #define VARYING_VS "varying"
    #define VARYING_FS "varying"
#endif
```

Reference: `sm64coopdx-android` commit `origin/android-dev`, diff in `src/pc/gfx/gfx_opengl.c`.

### P1 Details — Mumble Disable

Mumble (positional audio for VoIP) is not applicable on iOS. The Android port guards both calls:

```c
// In main_game_init():
#ifndef TARGET_ANDROID
    mumble_init();
#endif

// In main loop:
#ifndef TARGET_ANDROID
    mumble_update();
#endif
```

Use `#ifndef TARGET_IOS` for the same effect.

### P1 Details — Controller Binds Cleanup

The Android port removes the third bind slot for several keys when `TOUCH_CONTROLS` is defined. The upstream third slot values (e.g., `0x1103` for configKeyA) are invalid button indices that exceed `MAX_JOYBUTTONS=32`, which can cause out-of-bounds access:

```c
#ifdef TOUCH_CONTROLS
unsigned int configKeyA[MAX_BINDS] = { 0x0026, 0x1000, VK_INVALID };
unsigned int configKeyB[MAX_BINDS] = { 0x0033, 0x1001, VK_INVALID };
unsigned int configKeyL[MAX_BINDS] = { 0x002A, 0x1009, VK_INVALID };
unsigned int configKeyR[MAX_BINDS] = { 0x0036, 0x100A, VK_INVALID };
unsigned int configKeyZ[MAX_BINDS] = { 0x0025, 0x101A, VK_INVALID };
#endif
```

### P1 Details — DJUI Interactable Changes

The Android port makes three changes to `djui_interactable.c`:

1. **Back button as escape**: `SCANCODE_BACK` treated same as `SCANCODE_ESCAPE` for panel navigation
2. **Touch cursor update**: Calls `djui_interactable_cursor_update_active()` when `gInteractableMouseDown == NULL` to fix touch-based menu interaction
3. **Focus begin hook**: On Android, skips overwriting `on_focus_begin` if the new callback is NULL, so the onscreen keyboard still appears for text input fields (e.g., player name)

### P1 Details — Controller SDL2 Duplication

The Android port wraps the entire `controller_sdl2.c` in an `#ifdef TOUCH_CONTROLS` / `#else` block. The `TOUCH_CONTROLS` version is a simplified copy that includes touch-specific headers and disables accelerometer. When porting, the iOS build should use this same pattern.

### P2 — Nice to Have

| Task | Status |
|------|--------|
| Haptic feedback via CoreHaptics | TODO |
| iCloud save sync | TODO |
| Metal rendering backend (GLES is deprecated on iOS) | TODO |
| CoopNet / online multiplayer support | TODO |
| iPad multitasking / Split View support | TODO |

---

## Key Differences from Android Port

| Aspect | Android | iOS |
|--------|---------|-----|
| Build system | Makefile + Gradle | CMake + Xcode |
| Graphics | GLES 3.0 (upgraded from 2.0) | GLES 3.0 (same shader rewrite needed) |
| Refresh rate | Standard 60Hz | ProMotion 120Hz via native UIKit API |
| ROM handling | File-based | Bundle (temporary) -> File picker (planned) |
| Distribution | APK sideload | Xcode sideload / AltStore / TrollStore |
| Menu centering | Left-aligned (HANDHELD) | Centered (HANDHELD override for iOS) |
| Touch controls | Implemented | Planned (porting from Android) |

---

## Files Changed (vs upstream)

New files:
- `platform/ios/Info.plist`
- `src/pc/platform_ios.m`
- `CMakeLists.txt` (new, upstream uses Makefile)

Modified files:
- `src/pc/configfile.c` — fullscreen default, theme center override
- `src/pc/gfx/gfx_sdl2.c` — GLES 3.0 context, landscape hint
- `src/pc/pc_main.c` — iOS refresh rate query
- `src/pc/platform.c` — `sys_resource_path()` iOS path
- `src/pc/platform.h` — iOS function declaration
- `src/pc/rom_checker.cpp` — bundle scanning, directory guard
- `src/pc/update_checker.c` — stubbed on iOS
