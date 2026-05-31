#pragma once
#include <lvgl.h>
#include <WebServer.h>
#include <functional>
#include "../managers/storage_manager.h"

/**
 * HASetupScreen — collects Home Assistant server URL and Long-Lived Token.
 *
 * UX strategy: once the device has WiFi, it starts a small HTTP server on
 * port 80 and shows the device IP on screen.  The user opens that address in
 * a browser on their phone, fills in the form, and submits.  This avoids
 * typing a 191-character token on a tiny keyboard.
 *
 * A "Manual entry" toggle is also available for users without a phone handy.
 *
 * Callbacks:
 *   onSaved(url, token) — credentials have been persisted; caller should
 *                         advance to the loading / connect state.
 */
class HASetupScreen {
public:
    static HASetupScreen& instance();

    void create(const String &deviceIP,
                std::function<void(String url, String token)> onSaved);
    void destroy();
    void update();   // must be called every loop iteration

private:
    HASetupScreen() = default;

    lv_obj_t *_screen       = nullptr;
    lv_obj_t *_lblIP        = nullptr;
    lv_obj_t *_lblStatus    = nullptr;
    lv_obj_t *_manualPanel  = nullptr;
    lv_obj_t *_taUrl        = nullptr;
    lv_obj_t *_taToken      = nullptr;
    lv_obj_t *_keyboard     = nullptr;

    WebServer *_server      = nullptr;
    bool       _serverReady = false;
    String     _deviceIP;

    std::function<void(String, String)> _onSaved;

    void startWebServer();
    void stopWebServer();
    void serveHomePage();
    void showManualPanel();
    void serveSetupPage();
    void handleSave();
    void serveConfigPage();
    void handleSaveConfig();

    static void onManualBtn(lv_event_t *e);
    static void onManualSaveBtn(lv_event_t *e);
};
