/*
 * Walnut-CGB Dreamcast frontend — start, main, pause, and settings menus.
 * Copyright (c) 2025 Mr. Paul (https://github.com/Mr-PauI)
 * Licensed under the MIT License.
 */

#include <stdio.h>
#include <string.h>

#include <kos.h>
#include <dc/maple/controller.h>

#include "display.h"
#include "input.h"
#include "menu.h"
#include "palette.h"
#include "settings.h"
#include "toast.h"
#include "ui.h"
#include "video.h"

#define DC_SETTINGS_LINE_HEIGHT 22
#define DC_SETTINGS_LIST_TOP    72

#define DC_MENU_LINE_HEIGHT    28
#define DC_MENU_LIST_TOP       120

#define DC_CONTROLS_LINE_HEIGHT 20
#define DC_MENU_FRAME_MS       16
#define DC_MENU_SPLASH_MS      2500

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

static dc_settings_apply_cb settings_apply_cb;

static void dc_menu_poll_input(struct dc_menu_input *input)
{
	maple_device_t *controller = maple_enum_type(0, MAPLE_FUNC_CONTROLLER);
	static uint32_t previous_buttons = 0xFFFF;
	static int t_up, t_down;
	cont_state_t *pad;
	uint32_t buttons;
	uint32_t changed;
	int vert;

	memset(input, 0, sizeof(*input));
	if (!controller)
		goto release;

	pad = (cont_state_t *)maple_dev_status(controller);
	if (!pad)
		goto release;

	buttons = pad->buttons;
	changed = buttons ^ previous_buttons;

	vert = dc_input_axis((buttons & CONT_DPAD_UP) != 0,
			     (buttons & CONT_DPAD_DOWN) != 0, pad->joyy,
			     DC_INPUT_ANALOG_THRESHOLD);

	input->up = dc_input_repeat(vert < 0, &t_up);
	input->down = dc_input_repeat(vert > 0, &t_down);

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

	dc_ui_clear(screen, DC_UI_COLOR_BG);
	dc_ui_draw_header(screen, menu->title, menu->subtitle);

	for (i = 0; i < menu->count; i++) {
		const int y = DC_MENU_LIST_TOP + i * DC_MENU_LINE_HEIGHT;
		uint16_t fg = DC_UI_COLOR_FG;
		uint16_t bg = DC_UI_COLOR_BG;

		if (i == menu->selected) {
			dc_ui_fill_rect(screen, 24, y - 2, DC_SCREEN_WIDTH - 48,
					DC_MENU_LINE_HEIGHT, DC_UI_COLOR_SELECT);
			fg = DC_UI_COLOR_FG;
			bg = DC_UI_COLOR_SELECT;
		}

		snprintf(line, sizeof(line), "%c %s",
			 i == menu->selected ? '>' : ' ', menu->items[i]);
		dc_ui_draw_text(screen, 32, y, line, fg, bg);
	}

	if (footer)
		dc_ui_draw_footer(screen, footer);

	dc_toast_draw(screen);
}

