#include "loading_screen.h"
#include "../ui/theme.h"
#include "config.h"
#include <Arduino.h>

// ─────────────────────────────────────────────────────────────────────────────
LoadingScreen& LoadingScreen::instance() {
    static LoadingScreen s;
    return s;
}

// ─────────────────────────────────────────────────────────────────────────────
void LoadingScreen::create(const char *initialMessage) {
    _waitingResult = false;

    _screen = lv_obj_create(nullptr);
    lv_obj_add_style(_screen, &Theme::style_screen, 0);

    // Centred spinner
    _spinner = lv_spinner_create(_screen, 1000, 60);
    lv_obj_set_size(_spinner, 60, 60);
    lv_obj_align(_spinner, LV_ALIGN_CENTER, 0, -20);
    lv_obj_set_style_arc_color(_spinner, CLR_ACCENT, LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(_spinner, CLR_SURFACE2, LV_PART_MAIN);
    lv_obj_set_style_arc_width(_spinner, 5, LV_PART_INDICATOR);
    lv_obj_set_style_arc_width(_spinner, 5, LV_PART_MAIN);

    // Status message below spinner
    _lblMsg = lv_label_create(_screen);
    lv_obj_add_style(_lblMsg, &Theme::style_label_body, 0);
    lv_obj_set_style_text_color(_lblMsg, CLR_TEXT_DIM, 0);
    lv_label_set_long_mode(_lblMsg, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(_lblMsg, SCREEN_W - 32);
    lv_obj_align(_lblMsg, LV_ALIGN_CENTER, 0, 30);
    lv_label_set_text(_lblMsg, initialMessage);
    lv_obj_set_style_text_align(_lblMsg, LV_TEXT_ALIGN_CENTER, 0);

    // App name (top-left, subtle)
    lv_obj_t *appLbl = lv_label_create(_screen);
    lv_obj_add_style(appLbl, &Theme::style_label_dim, 0);
    lv_label_set_text(appLbl, APP_NAME);
    lv_obj_align(appLbl, LV_ALIGN_TOP_LEFT, 8, 8);

    lv_scr_load(_screen);
}

// ─────────────────────────────────────────────────────────────────────────────
void LoadingScreen::setStatus(const char *msg) {
    if (_lblMsg) lv_label_set_text(_lblMsg, msg);
}

// ─────────────────────────────────────────────────────────────────────────────
void LoadingScreen::showResult(bool success, const char *msg,
                               uint32_t delayMs,
                               std::function<void()> onDone) {
    // Hide spinner
    if (_spinner) lv_obj_add_flag(_spinner, LV_OBJ_FLAG_HIDDEN);

    // Coloured banner
    _banner = lv_obj_create(_screen);
    lv_obj_set_size(_banner, SCREEN_W - 32, 50);
    lv_obj_align(_banner, LV_ALIGN_CENTER, 0, -10);
    lv_obj_set_style_radius(_banner, 10, 0);
    lv_obj_set_style_border_width(_banner, 0, 0);
    lv_obj_set_style_bg_color(_banner, success ? CLR_OK : CLR_ERR, 0);
    lv_obj_set_style_bg_opa(_banner, LV_OPA_COVER, 0);

    lv_obj_t *icon = lv_label_create(_banner);
    lv_obj_set_style_text_font(icon, FONT_XL, 0);
    lv_obj_set_style_text_color(icon, CLR_BG, 0);
    lv_label_set_text(icon, success ? LV_SYMBOL_OK : LV_SYMBOL_CLOSE);
    lv_obj_align(icon, LV_ALIGN_LEFT_MID, 10, 0);

    lv_obj_t *bannerLbl = lv_label_create(_banner);
    lv_obj_add_style(bannerLbl, &Theme::style_label_body, 0);
    lv_obj_set_style_text_color(bannerLbl, CLR_BG, 0);
    lv_label_set_text(bannerLbl, msg);
    lv_obj_align_to(bannerLbl, icon, LV_ALIGN_OUT_RIGHT_MID, 10, 0);

    _resultCb      = onDone;
    _resultShowMs  = millis();
    _resultDelayMs = delayMs;
    _waitingResult = true;
}

// ─────────────────────────────────────────────────────────────────────────────
void LoadingScreen::update() {
    if (!_waitingResult) return;
    if (millis() - _resultShowMs >= _resultDelayMs) {
        _waitingResult = false;
        if (_resultCb) _resultCb();
    }
}

// ─────────────────────────────────────────────────────────────────────────────
void LoadingScreen::destroy() {
    _screen = nullptr;
    _spinner = _lblMsg = _banner = nullptr;
    _waitingResult = false;
}
