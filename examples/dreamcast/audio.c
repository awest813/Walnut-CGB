/*
 * Walnut-CGB Dreamcast frontend — AICA audio via snd_stream.
 * Copyright (c) 2025 Mr. Paul (https://github.com/Mr-PauI)
 * Licensed under the MIT License.
 */

#include <string.h>

#include <kos.h>
#include <dc/sound/stream.h>

#define AUDIO_SAMPLE_RATE 44100
#define MINIGB_APU_AUDIO_FORMAT_S16SYS
#include "../sdl2/minigb_apu/minigb_apu.h"

#define DC_AUDIO_RING_SAMPLES 8192

static struct minigb_apu_ctx apu;
static snd_stream_hnd_t stream;
static int16_t ring[DC_AUDIO_RING_SAMPLES * 2];
static unsigned int ring_read;
static unsigned int ring_write;
static int16_t stream_buf[4096 * 2];

static unsigned int dc_audio_ring_used(void)
{
	if (ring_write >= ring_read)
		return ring_write - ring_read;
	return DC_AUDIO_RING_SAMPLES - ring_read + ring_write;
}

static unsigned int dc_audio_ring_free(void)
{
	return DC_AUDIO_RING_SAMPLES - dc_audio_ring_used() - 1;
}

static void dc_audio_ring_push(const int16_t *samples, unsigned int count)
{
	unsigned int i;

	for (i = 0; i < count; i++) {
		ring[ring_write * 2] = samples[i * 2];
		ring[ring_write * 2 + 1] = samples[i * 2 + 1];
		ring_write = (ring_write + 1) % DC_AUDIO_RING_SAMPLES;
	}
}

static unsigned int dc_audio_ring_pop(int16_t *dst, unsigned int count)
{
	unsigned int i;
	unsigned int popped = 0;

	for (i = 0; i < count && ring_read != ring_write; i++) {
		dst[i * 2] = ring[ring_read * 2];
		dst[i * 2 + 1] = ring[ring_read * 2 + 1];
		ring_read = (ring_read + 1) % DC_AUDIO_RING_SAMPLES;
		popped++;
	}

	return popped;
}

static void *dc_audio_stream_callback(snd_stream_hnd_t hnd, int smp_req, int *smp_recv)
{
	const int frames_requested = smp_req / 2;
	int frames_written = 0;
	(void)hnd;

	memset(stream_buf, 0, sizeof(stream_buf));
	frames_written = (int)dc_audio_ring_pop(stream_buf, (unsigned int)frames_requested);
	*smp_recv = frames_written * 2;
	return stream_buf;
}

int dc_audio_init(void)
{
	ring_read = 0;
	ring_write = 0;
	minigb_apu_audio_init(&apu);

	snd_stream_init();
	stream = snd_stream_alloc(dc_audio_stream_callback, 0x4000);
	if (stream == SND_STREAM_INVALID)
		return -1;

	snd_stream_start(stream, AUDIO_SAMPLE_RATE, 1);
	return 0;
}

void dc_audio_shutdown(void)
{
	if (stream != SND_STREAM_INVALID) {
		snd_stream_stop(stream);
		snd_stream_destroy(stream);
		stream = SND_STREAM_INVALID;
	}
}

void dc_audio_frame(void)
{
	int16_t frame_buf[AUDIO_SAMPLES_TOTAL];
	unsigned int samples = AUDIO_SAMPLES;

	if (dc_audio_ring_free() < samples)
		return;

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
