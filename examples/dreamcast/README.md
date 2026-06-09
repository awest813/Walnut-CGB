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

### dcload (development)

```bash
dc-tool -x walnut-dc.elf /pc/roms/tetris.gb
```

Optional explicit save path:

```bash
dc-tool -x walnut-dc.elf /pc/roms/tetris.gb /pc/roms/tetris.sav
```

### Flycast

Load `walnut-dc.elf` with a ROM path argument if your loader supports argv, or use dcload/IP loading.

## Controls

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
| Start + B | Exit |
| Y | Cycle palette |
| L / R trigger | Fast-forward (2×) |

## Optional Files

- `dmg_boot.bin` — DMG boot ROM in the current working directory (optional)

## Status

See [PHASED_IMPLEMENTATION.md](PHASED_IMPLEMENTATION.md) for the port roadmap and checklist.

Phase 1 (minimal core) and Phase 2 scaffolding (audio + saves) are implemented. ROM browser UI is planned for Phase 3.

## Licensing

Walnut-CGB is MIT licensed. MiniGB APU has its own license in `examples/sdl2/minigb_apu/LICENSE`. KOS requires attribution in distributed binaries.

Do not ship copyrighted ROMs with homebrew releases.
