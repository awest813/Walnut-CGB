/*
 * Walnut-CGB Dreamcast frontend — start, main, pause, and settings menus.
 * Copyright (c) 2025 Mr. Paul (https://github.com/Mr-PauI)
 * Licensed under the MIT License.
 */

#include <stdio.h>
#include <string.h>

#include <kos.h>
#include <dc/maple/controller.h>

#include "menu.h"
#include "settings.h"
#include "ui.h"
#include "video.h"

#define DC_MENU_LINE_HEIGHT    28
#define DC_MENU_LIST_TOP       120
#define DC_MENU_REPEAT_DELAY   18
#define DC_MENU_REPEAT_RATE    4
#define DC_MENU_ANALOG_THRESH  64
#define DC_MENU_FRAME_MS       16
#define DC_MENU_SPLASH_MS      2500

#define DC_COLOR_BG      0x0000
#define DC_COLOR_FG      0x7FFF
#define DC_COLOR_DIM     0x39CE
#define DC_COLOR_SELECT  0x001F
#define DC_COLOR_HEADER  0x03FF
#define DC_COLOR_TITLE   0x7FE0
#define DC_COLOR_ACCENT  0xF800

struct dc_menu_input
{
	bool up;
	bool down;
	bool select;
	bool back;
};

struct dc_menu_list
{
	const char *title;
	const char *subtitle;
	const char *const *items;
	int count;
	int selected;
};

static bool dc_menu_repeat(bool pressed, int *timer)
{
	if (!pressed) {
		*timer = 0;
		return false;
	}

	if (*timer <= 0) {
		*timer = DC_MENU_REPEAT_DELAY;
		return true;
	}

	if (--(*timer) == 0) {
		*timer = DC_MENU_REPEAT_RATE;
		return true;
	}

	return false;
}

static void dc_menu_poll_input(struct dc_menu_input *input)
{
	maple_device_t *controller = maple_enum_type(0, MAPLE_FUNC_CONTROLLER);
	static uint32_t previous_buttons = 0xFFFF;
	static int t_up, t_down;
	cont_state_t *pad;
	uint32_t buttons;
	uint32_t changed;
	bool up, down;

	memset(input, 0, sizeof(*input));
	if (!controller)
		goto release;

	pad = (cont_state_t *)maple_dev_status(controller);
	if (!pad)
		goto release;

	buttons = pad->buttons;
	changed = buttons ^ previous_buttons;

	up = (buttons & CONT_DPAD_UP) || pad->joyy < -DC_MENU_ANALOG_THRESH;
	down = (buttons & CONT_DPAD_DOWN) || pad->joyy > DC_MENU_ANALOG_THRESH;

	input->up = dc_menu_repeat(up, &t_up);
	input->down = dc_menu_repeat(down, &t_down);

	if ((buttons & CONT_A) && (changed & CONT_A))
		input->select = true;
	if ((buttons & CONT_B) && (changed & CONT_B))
		input->back = true;
	if ((buttons & CONT_START) && (changed & CONT_START))
		input->select = true;
	if ((buttons & CONT_X) && (changed & CONT_X))
		input->back = true;

	previous_buttons = buttons;
	return;

release:
	t_up = t_down = 0;
	previous_buttons = 0xFFFF;
}

static void dc_menu_draw_list(const struct dc_menu_list *menu,
			      uint16_t screen[DC_SCREEN_HEIGHT][DC_SCREEN_WIDTH],
			      const char *footer)
{
	char line[80];
	int i;

	dc_ui_clear(screen, DC_COLOR_BG);
	dc_ui_fill_rect(screen, 0, 0, DC_SCREEN_WIDTH, 36, DC_COLOR_HEADER);
	dc_ui_draw_text(screen, 12, 12, menu->title, DC_COLOR_BG, DC_COLOR_HEADER);

	if (menu->subtitle && menu->subtitle[0] != '\0')
		dc_ui_draw_text(screen, 12, 48, menu->subtitle, DC_COLOR_DIM, DC_COLOR_BG);

	for (i = 0; i < menu->count; i++) {
		const int y = DC_MENU_LIST_TOP + i * DC_MENU_LINE_HEIGHT;
		uint16_t fg = DC_COLOR_FG;
		uint16_t bg = DC_COLOR_BG;

		if (i == menu->selected) {
			dc_ui_fill_rect(screen, 24, y - 2, DC_SCREEN_WIDTH - 48,
					DC_MENU_LINE_HEIGHT, DC_COLOR_SELECT);
			fg = DC_COLOR_FG;
			bg = DC_COLOR_SELECT;
		}

		snprintf(line, sizeof(line), "%c %s",
			 i == menu->selected ? '>' : ' ', menu->items[i]);
		dc_ui_draw_text(screen, 32, y, line, fg, bg);
	}

	if (footer)
		dc_ui_draw_text(screen, 12, 452, footer, DC_COLOR_DIM, DC_COLOR_BG);
}

