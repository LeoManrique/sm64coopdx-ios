#ifdef TOUCH_CONTROLS
#include "djui.h"
#include "djui_panel.h"
#include "djui_panel_menu.h"
#include "pc/configfile.h"
#include "pc/controller/controller_touchscreen.h"

// Same things as djui panel player
static struct DjuiSlider *sTouchConfigSliderR = NULL;
static struct DjuiSlider *sTouchConfigSliderG = NULL;
static struct DjuiSlider *sTouchConfigSliderB = NULL;
static struct DjuiSlider *sTouchConfigSliderA = NULL;
static struct DjuiSlider *sTouchConfigSliderS = NULL;

static struct DjuiSelectionbox* sTouchConfigSelectionboxAnchor = NULL;

static void djui_panel_touch_controls_editor_update_values(struct DjuiBase* caller) {
    struct DjuiSelectionbox* selectionbox = (struct DjuiSelectionbox*)caller;
    bool enabled = *selectionbox->value != TOUCH_MOUSE ? true : false;
    djui_base_set_enabled(&sTouchConfigSelectionboxAnchor->base, enabled);
    djui_base_set_enabled(&sTouchConfigSliderR->base, enabled);
    djui_base_set_enabled(&sTouchConfigSliderG->base, enabled);
    djui_base_set_enabled(&sTouchConfigSliderB->base, enabled);
    djui_base_set_enabled(&sTouchConfigSliderA->base, enabled);
    djui_base_set_enabled(&sTouchConfigSliderS->base, enabled);

    sTouchConfigSelectionboxAnchor->value = &configControlElements[*selectionbox->value].anchor;
    sTouchConfigSliderR->value = &configControlElements[*selectionbox->value].r;
    sTouchConfigSliderG->value = &configControlElements[*selectionbox->value].g;
    sTouchConfigSliderB->value = &configControlElements[*selectionbox->value].b;
    sTouchConfigSliderA->value = &configControlElements[*selectionbox->value].a;
    sTouchConfigSliderS->value = &configControlElements[*selectionbox->value].size;
    djui_selectionbox_update_value(&sTouchConfigSelectionboxAnchor->base);
    djui_slider_update_value(&sTouchConfigSliderR->base);
    djui_slider_update_value(&sTouchConfigSliderG->base);
    djui_slider_update_value(&sTouchConfigSliderB->base);
    djui_slider_update_value(&sTouchConfigSliderA->base);
    djui_slider_update_value(&sTouchConfigSliderS->base);
}

static void djui_panel_touch_controls_editor_update_anchor(struct DjuiBase* caller) {
    struct DjuiSelectionbox* selectionbox = (struct DjuiSelectionbox*)caller;
    bool enabled = *selectionbox->value != CONTROL_ELEMENT_HIDDEN ? true : false;
    djui_base_set_enabled(&sTouchConfigSliderR->base, enabled);
    djui_base_set_enabled(&sTouchConfigSliderG->base, enabled);
    djui_base_set_enabled(&sTouchConfigSliderB->base, enabled);
    djui_base_set_enabled(&sTouchConfigSliderA->base, enabled);
    djui_base_set_enabled(&sTouchConfigSliderS->base, enabled);
}

static struct DjuiButton* sFloatingDoneButton = NULL;
static f32 sMoveStartCursorX = 0;
static f32 sMoveStartCursorY = 0;
static f32 sMoveStartBtnX = 0;
static f32 sMoveStartBtnY = 0;
static bool sMoveDragging = false;

static void move_floating_destroy(void) {
    if (sFloatingDoneButton) {
        djui_base_destroy(&sFloatingDoneButton->base);
        sFloatingDoneButton = NULL;
    }
}

static void move_done_begin(struct DjuiBase* base, UNUSED bool inputCursor) {
    (void)base;
    sMoveStartCursorX = gCursorX;
    sMoveStartCursorY = gCursorY;
    sMoveStartBtnX = sFloatingDoneButton->base.x.value;
    sMoveStartBtnY = sFloatingDoneButton->base.y.value;
    sMoveDragging = false;
}

static void move_done_update(UNUSED struct DjuiBase* base) {
    f32 dx = gCursorX - sMoveStartCursorX;
    f32 dy = gCursorY - sMoveStartCursorY;
    if (dx * dx + dy * dy > 100.0f) { sMoveDragging = true; }
    if (sMoveDragging && sFloatingDoneButton) {
        sFloatingDoneButton->base.x.value = sMoveStartBtnX + dx;
        sFloatingDoneButton->base.y.value = sMoveStartBtnY + dy;
    }
}

static void move_done_end(struct DjuiBase* base) {
    if (!sMoveDragging) {
        move_floating_destroy();
        djui_panel_menu_back(base);
    }
}

static void djui_panel_move_on_destroy(UNUSED struct DjuiBase* base) {
    move_floating_destroy();
}

static void djui_panel_touch_controls_editor_move(struct DjuiBase* caller) {
    // Create an invisible panel just for the back-navigation stack
    struct DjuiThreePanel* panel = djui_panel_menu_create("", false);
    djui_base_set_size(&panel->base, 1, 1);
    djui_base_set_color(&panel->base, 0, 0, 0, 0);
    djui_base_set_border_color(&panel->base, 0, 0, 0, 0);
    struct DjuiPanel* p = djui_panel_add(caller, panel, NULL);
    if (p) { p->on_panel_destroy = djui_panel_move_on_destroy; }

    // Create a floating Done button on the root, free from panel positioning
    sFloatingDoneButton = djui_button_create(&gDjuiRoot->base, "Done", DJUI_BUTTON_STYLE_BACK, NULL);
    djui_base_set_size_type(&sFloatingDoneButton->base, DJUI_SVT_ABSOLUTE, DJUI_SVT_ABSOLUTE);
    djui_base_set_size(&sFloatingDoneButton->base, 200, 64);
    djui_base_set_location_type(&sFloatingDoneButton->base, DJUI_SVT_ABSOLUTE, DJUI_SVT_ABSOLUTE);
    djui_base_set_location(&sFloatingDoneButton->base, gDjuiRoot->base.elem.width / 2.0f - 100, gDjuiRoot->base.elem.height / 2.0f - 32);
    djui_interactable_hook_cursor_down(&sFloatingDoneButton->base, move_done_begin, move_done_update, move_done_end);
}

