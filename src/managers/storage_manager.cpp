#include "storage_manager.h"
#include <Preferences.h>
#include "config.h"

// ─────────────────────────────────────────────────────────────────────────────
StorageManager& StorageManager::instance() {
    static StorageManager s;
    return s;
}

// ─────────────────────────────────────────────────────────────────────────────
void StorageManager::begin() {
    _ready = true;
}

void StorageManager::clear() {
    Preferences prefs;
    prefs.begin(NVS_NAMESPACE, false);
    prefs.clear();
    prefs.end();
}

// ─────────────────────────────────────────────────────────────────────────────
//  WiFi
// ─────────────────────────────────────────────────────────────────────────────
String StorageManager::getSSID() const {
    Preferences prefs;
    prefs.begin(NVS_NAMESPACE, true);
    String v = prefs.getString(NVS_KEY_SSID, "");
    prefs.end();
    return v;
}

String StorageManager::getPass() const {
    Preferences prefs;
    prefs.begin(NVS_NAMESPACE, true);
    String v = prefs.getString(NVS_KEY_PASS, "");
    prefs.end();
    return v;
}

void StorageManager::setWiFi(const String &ssid, const String &pass) {
    Preferences prefs;
    prefs.begin(NVS_NAMESPACE, false);
    prefs.putString(NVS_KEY_SSID, ssid);
    prefs.putString(NVS_KEY_PASS, pass);
    prefs.end();
}

bool StorageManager::hasWiFi() const {
    return getSSID().length() > 0;
}

// ─────────────────────────────────────────────────────────────────────────────
//  Home Assistant
// ─────────────────────────────────────────────────────────────────────────────
String StorageManager::getHAUrl() const {
    Preferences prefs;
    prefs.begin(NVS_NAMESPACE, true);
    String v = prefs.getString(NVS_KEY_HA_URL, "");
    prefs.end();
    return v;
}

String StorageManager::getHAToken() const {
    Preferences prefs;
    prefs.begin(NVS_NAMESPACE, true);
    String v = prefs.getString(NVS_KEY_HA_TOK, "");
    prefs.end();
    return v;
}

void StorageManager::setHA(const String &url, const String &token) {
    Preferences prefs;
    prefs.begin(NVS_NAMESPACE, false);
    prefs.putString(NVS_KEY_HA_URL, url);
    prefs.putString(NVS_KEY_HA_TOK, token);
    prefs.end();
}

bool StorageManager::hasHA() const {
    return getHAUrl().length() > 0 && getHAToken().length() > 0;
}

// ─────────────────────────────────────────────────────────────────────────────
//  Display / UI
// ─────────────────────────────────────────────────────────────────────────────
uint8_t StorageManager::getDisplayRotation() const {
    Preferences prefs;
    prefs.begin(NVS_NAMESPACE, true);
    uint8_t v = prefs.getUChar(NVS_KEY_ROT, DEFAULT_TFT_ROTATION);
    prefs.end();
    return v;
}

void StorageManager::setDisplayRotation(uint8_t rotation) {
    Preferences prefs;
    prefs.begin(NVS_NAMESPACE, false);
    prefs.putUChar(NVS_KEY_ROT, rotation);
    prefs.end();
}

uint8_t StorageManager::getThemePreset() const {
    Preferences prefs;
    prefs.begin(NVS_NAMESPACE, true);
    uint8_t v = prefs.getUChar(NVS_KEY_THEME, DEFAULT_THEME_PRESET);
    prefs.end();
    return v;
}

void StorageManager::setThemePreset(uint8_t preset) {
    Preferences prefs;
    prefs.begin(NVS_NAMESPACE, false);
    prefs.putUChar(NVS_KEY_THEME, preset);
    prefs.end();
}

uint32_t StorageManager::getRefreshIntervalMs() const {
    Preferences prefs;
    prefs.begin(NVS_NAMESPACE, true);
    uint32_t v = prefs.getUInt(NVS_KEY_REFRESH, DEFAULT_REFRESH_INTERVAL_MS);
    prefs.end();
    return v;
}

void StorageManager::setRefreshIntervalMs(uint32_t intervalMs) {
    Preferences prefs;
    prefs.begin(NVS_NAMESPACE, false);
    prefs.putUInt(NVS_KEY_REFRESH, intervalMs);
    prefs.end();
}

uint8_t StorageManager::getEntityFilterMode() const {
    Preferences prefs;
    prefs.begin(NVS_NAMESPACE, true);
    uint8_t v = prefs.getUChar(NVS_KEY_FILTER, DEFAULT_ENTITY_FILTER_MODE);
    prefs.end();
    return v;
}

void StorageManager::setEntityFilterMode(uint8_t mode) {
    Preferences prefs;
    prefs.begin(NVS_NAMESPACE, false);
    prefs.putUChar(NVS_KEY_FILTER, mode);
    prefs.end();
}

// ─────────────────────────────────────────────────────────────────────────────
bool StorageManager::isFullyConfigured() const {
    return hasWiFi() && hasHA();
}
