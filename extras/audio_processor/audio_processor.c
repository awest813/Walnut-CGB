/*
 * PocketDC / Walnut-CGB — lightweight stereo sample processor.
 * Copyright (c) 2025 Mr. Paul (https://github.com/Mr-PauI)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <limits.h>
#include <string.h>

#include "audio_processor.h"

#define AUDIO_PROCESSOR_HP_SHIFT 8

void audio_processor_init(struct audio_processor *proc)
{
	if (!proc)
		return;

	memset(proc, 0, sizeof(*proc));
	proc->volume = AUDIO_PROCESSOR_VOLUME_DEFAULT;
}

void audio_processor_reset(struct audio_processor *proc)
{
	if (!proc)
		return;

	proc->fade_remaining = 0;
	proc->fade_total = 0;
	proc->hp_state[0] = 0;
	proc->hp_state[1] = 0;
}

void audio_processor_set_volume(struct audio_processor *proc, uint8_t volume)
{
	if (!proc)
		return;

	if (volume > AUDIO_PROCESSOR_VOLUME_MAX)
		volume = AUDIO_PROCESSOR_VOLUME_MAX;

	proc->volume = volume;
}

void audio_processor_set_muted(struct audio_processor *proc, bool muted)
{
	if (!proc || proc->muted == muted)
		return;

	proc->muted = muted;
	proc->fade_total = AUDIO_PROCESSOR_FADE_SAMPLES;
	proc->fade_remaining = AUDIO_PROCESSOR_FADE_SAMPLES;
}

static int16_t audio_processor_clip_s16(int32_t sample)
{
	if (sample > INT16_MAX)
		return INT16_MAX;
	if (sample < INT16_MIN)
		return INT16_MIN;

	return (int16_t)sample;
}

static int16_t audio_processor_dc_block(int16_t sample, int32_t *state)
{
	const int32_t in = sample;
	const int32_t out = in - *state;

	*state = in - (out >> AUDIO_PROCESSOR_HP_SHIFT);
	return audio_processor_clip_s16(out);
}

static uint16_t audio_processor_mute_gain_q15(const struct audio_processor *proc)
{
	if (proc->fade_remaining == 0)
		return proc->muted ? 0 : 32768;

	if (proc->muted) {
		return (uint16_t)((uint32_t)proc->fade_remaining * 32768U /
				  proc->fade_total);
	}

	return (uint16_t)(((uint32_t)proc->fade_total - proc->fade_remaining) *
			  32768U / proc->fade_total);
}

void audio_processor_process_s16_stereo(struct audio_processor *proc,
					int16_t *interleaved,
					unsigned int frame_count)
{
	unsigned int frame;

	if (!proc || !interleaved || frame_count == 0)
		return;

	for (frame = 0; frame < frame_count; frame++) {
		const unsigned int left = frame * 2U;
		const unsigned int right = left + 1U;
		const uint16_t mute_gain = audio_processor_mute_gain_q15(proc);
		int32_t sample_l;
		int32_t sample_r;

		if (proc->fade_remaining > 0)
			proc->fade_remaining--;

		sample_l = audio_processor_dc_block(interleaved[left], &proc->hp_state[0]);
		sample_r = audio_processor_dc_block(interleaved[right], &proc->hp_state[1]);

		if (mute_gain == 0) {
			interleaved[left] = 0;
			interleaved[right] = 0;
			continue;
		}

		if (proc->volume == AUDIO_PROCESSOR_VOLUME_MAX && mute_gain == 32768) {
			interleaved[left] = (int16_t)sample_l;
			interleaved[right] = (int16_t)sample_r;
			continue;
		}

		{
			const uint32_t gain =
				(uint32_t)proc->volume * (uint32_t)mute_gain /
				AUDIO_PROCESSOR_VOLUME_MAX;

			interleaved[left] =
				audio_processor_clip_s16((sample_l * (int32_t)gain) / 32768);
			interleaved[right] =
				audio_processor_clip_s16((sample_r * (int32_t)gain) / 32768);
		}
	}
}