static int dc_menu_run_list(struct dc_menu_list *menu, const char *footer)
{
	uint16_t screen[DC_SCREEN_HEIGHT][DC_SCREEN_WIDTH];
	bool dirty = true;

	while (1) {
		const uint64_t frame_start = timer_ms_gettime64();
		struct dc_menu_input input;
		uint64_t elapsed;

		if (dirty) {
			dc_menu_draw_list(menu, screen, footer);
			dc_video_present_screen(screen);
			dirty = false;
		}

		dc_menu_poll_input(&input);

		if (input.back)
			return -1;

		if (input.up) {
			menu->selected--;
			if (menu->selected < 0)
				menu->selected = menu->count - 1;
			dirty = true;
		}

		if (input.down) {
			menu->selected++;
			if (menu->selected >= menu->count)
				menu->selected = 0;
			dirty = true;
		}

		if (input.select)
			return menu->selected;

		elapsed = timer_ms_gettime64() - frame_start;
		if (elapsed < DC_MENU_FRAME_MS)
			timer_spin((int)(DC_MENU_FRAME_MS - elapsed));
	}
}

static void dc_menu_draw_splash(uint16_t screen[DC_SCREEN_HEIGHT][DC_SCREEN_WIDTH],
				const char *prompt)
{
	dc_ui_clear(screen, DC_COLOR_BG);
	dc_ui_fill_rect(screen, 0, 180, DC_SCREEN_WIDTH, 4, DC_COLOR_ACCENT);
	dc_ui_draw_text(screen, 168, 200, "WALNUT-CGB", DC_COLOR_TITLE, DC_COLOR_BG);
	dc_ui_draw_text(screen, 120, 232, "Game Boy / Game Boy Color", DC_COLOR_FG, DC_COLOR_BG);
	dc_ui_draw_text(screen, 168, 264, "Dreamcast Edition", DC_COLOR_DIM, DC_COLOR_BG);
	dc_ui_fill_rect(screen, 0, 300, DC_SCREEN_WIDTH, 4, DC_COLOR_ACCENT);
	dc_ui_draw_text(screen, 200, 340, prompt, DC_COLOR_FG, DC_COLOR_BG);
}

bool dc_start_menu_run(void)
{
	uint16_t screen[DC_SCREEN_HEIGHT][DC_SCREEN_WIDTH];
	const uint64_t splash_end = timer_ms_gettime64() + DC_MENU_SPLASH_MS;
	bool dirty = true;

	while (1) {
		const uint64_t frame_start = timer_ms_gettime64();
		struct dc_menu_input input;
		uint64_t elapsed;

		if (dirty) {
			dc_menu_draw_splash(screen, "Press A or Start");
			dc_video_present_screen(screen);
			dirty = false;
		}

		dc_menu_poll_input(&input);
		if (input.select)
			return true;

		if (timer_ms_gettime64() >= splash_end)
			return true;

		elapsed = timer_ms_gettime64() - frame_start;
		if (elapsed < DC_MENU_FRAME_MS)
			timer_spin((int)(DC_MENU_FRAME_MS - elapsed));
	}
}

enum dc_main_menu_action dc_main_menu_run(void)
{
	static const char *items[] = {
		"ROM Library",
		"Settings",
		"Exit"
	};
	struct dc_menu_list menu = {
		.title = "Walnut-CGB",
		.subtitle = "Main Menu",
		.items = items,
		.count = 3,
		.selected = 0
	};
	int choice;

	choice = dc_menu_run_list(&menu, "A:Select  B:Exit");
	if (choice < 0)
		return DC_MAIN_MENU_EXIT;

