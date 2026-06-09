/*
 * Walnut-CGB Dreamcast frontend — ROM browser and file I/O.
 * Copyright (c) 2025 Mr. Paul (https://github.com/Mr-PauI)
 * Licensed under the MIT License.
 */

#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include <kos.h>
#include <dc/maple/controller.h>

#include "rom_browser.h"
#include "ui.h"
#include "video.h"

#define DC_BROWSER_LINE_HEIGHT 24
#define DC_BROWSER_LIST_TOP    72
#define DC_REPEAT_DELAY_FRAMES 18
#define DC_REPEAT_RATE_FRAMES  4
#define DC_ANALOG_THRESHOLD    64
#define DC_FRAME_MS            16
#define DC_COLOR_BG     0x0000
#define DC_COLOR_FG     0x7FFF
#define DC_COLOR_DIM    0x39CE
#define DC_COLOR_SELECT 0x001F
#define DC_COLOR_HEADER 0x03FF
#define DC_COLOR_TRACK  0x2108
#define DC_COLOR_THUMB  0x5294

static const char *dc_browser_roots[] = {
	"/cd/roms",
	"/cd",
	"/sd/roms",
	"/sd",
	"/ide/roms",
	"/ide",
	"/pc",
	NULL
};

static int dc_has_rom_extension(const char *name)
{
	size_t len;

	if (!name)
		return 0;

	len = strlen(name);
	if (len >= 3 && strcasecmp(name + len - 3, ".gb") == 0)
		return 1;
	if (len >= 4 && strcasecmp(name + len - 4, ".gbc") == 0)
		return 1;

	return 0;
}

static int dc_browser_entry_compare(const void *a, const void *b)
{
	const struct dc_browser_entry *ea = a;
	const struct dc_browser_entry *eb = b;

	return strcasecmp(ea->name, eb->name);
}

void dc_browser_init(struct dc_browser *browser)
{
	memset(browser, 0, sizeof(*browser));
	browser->root_index = 0;
	strncpy(browser->root_path, dc_browser_roots[0], sizeof(browser->root_path) - 1);
}

int dc_browser_scan(struct dc_browser *browser)
{
	DIR *dir;
	struct dirent *entry;
	int count = 0;

	browser->count = 0;
	browser->selected = 0;
	browser->scroll = 0;

	dir = opendir(browser->root_path);
	if (!dir)
		return -1;

	while ((entry = readdir(dir)) != NULL && count < DC_BROWSER_MAX_ENTRIES) {
		struct dc_browser_entry *slot = &browser->entries[count];
		const char *name = entry->d_name;

		if (name[0] == '.')
			continue;
		if (!dc_has_rom_extension(name))
			continue;

		snprintf(slot->path, sizeof(slot->path), "%s/%s",
			 browser->root_path, name);
		strncpy(slot->name, name, sizeof(slot->name) - 1);
		slot->name[sizeof(slot->name) - 1] = '\0';
		count++;
	}

	closedir(dir);
	browser->count = count;
	qsort(browser->entries, (size_t)browser->count, sizeof(browser->entries[0]),
	      dc_browser_entry_compare);
	return count;
}

