# SM64CoopDX iOS Port

Official iOS platform support for sm64coopdx, targeting sideloaded deployment via Xcode.

## Contribution Strategy

This port follows the patterns established by the existing Android port (`TOUCH_CONTROLS`, `HANDHELD` defines) to minimize divergence from upstream. All iOS-specific code is guarded behind `#ifdef TARGET_IOS` and isolated to platform files where possible.

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
| `TOUCH_CONTROLS` | Touch input (matches Android) |

### App Bundle Layout

```
sm64coopdx.app/
  sm64coopdx          (binary)
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
| `src/pc/gfx/gfx_sdl2.c` | GLES 3.0 context, landscape orientation hint, SDL finger event handling |
| `src/pc/configfile.c` | Default fullscreen on iOS; `configDjuiThemeCenter = true` on iOS; touch control config vars; third bind slot cleanup (`VK_INVALID` under `TOUCH_CONTROLS`) |
| `src/pc/configfile.h` | Touch control config declarations |
| `src/pc/platform.c` | `sys_resource_path()` returns bundle root on iOS |
| `src/pc/platform.h` | iOS function declarations (refresh rate, ROM picker) |
| `src/pc/platform_ios.m` | Native UIKit: refresh rate query, ROM file picker (`UIDocumentPickerViewController`) |
| `src/pc/pc_main.c` | iOS refresh rate via native API, touch callback registration |
| `src/pc/loading.c` | iOS ROM file picker integration |
| `src/pc/rom_checker.cpp` | Scan app bundle for ROM; guard `directory_iterator` against non-existent dirs |
| `src/pc/update_checker.c` | Stubbed on iOS (no curl) |
| `src/pc/controller/controller_sdl2.c` | Accelerometer-as-joystick disabled; `gGamepadActive` axis deadzone for touch auto-hide |
| `src/pc/controller/controller_entry_point.c` | Touch controller registration |
| `src/pc/gfx/gfx_window_manager_api.h` | `set_touchscreen_callbacks` API |
| `src/pc/gfx/gfx_dummy.c` | Touch callback stub |
| `src/pc/djui/djui.c` | Touch controls render call |
| `src/pc/djui/djui_cursor.c` | Touch input cursor handling |
| `src/pc/djui/djui_interactable.c` | `SCANCODE_BACK` as escape; touch cursor update; touch-aware focus |
| `src/pc/djui/djui_panel.c` | Touch controls menu integration |
| `src/pc/djui/djui_panel_controls.c` | Touch controls button in controls menu |
| `src/pc/djui/djui_panel_display.c` | Fullscreen toggle hidden on iOS |
| `src/pc/djui/djui_panel_sound.c` | "Mute on focus loss" hidden on iOS |
| `src/game/bettercamera.inc.h` | Touch mouse input for camera control |

### Phase 4: Asset Pipeline

- `lang/` and `dynos/` directories bundled from `build/us_pc/`
- iOS bundle path handling verified (no `Contents/` prefix)
- ROM not bundled — user imports via file picker on first launch

### Phase 5: Runtime Fixes

| Issue | Fix |
|-------|-----|
| Wrong Xcode scheme | Must select `sm64coopdx` (not `ALL_BUILD`) |
| SIGABRT on `directory_iterator` | Added existence guard in `scan_path_for_rom()` |
| Game not filling screen | Fullscreen default + landscape hints + GLES 3.0 |
| Language folder not found | `sys_resource_path()` iOS fix (returns bundle root) |
| Menu left-aligned | `configDjuiThemeCenter` excluded from HANDHELD override on iOS |
| FPS capped at 60 on ProMotion | Native UIKit refresh rate query + Info.plist key |
| Accelerometer detected as gamepad | `SDL_HINT_ACCELEROMETER_AS_JOYSTICK` set to `"0"` |

### Phase 6: Touch Controls

| Item | Details |
|------|---------|
| Touch controller core | `src/pc/controller/controller_touchscreen.c/h` — ported from Android, 21 input elements (stick, A/B/X/Y/L/R/Z/Start, C-buttons, D-pad, chat, playerlist, console) |
| Touch button layout | `src/pc/controller/controller_touchscreen_layout.inc` — default element positions and sizes |
| Touch button textures | `src/pc/controller/controller_touchscreen_textures.c/h` — 42 RGBA16 textures (16x16), generated from PNGs via `n64graphics` |
| DJUI settings panel | `src/pc/djui/djui_panel_touch_controls.c/h` — touch controls settings UI |
| DJUI layout editor | `src/pc/djui/djui_panel_touch_controls_editor.c/h` — drag-to-reposition touch elements |
| Gamepad auto-hide | `gGamepadActive` set on stick/trigger axis input (deadzone 4000) to auto-hide touch controls |

### Phase 7: iPad Gamepad Fix

iPadOS converts game controller button presses into phantom `SDL_KEYDOWN` keyboard events through three SDL2 pathways (GCKeyboard, UIPress, UIKeyCommand). Fixed by patching SDL2 source:

| File | Change |
|------|--------|
| `SDL_uikitevents.m` | Disabled `SDL_InitGCKeyboard()` on iOS |
| `SDL_uikitview.m` | Guarded `pressesBegan`/`pressesEnded`/`pressesCancelled` with `#if !TARGET_OS_IOS` |
| `SDL_uikitviewcontroller.m` | `keyCommands` returns empty array on iOS |
| `CMakeLists.txt` | Auto-applies `patches/sdl2-ios-gamepad-fix.patch` at configure time |

