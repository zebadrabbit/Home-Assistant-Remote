/**
 * lv_conf.h — LVGL 8.3 configuration for ESP32 CYD (320×240 landscape)
 * Place in project's include/ directory with -DLV_CONF_INCLUDE_SIMPLE build flag.
 */

#if 1   /* Set to "1" to enable */

#ifndef LV_CONF_H
#define LV_CONF_H

#include <stdint.h>

/*====================
   COLOR
 *====================*/
#define LV_COLOR_DEPTH     16
#define LV_COLOR_16_SWAP   0     /* TFT_eSPI pushColors(..., true) already swaps */
#define LV_COLOR_SCREEN_TRANSP 0
#define LV_COLOR_MIX_ROUND_OFS 0

/*========================
   MEMORY
 *========================*/
#define LV_MEM_CUSTOM 0
#if LV_MEM_CUSTOM == 0
    #define LV_MEM_SIZE (48U * 1024U)   /* 48 KB LVGL heap */
    #define LV_MEM_ADR  0               /* 0 = allocate automatically */
    #define LV_MEM_POOL_INCLUDE <stdlib.h>
    #define LV_MEM_POOL_ALLOC   malloc
    #define LV_MEM_POOL_FREE    free
#endif
#define LV_MEM_BUF_MAX_NUM 16
#define LV_STDLIB_INCLUDE <stdlib.h>
#define LV_SPRINTF_CUSTOM 0

/*===================
   HAL
 *===================*/
#define LV_DISP_DEF_REFR_PERIOD  30   /* ms — ~33 fps */
#define LV_INDEV_DEF_READ_PERIOD 30   /* ms */
#define LV_TICK_CUSTOM           0
#define LV_DPI_DEF               130

/*=====================================
   DRAW
 *=====================================*/
#define LV_DRAW_COMPLEX 1
#if LV_DRAW_COMPLEX != 0
    #define LV_SHADOW_CACHE_SIZE    0
    #define LV_CIRCLE_CACHE_SIZE    4
#endif
#define LV_IMG_CACHE_DEF_SIZE       0
#define LV_GRADIENT_MAX_STOPS       2
#define LV_GRAD_CACHE_DEF_SIZE      0
#define LV_DITHER_GRADIENT          0
#define LV_DISP_ROT_MAX_BUF         (10*1024)

/*===================
   LOG
 *===================*/
#define LV_USE_LOG 1
#if LV_USE_LOG
    #define LV_LOG_LEVEL            LV_LOG_LEVEL_WARN
    #define LV_LOG_PRINTF           1
    #define LV_LOG_TRACE_MEM        0
    #define LV_LOG_TRACE_TIMER      0
    #define LV_LOG_TRACE_INDEV      0
    #define LV_LOG_TRACE_DISP_REFR  0
    #define LV_LOG_TRACE_EVENT      0
    #define LV_LOG_TRACE_OBJ_CREATE 0
    #define LV_LOG_TRACE_LAYOUT     0
    #define LV_LOG_TRACE_ANIM       0
#endif

/*=================
   ASSERTS
 *=================*/
#define LV_USE_ASSERT_NULL          1
#define LV_USE_ASSERT_MALLOC        1
#define LV_USE_ASSERT_STYLE         0
#define LV_USE_ASSERT_MEM_INTEGRITY 0
#define LV_USE_ASSERT_OBJ           0
#define LV_ASSERT_HANDLER_INCLUDE   <stdint.h>
#define LV_ASSERT_HANDLER           while(1);

/*==================
   FONTS
 *==================*/
#define LV_FONT_MONTSERRAT_8     0
#define LV_FONT_MONTSERRAT_10    0
#define LV_FONT_MONTSERRAT_12    0
#define LV_FONT_MONTSERRAT_14    0
#define LV_FONT_MONTSERRAT_16    1
#define LV_FONT_MONTSERRAT_18    1
#define LV_FONT_MONTSERRAT_20    0
#define LV_FONT_MONTSERRAT_22    0
#define LV_FONT_MONTSERRAT_24    0
#define LV_FONT_MONTSERRAT_26    0
#define LV_FONT_MONTSERRAT_28    0
#define LV_FONT_MONTSERRAT_30    0
#define LV_FONT_MONTSERRAT_32    0
#define LV_FONT_MONTSERRAT_34    0
#define LV_FONT_MONTSERRAT_36    0
#define LV_FONT_MONTSERRAT_38    0
#define LV_FONT_MONTSERRAT_40    0
#define LV_FONT_MONTSERRAT_42    0
#define LV_FONT_MONTSERRAT_44    0
#define LV_FONT_MONTSERRAT_46    0
#define LV_FONT_MONTSERRAT_48    0
#define LV_FONT_MONTSERRAT_12_SUBPX      0
#define LV_FONT_MONTSERRAT_28_COMPRESSED 0
#define LV_FONT_DEJAVU_16_PERSIAN_HEBREW 0
#define LV_FONT_SIMSUN_16_CJK            0
#define LV_FONT_UNSCII_8                 0
#define LV_FONT_UNSCII_16                0
#define LV_FONT_CUSTOM_DECLARE

