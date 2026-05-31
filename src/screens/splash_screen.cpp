#include "splash_screen.h"
#include "../ui/theme.h"
#include "config.h"
#include <Arduino.h>

// ─────────────────────────────────────────────────────────────────────────────
SplashScreen& SplashScreen::instance() {
    static SplashScreen s;
    return s;
}

// ─────────────────────────────────────────────────────────────────────────────
void SplashScreen::create(std::function<void()> onDone) {
    _onDone  = onDone;
    _startMs = millis();
    _done    = false;

    // ── Root screen ──────────────────────────────────────────────────────────
    _screen = lv_obj_create(nullptr);
    lv_obj_add_style(_screen, &Theme::style_screen, 0);

    // ── App title ────────────────────────────────────────────────────────────
    _title = lv_label_create(_screen);
    lv_obj_add_style(_title, &Theme::style_label_title, 0);
    lv_label_set_text(_title, "Home Assistant Remote");
    lv_obj_align(_title, LV_ALIGN_CENTER, 0, -28);
    lv_obj_set_style_text_align(_title, LV_TEXT_ALIGN_CENTER, 0);

    // ── Spinner ──────────────────────────────────────────────────────────────
    _spinner = lv_spinner_create(_screen, 1000, 60);
    lv_obj_set_size(_spinner, 56, 56);
    lv_obj_align(_spinner, LV_ALIGN_CENTER, 0, 12);
    lv_obj_set_style_arc_color(_spinner, CLR_ACCENT, LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(_spinner, CLR_SURFACE2, LV_PART_MAIN);
    lv_obj_set_style_arc_width(_spinner, 5, LV_PART_INDICATOR);
    lv_obj_set_style_arc_width(_spinner, 5, LV_PART_MAIN);

    // ── Version ──────────────────────────────────────────────────────────────
    _version = lv_label_create(_screen);
    lv_obj_add_style(_version, &Theme::style_label_dim, 0);
    lv_label_set_text(_version, "v" APP_VERSION);
    lv_obj_align(_version, LV_ALIGN_BOTTOM_RIGHT, -8, -8);

    lv_scr_load(_screen);
}

// ─────────────────────────────────────────────────────────────────────────────
void SplashScreen::update() {
    if (_done) return;
    if (millis() - _startMs >= SPLASH_DURATION_MS) {
        _done = true;
        if (_onDone) _onDone();
    }
}

// ─────────────────────────────────────────────────────────────────────────────
void SplashScreen::destroy() {
    _screen = nullptr;
    _title  = nullptr;
    _spinner = nullptr;
    _version = nullptr;
}