	switch (choice) {
	case 0:
		return DC_MAIN_MENU_ROM_LIBRARY;
	case 1:
		return DC_MAIN_MENU_SETTINGS;
	default:
		return DC_MAIN_MENU_EXIT;
	}
}

enum dc_pause_menu_action dc_pause_menu_run(const char *rom_title, bool has_save)
{
	char subtitle[64];
	static const char *items_with_save[] = {
		"Resume",
		"Save Game",
		"Load Game",
		"Settings",
		"Main Menu",
		"Exit Game"
	};
	static const char *items_no_save[] = {
		"Resume",
		"Settings",
		"Main Menu",
		"Exit Game"
	};
	struct dc_menu_list menu;
	int choice;

	snprintf(subtitle, sizeof(subtitle), "Paused: %s",
		 rom_title ? rom_title : "Game");
	subtitle[sizeof(subtitle) - 1] = '\0';

	if (has_save) {
		menu.title = "Pause Menu";
		menu.subtitle = subtitle;
		menu.items = items_with_save;
		menu.count = 6;
	} else {
		menu.title = "Pause Menu";
		menu.subtitle = subtitle;
		menu.items = items_no_save;
		menu.count = 4;
	}
	menu.selected = 0;

	choice = dc_menu_run_list(&menu, "A:Select  B:Resume");
	if (choice < 0)
		return DC_PAUSE_MENU_RESUME;

	if (has_save) {
		switch (choice) {
		case 0:
			return DC_PAUSE_MENU_RESUME;
		case 1:
			return DC_PAUSE_MENU_SAVE;
		case 2:
			return DC_PAUSE_MENU_LOAD;
		case 3:
			return DC_PAUSE_MENU_SETTINGS;
		case 4:
			return DC_PAUSE_MENU_MAIN_MENU;
		default:
			return DC_PAUSE_MENU_EXIT;
		}
	}

	switch (choice) {
	case 0:
		return DC_PAUSE_MENU_RESUME;
	case 1:
		return DC_PAUSE_MENU_SETTINGS;
	case 2:
		return DC_PAUSE_MENU_MAIN_MENU;
	default:
		return DC_PAUSE_MENU_EXIT;
	}
}

static void dc_settings_draw_value_screen(const struct dc_settings *settings,
					    int selected_row,
					    uint16_t screen[DC_SCREEN_HEIGHT][DC_SCREEN_WIDTH])
{
	char line[80];

	dc_ui_clear(screen, DC_COLOR_BG);
	dc_ui_fill_rect(screen, 0, 0, DC_SCREEN_WIDTH, 36, DC_COLOR_HEADER);
	dc_ui_draw_text(screen, 12, 12, "Settings", DC_COLOR_BG, DC_COLOR_HEADER);
	dc_ui_draw_text(screen, 12, 48, "Adjust options; changes save on exit.",
			DC_COLOR_DIM, DC_COLOR_BG);

	{
		const char *labels[] = {
			"Palette index",
			"Frameskip",
			"Autosave",
			"Autosave interval"
		};
		int row;

		for (row = 0; row < 4; row++) {
			const int y = DC_MENU_LIST_TOP + row * DC_MENU_LINE_HEIGHT;
			uint16_t fg = row == selected_row ? DC_COLOR_FG : DC_COLOR_DIM;
			uint16_t bg = DC_COLOR_BG;

			if (row == selected_row) {
				dc_ui_fill_rect(screen, 24, y - 2, DC_SCREEN_WIDTH - 48,
						DC_MENU_LINE_HEIGHT, DC_COLOR_SELECT);
				bg = DC_COLOR_SELECT;
			}

			switch (row) {
			case 0:
				snprintf(line, sizeof(line), "%c %s: %u",
					 row == selected_row ? '>' : ' ',
					 labels[row], settings->palette_index);
				break;
			case 1:
				snprintf(line, sizeof(line), "%c %s: %s",
					 row == selected_row ? '>' : ' ',
					 labels[row], settings->frameskip ? "On" : "Off");
				break;
			case 2:
				snprintf(line, sizeof(line), "%c %s: %s",
					 row == selected_row ? '>' : ' ',
					 labels[row],
					 settings->autosave_enabled ? "On" : "Off");
				break;
			default:
				snprintf(line, sizeof(line), "%c %s: %d sec",
					 row == selected_row ? '>' : ' ',
					 labels[row], settings->autosave_interval_sec);
				break;
			}

			dc_ui_draw_text(screen, 32, y, line, fg, bg);
		}
	}

	dc_ui_draw_text(screen, 12, 452,
			"Up/Dn:Row  L/R:Change  A/B:Back",
			DC_COLOR_DIM, DC_COLOR_BG);
}

