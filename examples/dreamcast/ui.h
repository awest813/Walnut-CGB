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

void dc_ui_clear(uint16_t fb[DC_SCREEN_HEIGHT][DC_SCREEN_WIDTH], uint16_t color);
void dc_ui_fill_rect(uint16_t fb[DC_SCREEN_HEIGHT][DC_SCREEN_WIDTH],
		     int x, int y, int w, int h, uint16_t color);
void dc_ui_draw_text(uint16_t fb[DC_SCREEN_HEIGHT][DC_SCREEN_WIDTH],
		     int x, int y, const char *text,
		     uint16_t fg, uint16_t bg);
void dc_ui_draw_text_clipped(uint16_t fb[DC_SCREEN_HEIGHT][DC_SCREEN_WIDTH],
			     int x, int y, int max_width, const char *text,
			     uint16_t fg, uint16_t bg);

#endif /* DC_UI_H */
