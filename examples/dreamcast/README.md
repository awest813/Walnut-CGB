# Walnut-CGB Dreamcast Example

KallistiOS frontend for running Walnut-CGB on the Sega Dreamcast.

## Prerequisites

1. Build the [KallistiOS](https://github.com/KallistiOS/KallistiOS) toolchain and SDK.
2. Source the KOS environment:

```bash
source /opt/toolchains/dc/kos/environ.sh
```

Adjust the path to match your install. See [dreamcast.wiki](https://dreamcast.wiki/Getting_Started_with_Dreamcast_development).

## Build

```bash
cd examples/dreamcast
make
```

This produces `walnut-dc.elf`.

## Run

### ROM browser (no arguments)

```bash
dc-tool -x walnut-dc.elf
```

Scans these paths (press **B** to cycle devices):

| Path | Typical source |
|------|----------------|
| `/cd/roms`, `/cd` | GD-ROM / burned disc |
| `/sd/roms`, `/sd` | SD adapter |
| `/ide/roms`, `/ide` | IDE/GDEMU/ODE |
| `/pc` | dcload transfer target |

Browser controls:

| Button | Action |
|--------|--------|
| Up / Down | Select ROM |
| Left / Right | Page up / down |
| A | Load selected ROM |
| B | Next device/path |
| Start | Refresh list |
| X | Exit browser |

### Direct ROM load (dcload)

```bash
dc-tool -x walnut-dc.elf /pc/roms/tetris.gb
```

Optional explicit save path:

```bash
dc-tool -x walnut-dc.elf /pc/roms/tetris.gb /pc/roms/tetris.sav
```

### Flycast

Load `walnut-dc.elf` with a ROM path argument if your loader supports argv, or use dcload/IP loading.

## In-Game Controls

| GB | Dreamcast |
|----|-----------|
| A | A |
| B | B |
| Start | Start |
| Select | X |
| D-Pad | D-Pad |

| Extra | Action |
|-------|--------|
| Start + A | Reset game |
| Start + B | Return to browser (browser mode) or exit (direct load) |
| Y | Cycle palette |
| Start + X | Toggle frameskip |
| L / R trigger | Fast-forward (2×) |

## Boot Disc (CDI/GDI)

1. Build the ELF: `make`
2. Package a disc image:

```bash
./scripts/build-disc.sh
```

3. Copy homebrew `.gb` / `.gbc` ROMs into `disc-build/roms/` before burning.
4. Burn `disc-build/walnut-dc.iso` or `disc-build/walnut-dc.cdi`.

Disc metadata is defined in `meta/ip.txt` (processed by KOS `makeip`).

## Optional Files

- `dmg_boot.bin` — DMG boot ROM in the working directory (optional)

## Status

See [PHASED_IMPLEMENTATION.md](PHASED_IMPLEMENTATION.md) for the port roadmap and checklist.

Phase 3 (ROM browser + disc packaging) is implemented. Phase 4 hardware validation remains.

## Licensing

Walnut-CGB is MIT licensed. MiniGB APU has its own license in `examples/sdl2/minigb_apu/LICENSE`. KOS requires attribution in distributed binaries.

Do not ship copyrighted ROMs with homebrew releases.
