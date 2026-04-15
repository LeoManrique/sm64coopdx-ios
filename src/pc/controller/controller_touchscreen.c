//Feel free to use it in your port too, but please keep authorship!
//Touch Controls made by: VDavid003
/* Heavily modified by xLuigiGamerx and ManIsCat2 to add new features, like:

- DJUI Based touch controls editor
- Different texture per button and joystick (joystick base as well)

*/

#ifdef TOUCH_CONTROLS
#include <ultra64.h>
#include <PR/ultratypes.h>
#include <PR/gbi.h>

#include "config.h"
#include "sm64.h"
#include "game/game_init.h"
#include "game/memory.h"
#include "game/segment2.h"
#include "game/object_helpers.h"
#include "gfx_dimensions.h"
#include "pc/pc_main.h"
#include "pc/gfx/gfx_pc.h"
#include "pc/djui/djui_panel.h"
#include "pc/djui/djui_panel_pause.h"
#include "pc/djui/djui_panel_main.h"
#include "pc/djui/djui_panel_touch_controls_editor.h"
#include "pc/djui/djui_console.h"
#include "pc/network/network.h"

#include "controller_api.h"
#include "controller_touchscreen.h"
#include "controller_touchscreen_textures.h"
#include "pc/djui/djui_gfx.h"
#include "game/ingame_menu.h"

#include "pc/configfile.h"

// Mouselook
s16 before_x = 0;
s16 before_y = 0;
s16 touch_x = 0;
s16 touch_y = 0;
s16 touch_cam_last_x = 0;
s16 touch_cam_last_y = 0;

// Config
bool gInTouchConfig = false, gGamepadActive = false;
enum ConfigControlElementIndex gSelectedTouchElement = TOUCH_MOUSE;

ConfigControlElement configControlElementsDefault[TOUCH_COUNT] = {
#include "controller_touchscreen_layout.inc"
};

ConfigControlElement configControlElements[TOUCH_COUNT] = {
#include "controller_touchscreen_layout.inc"
};

ConfigControlElement configControlElementsLast[TOUCH_COUNT] = {
#include "controller_touchscreen_layout.inc"
};

