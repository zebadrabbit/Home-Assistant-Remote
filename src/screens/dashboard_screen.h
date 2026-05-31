#pragma once
#include <lvgl.h>
#include <functional>
#include <vector>
#include "../managers/ha_client.h"

/**
 * DashboardScreen — main entity tile grid.
 *
 * Layout:
 *   ┌────────────────────────────────┐
 *   │  status bar (WiFi, time, HA)   │  24 px
 *   ├──────┬──────┬──────────────────┤
 *   │ tile │ tile │ tile  …          │  scrollable rows
 *   │      │      │                  │
 *   └──────┴──────┴──────────────────┘
 *
 * Each tile shows: domain icon, friendly name (truncated), state.
 * Tapping a controllable tile sends a toggle service call.
 * Pull-to-refresh re-fetches entities.
 * Long-press on status bar → settings (factory reset / re-setup).
 *
 * Callbacks:
 *   onSettingsRequested() — user wants to open settings
 */
class DashboardScreen {
public:
    static DashboardScreen& instance();

    void create(std::function<void()> onSettingsRequested,
                std::function<void()> onWifiReconfigureRequested = nullptr);
    void destroy();

    /** Rebuild the tile grid from the current HAClient entity list. */
    void populate(const std::vector<HAEntity> &entities, bool clearPending = true);

    /** Update the status bar WiFi RSSI and HA connection indicator. */
    void updateStatusBar(bool haConnected, int32_t rssi);

    /** Called every loop to drive polling timer. */
    void update();

private:
    DashboardScreen() = default;

    lv_obj_t *_screen     = nullptr;
    lv_obj_t *_statusBar  = nullptr;
    lv_obj_t *_lblWifi    = nullptr;
    lv_obj_t *_lblHA      = nullptr;
    lv_obj_t *_grid       = nullptr;   // scrollable container for tiles
    lv_obj_t *_lightPopup = nullptr;
    lv_obj_t *_weatherPopup = nullptr;
    lv_obj_t *_wifiPopup = nullptr;
    lv_obj_t *_sliderBrightness = nullptr;
    lv_obj_t *_sliderKelvin = nullptr;
    lv_obj_t *_colorWheel = nullptr;
    lv_obj_t *_lblBrightness = nullptr;
    lv_obj_t *_lblKelvin = nullptr;
    lv_obj_t *_lblMode = nullptr;
    lv_obj_t *_btnModeColor = nullptr;
    lv_obj_t *_btnModeWhite = nullptr;

    String _selectedLightId;
    uint8_t _selectedR = 255;
    uint8_t _selectedG = 255;
    uint8_t _selectedB = 255;
    bool _useKelvin = false;
    uint8_t _lightPrefBrightnessPct = 75;
    uint16_t _lightPrefKelvin = 3500;
    uint8_t _lightPrefR = 255;
    uint8_t _lightPrefG = 147;
    uint8_t _lightPrefB = 41;
    bool _lightPrefUseKelvin = false;
    uint32_t _suppressTileTapUntilMs = 0;
    std::vector<String> _pendingEntityIds;

    std::function<void()> _onSettings;
    std::function<void()> _onWifiReconfigure;

    uint32_t _lastPollMs = 0;

    // ── Tile helpers ─────────────────────────────────────────────────────────
    void buildTile(lv_obj_t *parent, const HAEntity &entity, int idx);
    bool isEntityPending(const String &entityId) const;
    void markEntityPending(const String &entityId);
    void clearPendingEntities();
    void openLightPopup(const HAEntity &entity);
    void closeLightPopup();
    void openWeatherPopup();
    void closeWeatherPopup();
    void openWifiInfoPopup();
    void closeWifiInfoPopup();
    void applyLightPopup();
    void updateLightPopupModeUi();

    // ── LVGL callbacks ───────────────────────────────────────────────────────
    static void onTileTap(lv_event_t *e);
    static void onTileLongPress(lv_event_t *e);
    static void onSettingsShortPress(lv_event_t *e);
    static void onStatusBarLongPress(lv_event_t *e);
    static void onWifiShortPress(lv_event_t *e);
    static void onWifiLongPress(lv_event_t *e);
    static void onColorModeBtn(lv_event_t *e);
    static void onWhiteModeBtn(lv_event_t *e);
    static void onApplyBtn(lv_event_t *e);
    static void onCancelBtn(lv_event_t *e);
    static void onCloseWeatherBtn(lv_event_t *e);
    static void onCloseWifiBtn(lv_event_t *e);
    static void onBrightnessChanged(lv_event_t *e);
    static void onKelvinChanged(lv_event_t *e);
    static void onColorWheelChanged(lv_event_t *e);
};