#define LV_FONT_DEFAULT &lv_font_montserrat_18

#define LV_FONT_FMT_TXT_LARGE   0
#define LV_USE_FONT_COMPRESSED  0
#define LV_USE_FONT_SUBPX       0
#if LV_USE_FONT_SUBPX
    #define LV_FONT_SUBPX_BGR 0
#endif

/*=================
   TEXT
 *=================*/
#define LV_TXT_ENC LV_TXT_ENC_UTF8
#define LV_TXT_BREAK_CHARS           " ,.;:-_"
#define LV_TXT_LINE_BREAK_LONG_LEN   0
#define LV_TXT_LINE_BREAK_LONG_PRE_MIN_LEN  3
#define LV_TXT_LINE_BREAK_LONG_POST_MIN_LEN 3
#define LV_TXT_COLOR_CMD  "#"
#define LV_USE_BIDI       0
#define LV_USE_ARABIC_PERSIAN_CHARS 0

/*==================
   WIDGETS
 *==================*/
#define LV_USE_ARC          1
#define LV_USE_BTN          1
#define LV_USE_BTNMATRIX    1
#define LV_USE_CANVAS       0
#define LV_USE_CHECKBOX     1
#define LV_USE_DROPDOWN     1
#define LV_USE_IMG          1
#define LV_USE_LABEL        1
#define LV_USE_LINE         1
#define LV_USE_ROLLER       1
#define LV_USE_SLIDER       1
#define LV_USE_SWITCH       1
#define LV_USE_TEXTAREA     1
#define LV_USE_TABLE        0

/*==================
   EXTRA WIDGETS
 *==================*/
#define LV_USE_ANIMIMG    0
#define LV_USE_CALENDAR   0
#define LV_USE_CHART      0
#define LV_USE_COLORWHEEL 1
#define LV_USE_IMGBTN     0
#define LV_USE_KEYBOARD   1
#define LV_USE_LED        1
#define LV_USE_LIST       1
#define LV_USE_MENU       0
#define LV_USE_METER      0
#define LV_USE_MSGBOX     1
#define LV_USE_SPAN       0
#define LV_USE_SPINBOX    0
#define LV_USE_SPINNER    1
#define LV_USE_TABVIEW    0
#define LV_USE_TILEVIEW   0
#define LV_USE_WIN        0

/*==================
   THEME
 *==================*/
#define LV_USE_THEME_DEFAULT 1
#if LV_USE_THEME_DEFAULT
    #define LV_THEME_DEFAULT_DARK             1
    #define LV_THEME_DEFAULT_GROW             1
    #define LV_THEME_DEFAULT_TRANSITION_TIME  80
#endif
#define LV_USE_THEME_SIMPLE 0
#define LV_USE_THEME_MONO   0

/*==================
   LAYOUT
 *==================*/
#define LV_USE_FLEX 1
#define LV_USE_GRID 1

/*==================
   GPU
 *==================*/
#define LV_USE_GPU_STM32_DMA2D  0
#define LV_USE_GPU_SWM341_DMA2D 0
#define LV_USE_GPU_NXP_PXP      0
#define LV_USE_GPU_NXP_VG_LITE  0
#define LV_USE_GPU_SDL          0

/*==================
   MISC
 *==================*/
#define LV_USE_SNAPSHOT 0
#define LV_USE_MONKEY   0
#define LV_USE_GRIDNAV  0
#define LV_USE_FRAGMENT 0
#define LV_USE_IMGFONT  0
#define LV_USE_MSG      0
#define LV_USE_IME_PINYIN 0

#define LV_BUILD_EXAMPLES 0

#endif /* LV_CONF_H */
#endif /* Content enable */