Text input (chat) still works via `SDL_TEXTINPUT` events (UIKit text field responder chain), which is separate from the disabled keyboard pathways.

### Phase 8: ROM File Picker

Replaced bundled ROM with a user-facing file picker so the ROM is not distributed inside the app.

Flow: app launches → `main_rom_handler()` scans write path → no ROM found → loading screen shows "Select your Super Mario 64 (US) ROM file" → `UIDocumentPickerViewController` opens automatically → user selects `.z64` file → imported to temp dir → validated via MD5 → copied to app write path → game loads. On subsequent launches, ROM is found in write path and picker is skipped.

---

## Verified Working

- [x] Fullscreen rendering at native resolution
- [x] 120Hz on ProMotion displays (iPhone 13 Pro+)
- [x] Language/translation loading
- [x] Shader compilation (GLES 3.0)
- [x] Texture rendering
- [x] Audio playback
- [x] Physical controller input (MFi / GameController via SDL2)
- [x] DynOS pack loading
- [x] Game fully playable with controller
- [x] Touch controls (all 21 elements, customizable layout)
- [x] Touch controls auto-hide when gamepad active
- [x] iPad gamepad support (no phantom keyboard events)
- [x] iPhone + iPad both working with touch and gamepad
- [x] ROM file picker (user imports via iOS document picker)
- [x] Accelerometer not exposed as joystick
- [x] Fullscreen/mute options hidden from iOS UI

---

## Key Differences from Android Port

| Aspect | Android | iOS |
|--------|---------|-----|
| Build system | Makefile + Gradle | CMake + Xcode |
| Graphics | GLES 3.0 (upgraded from 2.0) | GLES 3.0 (old shader syntax works on Apple GPUs) |
| Refresh rate | Standard 60Hz | ProMotion 120Hz via native UIKit API |
| ROM handling | File-based | iOS document picker (UIDocumentPickerViewController) |
| Distribution | APK sideload | Xcode sideload / AltStore / TrollStore |
| Menu centering | Left-aligned (HANDHELD) | Centered (HANDHELD override for iOS) |
| Touch controls | Implemented | Implemented (ported from Android) |

---

## Files Changed (vs upstream)

New files:
- `platform/ios/Info.plist`
- `src/pc/platform_ios.m`
- `CMakeLists.txt` (new, upstream uses Makefile)
- `patches/sdl2-ios-gamepad-fix.patch`
- `src/pc/controller/controller_touchscreen.c/h`
- `src/pc/controller/controller_touchscreen_layout.inc`
- `src/pc/controller/controller_touchscreen_textures.c/h`
- `textures/touchcontrols/*.rgba16.inc.c` (42 generated texture data files)
- `src/pc/djui/djui_panel_touch_controls.c/h`
- `src/pc/djui/djui_panel_touch_controls_editor.c/h`

Modified files:
- `src/pc/configfile.c` — fullscreen default, theme center override, touch control config, bind slot cleanup
- `src/pc/configfile.h` — touch control config declarations
- `src/pc/gfx/gfx_sdl2.c` — GLES 3.0 context, landscape hint, touch finger event handling
- `src/pc/gfx/gfx_window_manager_api.h` — `set_touchscreen_callbacks` API
- `src/pc/gfx/gfx_dummy.c` — touch callback stub
- `src/pc/pc_main.c` — iOS refresh rate query, touch callback registration
- `src/pc/platform.c` — `sys_resource_path()` iOS path
- `src/pc/platform.h` — iOS function declarations (refresh rate, ROM picker)
- `src/pc/loading.c` — iOS ROM file picker integration
- `src/pc/rom_checker.cpp` — bundle scanning, directory guard
- `src/pc/update_checker.c` — stubbed on iOS
- `src/pc/controller/controller_entry_point.c` — touch controller registration
- `src/pc/controller/controller_sdl2.c` — accelerometer disabled, gGamepadActive axis deadzone
- `src/pc/djui/djui.c` — touch controls render call
- `src/pc/djui/djui_cursor.c` — touch input cursor handling
- `src/pc/djui/djui_interactable.c` — touch-aware interaction, SCANCODE_BACK
- `src/pc/djui/djui_panel.c` — touch controls menu integration
- `src/pc/djui/djui_panel_controls.c` — touch controls button in controls menu
- `src/pc/djui/djui_panel_display.c` — fullscreen toggle hidden on iOS
- `src/pc/djui/djui_panel_sound.c` — mute on focus loss hidden on iOS
- `src/game/bettercamera.inc.h` — touch mouse camera control
- `.gitignore` — unignored `CMakeLists.txt`

---

## Future Ideas (P2)

| Idea | Notes |
|------|-------|
| Haptic feedback via CoreHaptics | Touch button feedback |
| iCloud save sync | Persist saves across devices |
| Metal rendering backend | GLES is deprecated on iOS (still works) |
| CoopNet / online multiplayer | Requires network backend port |
| iPad multitasking / Split View | Currently requires full screen |