// This order must match configControlElements and ConfigControlElementIndex
static struct ControlElement controlElements[TOUCH_COUNT] = {
    [TOUCH_STICK] =      {.type = Joystick},
    [TOUCH_MOUSE] =      {.type = Mouse},
    [TOUCH_A] =          {.type = Button, .buttonTexture = { .buttonUp = TEXTURE_TOUCH_A,          .buttonDown = TEXTURE_TOUCH_A_PRESSED },          .buttonID = A_BUTTON},
    [TOUCH_B] =          {.type = Button, .buttonTexture = { .buttonUp = TEXTURE_TOUCH_B,          .buttonDown = TEXTURE_TOUCH_B_PRESSED },          .buttonID = B_BUTTON},
    [TOUCH_X] =          {.type = Button, .buttonTexture = { .buttonUp = TEXTURE_TOUCH_X,          .buttonDown = TEXTURE_TOUCH_X_PRESSED },          .buttonID = X_BUTTON},
    [TOUCH_Y] =          {.type = Button, .buttonTexture = { .buttonUp = TEXTURE_TOUCH_Y,          .buttonDown = TEXTURE_TOUCH_Y_PRESSED },          .buttonID = Y_BUTTON},
    [TOUCH_START] =      {.type = Button, .buttonTexture = { .buttonUp = TEXTURE_TOUCH_START,      .buttonDown = TEXTURE_TOUCH_START_PRESSED },      .buttonID = START_BUTTON},
    [TOUCH_L] =          {.type = Button, .buttonTexture = { .buttonUp = TEXTURE_TOUCH_L,          .buttonDown = TEXTURE_TOUCH_L_PRESSED },          .buttonID = L_TRIG},
    [TOUCH_R] =          {.type = Button, .buttonTexture = { .buttonUp = TEXTURE_TOUCH_R,          .buttonDown = TEXTURE_TOUCH_R_PRESSED },          .buttonID = R_TRIG},
    [TOUCH_Z] =          {.type = Button, .buttonTexture = { .buttonUp = TEXTURE_TOUCH_Z,          .buttonDown = TEXTURE_TOUCH_Z_PRESSED },          .buttonID = Z_TRIG},
    [TOUCH_CUP] =        {.type = Button, .buttonTexture = { .buttonUp = TEXTURE_TOUCH_C_UP,       .buttonDown = TEXTURE_TOUCH_C_UP_PRESSED },       .buttonID = U_CBUTTONS},
    [TOUCH_CDOWN] =      {.type = Button, .buttonTexture = { .buttonUp = TEXTURE_TOUCH_C_DOWN,     .buttonDown = TEXTURE_TOUCH_C_DOWN_PRESSED },     .buttonID = D_CBUTTONS},
    [TOUCH_CLEFT] =      {.type = Button, .buttonTexture = { .buttonUp = TEXTURE_TOUCH_C_LEFT,     .buttonDown = TEXTURE_TOUCH_C_LEFT_PRESSED },     .buttonID = L_CBUTTONS},
    [TOUCH_CRIGHT] =     {.type = Button, .buttonTexture = { .buttonUp = TEXTURE_TOUCH_C_RIGHT,    .buttonDown = TEXTURE_TOUCH_C_RIGHT_PRESSED },    .buttonID = R_CBUTTONS},
    [TOUCH_CHAT] =       {.type = Button, .buttonTexture = { .buttonUp = TEXTURE_TOUCH_CHAT,       .buttonDown = TEXTURE_TOUCH_CHAT_PRESSED },       .buttonID = CHAT_BUTTON},
    [TOUCH_PLAYERLIST] = {.type = Button, .buttonTexture = { .buttonUp = TEXTURE_TOUCH_PLAYERLIST, .buttonDown = TEXTURE_TOUCH_PLAYERLIST_PRESSED }, .buttonID = PLAYERLIST_BUTTON},
    [TOUCH_DUP] =        {.type = Button, .buttonTexture = { .buttonUp = TEXTURE_TOUCH_DPAD_UP,    .buttonDown = TEXTURE_TOUCH_DPAD_UP_PRESSED },    .buttonID = U_JPAD},
    [TOUCH_DDOWN] =      {.type = Button, .buttonTexture = { .buttonUp = TEXTURE_TOUCH_DPAD_DOWN,  .buttonDown = TEXTURE_TOUCH_DPAD_DOWN_PRESSED },  .buttonID = D_JPAD},
    [TOUCH_DLEFT] =      {.type = Button, .buttonTexture = { .buttonUp = TEXTURE_TOUCH_DPAD_LEFT,  .buttonDown = TEXTURE_TOUCH_DPAD_LEFT_PRESSED },  .buttonID = L_JPAD},
    [TOUCH_DRIGHT] =     {.type = Button, .buttonTexture = { .buttonUp = TEXTURE_TOUCH_DPAD_RIGHT, .buttonDown = TEXTURE_TOUCH_DPAD_RIGHT_PRESSED }, .buttonID = R_JPAD},
    [TOUCH_CONSOLE] =    {.type = Button, .buttonTexture = { .buttonUp = TEXTURE_TOUCH_CONSOLE,    .buttonDown = TEXTURE_TOUCH_CONSOLE_PRESSED },    .buttonID = CONSOLE_BUTTON},
};

static u32 controlElementsLength = sizeof(controlElements)/sizeof(struct ControlElement);

struct Position get_pos(ConfigControlElement *config) {
    struct Position ret;

    if (config->anchor == CONTROL_ELEMENT_HIDDEN) {
        if (gInTouchConfig) {
            ret.x = config->x;
            ret.y = config->y;
        } else {
            ret.x = HIDE_POS;
            ret.y = HIDE_POS;
        }
    } else {
        switch (config->anchor) {
            case CONTROL_ELEMENT_LEFT:
                ret.x = RECT_FROM_LEFT_EDGE(config->x << 2);
                break;
            case CONTROL_ELEMENT_RIGHT:
                ret.x = RECT_FROM_RIGHT_EDGE(config->x << 2);
                break;
            case CONTROL_ELEMENT_CENTER:
            default:
                ret.x = config->x;
                break;
        }
        ret.y = config->y;
    }

