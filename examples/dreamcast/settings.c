/*
 * Walnut-CGB Dreamcast frontend — persistent settings.
 * Copyright (c) 2025 Mr. Paul (https://github.com/Mr-PauI)
 * Licensed under the MIT License.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "palette.h"
#include "settings.h"

static const char *const dc_audio_buffer_names[DC_AUDIO_BUFFER_COUNT] = {
	"Low latency",
	"Normal",
	"Stable"
};

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
	settings->video_output = DC_VIDEO_OUTPUT_AUTO;
	settings->scale_mode = DC_SCALE_3X;
	settings->status_bar = true;
	settings->frameskip = false;
	settings->autosave_enabled = true;
	settings->autosave_interval_sec = DC_SETTINGS_AUTOSAVE_DEFAULT_SEC;
	settings->volume = DC_SETTINGS_VOLUME_DEFAULT;
	settings->muted = false;
	settings->audio_buffer = DC_AUDIO_BUFFER_NORMAL;
}

const char *dc_audio_buffer_name(enum dc_audio_buffer_mode mode)
{
	if (mode >= DC_AUDIO_BUFFER_COUNT)
		return dc_audio_buffer_names[DC_AUDIO_BUFFER_NORMAL];

	return dc_audio_buffer_names[mode];
}

static void dc_settings_clamp(struct dc_settings *settings)
{
	if (settings->palette_index >= DC_PALETTE_COUNT)
		settings->palette_index = 3;

	if (settings->video_output >= DC_VIDEO_OUTPUT_COUNT)
		settings->video_output = DC_VIDEO_OUTPUT_AUTO;

	if (settings->scale_mode >= DC_SCALE_COUNT)
		settings->scale_mode = DC_SCALE_3X;

	if (settings->autosave_interval_sec < DC_SETTINGS_AUTOSAVE_MIN_SEC)
		settings->autosave_interval_sec = DC_SETTINGS_AUTOSAVE_MIN_SEC;
	if (settings->autosave_interval_sec > DC_SETTINGS_AUTOSAVE_MAX_SEC)
		settings->autosave_interval_sec = DC_SETTINGS_AUTOSAVE_MAX_SEC;

	if (settings->volume > DC_SETTINGS_VOLUME_MAX)
		settings->volume = DC_SETTINGS_VOLUME_DEFAULT;

	if (settings->audio_buffer >= DC_AUDIO_BUFFER_COUNT)
		settings->audio_buffer = DC_AUDIO_BUFFER_NORMAL;
}

static int dc_settings_parse_line(struct dc_settings *settings, const char *line)
{
	int value;

	if (sscanf(line, " palette_index = %d", &value) == 1 ||
	    sscanf(line, "palette_index=%d", &value) == 1) {
		settings->palette_index = (uint8_t)value;
		return 0;
	}

	if (sscanf(line, " video_output = %d", &value) == 1 ||
	    sscanf(line, "video_output=%d", &value) == 1) {
		settings->video_output = (enum dc_video_output)value;
		return 0;
	}

	if (sscanf(line, " scale_mode = %d", &value) == 1 ||
	    sscanf(line, "scale_mode=%d", &value) == 1) {
		settings->scale_mode = (enum dc_scale_mode)value;
		return 0;
	}

	if (sscanf(line, " status_bar = %d", &value) == 1 ||
	    sscanf(line, "status_bar=%d", &value) == 1) {
		settings->status_bar = value != 0;
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

	if (sscanf(line, " volume = %d", &value) == 1 ||
	    sscanf(line, "volume=%d", &value) == 1) {
		settings->volume = (uint8_t)value;
		return 0;
	}

	if (sscanf(line, " muted = %d", &value) == 1 ||
	    sscanf(line, "muted=%d", &value) == 1) {
		settings->muted = value != 0;
		return 0;
	}

	if (sscanf(line, " audio_buffer = %d", &value) == 1 ||
	    sscanf(line, "audio_buffer=%d", &value) == 1) {
		settings->audio_buffer = (enum dc_audio_buffer_mode)value;
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
		fprintf(f, "video_output=%d\n", (int)settings->video_output);
		fprintf(f, "scale_mode=%d\n", (int)settings->scale_mode);
		fprintf(f, "status_bar=%d\n", settings->status_bar ? 1 : 0);
		fprintf(f, "frameskip=%d\n", settings->frameskip ? 1 : 0);
		fprintf(f, "autosave_enabled=%d\n", settings->autosave_enabled ? 1 : 0);
		fprintf(f, "autosave_interval_sec=%d\n", settings->autosave_interval_sec);
		fprintf(f, "volume=%u\n", settings->volume);
		fprintf(f, "muted=%d\n", settings->muted ? 1 : 0);
		fprintf(f, "audio_buffer=%d\n", (int)settings->audio_buffer);
		fclose(f);
		return 0;
	}

	return -1;
}
