# ROM Cover Art

Walnut-DC can show cover art in the ROM picker. **This repository does not ship copyrighted box art** for commercial games. Instead:

1. Every ROM gets a **procedural placeholder** (color + initials) automatically.
2. You can add your own art as sidecar `.w555` files.

## Layout

Place covers next to your ROM library:

```
/cd/roms/tetris.gb
/cd/covers/tetris.w555
```

If ROMs live in `/cd/roms`, covers are read from `/cd/covers`. For `/pc` or `/sd`, use `/pc/covers` or `/sd/covers`.

## Create a `.w555` file

From a PNG or JPEG (96×96 recommended):

```bash
pip install pillow
./scripts/convert-cover.py my-cover.png disc-build/covers/tetris.w555
```

Format: `W555` magic, 96×96 RGB555 pixels (Dreamcast-native).

## ROM picker

- **List view** — titles from the ROM header + large preview panel
- **Grid view** — press **Y** to toggle a 5×3 cover grid
- Badges: DMG/GBC, `[SAV]`, and whether a real cover file was loaded

Only add cover images you have the right to use (your own shots, licensed packs, or homebrew artwork).
