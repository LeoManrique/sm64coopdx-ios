#include <string.h>
#include "djui.h"
#include "djui_panel.h"
#include "djui_panel_menu.h"
#include "djui_panel_rules.h"
#include "djui_panel_join_lobbies.h"

#ifdef COOPNET
static char sRules[1024];

static void djui_panel_rules_deny(struct DjuiBase* caller) {
    djui_panel_menu_back(caller);
}

static void djui_panel_rules_accept(struct DjuiBase* caller) {
    configRulesVersion = RULES_VERSION;
    djui_panel_join_lobbies_create(caller, "");
}

#ifdef TARGET_IOS
// Touch-scrollable rules panel for small screens.
// All content (text + buttons) lives in a single scroll container.
// Touch-dragging on the body scrolls the container.

static f32 sRulesScrollY = 0;
static f32 sRulesTouchStartY = 0;
static struct DjuiBase* sRulesScrollContainer = NULL;
static f32 sRulesContentHeight = 0;

static void rules_scroll_begin(UNUSED struct DjuiBase* base, UNUSED bool inputCursor) {
    sRulesTouchStartY = gCursorY;
}

static void rules_scroll_update(struct DjuiBase* base) {
    f32 delta = sRulesTouchStartY - gCursorY;
    sRulesScrollY += delta;
    sRulesTouchStartY = gCursorY;

    // elem.height is the body's actual visible height (comp.height gets
    // modified by the flow layout as children are rendered, so it's wrong here)
    f32 bodyH = base->elem.height;
    if (bodyH <= 0) { return; }

    // account for the 16px flow-layout top margin
    f32 maxScroll = sRulesContentHeight + 16 - bodyH;
    if (maxScroll < 0) { maxScroll = 0; }
    if (sRulesScrollY < 0) { sRulesScrollY = 0; }
    if (sRulesScrollY > maxScroll) { sRulesScrollY = maxScroll; }

    if (sRulesScrollContainer) {
        sRulesScrollContainer->y.value = -sRulesScrollY;
    }
}

static void rules_scroll_end(UNUSED struct DjuiBase* base) {}

static bool rules_body_render(struct DjuiBase* base) {
    // draw background
    djui_rect_render(base);

    // draw scrollbar indicator when content overflows
    f32 bodyH = base->elem.height;
    f32 contentH = sRulesContentHeight + 16;
    if (contentH <= bodyH || bodyH <= 0) { return true; }

    f32 maxScroll = contentH - bodyH;
    f32 barH = bodyH * (bodyH / contentH);
    if (barH < 20) { barH = 20; }

    f32 barY = (maxScroll > 0) ? sRulesScrollY / maxScroll * (bodyH - barH) : 0;

    struct DjuiBaseRect barClip;
    barClip.x      = base->elem.x + base->elem.width;
    barClip.y      = base->elem.y + barY;
    barClip.width  = 4;
    barClip.height = barH;

    f32 tx = barClip.x, ty = barClip.y;
    djui_gfx_position_translate(&tx, &ty);
    create_dl_translation_matrix(DJUI_MTX_PUSH, tx, ty, 0);

    f32 tw = barClip.width, th = barClip.height;
    djui_gfx_scale_translate(&tw, &th);
    create_dl_scale_matrix(DJUI_MTX_NOPUSH, tw, th, 1.0f);

    gDPSetEnvColor(gDisplayListHead++, 200, 200, 200, 120);
    gSPDisplayList(gDisplayListHead++, dl_djui_simple_rect);
    gSPPopMatrix(gDisplayListHead++, G_MTX_MODELVIEW);

    return true;
}
#endif

