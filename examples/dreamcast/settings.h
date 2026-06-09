/*
 * Walnut-CGB Dreamcast frontend — persistent settings.
 * Copyright (c) 2025 Mr. Paul (https://github.com/Mr-PauI)
 * Licensed under the MIT License.
 */

#ifndef DC_SETTINGS_H
#define DC_SETTINGS_H

#include <stdbool.h>
#include <stdint.h>

#include "video.h"

#define DC_SETTINGS_AUTOSAVE_MIN_SEC 10
#define DC_SETTINGS_AUTOSAVE_MAX_SEC 300
#define DC_SETTINGS_AUTOSAVE_DEFAULT_SEC 60
#define DC_SETTINGS_VOLUME_MIN 0
#define DC_SETTINGS_VOLUME_MAX 100
#define DC_SETTINGS_VOLUME_DEFAULT 100
#define DC_SETTINGS_ROW_COUNT 8

enum dc_audio_buffer_mode
{
	DC_AUDIO_BUFFER_LOW = 0,
	DC_AUDIO_BUFFER_NORMAL,
	DC_AUDIO_BUFFER_HIGH,
	DC_AUDIO_BUFFER_COUNT
};

struct dc_settings
{
	uint8_t palette_index;
	enum dc_scale_mode scale_mode;
	bool status_bar;
	bool frameskip;
	bool autosave_enabled;
	int autosave_interval_sec;
	uint8_t volume;
	bool muted;
	enum dc_audio_buffer_mode audio_buffer;
};

void dc_settings_init_defaults(struct dc_settings *settings);
void dc_settings_load(struct dc_settings *settings);
int dc_settings_save(const struct dc_settings *settings);
const char *dc_audio_buffer_name(enum dc_audio_buffer_mode mode);

#endif /* DC_SETTINGS_H */
