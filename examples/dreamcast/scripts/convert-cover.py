#!/usr/bin/env python3
"""Convert PNG/JPEG cover images to Walnut-DC .w555 thumbnail format."""

from __future__ import annotations

import argparse
import struct
import sys
from pathlib import Path

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


def convert_image(input_path: Path, output_path: Path) -> None:
    image = Image.open(input_path).convert("RGB")
    image = image.resize((COVER_WIDTH, COVER_HEIGHT), Image.Resampling.LANCZOS)
    pixels = image.load()

    output_path.parent.mkdir(parents=True, exist_ok=True)
    with output_path.open("wb") as out:
        out.write(struct.pack("<IHH", COVER_MAGIC, COVER_WIDTH, COVER_HEIGHT))
        for y in range(COVER_HEIGHT):
            for x in range(COVER_WIDTH):
                r, g, b = pixels[x, y]
                out.write(struct.pack("<H", rgb888_to_rgb555(r, g, b)))


def convert_tree(input_dir: Path, output_dir: Path) -> int:
    count = 0

    for source in sorted(input_dir.glob("*.png")):
        target = output_dir / f"{source.stem}.w555"
        convert_image(source, target)
        count += 1

    return count


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("input", nargs="?", help="Source PNG/JPEG image")
    parser.add_argument("output", nargs="?", help="Destination .w555 file")
    parser.add_argument(
        "--batch-dir",
        metavar="DIR",
        help="Convert every PNG in DIR to .w555 in --output-dir",
    )
    parser.add_argument(
        "--output-dir",
        metavar="DIR",
        help="Output directory for --batch-dir",
    )
    args = parser.parse_args()

    if args.batch_dir:
        if not args.output_dir:
            print("error: --batch-dir requires --output-dir", file=sys.stderr)
            return 1
        count = convert_tree(Path(args.batch_dir), Path(args.output_dir))
        print(f"Converted {count} covers into {args.output_dir}")
        return 0

    if not args.input or not args.output:
        parser.error("input and output are required unless --batch-dir is used")

    convert_image(Path(args.input), Path(args.output))
    print(f"Wrote {args.output}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
