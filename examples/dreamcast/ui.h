/*
 * Walnut-CGB Dreamcast frontend — simple 640x480 UI drawing.
 * Copyright (c) 2025 Mr. Paul (https://github.com/Mr-PauI)
 * Licensed under the MIT License.
 */

#ifndef DC_UI_H
#define DC_UI_H

#include <stdint.h>

#define DC_SCREEN_WIDTH  640
#define DC_SCREEN_HEIGHT 480

/* Shared UI palette (RGB555). */
#define DC_UI_COLOR_BG         0x0000
#define DC_UI_COLOR_FG         0x7FFF
#define DC_UI_COLOR_DIM        0x39CE
#define DC_UI_COLOR_SELECT     0x001F
#define DC_UI_COLOR_HEADER     0x03FF
#define DC_UI_COLOR_TITLE      0x7FE0
#define DC_UI_COLOR_ACCENT     0xF800
#define DC_UI_COLOR_TRACK      0x2108
#define DC_UI_COLOR_THUMB      0x5294
#define DC_UI_COLOR_SAVE       0x47E0
#define DC_UI_COLOR_PANEL      0x1084
#define DC_UI_COLOR_TOAST_BG   DC_UI_COLOR_PANEL
#define DC_UI_COLOR_TOAST_FG   0xFFFF
#define DC_UI_COLOR_WARN       0xFC00

void dc_ui_clear(uint16_t fb[DC_SCREEN_HEIGHT][DC_SCREEN_WIDTH], uint16_t color);
void dc_ui_fill_rect(uint16_t fb[DC_SCREEN_HEIGHT][DC_SCREEN_WIDTH],
		     int x, int y, int w, int h, uint16_t color);
void dc_ui_draw_text(uint16_t fb[DC_SCREEN_HEIGHT][DC_SCREEN_WIDTH],
		     int x, int y, const char *text,
		     uint16_t fg, uint16_t bg);
void dc_ui_draw_text_clipped(uint16_t fb[DC_SCREEN_HEIGHT][DC_SCREEN_WIDTH],
			     int x, int y, int max_width, const char *text,
			     uint16_t fg, uint16_t bg);
void dc_ui_draw_text_ellipsis(uint16_t fb[DC_SCREEN_HEIGHT][DC_SCREEN_WIDTH],
			      int x, int y, int max_width, const char *text,
			      uint16_t fg, uint16_t bg);
void dc_ui_draw_toast_bar(uint16_t fb[DC_SCREEN_HEIGHT][DC_SCREEN_WIDTH],
			  const char *message);
void dc_ui_draw_loading(uint16_t fb[DC_SCREEN_HEIGHT][DC_SCREEN_WIDTH],
			const char *title, const char *subtitle);

#endif /* DC_UI_H */
