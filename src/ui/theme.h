#pragma once
#include <lvgl.h>

// ─────────────────────────────────────────────────────────────────────────────
//  Colour palette (HA dark theme)
// ─────────────────────────────────────────────────────────────────────────────
#define CLR_BG          lv_color_hex(0x0D1117)   // deep page background
#define CLR_SURFACE     lv_color_hex(0x161B22)   // card / panel surface
#define CLR_SURFACE2    lv_color_hex(0x21262D)   // elevated surface
#define CLR_BORDER      lv_color_hex(0x30363D)   // subtle border
#define CLR_ACCENT      lv_color_hex(0x41BDF5)   // HA blue
#define CLR_ACCENT_DIM  lv_color_hex(0x1A5A78)   // pressed / dimmed accent
#define CLR_ON          lv_color_hex(0xFFBF00)   // entity ON  (warm amber)
#define CLR_OFF         lv_color_hex(0x30363D)   // entity OFF (muted)
#define CLR_OK          lv_color_hex(0x3FB950)   // success green
#define CLR_WARN        lv_color_hex(0xD29922)   // warning amber
#define CLR_ERR         lv_color_hex(0xF85149)   // error red
#define CLR_TEXT        lv_color_hex(0xE6EDF3)   // primary text
#define CLR_TEXT_DIM    lv_color_hex(0x8B949E)   // secondary text
#define CLR_TEXT_MUTED  lv_color_hex(0x484F58)   // placeholder / disabled

// ─────────────────────────────────────────────────────────────────────────────
//  Font shortcuts
// ─────────────────────────────────────────────────────────────────────────────
#define FONT_SM         &lv_font_montserrat_16
#define FONT_MD         &lv_font_montserrat_18
#define FONT_LG         &lv_font_montserrat_18
#define FONT_XL         &lv_font_montserrat_18
#define FONT_TITLE      &lv_font_montserrat_18

// ─────────────────────────────────────────────────────────────────────────────
//  Shared LVGL styles (initialized once in App::initLVGL)
// ─────────────────────────────────────────────────────────────────────────────
namespace Theme {

    inline lv_style_t style_screen;
    inline lv_style_t style_card;
    inline lv_style_t style_btn_primary;
    inline lv_style_t style_btn_primary_pr;
    inline lv_style_t style_btn_ghost;
    inline lv_style_t style_label_title;
    inline lv_style_t style_label_body;
    inline lv_style_t style_label_dim;
    inline lv_style_t style_tile_on;
    inline lv_style_t style_tile_off;
    inline lv_style_t style_status_bar;

