#pragma once
#include <lvgl.h>
#include <functional>

/**
 * LoadingScreen — full-screen spinner with a status message.
 *
 * Used during:
 *   • WiFi connection attempt
 *   • Home Assistant connection test
 *   • Initial entity fetch
 *
 * The caller drives progress by calling setStatus() and, when done,
 * either calls onDone or destroy.
 */
class LoadingScreen {
public:
    static LoadingScreen& instance();

    void create(const char *initialMessage);
    void destroy();

    /** Update the status text. Call from your connection task / loop. */
    void setStatus(const char *msg);

    /** Briefly show a coloured result banner, then call onDone after delayMs. */
    void showResult(bool success, const char *msg,
                    uint32_t delayMs,
                    std::function<void()> onDone);

    void update();  // call every loop tick (drives result timer)

private:
    LoadingScreen() = default;

    lv_obj_t *_screen   = nullptr;
    lv_obj_t *_spinner  = nullptr;
    lv_obj_t *_lblMsg   = nullptr;
    lv_obj_t *_banner   = nullptr;

    std::function<void()> _resultCb;
    uint32_t _resultShowMs = 0;
    uint32_t _resultDelayMs = 0;
    bool     _waitingResult = false;
};
