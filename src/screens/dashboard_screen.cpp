#include "dashboard_screen.h"
#include "../ui/theme.h"
#include "../managers/ha_client.h"
#include "../managers/wifi_manager.h"
#include "config.h"
#include <cstdint>

// ─────────────────────────────────────────────────────────────────────────────
DashboardScreen& DashboardScreen::instance() {
    static DashboardScreen s;
    return s;
}

// ─────────────────────────────────────────────────────────────────────────────
void DashboardScreen::create(std::function<void()> onSettingsRequested) {
    _onSettings  = onSettingsRequested;
    _lastPollMs  = millis();

    _screen = lv_obj_create(nullptr);
    lv_obj_add_style(_screen, &Theme::style_screen, 0);

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

    lv_obj_t *appName = lv_label_create(_statusBar);
    lv_obj_add_style(appName, &Theme::style_label_dim, 0);
    lv_label_set_text(appName, APP_NAME);
    lv_obj_align(appName, LV_ALIGN_CENTER, 0, 0);

    _lblHA = lv_label_create(_statusBar);
    lv_obj_add_style(_lblHA, &Theme::style_label_dim, 0);
    lv_label_set_text(_lblHA, LV_SYMBOL_HOME);
    lv_obj_align(_lblHA, LV_ALIGN_RIGHT_MID, 0, 0);

    // ── Scrollable tile container ─────────────────────────────────────────────
    _grid = lv_obj_create(_screen);
    lv_obj_set_size(_grid, SCREEN_W, SCREEN_H - DASH_STATUS_BAR_H);
    lv_obj_align(_grid, LV_ALIGN_BOTTOM_MID, 0, 0);
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

    if (entity.isControllable) {
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
    lv_label_set_text(icon, Theme::entityIcon(entity.domain.c_str()));
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

    // ── Friendly name ─────────────────────────────────────────────────────────
    lv_obj_t *name = lv_label_create(tile);
    lv_obj_set_style_text_font(name, FONT_SM, 0);
    lv_obj_set_style_text_color(name, isUnavailable ? CLR_TEXT_DIM : CLR_TEXT, 0);
    lv_label_set_long_mode(name, LV_LABEL_LONG_DOT);
    lv_obj_set_width(name, tileW - 16);
    lv_label_set_text(name, entity.friendlyName.c_str());
    lv_obj_align(name, LV_ALIGN_BOTTOM_LEFT, 0, hideStateText ? 0 : -14);

    // ── State value ──────────────────────────────────────────────────────────
    if (!hideStateText) {
        lv_obj_t *stateLbl = lv_label_create(tile);
        lv_obj_set_style_text_font(stateLbl, FONT_SM, 0);
        lv_obj_set_style_text_color(stateLbl, CLR_TEXT_DIM, 0);

        if (entity.unit.isEmpty()) {
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
    if (clearPending) clearPendingEntities();
    lv_coord_t scrollY = lv_obj_get_scroll_y(_grid);
    lv_obj_clean(_grid);

    if (entities.empty()) {
        lv_obj_t *empty = lv_label_create(_grid);
        lv_obj_add_style(empty, &Theme::style_label_dim, 0);
        lv_label_set_text(empty, "No entities found.\nCheck HA filters.");
        lv_obj_scroll_to_y(_grid, scrollY, LV_ANIM_OFF);
        return;
    }

    int idx = 0;
    for (const auto &e : entities) {
        buildTile(_grid, e, idx++);
    }

    lv_obj_scroll_to_y(_grid, scrollY, LV_ANIM_OFF);
}

// ─────────────────────────────────────────────────────────────────────────────
void DashboardScreen::updateStatusBar(bool haConnected, int32_t rssi) {
    if (_lblWifi) {
        const char *wifiSym = (rssi > -55) ? LV_SYMBOL_WIFI :
                              (rssi > -70) ? LV_SYMBOL_WIFI : LV_SYMBOL_WARNING;
        lv_label_set_text(_lblWifi, wifiSym);
        lv_obj_set_style_text_color(_lblWifi, (rssi > -70) ? CLR_TEXT_DIM : CLR_WARN, 0);
    }
    if (_lblHA) {
        lv_label_set_text(_lblHA, LV_SYMBOL_HOME);
        lv_obj_set_style_text_color(_lblHA, haConnected ? CLR_OK : CLR_ERR, 0);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
void DashboardScreen::update() {
    if (!_screen) return;
}

// ─────────────────────────────────────────────────────────────────────────────
void DashboardScreen::destroy() {
    closeLightPopup();
    clearPendingEntities();
    _screen = nullptr;
    _statusBar = _grid = _lblWifi = _lblHA = nullptr;
}

// ─────────────────────────────────────────────────────────────────────────────
void DashboardScreen::onTileTap(lv_event_t *e) {
    DashboardScreen &self = DashboardScreen::instance();
    if (millis() < self._suppressTileTapUntilMs) return;

    size_t index = static_cast<size_t>(reinterpret_cast<uintptr_t>(lv_event_get_user_data(e)));
    const auto &entities = HAClient::instance().entities();
    if (index >= entities.size()) return;

    const auto &en = entities[index];

    // Toggle: use "toggle" for switches/lights, "trigger" for scripts/scenes
    String service = "toggle";
    if (en.domain == "scene")  service = "turn_on";
    if (en.domain == "script") service = "turn_on";

    DashboardScreen::instance().markEntityPending(en.entityId);
    HAClient::instance().callService(en.domain, service, en.entityId);
    DashboardScreen::instance().populate(HAClient::instance().entities(), false);
}

void DashboardScreen::onStatusBarLongPress(lv_event_t *e) {
    DashboardScreen *self = static_cast<DashboardScreen*>(lv_event_get_user_data(e));
    self->_suppressTileTapUntilMs = millis() + 700;
    if (self->_onSettings) self->_onSettings();
}

void DashboardScreen::onTileLongPress(lv_event_t *e) {
    size_t index = static_cast<size_t>(reinterpret_cast<uintptr_t>(lv_event_get_user_data(e)));
    const auto &entities = HAClient::instance().entities();
    if (index >= entities.size()) return;

    const auto &en = entities[index];
    if (en.domain != "light") return;
    DashboardScreen::instance().openLightPopup(en);
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
