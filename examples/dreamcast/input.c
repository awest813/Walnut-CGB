/*
 * Walnut-CGB Dreamcast frontend — Maple controller input.
 * Copyright (c) 2025 Mr. Paul (https://github.com/Mr-PauI)
 * Licensed under the MIT License.
 */

#include <kos.h>
#include <dc/maple/controller.h>

#include "input.h"

static maple_device_t *controller;
static uint32_t previous_buttons;

void dc_input_init(void)
{
	controller = maple_enum_type(0, MAPLE_FUNC_CONTROLLER);
	previous_buttons = 0xFFFF;
}

static uint8_t dc_buttons_to_joypad(uint32_t buttons)
{
	uint8_t joypad = 0xFF;

	if (buttons & CONT_A)
		joypad &= (uint8_t)~JOYPAD_A;
	if (buttons & CONT_B)
		joypad &= (uint8_t)~JOYPAD_B;
	if (buttons & CONT_START)
		joypad &= (uint8_t)~JOYPAD_START;
	if (buttons & CONT_X)
		joypad &= (uint8_t)~JOYPAD_SELECT;
	if (buttons & CONT_DPAD_UP)
		joypad &= (uint8_t)~JOYPAD_UP;
	if (buttons & CONT_DPAD_DOWN)
		joypad &= (uint8_t)~JOYPAD_DOWN;
	if (buttons & CONT_DPAD_LEFT)
		joypad &= (uint8_t)~JOYPAD_LEFT;
	if (buttons & CONT_DPAD_RIGHT)
		joypad &= (uint8_t)~JOYPAD_RIGHT;

	return joypad;
}

void dc_input_poll(struct dc_input_state *state, struct gb_s *gb)
{
	cont_state_t *pad;
	uint32_t buttons;

	state->reset_game = false;
	state->exit_requested = false;
	state->cycle_palette = false;
	state->toggle_frameskip = false;

	if (!controller)
		controller = maple_enum_type(0, MAPLE_FUNC_CONTROLLER);

	if (!controller) {
		gb->direct.joypad = 0xFF;
		state->joypad = 0xFF;
		state->fast_mode = 1;
		return;
	}

	pad = (cont_state_t *)maple_dev_status(controller);
	if (!pad) {
		gb->direct.joypad = 0xFF;
		state->joypad = 0xFF;
		state->fast_mode = 1;
		return;
	}

	buttons = pad->buttons;
	{
		const uint32_t changed = buttons ^ previous_buttons;

		if ((buttons & CONT_START) && (buttons & CONT_A) &&
		    (changed & CONT_A))
			state->reset_game = true;
		if ((buttons & CONT_START) && (buttons & CONT_B) &&
		    (changed & CONT_B))
			state->exit_requested = true;
		if ((buttons & CONT_Y) && (changed & CONT_Y))
			state->cycle_palette = true;
		if ((buttons & CONT_X) && (changed & CONT_X))
			state->toggle_frameskip = true;
	}

	state->joypad = dc_buttons_to_joypad(buttons);
	gb->direct.joypad = state->joypad;

	state->fast_mode = ((buttons & CONT_LTRIGGER) || (buttons & CONT_RTRIGGER)) ? 2 : 1;
	previous_buttons = buttons;
}
