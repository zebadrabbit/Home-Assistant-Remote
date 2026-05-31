#include "dashboard_screen.h"
#include "../ui/theme.h"
#include "../managers/ha_client.h"
#include "../managers/wifi_manager.h"
#include "../managers/storage_manager.h"
#include "config.h"
#include <cstdint>

static const char* weatherIconForCondition(const String &condition) {
    String c = condition;
    c.toLowerCase();

    if (c.indexOf("thunder") >= 0 || c.indexOf("storm") >= 0 || c.indexOf("lightning") >= 0) {
        return LV_SYMBOL_CHARGE;
    }
    if (c.indexOf("rain") >= 0 || c.indexOf("shower") >= 0 || c.indexOf("drizzle") >= 0) {
        return LV_SYMBOL_TINT;
    }
    if (c.indexOf("snow") >= 0 || c.indexOf("sleet") >= 0 || c.indexOf("hail") >= 0) {
        return LV_SYMBOL_WARNING;
    }
    if (c.indexOf("wind") >= 0 || c.indexOf("breeze") >= 0) {
        return LV_SYMBOL_LOOP;
    }
    if (c.indexOf("cloud") >= 0 || c.indexOf("fog") >= 0 || c.indexOf("overcast") >= 0 || c.indexOf("mist") >= 0) {
        return LV_SYMBOL_EYE_CLOSE;
    }
    if (c.indexOf("sun") >= 0 || c.indexOf("clear") >= 0) {
        return LV_SYMBOL_EYE_OPEN;
    }

    return LV_SYMBOL_HOME;
}

static bool weatherHasWarning(const String &condition) {
    String c = condition;
    c.toLowerCase();

    return (c.indexOf("warning") >= 0 ||
            c.indexOf("alert") >= 0 ||
            c.indexOf("advisory") >= 0 ||
            c.indexOf("watch") >= 0 ||
            c.indexOf("severe") >= 0 ||
            c.indexOf("emergency") >= 0 ||
            c.indexOf("tornado") >= 0 ||
            c.indexOf("hurricane") >= 0 ||
            c.indexOf("cyclone") >= 0 ||
            c.indexOf("blizzard") >= 0 ||
            c.indexOf("flood") >= 0);
}

// ─────────────────────────────────────────────────────────────────────────────
DashboardScreen& DashboardScreen::instance() {
    static DashboardScreen s;
    return s;
}

bool DashboardScreen::isEntityHidden(const String &entityId) const {
    for (const auto &id : _hiddenEntityIds) {
        if (id == entityId) return true;
    }
    return false;
}

void DashboardScreen::loadHiddenEntities() {
    _hiddenEntityIds.clear();
    String packed = StorageManager::instance().getHiddenEntityIds();
    if (packed.isEmpty()) return;

    int start = 0;
    while (start < packed.length()) {
        int sep = packed.indexOf('\n', start);
        if (sep < 0) sep = packed.length();
        String token = packed.substring(start, sep);
        token.trim();
        if (!token.isEmpty()) _hiddenEntityIds.push_back(token);
        start = sep + 1;
    }
}

void DashboardScreen::saveHiddenEntities() const {
    String packed;
    for (size_t i = 0; i < _hiddenEntityIds.size(); i++) {
        packed += _hiddenEntityIds[i];
        if (i + 1 < _hiddenEntityIds.size()) packed += "\n";
    }
    StorageManager::instance().setHiddenEntityIds(packed);
}

void DashboardScreen::hideEntity(const String &entityId) {
    if (entityId.isEmpty() || isEntityHidden(entityId)) return;
    _hiddenEntityIds.push_back(entityId);
    saveHiddenEntities();
}

