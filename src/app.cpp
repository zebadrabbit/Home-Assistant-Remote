#include "app.h"
#include "config.h"
#include "ui/theme.h"
#include "managers/storage_manager.h"
#include "managers/wifi_manager.h"
#include "managers/ha_client.h"
#include "screens/splash_screen.h"
#include "screens/wifi_screen.h"
#include "screens/ha_setup_screen.h"
#include "screens/loading_screen.h"
#include "screens/dashboard_screen.h"

#include <SPI.h>
#include <SD.h>
#include <esp_timer.h>

// ─────────────────────────────────────────────────────────────────────────────
//  Hardware objects (global so callbacks can reach them)
// ─────────────────────────────────────────────────────────────────────────────
static TFT_eSPI          tft;
static SPIClass          touchSPI(VSPI);
static XPT2046_Touchscreen ts(TOUCH_CS, TOUCH_IRQ);

static void probeSdCard() {
    Serial.println("[SD] Probing SD card...");

    SPI.begin(SD_SCK_PIN, SD_MISO_PIN, SD_MOSI_PIN, SD_CS_PIN);
    if (!SD.begin(SD_CS_PIN, SPI, SD_SPI_FREQ_HZ)) {
        Serial.printf("[SD] Mount failed (CS=%d, SCK=%d, MISO=%d, MOSI=%d)\n",
                      SD_CS_PIN, SD_SCK_PIN, SD_MISO_PIN, SD_MOSI_PIN);
        return;
    }

    uint8_t cardType = SD.cardType();
    if (cardType == CARD_NONE) {
        Serial.println("[SD] No card detected");
        SD.end();
        return;
    }

    const char *typeName = "UNKNOWN";
    if (cardType == CARD_MMC) typeName = "MMC";
    else if (cardType == CARD_SD) typeName = "SDSC";
    else if (cardType == CARD_SDHC) typeName = "SDHC/SDXC";

    uint64_t cardSizeMB = SD.cardSize() / (1024ULL * 1024ULL);
    uint64_t totalMB = SD.totalBytes() / (1024ULL * 1024ULL);
    uint64_t usedMB = SD.usedBytes() / (1024ULL * 1024ULL);

    Serial.printf("[SD] Card type: %s\n", typeName);
    Serial.printf("[SD] Card size: %llu MB\n", cardSizeMB);
    Serial.printf("[SD] FS total/used: %llu MB / %llu MB\n", totalMB, usedMB);

    const char *probePath = "/sd_probe.txt";
    File writeFile = SD.open(probePath, FILE_WRITE);
    if (!writeFile) {
        Serial.println("[SD] Write test failed: open for write");
        SD.end();
        return;
    }

    writeFile.println("HA Remote SD probe");
    writeFile.close();

    File readFile = SD.open(probePath, FILE_READ);
    if (!readFile) {
        Serial.println("[SD] Read test failed: open for read");
        SD.end();
        return;
    }

    String line = readFile.readStringUntil('\n');
    readFile.close();
    Serial.printf("[SD] Read test OK: %s\n", line.c_str());
    SD.end();
}

// LVGL draw buffers (1/10 of screen — saves ~15 KB vs full buffer)
static lv_color_t lvBuf1[SCREEN_W * (SCREEN_H / 10)];
static lv_color_t lvBuf2[SCREEN_W * (SCREEN_H / 10)];

// ─────────────────────────────────────────────────────────────────────────────
App& App::instance() {
    static App a;
    return a;
}

// ─────────────────────────────────────────────────────────────────────────────
//  Hardware init
// ─────────────────────────────────────────────────────────────────────────────
void App::initDisplay() {
    tft.init();
    uint8_t rotation = StorageManager::instance().getDisplayRotation();
    tft.setRotation(rotation % 4);
    tft.fillScreen(TFT_BLACK);

    // Backlight on
    pinMode(SCREEN_BL_PIN, OUTPUT);
    digitalWrite(SCREEN_BL_PIN, HIGH);
}

