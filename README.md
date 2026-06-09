# Walnut-CGB

<img width="128" height="128" alt="WalnutCGB" src="https://github.com/user-attachments/assets/77e4290b-858b-4f64-bf6a-69d56f76c8e1" align="right" />

Single-header Game Boy / Game Boy Color emulator library, derived from the portable [Peanut-GB](https://github.com/deltabeard/Peanut-GB) project.

Walnut-CGB reimplements the CPU core for native 16-bit and 32-bit operations using a [dual-fetch chained dispatch model](https://github.com/awest813/Walnut-CGB/wiki/CPU-opcode-dispatch-model). It includes DMG updates from Peanut-GB mainline, integrated CGB support, and additional bug fixes — some of which are submitted upstream on the [upstream-changes](https://github.com/awest813/Walnut-CGB/tree/upstream-changes) branch.

**Current version:** 1.2.3

## Features

- Game Boy (DMG) and Game Boy Color (CGB) support
- Cartridge types: MBC1, MBC2, MBC3, MBC5
- Real-time clock (RTC) and serial link support
- Optional boot ROM execution
- Per-layer DMG palettes (3 layers × 4 shades = 12 colors)
- Frame skip and interlaced rendering modes
- LCD and sound can be disabled at compile time
- [Super Game Boy and Game Boy Color](https://github.com/awest813/Walnut-CGB/tree/master#additional-resources) 24-bit RGB palette databases
- Example frontends for desktop, embedded, and Dreamcast targets

Sound emulation is not built in. When `ENABLE_SOUND` is set, an external APU library is required — [MiniGB APU](examples/sdl2/minigb_apu) is included with the SDL2 and Dreamcast examples.

## Examples

| Platform | Directory | Description |
|----------|-----------|-------------|
| SDL2 | [`examples/sdl2`](examples/sdl2) | Desktop reference frontend with drag-and-drop ROM loading |
| Arduino | [`examples/arduino`](examples/arduino) | M5Stack Cardputer (ESP32-S3) demo |
| Dreamcast | [`examples/dreamcast`](examples/dreamcast) | KallistiOS frontend with ROM browser and disc packaging |

### SDL2

The flagship example is [`walnut_sdl.c`](examples/sdl2/walnut_sdl.c). Build with `cmake` or `make` in `examples/sdl2/`.

```bash
walnut-sdl                  # drop-zone window — drag and drop a ROM
walnut-sdl game.gb          # loads game.gb, creates game.sav if needed
walnut-sdl game.gb save.sav # explicit save file path
```

#### Controls

| Action | Keyboard | Gamepad |
|--------|----------|---------|
| A | z | A |
| B | x | B |
| Start | Return | START |
| Select | Backspace | BACK |
| D-Pad | Arrow keys | DPAD |
| Repeat A / B | a / s | |
| Normal speed | 1 | |
| Turbo ×2 (hold) | Space | |
| Turbo ×2 / ×3 / ×4 (toggle) | 2 / 3 / 4 | |
| Reset | r | |
| Change / reset palette | p / Shift+p | |
| Fullscreen | F11 / f | |
| Frameskip (toggle) | o | |
| Interlace (toggle) | i | |
| Dump BMP (toggle) | b | |

Frameskip and interlace are off by default. Frameskip toggles between 60 FPS and 30 FPS. Pressing `b` dumps each frame as a 24-bit bitmap in the current directory — see [`screencaps/README.md`](screencaps/README.md).

#### Screenshots

![Pokemon Blue](/screencaps/PKMN_BLUE.gif)
![Legend of Zelda: Link's Awakening](/screencaps/ZELDA.gif)
![Megaman V](/screencaps/MEGAMANV.png)
![Shantae](/screencaps/SHANTAE.png)
![Dragon Ball Z](/screencaps/DRAGONBALL_BBZP.png)

Animated GIFs are capped at 50 FPS by common decoders; emulation runs at native ~60 FPS.

#### Cardputer recordings

![Mario Deluxe - Cardputer](/screencaps/MarioDeluxe_Cardputer.gif)
![Zelda: Oracle of Seasons - Cardputer](/screencaps/Zelda_OracleSeasons_Cardputer.gif)

Recordings above were captured on a full implementation with paging. The [Arduino example](examples/arduino) does not include paging and is limited to available RAM.

### Arduino

Basic [M5Stack Cardputer](examples/arduino/arduino.ino) implementation demonstrating DMG/CGB palette output, a simple ROM browser, and display/input handling.

- No paging — only available RAM is used, so ROM size is very limited
- Internal flash storage works on ESP32/ESP32-S3 when alignment rules are followed
- ISO C memory function alternatives are noted in comments for other platforms

See [`examples/arduino/README.md`](examples/arduino/README.md) for details.

### Dreamcast

[KallistiOS](https://github.com/KallistiOS/KallistiOS) frontend with PVR video output, Maple controller input, ROM browser, and optional boot-disc packaging.

```bash
source /opt/toolchains/dc/kos/environ.sh   # adjust path to your KOS install
cd examples/dreamcast
make
dc-tool -x walnut-dc.elf                   # ROM browser
dc-tool -x walnut-dc.elf /pc/roms/tetris.gb
```

See [`examples/dreamcast/README.md`](examples/dreamcast/README.md) for controls, disc building, and port status.

## Migrating from Peanut-GB

Walnut-CGB is mostly a drop-in replacement. API differences are documented in the wiki:

- [Migrating from Peanut-GB](https://github.com/awest813/Walnut-CGB/wiki/Migrating-from-Peanut%E2%80%90GB)
- [Wiki home](https://github.com/awest813/Walnut-CGB/wiki) — implementation details, execution models, framebuffer handling, and code examples

## Integration

Include `walnut_cgb.h` and provide the required callbacks before calling [`gb_init`](https://github.com/awest813/Walnut-CGB/wiki/gb_init()).

### Required callbacks

| Callback | Purpose |
|----------|---------|
| [`gb_rom_read`](https://github.com/awest813/Walnut-CGB/wiki/gb_rom_read()) | 8-bit ROM read |
| [`gb_rom_read_16bit`](https://github.com/awest813/Walnut-CGB/wiki/gb_rom_read_16bit()) | 16-bit ROM read (dual-fetch / DMA) |
| [`gb_rom_read_32bit`](https://github.com/awest813/Walnut-CGB/wiki/gb_rom_read_32bit()) | 32-bit ROM read (32-bit DMA) |
| `gb_cart_ram_read` | Cart RAM read |
| `gb_cart_ram_write` | Cart RAM write |
| `gb_error` | Error handler |

### Frame execution

Both functions advance the emulator by one frame and can be swapped at runtime:

| Function | Model |
|----------|-------|
| [`gb_run_frame_dualfetch`](https://github.com/awest813/Walnut-CGB/wiki/gb_run_frame_dualfetch()) | 16-bit dual-fetch dispatch (**recommended**) |
| [`gb_run_frame`](https://github.com/awest813/Walnut-CGB/wiki/gb_run_frame()) | Original 8-bit single-instruction dispatch |

### Optional callbacks

| Callback | Enable with | Notes |
|----------|-------------|-------|
| [`lcd_draw_line`](https://github.com/awest813/Walnut-CGB/wiki/lcd_draw_line-example-%7C-Rendering-the-frame-for-DMG-and-CGB) | `ENABLE_LCD` (default 1), set via `gb_init_lcd` | Required for video output |
| `audio_read` / `audio_write` | `ENABLE_SOUND` (default 1) | Requires external APU |
| `gb_serial_tx` / `gb_serial_rx` | Set via `gb_init_serial` | Link cable; omitted = no cable connected |

**DMG pixel format:** bits 0–1 are shade; bits 4–5 are layer (OBJ0, OBJ1, BG).

**CGB pixel format:** indexes into the current palette.

### Useful functions

| Function | Description |
|----------|-------------|
| [`gb_reset`](https://github.com/awest813/Walnut-CGB/wiki/gb_reset()) | Power-cycle reset (also called by `gb_init`) |
| [`gb_get_save_size`](https://github.com/awest813/Walnut-CGB/wiki/gb_get_save_size_s()) | Cart RAM save size (0 if none) |
| [`gb_colour_hash`](https://github.com/awest813/Walnut-CGB/wiki/gb_colour_hash()) | Title hash for DMG auto-color (same algorithm as CGB hardware) |
| [`gb_get_rom_name`](https://github.com/awest813/Walnut-CGB/wiki/gb_get_rom_name()) | Game title from ROM header |
| [`gb_set_rtc`](https://github.com/awest813/Walnut-CGB/wiki/gb_set_rtc()) | Set RTC time for MBC3 games |
| [`gb_set_bootrom`](https://github.com/awest813/Walnut-CGB/wiki/gb_set_bootrom()) | Use DMG, MGB, or CGB boot ROM (call after `gb_init`, then reset) |

`gb_tick_rtc` is deprecated — RTC is ticked internally.

Full prototypes are at the bottom of [`walnut_cgb.h`](walnut_cgb.h).

### Compile-time macros

Set to `1` before including `walnut_cgb.h` to enable a feature.

| Macro | Description |
|-------|-------------|
| `WALNUT_GB_16BIT_DMA` | 16-bit DMA transfers (mutually exclusive with 32-bit DMA) |
| `WALNUT_GB_32BIT_DMA` | 32-bit DMA transfers (mutually exclusive with 16-bit DMA) |
| `WALNUT_GB_16BIT_ALIGNED` | Aligned 16-bit reads with 8-bit fallback |
| `WALNUT_GB_32BIT_ALIGNED` | Aligned 32-bit reads/writes with 8-bit fallback |
| `WALNUT_GB_RGB565_BIGENDIAN` | Big-endian RGB565 output instead of little-endian |

> **Note:** 16-bit DMA is limited to platforms without strict aliasing/alignment penalties (e.g. ESP32-S3). On others, compile with `-fno-strict-aliasing` if needed. Dual-fetch dispatch works regardless. 32-bit DMA uses alignment-aware read/write paths by default.

## Caveats

- Without `gb_run_frame_dualfetch()` and 16/32-bit DMA, performance matches original Peanut-GB/CGB
- LCD rendering is line-by-line; some effects (e.g. Prehistorik Man) do not display correctly
- Some games may be unplayable due to emulation inaccuracy
- MiniGB APU runs on a separate thread in the SDL2 example — timing is approximate; use [Blargg's Gb_Snd_Emu](https://github.com/deltabeard/Game_Music_Emu) or similar for cycle-accurate audio

## Additional resources

**Palette databases** for automatic color matching:

- [`extras/sgb.h`](extras/sgb.h) — Super Game Boy palettes (indexed RGB888)
- [`extras/cgb.h`](extras/cgb.h) — Game Boy Color palettes (indexed RGB888)

See the wiki for [implementation details](https://github.com/awest813/Walnut-CGB/wiki/Using-the-color-databases).

![Super Game Boy palette example](screencaps/sgb_example.png)

**Audio:** [MiniGB APU](examples/sdl2/minigb_apu) — fast APU used by the SDL2 and Dreamcast examples.

## License

MIT License — see [`LICENSE`](LICENSE).
