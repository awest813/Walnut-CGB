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

#include <string.h>

#include "audio_ring.h"

void audio_ring_init(struct audio_ring *r, int16_t *buf, unsigned int capacity,
		     unsigned int target)
{
	if (!r)
		return;

	r->buf = buf;
	r->capacity = (buf && capacity >= 2) ? capacity : 0;
	r->read = 0;
	r->write = 0;
	r->underruns = 0;
	r->overruns = 0;

	/* One slot stays empty so write == read always means "empty". */
	if (r->capacity && target > r->capacity - 1)
		target = r->capacity - 1;
	r->target = target;

	audio_ring_prime(r, r->target);
}

unsigned int audio_ring_used(const struct audio_ring *r)
{
	if (!r || !r->capacity)
		return 0;

	if (r->write >= r->read)
		return r->write - r->read;

	return r->capacity - r->read + r->write;
}

unsigned int audio_ring_free(const struct audio_ring *r)
{
	if (!r || !r->capacity)
		return 0;

	return r->capacity - audio_ring_used(r) - 1;
}

void audio_ring_reset(struct audio_ring *r)
{
	if (!r)
		return;

	r->read = 0;
	r->write = 0;
	audio_ring_prime(r, r->target);
}

void audio_ring_prime(struct audio_ring *r, unsigned int frames)
{
	unsigned int i;

	if (!r || !r->capacity)
		return;

	if (frames > audio_ring_free(r))
		frames = audio_ring_free(r);

	for (i = 0; i < frames; i++) {
		r->buf[r->write * 2] = 0;
		r->buf[r->write * 2 + 1] = 0;
		r->write = (r->write + 1) % r->capacity;
	}
}

void audio_ring_push(struct audio_ring *r, const int16_t *src,
		     unsigned int frames)
{
	unsigned int i;

	if (!r || !r->capacity || !src)
		return;

	/*
	 * The producer is authoritative: if the buffer cannot hold this block,
	 * drop the oldest queued frames rather than the freshest. Dropping at
	 * most a block keeps the discontinuity short and the cushion intact.
	 */
	if (frames >= r->capacity) {
		/* Block alone exceeds the buffer; keep only its newest tail. */
		const unsigned int keep = r->capacity - 1;

		r->overruns += frames - keep;
		src += (frames - keep) * 2;
		frames = keep;
		r->read = 0;
		r->write = 0;
	} else {
		unsigned int room = audio_ring_free(r);

		if (frames > room) {
			const unsigned int drop = frames - room;

			r->overruns += drop;
			r->read = (r->read + drop) % r->capacity;
		}
	}

	for (i = 0; i < frames; i++) {
		r->buf[r->write * 2] = src[i * 2];
		r->buf[r->write * 2 + 1] = src[i * 2 + 1];
		r->write = (r->write + 1) % r->capacity;
	}
}

unsigned int audio_ring_pop(struct audio_ring *r, int16_t *dst,
			    unsigned int frames)
{
	unsigned int popped = 0;

	if (!dst)
		return 0;

	if (r && r->capacity) {
		while (popped < frames && r->read != r->write) {
			dst[popped * 2] = r->buf[r->read * 2];
			dst[popped * 2 + 1] = r->buf[r->read * 2 + 1];
			r->read = (r->read + 1) % r->capacity;
			popped++;
		}

		if (popped < frames)
			r->underruns += frames - popped;
	}

	if (popped < frames)
		memset(&dst[popped * 2], 0,
		       (size_t)(frames - popped) * 2 * sizeof(int16_t));

	return popped;
}
