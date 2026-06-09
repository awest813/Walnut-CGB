/*
 * Walnut-CGB Dreamcast frontend.
 * Copyright (c) 2025 Mr. Paul (https://github.com/Mr-PauI)
 * Licensed under the MIT License.
 */

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <kos.h>
#include <dc/video.h>

uint8_t audio_read(uint16_t addr);
void audio_write(uint16_t addr, uint8_t val);

#define WALNUT_GB_IS_LITTLE_ENDIAN 1
#define WALNUT_GB_32BIT_DMA 1
#define WALNUT_GB_16BIT_DMA 0
#define WALNUT_GB_32BIT_ALIGNED 1
#define WALNUT_GB_USE_INTRINSICS 1
#define ENABLE_LCD 1
#define ENABLE_SOUND 1
#define WALNUT_GB_12_COLOUR 1

#include "../../walnut_cgb.h"

#include "audio.h"
#include "dc_priv.h"
#include "input.h"
#include "palette.h"
#include "rom_browser.h"
#include "video.h"

KOS_INIT_FLAGS(INIT_DEFAULT | INIT_MALLOCSTATS);

/* Autosave cadence: roughly every 60 seconds of presented frames. */
#define DC_AUTOSAVE_INTERVAL_FRAMES ((int)(VERTICAL_SYNC * 60.0))

static struct dc_priv priv;
static unsigned int palette_selection = 3;

uint8_t gb_rom_read(struct gb_s *gb, const uint_fast32_t addr)
{
	const struct dc_priv *p = gb->direct.priv;

	return p->rom[addr];
}

uint16_t gb_rom_read_16bit(struct gb_s *gb, const uint_fast32_t addr)
{
	const uint8_t *src = &((const struct dc_priv *)gb->direct.priv)->rom[addr];

	if ((uintptr_t)src & 1)
		return (uint16_t)src[0] | ((uint16_t)src[1] << 8);

	return *(const uint16_t *)src;
}

uint32_t gb_rom_read_32bit(struct gb_s *gb, const uint_fast32_t addr)
{
	const uint8_t *src = &((const struct dc_priv *)gb->direct.priv)->rom[addr];

	if ((uintptr_t)src & 3) {
		return (uint32_t)src[0] |
		       ((uint32_t)src[1] << 8) |
		       ((uint32_t)src[2] << 16) |
		       ((uint32_t)src[3] << 24);
	}

	return *(const uint32_t *)src;
}

uint8_t gb_cart_ram_read(struct gb_s *gb, const uint_fast32_t addr)
{
	const struct dc_priv *p = gb->direct.priv;

	return p->cart_ram[addr];
}

void gb_cart_ram_write(struct gb_s *gb, const uint_fast32_t addr, const uint8_t val)
{
	struct dc_priv *p = gb->direct.priv;

	p->cart_ram[addr] = val;
}

uint8_t gb_bootrom_read(struct gb_s *gb, const uint_fast16_t addr)
{
	const struct dc_priv *p = gb->direct.priv;

	return p->bootrom[addr];
}

static inline uint16_t rgb565_to_rgb555(uint16_t c565)
{
	const uint16_t r5 = (c565 >> 11) & 0x1F;
	const uint16_t g6 = (c565 >> 5) & 0x3F;
	const uint16_t b5 = c565 & 0x1F;
	const uint16_t g5 = (uint16_t)((g6 * 31 + 31 / 2) / 63);

	return (uint16_t)((r5 << 10) | (g5 << 5) | b5);
}

void lcd_draw_line(struct gb_s *gb, const uint8_t pixels[160], const uint_fast8_t line)
{
	struct dc_priv *p = gb->direct.priv;
	uint16_t(*fb565)[LCD_WIDTH] = p->fb;

	if (gb->cgb.cgbMode) {
		unsigned int x;

		for (x = 0; x < LCD_WIDTH; x++)
			fb565[line][x] = rgb565_to_rgb555(gb->cgb.fixPalette[pixels[x]]);
	}
#if WALNUT_GB_12_COLOUR
	else {
		unsigned int x;

		for (x = 0; x < LCD_WIDTH; x++)
			fb565[line][x] =
				p->selected_palette[((pixels[x] & 18) >> 1) | (pixels[x] & 3)];
	}
#else
	else {
		unsigned int x;

		for (x = 0; x < LCD_WIDTH; x++)
			fb565[line][x] = p->selected_palette[pixels[x] & 3];
	}
#endif
}

#if ENABLE_SOUND
uint8_t audio_read(uint16_t addr)
{
	return dc_audio_read(addr);
}

void audio_write(uint16_t addr, uint8_t val)
{
	dc_audio_write(addr, val);
}
#endif

void gb_error(struct gb_s *gb, const enum gb_error_e gb_err, const uint16_t addr)
{
	struct dc_priv *p = gb->direct.priv;
	const char *gb_err_str[GB_INVALID_MAX] = {
		"UNKNOWN",
		"INVALID OPCODE",
		"INVALID READ",
		"INVALID WRITE",
		""
	};

	(void)addr;
	if (p->save_size > 0 && p->save_path[0] != '\0')
		dc_cart_ram_write_file(p->save_path, p->cart_ram, p->save_size);

	printf("walnut-dc: emulator error %s\n", gb_err_str[gb_err]);
	arch_exit();
}