void djui_panel_rules_create(struct DjuiBase* caller) {
    struct DjuiThreePanel* panel = djui_panel_menu_create(DLANG(RULES, RULES_TITLE), false);
    struct DjuiBase* body = djui_three_panel_get_body(panel);
    f32 textWidth = (DJUI_DEFAULT_PANEL_WIDTH * (configDjuiThemeCenter ? DJUI_THEME_CENTERED_WIDTH : 1)) - 64;

    snprintf(
        sRules, 1024,
        "%s\n%s\n%s\n%s\n%s\n%s",
        DLANG(RULES, RULE_1),
        DLANG(RULES, RULE_2),
        DLANG(RULES, RULE_3),
        DLANG(RULES, RULE_4),
        DLANG(RULES, RULE_5),
        DLANG(RULES, RULE_6)
    );

#ifdef TARGET_IOS
    {
        sRulesScrollY = 0;

        f32 rulesH   = 12 * 30;
        f32 subjectH = 2  * 30;
        f32 noticeH  = 3  * 30;
        f32 btnH     = 64;
        f32 gap      = 16;

        // Calculate total content height and build the scroll container
        f32 yPos = 0;
        sRulesContentHeight = rulesH + gap + subjectH;
        if (configRulesVersion != RULES_VERSION) {
            sRulesContentHeight += gap + noticeH;
        }
        sRulesContentHeight += gap + btnH;

        struct DjuiRect* sc = djui_rect_container_create(body, sRulesContentHeight);
        sRulesScrollContainer = &sc->base;

        // --- rules text ---
        struct DjuiText* text1 = djui_text_create(&sc->base, sRules);
        djui_base_set_location(&text1->base, 0, yPos);
        djui_base_set_size(&text1->base, textWidth, rulesH);
        djui_base_set_color(&text1->base, 220, 220, 220, 255);
        djui_text_set_drop_shadow(text1, 64, 64, 64, 100);
        djui_text_set_alignment(text1, DJUI_HALIGN_LEFT, DJUI_VALIGN_TOP);
        yPos += rulesH + gap;

        // --- "subject to change" ---
        struct DjuiText* text2 = djui_text_create(&sc->base, DLANG(RULES, SUBJECT_TO_CHANGE));
        djui_base_set_location(&text2->base, 0, yPos);
        djui_base_set_size(&text2->base, textWidth, subjectH);
        djui_base_set_color(&text2->base, 220, 220, 220, 255);
        djui_text_set_drop_shadow(text2, 64, 64, 64, 100);
        djui_text_set_alignment(text2, DJUI_HALIGN_CENTER, DJUI_VALIGN_CENTER);
        yPos += subjectH;

        // --- notice (only when rules not yet accepted) ---
        if (configRulesVersion != RULES_VERSION) {
            yPos += gap;
            struct DjuiText* text3 = djui_text_create(&sc->base, DLANG(RULES, NOTICE));
            djui_base_set_location(&text3->base, 0, yPos);
            djui_base_set_size(&text3->base, textWidth, noticeH);
            djui_base_set_color(&text3->base, 220, 220, 220, 255);
            djui_text_set_drop_shadow(text3, 64, 64, 64, 100);
            djui_text_set_alignment(text3, DJUI_HALIGN_CENTER, DJUI_VALIGN_CENTER);
            yPos += noticeH;
        }
        yPos += gap;

        // --- buttons ---
        struct DjuiButton* defaultBtn;
        if (configRulesVersion != RULES_VERSION) {
            struct DjuiRect* rect = djui_rect_container_create(&sc->base, btnH);
            djui_base_set_location(&rect->base, 0, yPos);
            defaultBtn = djui_button_left_create(&rect->base, DLANG(MENU, BACK), DJUI_BUTTON_STYLE_BACK, djui_panel_rules_deny);
            djui_button_right_create(&rect->base, DLANG(MENU, YES), DJUI_BUTTON_STYLE_NORMAL, djui_panel_rules_accept);
        } else {
            defaultBtn = djui_button_create(&sc->base, DLANG(MENU, BACK), DJUI_BUTTON_STYLE_BACK, djui_panel_menu_back);
            djui_base_set_location(&defaultBtn->base, 0, yPos);
        }

        // Touch-scroll on the body
        djui_interactable_create(body, NULL);
        djui_interactable_hook_cursor_down(body,
            rules_scroll_begin,
            rules_scroll_update,
            rules_scroll_end);

        // Custom render for scrollbar indicator
        body->render = rules_body_render;

        panel->temporary = true;
        djui_panel_add(caller, panel, &defaultBtn->base);
    }
#else
    {
        struct DjuiText* text1 = djui_text_create(body, sRules);
        djui_base_set_location(&text1->base, 0, 0);
        djui_base_set_size(&text1->base, textWidth, 12 * 30);
        djui_base_set_color(&text1->base, 220, 220, 220, 255);
        djui_text_set_drop_shadow(text1, 64, 64, 64, 100);
        djui_text_set_alignment(text1, DJUI_HALIGN_LEFT, DJUI_VALIGN_TOP);

        struct DjuiText* text2 = djui_text_create(body, DLANG(RULES, SUBJECT_TO_CHANGE));
        djui_base_set_location(&text2->base, 0, 0);
        djui_base_set_size(&text2->base, textWidth, 2 * 30);
        djui_base_set_color(&text2->base, 220, 220, 220, 255);
        djui_text_set_drop_shadow(text2, 64, 64, 64, 100);
        djui_text_set_alignment(text2, DJUI_HALIGN_CENTER, DJUI_VALIGN_CENTER);

        if (configRulesVersion != RULES_VERSION) {
            struct DjuiText* text3 = djui_text_create(body, DLANG(RULES, NOTICE));
            djui_base_set_location(&text3->base, 0, 0);
            djui_base_set_size(&text3->base, textWidth, 3 * 30);
            djui_base_set_color(&text3->base, 220, 220, 220, 255);
            djui_text_set_drop_shadow(text3, 64, 64, 64, 100);
            djui_text_set_alignment(text3, DJUI_HALIGN_CENTER, DJUI_VALIGN_CENTER);

            struct DjuiRect* rect = djui_rect_container_create(body, 64);
            {
                djui_button_left_create(&rect->base, DLANG(MENU, BACK), DJUI_BUTTON_STYLE_BACK, djui_panel_rules_deny);
                djui_button_right_create(&rect->base, DLANG(MENU, YES), DJUI_BUTTON_STYLE_NORMAL, djui_panel_rules_accept);
            }
        } else {
            djui_button_create(body, DLANG(MENU, BACK), DJUI_BUTTON_STYLE_BACK, djui_panel_menu_back);
        }

        panel->temporary = true;
        djui_panel_add(caller, panel, NULL);
    }
#endif
}
#endif
