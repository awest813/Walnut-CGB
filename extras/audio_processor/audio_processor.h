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

#ifndef AUDIO_PROCESSOR_H
#define AUDIO_PROCESSOR_H

#include <stdbool.h>
#include <stdint.h>

#define AUDIO_PROCESSOR_VOLUME_MIN 0
#define AUDIO_PROCESSOR_VOLUME_MAX 100
#define AUDIO_PROCESSOR_VOLUME_DEFAULT 100

struct audio_processor
{
	uint8_t volume;
	bool muted;
};

void audio_processor_init(struct audio_processor *proc);
void audio_processor_set_volume(struct audio_processor *proc, uint8_t volume);
void audio_processor_set_muted(struct audio_processor *proc, bool muted);

/**
 * Apply master volume and mute to interleaved stereo S16 samples in place.
 *
 * \param interleaved  L/R pairs: [L0, R0, L1, R1, ...]
 * \param frame_count  Number of stereo frames (not individual samples).
 */
void audio_processor_process_s16_stereo(struct audio_processor *proc,
					int16_t *interleaved,
					unsigned int frame_count);

#endif /* AUDIO_PROCESSOR_H */
