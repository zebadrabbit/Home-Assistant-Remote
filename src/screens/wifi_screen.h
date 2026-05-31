#pragma once
#include <lvgl.h>
#include <functional>
#include <vector>
#include "../managers/wifi_manager.h"

/**
 * WiFiScreen — 2-step WiFi setup wizard.
 *
 * Step 1 — Network list: Scan and show SSIDs as a scrollable list.
 *           Tap a network to proceed to Step 2.
 *
 * Step 2 — Password entry: On-screen LVGL keyboard.
 *           "Connect" saves credentials and calls onConnected.
 *           "Back" returns to Step 1.
 *
 * Callbacks:
 *   onConnected(ssid, pass) — WiFi credentials accepted; caller
 *                             should attempt the actual connection.
 */
class WiFiScreen {
public:
    static WiFiScreen& instance();

    void create(std::function<void(String ssid, String pass)> onConnected);
    void destroy();
    void update();   // pump scan / UI state

private:
    WiFiScreen() = default;

    // ── Screens / panels ─────────────────────────────────────────────────────
    lv_obj_t *_screen     = nullptr;
    lv_obj_t *_list       = nullptr;    // Step 1: network list
    lv_obj_t *_passPanel  = nullptr;    // Step 2: password panel
    lv_obj_t *_keyboard   = nullptr;
    lv_obj_t *_taPass     = nullptr;    // textarea for password
    lv_obj_t *_lblSsid    = nullptr;    // selected SSID label
    lv_obj_t *_lblStatus  = nullptr;    // status / error text
    lv_obj_t *_spinner    = nullptr;    // scanning indicator

    std::vector<WiFiNetwork> _networks;
    String _selectedSsid;
    std::function<void(String, String)> _onConnected;

    bool _scanning = false;
    uint32_t _scanStartMs = 0;

    // ── Internal helpers ─────────────────────────────────────────────────────
    void showNetworkList();
    void showPasswordPanel(const String &ssid);
    void populateList();
    void startScan();

    // ── LVGL callbacks ───────────────────────────────────────────────────────
    static void onNetworkTap(lv_event_t *e);
    static void onConnectBtn(lv_event_t *e);
    static void onBackBtn(lv_event_t *e);
    static void onRescanBtn(lv_event_t *e);
};