void App::initTouch() {
    touchSPI.begin(TOUCH_CLK, TOUCH_MISO, TOUCH_MOSI, TOUCH_CS);
    ts.begin(touchSPI);
    uint8_t rotation = StorageManager::instance().getDisplayRotation();
    ts.setRotation(rotation % 4);
}

// ─────────────────────────────────────────────────────────────────────────────
//  LVGL bridge callbacks
// ─────────────────────────────────────────────────────────────────────────────
void App::dispFlush(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *color_p) {
    uint32_t w = area->x2 - area->x1 + 1;
    uint32_t h = area->y2 - area->y1 + 1;
    tft.startWrite();
    tft.setAddrWindow(area->x1, area->y1, w, h);
    tft.pushColors(reinterpret_cast<uint16_t*>(&color_p->full), w * h, true);
    tft.endWrite();
    lv_disp_flush_ready(drv);
}

void App::touchRead(lv_indev_drv_t * /*drv*/, lv_indev_data_t *data) {
    bool touched = ts.tirqTouched() && ts.touched();
    if (!touched) {
        data->state = LV_INDEV_STATE_REL;
        return;
    }

    TS_Point p = ts.getPoint();

    // Map raw ADC values → screen pixels
    int16_t x = map(p.x, TOUCH_X_MIN, TOUCH_X_MAX, 0, SCREEN_W  - 1);
    int16_t y = map(p.y, TOUCH_Y_MIN, TOUCH_Y_MAX, 0, SCREEN_H - 1);

#if TOUCH_SWAP_XY
    int16_t tmp = x; x = y; y = tmp;
#endif
#if TOUCH_INVERT_X
    x = SCREEN_W - 1 - x;
#endif
#if TOUCH_INVERT_Y
    y = SCREEN_H - 1 - y;
#endif

    x = constrain(x, 0, SCREEN_W  - 1);
    y = constrain(y, 0, SCREEN_H - 1);

    if (App::instance().onTouchActivity()) {
        data->state = LV_INDEV_STATE_REL;
        return;
    }

    data->state   = LV_INDEV_STATE_PR;
    data->point.x = x;
    data->point.y = y;
}

bool App::onTouchActivity() {
    _lastTouchActivityMs = millis();
    if (_idleModeActive) {
        disableIdleMode();
        return true;  // consume first touch as wake gesture
    }
    return false;
}

