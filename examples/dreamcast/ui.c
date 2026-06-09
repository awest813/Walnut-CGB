/*
 * Walnut-CGB Dreamcast frontend — simple 640x480 UI drawing.
 * Copyright (c) 2025 Mr. Paul (https://github.com/Mr-PauI)
 * Licensed under the MIT License.
 */

#include <string.h>

#include "font8x8.h"
#include "ui.h"

void dc_ui_clear(uint16_t fb[DC_SCREEN_HEIGHT][DC_SCREEN_WIDTH], uint16_t color)
{
	unsigned int y;

	for (y = 0; y < DC_SCREEN_HEIGHT; y++)
		memset(fb[y], color, DC_SCREEN_WIDTH * sizeof(uint16_t));
}

void dc_ui_fill_rect(uint16_t fb[DC_SCREEN_HEIGHT][DC_SCREEN_WIDTH],
		     int x, int y, int w, int h, uint16_t color)
{
	int row;
	int col;

	for (row = y; row < y + h; row++) {
		if (row < 0 || row >= DC_SCREEN_HEIGHT)
			continue;

		for (col = x; col < x + w; col++) {
			if (col < 0 || col >= DC_SCREEN_WIDTH)
				continue;

			fb[row][col] = color;
		}
	}
}

static void dc_ui_draw_glyph(uint16_t fb[DC_SCREEN_HEIGHT][DC_SCREEN_WIDTH],
			     int x, int y, unsigned char c,
			     uint16_t fg, uint16_t bg)
{
	const uint8_t *glyph = dc_font8x8_basic[c];
	int row;
	int col;

	for (row = 0; row < 8; row++) {
		uint8_t bits = glyph[row];

		for (col = 0; col < 8; col++) {
			const int px = x + col;
			const int py = y + row;

			if (px < 0 || px >= DC_SCREEN_WIDTH ||
			    py < 0 || py >= DC_SCREEN_HEIGHT)
				continue;

			fb[py][px] = (bits & (1U << (7 - col))) ? fg : bg;
		}
	}
}

void dc_ui_draw_text(uint16_t fb[DC_SCREEN_HEIGHT][DC_SCREEN_WIDTH],
		     int x, int y, const char *text,
		     uint16_t fg, uint16_t bg)
{
	int cursor_x = x;

	while (text && *text) {
		dc_ui_draw_glyph(fb, cursor_x, y, (unsigned char)*text, fg, bg);
		cursor_x += 8;
		text++;
	}
}

void dc_ui_draw_text_clipped(uint16_t fb[DC_SCREEN_HEIGHT][DC_SCREEN_WIDTH],
			     int x, int y, int max_width, const char *text,
			     uint16_t fg, uint16_t bg)
{
	int cursor_x = x;
	const int max_chars = max_width / 8;

	if (max_chars <= 0 || !text)
		return;

	for (int i = 0; text[i] != '\0' && i < max_chars; i++) {
		dc_ui_draw_glyph(fb, cursor_x, y, (unsigned char)text[i], fg, bg);
		cursor_x += 8;
	}
}
