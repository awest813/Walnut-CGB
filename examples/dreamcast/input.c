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

int dc_input_axis(bool dpad_negative, bool dpad_positive, int16_t analog,
		  int threshold)
{
	if (dpad_negative && !dpad_positive)
		return -1;
	if (dpad_positive && !dpad_negative)
		return 1;
	if (analog < -threshold)
		return -1;
	if (analog > threshold)
		return 1;

	return 0;
}

bool dc_input_repeat(bool pressed, int *timer)
{
	if (!pressed) {
		*timer = 0;
		return false;
	}

	if (*timer <= 0) {
		*timer = DC_INPUT_REPEAT_DELAY;
		return true;
	}

	if (--(*timer) == 0) {
		*timer = DC_INPUT_REPEAT_RATE;
		return true;
	}

	return false;
}

int dc_input_repeat_axis(int axis, int *timer, int *last_axis)
{
	if (axis == 0) {
		*timer = 0;
		*last_axis = 0;
		return 0;
	}

	if (axis != *last_axis) {
		*timer = 0;
		*last_axis = axis;
	}

	if (!dc_input_repeat(true, timer))
		return 0;

	return axis;
}

int dc_input_axis_edge(int axis, int *last_axis)
{
	if (axis == 0) {
		*last_axis = 0;
		return 0;
	}

	if (axis == *last_axis)
		return 0;

	*last_axis = axis;
	return axis;
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
	state->pause_requested = false;
	state->cycle_palette = false;
	state->toggle_frameskip = false;
	state->cycle_scale = false;

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
		if ((buttons & CONT_START) && (buttons & CONT_Y) &&
		    (changed & CONT_Y))
			state->pause_requested = true;
		if ((buttons & CONT_Y) && (changed & CONT_Y) &&
		    !(buttons & CONT_START))
			state->cycle_palette = true;
		/*
		 * X alone is Game Boy Select, so gate the frameskip toggle
		 * behind Start+X to avoid flipping it on every menu/map press.
		 */
		if ((buttons & CONT_START) && (buttons & CONT_X) &&
		    (changed & CONT_X))
			state->toggle_frameskip = true;
		if ((buttons & CONT_START) && (buttons & CONT_LTRIGGER) &&
		    (changed & CONT_LTRIGGER))
			state->cycle_scale = true;
	}

	state->joypad = dc_buttons_to_joypad(buttons);
	gb->direct.joypad = state->joypad;

	state->fast_mode = ((buttons & CONT_LTRIGGER) || (buttons & CONT_RTRIGGER)) ? 2 : 1;
	previous_buttons = buttons;
}
