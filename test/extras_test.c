#include "minctest.h"

#include "../extras/audio_processor/audio_processor.h"
#include "../extras/ini_kv/ini_kv.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

static void test_ini_kv_get_int(void)
{
	int value;

	lequal(ini_kv_get_int("volume=75", "volume", &value), 1);
	lequal(value, 75);
	lequal(ini_kv_get_int("volume=75", "mute", &value), 0);
	lequal(ini_kv_get_int("  volume = 42", "volume", &value), 1);
	lequal(value, 42);
}

static void test_ini_kv_get_string(void)
{
	char value[32];

	lequal(ini_kv_get_string("last_rom_path=/sd/roms/tetris.gb",
				 "last_rom_path", value, sizeof(value)),
	       1);
	lok(strcmp(value, "/sd/roms/tetris.gb") == 0);

	memset(value, 0, sizeof(value));
	lequal(ini_kv_get_string("last_rom_path=/pc/foo.gb  ",
				 "last_rom_path", value, sizeof(value)),
	       1);
	lok(strcmp(value, "/pc/foo.gb") == 0);
}

static void test_ini_kv_parse_bool(void)
{
	bool value;

	lequal(ini_kv_parse_bool("1", &value), 1);
	lok(value);
	lequal(ini_kv_parse_bool("true", &value), 1);
	lok(value);
	lequal(ini_kv_parse_bool("no", &value), 1);
	lok(!value);
	lequal(ini_kv_parse_bool("maybe", &value), 0);
}

static void test_ini_kv_fprint_roundtrip(void)
{
	char line[64];
	FILE *f;

	f = tmpfile();
	lok(f != NULL);
	ini_kv_fprint_int(f, "browser_filter", 2);
	ini_kv_fprint_bool(f, "muted", true);
	ini_kv_fprint_string(f, "last_rom_path", "/cd/roms/game.gbc");
	rewind(f);

	lok(fgets(line, sizeof(line), f) != NULL);
	{
		int value;

		lequal(ini_kv_get_int(line, "browser_filter", &value), 1);
		lequal(value, 2);
	}

	lok(fgets(line, sizeof(line), f) != NULL);
	{
		int value;

		lequal(ini_kv_get_int(line, "muted", &value), 1);
		lequal(value, 1);
	}

	lok(fgets(line, sizeof(line), f) != NULL);
	{
		char value[64];

		lequal(ini_kv_get_string(line, "last_rom_path", value, sizeof(value)),
		       1);
		lok(strcmp(value, "/cd/roms/game.gbc") == 0);
	}

	fclose(f);
}

static void test_audio_processor_volume(void)
{
	struct audio_processor proc;
	int16_t samples[2] = { 1000, -1000 };

	audio_processor_init(&proc);
	audio_processor_set_volume(&proc, 50);
	audio_processor_process_s16_stereo(&proc, samples, 1);

	lequal(samples[0], 500);
	lequal(samples[1], -500);
}

static void test_audio_processor_mute_fade(void)
{
	struct audio_processor proc;
	int16_t samples[2];
	unsigned int frame;

	audio_processor_init(&proc);
	samples[0] = 8000;
	samples[1] = -8000;
	audio_processor_set_muted(&proc, true);

	for (frame = 0; frame < AUDIO_PROCESSOR_FADE_SAMPLES; frame++) {
		samples[0] = 8000;
		samples[1] = -8000;
		audio_processor_process_s16_stereo(&proc, samples, 1);
	}

	lequal(samples[0], 0);
	lequal(samples[1], 0);
}

static void test_audio_processor_reset(void)
{
	struct audio_processor proc;
	int16_t samples[2] = { 0, 0 };

	audio_processor_init(&proc);
	proc.hp_state[0] = 5000;
	proc.hp_state[1] = -5000;
	audio_processor_process_s16_stereo(&proc, samples, 1);
	lok(samples[0] != 0 || samples[1] != 0);

	audio_processor_reset(&proc);
	samples[0] = 0;
	samples[1] = 0;
	audio_processor_process_s16_stereo(&proc, samples, 1);
	lequal(samples[0], 0);
	lequal(samples[1], 0);
}

int main(void)
{
	lrun("ini_kv_get_int", test_ini_kv_get_int);
	lrun("ini_kv_get_string", test_ini_kv_get_string);
	lrun("ini_kv_parse_bool", test_ini_kv_parse_bool);
	lrun("ini_kv_fprint_roundtrip", test_ini_kv_fprint_roundtrip);
	lrun("audio_processor_volume", test_audio_processor_volume);
	lrun("audio_processor_mute_fade", test_audio_processor_mute_fade);
	lrun("audio_processor_reset", test_audio_processor_reset);
	lresults();
	return lfails != 0;
}