static void dc_browser_draw(const struct dc_browser *browser,
			    uint16_t screen[DC_SCREEN_HEIGHT][DC_SCREEN_WIDTH])
{
	char line[80];
	int visible = DC_BROWSER_VISIBLE_LINES;
	int i;

	dc_ui_clear(screen, DC_COLOR_BG);
	dc_ui_fill_rect(screen, 0, 0, DC_SCREEN_WIDTH, 28, DC_COLOR_HEADER);
	snprintf(line, sizeof(line), "Walnut-DC  %s", browser->root_path);
	dc_ui_draw_text(screen, 12, 10, line, DC_COLOR_BG, DC_COLOR_HEADER);
	dc_ui_draw_text(screen, 12, 40,
			"Up/Dn:Move  L/R:Page  A:Load  B:Device  Start:Refresh  X:Exit",
			DC_COLOR_DIM, DC_COLOR_BG);

	if (browser->count == 0) {
		dc_ui_draw_text(screen, 12, 80, "No ROM files found.",
				DC_COLOR_FG, DC_COLOR_BG);
		dc_ui_draw_text(screen, 12, 104, "Place .gb/.gbc files here or press B.",
				DC_COLOR_DIM, DC_COLOR_BG);
		return;
	}

	for (i = 0; i < visible; i++) {
		const int index = browser->scroll + i;
		const int y = DC_BROWSER_LIST_TOP + i * DC_BROWSER_LINE_HEIGHT;
		uint16_t fg = DC_COLOR_FG;
		uint16_t bg = DC_COLOR_BG;

		if (index >= browser->count)
			break;

		if (index == browser->selected) {
			dc_ui_fill_rect(screen, 8, y - 2, DC_SCREEN_WIDTH - 16,
					DC_BROWSER_LINE_HEIGHT, DC_COLOR_SELECT);
			fg = DC_COLOR_FG;
			bg = DC_COLOR_SELECT;
		}

		snprintf(line, sizeof(line), "%c %s",
			 index == browser->selected ? '>' : ' ',
			 browser->entries[index].name);
		dc_ui_draw_text_clipped(screen, 16, y, DC_SCREEN_WIDTH - 32, line, fg, bg);
	}

	if (browser->count > DC_BROWSER_VISIBLE_LINES) {
		const int track_x = DC_SCREEN_WIDTH - 10;
		const int track_y = DC_BROWSER_LIST_TOP - 4;
		const int track_h = DC_BROWSER_VISIBLE_LINES * DC_BROWSER_LINE_HEIGHT;
		const int max_scroll = browser->count - DC_BROWSER_VISIBLE_LINES;
		int thumb_h = track_h * DC_BROWSER_VISIBLE_LINES / browser->count;
		int thumb_y;

		if (thumb_h < 12)
			thumb_h = 12;
		thumb_y = track_y + (track_h - thumb_h) * browser->scroll / max_scroll;

		dc_ui_fill_rect(screen, track_x, track_y, 4, track_h, DC_COLOR_TRACK);
		dc_ui_fill_rect(screen, track_x, thumb_y, 4, thumb_h, DC_COLOR_THUMB);
	}

	snprintf(line, sizeof(line), "ROM %d / %d", browser->selected + 1,
		 browser->count);
	dc_ui_draw_text(screen, 12, 452, line, DC_COLOR_DIM, DC_COLOR_BG);
}

/*
 * Edge press plus auto-repeat: emits immediately when first held, pauses for
 * an initial delay, then fires at a steady rate while the direction is held.
 * Assumes a fixed poll cadence (see DC_FRAME_MS in dc_browser_run).
 */
static bool dc_repeat(bool pressed, int *timer)
{
	if (!pressed) {
		*timer = 0;
		return false;
	}

	if (*timer <= 0) {
		*timer = DC_REPEAT_DELAY_FRAMES;
		return true;
	}

	if (--(*timer) == 0) {
		*timer = DC_REPEAT_RATE_FRAMES;
		return true;
	}

	return false;
}

static void dc_browser_poll_input(struct dc_browser *browser,
				  struct dc_browser_input *input)
{
	maple_device_t *controller = maple_enum_type(0, MAPLE_FUNC_CONTROLLER);
	static uint32_t previous_buttons = 0xFFFF;
	static int t_up, t_down, t_left, t_right;
	cont_state_t *pad;
	uint32_t buttons;
	uint32_t changed;
	bool up, down, left, right;

	(void)browser;
	memset(input, 0, sizeof(*input));
	if (!controller)
		goto release;

	pad = (cont_state_t *)maple_dev_status(controller);
	if (!pad)
		goto release;

