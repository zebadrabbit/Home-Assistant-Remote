#pragma once
#include <lvgl.h>
#include <vector>
#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "config.h"

/**
 * App — singleton that owns:
 *   • LVGL initialisation (display driver + touch input driver)
 *   • Top-level state machine
 *   • Routing between screens / managers
 *
 * States
 * ──────
 *   SPLASH        Boot animation.  Auto-advances after SPLASH_DURATION_MS.
 *   CHECK_CONFIG  Reads NVS; routes to WIFI_SETUP, HA_SETUP, or LOADING.
 *   WIFI_SETUP    WiFi scan list + on-screen keyboard.
 *   WIFI_CONNECT  Attempting WiFi connection (loading screen).
 *   HA_SETUP      Web-portal or manual entry for HA server + token.
 *   HA_CONNECT    Testing HA connection + fetching entities (loading screen).
 *   DASHBOARD     Main tile grid.
 *   SETTINGS      Factory-reset / reconfigure dialog.
 */
enum class AppState {
    SPLASH,
    CHECK_CONFIG,
    WIFI_SETUP,
    WIFI_CONNECT,
    HA_SETUP,
    HA_CONNECT,
    DASHBOARD,
    SETTINGS
};

class App {
public:
    static App& instance();

    void init();    // call once from setup()
    void loop();    // call every iteration from loop()

private:
    App() = default;

    // ── State machine ────────────────────────────────────────────────────────
    AppState _state = AppState::SPLASH;
    void     transitionTo(AppState next);
    void     requestTransition(AppState next);
    void     processPendingTransition();
    void     processPendingDestroy();
    void     scheduleDestroy(AppState state);
    void     destroyState(AppState state);
    bool     _transitionPending = false;
    AppState _pendingState = AppState::SPLASH;
    std::vector<AppState> _pendingDestroys;
    uint8_t  _destroyDelayLoops = 0;

    // ── State handlers ───────────────────────────────────────────────────────
    void enterSplash();
    void enterCheckConfig();
    void enterWifiSetup();
    void enterWifiConnect(const String &ssid, const String &pass);
    void enterHaSetup();
    void enterHaConnect();
    void enterDashboard();
    void enterSettings();
    void startHaEntityFetch();
    static void haEntityFetchTask(void *param);

    // ── LVGL hardware bridge ─────────────────────────────────────────────────
    void initDisplay();
    void initTouch();
    void initLVGL();

    static void dispFlush(lv_disp_drv_t *drv, const lv_area_t *area,
                          lv_color_t *color_p);
    static void touchRead(lv_indev_drv_t *drv, lv_indev_data_t *data);
    bool onTouchActivity();
    void updateIdleMode();
    void enableIdleMode();
    void disableIdleMode();

    // ── Dashboard polling ────────────────────────────────────────────────────
    uint32_t _lastEntityPollMs = 0;
    uint32_t _entityPollIntervalMs = DEFAULT_REFRESH_INTERVAL_MS;
    uint32_t _settingsOpenedMs = 0;
    uint32_t _lastTouchActivityMs = 0;
    bool _idleModeActive = false;
    lv_obj_t *_idleOverlay = nullptr;

    // ── Async HA entity fetch ────────────────────────────────────────────────
    volatile bool _haFetchInProgress = false;
    volatile bool _haFetchDone = false;
    volatile bool _haFetchOk = false;
    TaskHandle_t _haFetchTask = nullptr;
    uint32_t _haFetchStartMs = 0;

    // ── Pending WiFi credentials (held between screens) ───────────────────────
    String _pendingSSID;
    String _pendingPass;
    bool _wifiSetupCanCancelToDashboard = false;
};
