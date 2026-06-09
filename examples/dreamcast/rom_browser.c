/*
 * Walnut-CGB Dreamcast frontend — ROM and save file I/O.
 * Copyright (c) 2025 Mr. Paul (https://github.com/Mr-PauI)
 * Licensed under the MIT License.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "rom_browser.h"

int dc_save_path_from_rom(const char *rom_path, char *save_path, size_t save_path_len)
{
	const char *dot;
	size_t base_len;

	if (!rom_path || !save_path || save_path_len == 0)
		return -1;

	dot = strrchr(rom_path, '.');
	if (!dot || dot == rom_path)
		base_len = strlen(rom_path);
	else
		base_len = (size_t)(dot - rom_path);

	if (base_len + 4 + 1 > save_path_len)
		return -1;

	memcpy(save_path, rom_path, base_len);
	memcpy(save_path + base_len, ".sav", 5);
	return 0;
}

int dc_rom_load(struct dc_priv *priv, const char *rom_path)
{
	FILE *f;
	long size;

	if (!priv || !rom_path)
		return -1;

	f = fopen(rom_path, "rb");
	if (!f) {
		printf("walnut-dc: unable to open ROM '%s'\n", rom_path);
		return -1;
	}

	if (fseek(f, 0, SEEK_END) != 0) {
		fclose(f);
		return -1;
	}

	size = ftell(f);
	if (size <= 0 || size > (8 * 1024 * 1024)) {
		fclose(f);
		printf("walnut-dc: invalid ROM size (%ld)\n", size);
		return -1;
	}

	rewind(f);
	priv->rom = (uint8_t *)malloc((size_t)size);
	if (!priv->rom) {
		fclose(f);
		return -1;
	}

	if (fread(priv->rom, 1, (size_t)size, f) != (size_t)size) {
		free(priv->rom);
		priv->rom = NULL;
		fclose(f);
		return -1;
	}

	fclose(f);
	strncpy(priv->rom_path, rom_path, sizeof(priv->rom_path) - 1);
	priv->rom_path[sizeof(priv->rom_path) - 1] = '\0';

	if (dc_save_path_from_rom(rom_path, priv->save_path, sizeof(priv->save_path)) != 0)
		priv->save_path[0] = '\0';

	return 0;
}

void dc_rom_unload(struct dc_priv *priv)
{
	if (!priv)
		return;

	free(priv->rom);
	priv->rom = NULL;
	free(priv->cart_ram);
	priv->cart_ram = NULL;
	free(priv->bootrom);
	priv->bootrom = NULL;
	priv->save_size = 0;
	priv->rom_path[0] = '\0';
	priv->save_path[0] = '\0';
}

int dc_cart_ram_read_file(const char *save_path, uint8_t **dest, size_t len)
{
	FILE *f;

	if (len == 0) {
		*dest = NULL;
		return 0;
	}

	*dest = (uint8_t *)calloc(1, len);
	if (!*dest)
		return -1;

	f = fopen(save_path, "rb");
	if (!f)
		return 0;

	if (fread(*dest, 1, len, f) != len)
		memset(*dest, 0, len);

	fclose(f);
	return 0;
}

int dc_cart_ram_write_file(const char *save_path, const uint8_t *data, size_t len)
{
	FILE *f;

	if (!save_path || !data || len == 0)
		return 0;

	f = fopen(save_path, "wb");
	if (!f) {
		printf("walnut-dc: unable to write save '%s'\n", save_path);
		return -1;
	}

	if (fwrite(data, 1, len, f) != len) {
		fclose(f);
		return -1;
	}

	fclose(f);
	return 0;
}