    if (configSnapTouch) {
        ret.x = 50 * ((ret.x + 49) / 50) - 25;
        ret.y = 50 * ((ret.y + 49) / 50) - 25;
    }

    if (!gInTouchConfig && (gDjuiInMainMenu && !gDjuiDisabled)) {
        ret.x = HIDE_POS;
        ret.y = HIDE_POS;
    }

    return ret;
}

Colors get_color(ConfigControlElement *config) {
    Colors ret;
    
    ret.r = config->r;
    ret.g = config->g;
    ret.b = config->b;
    ret.a = config->a;

    return ret;
}

void move_touch_element(struct TouchEvent *event, enum ConfigControlElementIndex i) {
    s32 x_raw = CORRECT_TOUCH_X(event->x);
    s32 y = CORRECT_TOUCH_Y(event->y);
    ConfigControlElement *config = &configControlElements[i];

    config->y = y;

    switch (config->anchor) {
        case CONTROL_ELEMENT_LEFT:
            config->x = (x_raw - RECT_FROM_LEFT_EDGE(0)) >> 2;
            break;
        case CONTROL_ELEMENT_RIGHT:
            config->x = (RECT_FROM_RIGHT_EDGE(0) - x_raw) >> 2;
            break;
        case CONTROL_ELEMENT_CENTER:
        default:
            config->x = x_raw;
            break;
    }
}

// Custom pinch tracking — more reliable than SDL_MULTIGESTURE on iOS.
// Tracks the two most recent fingers and converts distance change into size delta.
typedef struct { SDL_TouchID id; f32 x, y; } PinchFinger;
static PinchFinger sPinchFingers[2] = {{0,0,0},{0,0,0}};
static f32 sLastPinchDist = -1.0f;
static f32 sPinchAccumulator = 0.0f;

static f32 pinch_compute_dist(void) {
    f32 dx = (sPinchFingers[0].x - sPinchFingers[1].x) * (f32)SCREEN_WIDTH_API;
    f32 dy = (sPinchFingers[0].y - sPinchFingers[1].y) * (f32)SCREEN_HEIGHT_API;
    return sqrtf(dx*dx + dy*dy);
}

static void pinch_finger_down(SDL_TouchID id, f32 x, f32 y) {
    for (int i = 0; i < 2; i++) {
        if (sPinchFingers[i].id == 0) {
            sPinchFingers[i].id = id;
            sPinchFingers[i].x = x;
            sPinchFingers[i].y = y;
            if (sPinchFingers[0].id != 0 && sPinchFingers[1].id != 0) {
                sLastPinchDist = pinch_compute_dist();
                sPinchAccumulator = 0.0f;
            }
            return;
        }
    }
}

static void pinch_finger_motion(SDL_TouchID id, f32 x, f32 y) {
    for (int i = 0; i < 2; i++) {
        if (sPinchFingers[i].id == id) {
            sPinchFingers[i].x = x;
            sPinchFingers[i].y = y;
            break;
        }
    }
    if (sPinchFingers[0].id == 0 || sPinchFingers[1].id == 0) return;
    if (!gInTouchConfig) return;
    if (gSelectedTouchElement >= TOUCH_COUNT) return;
    if (controlElements[gSelectedTouchElement].type == Mouse) return;

    f32 dist = pinch_compute_dist();
    if (sLastPinchDist < 0) { sLastPinchDist = dist; return; }
    f32 dDist = dist - sLastPinchDist;
    sLastPinchDist = dist;

    // Map pixel-distance delta to size delta. ~1 size unit per pixel of finger separation change.
    sPinchAccumulator += dDist * 0.5f;
    s32 delta = (s32)sPinchAccumulator;
    if (delta == 0) return;
    sPinchAccumulator -= (f32)delta;

    ConfigControlElement *elem = &configControlElements[gSelectedTouchElement];
    s32 newSize = (s32)elem->size + delta;
    if (newSize < 50) newSize = 50;
    if (newSize > 500) newSize = 500;
    elem->size = (u32)newSize;
}

static void pinch_finger_up(SDL_TouchID id) {
    for (int i = 0; i < 2; i++) {
        if (sPinchFingers[i].id == id) {
            sPinchFingers[i].id = 0;
            sLastPinchDist = -1.0f;
            sPinchAccumulator = 0.0f;
            break;
        }
    }
}

