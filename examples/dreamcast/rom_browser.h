/*
 * Walnut-CGB Dreamcast frontend — ROM and save file I/O.
 * Copyright (c) 2025 Mr. Paul (https://github.com/Mr-PauI)
 * Licensed under the MIT License.
 */

#ifndef DC_ROM_BROWSER_H
#define DC_ROM_BROWSER_H

#include <stddef.h>

#include "dc_priv.h"

int dc_rom_load(struct dc_priv *priv, const char *rom_path);
void dc_rom_unload(struct dc_priv *priv);
int dc_save_path_from_rom(const char *rom_path, char *save_path, size_t save_path_len);
int dc_cart_ram_read_file(const char *save_path, uint8_t **dest, size_t len);
int dc_cart_ram_write_file(const char *save_path, const uint8_t *data, size_t len);

#endif /* DC_ROM_BROWSER_H */
