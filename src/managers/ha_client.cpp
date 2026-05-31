#include "ha_client.h"
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "config.h"

// Domains displayed on the dashboard
static const char* SUPPORTED_DOMAINS[] = {
    "switch", "light", "climate"
};
static const size_t SUPPORTED_DOMAIN_COUNT =
    sizeof(SUPPORTED_DOMAINS) / sizeof(SUPPORTED_DOMAINS[0]);

static bool endsWithIgnoreCase(const String &value, const char *suffix) {
    size_t valueLen = value.length();
    size_t suffixLen = strlen(suffix);
    if (valueLen < suffixLen) return false;

    size_t offset = valueLen - suffixLen;
    for (size_t i = 0; i < suffixLen; i++) {
        char a = tolower(static_cast<unsigned char>(value[offset + i]));
        char b = tolower(static_cast<unsigned char>(suffix[i]));
        if (a != b) return false;
    }
    return true;
}

static bool isHelperEntity(const String &entityId, const String &friendlyName) {
    String lowerId = entityId;
    lowerId.toLowerCase();

    String lowerName = friendlyName;
    lowerName.toLowerCase();

    if (lowerId.endsWith("_led") || lowerId.endsWith(".led")) return true;
    if (endsWithIgnoreCase(friendlyName, " LED")) return true;
    if (lowerName.endsWith(" indicator")) return true;
    return false;
}

// ─────────────────────────────────────────────────────────────────────────────
HAClient& HAClient::instance() {
    static HAClient c;
    return c;
}

// ─────────────────────────────────────────────────────────────────────────────
void HAClient::setServer(const String &url, const String &token) {
    // Strip trailing slash for consistent path building
    _serverUrl = url;
    while (_serverUrl.endsWith("/")) _serverUrl.remove(_serverUrl.length() - 1);
    _token = token;
}

// ─────────────────────────────────────────────────────────────────────────────
String HAClient::buildUrl(const String &path) const {
    return _serverUrl + path;
}

bool HAClient::isSupportedDomain(const String &domain) const {
    if (_entityFilterMode == 0) {
        return (domain == "switch" || domain == "light" || domain == "climate");
    }

    if (_entityFilterMode == 1) {
        return (domain == "light");
    }

    for (size_t i = 0; i < SUPPORTED_DOMAIN_COUNT; i++) {
        if (domain == SUPPORTED_DOMAINS[i]) return true;
    }
    return false;
}

// ─────────────────────────────────────────────────────────────────────────────
bool HAClient::testConnection() {
    if (_serverUrl.isEmpty() || _token.isEmpty()) {
        _lastError = "No server configured";
        return false;
    }

    HTTPClient http;
    http.begin(buildUrl("/api/"));
    http.addHeader("Authorization", "Bearer " + _token);
    http.addHeader("Content-Type", "application/json");
    http.setTimeout(8000);

    int code = http.GET();
    http.end();

    if (code == 200) return true;

    _lastError = "HTTP " + String(code);
    return false;
}