static void dc_settings_poll_input(struct dc_settings *settings, int *selected_row,
				   bool *done)
{
	maple_device_t *controller = maple_enum_type(0, MAPLE_FUNC_CONTROLLER);
	static uint32_t previous_buttons = 0xFFFF;
	static int t_up, t_down, t_left, t_right;
	cont_state_t *pad;
	uint32_t buttons;
	uint32_t changed;
	bool up, down, left, right;

	*done = false;
	if (!controller)
		goto release;

	pad = (cont_state_t *)maple_dev_status(controller);
	if (!pad)
		goto release;

	buttons = pad->buttons;
	changed = buttons ^ previous_buttons;

	up = (buttons & CONT_DPAD_UP) || pad->joyy < -DC_MENU_ANALOG_THRESH;
	down = (buttons & CONT_DPAD_DOWN) || pad->joyy > DC_MENU_ANALOG_THRESH;
	left = (buttons & CONT_DPAD_LEFT) || pad->joyx < -DC_MENU_ANALOG_THRESH;
	right = (buttons & CONT_DPAD_RIGHT) || pad->joyx > DC_MENU_ANALOG_THRESH;

	if (dc_menu_repeat(up, &t_up)) {
		(*selected_row)--;
		if (*selected_row < 0)
			*selected_row = 3;
	}

	if (dc_menu_repeat(down, &t_down)) {
		(*selected_row)++;
		if (*selected_row > 3)
			*selected_row = 0;
	}

	if (dc_menu_repeat(left, &t_left) || dc_menu_repeat(right, &t_right)) {
		const int inc = right ? 1 : -1;

		switch (*selected_row) {
		case 0:
			settings->palette_index =
				(uint8_t)((settings->palette_index + inc + 13) % 13);
			break;
		case 1:
			settings->frameskip = !settings->frameskip;
			break;
		case 2:
			settings->autosave_enabled = !settings->autosave_enabled;
			break;
		case 3:
			settings->autosave_interval_sec += inc * 10;
			if (settings->autosave_interval_sec < DC_SETTINGS_AUTOSAVE_MIN_SEC)
				settings->autosave_interval_sec = DC_SETTINGS_AUTOSAVE_MIN_SEC;
			if (settings->autosave_interval_sec > DC_SETTINGS_AUTOSAVE_MAX_SEC)
				settings->autosave_interval_sec = DC_SETTINGS_AUTOSAVE_MAX_SEC;
			break;
		default:
			break;
		}
	}

	if (((buttons & CONT_A) && (changed & CONT_A)) ||
	    ((buttons & CONT_B) && (changed & CONT_B)) ||
	    ((buttons & CONT_X) && (changed & CONT_X)))
		*done = true;

	previous_buttons = buttons;
	return;

release:
	t_up = t_down = t_left = t_right = 0;
	previous_buttons = 0xFFFF;
}

bool dc_settings_menu_run(struct dc_settings *settings)
{
	uint16_t screen[DC_SCREEN_HEIGHT][DC_SCREEN_WIDTH];
	int selected_row = 0;
	bool dirty = true;

	if (!settings)
		return false;

	while (1) {
		const uint64_t frame_start = timer_ms_gettime64();
		bool done = false;
		uint64_t elapsed;

		if (dirty) {
			dc_settings_draw_value_screen(settings, selected_row, screen);
			dc_video_present_screen(screen);
			dirty = false;
		}

		dc_settings_poll_input(settings, &selected_row, &done);
		if (done) {
			dc_settings_save(settings);
			return true;
		}

		dirty = true;

		elapsed = timer_ms_gettime64() - frame_start;
		if (elapsed < DC_MENU_FRAME_MS)
			timer_spin((int)(DC_MENU_FRAME_MS - elapsed));
	}
}
