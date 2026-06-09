/*
 * Walnut-CGB Dreamcast frontend — persistent settings.
 * Copyright (c) 2025 Mr. Paul (https://github.com/Mr-PauI)
 * Licensed under the MIT License.
 */

#ifndef DC_SETTINGS_H
#define DC_SETTINGS_H

#include <stdbool.h>
#include <stdint.h>

#define DC_SETTINGS_AUTOSAVE_MIN_SEC 10
#define DC_SETTINGS_AUTOSAVE_MAX_SEC 300
#define DC_SETTINGS_AUTOSAVE_DEFAULT_SEC 60

struct dc_settings
{
	uint8_t palette_index;
	bool frameskip;
	bool autosave_enabled;
	int autosave_interval_sec;
};

void dc_settings_init_defaults(struct dc_settings *settings);
void dc_settings_load(struct dc_settings *settings);
int dc_settings_save(const struct dc_settings *settings);

#endif /* DC_SETTINGS_H */