void touch_pinch(UNUSED f32 dDist) {
    // Kept for SDL_MULTIGESTURE wiring; custom tracking handles the actual pinch.
}

void touch_down(struct TouchEvent* event) {
    gGamepadActive = false;
    pinch_finger_down(event->touchID, event->x, event->y);
    struct Position pos;
    s32 size;
    for(u32 i = 0; i < controlElementsLength; i++) {
        if (controlElements[i].touchID == 0) {
            pos = get_pos(&configControlElements[i]);
            if (pos.y == HIDE_POS) continue;
            size = (s32)((f32)configControlElements[i].size / 100.0f * 50.0f);
            if (!TRIGGER_DETECT(size)) continue;
            switch (controlElements[i].type) {
                case Joystick:
                    controlElements[i].touchID = event->touchID;
                    gSelectedTouchElement = i;
                    if (!gInTouchConfig) {
                        controlElements[i].joyX = CORRECT_TOUCH_X(event->x) - pos.x;
                        controlElements[i].joyY = CORRECT_TOUCH_Y(event->y) - pos.y;
                    }
                    break;
                case Mouse:
                    controlElements[i].touchID = event->touchID;
                    break;
                case Button:
                    controlElements[i].touchID = event->touchID;
                    gSelectedTouchElement = i;
                    // messy
                    if (controlElements[i].buttonID == CHAT_BUTTON && !gInTouchConfig)
                        djui_interactable_on_key_down(configKeyChat[0]);
                    if (controlElements[i].buttonID == PLAYERLIST_BUTTON && !gInTouchConfig)
                        djui_interactable_on_key_down(configKeyPlayerList[0]);
                    break;
            }
        }
    }
}

void touch_motion(struct TouchEvent* event) {
    pinch_finger_motion(event->touchID, event->x, event->y);
    struct Position pos;
    s32 size;
    for(u32 i = 0; i < controlElementsLength; i++) {
        pos = get_pos(&configControlElements[i]);
        if (pos.y == HIDE_POS) continue;
        size = (s32)((f32)configControlElements[i].size / 100.0f * 50.0f);
        if (gInTouchConfig) {
            // Don't drag while pinching with 2 fingers
            bool pinching = (sPinchFingers[0].id != 0 && sPinchFingers[1].id != 0);
            if (!pinching && controlElements[i].touchID == event->touchID && controlElements[i].type != Mouse && gSelectedTouchElement == i) {
                move_touch_element(event, gSelectedTouchElement);
            }
        } else {
            if (!gDjuiPanelPauseCreated) {
                if (controlElements[i].touchID == event->touchID) {
                    s32 x, y;
                    switch (controlElements[i].type) {
                        case Joystick:
                            if (configPhantomTouch && !TRIGGER_DETECT(size * 6)) {
                                controlElements[i].joyX = 0;
                                controlElements[i].joyY = 0;
                                controlElements[i].touchID = 0;
                                break;
                            }
                            x = CORRECT_TOUCH_X(event->x) - pos.x;
                            y = CORRECT_TOUCH_Y(event->y) - pos.y;
                            if (pos.x + size / 2 < CORRECT_TOUCH_X(event->x))
                                x = size / 2;
                            if (pos.x - size / 2 > CORRECT_TOUCH_X(event->x))
                                x = - size / 2;
                            if (pos.y + size / 2 < CORRECT_TOUCH_Y(event->y))
                                y = size / 2;
                            if (pos.y - size / 2 > CORRECT_TOUCH_Y(event->y))
                                y = - size / 2;
                            controlElements[i].joyX = x;
                            controlElements[i].joyY = y;
                            break;
                        case Mouse:
                            if (configPhantomTouch && !TRIGGER_DETECT(size)) {
                                touch_x = before_x = 0;
                                touch_y = before_y = 0;
                                controlElements[i].touchID = 0;
                                break;
                            }
                            if (before_x > 0)
                                touch_x = CORRECT_TOUCH_X(event->x) - before_x;
                            if (before_y > 0)
                                touch_y = CORRECT_TOUCH_Y(event->y) - before_y;
                            before_x = CORRECT_TOUCH_X(event->x);
                            before_y = CORRECT_TOUCH_Y(event->y);
                            if ((u16)abs(touch_x) < configStickDeadzone / 4)
                                touch_x = 0;
                            if ((u16)abs(touch_y) < configStickDeadzone / 4)
                                touch_y = 0;
                            break;
                        case Button:
                            if ((controlElements[i].slideTouch && !TRIGGER_DETECT(size)) || (configPhantomTouch && !controlElements[i].slideTouch && !TRIGGER_DETECT(size * 3))) {
                                controlElements[i].slideTouch = 0;
                                controlElements[i].touchID = 0;
                            }
                            break;
                    }
                } else if ((TRIGGER_DETECT(size) || (configPhantomTouch && TRIGGER_DETECT(size * 6) && controlElements[i].type == Joystick)) && (controlElements[TOUCH_MOUSE].touchID != event->touchID || !configFreeCameraMouse) && configSlideTouch) {
                    if (configPhantomTouch)
                        controlElements[i].touchID = event->touchID;
                    switch (controlElements[i].type) {
                        case Joystick:
                            break;
                        case Mouse:
                            break;
                        case Button:
                            controlElements[i].slideTouch = 1;
                            controlElements[i].touchID = event->touchID;
                            if (controlElements[i].buttonID == CHAT_BUTTON)
                                djui_interactable_on_key_down(configKeyChat[0]);
                            if (controlElements[i].buttonID == PLAYERLIST_BUTTON)
                                djui_interactable_on_key_down(configKeyPlayerList[0]);
                            break;
                    }
                }
            }
        }
    }
}