static bool djui_panel_touch_controls_editor_leave_menu(UNUSED struct DjuiBase* base) {
    gInTouchConfig = false;
    return false;
}

static void djui_panel_touch_controls_editor_set_alpha(UNUSED struct DjuiBase* base) {
    u8 alpha = configControlElements[gSelectedTouchElement].a;
    for (s32 i = 0; i < TOUCH_COUNT; i++) {
        configControlElements[i].a = alpha;
    }
}

void djui_panel_touch_controls_editor_create(struct DjuiBase* caller) {
    struct DjuiThreePanel* panel = djui_panel_menu_create(DLANG(CONTROLS, CONTROLS), false);
    struct DjuiBase* body = djui_three_panel_get_body(panel);

    gInTouchConfig = true;
    gSelectedTouchElement = TOUCH_MOUSE;
    {
        djui_checkbox_create(body, "Snap To Grid"/*DLANG(TOUCH_CONTROLS, TOUCH_CONTROLS_SNAP)*/, &configSnapTouch, NULL);

        char* buttonChoices[21] = { "Joystick", "None", "A Button", "B Button", "X Button", "Y Button", "Start Button", "L Trigger", "R Trigger", "Z Trigger", "C-Up", "C-Down", "C-Left", "C-Right", "Chat", "Playerlist", "Dpad-Up", "Dpad-Down", "Dpad-Left", "Dpad-Right", "Console" };
        djui_selectionbox_create(body, "Selected Button"/*DLANG(TOUCH_CONTROLS, TOUCH_CONTROLS_BUTTON)*/, buttonChoices, 21, &gSelectedTouchElement, djui_panel_touch_controls_editor_update_values);
        char* anchorChoices[4] = { "Left", "Right", "Center", "Hidden" /*DLANG(TOUCH_CONTROLS, TOUCH_CONTROLS_ANCHOR_LEFT), DLANG(TOUCH_CONTROLS, TOUCH_CONTROLS_ANCHOR_RIGHT), DLANG(TOUCH_CONTROLS, TOUCH_CONTROLS_ANCHOR_CENTER), DLANG(TOUCH_CONTROLS, TOUCH_CONTROLS_ANCHOR_HIDDEN)*/ };
        sTouchConfigSelectionboxAnchor = djui_selectionbox_create(body, "Anchor"/*DLANG(TOUCH_CONTROLS, TOUCH_CONTROLS_ANCHOR)*/, anchorChoices, 4, &configControlElements[gSelectedTouchElement].anchor, djui_panel_touch_controls_editor_update_anchor);

        sTouchConfigSliderR = djui_slider_create(body, DLANG(PLAYER, RED), &configControlElements[gSelectedTouchElement].r, 0, 255, NULL);
        djui_base_set_color(&sTouchConfigSliderR->rectValue->base, 255, 0, 0, 255);
        sTouchConfigSliderR->updateRectValueColor = false;

        sTouchConfigSliderG = djui_slider_create(body, DLANG(PLAYER, GREEN), &configControlElements[gSelectedTouchElement].g, 0, 255, NULL);
        djui_base_set_color(&sTouchConfigSliderG->rectValue->base, 0, 255, 0, 255);
        sTouchConfigSliderG->updateRectValueColor = false;

        sTouchConfigSliderB = djui_slider_create(body, DLANG(PLAYER, BLUE), &configControlElements[gSelectedTouchElement].b, 0, 255, NULL);
        djui_base_set_color(&sTouchConfigSliderB->rectValue->base, 0, 0, 255, 255);
        sTouchConfigSliderB->updateRectValueColor = false;
        
        sTouchConfigSliderA = djui_slider_create(body, DLANG(TOUCH_CONTROLS, TOUCH_CONTROLS_OPACITY), &configControlElements[gSelectedTouchElement].a, 0, 255, NULL);
        
        sTouchConfigSliderS = djui_slider_create(body, DLANG(TOUCH_CONTROLS, TOUCH_CONTROLS_SCALE), &configControlElements[gSelectedTouchElement].size, 1, 2, NULL);

        if (gSelectedTouchElement == TOUCH_MOUSE) {
            djui_base_set_enabled(&sTouchConfigSelectionboxAnchor->base, false);
            djui_base_set_enabled(&sTouchConfigSliderR->base, false);
            djui_base_set_enabled(&sTouchConfigSliderG->base, false);
            djui_base_set_enabled(&sTouchConfigSliderB->base, false);
            djui_base_set_enabled(&sTouchConfigSliderA->base, false);
            djui_base_set_enabled(&sTouchConfigSliderS->base, false);
        }

        djui_button_create(body, "Apply Opacity to All", DJUI_BUTTON_STYLE_NORMAL, djui_panel_touch_controls_editor_set_alpha);
        djui_button_create(body, "Move", DJUI_BUTTON_STYLE_NORMAL, djui_panel_touch_controls_editor_move);
        djui_button_create(body, DLANG(MENU, BACK), DJUI_BUTTON_STYLE_BACK, djui_panel_menu_back);
    }

    panel->on_back = djui_panel_touch_controls_editor_leave_menu; 
    djui_panel_add(caller, panel, NULL);
}
#endif