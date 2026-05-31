#pragma once

// ─── App identity ────────────────────────────────────────────────────────────
#define APP_NAME        "HA Remote"
#define APP_VERSION     "1.0.0"

// ─── Screen ──────────────────────────────────────────────────────────────────
#define DEFAULT_TFT_ROTATION    1   // 1 = landscape 320×240
#define SCREEN_W        320
#define SCREEN_H        240
#define SCREEN_BL_PIN   21          // Backlight PWM pin

// ─── SD card (ELEGOO ESP32-32E documented pins) ───────────────────────────
#define SD_SCK_PIN      18
#define SD_MISO_PIN     19
#define SD_MOSI_PIN     23
#define SD_CS_PIN       5
#define SD_SPI_FREQ_HZ  25000000

// ─── Touch (XPT2046 — VSPI bus) ──────────────────────────────────────────────
#define TOUCH_CLK       25
#define TOUCH_MISO      39
#define TOUCH_MOSI      32
#define TOUCH_CS        33
#define TOUCH_IRQ       36

// Calibration: map raw ADC values → screen pixels
// Adjust these if touch accuracy is off
#define TOUCH_X_MIN     200
#define TOUCH_X_MAX     3700
#define TOUCH_Y_MIN     240
#define TOUCH_Y_MAX     3800
#define TOUCH_SWAP_XY   false
#define TOUCH_INVERT_X  false
#define TOUCH_INVERT_Y  false

// ─── RGB LED (common-anode, active-LOW on ELEGOO ESP32-32E) ───────────────
#define LED_R           22
#define LED_G           16
#define LED_B           17

// ─── Light sensor ────────────────────────────────────────────────────────────
#define PIN_LDR         34

// ─── NVS storage ─────────────────────────────────────────────────────────────
#define NVS_NAMESPACE   "ha_remote"
#define NVS_KEY_SSID    "wifi_ssid"
#define NVS_KEY_PASS    "wifi_pass"
#define NVS_KEY_HA_URL  "ha_url"
#define NVS_KEY_HA_TOK  "ha_token"
#define NVS_KEY_ROT     "rotation"
#define NVS_KEY_THEME   "theme"
#define NVS_KEY_REFRESH "refresh_ms"
#define NVS_KEY_FILTER  "entity_filter"

// ─── Timings ─────────────────────────────────────────────────────────────────
#define SPLASH_DURATION_MS      2500
#define WIFI_CONNECT_TIMEOUT_MS 15000
#define HA_CONNECT_TIMEOUT_MS   10000
#define WEB_PORTAL_TIMEOUT_MS   120000  // 2-min portal auto-close
#define SCREEN_IDLE_TIMEOUT_MS  300000  // 5 min idle -> dim/screensaver

// ─── UI defaults ─────────────────────────────────────────────────────────────
#define DEFAULT_THEME_PRESET        0   // dark
#define DEFAULT_ENTITY_FILTER_MODE  0   // lights + switches
#define DEFAULT_REFRESH_INTERVAL_MS  10000

// ─── Home Assistant ──────────────────────────────────────────────────────────
#define HA_DEFAULT_PORT         8123
#define HA_MAX_ENTITIES         64      // tile grid cap
#define HA_API_PATH             "/api/states"
#define HA_SERVICES_PATH        "/api/services"

// ─── Dashboard layout ────────────────────────────────────────────────────────
#define DASH_COLS               3
#define DASH_TILE_W             ((SCREEN_W) / (DASH_COLS))   // 106 px
#define DASH_TILE_H             90
#define DASH_STATUS_BAR_H       24