// ─────────────────────────────────────────────────────────────────────────────
void DashboardScreen::create(std::function<void()> onSettingsRequested,
                             std::function<void()> onWifiReconfigureRequested) {
    _onSettings  = onSettingsRequested;
    _onWifiReconfigure = onWifiReconfigureRequested;
    _lastPollMs  = millis();
    loadHiddenEntities();

    _screen = lv_obj_create(nullptr);
    lv_obj_add_style(_screen, &Theme::style_screen, 0);
    _tickerQueue.clear();
    _tickerIndex = 0;
    _lastTickerStepMs = millis();
    _lastTickerMessageMs = _lastTickerStepMs;
    _tickerCollapsed = false;

    // ── Status bar ───────────────────────────────────────────────────────────
    _statusBar = lv_obj_create(_screen);
    lv_obj_add_style(_statusBar, &Theme::style_status_bar, 0);
    lv_obj_set_size(_statusBar, SCREEN_W, DASH_STATUS_BAR_H);
    lv_obj_align(_statusBar, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_add_event_cb(_statusBar, onStatusBarLongPress, LV_EVENT_LONG_PRESSED, this);
    lv_obj_set_style_pad_hor(_statusBar, 6, 0);

    _lblWifi = lv_label_create(_statusBar);
    lv_obj_add_style(_lblWifi, &Theme::style_label_dim, 0);
    lv_label_set_text(_lblWifi, LV_SYMBOL_WIFI);
    lv_obj_align(_lblWifi, LV_ALIGN_LEFT_MID, 0, 0);
    lv_obj_add_flag(_lblWifi, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(_lblWifi, onWifiShortPress, LV_EVENT_SHORT_CLICKED, this);
    lv_obj_add_event_cb(_lblWifi, onWifiLongPress, LV_EVENT_LONG_PRESSED, this);

    lv_obj_t *appName = lv_label_create(_statusBar);
    lv_obj_add_style(appName, &Theme::style_label_dim, 0);
    lv_label_set_text(appName, APP_NAME);
    lv_obj_align(appName, LV_ALIGN_CENTER, 0, 0);

    _lblHA = lv_label_create(_statusBar);
    lv_obj_add_style(_lblHA, &Theme::style_label_dim, 0);
    lv_label_set_text(_lblHA, LV_SYMBOL_SETTINGS);
    lv_obj_align(_lblHA, LV_ALIGN_RIGHT_MID, 0, 0);
    lv_obj_add_flag(_lblHA, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(_lblHA, onSettingsShortPress, LV_EVENT_SHORT_CLICKED, this);

    _lblBattery = lv_label_create(_statusBar);
    lv_obj_add_style(_lblBattery, &Theme::style_label_dim, 0);
    lv_label_set_long_mode(_lblBattery, LV_LABEL_LONG_CLIP);
    lv_obj_set_width(_lblBattery, 26);
    lv_label_set_text(_lblBattery, "");
    lv_obj_align_to(_lblBattery, _lblHA, LV_ALIGN_OUT_LEFT_MID, -8, 0);

    _tickerBar = lv_obj_create(_screen);
    lv_obj_add_style(_tickerBar, &Theme::style_status_bar, 0);
    lv_obj_set_size(_tickerBar, SCREEN_W, DASH_TICKER_H);
    lv_obj_align(_tickerBar, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_style_pad_hor(_tickerBar, 6, 0);
    lv_obj_set_style_pad_ver(_tickerBar, 2, 0);
    lv_obj_add_flag(_tickerBar, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(_tickerBar, onTickerTap, LV_EVENT_SHORT_CLICKED, this);

    _lblTicker = lv_label_create(_tickerBar);
    lv_obj_add_style(_lblTicker, &Theme::style_label_dim, 0);
    lv_obj_set_width(_lblTicker, SCREEN_W - 12);
    lv_label_set_long_mode(_lblTicker, LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_label_set_text(_lblTicker, "No recent updates");
    lv_obj_align(_lblTicker, LV_ALIGN_LEFT_MID, 0, 0);

    // ── Scrollable tile container ─────────────────────────────────────────────
    _grid = lv_obj_create(_screen);
    lv_obj_set_size(_grid, SCREEN_W, SCREEN_H - DASH_STATUS_BAR_H - DASH_TICKER_H);
    lv_obj_align(_grid, LV_ALIGN_TOP_MID, 0, DASH_STATUS_BAR_H);
    lv_obj_set_style_bg_color(_grid, CLR_BG, 0);
    lv_obj_set_style_border_width(_grid, 0, 0);
    lv_obj_set_style_pad_all(_grid, 4, 0);
    lv_obj_set_style_pad_gap(_grid, 4, 0);
    lv_obj_set_scroll_dir(_grid, LV_DIR_VER);
    lv_obj_set_scrollbar_mode(_grid, LV_SCROLLBAR_MODE_ACTIVE);

    // Flex wrap — 3 tiles per row
    lv_obj_set_flex_flow(_grid, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_flex_align(_grid, LV_FLEX_ALIGN_START,
                           LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);

    lv_scr_load(_screen);
}

bool DashboardScreen::isEntityPending(const String &entityId) const {
    for (const auto &id : _pendingEntityIds) {
        if (id == entityId) return true;
    }
    return false;
}

void DashboardScreen::markEntityPending(const String &entityId) {
    if (entityId.isEmpty() || isEntityPending(entityId)) return;
    _pendingEntityIds.push_back(entityId);
}

void DashboardScreen::clearPendingEntities() {
    _pendingEntityIds.clear();
}

// ─────────────────────────────────────────────────────────────────────────────
void DashboardScreen::buildTile(lv_obj_t *parent, const HAEntity &entity, int idx) {
    const int tileW = (SCREEN_W - 8 - 8) / DASH_COLS;
    bool isOn = (entity.state == "on" || entity.state == "home"  ||
                 entity.state == "open" || entity.state == "locked" ||
                 entity.state == "playing");
    bool isUnavailable = (entity.state == "unavailable" || entity.state == "unknown");
    bool isPending = isEntityPending(entity.entityId);

    lv_obj_t *tile = lv_obj_create(parent);
    lv_obj_set_size(tile, tileW, DASH_TILE_H);
    lv_obj_add_style(tile, isOn ? &Theme::style_tile_on : &Theme::style_tile_off, 0);
    lv_obj_set_style_pad_all(tile, 6, 0);
    if (isUnavailable) {
        lv_obj_set_style_bg_opa(tile, LV_OPA_50, 0);
        lv_obj_set_style_border_color(tile, CLR_TEXT_MUTED, 0);
    }
    if (isPending) {
        lv_obj_set_style_border_color(tile, CLR_WARN, 0);
    }
    if (entity.domain == "weather") {
        bool hasWarning = weatherHasWarning(entity.weatherCondition);
        lv_obj_set_style_border_color(tile, hasWarning ? CLR_ERR : CLR_ACCENT, 0);
        lv_obj_set_style_border_width(tile, hasWarning ? 3 : 2, 0);
    }
    if (entity.domain == "climate") {
        String action = entity.climateHvacAction;
        action.toLowerCase();
        lv_color_t clColor = CLR_BORDER;
        if (action == "heating") clColor = CLR_WARN;
        else if (action == "cooling") clColor = CLR_ACCENT;
        else if (action.indexOf("fan") >= 0) clColor = CLR_OK;
        lv_obj_set_style_border_color(tile, clColor, 0);
        lv_obj_set_style_border_width(tile, 2, 0);
    }

    if (entity.isControllable || entity.domain == "weather") {
        lv_obj_add_flag(tile, LV_OBJ_FLAG_CLICKABLE);
        void *tileIndex = reinterpret_cast<void*>(static_cast<uintptr_t>(idx));
        lv_obj_add_event_cb(tile, onTileTap, LV_EVENT_SHORT_CLICKED,
                            tileIndex);
        lv_obj_add_event_cb(tile, onTileLongPress, LV_EVENT_LONG_PRESSED,
                            tileIndex);
    }

    // ── Icon ─────────────────────────────────────────────────────────────────
    lv_obj_t *icon = lv_label_create(tile);
    lv_obj_set_style_text_font(icon, FONT_LG, 0);
    lv_obj_set_style_text_color(icon, isPending ? CLR_WARN : (isUnavailable ? CLR_TEXT_MUTED : (isOn ? CLR_ON : CLR_TEXT_MUTED)), 0);
    if (entity.domain == "weather") {
        lv_label_set_text(icon, weatherIconForCondition(entity.weatherCondition));
    } else {
        lv_label_set_text(icon, Theme::entityIcon(entity.domain.c_str()));
    }
    lv_obj_align(icon, LV_ALIGN_TOP_LEFT, 0, 0);

    if (entity.domain == "light" && entity.hasColor) {
        lv_obj_t *colorDot = lv_obj_create(tile);
        lv_obj_set_size(colorDot, 12, 12);
        lv_obj_set_style_radius(colorDot, LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_bg_color(colorDot, lv_color_make(entity.colorR, entity.colorG, entity.colorB), 0);
        lv_obj_set_style_bg_opa(colorDot, LV_OPA_COVER, 0);
        lv_obj_set_style_border_color(colorDot, CLR_TEXT_DIM, 0);
        lv_obj_set_style_border_width(colorDot, 1, 0);
        lv_obj_set_style_pad_all(colorDot, 0, 0);
        lv_obj_align(colorDot, LV_ALIGN_TOP_RIGHT, -2, 2);
        lv_obj_clear_flag(colorDot, LV_OBJ_FLAG_CLICKABLE);
    }

    bool hideBinaryState =
        (entity.state == "on" || entity.state == "off") &&
        (entity.domain == "light" || entity.domain == "switch" || entity.domain == "input_boolean");
    bool hideStateText = hideBinaryState || isUnavailable;

    if (entity.domain == "weather") {
        hideStateText = false;
    }

    // ── Friendly name ─────────────────────────────────────────────────────────
    if (entity.domain != "weather") {
        lv_obj_t *name = lv_label_create(tile);
        lv_obj_set_style_text_font(name, FONT_SM, 0);
        lv_obj_set_style_text_color(name, isUnavailable ? CLR_TEXT_DIM : CLR_TEXT, 0);
        lv_label_set_long_mode(name, LV_LABEL_LONG_DOT);
        lv_obj_set_width(name, tileW - 16);
        lv_obj_set_height(name, lv_font_get_line_height(FONT_SM) + 2);
        lv_label_set_text(name, entity.friendlyName.c_str());
        if (entity.domain == "climate" && !hideStateText) {
            // Pin climate name high to keep clear separation from temp/humidity text.
            lv_obj_align(name, LV_ALIGN_TOP_LEFT, 0, 18);
        } else {
            int nameYOffset = hideStateText ? 0 : -14;
            lv_obj_align(name, LV_ALIGN_BOTTOM_LEFT, 0, nameYOffset);
        }
    }

    // ── State value ──────────────────────────────────────────────────────────
    if (!hideStateText) {
        lv_obj_t *stateLbl = lv_label_create(tile);
        lv_obj_set_style_text_font(stateLbl, FONT_SM, 0);
        lv_obj_set_style_text_color(stateLbl, CLR_TEXT_DIM, 0);

        if (entity.domain == "weather") {
            String stats;
            if (!entity.weatherTemp.isEmpty()) stats += entity.weatherTemp;
            if (!entity.weatherHumidity.isEmpty()) {
                if (!stats.isEmpty()) stats += "\n";
                stats += entity.weatherHumidity;
            }
            if (stats.isEmpty()) stats = entity.state;
            lv_label_set_text(stateLbl, stats.c_str());
        } else if (entity.domain == "climate") {
            String stats;
            if (!entity.climateCurrentTemp.isEmpty()) stats += entity.climateCurrentTemp;
            if (!entity.climateHumidity.isEmpty()) {
                if (!stats.isEmpty()) stats += "\n";
                stats += entity.climateHumidity;
            }
            if (stats.isEmpty()) stats = entity.state;
            lv_label_set_text(stateLbl, stats.c_str());
        } else if (entity.unit.isEmpty()) {
            // Capitalise first letter of state string
            String s = entity.state;
            if (s.length() > 0) s[0] = toupper(s[0]);
            lv_label_set_text(stateLbl, s.c_str());
        } else {
            lv_label_set_text_fmt(stateLbl, "%s %s",
                                  entity.state.c_str(), entity.unit.c_str());
        }
        lv_obj_align(stateLbl, LV_ALIGN_BOTTOM_LEFT, 0, 0);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
void DashboardScreen::populate(const std::vector<HAEntity> &entities, bool clearPending) {
    if (!_grid) return;
    loadHiddenEntities();
    if (clearPending) clearPendingEntities();
    _visibleEntityIndices.clear();
    lv_coord_t scrollY = lv_obj_get_scroll_y(_grid);
    lv_obj_clean(_grid);

    if (entities.empty()) {
        lv_obj_t *empty = lv_label_create(_grid);
        lv_obj_add_style(empty, &Theme::style_label_dim, 0);
        lv_label_set_text(empty, "No entities found.\nCheck HA filters.");
        lv_obj_scroll_to_y(_grid, scrollY, LV_ANIM_OFF);
        return;
    }

    int sourceIdx = 0;
    for (const auto &e : entities) {
        if (isEntityHidden(e.entityId)) {
            sourceIdx++;
            continue;
        }

        int visibleIdx = static_cast<int>(_visibleEntityIndices.size());
        _visibleEntityIndices.push_back(static_cast<size_t>(sourceIdx));
        buildTile(_grid, e, visibleIdx);
        sourceIdx++;
    }

    lv_obj_scroll_to_y(_grid, scrollY, LV_ANIM_OFF);
}

// ─────────────────────────────────────────────────────────────────────────────
void DashboardScreen::updateStatusBar(bool haConnected, int32_t rssi,
                                      bool batteryValid,
                                      uint8_t batteryPercent,
                                      bool batteryCharging) {
    if (_lblWifi) {
        int bars = 0;
        if (rssi > -55) bars = 4;
        else if (rssi > -65) bars = 3;
        else if (rssi > -75) bars = 2;
        else if (rssi > -85) bars = 1;

        const char *strength = "....";
        lv_color_t color = CLR_ERR;
        if (bars == 4) { strength = "||||"; color = CLR_OK; }
        else if (bars == 3) { strength = "|||."; color = CLR_ACCENT; }
        else if (bars == 2) { strength = "||.."; color = CLR_TEXT_DIM; }
        else if (bars == 1) { strength = "|..."; color = CLR_WARN; }

        String wifiText = String(LV_SYMBOL_WIFI) + " " + strength;
        lv_label_set_text(_lblWifi, wifiText.c_str());
        lv_obj_set_style_text_color(_lblWifi, color, 0);
    }
    if (_lblHA) {
        lv_label_set_text(_lblHA, LV_SYMBOL_SETTINGS);
        lv_obj_set_style_text_color(_lblHA, haConnected ? CLR_OK : CLR_ERR, 0);
    }
    if (_lblBattery) {
        if (!batteryValid) {
            lv_label_set_text(_lblBattery, "");
        } else {
            const char *icon = LV_SYMBOL_BATTERY_EMPTY;
            lv_color_t color = CLR_ERR;

            if (batteryPercent >= 90) {
                icon = LV_SYMBOL_BATTERY_FULL;
                color = CLR_OK;
            } else if (batteryPercent >= 65) {
                icon = LV_SYMBOL_BATTERY_3;
                color = CLR_ACCENT;
            } else if (batteryPercent >= 40) {
                icon = LV_SYMBOL_BATTERY_2;
                color = CLR_WARN;
            } else if (batteryPercent >= 15) {
                icon = LV_SYMBOL_BATTERY_1;
                color = CLR_WARN;
            }

            if (batteryCharging) {
                lv_label_set_text_fmt(_lblBattery, "%s%s", LV_SYMBOL_CHARGE, icon);
                color = CLR_OK;
            } else {
                lv_label_set_text(_lblBattery, icon);
            }

            lv_obj_set_style_text_color(_lblBattery, color, 0);
        }
    }
}

// ─────────────────────────────────────────────────────────────────────────────
void DashboardScreen::update() {
    if (!_screen) return;
    if (!_lblTicker || _tickerQueue.empty()) return;

    uint32_t now = millis();
    if (!_tickerCollapsed && (now - _lastTickerMessageMs >= DASH_TICKER_COLLAPSE_MS)) {
        _tickerCollapsed = true;
        lv_label_set_long_mode(_lblTicker, LV_LABEL_LONG_CLIP);
        lv_label_set_text_fmt(_lblTicker, "%s %u", LV_SYMBOL_BELL,
                              static_cast<unsigned>(_tickerQueue.size()));
        lv_obj_set_style_text_color(_lblTicker, CLR_ACCENT, 0);
        return;
    }

    if (_tickerCollapsed) return;
    if (now - _lastTickerStepMs < 4500) return;

    _tickerIndex = (_tickerIndex + 1) % _tickerQueue.size();
    lv_label_set_text(_lblTicker, _tickerQueue[_tickerIndex].c_str());
    _lastTickerStepMs = now;
}

void DashboardScreen::queueTickerMessage(const String &message) {
    if (!_lblTicker) return;

    String text = message;
    text.trim();
    if (text.isEmpty()) return;
    if (!_tickerQueue.empty() && _tickerQueue.back() == text) return;

    uint32_t now = millis();
    _tickerQueue.push_back(text);
    if (_tickerQueue.size() > 8) {
        _tickerQueue.erase(_tickerQueue.begin());
        if (_tickerIndex > 0) _tickerIndex--;
    }

    _tickerCollapsed = false;
    _tickerIndex = _tickerQueue.size() - 1;
    lv_label_set_long_mode(_lblTicker, LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_obj_set_style_text_color(_lblTicker, CLR_TEXT_DIM, 0);
    lv_label_set_text(_lblTicker, _tickerQueue[_tickerIndex].c_str());

    _lastTickerStepMs = now;
    _lastTickerMessageMs = now;
}

void DashboardScreen::openTickerLogPopup() {
    closeTickerLogPopup();

    _tickerLogPopup = lv_obj_create(lv_scr_act());
    lv_obj_set_size(_tickerLogPopup, SCREEN_W - 20, SCREEN_H - 24);
    lv_obj_center(_tickerLogPopup);
    lv_obj_set_style_bg_color(_tickerLogPopup, CLR_SURFACE, 0);
    lv_obj_set_style_border_color(_tickerLogPopup, CLR_ACCENT, 0);
    lv_obj_set_style_border_width(_tickerLogPopup, 1, 0);
    lv_obj_set_style_radius(_tickerLogPopup, 10, 0);
    lv_obj_set_style_pad_all(_tickerLogPopup, 8, 0);

    lv_obj_t *title = lv_label_create(_tickerLogPopup);
    lv_obj_set_style_text_font(title, FONT_MD, 0);
    lv_label_set_text(title, LV_SYMBOL_BELL "  Notifications");
    lv_obj_align(title, LV_ALIGN_TOP_LEFT, 0, 0);

    lv_obj_t *list = lv_obj_create(_tickerLogPopup);
    lv_obj_set_size(list, SCREEN_W - 40, SCREEN_H - 92);
    lv_obj_align_to(list, title, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 8);
    lv_obj_set_style_bg_color(list, CLR_SURFACE2, 0);
    lv_obj_set_style_border_color(list, CLR_TEXT_MUTED, 0);
    lv_obj_set_style_border_width(list, 1, 0);
    lv_obj_set_style_radius(list, 6, 0);
    lv_obj_set_style_pad_all(list, 6, 0);
    lv_obj_set_style_pad_gap(list, 6, 0);
    lv_obj_set_scroll_dir(list, LV_DIR_VER);
    lv_obj_set_scrollbar_mode(list, LV_SCROLLBAR_MODE_ACTIVE);
    lv_obj_set_flex_flow(list, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(list, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);

    if (_tickerQueue.empty()) {
        lv_obj_t *empty = lv_label_create(list);
        lv_obj_add_style(empty, &Theme::style_label_dim, 0);
        lv_label_set_text(empty, "No notifications yet.");
    } else {
        for (int i = static_cast<int>(_tickerQueue.size()) - 1; i >= 0; --i) {
            lv_obj_t *entry = lv_label_create(list);
            lv_obj_add_style(entry, &Theme::style_label_dim, 0);
            lv_obj_set_width(entry, SCREEN_W - 58);
            lv_label_set_long_mode(entry, LV_LABEL_LONG_WRAP);
            lv_label_set_text_fmt(entry, "%s %s", LV_SYMBOL_BELL, _tickerQueue[static_cast<size_t>(i)].c_str());
        }
    }

    lv_obj_t *closeBtn = lv_btn_create(_tickerLogPopup);
    Theme::applyBtnGhost(closeBtn);
    lv_obj_set_size(closeBtn, 88, 30);
    lv_obj_align(closeBtn, LV_ALIGN_BOTTOM_RIGHT, 0, 0);
    lv_obj_add_event_cb(closeBtn, onCloseTickerLogBtn, LV_EVENT_CLICKED, this);
    lv_obj_t *closeLbl = lv_label_create(closeBtn);
    lv_label_set_text(closeLbl, "Close");
    lv_obj_center(closeLbl);
}

void DashboardScreen::closeTickerLogPopup() {
    if (_tickerLogPopup) {
        lv_obj_del(_tickerLogPopup);
    }
    _tickerLogPopup = nullptr;
}

// ─────────────────────────────────────────────────────────────────────────────
void DashboardScreen::destroy() {
    closeTickerLogPopup();
    closeWifiInfoPopup();
    closeWeatherPopup();
    closeClimatePopup();
    closeLightPopup();
    clearPendingEntities();
    _visibleEntityIndices.clear();
    _tickerQueue.clear();
    _tickerIndex = 0;
    _tickerCollapsed = false;
    _screen = nullptr;
    _climatePopup = nullptr;
    _statusBar = _tickerBar = _grid = _lblWifi = _lblBattery = _lblHA = _lblTicker = _tickerLogPopup = nullptr;
}

// ─────────────────────────────────────────────────────────────────────────────
void DashboardScreen::onTileTap(lv_event_t *e) {
    DashboardScreen &self = DashboardScreen::instance();
    if (millis() < self._suppressTileTapUntilMs) return;

    size_t visibleIndex = static_cast<size_t>(reinterpret_cast<uintptr_t>(lv_event_get_user_data(e)));
    if (visibleIndex >= self._visibleEntityIndices.size()) return;
    size_t index = self._visibleEntityIndices[visibleIndex];
    const auto &entities = HAClient::instance().entities();
    if (index >= entities.size()) return;

    const auto &en = entities[index];

    if (en.domain == "weather") {
        DashboardScreen::instance().openWeatherPopup();
        return;
    }
    if (en.domain == "climate") {
        DashboardScreen::instance().openClimatePopup(en);
        return;
    }

    if (!en.isControllable) return;

    // Toggle: use "toggle" for switches/lights, "trigger" for scripts/scenes
    String service = "toggle";
    if (en.domain == "scene")  service = "turn_on";
    if (en.domain == "script") service = "turn_on";

    DashboardScreen::instance().markEntityPending(en.entityId);
    bool ok = HAClient::instance().callService(en.domain, service, en.entityId);
    if (!ok) {
        DashboardScreen::instance().clearPendingEntities();
    }
    DashboardScreen::instance().populate(HAClient::instance().entities(), false);
}

void DashboardScreen::onStatusBarLongPress(lv_event_t *e) {
    DashboardScreen *self = static_cast<DashboardScreen*>(lv_event_get_user_data(e));
    if (!self) return;

    lv_obj_t *target = lv_event_get_target(e);
    if (target == self->_lblWifi) return;

    self->_suppressTileTapUntilMs = millis() + 700;
    if (self->_onSettings) self->_onSettings();
}

void DashboardScreen::onSettingsShortPress(lv_event_t *e) {
    DashboardScreen *self = static_cast<DashboardScreen*>(lv_event_get_user_data(e));
    if (!self) return;
    self->_suppressTileTapUntilMs = millis() + 300;
    if (self->_onSettings) self->_onSettings();
}

void DashboardScreen::onWifiShortPress(lv_event_t *e) {
    DashboardScreen *self = static_cast<DashboardScreen*>(lv_event_get_user_data(e));
    if (!self) return;
    self->_suppressTileTapUntilMs = millis() + 300;
    self->openWifiInfoPopup();
}

void DashboardScreen::onWifiLongPress(lv_event_t *e) {
    DashboardScreen *self = static_cast<DashboardScreen*>(lv_event_get_user_data(e));
    if (!self) return;
    self->_suppressTileTapUntilMs = millis() + 700;
    self->closeWifiInfoPopup();
    if (self->_onWifiReconfigure) self->_onWifiReconfigure();
}

void DashboardScreen::onTileLongPress(lv_event_t *e) {
    DashboardScreen &self = DashboardScreen::instance();
    size_t visibleIndex = static_cast<size_t>(reinterpret_cast<uintptr_t>(lv_event_get_user_data(e)));
    if (visibleIndex >= self._visibleEntityIndices.size()) return;
    size_t index = self._visibleEntityIndices[visibleIndex];
    const auto &entities = HAClient::instance().entities();
    if (index >= entities.size()) return;

    const auto &en = entities[index];
    if (en.domain != "light") return;
    self.openLightPopup(en);
}

void DashboardScreen::openLightPopup(const HAEntity &entity) {
    closeLightPopup();

    _selectedLightId = entity.entityId;
    _selectedR = entity.hasColor ? entity.colorR : _lightPrefR;
    _selectedG = entity.hasColor ? entity.colorG : _lightPrefG;
    _selectedB = entity.hasColor ? entity.colorB : _lightPrefB;
    _useKelvin = entity.hasKelvin ? entity.colorModeIsWhite : _lightPrefUseKelvin;

    _lightPrefR = _selectedR;
    _lightPrefG = _selectedG;
    _lightPrefB = _selectedB;
    if (entity.hasBrightness) _lightPrefBrightnessPct = entity.brightnessPct;
    if (entity.hasKelvin) _lightPrefKelvin = entity.kelvin;
    _lightPrefUseKelvin = _useKelvin;

    _lightPopup = lv_obj_create(lv_scr_act());
    lv_obj_set_size(_lightPopup, SCREEN_W - 16, SCREEN_H - 16);
    lv_obj_center(_lightPopup);
    lv_obj_set_style_bg_color(_lightPopup, CLR_SURFACE, 0);
    lv_obj_set_style_border_color(_lightPopup, CLR_ACCENT, 0);
    lv_obj_set_style_border_width(_lightPopup, 1, 0);
    lv_obj_set_style_radius(_lightPopup, 10, 0);
    lv_obj_set_style_pad_all(_lightPopup, 8, 0);

    lv_obj_t *title = lv_label_create(_lightPopup);
    lv_obj_set_style_text_font(title, FONT_MD, 0);
    lv_label_set_text_fmt(title, LV_SYMBOL_TINT "  %s", entity.friendlyName.c_str());
    lv_obj_align(title, LV_ALIGN_TOP_LEFT, 0, 0);

    lv_obj_t *hideBtn = lv_btn_create(_lightPopup);
    Theme::applyBtnGhost(hideBtn);
    lv_obj_set_size(hideBtn, 60, 26);
    lv_obj_align(hideBtn, LV_ALIGN_TOP_RIGHT, 0, 0);
    lv_obj_add_event_cb(hideBtn, onHideLightBtn, LV_EVENT_CLICKED, this);
    lv_obj_t *hideLbl = lv_label_create(hideBtn);
    lv_label_set_text(hideLbl, "Hide");
    lv_obj_center(hideLbl);

    _lblMode = lv_label_create(_lightPopup);
    lv_obj_add_style(_lblMode, &Theme::style_label_dim, 0);
    lv_label_set_text(_lblMode, "Mode: Color");
    lv_obj_align_to(_lblMode, title, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 4);

    _lblBrightness = lv_label_create(_lightPopup);
    lv_obj_add_style(_lblBrightness, &Theme::style_label_dim, 0);
    lv_label_set_text_fmt(_lblBrightness, "Brightness: %u%%", static_cast<unsigned>(_lightPrefBrightnessPct));
    lv_obj_align_to(_lblBrightness, _lblMode, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 8);

    _sliderBrightness = lv_slider_create(_lightPopup);
    lv_obj_set_width(_sliderBrightness, SCREEN_W - 40);
    lv_slider_set_range(_sliderBrightness, 1, 100);
    lv_slider_set_value(_sliderBrightness, _lightPrefBrightnessPct, LV_ANIM_OFF);
    lv_obj_align_to(_sliderBrightness, _lblBrightness, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 4);
    lv_obj_add_event_cb(_sliderBrightness, onBrightnessChanged, LV_EVENT_VALUE_CHANGED, this);

    _btnModeColor = lv_btn_create(_lightPopup);
    Theme::applyBtnGhost(_btnModeColor);
    lv_obj_set_size(_btnModeColor, 82, 28);
    lv_obj_align_to(_btnModeColor, _sliderBrightness, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 8);
    lv_obj_add_event_cb(_btnModeColor, onColorModeBtn, LV_EVENT_CLICKED, this);
    lv_obj_t *modeColorLbl = lv_label_create(_btnModeColor);
    lv_label_set_text(modeColorLbl, "Color");
    lv_obj_center(modeColorLbl);

    _btnModeWhite = lv_btn_create(_lightPopup);
    Theme::applyBtnGhost(_btnModeWhite);
    lv_obj_set_size(_btnModeWhite, 82, 28);
    lv_obj_align_to(_btnModeWhite, _btnModeColor, LV_ALIGN_OUT_RIGHT_MID, 8, 0);
    lv_obj_add_event_cb(_btnModeWhite, onWhiteModeBtn, LV_EVENT_CLICKED, this);
    lv_obj_t *modeWhiteLbl = lv_label_create(_btnModeWhite);
    lv_label_set_text(modeWhiteLbl, "White");
    lv_obj_center(modeWhiteLbl);

    _colorWheel = lv_colorwheel_create(_lightPopup, true);
    lv_obj_set_size(_colorWheel, 84, 84);
    lv_obj_align_to(_colorWheel, _btnModeColor, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 8);
    lv_colorwheel_set_rgb(_colorWheel, lv_color_make(_selectedR, _selectedG, _selectedB));
    lv_obj_add_event_cb(_colorWheel, onColorWheelChanged, LV_EVENT_VALUE_CHANGED, this);

    _lblKelvin = lv_label_create(_lightPopup);
    lv_obj_add_style(_lblKelvin, &Theme::style_label_dim, 0);
    lv_label_set_text_fmt(_lblKelvin, "White Kelvin: %uK", static_cast<unsigned>(_lightPrefKelvin));
    lv_obj_align_to(_lblKelvin, _btnModeColor, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 10);

    _sliderKelvin = lv_slider_create(_lightPopup);
    lv_obj_set_width(_sliderKelvin, SCREEN_W - 40);
    lv_slider_set_range(_sliderKelvin, 2200, 6500);
    lv_slider_set_value(_sliderKelvin, _lightPrefKelvin, LV_ANIM_OFF);
    lv_obj_align_to(_sliderKelvin, _lblKelvin, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 4);
    lv_obj_add_event_cb(_sliderKelvin, onKelvinChanged, LV_EVENT_VALUE_CHANGED, this);

    lv_obj_t *applyBtn = lv_btn_create(_lightPopup);
    Theme::applyBtnPrimary(applyBtn);
    lv_obj_set_size(applyBtn, 88, 30);
    lv_obj_align(applyBtn, LV_ALIGN_BOTTOM_RIGHT, 0, 0);
    lv_obj_add_event_cb(applyBtn, onApplyBtn, LV_EVENT_CLICKED, this);
    lv_obj_t *applyLbl = lv_label_create(applyBtn);
    lv_label_set_text(applyLbl, "Apply");
    lv_obj_center(applyLbl);

    lv_obj_t *cancelBtn = lv_btn_create(_lightPopup);
    Theme::applyBtnGhost(cancelBtn);
    lv_obj_set_size(cancelBtn, 88, 30);
    lv_obj_align_to(cancelBtn, applyBtn, LV_ALIGN_OUT_LEFT_MID, -8, 0);
    lv_obj_add_event_cb(cancelBtn, onCancelBtn, LV_EVENT_CLICKED, this);
    lv_obj_t *cancelLbl = lv_label_create(cancelBtn);
    lv_label_set_text(cancelLbl, "Close");
    lv_obj_center(cancelLbl);

    updateLightPopupModeUi();
}

void DashboardScreen::closeLightPopup() {
    if (_lightPopup) {
        lv_obj_del(_lightPopup);
    }

    _lightPopup = nullptr;
    _sliderBrightness = nullptr;
    _sliderKelvin = nullptr;
    _colorWheel = nullptr;
    _lblBrightness = nullptr;
    _lblKelvin = nullptr;
    _lblMode = nullptr;
    _btnModeColor = nullptr;
    _btnModeWhite = nullptr;
    _selectedLightId = "";
}

void DashboardScreen::openWeatherPopup() {
    closeWeatherPopup();

    const HAWeatherData &w = HAClient::instance().weather();

    _weatherPopup = lv_obj_create(lv_scr_act());
    lv_obj_set_size(_weatherPopup, SCREEN_W - 20, SCREEN_H - 24);
    lv_obj_center(_weatherPopup);
    lv_obj_set_style_bg_color(_weatherPopup, CLR_SURFACE, 0);
    lv_obj_set_style_border_color(_weatherPopup, CLR_ACCENT, 0);
    lv_obj_set_style_border_width(_weatherPopup, 1, 0);
    lv_obj_set_style_radius(_weatherPopup, 10, 0);
    lv_obj_set_style_pad_all(_weatherPopup, 8, 0);

    lv_obj_t *title = lv_label_create(_weatherPopup);
    lv_obj_set_style_text_font(title, FONT_MD, 0);
    lv_label_set_text_fmt(title, "%s  Forecast", weatherIconForCondition(w.state));
    lv_obj_align(title, LV_ALIGN_TOP_LEFT, 0, 0);

    lv_obj_t *details = lv_label_create(_weatherPopup);
    lv_obj_set_style_text_font(details, FONT_SM, 0);
    lv_obj_set_style_text_color(details, CLR_TEXT, 0);
    lv_label_set_long_mode(details, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(details, SCREEN_W - 44);

    String lines;
    if (!w.state.isEmpty()) lines += "Condition: " + w.state + "\n";
    if (!w.temperature.isEmpty()) lines += "Temperature: " + w.temperature + "\n";
    if (!w.humidity.isEmpty()) lines += "Humidity: " + w.humidity + "\n";
    if (!w.apparentTemperature.isEmpty()) lines += "Feels Like: " + w.apparentTemperature + "\n";
    if (!w.dewPoint.isEmpty()) lines += "Dew Point: " + w.dewPoint + "\n";
    if (!w.pressure.isEmpty()) lines += "Pressure: " + w.pressure + "\n";
    if (!w.windSpeed.isEmpty()) lines += "Wind: " + w.windSpeed;
    if (!w.windBearing.isEmpty()) {
        if (!w.windSpeed.isEmpty()) lines += " @ ";
        else lines += "Wind Dir: ";
        lines += w.windBearing;
    }

    if (lines.isEmpty()) {
        lines = "Weather data unavailable.";
    }

    lv_label_set_text(details, lines.c_str());
    lv_obj_align_to(details, title, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 8);

    lv_obj_t *closeBtn = lv_btn_create(_weatherPopup);
    Theme::applyBtnGhost(closeBtn);
    lv_obj_set_size(closeBtn, 88, 30);
    lv_obj_align(closeBtn, LV_ALIGN_BOTTOM_RIGHT, 0, 0);
    lv_obj_add_event_cb(closeBtn, onCloseWeatherBtn, LV_EVENT_CLICKED, this);
    lv_obj_t *closeLbl = lv_label_create(closeBtn);
    lv_label_set_text(closeLbl, "Close");
    lv_obj_center(closeLbl);
}

void DashboardScreen::closeWeatherPopup() {
    if (_weatherPopup) {
        lv_obj_del(_weatherPopup);
    }
    _weatherPopup = nullptr;
}

void DashboardScreen::openClimatePopup(const HAEntity &entity) {
    closeClimatePopup();
    _selectedClimateId = entity.entityId;

    _climatePopup = lv_obj_create(lv_scr_act());
    lv_obj_set_size(_climatePopup, SCREEN_W - 20, SCREEN_H - 80);
    lv_obj_center(_climatePopup);
    lv_obj_set_style_bg_color(_climatePopup, CLR_SURFACE, 0);
    lv_obj_set_style_border_color(_climatePopup, CLR_ACCENT, 0);
    lv_obj_set_style_border_width(_climatePopup, 1, 0);
    lv_obj_set_style_radius(_climatePopup, 10, 0);
    lv_obj_set_style_pad_all(_climatePopup, 8, 0);

    lv_obj_t *title = lv_label_create(_climatePopup);
    lv_obj_set_style_text_font(title, FONT_MD, 0);
    lv_label_set_text_fmt(title, LV_SYMBOL_LOOP "  %s", entity.friendlyName.c_str());
    lv_obj_align(title, LV_ALIGN_TOP_LEFT, 0, 0);

    lv_obj_t *details = lv_label_create(_climatePopup);
    lv_obj_set_style_text_font(details, FONT_SM, 0);
    lv_obj_set_style_text_color(details, CLR_TEXT, 0);
    lv_label_set_long_mode(details, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(details, SCREEN_W - 44);

    String lines;
    if (!entity.state.isEmpty()) {
        String mode = entity.state;
        if (mode.length() > 0) mode[0] = toupper(mode[0]);
        lines += "Mode: " + mode + "\n";
    }
    if (!entity.climateHvacAction.isEmpty()) {
        String action = entity.climateHvacAction;
        if (action.length() > 0) action[0] = toupper(action[0]);
        lines += "Action: " + action + "\n";
    }
    if (!entity.climateCurrentTemp.isEmpty()) lines += "Current Temp: " + entity.climateCurrentTemp + "\n";
    if (!entity.climateHumidity.isEmpty()) lines += "Humidity: " + entity.climateHumidity + "\n";
    if (!entity.climateSetpoint.isEmpty()) lines += "Setpoint: " + entity.climateSetpoint;
    if (lines.isEmpty()) lines = "No climate data.";

    lv_label_set_text(details, lines.c_str());
    lv_obj_align_to(details, title, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 8);

    lv_obj_t *closeBtn = lv_btn_create(_climatePopup);
    Theme::applyBtnGhost(closeBtn);
    lv_obj_set_size(closeBtn, 88, 30);
    lv_obj_align(closeBtn, LV_ALIGN_BOTTOM_RIGHT, 0, 0);
    lv_obj_add_event_cb(closeBtn, onCloseClimateBtn, LV_EVENT_CLICKED, this);
    lv_obj_t *closeLbl = lv_label_create(closeBtn);
    lv_label_set_text(closeLbl, "Close");
    lv_obj_center(closeLbl);
}

void DashboardScreen::closeClimatePopup() {
    if (_climatePopup) lv_obj_del(_climatePopup);
    _climatePopup = nullptr;
    _selectedClimateId = "";
}

void DashboardScreen::openWifiInfoPopup() {
    closeWifiInfoPopup();

    _wifiPopup = lv_obj_create(lv_scr_act());
    lv_obj_set_size(_wifiPopup, SCREEN_W - 20, 156);
    lv_obj_center(_wifiPopup);
    lv_obj_set_style_bg_color(_wifiPopup, CLR_SURFACE, 0);
    lv_obj_set_style_border_color(_wifiPopup, CLR_ACCENT, 0);
    lv_obj_set_style_border_width(_wifiPopup, 1, 0);
    lv_obj_set_style_radius(_wifiPopup, 10, 0);
    lv_obj_set_style_pad_all(_wifiPopup, 8, 0);

    lv_obj_t *title = lv_label_create(_wifiPopup);
    lv_obj_set_style_text_font(title, FONT_MD, 0);
    lv_label_set_text(title, LV_SYMBOL_WIFI "  WiFi Info");
    lv_obj_align(title, LV_ALIGN_TOP_LEFT, 0, 0);

    String ssid = StorageManager::instance().getSSID();
    if (ssid.isEmpty()) ssid = "(unknown)";

    String status = WiFiMgr::instance().isConnected() ? "Connected" : "Disconnected";
    String ip = WiFiMgr::instance().isConnected() ? WiFiMgr::instance().localIP() : "-";
    String mac = WiFiMgr::instance().macAddress();
    String rssi = WiFiMgr::instance().isConnected() ? (String(WiFiMgr::instance().rssi()) + " dBm") : "-";

    lv_obj_t *details = lv_label_create(_wifiPopup);
    lv_obj_set_style_text_font(details, FONT_SM, 0);
    lv_obj_set_style_text_color(details, CLR_TEXT, 0);
    lv_label_set_long_mode(details, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(details, SCREEN_W - 44);
    lv_label_set_text_fmt(details,
                          "Status: %s\nSSID: %s\nIP: %s\nRSSI: %s\nMAC: %s",
                          status.c_str(),
                          ssid.c_str(),
                          ip.c_str(),
                          rssi.c_str(),
                          mac.c_str());
    lv_obj_align_to(details, title, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 8);

    lv_obj_t *closeBtn = lv_btn_create(_wifiPopup);
    Theme::applyBtnGhost(closeBtn);
    lv_obj_set_size(closeBtn, 88, 30);
    lv_obj_align(closeBtn, LV_ALIGN_BOTTOM_RIGHT, 0, 0);
    lv_obj_add_event_cb(closeBtn, onCloseWifiBtn, LV_EVENT_CLICKED, this);
    lv_obj_t *closeLbl = lv_label_create(closeBtn);
    lv_label_set_text(closeLbl, "Close");
    lv_obj_center(closeLbl);
}

void DashboardScreen::closeWifiInfoPopup() {
    if (_wifiPopup) {
        lv_obj_del(_wifiPopup);
    }
    _wifiPopup = nullptr;
}

void DashboardScreen::updateLightPopupModeUi() {
    if (_lblMode) {
        if (_useKelvin) {
            int kelvin = _sliderKelvin ? lv_slider_get_value(_sliderKelvin) : 3500;
            lv_label_set_text_fmt(_lblMode, "Mode: White %dK", kelvin);
        } else {
            lv_label_set_text(_lblMode, "Mode: Color");
        }
    }

    if (_colorWheel) {
        if (_useKelvin) lv_obj_add_flag(_colorWheel, LV_OBJ_FLAG_HIDDEN);
        else lv_obj_clear_flag(_colorWheel, LV_OBJ_FLAG_HIDDEN);
    }

    if (_lblKelvin) {
        if (_useKelvin) lv_obj_clear_flag(_lblKelvin, LV_OBJ_FLAG_HIDDEN);
        else lv_obj_add_flag(_lblKelvin, LV_OBJ_FLAG_HIDDEN);
    }

    if (_sliderKelvin) {
        if (_useKelvin) lv_obj_clear_flag(_sliderKelvin, LV_OBJ_FLAG_HIDDEN);
        else lv_obj_add_flag(_sliderKelvin, LV_OBJ_FLAG_HIDDEN);
    }

    if (_btnModeColor) {
        lv_obj_set_style_bg_opa(_btnModeColor, LV_OPA_COVER, 0);
        lv_obj_set_style_bg_color(_btnModeColor, _useKelvin ? CLR_SURFACE2 : CLR_ACCENT, 0);
        lv_obj_set_style_text_color(_btnModeColor, _useKelvin ? CLR_TEXT : CLR_BG, 0);
    }

    if (_btnModeWhite) {
        lv_obj_set_style_bg_opa(_btnModeWhite, LV_OPA_COVER, 0);
        lv_obj_set_style_bg_color(_btnModeWhite, _useKelvin ? CLR_ACCENT : CLR_SURFACE2, 0);
        lv_obj_set_style_text_color(_btnModeWhite, _useKelvin ? CLR_BG : CLR_TEXT, 0);
    }
}

void DashboardScreen::applyLightPopup() {
    if (_selectedLightId.isEmpty() || !_sliderBrightness) {
        closeLightPopup();
        return;
    }

    int brightnessPct = lv_slider_get_value(_sliderBrightness);
    int brightness = (brightnessPct * 255) / 100;
    _lightPrefBrightnessPct = static_cast<uint8_t>(brightnessPct);

    bool ok = false;
    if (_useKelvin) {
        int kelvin = lv_slider_get_value(_sliderKelvin);
        _lightPrefUseKelvin = true;
        _lightPrefKelvin = static_cast<uint16_t>(kelvin);
        uint32_t mired = 1000000UL / static_cast<uint32_t>(kelvin);

        String extraKelvin = "\"brightness\":" + String(brightness) +
                             ",\"color_temp_kelvin\":" + String(kelvin);
        ok = HAClient::instance().callService("light", "turn_on", _selectedLightId, extraKelvin);

        if (!ok) {
            String extraMired = "\"brightness\":" + String(brightness) +
                                ",\"color_temp\":" + String(mired);
            ok = HAClient::instance().callService("light", "turn_on", _selectedLightId, extraMired);
        }
    } else {
        _lightPrefUseKelvin = false;
        if (_colorWheel) {
            lv_color_t color = lv_colorwheel_get_rgb(_colorWheel);
            lv_color32_t color32;
            color32.full = lv_color_to32(color);
            _selectedR = color32.ch.red;
            _selectedG = color32.ch.green;
            _selectedB = color32.ch.blue;
        }
        _lightPrefR = _selectedR;
        _lightPrefG = _selectedG;
        _lightPrefB = _selectedB;

        String extraRgb = "\"brightness\":" + String(brightness) +
                          ",\"rgb_color\":[" + String(_selectedR) + "," + String(_selectedG) + "," + String(_selectedB) + "]";
        ok = HAClient::instance().callService("light", "turn_on", _selectedLightId, extraRgb);
    }

    if (!ok) {
        if (_lblMode) lv_label_set_text(_lblMode, "Apply failed (unsupported)");
        return;
    }

    delay(250);
    HAClient::instance().fetchEntities();
    DashboardScreen::instance().populate(HAClient::instance().entities());

    if (_lblMode) {
        lv_label_set_text(_lblMode, _useKelvin ? "Applied: White" : "Applied: Color");
    }
}

void DashboardScreen::onColorModeBtn(lv_event_t *e) {
    DashboardScreen *self = static_cast<DashboardScreen*>(lv_event_get_user_data(e));
    if (!self) return;
    self->_useKelvin = false;
    self->_lightPrefUseKelvin = false;
    self->updateLightPopupModeUi();
}

void DashboardScreen::onWhiteModeBtn(lv_event_t *e) {
    DashboardScreen *self = static_cast<DashboardScreen*>(lv_event_get_user_data(e));
    if (!self) return;
    self->_useKelvin = true;
    self->_lightPrefUseKelvin = true;
    self->updateLightPopupModeUi();
}

void DashboardScreen::onApplyBtn(lv_event_t *e) {
    DashboardScreen *self = static_cast<DashboardScreen*>(lv_event_get_user_data(e));
    if (!self) return;
    self->applyLightPopup();
}

void DashboardScreen::onCancelBtn(lv_event_t *e) {
    DashboardScreen *self = static_cast<DashboardScreen*>(lv_event_get_user_data(e));
    if (!self) return;
    self->closeLightPopup();
}

void DashboardScreen::onHideLightBtn(lv_event_t *e) {
    DashboardScreen *self = static_cast<DashboardScreen*>(lv_event_get_user_data(e));
    if (!self || self->_selectedLightId.isEmpty()) return;

    self->hideEntity(self->_selectedLightId);
    self->closeLightPopup();
    self->populate(HAClient::instance().entities(), false);
}

void DashboardScreen::onCloseWeatherBtn(lv_event_t *e) {
    DashboardScreen *self = static_cast<DashboardScreen*>(lv_event_get_user_data(e));
    if (!self) return;
    self->closeWeatherPopup();
}

void DashboardScreen::onCloseClimateBtn(lv_event_t *e) {
    DashboardScreen *self = static_cast<DashboardScreen*>(lv_event_get_user_data(e));
    if (!self) return;
    self->closeClimatePopup();
}

void DashboardScreen::onCloseWifiBtn(lv_event_t *e) {
    DashboardScreen *self = static_cast<DashboardScreen*>(lv_event_get_user_data(e));
    if (!self) return;
    self->closeWifiInfoPopup();
}

void DashboardScreen::onTickerTap(lv_event_t *e) {
    DashboardScreen *self = static_cast<DashboardScreen*>(lv_event_get_user_data(e));
    if (!self) return;
    self->_suppressTileTapUntilMs = millis() + 250;
    self->openTickerLogPopup();
}

void DashboardScreen::onCloseTickerLogBtn(lv_event_t *e) {
    DashboardScreen *self = static_cast<DashboardScreen*>(lv_event_get_user_data(e));
    if (!self) return;
    self->closeTickerLogPopup();
}

void DashboardScreen::onBrightnessChanged(lv_event_t *e) {
    DashboardScreen *self = static_cast<DashboardScreen*>(lv_event_get_user_data(e));
    if (!self || !self->_sliderBrightness || !self->_lblBrightness) return;
    int value = lv_slider_get_value(self->_sliderBrightness);
    self->_lightPrefBrightnessPct = static_cast<uint8_t>(value);
    lv_label_set_text_fmt(self->_lblBrightness, "Brightness: %d%%", value);
}

void DashboardScreen::onKelvinChanged(lv_event_t *e) {
    DashboardScreen *self = static_cast<DashboardScreen*>(lv_event_get_user_data(e));
    if (!self || !self->_sliderKelvin || !self->_lblKelvin) return;
    int kelvin = lv_slider_get_value(self->_sliderKelvin);
    self->_lightPrefKelvin = static_cast<uint16_t>(kelvin);
    lv_label_set_text_fmt(self->_lblKelvin, "White Kelvin: %dK", kelvin);
    if (self->_useKelvin) {
        self->updateLightPopupModeUi();
    }
}

void DashboardScreen::onColorWheelChanged(lv_event_t *e) {
    DashboardScreen *self = static_cast<DashboardScreen*>(lv_event_get_user_data(e));
    if (!self || !self->_colorWheel || self->_useKelvin) return;

    lv_color_t color = lv_colorwheel_get_rgb(self->_colorWheel);
    lv_color32_t color32;
    color32.full = lv_color_to32(color);
    self->_selectedR = color32.ch.red;
    self->_selectedG = color32.ch.green;
    self->_selectedB = color32.ch.blue;
    self->_lightPrefR = self->_selectedR;
    self->_lightPrefG = self->_selectedG;
    self->_lightPrefB = self->_selectedB;

    if (self->_lblMode) {
        lv_label_set_text_fmt(self->_lblMode, "Mode: Color #%02X%02X%02X", self->_selectedR, self->_selectedG, self->_selectedB);
    }
}
