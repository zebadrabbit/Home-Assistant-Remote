#pragma once
#include <lvgl.h>
#include <functional>

/**
 * SplashScreen — shown on boot for SPLASH_DURATION_MS.
 *
 * Displays: app title text, spinner, and version.
 * On completion calls the onDone callback.
 */
class SplashScreen {
public:
    static SplashScreen& instance();

    void create(std::function<void()> onDone);
    void destroy();
    void update();   // call every loop tick

private:
    SplashScreen() = default;

    lv_obj_t *_screen   = nullptr;
    lv_obj_t *_title    = nullptr;
    lv_obj_t *_spinner  = nullptr;
    lv_obj_t *_version  = nullptr;

    std::function<void()> _onDone;
    uint32_t _startMs = 0;
    bool     _done    = false;
};