// ─────────────────────────────────────────────────────────────────────────────
bool HAClient::fetchEntities() {
    HTTPClient http;
    http.begin(buildUrl("/api/states"));
    http.addHeader("Authorization", "Bearer " + _token);
    http.addHeader("Content-Type", "application/json");
    http.setTimeout(10000);

    int code = http.GET();
    if (code != 200) {
        _lastError = "HTTP " + String(code);
        http.end();
        return false;
    }

    // Stream-parse the JSON array to save RAM
    WiFiClient *stream = http.getStreamPtr();

    // Use a filter to limit memory use
    JsonDocument filter;
    filter[0]["entity_id"]                         = true;
    filter[0]["state"]                             = true;
    filter[0]["attributes"]["friendly_name"]       = true;
    filter[0]["attributes"]["unit_of_measurement"] = true;
    filter[0]["attributes"]["rgb_color"]            = true;
    filter[0]["attributes"]["brightness"]           = true;
    filter[0]["attributes"]["color_temp"]           = true;
    filter[0]["attributes"]["color_temp_kelvin"]    = true;
    filter[0]["attributes"]["color_mode"]           = true;
    filter[0]["attributes"]["temperature"]          = true;
    filter[0]["attributes"]["temperature_unit"]     = true;
    filter[0]["attributes"]["humidity"]             = true;
    filter[0]["attributes"]["pressure"]             = true;
    filter[0]["attributes"]["pressure_unit"]        = true;
    filter[0]["attributes"]["wind_speed"]           = true;
    filter[0]["attributes"]["wind_speed_unit"]      = true;
    filter[0]["attributes"]["wind_bearing"]         = true;
    filter[0]["attributes"]["apparent_temperature"] = true;
    filter[0]["attributes"]["dew_point"]            = true;
    filter[0]["attributes"]["current_temperature"]  = true;
    filter[0]["attributes"]["current_humidity"]     = true;
    filter[0]["attributes"]["hvac_action"]          = true;

    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, *stream,
                                               DeserializationOption::Filter(filter));
    http.end();

    if (err) {
        _lastError = err.c_str();
        return false;
    }

    _entities.clear();
    _weather = HAWeatherData();
    bool weatherTileAdded = false;

    for (JsonObject obj : doc.as<JsonArray>()) {
        String id = obj["entity_id"].as<String>();
        int dotIdx = id.indexOf('.');
        if (dotIdx < 0) continue;

        String domain = id.substring(0, dotIdx);
        JsonObject attrs = obj["attributes"].as<JsonObject>();

        if (domain == "weather" && !_weather.available) {
            _weather.available = true;
            _weather.entityId = id;
            _weather.friendlyName = attrs["friendly_name"] | id.substring(dotIdx + 1).c_str();
            _weather.state = obj["state"].as<String>();

            String temperatureUnit = attrs["temperature_unit"] | "";
            JsonVariant t = attrs["temperature"];
            if (!t.isNull()) {
                if (t.is<float>() || t.is<double>() || t.is<int>()) {
                    _weather.temperature = String(t.as<float>(), 1);
                } else {
                    _weather.temperature = t.as<String>();
                }
                if (!temperatureUnit.isEmpty()) _weather.temperature += " " + temperatureUnit;
            }

            JsonVariant h = attrs["humidity"];
            if (!h.isNull()) {
                if (h.is<float>() || h.is<double>() || h.is<int>()) {
                    _weather.humidity = String(h.as<float>(), 0) + "%";
                } else {
                    _weather.humidity = h.as<String>();
                }
            }

            String pressureUnit = attrs["pressure_unit"] | "";
            JsonVariant p = attrs["pressure"];
            if (!p.isNull()) {
                if (p.is<float>() || p.is<double>() || p.is<int>()) {
                    _weather.pressure = String(p.as<float>(), 1);
                } else {
                    _weather.pressure = p.as<String>();
                }
                if (!pressureUnit.isEmpty()) _weather.pressure += " " + pressureUnit;
            }

            String windUnit = attrs["wind_speed_unit"] | "";
            JsonVariant ws = attrs["wind_speed"];
            if (!ws.isNull()) {
                if (ws.is<float>() || ws.is<double>() || ws.is<int>()) {
                    _weather.windSpeed = String(ws.as<float>(), 1);
                } else {
                    _weather.windSpeed = ws.as<String>();
                }
                if (!windUnit.isEmpty()) _weather.windSpeed += " " + windUnit;
            }

            JsonVariant wb = attrs["wind_bearing"];
            if (!wb.isNull()) {
                if (wb.is<float>() || wb.is<double>() || wb.is<int>()) {
                    _weather.windBearing = String(wb.as<float>(), 0) + " deg";
                } else {
                    _weather.windBearing = wb.as<String>();
                }
            }

            JsonVariant at = attrs["apparent_temperature"];
            if (!at.isNull()) {
                if (at.is<float>() || at.is<double>() || at.is<int>()) {
                    _weather.apparentTemperature = String(at.as<float>(), 1);
                } else {
                    _weather.apparentTemperature = at.as<String>();
                }
                if (!temperatureUnit.isEmpty()) _weather.apparentTemperature += " " + temperatureUnit;
            }

            JsonVariant dp = attrs["dew_point"];
            if (!dp.isNull()) {
                if (dp.is<float>() || dp.is<double>() || dp.is<int>()) {
                    _weather.dewPoint = String(dp.as<float>(), 1);
                } else {
                    _weather.dewPoint = dp.as<String>();
                }
                if (!temperatureUnit.isEmpty()) _weather.dewPoint += " " + temperatureUnit;
            }
        }

        if (domain == "weather" && !weatherTileAdded) {
            HAEntity w;
            w.entityId = id;
            w.domain = "weather";
            w.friendlyName = _weather.friendlyName.isEmpty() ? String("Weather") : _weather.friendlyName;
            w.state = _weather.state;
            w.isControllable = false;
            w.weatherCondition = _weather.state;
            w.weatherTemp = _weather.temperature;
            w.weatherHumidity = _weather.humidity;

            _entities.push_back(w);
            weatherTileAdded = true;

            if (_entities.size() >= HA_MAX_ENTITIES) break;
            continue;
        }

        if (!isSupportedDomain(domain)) continue;

        HAEntity e;
        e.entityId      = id;
        e.domain        = domain;
        e.state         = obj["state"].as<String>();
        e.friendlyName  = attrs["friendly_name"]
                              | id.substring(dotIdx + 1).c_str();
        e.unit          = attrs["unit_of_measurement"] | "";

        if (domain == "light") {
            int brightnessRaw = attrs["brightness"] | -1;
            if (brightnessRaw >= 0) {
                e.hasBrightness = true;
                int pct = (brightnessRaw * 100) / 255;
                e.brightnessPct = static_cast<uint8_t>(constrain(pct, 1, 100));
            }

            int kelvin = attrs["color_temp_kelvin"] | -1;
            if (kelvin > 0) {
                e.hasKelvin = true;
                e.kelvin = static_cast<uint16_t>(constrain(kelvin, 1500, 9000));
            } else {
                int mired = attrs["color_temp"] | -1;
                if (mired > 0) {
                    uint32_t converted = 1000000UL / static_cast<uint32_t>(mired);
                    e.hasKelvin = true;
                    e.kelvin = static_cast<uint16_t>(constrain(static_cast<int>(converted), 1500, 9000));
                }
            }

            String colorMode = attrs["color_mode"] | "";
            colorMode.toLowerCase();
            e.colorModeIsWhite = (colorMode == "color_temp" || colorMode == "white");

            JsonArray rgb = attrs["rgb_color"].as<JsonArray>();
            if (!rgb.isNull() && rgb.size() >= 3) {
                int r = rgb[0] | -1;
                int g = rgb[1] | -1;
                int b = rgb[2] | -1;
                if (r >= 0 && g >= 0 && b >= 0) {
                    e.hasColor = true;
                    e.colorR = static_cast<uint8_t>(constrain(r, 0, 255));
                    e.colorG = static_cast<uint8_t>(constrain(g, 0, 255));
                    e.colorB = static_cast<uint8_t>(constrain(b, 0, 255));
                }
            }
        }

        if (domain == "climate") {
            JsonVariant ct = attrs["current_temperature"];
            if (!ct.isNull()) e.climateCurrentTemp = String(ct.as<float>(), 1);
            JsonVariant ch = attrs["current_humidity"];
            if (!ch.isNull()) e.climateHumidity = String(ch.as<float>(), 0) + "%";
            JsonVariant sp = attrs["temperature"];
            if (!sp.isNull()) e.climateSetpoint = String(sp.as<float>(), 1);
            e.climateHvacAction = attrs["hvac_action"] | "";
        }

        if (isHelperEntity(e.entityId, e.friendlyName)) continue;

        // Entities we can control
        e.isControllable = (domain == "switch"  || domain == "light"  ||
                            domain == "input_boolean" || domain == "climate" ||
                            domain == "cover"   || domain == "media_player" ||
                            domain == "lock"    || domain == "automation" ||
                            domain == "script"  || domain == "scene");

        _entities.push_back(e);

        if (_entities.size() >= HA_MAX_ENTITIES) break;
    }

    return true;
}

// ─────────────────────────────────────────────────────────────────────────────
bool HAClient::callService(const String &domain,
                           const String &service,
                           const String &entityId,
                           const String &extraJson) {
    HTTPClient http;
    http.begin(buildUrl("/api/services/" + domain + "/" + service));
    http.addHeader("Authorization", "Bearer " + _token);
    http.addHeader("Content-Type", "application/json");
    http.setTimeout(8000);

    String body = "{\"entity_id\":\"" + entityId + "\"";
    if (!extraJson.isEmpty()) {
        body += "," + extraJson;
    }
    body += "}";
    int code = http.POST(body);
    http.end();

    if (code == 200 || code == 201) return true;

    _lastError = "HTTP " + String(code);
    return false;
}
