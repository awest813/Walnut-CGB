/*
 * PocketDC / Walnut-CGB — interleaved S16 stereo ring buffer.
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

#ifndef AUDIO_RING_H
#define AUDIO_RING_H

#include <stdint.h>

/*
 * Interleaved 16-bit stereo frame buffer ([L0, R0, L1, R1, ...]). The producer
 * pushes a fixed-size block per emulated video frame; the consumer (audio
 * hardware callback) pops whatever the DMA needs. The two cadences never match
 * exactly, so the buffer absorbs jitter with a latency cushion, pads consumer
 * underruns with silence, and drops the oldest frames on overrun — counting
 * both so callers can monitor health. The caller owns the sample storage
 * (capacity * 2 int16_t).
 *
 * NOT thread-safe / not lock-free: on overrun the producer advances the
 * consumer's read index, so push() and pop() must not run concurrently. Both
 * frontends serialize them (the Dreamcast pops inside snd_stream_poll() on the
 * same thread that pushes; the SDL callback both pushes and pops on the audio
 * thread). Add external locking before sharing one ring across threads.
 */
struct audio_ring
{
	int16_t *buf;            /* caller-owned, holds capacity * 2 samples */
	unsigned int capacity;   /* usable size in stereo frames (one slot reserved) */
	unsigned int read;       /* frame index of next frame to pop */
	unsigned int write;      /* frame index of next frame to push */
	unsigned int target;     /* desired steady-state fill (latency cushion) */
	unsigned long underruns; /* frames of silence emitted on consumer starvation */
	unsigned long overruns;  /* frames discarded to make room for the producer */
};

/**
 * Bind a ring to caller-owned storage and prime it to the target cushion.
 *
 * \param buf        Storage for capacity * 2 interleaved samples.
 * \param capacity   Storage size in stereo frames (must be >= 2).
 * \param target     Latency cushion in frames; clamped to capacity / 2.
 */
void audio_ring_init(struct audio_ring *r, int16_t *buf, unsigned int capacity,
		     unsigned int target);

/** Discard all queued audio and re-prime the cushion with silence. */
void audio_ring_reset(struct audio_ring *r);

/** Number of stereo frames currently queued. */
unsigned int audio_ring_used(const struct audio_ring *r);

/** Number of stereo frames that can be pushed before the buffer is full. */
unsigned int audio_ring_free(const struct audio_ring *r);

/** Append `frames` of silence (used to establish the latency cushion). */
void audio_ring_prime(struct audio_ring *r, unsigned int frames);

/**
 * Push `frames` interleaved stereo frames, dropping the oldest queued frames
 * when the buffer cannot hold them (tracked in `overruns`).
 */
void audio_ring_push(struct audio_ring *r, const int16_t *src,
		     unsigned int frames);

/**
 * Pop up to `frames` interleaved stereo frames into `dst`, zero-padding any
 * shortfall so `dst` always receives exactly `frames` frames (the padded
 * frames are tracked in `underruns`).
 *
 * \return Number of real (non-silence) frames written, i.e. the prefix of
 *         `dst` that holds queued audio.
 */
unsigned int audio_ring_pop(struct audio_ring *r, int16_t *dst,
			    unsigned int frames);

#endif /* AUDIO_RING_H */
