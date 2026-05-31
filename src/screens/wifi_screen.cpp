#include "wifi_screen.h"
#include "../ui/theme.h"
#include "config.h"
#include <Arduino.h>
#include <WiFi.h>

// ─────────────────────────────────────────────────────────────────────────────
WiFiScreen& WiFiScreen::instance() {
    static WiFiScreen s;
    return s;
}

// ─────────────────────────────────────────────────────────────────────────────
void WiFiScreen::create(std::function<void(String, String)> onConnected,
                        std::function<void()> onCancel) {
    _onConnected = onConnected;
    _onCancel = onCancel;

    _screen = lv_obj_create(nullptr);
    lv_obj_add_style(_screen, &Theme::style_screen, 0);
    lv_scr_load(_screen);

    showNetworkList();
    startScan();
}

// ─────────────────────────────────────────────────────────────────────────────
void WiFiScreen::showNetworkList() {
    // Clear any existing password panel
    if (_passPanel) { lv_obj_add_flag(_passPanel, LV_OBJ_FLAG_HIDDEN); _passPanel = nullptr; }

    // ── Header ───────────────────────────────────────────────────────────────
    lv_obj_t *hdr = lv_obj_create(_screen);
    lv_obj_set_size(hdr, SCREEN_W, 36);
    lv_obj_align(hdr, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_add_style(hdr, &Theme::style_status_bar, 0);

    lv_obj_t *hdrLbl = lv_label_create(hdr);
    lv_obj_add_style(hdrLbl, &Theme::style_label_body, 0);
    lv_label_set_text(hdrLbl, "Networks");
    lv_obj_align(hdrLbl, LV_ALIGN_CENTER, 0, 0);

    if (_onCancel) {
        lv_obj_t *backBtn = lv_btn_create(hdr);
        Theme::applyBtnGhost(backBtn);
        lv_obj_set_size(backBtn, 70, 26);
        lv_obj_align(backBtn, LV_ALIGN_LEFT_MID, 0, 0);
        lv_obj_add_event_cb(backBtn, onCancelBtn, LV_EVENT_CLICKED, this);
        lv_obj_t *backLbl = lv_label_create(backBtn);
        lv_label_set_text(backLbl, LV_SYMBOL_LEFT " Back");
        lv_obj_center(backLbl);
    }

    // Rescan button
    lv_obj_t *rescanBtn = lv_btn_create(hdr);
    Theme::applyBtnGhost(rescanBtn);
    lv_obj_set_size(rescanBtn, 70, 26);
    lv_obj_align(rescanBtn, LV_ALIGN_RIGHT_MID, 0, 0);
    lv_obj_add_event_cb(rescanBtn, onRescanBtn, LV_EVENT_CLICKED, this);
    lv_obj_t *rescanLbl = lv_label_create(rescanBtn);
    lv_label_set_text(rescanLbl, LV_SYMBOL_LOOP " Scan");
    lv_obj_center(rescanLbl);

    // ── Spinner (shown during scan) ───────────────────────────────────────────
    _spinner = lv_spinner_create(_screen, 1000, 60);
    lv_obj_set_size(_spinner, 40, 40);
    lv_obj_align(_spinner, LV_ALIGN_CENTER, 0, 20);
    lv_obj_set_style_arc_color(_spinner, CLR_ACCENT, LV_PART_INDICATOR);

    // ── Scrollable list ───────────────────────────────────────────────────────
    _list = lv_list_create(_screen);
    lv_obj_set_size(_list, SCREEN_W - 8, SCREEN_H - 44);
    lv_obj_align(_list, LV_ALIGN_BOTTOM_MID, 0, -2);
    lv_obj_set_style_bg_color(_list, CLR_BG, 0);
    lv_obj_set_style_border_width(_list, 0, 0);
    lv_obj_add_flag(_list, LV_OBJ_FLAG_HIDDEN);   // hidden until scan done

    // ── Status label ─────────────────────────────────────────────────────────
    _lblStatus = lv_label_create(_screen);
    lv_obj_add_style(_lblStatus, &Theme::style_label_dim, 0);
    lv_label_set_text(_lblStatus, "Scanning...");
    lv_obj_align(_lblStatus, LV_ALIGN_CENTER, 0, 50);
}

// ─────────────────────────────────────────────────────────────────────────────
void WiFiScreen::showPasswordPanel(const String &ssid) {
    _selectedSsid = ssid;

    // Hide list
    if (_list)    lv_obj_add_flag(_list,    LV_OBJ_FLAG_HIDDEN);
    if (_spinner) lv_obj_add_flag(_spinner, LV_OBJ_FLAG_HIDDEN);

    // ── Password panel ────────────────────────────────────────────────────────
    _passPanel = lv_obj_create(_screen);
    lv_obj_set_size(_passPanel, SCREEN_W, SCREEN_H);
    lv_obj_align(_passPanel, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_add_style(_passPanel, &Theme::style_screen, 0);

    // Header
    lv_obj_t *hdrLbl = lv_label_create(_passPanel);
    lv_obj_add_style(hdrLbl, &Theme::style_label_body, 0);
    lv_label_set_text_fmt(hdrLbl, LV_SYMBOL_WIFI "  %s", ssid.c_str());
    lv_obj_align(hdrLbl, LV_ALIGN_TOP_LEFT, 8, 8);

    // Password textarea
    _taPass = lv_textarea_create(_passPanel);
    lv_obj_set_size(_taPass, SCREEN_W - 16, 36);
    lv_obj_align(_taPass, LV_ALIGN_TOP_LEFT, 8, 36);
    lv_textarea_set_placeholder_text(_taPass, "Password");
    lv_textarea_set_password_mode(_taPass, true);
    lv_textarea_set_one_line(_taPass, true);

    // On-screen keyboard
    _keyboard = lv_keyboard_create(_passPanel);
    lv_obj_set_size(_keyboard, SCREEN_W, 130);
    lv_obj_align(_keyboard, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_keyboard_set_textarea(_keyboard, _taPass);

    // Action buttons row
    lv_obj_t *row = lv_obj_create(_passPanel);
    lv_obj_set_size(row, SCREEN_W - 16, 36);
    lv_obj_align(row, LV_ALIGN_TOP_LEFT, 8, 80);
    lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(row, 0, 0);
    lv_obj_set_style_pad_all(row, 0, 0);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t *backBtn = lv_btn_create(row);
    Theme::applyBtnGhost(backBtn);
    lv_obj_set_width(backBtn, 90);
    lv_obj_add_event_cb(backBtn, onBackBtn, LV_EVENT_CLICKED, this);
    lv_obj_t *backLbl = lv_label_create(backBtn);
    lv_label_set_text(backLbl, LV_SYMBOL_LEFT " Back");
    lv_obj_center(backLbl);

    lv_obj_t *connectBtn = lv_btn_create(row);
    Theme::applyBtnPrimary(connectBtn);
    lv_obj_set_width(connectBtn, 110);
    lv_obj_add_event_cb(connectBtn, onConnectBtn, LV_EVENT_CLICKED, this);
    lv_obj_t *connectLbl = lv_label_create(connectBtn);
    lv_label_set_text(connectLbl, "Connect " LV_SYMBOL_RIGHT);
    lv_obj_center(connectLbl);
}

// ─────────────────────────────────────────────────────────────────────────────
void WiFiScreen::startScan() {
    _scanning    = true;
    _scanStartMs = millis();

    if (_lblStatus) lv_label_set_text(_lblStatus, "Scanning...");
    if (_spinner)   lv_obj_clear_flag(_spinner, LV_OBJ_FLAG_HIDDEN);
    if (_list)      lv_obj_add_flag(_list, LV_OBJ_FLAG_HIDDEN);

    // Non-blocking async scan
    WiFi.mode(WIFI_STA);
    WiFi.scanNetworks(true /*async*/, false /*show hidden*/);
}

// ─────────────────────────────────────────────────────────────────────────────
void WiFiScreen::populateList() {
    lv_obj_clean(_list);

    if (_networks.empty()) {
        lv_list_add_text(_list, "No networks found");
        return;
    }

    for (auto &net : _networks) {
        char label[64];
        snprintf(label, sizeof(label), "%s%s",
                 net.ssid.c_str(), net.isOpen ? "" : " " LV_SYMBOL_SETTINGS);

        lv_obj_t *btn = lv_list_add_btn(_list, nullptr, label);
        lv_obj_set_style_bg_color(btn, CLR_SURFACE, 0);
        lv_obj_set_style_bg_color(btn, CLR_SURFACE2, LV_STATE_PRESSED);
        lv_obj_set_style_text_color(btn, CLR_TEXT, 0);
        lv_obj_add_event_cb(btn, onNetworkTap, LV_EVENT_CLICKED, this);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
void WiFiScreen::update() {
    if (!_scanning) return;

    int n = WiFi.scanComplete();
    if (n == WIFI_SCAN_RUNNING) return;   // still scanning

    _scanning = false;

    if (_spinner) lv_obj_add_flag(_spinner, LV_OBJ_FLAG_HIDDEN);
    if (_list)    lv_obj_clear_flag(_list, LV_OBJ_FLAG_HIDDEN);

    _networks.clear();
    if (n > 0) {
        for (int i = 0; i < n; i++) {
            WiFiNetwork net;
            net.ssid   = WiFi.SSID(i);
            net.rssi   = WiFi.RSSI(i);
            net.isOpen = (WiFi.encryptionType(i) == WIFI_AUTH_OPEN);
            _networks.push_back(net);
        }
        WiFi.scanDelete();
    }

    populateList();
    if (_lblStatus) lv_obj_add_flag(_lblStatus, LV_OBJ_FLAG_HIDDEN);
}

// ─────────────────────────────────────────────────────────────────────────────
void WiFiScreen::destroy() {
    _screen = nullptr;
    _list = _passPanel = _keyboard = _taPass = _spinner = nullptr;
    _lblSsid = _lblStatus = nullptr;
}

// ─────────────────────────────────────────────────────────────────────────────
//  Static LVGL callbacks
// ─────────────────────────────────────────────────────────────────────────────
void WiFiScreen::onNetworkTap(lv_event_t *e) {
    WiFiScreen *self = static_cast<WiFiScreen*>(lv_event_get_user_data(e));
    lv_obj_t *btn   = lv_event_get_target(e);
    lv_obj_t *lbl   = lv_obj_get_child(btn, 0);
    if (!lbl) return;

    // Extract SSID from the label text (skip leading icon + 2 spaces)
    String txt = lv_label_get_text(lbl);
    int start = txt.indexOf("  ");
    if (start < 0) return;
    String ssid = txt.substring(start + 2);
    ssid.trim();
    // Remove lock icon suffix if present
    int lock = ssid.indexOf(" " LV_SYMBOL_SETTINGS);
    if (lock >= 0) ssid = ssid.substring(0, lock);

    self->showPasswordPanel(ssid);
}

void WiFiScreen::onConnectBtn(lv_event_t *e) {
    WiFiScreen *self = static_cast<WiFiScreen*>(lv_event_get_user_data(e));
    if (!self->_taPass) return;
    String pass = lv_textarea_get_text(self->_taPass);
    if (self->_onConnected) {
        self->_onConnected(self->_selectedSsid, pass);
    }
}

void WiFiScreen::onBackBtn(lv_event_t *e) {
    WiFiScreen *self = static_cast<WiFiScreen*>(lv_event_get_user_data(e));
    if (self->_passPanel) {
        lv_obj_add_flag(self->_passPanel, LV_OBJ_FLAG_HIDDEN);
        self->_passPanel = self->_keyboard = self->_taPass = nullptr;
    }
    if (self->_list)    lv_obj_clear_flag(self->_list,    LV_OBJ_FLAG_HIDDEN);
    if (self->_spinner) lv_obj_clear_flag(self->_spinner, LV_OBJ_FLAG_HIDDEN);
    self->startScan();
}

void WiFiScreen::onRescanBtn(lv_event_t *e) {
    WiFiScreen *self = static_cast<WiFiScreen*>(lv_event_get_user_data(e));
    self->startScan();
}

void WiFiScreen::onCancelBtn(lv_event_t *e) {
    WiFiScreen *self = static_cast<WiFiScreen*>(lv_event_get_user_data(e));
    if (!self) return;
    if (self->_onCancel) self->_onCancel();
}