	buttons = pad->buttons;
	changed = buttons ^ previous_buttons;

	/* D-pad or analog stick drive navigation, with hold-to-repeat. */
	up    = (buttons & CONT_DPAD_UP)    || pad->joyy < -DC_ANALOG_THRESHOLD;
	down  = (buttons & CONT_DPAD_DOWN)  || pad->joyy >  DC_ANALOG_THRESHOLD;
	left  = (buttons & CONT_DPAD_LEFT)  || pad->joyx < -DC_ANALOG_THRESHOLD;
	right = (buttons & CONT_DPAD_RIGHT) || pad->joyx >  DC_ANALOG_THRESHOLD;

	input->up = dc_repeat(up, &t_up);
	input->down = dc_repeat(down, &t_down);
	input->page_up = dc_repeat(left, &t_left);
	input->page_down = dc_repeat(right, &t_right);

	/* Action buttons stay edge-triggered. */
	if ((buttons & CONT_A) && (changed & CONT_A))
		input->select = true;
	if ((buttons & CONT_B) && (changed & CONT_B))
		input->next_device = true;
	if ((buttons & CONT_START) && (changed & CONT_START))
		input->refresh = true;
	if ((buttons & CONT_X) && (changed & CONT_X))
		input->exit = true;

	previous_buttons = buttons;
	return;

release:
	t_up = t_down = t_left = t_right = 0;
	previous_buttons = 0xFFFF;
}

static void dc_browser_update_scroll(struct dc_browser *browser)
{
	if (browser->count <= 0) {
		browser->scroll = 0;
		return;
	}

	if (browser->selected < browser->scroll)
		browser->scroll = browser->selected;
	if (browser->selected >= browser->scroll + DC_BROWSER_VISIBLE_LINES)
		browser->scroll = browser->selected - DC_BROWSER_VISIBLE_LINES + 1;
}

bool dc_browser_run(struct dc_browser *browser, char *selected_path,
		    size_t selected_len)
{
	uint16_t screen[DC_SCREEN_HEIGHT][DC_SCREEN_WIDTH];
	bool dirty = true;

	if (!browser || !selected_path || selected_len == 0)
		return false;

	if (dc_browser_scan(browser) < 0)
		printf("walnut-dc: unable to scan '%s'\n", browser->root_path);

	while (1) {
		const uint64_t frame_start = timer_ms_gettime64();
		struct dc_browser_input input;
		uint64_t elapsed;

		/* Only repaint and re-upload the UI texture when state changes. */
		if (dirty) {
			dc_browser_draw(browser, screen);
			dc_video_present_screen(screen);
			dirty = false;
		}

		dc_browser_poll_input(browser, &input);

		if (input.exit)
			return false;

		if (input.next_device) {
			browser->root_index++;
			if (!dc_browser_roots[browser->root_index])
				browser->root_index = 0;
			strncpy(browser->root_path, dc_browser_roots[browser->root_index],
				sizeof(browser->root_path) - 1);
			browser->root_path[sizeof(browser->root_path) - 1] = '\0';
			dc_browser_scan(browser);
			dirty = true;
		}

		if (input.refresh) {
			dc_browser_scan(browser);
			dirty = true;
		}

		if (input.up && browser->count > 0) {
			browser->selected--;
			if (browser->selected < 0)
				browser->selected = browser->count - 1;
			dirty = true;
		}

		if (input.down && browser->count > 0) {
			browser->selected++;
			if (browser->selected >= browser->count)
				browser->selected = 0;
			dirty = true;
		}

		if (input.page_up && browser->count > 0) {
			browser->selected -= DC_BROWSER_VISIBLE_LINES;
			if (browser->selected < 0)
				browser->selected = 0;
			dirty = true;
		}

		if (input.page_down && browser->count > 0) {
			browser->selected += DC_BROWSER_VISIBLE_LINES;
			if (browser->selected >= browser->count)
				browser->selected = browser->count - 1;
			dirty = true;
		}

		if (input.select && browser->count > 0) {
			strncpy(selected_path, browser->entries[browser->selected].path,
				selected_len - 1);
			selected_path[selected_len - 1] = '\0';
			return true;
		}

		if (dirty)
			dc_browser_update_scroll(browser);

		/* Hold the loop near 60 Hz so auto-repeat timing is stable. */
		elapsed = timer_ms_gettime64() - frame_start;
		if (elapsed < DC_FRAME_MS)
			timer_spin((int)(DC_FRAME_MS - elapsed));
	}
}

