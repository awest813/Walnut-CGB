/*
 * Walnut-CGB Dreamcast frontend — shared private state.
 * Copyright (c) 2025 Mr. Paul (https://github.com/Mr-PauI)
 * Licensed under the MIT License.
 */

#ifndef DC_PRIV_H
#define DC_PRIV_H

#include <stddef.h>
#include <stdint.h>

#include "../../walnut_cgb.h"

#define DC_FB_TEX_WIDTH  256
#define DC_FB_TEX_HEIGHT 256

struct dc_priv
{
	uint8_t *rom;
	uint8_t *cart_ram;
	size_t save_size;
	uint8_t *bootrom;
	char rom_path[256];
	char save_path[256];
	uint16_t selected_palette[12];
	uint16_t fb[LCD_HEIGHT][LCD_WIDTH];
};

#endif /* DC_PRIV_H */