static void handle_touch_up(u32 i) { // separated for when the layout changes
    controlElements[i].touchID = 0;
    struct Position pos = get_pos(&configControlElements[i]);
    if (pos.y == HIDE_POS) { return; }
    switch (controlElements[i].type) {
        case Joystick:
            controlElements[i].joyX = 0;
            controlElements[i].joyY = 0;
            break;
        case Mouse:
            touch_x = before_x = 0;
            touch_y = before_y = 0;
            break;
        case Button:
            if (controlElements[i].buttonID == CHAT_BUTTON && !gInTouchConfig)
                djui_interactable_on_key_up(configKeyChat[0]);
            if (controlElements[i].buttonID == PLAYERLIST_BUTTON && !gInTouchConfig)
                djui_interactable_on_key_up(configKeyPlayerList[0]);
            if (controlElements[i].buttonID == CONSOLE_BUTTON && !gInTouchConfig)
                djui_console_toggle();
            break;
    }
}

void touch_up(struct TouchEvent* event) {
    pinch_finger_up(event->touchID);
    for(u32 i = 0; i < controlElementsLength; i++) {
        if (controlElements[i].touchID == event->touchID) {
            handle_touch_up(i);
        }
    }
}

static void render_texture(const Texture *texture, s32 x, s32 y, u32 w, u32 h, f32 scale, u8 r, u8 g, u8 b, u8 a) {
    // Render via matrix-transformed quad (float precision) instead of gSPTextureRectangle
    // (integer dsdx quantization). The touch coord system is 4x SCREEN_WIDTH (U10.2-style),
    // so divide by 4 to get ortho-space SCREEN units. Ortho Y is flipped (up = +y).
    f32 full_w = (f32)w * scale;
    f32 full_h = (f32)h * scale;
    f32 cx = (f32)x * 0.25f;
    f32 cy = (f32)y * 0.25f;
    f32 tx = cx - full_w * 0.5f;
    f32 ty = (f32)SCREEN_HEIGHT - cy + full_h * 0.5f;

    create_dl_translation_matrix(DJUI_MTX_PUSH, tx, ty, 0);
    create_dl_scale_matrix(DJUI_MTX_NOPUSH, full_w, full_h, 1.0f);

    gDPSetEnvColor(gDisplayListHead++, r, g, b, a);
    djui_gfx_render_texture(texture, w, h, G_IM_FMT_RGBA, G_IM_SIZ_16b, false);

    gSPPopMatrix(gDisplayListHead++, G_MTX_MODELVIEW);
}

