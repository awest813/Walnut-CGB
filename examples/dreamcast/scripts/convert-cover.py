#!/usr/bin/env python3
"""Convert a PNG/JPEG cover image to Walnut-DC .w555 thumbnail format."""

from __future__ import annotations

import argparse
import struct
import sys

try:
    from PIL import Image
except ImportError:
    print("error: Pillow is required (pip install pillow)", file=sys.stderr)
    sys.exit(1)

COVER_WIDTH = 96
COVER_HEIGHT = 96
COVER_MAGIC = 0x35353557  # "W555"


def rgb888_to_rgb555(r: int, g: int, b: int) -> int:
    return ((r * 31 // 255) << 10) | ((g * 31 // 255) << 5) | (b * 31 // 255)


def convert_image(input_path: str, output_path: str) -> None:
    image = Image.open(input_path).convert("RGB")
    image = image.resize((COVER_WIDTH, COVER_HEIGHT), Image.Resampling.LANCZOS)
    pixels = image.load()

    with open(output_path, "wb") as out:
        out.write(struct.pack("<IHH", COVER_MAGIC, COVER_WIDTH, COVER_HEIGHT))
        for y in range(COVER_HEIGHT):
            for x in range(COVER_WIDTH):
                r, g, b = pixels[x, y]
                out.write(struct.pack("<H", rgb888_to_rgb555(r, g, b)))


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("input", help="Source PNG/JPEG image")
    parser.add_argument("output", help="Destination .w555 file")
    args = parser.parse_args()

    convert_image(args.input, args.output)
    print(f"Wrote {args.output}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
