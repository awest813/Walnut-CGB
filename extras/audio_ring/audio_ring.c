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

	/*
	 * One slot stays empty so write == read always means "empty". Cap the
	 * cushion at half the buffer so priming always leaves room to push;
	 * otherwise the very first push would overrun the just-primed cushion.
	 */
	if (r->capacity && target > r->capacity / 2)
		target = r->capacity / 2;
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

/* Frames from `write` to the end of the buffer before the index wraps. */
static unsigned int audio_ring_write_run(const struct audio_ring *r)
{
	return r->capacity - r->write;
}

/* Frames from `read` to the end of the buffer before the index wraps. */
static unsigned int audio_ring_read_run(const struct audio_ring *r)
{
	return r->capacity - r->read;
}

void audio_ring_prime(struct audio_ring *r, unsigned int frames)
{
	unsigned int first;

	if (!r || !r->capacity)
		return;

	if (frames > audio_ring_free(r))
		frames = audio_ring_free(r);

	/* Zero in at most two contiguous runs, wrapping once. */
	first = audio_ring_write_run(r);
	if (first > frames)
		first = frames;

	memset(&r->buf[r->write * 2], 0, (size_t)first * 2 * sizeof(int16_t));
	if (frames > first)
		memset(&r->buf[0], 0,
		       (size_t)(frames - first) * 2 * sizeof(int16_t));

	r->write = (r->write + frames) % r->capacity;
}

void audio_ring_push(struct audio_ring *r, const int16_t *src,
		     unsigned int frames)
{
	unsigned int room;
	unsigned int first;

	if (!r || !r->capacity || !src)
		return;

	/*
	 * The producer is authoritative: if the buffer cannot hold this block,
	 * drop the oldest queued frames rather than the freshest. A block that
	 * alone exceeds the buffer keeps only its newest capacity-1 frames.
	 */
	if (frames >= r->capacity) {
		const unsigned int keep = r->capacity - 1;

		r->overruns += frames - keep;
		src += (frames - keep) * 2;
		frames = keep;
	}

	room = audio_ring_free(r);
	if (frames > room) {
		const unsigned int drop = frames - room;

		r->overruns += drop;
		r->read = (r->read + drop) % r->capacity;
	}

	/* Copy in at most two contiguous runs, wrapping once. */
	first = audio_ring_write_run(r);
	if (first > frames)
		first = frames;

	memcpy(&r->buf[r->write * 2], src, (size_t)first * 2 * sizeof(int16_t));
	if (frames > first)
		memcpy(&r->buf[0], &src[first * 2],
		       (size_t)(frames - first) * 2 * sizeof(int16_t));

	r->write = (r->write + frames) % r->capacity;
}

unsigned int audio_ring_pop(struct audio_ring *r, int16_t *dst,
			    unsigned int frames)
{
	unsigned int popped = 0;

	if (!dst)
		return 0;

	if (r && r->capacity) {
		const unsigned int avail = audio_ring_used(r);
		unsigned int first;

		popped = (frames < avail) ? frames : avail;

		/* Copy in at most two contiguous runs, wrapping once. */
		first = audio_ring_read_run(r);
		if (first > popped)
			first = popped;

		memcpy(dst, &r->buf[r->read * 2],
		       (size_t)first * 2 * sizeof(int16_t));
		if (popped > first)
			memcpy(&dst[first * 2], &r->buf[0],
			       (size_t)(popped - first) * 2 * sizeof(int16_t));

		r->read = (r->read + popped) % r->capacity;

		if (popped < frames)
			r->underruns += frames - popped;
	}

	/* Silence-pad any shortfall so dst always holds `frames` frames. */
	if (popped < frames)
		memset(&dst[popped * 2], 0,
		       (size_t)(frames - popped) * 2 * sizeof(int16_t));

	return popped;
}
