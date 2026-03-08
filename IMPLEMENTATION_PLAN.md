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
| `TOUCH_CONTROLS` | Touch input (planned, matches Android) |

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
| Graphics | GLES 2.0 | GLES 3.0 (required for shaders) |
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