void render_touch_controls(void) {
    if ((gGamepadActive && configAutohideTouch) || (!gDjuiInMainMenu && gDjuiDisabled)) { return; }

    struct Position pos;
    struct Position stick;
    Colors color;
    f32 normalizedVectorMultiplier;
    
    create_dl_ortho_matrix();

    for (u32 i = 0; i < controlElementsLength; i++) {
        pos = get_pos(&configControlElements[i]);
        color = get_color(&configControlElements[i]);
        f32 scale = (f32)configControlElements[i].size / 100.0f;
        if (pos.y == HIDE_POS) continue;
        switch (controlElements[i].type) {
            case Joystick:
                if (absi(controlElements[i].joyX) + absi(controlElements[i].joyY) != 0) {
                    normalizedVectorMultiplier = sqrt((controlElements[i].joyX * controlElements[i].joyX) + (controlElements[i].joyY * controlElements[i].joyY))/(absi(controlElements[i].joyX) + absi(controlElements[i].joyY));
                } else {
                    normalizedVectorMultiplier = 0;
                }
                if (gInTouchConfig || gDjuiPanelPauseCreated) {
                    stick.x = 0;
                    stick.y = 0;
                } else {
                    stick.x = (controlElements[i].joyX * normalizedVectorMultiplier * 2);
                    stick.y = (controlElements[i].joyY * normalizedVectorMultiplier * 2);
                }
                render_texture(touch_textures[TEXTURE_TOUCH_JOYSTICK_BASE], pos.x, pos.y, 32, 32, scale, color.r, color.g, color.b, color.a);
                render_texture(touch_textures[TEXTURE_TOUCH_JOYSTICK], pos.x + stick.x, pos.y + stick.y, 16, 16, scale, color.r, color.g, color.b, color.a);
                break;
            case Mouse:
                break;
            case Button:
                if (!controlElements[i].touchID || gInTouchConfig || gDjuiPanelPauseCreated) {
                    render_texture(touch_textures[controlElements[i].buttonTexture.buttonUp], pos.x, pos.y, 16, 16, scale, color.r, color.g, color.b, color.a);
                } else {
                    render_texture(touch_textures[controlElements[i].buttonTexture.buttonDown], pos.x, pos.y, 16, 16, scale, color.r, color.g, color.b, color.a);
                }
                break;
        }
    }
}

static void touchscreen_init(void) {
    for (u32 i = 0; i < controlElementsLength; i++) {
        controlElements[i].touchID = 0;
        controlElements[i].joyX = 0;
        controlElements[i].joyY = 0;
        controlElements[i].slideTouch = 0;
    }
}

static void touchscreen_read(OSContPad *pad) {
    struct Position pos;
    s32 size;
    if (!gInTouchConfig && !gDjuiPanelPauseCreated) {
        for(u32 i = 0; i < controlElementsLength; i++) {
            pos = get_pos(&configControlElements[i]);
            size = (s32)((f32)configControlElements[i].size / 100.0f * 50.0f);
            if (pos.y == HIDE_POS) continue;
            switch (controlElements[i].type) {
                case Joystick:
                    if (controlElements[i].joyX || controlElements[i].joyY) {
                        pad->stick_x = (controlElements[i].joyX + size / 2) * 255 / size - 128;
                        pad->stick_y = (-controlElements[i].joyY + size / 2) * 255 / size - 128; //inverted for some reason
                    }
                    break;
                case Mouse:
                    break;
                case Button:
                    if (controlElements[i].touchID && controlElements[i].buttonID != CHAT_BUTTON && controlElements[i].buttonID != PLAYERLIST_BUTTON && controlElements[i].buttonID != CONSOLE_BUTTON) {
                        pad->button |= controlElements[i].buttonID;
                    }
                    break;
            }
        }
    }
}

// Used by other controller types for setting keybinds
// Doesn't make a huge amount of sense for a touchscreen,
// So instead I allow customizing all button positions in
// an entirely separate construction, which is fine for now
// until someone wants multiple copies of the same button,
// at which point I will have to decide how to do that
static u32 touchscreen_rawkey(void) { 
    return VK_INVALID;
}

struct ControllerAPI controller_touchscreen = {
    0,
    touchscreen_init,
    touchscreen_read,
    touchscreen_rawkey,
    NULL,
    NULL,
    NULL,
    NULL
};
#endif
