#pragma once
#include <Arduino.h>
#include <vector>

struct WiFiNetwork {
    String  ssid;
    int32_t rssi;
    bool    isOpen;   // no password required
};

/**
 * WiFiManager — wraps ESP32 WiFi operations needed by the setup wizard
 * and the running app.  Does NOT conflict with the Arduino WiFiManager
 * library — that library is not used in this project.
 */
class WiFiMgr {
public:
    static WiFiMgr& instance();

    // ── Scan ─────────────────────────────────────────────────────────────────
    /** Blocking scan. Returns list sorted by signal strength. */
    std::vector<WiFiNetwork> scan();

    // ── Connect ──────────────────────────────────────────────────────────────
    /** Async connect — call isConnected() / waitForConnect() to poll. */
    void connect(const String &ssid, const String &pass);

    /** Block up to timeoutMs. Returns true on success. */
    bool waitForConnect(uint32_t timeoutMs);

    bool        isConnected()  const;
    String      localIP()      const;
    String      macAddress()   const;
    int32_t     rssi()         const;

    // ── Disconnect ───────────────────────────────────────────────────────────
    void disconnect();

private:
    WiFiMgr() = default;
};