int dc_save_path_from_rom(const char *rom_path, char *save_path, size_t save_path_len)
{
	const char *dot;
	size_t base_len;

	if (!rom_path || !save_path || save_path_len == 0)
		return -1;

	dot = strrchr(rom_path, '.');
	if (!dot || dot == rom_path)
		base_len = strlen(rom_path);
	else
		base_len = (size_t)(dot - rom_path);

	if (base_len + 4 + 1 > save_path_len)
		return -1;

	memcpy(save_path, rom_path, base_len);
	memcpy(save_path + base_len, ".sav", 5);
	return 0;
}

int dc_rom_load(struct dc_priv *priv, const char *rom_path)
{
	FILE *f;
	long size;

	if (!priv || !rom_path)
		return -1;

	f = fopen(rom_path, "rb");
	if (!f) {
		printf("walnut-dc: unable to open ROM '%s'\n", rom_path);
		return -1;
	}

	if (fseek(f, 0, SEEK_END) != 0) {
		fclose(f);
		return -1;
	}

	size = ftell(f);
	if (size <= 0 || size > (8 * 1024 * 1024)) {
		fclose(f);
		printf("walnut-dc: invalid ROM size (%ld)\n", size);
		return -1;
	}

	rewind(f);
	priv->rom = (uint8_t *)malloc((size_t)size);
	if (!priv->rom) {
		fclose(f);
		return -1;
	}

	if (fread(priv->rom, 1, (size_t)size, f) != (size_t)size) {
		free(priv->rom);
		priv->rom = NULL;
		fclose(f);
		return -1;
	}

	fclose(f);
	strncpy(priv->rom_path, rom_path, sizeof(priv->rom_path) - 1);
	priv->rom_path[sizeof(priv->rom_path) - 1] = '\0';

	if (dc_save_path_from_rom(rom_path, priv->save_path, sizeof(priv->save_path)) != 0)
		priv->save_path[0] = '\0';

	return 0;
}

void dc_rom_unload(struct dc_priv *priv)
{
	if (!priv)
		return;

	free(priv->rom);
	priv->rom = NULL;
	free(priv->cart_ram);
	priv->cart_ram = NULL;
	free(priv->bootrom);
	priv->bootrom = NULL;
	priv->save_size = 0;
	priv->rom_path[0] = '\0';
	priv->save_path[0] = '\0';
}

int dc_cart_ram_read_file(const char *save_path, uint8_t **dest, size_t len)
{
	FILE *f;

	if (len == 0) {
		*dest = NULL;
		return 0;
	}

	*dest = (uint8_t *)calloc(1, len);
	if (!*dest)
		return -1;

	f = fopen(save_path, "rb");
	if (!f)
		return 0;

	if (fread(*dest, 1, len, f) != len)
		memset(*dest, 0, len);

	fclose(f);
	return 0;
}

int dc_cart_ram_write_file(const char *save_path, const uint8_t *data, size_t len)
{
	FILE *f;

	if (!save_path || !data || len == 0)
		return 0;

	f = fopen(save_path, "wb");
	if (!f) {
		printf("walnut-dc: unable to write save '%s'\n", save_path);
		return -1;
	}

	if (fwrite(data, 1, len, f) != len) {
		fclose(f);
		return -1;
	}

	fclose(f);
	return 0;
}
