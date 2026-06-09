/*
 * Walnut-CGB Dreamcast frontend — persistent settings.
 * Copyright (c) 2025 Mr. Paul (https://github.com/Mr-PauI)
 * Licensed under the MIT License.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "settings.h"

static const char *dc_settings_paths[] = {
	"/pc/walnut-dc.cfg",
	"/sd/walnut-dc.cfg",
	"/ide/walnut-dc.cfg",
	"/cd/walnut-dc.cfg",
	NULL
};

void dc_settings_init_defaults(struct dc_settings *settings)
{
	settings->palette_index = 3;
	settings->frameskip = false;
	settings->autosave_enabled = true;
	settings->autosave_interval_sec = DC_SETTINGS_AUTOSAVE_DEFAULT_SEC;
}

static void dc_settings_clamp(struct dc_settings *settings)
{
	if (settings->palette_index > 12)
		settings->palette_index = 3;

	if (settings->autosave_interval_sec < DC_SETTINGS_AUTOSAVE_MIN_SEC)
		settings->autosave_interval_sec = DC_SETTINGS_AUTOSAVE_MIN_SEC;
	if (settings->autosave_interval_sec > DC_SETTINGS_AUTOSAVE_MAX_SEC)
		settings->autosave_interval_sec = DC_SETTINGS_AUTOSAVE_MAX_SEC;
}

static int dc_settings_parse_line(struct dc_settings *settings, const char *line)
{
	int value;

	if (sscanf(line, " palette_index = %d", &value) == 1 ||
	    sscanf(line, "palette_index=%d", &value) == 1) {
		settings->palette_index = (uint8_t)value;
		return 0;
	}

	if (sscanf(line, " frameskip = %d", &value) == 1 ||
	    sscanf(line, "frameskip=%d", &value) == 1) {
		settings->frameskip = value != 0;
		return 0;
	}

	if (sscanf(line, " autosave_enabled = %d", &value) == 1 ||
	    sscanf(line, "autosave_enabled=%d", &value) == 1) {
		settings->autosave_enabled = value != 0;
		return 0;
	}

	if (sscanf(line, " autosave_interval_sec = %d", &value) == 1 ||
	    sscanf(line, "autosave_interval_sec=%d", &value) == 1) {
		settings->autosave_interval_sec = value;
		return 0;
	}

	return -1;
}

void dc_settings_load(struct dc_settings *settings)
{
	FILE *f;
	char line[128];
	unsigned int i;

	dc_settings_init_defaults(settings);

	for (i = 0; dc_settings_paths[i] != NULL; i++) {
		f = fopen(dc_settings_paths[i], "r");
		if (!f)
			continue;

		while (fgets(line, sizeof(line), f) != NULL)
			dc_settings_parse_line(settings, line);

		fclose(f);
		break;
	}

	dc_settings_clamp(settings);
}

int dc_settings_save(const struct dc_settings *settings)
{
	FILE *f;
	unsigned int i;

	for (i = 0; dc_settings_paths[i] != NULL; i++) {
		f = fopen(dc_settings_paths[i], "w");
		if (!f)
			continue;

		fprintf(f, "palette_index=%u\n", settings->palette_index);
		fprintf(f, "frameskip=%d\n", settings->frameskip ? 1 : 0);
		fprintf(f, "autosave_enabled=%d\n", settings->autosave_enabled ? 1 : 0);
		fprintf(f, "autosave_interval_sec=%d\n", settings->autosave_interval_sec);
		fclose(f);
		return 0;
	}

	return -1;
}
