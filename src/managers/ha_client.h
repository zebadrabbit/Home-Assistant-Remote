#pragma once
#include <Arduino.h>
#include <vector>
#include <functional>
#include "config.h"

// ─────────────────────────────────────────────────────────────────────────────
//  Entity model
// ─────────────────────────────────────────────────────────────────────────────
struct HAEntity {
    String entityId;        // e.g. "switch.living_room"
    String domain;          // e.g. "switch"
    String friendlyName;    // e.g. "Living Room"
    String state;           // e.g. "on", "off", "22.5"
    String unit;            // e.g. "°C", "%" (sensors)
    bool   isControllable;  // false for pure sensors
    bool   hasColor = false;
    uint8_t colorR = 0;
    uint8_t colorG = 0;
    uint8_t colorB = 0;
    bool   hasBrightness = false;
    uint8_t brightnessPct = 0;
    bool   hasKelvin = false;
    uint16_t kelvin = 0;
    bool   colorModeIsWhite = false;
    String weatherCondition;
    String weatherTemp;
    String weatherHumidity;
};

struct HAWeatherData {
    bool available = false;
    String entityId;
    String friendlyName;
    String state;
    String temperature;
    String humidity;
    String pressure;
    String windSpeed;
    String windBearing;
    String apparentTemperature;
    String dewPoint;
};

// ─────────────────────────────────────────────────────────────────────────────
//  Client
// ─────────────────────────────────────────────────────────────────────────────
class HAClient {
public:
    static HAClient& instance();

    // ── Configuration ────────────────────────────────────────────────────────
    void setServer(const String &url, const String &token);

    // ── Connection test ──────────────────────────────────────────────────────
    /** Sends GET /api/ and checks for 200 OK. Blocking ~3 s. */
    bool testConnection();

    // ── Entity fetch ─────────────────────────────────────────────────────────
    /**
     * Fetches /api/states, filters to supported domains, fills _entities.
     * Returns true on success.
     */
    bool fetchEntities();
    const std::vector<HAEntity>& entities() const { return _entities; }
    const HAWeatherData& weather() const { return _weather; }
    void setEntityFilterMode(uint8_t mode) { _entityFilterMode = mode; }

    // ── Service call ─────────────────────────────────────────────────────────
    /**
     * Calls /api/services/<domain>/<service> with {"entity_id": entityId}.
     * Common services: "turn_on", "turn_off", "toggle".
     */
    bool callService(const String &domain,
                     const String &service,
                     const String &entityId,
                     const String &extraJson = "");

    // ── Helpers ──────────────────────────────────────────────────────────────
    String lastError() const { return _lastError; }

private:
    HAClient() = default;

    String _serverUrl;
    String _token;
    String _lastError;
    std::vector<HAEntity> _entities;
    HAWeatherData _weather;
    uint8_t _entityFilterMode = DEFAULT_ENTITY_FILTER_MODE;

    String  buildUrl(const String &path) const;
    bool    isSupportedDomain(const String &domain) const;
};
