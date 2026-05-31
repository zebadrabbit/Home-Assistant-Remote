#include "ha_client.h"
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "config.h"

// Domains displayed on the dashboard
static const char* SUPPORTED_DOMAINS[] = {
    "switch", "light"
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
        return (domain == "switch" || domain == "light");
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

    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, *stream,
                                               DeserializationOption::Filter(filter));
    http.end();

    if (err) {
        _lastError = err.c_str();
        return false;
    }

    _entities.clear();

    for (JsonObject obj : doc.as<JsonArray>()) {
        String id = obj["entity_id"].as<String>();
        int dotIdx = id.indexOf('.');
        if (dotIdx < 0) continue;

        String domain = id.substring(0, dotIdx);
        if (!isSupportedDomain(domain)) continue;

        HAEntity e;
        e.entityId      = id;
        e.domain        = domain;
        e.state         = obj["state"].as<String>();
        e.friendlyName  = obj["attributes"]["friendly_name"]
                              | id.substring(dotIdx + 1).c_str();
        e.unit          = obj["attributes"]["unit_of_measurement"] | "";

        if (domain == "light") {
            int brightnessRaw = obj["attributes"]["brightness"] | -1;
            if (brightnessRaw >= 0) {
                e.hasBrightness = true;
                int pct = (brightnessRaw * 100) / 255;
                e.brightnessPct = static_cast<uint8_t>(constrain(pct, 1, 100));
            }

            int kelvin = obj["attributes"]["color_temp_kelvin"] | -1;
            if (kelvin > 0) {
                e.hasKelvin = true;
                e.kelvin = static_cast<uint16_t>(constrain(kelvin, 1500, 9000));
            } else {
                int mired = obj["attributes"]["color_temp"] | -1;
                if (mired > 0) {
                    uint32_t converted = 1000000UL / static_cast<uint32_t>(mired);
                    e.hasKelvin = true;
                    e.kelvin = static_cast<uint16_t>(constrain(static_cast<int>(converted), 1500, 9000));
                }
            }

            String colorMode = obj["attributes"]["color_mode"] | "";
            colorMode.toLowerCase();
            e.colorModeIsWhite = (colorMode == "color_temp" || colorMode == "white");

            JsonArray rgb = obj["attributes"]["rgb_color"].as<JsonArray>();
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