    inline void init() {
        // ── Screen background ────────────────────────────────────────────
        lv_style_init(&style_screen);
        lv_style_set_bg_color(&style_screen, CLR_BG);
        lv_style_set_bg_opa(&style_screen, LV_OPA_COVER);
        lv_style_set_border_width(&style_screen, 0);
        lv_style_set_pad_all(&style_screen, 0);

        // ── Card / panel ─────────────────────────────────────────────────
        lv_style_init(&style_card);
        lv_style_set_bg_color(&style_card, CLR_SURFACE);
        lv_style_set_bg_opa(&style_card, LV_OPA_COVER);
        lv_style_set_border_color(&style_card, CLR_BORDER);
        lv_style_set_border_width(&style_card, 1);
        lv_style_set_radius(&style_card, 10);
        lv_style_set_pad_all(&style_card, 8);

        // ── Primary button ───────────────────────────────────────────────
        lv_style_init(&style_btn_primary);
        lv_style_set_bg_color(&style_btn_primary, CLR_ACCENT);
        lv_style_set_bg_opa(&style_btn_primary, LV_OPA_COVER);
        lv_style_set_border_width(&style_btn_primary, 0);
        lv_style_set_radius(&style_btn_primary, 8);
        lv_style_set_pad_ver(&style_btn_primary, 10);
        lv_style_set_pad_hor(&style_btn_primary, 16);
        lv_style_set_text_color(&style_btn_primary, CLR_BG);
        lv_style_set_text_font(&style_btn_primary, FONT_MD);

        lv_style_init(&style_btn_primary_pr);
        lv_style_set_bg_color(&style_btn_primary_pr, CLR_ACCENT_DIM);

        // ── Ghost button ─────────────────────────────────────────────────
        lv_style_init(&style_btn_ghost);
        lv_style_set_bg_opa(&style_btn_ghost, LV_OPA_TRANSP);
        lv_style_set_border_color(&style_btn_ghost, CLR_BORDER);
        lv_style_set_border_width(&style_btn_ghost, 1);
        lv_style_set_radius(&style_btn_ghost, 8);
        lv_style_set_pad_ver(&style_btn_ghost, 10);
        lv_style_set_pad_hor(&style_btn_ghost, 16);
        lv_style_set_text_color(&style_btn_ghost, CLR_TEXT);
        lv_style_set_text_font(&style_btn_ghost, FONT_MD);

        // ── Labels ───────────────────────────────────────────────────────
        lv_style_init(&style_label_title);
        lv_style_set_text_font(&style_label_title, FONT_XL);
        lv_style_set_text_color(&style_label_title, CLR_TEXT);

        lv_style_init(&style_label_body);
        lv_style_set_text_font(&style_label_body, FONT_MD);
        lv_style_set_text_color(&style_label_body, CLR_TEXT);

        lv_style_init(&style_label_dim);
        lv_style_set_text_font(&style_label_dim, FONT_SM);
        lv_style_set_text_color(&style_label_dim, CLR_TEXT_DIM);

        // ── Entity tile — ON ─────────────────────────────────────────────
        lv_style_init(&style_tile_on);
        lv_style_set_bg_color(&style_tile_on, CLR_SURFACE2);
        lv_style_set_bg_opa(&style_tile_on, LV_OPA_COVER);
        lv_style_set_border_color(&style_tile_on, CLR_ON);
        lv_style_set_border_width(&style_tile_on, 2);
        lv_style_set_radius(&style_tile_on, 10);

        // ── Entity tile — OFF ────────────────────────────────────────────
        lv_style_init(&style_tile_off);
        lv_style_set_bg_color(&style_tile_off, CLR_SURFACE);
        lv_style_set_bg_opa(&style_tile_off, LV_OPA_COVER);
        lv_style_set_border_color(&style_tile_off, CLR_BORDER);
        lv_style_set_border_width(&style_tile_off, 1);
        lv_style_set_radius(&style_tile_off, 10);

        // ── Status bar ───────────────────────────────────────────────────
        lv_style_init(&style_status_bar);
        lv_style_set_bg_color(&style_status_bar, CLR_SURFACE);
        lv_style_set_bg_opa(&style_status_bar, LV_OPA_COVER);
        lv_style_set_border_width(&style_status_bar, 0);
        lv_style_set_radius(&style_status_bar, 0);
        lv_style_set_pad_hor(&style_status_bar, 8);
        lv_style_set_pad_ver(&style_status_bar, 4);
    }

    // ── Helpers ──────────────────────────────────────────────────────────────

    /** Apply primary-button style pair to an lv_btn object. */
    inline void applyBtnPrimary(lv_obj_t *btn) {
        lv_obj_add_style(btn, &style_btn_primary, 0);
        lv_obj_add_style(btn, &style_btn_primary_pr, LV_STATE_PRESSED);
    }

    /** Apply ghost-button style to an lv_btn object. */
    inline void applyBtnGhost(lv_obj_t *btn) {
        lv_obj_add_style(btn, &style_btn_ghost, 0);
    }

    /** Map entity domain → LVGL symbol string. */
    inline const char* entityIcon(const char *domain) {
        if (strcmp(domain, "light")         == 0) return LV_SYMBOL_IMAGE;
        if (strcmp(domain, "switch")        == 0) return LV_SYMBOL_POWER;
        if (strcmp(domain, "input_boolean") == 0) return LV_SYMBOL_POWER;
        if (strcmp(domain, "sensor")        == 0) return LV_SYMBOL_SETTINGS;
        if (strcmp(domain, "climate")       == 0) return LV_SYMBOL_LOOP;
        if (strcmp(domain, "cover")         == 0) return LV_SYMBOL_UPLOAD;
        if (strcmp(domain, "media_player")  == 0) return LV_SYMBOL_AUDIO;
        if (strcmp(domain, "automation")    == 0) return LV_SYMBOL_PLAY;
        if (strcmp(domain, "script")        == 0) return LV_SYMBOL_PLAY;
        if (strcmp(domain, "scene")         == 0) return LV_SYMBOL_HOME;
        if (strcmp(domain, "lock")          == 0) return LV_SYMBOL_WARNING;
        return LV_SYMBOL_SETTINGS;
    }

} // namespace Theme