static int dc_menu_run_list(struct dc_menu_list *menu, const char *footer)
{
	uint16_t screen[DC_SCREEN_HEIGHT][DC_SCREEN_WIDTH];
	bool dirty = true;
	bool toast_visible = false;

	while (1) {
		const uint64_t frame_start = timer_ms_gettime64();
		struct dc_menu_input input;
		const bool toast_active = dc_toast_active();
		uint64_t elapsed;

		if (dirty || toast_active || toast_visible) {
			dc_menu_draw_list(menu, screen, footer);
			dc_video_present_screen(screen);
			dirty = false;
			toast_visible = toast_active;
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
	dc_ui_clear(screen, DC_UI_COLOR_BG);
	dc_ui_fill_rect(screen, 0, 180, DC_SCREEN_WIDTH, 4, DC_UI_COLOR_ACCENT);
	dc_ui_draw_text(screen, 168, 200, "WALNUT-CGB", DC_UI_COLOR_TITLE, DC_UI_COLOR_BG);
	dc_ui_draw_text(screen, 120, 232, "Game Boy / Game Boy Color",
			DC_UI_COLOR_FG, DC_UI_COLOR_BG);
	dc_ui_draw_text(screen, 168, 264, "Dreamcast Edition", DC_UI_COLOR_DIM,
			DC_UI_COLOR_BG);
	dc_ui_fill_rect(screen, 0, 300, DC_SCREEN_WIDTH, 4, DC_UI_COLOR_ACCENT);
	dc_ui_draw_text(screen, 200, 340, prompt, DC_UI_COLOR_FG, DC_UI_COLOR_BG);
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

void dc_menu_show_message(const char *title, const char *message, int duration_ms)
{
	uint16_t screen[DC_SCREEN_HEIGHT][DC_SCREEN_WIDTH];
	const uint64_t end_time = timer_ms_gettime64() + (uint64_t)duration_ms;

	dc_ui_clear(screen, DC_UI_COLOR_BG);
	dc_ui_draw_header(screen, title ? title : "Walnut-CGB", NULL);
	dc_ui_draw_text(screen, 120, 220, message ? message : "", DC_UI_COLOR_FG,
			DC_UI_COLOR_BG);

	while (timer_ms_gettime64() < end_time) {
		dc_video_present_screen(screen);
		timer_spin(DC_MENU_FRAME_MS);
	}
}

enum dc_main_menu_action dc_main_menu_run(void)
{
	static const char *items[] = {
		"ROM Library",
		"Settings",
		"Controls",
		"Exit"
	};
	struct dc_menu_list menu = {
		.title = "Walnut-CGB",
		.subtitle = "Main Menu",
		.items = items,
		.count = 4,
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
	case 2:
		return DC_MAIN_MENU_CONTROLS;
	default:
		return DC_MAIN_MENU_EXIT;
	}
}

enum dc_pause_menu_action dc_pause_menu_run(const char *rom_title, bool has_save)
{
	char subtitle[80];
	char paused_line[80];
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

	snprintf(paused_line, sizeof(paused_line), "Paused: %s",
		 rom_title ? rom_title : "Game");
	paused_line[sizeof(paused_line) - 1] = '\0';
	snprintf(subtitle, sizeof(subtitle), "%.68s", paused_line);
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

void dc_menu_set_settings_apply_callback(dc_settings_apply_cb callback)
{
	settings_apply_cb = callback;
}

static int dc_controls_count_lines(const char *const *lines, unsigned int count)
{
	unsigned int i;
	int visible = 0;

	for (i = 0; i < count; i++) {
		if (lines[i][0] != '\0')
			visible++;
	}

	return visible;
}

static bool dc_controls_is_section_title(const char *const *lines, unsigned int index)
{
	if (!lines[index][0])
		return false;
	if (strchr(lines[index], '=') != NULL)
		return false;

	return index == 0 || lines[index - 1][0] == '\0';
}

static void dc_controls_draw(uint16_t screen[DC_SCREEN_HEIGHT][DC_SCREEN_WIDTH],
			     const char *const *lines, unsigned int count,
			     int scroll)
{
	unsigned int i;
	int line_index = 0;
	int y = DC_UI_CONTENT_TOP;

	dc_ui_clear(screen, DC_UI_COLOR_BG);
	dc_ui_draw_header(screen, "Controls", "Button reference");
	dc_ui_draw_footer(screen, "Up/Dn:Scroll  B:Back");

	for (i = 0; i < count; i++) {
		uint16_t color = DC_UI_COLOR_FG;

		if (lines[i][0] == '\0')
			continue;

		if (line_index < scroll) {
			line_index++;
			continue;
		}

		if (y + 8 > DC_UI_FOOTER_Y)
			break;

		if (dc_controls_is_section_title(lines, i))
			color = DC_UI_COLOR_TITLE;

		dc_ui_draw_text(screen, 24, y, lines[i], color, DC_UI_COLOR_BG);
		y += DC_CONTROLS_LINE_HEIGHT;
		line_index++;
	}
}

void dc_controls_menu_run(void)
{
	static const char *lines[] = {
		"In Game",
		"A/B/Start/X = GB buttons",
		"D-Pad = GB D-Pad",
		"Start+Y = Pause menu",
		"Start+A = Reset game",
		"Start+B = Main menu / exit",
		"Start+X = Toggle frameskip",
		"Start+L = Cycle scale mode",
		"Y = Cycle palette",
		"L/R = Fast-forward (2x)",
		"",
		"Settings (main or pause menu)",
		"Video output, scale, status bar, audio",
		"Changes apply immediately",
		"",
		"ROM Library",
		"A = Load  B = Next device  Y = Grid/List",
		"Left/Right = Page  Start = Refresh  X = Back"
	};
	const unsigned int line_count = sizeof(lines) / sizeof(lines[0]);
	const int visible_lines =
		(DC_UI_FOOTER_Y - DC_UI_CONTENT_TOP) / DC_CONTROLS_LINE_HEIGHT;
	const int total_lines = dc_controls_count_lines(lines, line_count);
	int scroll = 0;
	uint16_t screen[DC_SCREEN_HEIGHT][DC_SCREEN_WIDTH];
	bool dirty = true;

	while (1) {
		const uint64_t frame_start = timer_ms_gettime64();
		struct dc_menu_input input;
		uint64_t elapsed;
		int max_scroll;

		max_scroll = total_lines - visible_lines;
		if (max_scroll < 0)
			max_scroll = 0;
		if (scroll > max_scroll)
			scroll = max_scroll;

		if (dirty) {
			dc_controls_draw(screen, lines, line_count, scroll);
			dc_video_present_screen(screen);
			dirty = false;
		}

		dc_menu_poll_input(&input);
		if (input.back)
			return;

		if (input.up) {
			scroll--;
			if (scroll < 0)
				scroll = 0;
			dirty = true;
		}

		if (input.down) {
			scroll++;
			if (scroll > max_scroll)
				scroll = max_scroll;
			dirty = true;
		}

		elapsed = timer_ms_gettime64() - frame_start;
		if (elapsed < DC_MENU_FRAME_MS)
			timer_spin((int)(DC_MENU_FRAME_MS - elapsed));
	}
}

static void dc_settings_format_row(const struct dc_settings *settings, int row,
				   char *line, size_t line_len, bool selected)
{
	const char *labels[DC_SETTINGS_ROW_COUNT] = {
		"Palette",
		"Video output",
		"Scale mode",
		"Status bar",
		"Frameskip",
		"Autosave",
		"Autosave interval",
		"Volume",
		"Audio buffer"
	};
	const char marker = selected ? '>' : ' ';

	switch (row) {
	case 0:
		snprintf(line, line_len, "%c %s: %s", marker, labels[row],
			 dc_palette_name(settings->palette_index));
		break;
	case 1:
		snprintf(line, line_len, "%c %s: %s", marker, labels[row],
			 dc_video_output_name(settings->video_output));
		break;
	case 2:
		snprintf(line, line_len, "%c %s: %s", marker, labels[row],
			 dc_video_scale_mode_name(settings->scale_mode));
		break;
	case 3:
		snprintf(line, line_len, "%c %s: %s", marker, labels[row],
			 settings->status_bar ? "On" : "Off");
		break;
	case 4:
		snprintf(line, line_len, "%c %s: %s", marker, labels[row],
			 settings->frameskip ? "On" : "Off");
		break;
	case 5:
		snprintf(line, line_len, "%c %s: %s", marker, labels[row],
			 settings->autosave_enabled ? "On" : "Off");
		break;
	case 6:
		snprintf(line, line_len, "%c %s: %d sec", marker, labels[row],
			 settings->autosave_interval_sec);
		break;
	case 7:
		snprintf(line, line_len, "%c %s: %s%u%%", marker, labels[row],
			 settings->muted ? "Mute " : "",
			 settings->muted ? 0U : settings->volume);
		break;
	default:
		snprintf(line, line_len, "%c %s: %s", marker, labels[row],
			 dc_audio_buffer_name(settings->audio_buffer));
		break;
	}
}

static void dc_settings_draw_value_screen(const struct dc_settings *settings,
					    int selected_row,
					    uint16_t screen[DC_SCREEN_HEIGHT][DC_SCREEN_WIDTH])
{
	char line[80];
	int row;

	dc_ui_clear(screen, DC_UI_COLOR_BG);
	dc_ui_draw_header(screen, "Settings", "Video, status bar, and audio options.");

	for (row = 0; row < DC_SETTINGS_ROW_COUNT; row++) {
		const int y = DC_SETTINGS_LIST_TOP + row * DC_SETTINGS_LINE_HEIGHT;
		uint16_t fg = row == selected_row ? DC_UI_COLOR_FG : DC_UI_COLOR_DIM;
		uint16_t bg = DC_UI_COLOR_BG;

		if (row == selected_row) {
			dc_ui_fill_rect(screen, 24, y - 2, DC_SCREEN_WIDTH - 48,
					DC_SETTINGS_LINE_HEIGHT, DC_UI_COLOR_SELECT);
			bg = DC_UI_COLOR_SELECT;
			fg = DC_UI_COLOR_FG;
		}

		dc_settings_format_row(settings, row, line, sizeof(line),
				       row == selected_row);
		dc_ui_draw_text_clipped(screen, 32, y, DC_SCREEN_WIDTH - 48, line, fg, bg);
	}

	dc_ui_draw_footer(screen, "Up/Dn:Row  L/R:Change  A:Mute(vol)  B:Back");
	dc_toast_draw(screen);
}

static bool dc_settings_poll_input(struct dc_settings *settings, int *selected_row,
				   bool *done)
{
	maple_device_t *controller = maple_enum_type(0, MAPLE_FUNC_CONTROLLER);
	static uint32_t previous_buttons = 0xFFFF;
	static int t_up, t_down, t_horiz, last_horiz;
	cont_state_t *pad;
	uint32_t buttons;
	uint32_t changed;
	int vert;
	int horiz;
	int axis_step;
	bool changed_value = false;

	*done = false;
	if (!controller)
		goto release;

	pad = (cont_state_t *)maple_dev_status(controller);
	if (!pad)
		goto release;

	buttons = pad->buttons;
	changed = buttons ^ previous_buttons;

	vert = dc_input_axis((buttons & CONT_DPAD_UP) != 0,
			     (buttons & CONT_DPAD_DOWN) != 0, pad->joyy,
			     DC_INPUT_ANALOG_THRESHOLD);
	horiz = dc_input_axis((buttons & CONT_DPAD_LEFT) != 0,
			      (buttons & CONT_DPAD_RIGHT) != 0, pad->joyx,
			      DC_INPUT_ANALOG_THRESHOLD);

	if (dc_input_repeat(vert < 0, &t_up)) {
		(*selected_row)--;
		if (*selected_row < 0)
			*selected_row = DC_SETTINGS_ROW_COUNT - 1;
		changed_value = true;
	} else if (dc_input_repeat(vert > 0, &t_down)) {
		(*selected_row)++;
		if (*selected_row >= DC_SETTINGS_ROW_COUNT)
			*selected_row = 0;
		changed_value = true;
	}

	axis_step = dc_input_repeat_axis(horiz, &t_horiz, &last_horiz);
	if (axis_step != 0) {
		const int inc = axis_step;
		char toast_line[48];

		switch (*selected_row) {
		case 0:
			settings->palette_index =
				(uint8_t)((settings->palette_index + inc + DC_PALETTE_COUNT) %
					  DC_PALETTE_COUNT);
			dc_toast_show(dc_palette_name(settings->palette_index), 1000);
			break;
		case 1:
			settings->video_output =
				(enum dc_video_output)((settings->video_output + inc +
							DC_VIDEO_OUTPUT_COUNT) %
						       DC_VIDEO_OUTPUT_COUNT);
			dc_toast_show(dc_video_output_name(settings->video_output), 1000);
			break;
		case 2:
			settings->scale_mode =
				(enum dc_scale_mode)((settings->scale_mode + inc + DC_SCALE_COUNT) %
						     DC_SCALE_COUNT);
			dc_toast_show(dc_video_scale_mode_name(settings->scale_mode), 1000);
			break;
		case 3:
			settings->status_bar = !settings->status_bar;
			dc_toast_show(settings->status_bar ? "Status bar on" :
							       "Status bar off",
				      1000);
			break;
		case 4:
			settings->frameskip = !settings->frameskip;
			dc_toast_show(settings->frameskip ? "Frameskip on" : "Frameskip off",
				      1000);
			break;
		case 5:
			settings->autosave_enabled = !settings->autosave_enabled;
			dc_toast_show(settings->autosave_enabled ? "Autosave on" :
								   "Autosave off",
				      1000);
			break;
		case 6:
			settings->autosave_interval_sec += inc * 10;
			if (settings->autosave_interval_sec < DC_SETTINGS_AUTOSAVE_MIN_SEC)
				settings->autosave_interval_sec = DC_SETTINGS_AUTOSAVE_MIN_SEC;
			if (settings->autosave_interval_sec > DC_SETTINGS_AUTOSAVE_MAX_SEC)
				settings->autosave_interval_sec = DC_SETTINGS_AUTOSAVE_MAX_SEC;
			snprintf(toast_line, sizeof(toast_line), "Autosave: %d sec",
				 settings->autosave_interval_sec);
			dc_toast_show(toast_line, 1000);
			break;
		case 7:
			if (settings->muted) {
				settings->muted = false;
				dc_toast_show("Audio unmuted", 1000);
				break;
			}
			settings->volume = (uint8_t)((int)settings->volume + inc * 10);
			if ((int)settings->volume < DC_SETTINGS_VOLUME_MIN)
				settings->volume = DC_SETTINGS_VOLUME_MIN;
			if ((int)settings->volume > DC_SETTINGS_VOLUME_MAX)
				settings->volume = DC_SETTINGS_VOLUME_MAX;
			snprintf(toast_line, sizeof(toast_line), "Volume: %u%%",
				 settings->volume);
			dc_toast_show(toast_line, 1000);
			break;
		default:
			settings->audio_buffer =
				(enum dc_audio_buffer_mode)((settings->audio_buffer + inc +
							     DC_AUDIO_BUFFER_COUNT) %
							    DC_AUDIO_BUFFER_COUNT);
			dc_toast_show(dc_audio_buffer_name(settings->audio_buffer), 1000);
			break;
		}
		changed_value = true;
		if (settings_apply_cb)
			settings_apply_cb(settings);
	}

	if ((buttons & CONT_A) && (changed & CONT_A) && *selected_row == 7) {
		settings->muted = !settings->muted;
		dc_toast_show(settings->muted ? "Audio muted" : "Audio unmuted", 1000);
		changed_value = true;
		if (settings_apply_cb)
			settings_apply_cb(settings);
	}

	if (((buttons & CONT_B) && (changed & CONT_B)) ||
	    ((buttons & CONT_X) && (changed & CONT_X)) ||
	    ((buttons & CONT_A) && (changed & CONT_A) && *selected_row != 7))
		*done = true;

	previous_buttons = buttons;
	return changed_value;

release:
	t_up = t_down = 0;
	t_horiz = last_horiz = 0;
	previous_buttons = 0xFFFF;
	return false;
}

bool dc_settings_menu_run(struct dc_settings *settings)
{
	uint16_t screen[DC_SCREEN_HEIGHT][DC_SCREEN_WIDTH];
	int selected_row = 0;
	bool dirty = true;
	bool toast_visible = false;

	if (!settings)
		return false;

	while (1) {
		const uint64_t frame_start = timer_ms_gettime64();
		bool done = false;
		bool changed;
		const bool toast_active = dc_toast_active();
		uint64_t elapsed;

		if (dirty || toast_active || toast_visible) {
			dc_settings_draw_value_screen(settings, selected_row, screen);
			dc_video_present_screen(screen);
			dirty = false;
			toast_visible = toast_active;
		}

		changed = dc_settings_poll_input(settings, &selected_row, &done);
		if (done) {
			dc_settings_save(settings);
			return true;
		}

		if (changed)
			dirty = true;

		elapsed = timer_ms_gettime64() - frame_start;
		if (elapsed < DC_MENU_FRAME_MS)
			timer_spin((int)(DC_MENU_FRAME_MS - elapsed));
	}
}