void App::enableIdleMode() {
    if (_idleModeActive) return;

    _idleOverlay = lv_obj_create(lv_layer_top());
    lv_obj_set_size(_idleOverlay, SCREEN_W, SCREEN_H);
    lv_obj_set_style_bg_color(_idleOverlay, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(_idleOverlay, LV_OPA_80, 0);
    lv_obj_set_style_border_width(_idleOverlay, 0, 0);
    lv_obj_set_style_radius(_idleOverlay, 0, 0);
    lv_obj_set_style_pad_all(_idleOverlay, 0, 0);

    lv_obj_t *hint = lv_label_create(_idleOverlay);
    lv_obj_add_style(hint, &Theme::style_label_dim, 0);
    lv_label_set_text(hint, "Tap to wake");
    lv_obj_center(hint);

    _idleModeActive = true;
}

void App::disableIdleMode() {
    if (_idleOverlay) {
        lv_obj_del(_idleOverlay);
        _idleOverlay = nullptr;
    }
    _idleModeActive = false;
}

void App::updateIdleMode() {
    if (_state != AppState::DASHBOARD) {
        disableIdleMode();
        return;
    }

    if (_idleModeActive) return;
    if (millis() - _lastTouchActivityMs >= SCREEN_IDLE_TIMEOUT_MS) {
        enableIdleMode();
    }
}

// ─────────────────────────────────────────────────────────────────────────────
void App::initLVGL() {
    lv_init();
    Theme::init();

    // ── Display driver ───────────────────────────────────────────────────────
    static lv_disp_draw_buf_t drawBuf;
    lv_disp_draw_buf_init(&drawBuf, lvBuf1, lvBuf2,
                          SCREEN_W * (SCREEN_H / 10));

    static lv_disp_drv_t dispDrv;
    lv_disp_drv_init(&dispDrv);
    dispDrv.hor_res  = SCREEN_W;
    dispDrv.ver_res  = SCREEN_H;
    dispDrv.flush_cb = dispFlush;
    dispDrv.draw_buf = &drawBuf;
    lv_disp_drv_register(&dispDrv);

    // ── Touch input driver ───────────────────────────────────────────────────
    static lv_indev_drv_t indevDrv;
    lv_indev_drv_init(&indevDrv);
    indevDrv.type    = LV_INDEV_TYPE_POINTER;
    indevDrv.read_cb = touchRead;
    lv_indev_drv_register(&indevDrv);

    // ── 1 ms LVGL tick via esp_timer ─────────────────────────────────────────
    esp_timer_create_args_t timerArgs = {};
    timerArgs.callback = [](void*){ lv_tick_inc(1); };
    timerArgs.name     = "lv_tick";
    timerArgs.skip_unhandled_events = true;

    esp_timer_handle_t tickTimer;
    esp_timer_create(&timerArgs, &tickTimer);
    esp_timer_start_periodic(tickTimer, 1000);  // 1000 µs = 1 ms
}

// ─────────────────────────────────────────────────────────────────────────────
//  Public entry points
// ─────────────────────────────────────────────────────────────────────────────
void App::init() {
    Serial.begin(115200);
    probeSdCard();

    StorageManager::instance().begin();
    _entityPollIntervalMs = StorageManager::instance().getRefreshIntervalMs();
    HAClient::instance().setEntityFilterMode(StorageManager::instance().getEntityFilterMode());
    initDisplay();
    initTouch();
    initLVGL();
    _lastTouchActivityMs = millis();

    transitionTo(AppState::SPLASH);
}

void App::loop() {
    lv_task_handler();

    processPendingTransition();
    processPendingDestroy();

    switch (_state) {
        case AppState::SPLASH:
            SplashScreen::instance().update();
            break;

        case AppState::WIFI_SETUP:
            WiFiScreen::instance().update();
            break;

        case AppState::WIFI_CONNECT:
            LoadingScreen::instance().update();
            // WiFi connection is attempted in enterWifiConnect; polling handled
            // via LoadingScreen::showResult callback
            break;

        case AppState::HA_SETUP:
            HASetupScreen::instance().update();  // pumps WebServer
            LoadingScreen::instance().update();
            break;

        case AppState::HA_CONNECT:
            LoadingScreen::instance().update();
            if (_haFetchDone) {
                _haFetchDone = false;
                _haFetchInProgress = false;
                _haFetchTask = nullptr;

                auto &ha = HAClient::instance();
                if (_haFetchOk) {
                    LoadingScreen::instance().showResult(
                        true,
                        ("Found " + String(ha.entities().size()) + " entities").c_str(),
                        1200,
                        [this]() { requestTransition(AppState::DASHBOARD); }
                    );
                } else {
                    LoadingScreen::instance().showResult(
                        false,
                        ("HA error: " + ha.lastError()).c_str(),
                        3000,
                        [this]() { requestTransition(AppState::HA_SETUP); }
                    );
                }
            } else if (_haFetchInProgress && (millis() - _haFetchStartMs > 30000)) {
                _haFetchInProgress = false;
                _haFetchDone = false;
                _haFetchTask = nullptr;
                LoadingScreen::instance().showResult(
                    false,
                    "HA entity fetch timed out.",
                    3000,
                    [this]() { requestTransition(AppState::HA_SETUP); }
                );
            }
            break;

        case AppState::DASHBOARD: {
            DashboardScreen::instance().update();
            updateIdleMode();

            // Periodic entity refresh
            if (millis() - _lastEntityPollMs >= _entityPollIntervalMs) {
                _lastEntityPollMs = millis();
                if (HAClient::instance().fetchEntities()) {
                    DashboardScreen::instance().populate(
                        HAClient::instance().entities());
                    DashboardScreen::instance().updateStatusBar(
                        true, WiFiMgr::instance().rssi());
                }
            }
            break;
        }

        default:
            disableIdleMode();
            break;
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  State transitions
// ─────────────────────────────────────────────────────────────────────────────
void App::transitionTo(AppState next) {
    AppState previous = _state;
    _state = next;

    if (next == AppState::SETTINGS) {
        enterSettings();
        return;
    }

    switch (next) {
        case AppState::SPLASH:       enterSplash();      break;
        case AppState::CHECK_CONFIG: enterCheckConfig(); break;
        case AppState::WIFI_SETUP:   enterWifiSetup();   break;
        case AppState::WIFI_CONNECT: enterWifiConnect(_pendingSSID, _pendingPass); break;
        case AppState::HA_SETUP:     enterHaSetup();     break;
        case AppState::HA_CONNECT:   enterHaConnect();   break;
        case AppState::DASHBOARD:    enterDashboard();   break;
        default: break;
    }

    if (_transitionPending) {
        AppState pending = _pendingState;
        _transitionPending = false;
        transitionTo(pending);
    }

    if (previous != next) {
        scheduleDestroy(previous);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
void App::requestTransition(AppState next) {
    _pendingState = next;
    _transitionPending = true;
}

// ─────────────────────────────────────────────────────────────────────────────
void App::processPendingTransition() {
    if (!_transitionPending) return;
    _transitionPending = false;
    transitionTo(_pendingState);
}

// ─────────────────────────────────────────────────────────────────────────────
void App::processPendingDestroy() {
    if (_destroyDelayLoops > 0) {
        _destroyDelayLoops--;
        return;
    }

    if (_pendingDestroys.empty()) return;

    for (AppState state : _pendingDestroys) {
        destroyState(state);
    }
    _pendingDestroys.clear();
}

// ─────────────────────────────────────────────────────────────────────────────
void App::scheduleDestroy(AppState state) {
    _pendingDestroys.push_back(state);
    _destroyDelayLoops = 1;
}

// ─────────────────────────────────────────────────────────────────────────────
void App::destroyState(AppState state) {
    switch (state) {
        case AppState::SPLASH:       SplashScreen::instance().destroy();    break;
        case AppState::WIFI_SETUP:   WiFiScreen::instance().destroy();      break;
        case AppState::WIFI_CONNECT: LoadingScreen::instance().destroy();   break;
        case AppState::HA_SETUP:
            HASetupScreen::instance().destroy();
            LoadingScreen::instance().destroy();
            break;
        case AppState::HA_CONNECT:   LoadingScreen::instance().destroy();   break;
        case AppState::DASHBOARD:    DashboardScreen::instance().destroy(); break;
        default: break;
    }
}

// ─────────────────────────────────────────────────────────────────────────────
void App::enterSplash() {
    SplashScreen::instance().create([this]() {
        requestTransition(AppState::CHECK_CONFIG);
    });
}

// ─────────────────────────────────────────────────────────────────────────────
void App::enterCheckConfig() {
    auto &store = StorageManager::instance();

    if (store.isFullyConfigured()) {
        // Reconnect with saved credentials
        _pendingSSID = store.getSSID();
        _pendingPass = store.getPass();
        requestTransition(AppState::WIFI_CONNECT);
    } else if (!store.hasWiFi()) {
        requestTransition(AppState::WIFI_SETUP);
    } else {
        // Has WiFi but no HA config — connect WiFi then set up HA
        _pendingSSID = store.getSSID();
        _pendingPass = store.getPass();
        requestTransition(AppState::WIFI_CONNECT);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
void App::enterWifiSetup() {
    std::function<void()> onCancel = nullptr;
    if (_wifiSetupCanCancelToDashboard) {
        onCancel = [this]() {
            _wifiSetupCanCancelToDashboard = false;
            requestTransition(AppState::DASHBOARD);
        };
    }

    WiFiScreen::instance().create([this](String ssid, String pass) {
        _wifiSetupCanCancelToDashboard = false;
        enterWifiConnect(ssid, pass);
    }, onCancel);
}

void App::enterWifiConnect(const String &ssid, const String &pass) {
    _pendingSSID = ssid;
    _pendingPass = pass;
    _state = AppState::WIFI_CONNECT;

    LoadingScreen::instance().create("Connecting to WiFi...");
    WiFiMgr::instance().connect(ssid, pass);

    // Poll in a tight loop (UI keeps updating via lv_task_handler)
    uint32_t start = millis();
    bool connected = false;
    while (millis() - start < WIFI_CONNECT_TIMEOUT_MS) {
        lv_task_handler();
        if (WiFiMgr::instance().isConnected()) { connected = true; break; }
        delay(200);
    }

    if (connected) {
        StorageManager::instance().setWiFi(ssid, pass);
        LoadingScreen::instance().setStatus(
            ("Connected: " + WiFiMgr::instance().localIP()).c_str());

        LoadingScreen::instance().showResult(true, "WiFi connected!", 1200, [this]() {
            if (!StorageManager::instance().hasHA()) {
                requestTransition(AppState::HA_SETUP);
            } else {
                requestTransition(AppState::HA_CONNECT);
            }
        });
    } else {
        WiFiMgr::instance().disconnect();
        LoadingScreen::instance().showResult(false, "Connection failed.", 2000, [this]() {
            requestTransition(AppState::WIFI_SETUP);
        });
    }
}

// ─────────────────────────────────────────────────────────────────────────────
void App::enterHaSetup() {
    String ip = WiFiMgr::instance().localIP();
    HASetupScreen::instance().create(ip, [this](String url, String token) {
        StorageManager::instance().setHA(url, token);
        requestTransition(AppState::HA_CONNECT);
    });
}

// ─────────────────────────────────────────────────────────────────────────────
void App::enterHaConnect() {
    _state = AppState::HA_CONNECT;
    LoadingScreen::instance().create("Connecting to Home Assistant...");

    auto &ha    = HAClient::instance();
    auto &store = StorageManager::instance();
    ha.setServer(store.getHAUrl(), store.getHAToken());

    LoadingScreen::instance().setStatus("Testing connection...");

    bool ok = ha.testConnection();
    if (ok) {
        LoadingScreen::instance().setStatus("Loading entities...");
        startHaEntityFetch();
        return;
    }

    if (!ok) {
        LoadingScreen::instance().showResult(
            false,
            ("HA error: " + ha.lastError()).c_str(),
            3000,
            [this]() { requestTransition(AppState::HA_SETUP); }
        );
    }
}

// ─────────────────────────────────────────────────────────────────────────────
void App::startHaEntityFetch() {
    if (_haFetchInProgress) return;

    _haFetchInProgress = true;
    _haFetchDone = false;
    _haFetchOk = false;
    _haFetchStartMs = millis();

    Serial.println("[HA] async entity fetch start");

    xTaskCreatePinnedToCore(
        haEntityFetchTask,
        "ha_fetch",
        8192,
        this,
        1,
        &_haFetchTask,
        APP_CPU_NUM
    );
}

// ─────────────────────────────────────────────────────────────────────────────
void App::haEntityFetchTask(void *param) {
    App *self = static_cast<App*>(param);
    bool ok = HAClient::instance().fetchEntities();
    self->_haFetchOk = ok;
    self->_haFetchDone = true;
    Serial.printf("[HA] async entity fetch done ok=%d count=%u\n",
                  ok ? 1 : 0,
                  static_cast<unsigned>(HAClient::instance().entities().size()));
    vTaskDelete(nullptr);
}

// ─────────────────────────────────────────────────────────────────────────────
void App::enterDashboard() {
    _lastEntityPollMs = millis();
    _lastTouchActivityMs = millis();
    disableIdleMode();
    DashboardScreen::instance().create([this]() {
        requestTransition(AppState::SETTINGS);
    }, [this]() {
        _wifiSetupCanCancelToDashboard = true;
        requestTransition(AppState::WIFI_SETUP);
    });
    DashboardScreen::instance().populate(HAClient::instance().entities());
    DashboardScreen::instance().updateStatusBar(true, WiFiMgr::instance().rssi());
}

// ─────────────────────────────────────────────────────────────────────────────
void App::enterSettings() {
    _settingsOpenedMs = millis();

    auto &storage = StorageManager::instance();
    uint32_t refreshMs = storage.getRefreshIntervalMs();
    uint8_t filterMode = storage.getEntityFilterMode();
    uint8_t rotation = storage.getDisplayRotation();
    uint8_t theme = storage.getThemePreset();

    static const uint32_t refreshOptionsMs[] = {5000, 10000, 15000, 30000, 60000};
    uint8_t refreshIndex = 1;
    for (uint8_t i = 0; i < 5; i++) {
        if (refreshMs == refreshOptionsMs[i]) {
            refreshIndex = i;
            break;
        }
    }

    struct SettingsUiCtx {
        App *app;
        lv_obj_t *overlay;
        lv_obj_t *ddRotation;
        lv_obj_t *ddTheme;
        lv_obj_t *ddRefresh;
        lv_obj_t *ddFilter;
    };

    lv_obj_t *overlay = lv_obj_create(lv_scr_act());
    lv_obj_set_size(overlay, SCREEN_W, SCREEN_H);
    lv_obj_set_style_bg_color(overlay, CLR_BG, 0);
    lv_obj_set_style_bg_opa(overlay, 220, 0);
    lv_obj_center(overlay);

    lv_obj_t *card = lv_obj_create(overlay);
    lv_obj_add_style(card, &Theme::style_card, 0);
    lv_obj_set_size(card, SCREEN_W - 12, SCREEN_H - 12);
    lv_obj_center(card);

    lv_obj_t *title = lv_label_create(card);
    lv_obj_add_style(title, &Theme::style_label_body, 0);
    lv_label_set_text(title, LV_SYMBOL_SETTINGS "  Settings");
    lv_obj_align(title, LV_ALIGN_TOP_LEFT, 0, 0);

    lv_obj_t *hint = lv_label_create(card);
    lv_obj_add_style(hint, &Theme::style_label_dim, 0);
    lv_label_set_text(hint, "Grouped editor");
    lv_obj_align_to(hint, title, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 2);

    lv_obj_t *content = lv_obj_create(card);
    lv_obj_set_size(content, SCREEN_W - 32, SCREEN_H - 96);
    lv_obj_align(content, LV_ALIGN_TOP_MID, 0, 36);
    lv_obj_set_style_bg_opa(content, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(content, 0, 0);
    lv_obj_set_style_pad_all(content, 0, 0);
    lv_obj_set_style_pad_gap(content, 6, 0);
    lv_obj_set_scroll_dir(content, LV_DIR_VER);
    lv_obj_set_flex_flow(content, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(content, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);

    auto makeSection = [&](const char *name) {
        lv_obj_t *section = lv_obj_create(content);
        lv_obj_set_width(section, lv_pct(100));
        lv_obj_set_height(section, LV_SIZE_CONTENT);
        lv_obj_set_style_bg_color(section, CLR_SURFACE2, 0);
        lv_obj_set_style_border_color(section, CLR_BORDER, 0);
        lv_obj_set_style_border_width(section, 1, 0);
        lv_obj_set_style_radius(section, 8, 0);
        lv_obj_set_style_pad_all(section, 6, 0);
        lv_obj_set_style_pad_gap(section, 4, 0);
        lv_obj_set_flex_flow(section, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_flex_align(section, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);

        lv_obj_t *label = lv_label_create(section);
        lv_obj_add_style(label, &Theme::style_label_dim, 0);
        lv_label_set_text(label, name);
        return section;
    };

    lv_obj_t *displaySec = makeSection("Display");
    lv_obj_t *rotLbl = lv_label_create(displaySec);
    lv_obj_add_style(rotLbl, &Theme::style_label_dim, 0);
    lv_label_set_text(rotLbl, "Rotation");
    lv_obj_t *ddRotation = lv_dropdown_create(displaySec);
    lv_dropdown_set_options(ddRotation, "Landscape 0\nPortrait 0\nLandscape 1\nPortrait 1");
    lv_dropdown_set_selected(ddRotation, rotation % 4);
    lv_obj_set_width(ddRotation, 180);

    lv_obj_t *themeLbl = lv_label_create(displaySec);
    lv_obj_add_style(themeLbl, &Theme::style_label_dim, 0);
    lv_label_set_text(themeLbl, "Theme");
    lv_obj_t *ddTheme = lv_dropdown_create(displaySec);
    lv_dropdown_set_options(ddTheme, "Dark\nDim Dark");
    lv_dropdown_set_selected(ddTheme, theme <= 1 ? theme : 0);
    lv_obj_set_width(ddTheme, 180);

    lv_obj_t *dashSec = makeSection("Dashboard");
    lv_obj_t *refreshLbl = lv_label_create(dashSec);
    lv_obj_add_style(refreshLbl, &Theme::style_label_dim, 0);
    lv_label_set_text(refreshLbl, "Refresh Interval");
    lv_obj_t *ddRefresh = lv_dropdown_create(dashSec);
    lv_dropdown_set_options(ddRefresh, "5 sec\n10 sec\n15 sec\n30 sec\n60 sec");
    lv_dropdown_set_selected(ddRefresh, refreshIndex);
    lv_obj_set_width(ddRefresh, 180);

    lv_obj_t *filterLbl = lv_label_create(dashSec);
    lv_obj_add_style(filterLbl, &Theme::style_label_dim, 0);
    lv_label_set_text(filterLbl, "Entity Filter");
    lv_obj_t *ddFilter = lv_dropdown_create(dashSec);
    lv_dropdown_set_options(ddFilter, "Lights + Outlets\nLights only");
    lv_dropdown_set_selected(ddFilter, filterMode == 1 ? 1 : 0);
    lv_obj_set_width(ddFilter, 180);

    lv_obj_t *connSec = makeSection("Connections");
    lv_obj_t *connRow = lv_obj_create(connSec);
    lv_obj_set_width(connRow, lv_pct(100));
    lv_obj_set_height(connRow, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(connRow, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(connRow, 0, 0);
    lv_obj_set_style_pad_all(connRow, 0, 0);
    lv_obj_set_style_pad_gap(connRow, 8, 0);
    lv_obj_set_flex_flow(connRow, LV_FLEX_FLOW_ROW_WRAP);

    lv_obj_t *wifiBtn = lv_btn_create(connRow);
    Theme::applyBtnGhost(wifiBtn);
    lv_obj_set_size(wifiBtn, 118, 32);
    lv_obj_t *wifiLbl = lv_label_create(wifiBtn);
    lv_label_set_text(wifiLbl, "WiFi AP");
    lv_obj_center(wifiLbl);

    lv_obj_t *haBtn = lv_btn_create(connRow);
    Theme::applyBtnGhost(haBtn);
    lv_obj_set_size(haBtn, 118, 32);
    lv_obj_t *haLbl = lv_label_create(haBtn);
    lv_label_set_text(haLbl, "HA Setup");
    lv_obj_center(haLbl);

    lv_obj_t *sysSec = makeSection("System");
    lv_obj_t *resetBtn = lv_btn_create(sysSec);
    Theme::applyBtnPrimary(resetBtn);
    lv_obj_set_size(resetBtn, 140, 32);
    lv_obj_t *resetLbl = lv_label_create(resetBtn);
    lv_label_set_text(resetLbl, "Factory Reset");
    lv_obj_center(resetLbl);

    lv_obj_t *closeBtn = lv_btn_create(card);
    Theme::applyBtnGhost(closeBtn);
    lv_obj_set_size(closeBtn, 96, 32);
    lv_obj_align(closeBtn, LV_ALIGN_BOTTOM_LEFT, 0, 0);
    lv_obj_t *closeLbl = lv_label_create(closeBtn);
    lv_label_set_text(closeLbl, "Close");
    lv_obj_center(closeLbl);

    lv_obj_t *saveBtn = lv_btn_create(card);
    Theme::applyBtnPrimary(saveBtn);
    lv_obj_set_size(saveBtn, 110, 32);
    lv_obj_align(saveBtn, LV_ALIGN_BOTTOM_RIGHT, 0, 0);
    lv_obj_t *saveLbl = lv_label_create(saveBtn);
    lv_label_set_text(saveLbl, "Save");
    lv_obj_center(saveLbl);

    auto *ctx = new SettingsUiCtx{this, overlay, ddRotation, ddTheme, ddRefresh, ddFilter};

    lv_obj_add_event_cb(saveBtn, [](lv_event_t *e) {
        SettingsUiCtx *ctx = static_cast<SettingsUiCtx*>(lv_event_get_user_data(e));
        if (!ctx || millis() - ctx->app->_settingsOpenedMs < 300) return;

        auto &storage = StorageManager::instance();
        uint8_t newRotation = static_cast<uint8_t>(lv_dropdown_get_selected(ctx->ddRotation));
        uint8_t newTheme = static_cast<uint8_t>(lv_dropdown_get_selected(ctx->ddTheme));
        uint8_t refreshIdx = static_cast<uint8_t>(lv_dropdown_get_selected(ctx->ddRefresh));
        uint8_t filterIdx = static_cast<uint8_t>(lv_dropdown_get_selected(ctx->ddFilter));
        if (refreshIdx > 4) refreshIdx = 1;

        static const uint32_t refreshValues[] = {5000, 10000, 15000, 30000, 60000};
        uint32_t newRefreshMs = refreshValues[refreshIdx];
        uint8_t newFilter = (filterIdx == 1) ? 1 : 0;

        uint8_t oldRotation = storage.getDisplayRotation();
        uint8_t oldTheme = storage.getThemePreset();

        storage.setDisplayRotation(newRotation);
        storage.setThemePreset(newTheme);
        storage.setRefreshIntervalMs(newRefreshMs);
        storage.setEntityFilterMode(newFilter);

        ctx->app->_entityPollIntervalMs = newRefreshMs;
        ctx->app->_lastEntityPollMs = millis();
        HAClient::instance().setEntityFilterMode(newFilter);
        if (HAClient::instance().fetchEntities()) {
            DashboardScreen::instance().populate(HAClient::instance().entities());
            DashboardScreen::instance().updateStatusBar(true, WiFiMgr::instance().rssi());
        }

        bool needsRestart = (newRotation != oldRotation) || (newTheme != oldTheme);
        lv_obj_del(ctx->overlay);
        delete ctx;
        if (needsRestart) {
            esp_restart();
        }
    }, LV_EVENT_CLICKED, ctx);

    lv_obj_add_event_cb(closeBtn, [](lv_event_t *e) {
        SettingsUiCtx *ctx = static_cast<SettingsUiCtx*>(lv_event_get_user_data(e));
        if (!ctx || millis() - ctx->app->_settingsOpenedMs < 300) return;
        lv_obj_del(ctx->overlay);
        delete ctx;
    }, LV_EVENT_CLICKED, ctx);

    lv_obj_add_event_cb(wifiBtn, [](lv_event_t *e) {
        SettingsUiCtx *ctx = static_cast<SettingsUiCtx*>(lv_event_get_user_data(e));
        if (!ctx || millis() - ctx->app->_settingsOpenedMs < 300) return;
        lv_obj_del(ctx->overlay);
        ctx->app->_wifiSetupCanCancelToDashboard = true;
        ctx->app->requestTransition(AppState::WIFI_SETUP);
        delete ctx;
    }, LV_EVENT_CLICKED, ctx);

    lv_obj_add_event_cb(haBtn, [](lv_event_t *e) {
        SettingsUiCtx *ctx = static_cast<SettingsUiCtx*>(lv_event_get_user_data(e));
        if (!ctx || millis() - ctx->app->_settingsOpenedMs < 300) return;
        lv_obj_del(ctx->overlay);
        ctx->app->requestTransition(AppState::HA_SETUP);
        delete ctx;
    }, LV_EVENT_CLICKED, ctx);

    lv_obj_add_event_cb(resetBtn, [](lv_event_t *e) {
        SettingsUiCtx *ctx = static_cast<SettingsUiCtx*>(lv_event_get_user_data(e));
        if (!ctx || millis() - ctx->app->_settingsOpenedMs < 300) return;
        StorageManager::instance().clear();
        WiFiMgr::instance().disconnect();
        lv_obj_del(ctx->overlay);
        delete ctx;
        esp_restart();
    }, LV_EVENT_CLICKED, ctx);

    _state = AppState::DASHBOARD;
}
