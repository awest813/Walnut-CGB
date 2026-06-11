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

void audio_processor_init(struct audio_processor *proc)
{
	if (!proc)
		return;

	proc->volume = AUDIO_PROCESSOR_VOLUME_DEFAULT;
	proc->muted = false;
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
	if (!proc)
		return;

	proc->muted = muted;
}

static int16_t audio_processor_clip_s16(int32_t sample)
{
	if (sample > INT16_MAX)
		return INT16_MAX;
	if (sample < INT16_MIN)
		return INT16_MIN;

	return (int16_t)sample;
}

void audio_processor_process_s16_stereo(struct audio_processor *proc,
					int16_t *interleaved,
					unsigned int frame_count)
{
	unsigned int i;
	unsigned int sample_count;

	if (!proc || !interleaved || frame_count == 0)
		return;

	sample_count = frame_count * 2U;

	if (proc->muted || proc->volume == 0) {
		memset(interleaved, 0, sample_count * sizeof(int16_t));
		return;
	}

	if (proc->volume == AUDIO_PROCESSOR_VOLUME_MAX)
		return;

	for (i = 0; i < sample_count; i++) {
		const int32_t scaled =
			((int32_t)interleaved[i] * (int32_t)proc->volume) /
			AUDIO_PROCESSOR_VOLUME_MAX;

		interleaved[i] = audio_processor_clip_s16(scaled);
	}
}
