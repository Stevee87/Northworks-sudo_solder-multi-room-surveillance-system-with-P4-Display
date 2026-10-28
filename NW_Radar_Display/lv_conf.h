#pragma once
#ifndef LV_CONF_H
#define LV_CONF_H
// LVGL 8.4, matching Steve's existing UI. No canvas or screen transparency.
#define LV_COLOR_DEPTH 16
#define LV_COLOR_16_SWAP 0
#define LV_COLOR_SCREEN_TRANSP 0
#define LV_MEM_CUSTOM 0
#define LV_MEM_SIZE (96U * 1024U)
#define LV_DISP_DEF_REFR_PERIOD 30
#define LV_TICK_CUSTOM 0
#define LV_FONT_MONTSERRAT_14 1
#define LV_FONT_MONTSERRAT_18 1
#define LV_FONT_MONTSERRAT_24 1
#define LV_FONT_DEFAULT &lv_font_montserrat_14
#define LV_USE_LOG 0
#define LV_USE_PERF_MONITOR 0
#define LV_USE_MEM_MONITOR 0
#define LV_USE_THEME_DEFAULT 0
#endif
