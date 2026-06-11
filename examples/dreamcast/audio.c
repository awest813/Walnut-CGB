/*
 * Walnut-CGB Dreamcast frontend — AICA audio via snd_stream.
 * Copyright (c) 2025 Mr. Paul (https://github.com/Mr-PauI)
 * Licensed under the MIT License.
 */

#include <stdbool.h>
#include <string.h>

#include <kos.h>
#include <dc/sound/stream.h>

#include "../../extras/audio_processor/audio_processor.h"

#define AUDIO_SAMPLE_RATE 44100
#define MINIGB_APU_AUDIO_FORMAT_S16SYS
#include "../sdl2/minigb_apu/minigb_apu.h"

#define DC_AUDIO_RING_MAX_SAMPLES 32768
#define DC_AUDIO_STEREO_FRAME_BYTES (sizeof(int16_t) * AUDIO_CHANNELS)

static struct minigb_apu_ctx apu;
static snd_stream_hnd_t stream = SND_STREAM_INVALID;
static bool audio_active;
static int16_t ring[DC_AUDIO_RING_MAX_SAMPLES * 2];
static unsigned int ring_capacity = 16384;
static unsigned int ring_read;
static unsigned int ring_write;
static struct audio_processor processor;
static int16_t stream_buf[4096 * 2] __attribute__((aligned(32)));

static unsigned int dc_audio_ring_capacity_for_mode(enum dc_audio_buffer_mode mode)
{
	switch (mode) {
	case DC_AUDIO_BUFFER_LOW:
		return 4096;
	case DC_AUDIO_BUFFER_HIGH:
		return DC_AUDIO_RING_MAX_SAMPLES;
	case DC_AUDIO_BUFFER_NORMAL:
	default:
		return 16384;
	}
}

static unsigned int dc_audio_ring_used(void)
{
	if (ring_write >= ring_read)
		return ring_write - ring_read;
	return ring_capacity - ring_read + ring_write;
}

static unsigned int dc_audio_ring_free(void)
{
	return ring_capacity - dc_audio_ring_used() - 1;
}

static void dc_audio_ring_reset(void)
{
	ring_read = 0;
	ring_write = 0;
}

static void dc_audio_ring_push(const int16_t *samples, unsigned int count)
{
	unsigned int i;

	for (i = 0; i < count; i++) {
		ring[ring_write * 2] = samples[i * 2];
		ring[ring_write * 2 + 1] = samples[i * 2 + 1];
		ring_write = (ring_write + 1) % ring_capacity;
	}
}

static unsigned int dc_audio_ring_pop(int16_t *dst, unsigned int count)
{
	unsigned int i;
	unsigned int popped = 0;

	for (i = 0; i < count && ring_read != ring_write; i++) {
		dst[i * 2] = ring[ring_read * 2];
		dst[i * 2 + 1] = ring[ring_read * 2 + 1];
		ring_read = (ring_read + 1) % ring_capacity;
		popped++;
	}

	audio_processor_process_s16_stereo(&processor, dst, popped);
	return popped;
}

static void dc_audio_ring_make_room(unsigned int frames)
{
	while (dc_audio_ring_free() < frames && ring_read != ring_write)
		ring_read = (ring_read + 1) % ring_capacity;
}

static void *dc_audio_stream_callback(snd_stream_hnd_t hnd, int smp_req, int *smp_recv)
{
	const unsigned int frames_requested = (unsigned int)smp_req / DC_AUDIO_STEREO_FRAME_BYTES;
	unsigned int frames_written;

	(void)hnd;

	memset(stream_buf, 0, sizeof(stream_buf));
	frames_written = dc_audio_ring_pop(stream_buf, frames_requested);
	*smp_recv = (int)(frames_written * DC_AUDIO_STEREO_FRAME_BYTES);
	return stream_buf;
}

int dc_audio_init(void)
{
	dc_audio_ring_reset();
	audio_active = false;
	audio_processor_init(&processor);
	minigb_apu_audio_init(&apu);

	snd_stream_init();
	stream = snd_stream_alloc(dc_audio_stream_callback, 0x4000);
	if (stream == SND_STREAM_INVALID)
		return -1;

	snd_stream_start(stream, AUDIO_SAMPLE_RATE, 1);
	audio_active = true;
	return 0;
}

void dc_audio_shutdown(void)
{
	audio_active = false;

	if (stream != SND_STREAM_INVALID) {
		snd_stream_stop(stream);
		snd_stream_destroy(stream);
		stream = SND_STREAM_INVALID;
	}
}

void dc_audio_configure(uint8_t volume, bool muted,
			enum dc_audio_buffer_mode buffer_mode)
{
	const unsigned int new_capacity =
		dc_audio_ring_capacity_for_mode(buffer_mode);

	audio_processor_set_volume(&processor, volume);
	audio_processor_set_muted(&processor, muted);

	if (new_capacity != ring_capacity) {
		ring_capacity = new_capacity;
		dc_audio_ring_reset();
	}
}

bool dc_audio_ready(void)
{
	return audio_active;
}

void dc_audio_frame(void)
{
	int16_t frame_buf[AUDIO_SAMPLES_TOTAL];
	const unsigned int samples = AUDIO_SAMPLES;

	if (!audio_active || stream == SND_STREAM_INVALID)
		return;

	dc_audio_ring_make_room(samples);
	minigb_apu_audio_callback(&apu, frame_buf);
	dc_audio_ring_push(frame_buf, samples);
	snd_stream_poll(stream);
}

uint8_t dc_audio_read(uint16_t addr)
{
	return minigb_apu_audio_read(&apu, addr);
}

void dc_audio_write(uint16_t addr, uint8_t val)
{
	minigb_apu_audio_write(&apu, addr, val);
}
