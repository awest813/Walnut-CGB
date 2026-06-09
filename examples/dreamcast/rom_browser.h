/*
 * Walnut-CGB Dreamcast frontend — ROM and save file I/O.
 * Copyright (c) 2025 Mr. Paul (https://github.com/Mr-PauI)
 * Licensed under the MIT License.
 */

#ifndef DC_ROM_BROWSER_H
#define DC_ROM_BROWSER_H

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#include "cover.h"
#include "dc_priv.h"

#define DC_BROWSER_MAX_ENTRIES 128
#define DC_BROWSER_LIST_LINES    9
#define DC_BROWSER_GRID_COLS     5
#define DC_BROWSER_GRID_ROWS     3

enum dc_browser_view
{
	DC_BROWSER_VIEW_LIST = 0,
	DC_BROWSER_VIEW_GRID
};

struct dc_browser_entry
{
	char path[256];
	char name[48];
	char title[17];
	bool is_cgb;
	bool has_save;
	bool cover_ready;
	bool cover_from_file;
	uint16_t cover[DC_COVER_HEIGHT][DC_COVER_WIDTH];
};

struct dc_browser
{
	char root_path[64];
	char covers_path[128];
	struct dc_browser_entry entries[DC_BROWSER_MAX_ENTRIES];
	int count;
	int selected;
	int scroll;
	int root_index;
	enum dc_browser_view view;
};

struct dc_browser_input
{
	bool up;
	bool down;
	bool select;
	bool next_device;
	bool refresh;
	bool page_up;
	bool page_down;
	bool toggle_view;
	bool exit;
};

int dc_rom_load(struct dc_priv *priv, const char *rom_path);
void dc_rom_unload(struct dc_priv *priv);
int dc_save_path_from_rom(const char *rom_path, char *save_path, size_t save_path_len);
int dc_cart_ram_read_file(const char *save_path, uint8_t **dest, size_t len);
int dc_cart_ram_reload_file(const char *save_path, uint8_t *dest, size_t len);
int dc_cart_ram_write_file(const char *save_path, const uint8_t *data, size_t len);

void dc_browser_init(struct dc_browser *browser);
int dc_browser_scan(struct dc_browser *browser);
const char *dc_browser_device_label(const struct dc_browser *browser);
bool dc_browser_run(struct dc_browser *browser, char *selected_path, size_t selected_len);
void dc_browser_show_loading(const char *rom_name);

#endif /* DC_ROM_BROWSER_H */
