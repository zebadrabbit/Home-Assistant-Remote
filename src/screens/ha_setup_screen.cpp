#include "ha_setup_screen.h"
#include "../ui/theme.h"
#include "config.h"

// ─── Minimal HTML form served to the user's phone ────────────────────────────
static const char HA_SETUP_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width,initial-scale=1">
  <title>HA Remote — Setup</title>
  <style>
    *{box-sizing:border-box;margin:0;padding:0}
    body{font-family:system-ui,sans-serif;background:#0d1117;color:#e6edf3;
         display:flex;flex-direction:column;align-items:center;padding:24px 16px}
    h1{color:#41bdf5;font-size:1.4rem;margin-bottom:4px}
    p{color:#8b949e;font-size:.85rem;margin-bottom:20px}
    label{display:block;font-size:.85rem;color:#8b949e;margin-bottom:4px}
    input{width:100%;padding:10px 12px;background:#161b22;border:1px solid #30363d;
          color:#e6edf3;border-radius:6px;font-size:.95rem;margin-bottom:16px}
    input:focus{outline:none;border-color:#41bdf5}
    button{width:100%;padding:12px;background:#41bdf5;color:#0d1117;border:none;
           border-radius:6px;font-size:1rem;font-weight:600;cursor:pointer}
    button:active{opacity:.85}
    .note{font-size:.75rem;color:#484f58;margin-top:12px;text-align:center}
  </style>
</head>
<body>
  <h1>HA Remote Setup</h1>
  <p>Connect to your Home Assistant server.</p>
  <form action="/save" method="POST">
    <label for="url">Server URL</label>
    <input type="text" id="url" name="url"
           placeholder="http://homeassistant.local:8123" required>
    <label for="token">Long-Lived Access Token</label>
    <input type="password" id="token" name="token"
           placeholder="Paste from Profile → Long-Lived Access Tokens" required>
    <button type="submit">Connect to Home Assistant</button>
  </form>
  <p class="note">
    Token: Profile → Security → Long-Lived Access Tokens → Create Token
  </p>
</body>
</html>
)rawliteral";

static const char HA_SAVED_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html><html><head>
<meta charset="UTF-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>HA Remote</title>
<style>body{font-family:system-ui,sans-serif;background:#0d1117;color:#e6edf3;
display:flex;flex-direction:column;align-items:center;justify-content:center;
height:100vh;gap:12px;}h1{color:#3fb950}p{color:#8b949e;font-size:.9rem}</style>
</head><body>
<h1>&#10003; Saved!</h1>
<p>Your device is connecting to Home Assistant...</p>
</body></html>
)rawliteral";

static const char CONFIG_SAVED_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html><html><head>
<meta charset="UTF-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>HA Remote</title>
<style>body{font-family:system-ui,sans-serif;background:#0d1117;color:#e6edf3;
display:flex;flex-direction:column;align-items:center;justify-content:center;
height:100vh;gap:12px;}h1{color:#3fb950}p{color:#8b949e;font-size:.9rem}</style>
</head><body>
<h1>&#10003; Settings Saved!</h1>
<p>Reboot the remote to apply orientation/theme updates.</p>
<p><a href="/config" style="color:#41bdf5">Back to settings</a></p>
</body></html>
)rawliteral";

// ─────────────────────────────────────────────────────────────────────────────
HASetupScreen& HASetupScreen::instance() {
    static HASetupScreen s;
    return s;
}

// ─────────────────────────────────────────────────────────────────────────────
void HASetupScreen::create(const String &deviceIP,
                           std::function<void(String, String)> onSaved) {
    _deviceIP  = deviceIP;
    _onSaved   = onSaved;

    _screen = lv_obj_create(nullptr);
    lv_obj_add_style(_screen, &Theme::style_screen, 0);

    // ── Header ───────────────────────────────────────────────────────────────
    lv_obj_t *hdr = lv_obj_create(_screen);
    lv_obj_set_size(hdr, SCREEN_W, 36);
    lv_obj_align(hdr, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_add_style(hdr, &Theme::style_status_bar, 0);

    lv_obj_t *hdrLbl = lv_label_create(hdr);
    lv_obj_add_style(hdrLbl, &Theme::style_label_body, 0);
    lv_label_set_text(hdrLbl, LV_SYMBOL_HOME "  Home Assistant");
    lv_obj_align(hdrLbl, LV_ALIGN_LEFT_MID, 0, 0);

    // ── Instructions card ────────────────────────────────────────────────────
    lv_obj_t *card = lv_obj_create(_screen);
    lv_obj_add_style(card, &Theme::style_card, 0);
    lv_obj_set_size(card, SCREEN_W - 16, 110);
    lv_obj_align(card, LV_ALIGN_TOP_MID, 0, 44);

    lv_obj_t *step1 = lv_label_create(card);
    lv_obj_add_style(step1, &Theme::style_label_body, 0);
    lv_label_set_text(step1, "Open on your phone:");
    lv_obj_align(step1, LV_ALIGN_TOP_LEFT, 0, 0);

    _lblIP = lv_label_create(card);
    lv_obj_set_style_text_font(_lblIP, FONT_XL, 0);
    lv_obj_set_style_text_color(_lblIP, CLR_ACCENT, 0);
    lv_label_set_text_fmt(_lblIP, "http://%s", deviceIP.c_str());
    lv_obj_align_to(_lblIP, step1, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 6);

    lv_obj_t *step2 = lv_label_create(card);
    lv_obj_add_style(step2, &Theme::style_label_dim, 0);
    lv_label_set_text(step2, "Enter server URL and access token.");
    lv_obj_align_to(step2, _lblIP, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 6);

    // ── Status ───────────────────────────────────────────────────────────────
    _lblStatus = lv_label_create(_screen);
    lv_obj_add_style(_lblStatus, &Theme::style_label_dim, 0);
    lv_label_set_text(_lblStatus, "Waiting for browser...");
    lv_obj_align(_lblStatus, LV_ALIGN_CENTER, 0, 80);

    // ── Manual entry button ───────────────────────────────────────────────────
    lv_obj_t *manBtn = lv_btn_create(_screen);
    Theme::applyBtnGhost(manBtn);
    lv_obj_set_size(manBtn, 160, 32);
    lv_obj_align(manBtn, LV_ALIGN_BOTTOM_MID, 0, -10);
    lv_obj_add_event_cb(manBtn, onManualBtn, LV_EVENT_CLICKED, this);
    lv_obj_t *manLbl = lv_label_create(manBtn);
    lv_label_set_text(manLbl, "Enter manually");
    lv_obj_center(manLbl);

    lv_scr_load(_screen);
    startWebServer();
}

// ─────────────────────────────────────────────────────────────────────────────
void HASetupScreen::startWebServer() {
    _server = new WebServer(80);

    _server->on("/", HTTP_GET, [this]() { serveHomePage(); });
    _server->on("/quick", HTTP_GET, [this]() { serveSetupPage(); });
    _server->on("/save", HTTP_POST, [this]() { handleSave(); });
    _server->on("/config", HTTP_GET, [this]() { serveConfigPage(); });
    _server->on("/save-config", HTTP_POST, [this]() { handleSaveConfig(); });
    _server->onNotFound([this]() { _server->send(404, "text/plain", "Not Found"); });

    _server->begin();
    _serverReady = true;
}

void HASetupScreen::stopWebServer() {
    if (_server) {
        _server->stop();
        delete _server;
        _server = nullptr;
    }
    _serverReady = false;
}

void HASetupScreen::serveHomePage() {
    String html;
    html.reserve(2500);
    html += "<!DOCTYPE html><html><head><meta charset='UTF-8'>";
    html += "<meta name='viewport' content='width=device-width,initial-scale=1'>";
    html += "<title>HA Remote</title>";
    html += "<style>*{box-sizing:border-box}body{margin:0;font-family:system-ui,sans-serif;background:#0b1220;color:#e6edf3}";
    html += ".wrap{max-width:720px;margin:0 auto;padding:18px}.card{background:#121a2b;border:1px solid #253147;border-radius:12px;padding:14px;margin-top:12px}";
    html += "h1{margin:0;color:#5cd4ff}p{color:#9fb1c7}.btn{display:inline-block;padding:10px 12px;border-radius:8px;text-decoration:none;font-weight:700;margin-top:8px}";
    html += ".pri{background:#5cd4ff;color:#07111f}.sec{background:#24344f;color:#e6edf3}</style></head><body><div class='wrap'>";
    html += "<h1>HA Remote Setup Portal</h1><p>Device IP: http://" + _deviceIP + "</p>";
    html += "<div class='card'><h2>Quick Setup</h2><p>Enter Home Assistant URL and token.</p><a class='btn pri' href='/quick'>Open Quick Setup</a></div>";
    html += "<div class='card'><h2>Full Configuration</h2><p>Edit WiFi, HA, orientation, theme, and dashboard settings.</p><a class='btn sec' href='/config'>Open Full Config</a></div>";
    html += "</div></body></html>";

    _server->send(200, "text/html", html);
}

void HASetupScreen::serveSetupPage() {
    _server->send_P(200, "text/html", HA_SETUP_HTML);
}

void HASetupScreen::handleSave() {
    if (!_server->hasArg("url") || !_server->hasArg("token")) {
        _server->send(400, "text/plain", "Missing fields");
        return;
    }

    String url   = _server->arg("url");
    String token = _server->arg("token");
    url.trim();
    token.trim();

    if (url.isEmpty() || token.isEmpty()) {
        _server->send(400, "text/plain", "Fields cannot be empty");
        return;
    }

    _server->send_P(200, "text/html", HA_SAVED_HTML);

    // Update status on screen
    if (_lblStatus) lv_label_set_text(_lblStatus, "Credentials received!");

    stopWebServer();

    if (_onSaved) _onSaved(url, token);
}

void HASetupScreen::serveConfigPage() {
    StorageManager &store = StorageManager::instance();

    String html;
    html.reserve(5000);
    html += "<!DOCTYPE html><html><head><meta charset='UTF-8'>";
    html += "<meta name='viewport' content='width=device-width,initial-scale=1'>";
    html += "<title>HA Remote Config</title>";
    html += "<style>*{box-sizing:border-box}body{font-family:system-ui,sans-serif;background:#0d1117;color:#e6edf3;padding:18px;max-width:700px;margin:0 auto}";
    html += "h1{color:#41bdf5;margin:0 0 8px}h2{margin:18px 0 8px;color:#e6edf3;font-size:1rem}";
    html += "label{display:block;color:#8b949e;font-size:.85rem;margin:8px 0 4px}";
    html += "input,select{width:100%;padding:10px;background:#161b22;border:1px solid #30363d;color:#e6edf3;border-radius:6px}";
    html += "button{margin-top:14px;padding:12px;background:#41bdf5;border:none;border-radius:6px;color:#0d1117;font-weight:700;width:100%}";
    html += "small{color:#8b949e}a{color:#41bdf5;text-decoration:none}</style></head><body>";
    html += "<h1>HA Remote Configuration</h1><small>Device: http://" + _deviceIP + "</small>";
    html += "<form action='/save-config' method='POST'>";

    html += "<h2>Home Assistant</h2>";
    html += "<label>Server URL</label><input name='url' value='" + store.getHAUrl() + "' required>";
    html += "<label>Long-Lived Token</label><input name='token' value='" + store.getHAToken() + "' required>";

    html += "<h2>WiFi</h2>";
    html += "<label>SSID</label><input name='ssid' value='" + store.getSSID() + "'>";
    html += "<label>Password</label><input name='pass' type='password' value='" + store.getPass() + "'>";

    html += "<h2>Display</h2>";
    html += "<label>Orientation</label><select name='rotation'>";
    for (int i = 0; i < 4; i++) {
        html += "<option value='" + String(i) + "'";
        if (store.getDisplayRotation() == i) html += " selected";
        html += ">Rotation " + String(i) + "</option>";
    }
    html += "</select>";

    html += "<label>Theme preset</label><select name='theme'>";
    html += "<option value='0'" + String(store.getThemePreset() == 0 ? " selected" : "") + ">Dark</option>";
    html += "<option value='1'" + String(store.getThemePreset() == 1 ? " selected" : "") + ">High Contrast</option>";
    html += "</select>";

    html += "<h2>Dashboard</h2>";
    html += "<label>Refresh interval (ms)</label><input name='refresh' type='number' min='2000' max='60000' value='" + String(store.getRefreshIntervalMs()) + "'>";
    html += "<label>Entity filter</label><select name='filter'>";
    html += "<option value='0'" + String(store.getEntityFilterMode() == 0 ? " selected" : "") + ">Lights + Outlets</option>";
    html += "<option value='1'" + String(store.getEntityFilterMode() == 1 ? " selected" : "") + ">Lights only</option>";
    html += "</select>";

    html += "<button type='submit'>Save All Settings</button></form>";
    html += "<p style='margin-top:12px'><a href='/'>Back to quick setup</a></p>";
    html += "</body></html>";

    _server->send(200, "text/html", html);
}

void HASetupScreen::handleSaveConfig() {
    StorageManager &store = StorageManager::instance();

    String url = _server->arg("url");
    String token = _server->arg("token");
    url.trim();
    token.trim();

    if (!url.isEmpty() && !token.isEmpty()) {
        store.setHA(url, token);
    }

    String ssid = _server->arg("ssid");
    String pass = _server->arg("pass");
    ssid.trim();
    if (!ssid.isEmpty()) {
        store.setWiFi(ssid, pass);
    }

    uint8_t rot = static_cast<uint8_t>(_server->arg("rotation").toInt() & 0x03);
    uint8_t theme = static_cast<uint8_t>(_server->arg("theme").toInt());
    uint32_t refresh = static_cast<uint32_t>(_server->arg("refresh").toInt());
    uint8_t filter = static_cast<uint8_t>(_server->arg("filter").toInt());

    if (refresh < 2000) refresh = 2000;
    if (refresh > 60000) refresh = 60000;
    if (theme > 1) theme = 0;
    if (filter > 1) filter = 0;

    store.setDisplayRotation(rot);
    store.setThemePreset(theme);
    store.setRefreshIntervalMs(refresh);
    store.setEntityFilterMode(filter);

    _server->send_P(200, "text/html", CONFIG_SAVED_HTML);

    if (_lblStatus) lv_label_set_text(_lblStatus, "Settings updated via web.");
}

// ─────────────────────────────────────────────────────────────────────────────
void HASetupScreen::update() {
    if (_serverReady && _server) {
        _server->handleClient();
    }
}

// ─────────────────────────────────────────────────────────────────────────────
void HASetupScreen::showManualPanel() {
    if (_manualPanel) return;   // already shown

    _manualPanel = lv_obj_create(_screen);
    lv_obj_set_size(_manualPanel, SCREEN_W, SCREEN_H);
    lv_obj_align(_manualPanel, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_add_style(_manualPanel, &Theme::style_screen, 0);

    lv_obj_t *lbl = lv_label_create(_manualPanel);
    lv_obj_add_style(lbl, &Theme::style_label_body, 0);
    lv_label_set_text(lbl, "Server URL");
    lv_obj_align(lbl, LV_ALIGN_TOP_LEFT, 8, 8);

    _taUrl = lv_textarea_create(_manualPanel);
    lv_obj_set_size(_taUrl, SCREEN_W - 16, 34);
    lv_obj_align(_taUrl, LV_ALIGN_TOP_LEFT, 8, 28);
    lv_textarea_set_placeholder_text(_taUrl, "http://192.168.1.10:8123");
    lv_textarea_set_one_line(_taUrl, true);

    lv_obj_t *lbl2 = lv_label_create(_manualPanel);
    lv_obj_add_style(lbl2, &Theme::style_label_body, 0);
    lv_label_set_text(lbl2, "Access Token");
    lv_obj_align_to(lbl2, _taUrl, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 4);

    _taToken = lv_textarea_create(_manualPanel);
    lv_obj_set_size(_taToken, SCREEN_W - 16, 34);
    lv_obj_align_to(_taToken, lbl2, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 2);
    lv_textarea_set_placeholder_text(_taToken, "Paste token here...");
    lv_textarea_set_one_line(_taToken, true);
    lv_textarea_set_password_mode(_taToken, true);

    _keyboard = lv_keyboard_create(_manualPanel);
    lv_obj_set_size(_keyboard, SCREEN_W, 120);
    lv_obj_align(_keyboard, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_keyboard_set_textarea(_keyboard, _taUrl);

    lv_obj_t *saveBtn = lv_btn_create(_manualPanel);
    Theme::applyBtnPrimary(saveBtn);
    lv_obj_set_size(saveBtn, 110, 30);
    lv_obj_align(saveBtn, LV_ALIGN_TOP_RIGHT, -8, 130);
    lv_obj_add_event_cb(saveBtn, onManualSaveBtn, LV_EVENT_CLICKED, this);
    lv_obj_t *saveLbl = lv_label_create(saveBtn);
    lv_label_set_text(saveLbl, "Save " LV_SYMBOL_RIGHT);
    lv_obj_center(saveLbl);
}

// ─────────────────────────────────────────────────────────────────────────────
void HASetupScreen::destroy() {
    stopWebServer();
    _screen = nullptr;
    _manualPanel = _taUrl = _taToken = _keyboard = nullptr;
    _lblIP = _lblStatus = nullptr;
}

// ─────────────────────────────────────────────────────────────────────────────
void HASetupScreen::onManualBtn(lv_event_t *e) {
    HASetupScreen *self = static_cast<HASetupScreen*>(lv_event_get_user_data(e));
    self->showManualPanel();
}

void HASetupScreen::onManualSaveBtn(lv_event_t *e) {
    HASetupScreen *self = static_cast<HASetupScreen*>(lv_event_get_user_data(e));
    if (!self->_taUrl || !self->_taToken) return;

    String url   = lv_textarea_get_text(self->_taUrl);
    String token = lv_textarea_get_text(self->_taToken);
    url.trim();
    token.trim();
    if (url.isEmpty() || token.isEmpty()) return;

    self->stopWebServer();
    if (self->_onSaved) self->_onSaved(url, token);
}