static int dc_load_bootrom(struct dc_priv *p)
{
	FILE *f;
	long size;

	f = fopen("dmg_boot.bin", "rb");
	if (!f)
		return -1;

	if (fseek(f, 0, SEEK_END) != 0) {
		fclose(f);
		return -1;
	}

	size = ftell(f);
	if (size <= 0) {
		fclose(f);
		return -1;
	}

	rewind(f);
	p->bootrom = (uint8_t *)malloc((size_t)size);
	if (!p->bootrom) {
		fclose(f);
		return -1;
	}

	if (fread(p->bootrom, 1, (size_t)size, f) != (size_t)size) {
		free(p->bootrom);
		p->bootrom = NULL;
		fclose(f);
		return -1;
	}

	fclose(f);
	return 0;
}

static int dc_init_emulator(struct gb_s *gb, struct dc_priv *p)
{
	enum gb_init_error_e gb_ret;

	gb_ret = gb_init(gb, &gb_rom_read, &gb_rom_read_16bit, &gb_rom_read_32bit,
			 &gb_cart_ram_read, &gb_cart_ram_write, &gb_error, p);
	if (gb_ret != GB_INIT_NO_ERROR) {
		printf("walnut-dc: gb_init failed (%d)\n", gb_ret);
		return -1;
	}

	if (dc_load_bootrom(p) == 0) {
		gb_set_bootrom(gb, gb_bootrom_read);
		gb_reset(gb);
	}

	if (gb_get_save_size_s(gb, &p->save_size) < 0) {
		printf("walnut-dc: unable to determine save size\n");
		return -1;
	}

	if (p->save_size > 0)
		dc_cart_ram_read_file(p->save_path, &p->cart_ram, p->save_size);

	{
		time_t rawtime;
		struct tm timeinfo;

		time(&rawtime);
		localtime_r(&rawtime, &timeinfo);
		gb_set_rtc(gb, &timeinfo);
	}

	gb_init_lcd(gb, &lcd_draw_line);
	dc_auto_assign_palette(p, gb_colour_hash(gb));
	return 0;
}

static void dc_write_save(struct dc_priv *p)
{
	if (p->save_size > 0 && p->cart_ram && p->save_path[0] != '\0')
		dc_cart_ram_write_file(p->save_path, p->cart_ram, p->save_size);
}

static bool dc_run_game(const char *rom_path, const char *save_path,
			bool return_to_browser)
{
	struct gb_s gb;
	struct dc_input_state input;
	unsigned int fast_mode = 1;
	unsigned int fast_mode_timer = 1;
	int save_timer = DC_AUTOSAVE_INTERVAL_FRAMES;
	uint64_t target_ticks;
	bool running = true;

	memset(&priv, 0, sizeof(priv));
	if (dc_rom_load(&priv, rom_path) != 0)
		return return_to_browser;

	if (save_path && save_path[0] != '\0')
		strncpy(priv.save_path, save_path, sizeof(priv.save_path) - 1);

	if (dc_init_emulator(&gb, &priv) != 0) {
		dc_rom_unload(&priv);
		return return_to_browser;
	}

	{
		char title[28] = "Walnut-DC: ";

		gb_get_rom_name(&gb, title + 11);
		printf("%s\n", title);
	}

	target_ticks = (uint64_t)(1000.0 / VERTICAL_SYNC);

	while (running) {
		uint64_t now = timer_ms_gettime64();

		dc_input_poll(&input, &gb);

		if (input.reset_game)
			gb_reset(&gb);
		if (input.cycle_palette) {
			palette_selection++;
			dc_manual_assign_palette(&priv, (uint8_t)palette_selection);
		}
		if (input.toggle_frameskip)
			gb.direct.frame_skip = !gb.direct.frame_skip;
		if (input.exit_requested) {
			running = false;
			break;
		}

		fast_mode = input.fast_mode;
		gb_run_frame_dualfetch(&gb);

#if ENABLE_SOUND
		if (dc_audio_ready())
			dc_audio_frame();
#endif

		if (fast_mode_timer > 1) {
			fast_mode_timer--;
			continue;
		}

		fast_mode_timer = fast_mode;
		dc_video_present(&priv);

		if (priv.save_size > 0 && --save_timer <= 0) {
			dc_write_save(&priv);
			save_timer = DC_AUTOSAVE_INTERVAL_FRAMES;
		}

		{
			const uint64_t elapsed = timer_ms_gettime64() - now;

			if (elapsed < target_ticks)
				timer_spin((int)(target_ticks - elapsed));
		}
	}

	dc_write_save(&priv);
	dc_rom_unload(&priv);

	if (return_to_browser && input.exit_requested)
		return true;

	return !input.exit_requested;
}

int main(int argc, char **argv)
{
	struct dc_browser browser;
	char selected_rom[256];

	vid_set_mode(DM_640x480, PM_RGB555);
	vid_clear(0, 0, 0);

	if (dc_video_init() != 0) {
		printf("walnut-dc: video init failed\n");
		return EXIT_FAILURE;
	}

#if ENABLE_SOUND
	if (dc_audio_init() != 0)
		printf("walnut-dc: audio init failed, continuing without sound\n");
#endif

	dc_input_init();
	dc_browser_init(&browser);

	if (argc >= 2) {
		const char *save_path = (argc >= 3) ? argv[2] : NULL;

		if (!dc_run_game(argv[1], save_path, false))
			goto shutdown;
	} else {
		while (1) {
			if (!dc_browser_run(&browser, selected_rom, sizeof(selected_rom)))
				break;
			if (!dc_run_game(selected_rom, NULL, true))
				break;
		}
	}

shutdown:
	dc_video_shutdown();
#if ENABLE_SOUND
	dc_audio_shutdown();
#endif

	return EXIT_SUCCESS;
}
