#pragma once
#include <Arduino.h>

/**
 * StorageManager — thin wrapper around ESP32 Preferences (NVS).
 *
 * All keys are defined in config.h. Stored data:
 *   wifi_ssid   / wifi_pass   — WiFi credentials
 *   ha_url      / ha_token    — Home Assistant server + Long-Lived Token
 */
class StorageManager {
public:
    static StorageManager& instance();

    // ── Lifecycle ────────────────────────────────────────────────────────────
    void begin();
    void clear();   // wipe all saved config (factory reset)

    // ── WiFi ─────────────────────────────────────────────────────────────────
    String  getSSID()   const;
    String  getPass()   const;
    void    setWiFi(const String &ssid, const String &pass);
    bool    hasWiFi()   const;

    // ── Home Assistant ───────────────────────────────────────────────────────
    String  getHAUrl()      const;
    String  getHAToken()    const;
    void    setHA(const String &url, const String &token);
    bool    hasHA()         const;

    // ── Display / UI ────────────────────────────────────────────────────────
    uint8_t getDisplayRotation() const;
    void    setDisplayRotation(uint8_t rotation);

    uint8_t getThemePreset() const;
    void    setThemePreset(uint8_t preset);

    uint32_t getRefreshIntervalMs() const;
    void     setRefreshIntervalMs(uint32_t intervalMs);

    uint8_t getEntityFilterMode() const;
    void    setEntityFilterMode(uint8_t mode);

    uint32_t getIdleTimeoutMs() const;
    void     setIdleTimeoutMs(uint32_t timeoutMs);

    String getHiddenEntityIds() const;
    void   setHiddenEntityIds(const String &ids);

    // ── Convenience ──────────────────────────────────────────────────────────
    /** Returns true when both WiFi and HA credentials are present. */
    bool    isFullyConfigured() const;

private:
    StorageManager() = default;
    bool _ready = false;
};
