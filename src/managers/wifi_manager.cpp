#include "wifi_manager.h"
#include <WiFi.h>
#include <algorithm>
#include "config.h"

// ─────────────────────────────────────────────────────────────────────────────
WiFiMgr& WiFiMgr::instance() {
    static WiFiMgr w;
    return w;
}

// ─────────────────────────────────────────────────────────────────────────────
std::vector<WiFiNetwork> WiFiMgr::scan() {
    std::vector<WiFiNetwork> results;

    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    delay(100);

    int n = WiFi.scanNetworks(false, false); // blocking, no hidden
    if (n <= 0) return results;

    results.reserve(n);
    for (int i = 0; i < n; i++) {
        WiFiNetwork net;
        net.ssid   = WiFi.SSID(i);
        net.rssi   = WiFi.RSSI(i);
        net.isOpen = (WiFi.encryptionType(i) == WIFI_AUTH_OPEN);
        results.push_back(net);
    }

    // Sort strongest first
    std::sort(results.begin(), results.end(),
              [](const WiFiNetwork &a, const WiFiNetwork &b) {
                  return a.rssi > b.rssi;
              });

    WiFi.scanDelete();
    return results;
}

// ─────────────────────────────────────────────────────────────────────────────
void WiFiMgr::connect(const String &ssid, const String &pass) {
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid.c_str(), pass.c_str());
}

bool WiFiMgr::waitForConnect(uint32_t timeoutMs) {
    uint32_t start = millis();
    while (WiFi.status() != WL_CONNECTED) {
        if (millis() - start >= timeoutMs) return false;
        delay(200);
    }
    return true;
}

bool WiFiMgr::isConnected() const {
    return WiFi.status() == WL_CONNECTED;
}

String WiFiMgr::localIP() const {
    return WiFi.localIP().toString();
}

String WiFiMgr::macAddress() const {
    return WiFi.macAddress();
}

int32_t WiFiMgr::rssi() const {
    return WiFi.RSSI();
}

void WiFiMgr::disconnect() {
    WiFi.disconnect(true);
}
