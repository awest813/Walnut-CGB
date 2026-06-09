/*
 * Walnut-CGB Dreamcast frontend — Maple controller input.
 * Copyright (c) 2025 Mr. Paul (https://github.com/Mr-PauI)
 * Licensed under the MIT License.
 */

#ifndef DC_INPUT_H
#define DC_INPUT_H

#include <stdbool.h>

#include "../../walnut_cgb.h"

struct dc_input_state
{
	uint8_t joypad;
	bool reset_game;
	bool exit_requested;
	bool pause_requested;
	bool cycle_palette;
	bool toggle_frameskip;
	unsigned int fast_mode;
};

void dc_input_init(void);
void dc_input_poll(struct dc_input_state *state, struct gb_s *gb);

#endif /* DC_INPUT_H */
