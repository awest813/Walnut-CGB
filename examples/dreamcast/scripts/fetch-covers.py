#!/usr/bin/env python3
"""Fetch Game Boy / Game Boy Color cover art from xero/boxart (CC0).

Downloads PNG box art by NoIntro-style ROM filename and writes Dreamcast
.w555 thumbnails. Skips covers that already exist unless --force is used.

Examples:
  ./scripts/fetch-covers.py --roms-dir disc-build/roms
  ./scripts/fetch-covers.py --sync-all
  ./scripts/fetch-covers.py --roms-dir /path/to/roms --dry-run
"""

from __future__ import annotations

import argparse
import importlib.util
import json
import sys
import time
import urllib.error
import urllib.parse
import urllib.request
from pathlib import Path

SCRIPT_DIR = Path(__file__).resolve().parent


def load_convert_module():
    spec = importlib.util.spec_from_file_location(
        "convert_cover", SCRIPT_DIR / "convert-cover.py"
    )
    if spec is None or spec.loader is None:
        raise RuntimeError("unable to load convert-cover.py")
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


_CONVERT = load_convert_module()
convert_image = _CONVERT.convert_image

BOXART_REPO = "xero/boxart"
BOXART_BRANCH = "main"
BOXART_RAW = f"https://raw.githubusercontent.com/{BOXART_REPO}/{BOXART_BRANCH}"
BOXART_CONTENTS_API = (
    f"https://api.github.com/repos/{BOXART_REPO}/contents"
)
ROM_EXTENSIONS = (".gb", ".gbc")
IMAGE_EXTENSIONS = (".png", ".jpg", ".jpeg")
PLATFORMS = ("GB", "GBC")
USER_AGENT = "walnut-dc-fetch-covers/1.0"


class FetchStats:
    def __init__(self) -> None:
        self.fetched = 0
        self.skipped = 0
        self.missing = 0
        self.failed = 0


def rom_stem(path: Path) -> str:
    suffix = path.suffix.lower()
    if suffix in ROM_EXTENSIONS:
        return path.stem
    return path.name


def is_gbc_rom(path: Path) -> bool:
    return path.suffix.lower() == ".gbc"


def output_cover_path(output_dir: Path, platform: str, stem: str) -> Path:
    return output_dir / platform / f"{stem}.w555"


def boxart_png_url(platform: str, stem: str) -> str:
    encoded = urllib.parse.quote(f"{stem}.png")
    return f"{BOXART_RAW}/{platform}/{encoded}"


def request_opener() -> urllib.request.OpenerDirector:
    return urllib.request.build_opener(
        urllib.request.HTTPRedirectHandler(),
        urllib.request.HTTPCookieProcessor(),
    )


def remote_exists(opener: urllib.request.OpenerDirector, url: str) -> bool:
    request = urllib.request.Request(url, method="HEAD", headers={"User-Agent": USER_AGENT})
    try:
        with opener.open(request, timeout=30) as response:
            return 200 <= response.status < 300
    except urllib.error.HTTPError as exc:
        if exc.code == 404:
            return False
        raise


def download_png(
    opener: urllib.request.OpenerDirector, url: str, destination: Path
) -> None:
    request = urllib.request.Request(url, headers={"User-Agent": USER_AGENT})
    destination.parent.mkdir(parents=True, exist_ok=True)
    with opener.open(request, timeout=60) as response, destination.open("wb") as out:
        while True:
            chunk = response.read(65536)
            if not chunk:
                break
            out.write(chunk)


def platform_search_order(is_cgb: bool) -> tuple[str, ...]:
    if is_cgb:
        return ("GBC", "GB")
    return ("GB",)


def write_cover_from_url(
    opener: urllib.request.OpenerDirector,
    output_dir: Path,
    platform: str,
    stem: str,
    *,
    force: bool,
    dry_run: bool,
    tmp_dir: Path,
    stats: FetchStats,
    verbose: bool,
) -> bool:
    target = output_cover_path(output_dir, platform, stem)
    if target.exists() and not force:
        stats.skipped += 1
        if verbose:
            print(f"skip  {target.relative_to(output_dir)}")
        return True

    if dry_run:
        print(f"fetch {stem} -> {target}")
        stats.fetched += 1
        return True

    url = boxart_png_url(platform, stem)
    tmp_png = tmp_dir / platform / f"{stem}.png"
    try:
        download_png(opener, url, tmp_png)
        convert_image(tmp_png, target)
        tmp_png.unlink(missing_ok=True)
    except (OSError, urllib.error.URLError, ValueError) as exc:
        print(f"error {stem}: {exc}", file=sys.stderr)
        stats.failed += 1
        return False

    stats.fetched += 1
    if verbose:
        print(f"wrote {target.relative_to(output_dir)}")
    return True


def fetch_cover_for_stem(
    opener: urllib.request.OpenerDirector,
    output_dir: Path,
    stem: str,
    is_cgb: bool,
    *,
    force: bool,
    dry_run: bool,
    tmp_dir: Path,
    stats: FetchStats,
    verbose: bool,
) -> bool:
    for platform in platform_search_order(is_cgb):
        target = output_cover_path(output_dir, platform, stem)
        if target.exists() and not force:
            stats.skipped += 1
            if verbose:
                print(f"skip  {target.relative_to(output_dir)}")
            return True

        url = boxart_png_url(platform, stem)
        if not dry_run and not remote_exists(opener, url):
            continue

        if dry_run:
            print(f"fetch {stem} -> {target} ({platform})")
            stats.fetched += 1
            return True

        return write_cover_from_url(
            opener,
            output_dir,
            platform,
            stem,
            force=force,
            dry_run=False,
            tmp_dir=tmp_dir,
            stats=stats,
            verbose=verbose,
        )

    stats.missing += 1
    if verbose:
        print(f"miss  {stem}")
    return False


def collect_rom_stems(rom_dirs: list[Path]) -> list[tuple[str, bool]]:
    seen: set[str] = set()
    entries: list[tuple[str, bool]] = []

    for rom_dir in rom_dirs:
        if not rom_dir.is_dir():
            print(f"warning: ROM directory not found: {rom_dir}", file=sys.stderr)
            continue

        for path in sorted(rom_dir.iterdir()):
            if not path.is_file():
                continue
            if path.suffix.lower() not in ROM_EXTENSIONS:
                continue

            stem = rom_stem(path)
            key = stem.lower()
            if key in seen:
                continue

            seen.add(key)
            entries.append((stem, is_gbc_rom(path)))

    return entries


def parse_link_next(link_header: str | None) -> str | None:
    if not link_header:
        return None

    for part in link_header.split(","):
        section = part.strip()
        if 'rel="next"' in section:
            return section[section.find("<") + 1 : section.find(">")]
    return None


def list_platform_catalog(
    opener: urllib.request.OpenerDirector, platform: str
) -> list[tuple[str, str]]:
    catalog: list[tuple[str, str]] = []
    url = (
        f"{BOXART_CONTENTS_API}/{platform}"
        f"?ref={BOXART_BRANCH}&per_page=100"
    )

    while url:
        request = urllib.request.Request(url, headers={"User-Agent": USER_AGENT})
        with opener.open(request, timeout=60) as response:
            payload = json.load(response)
            next_url = parse_link_next(response.headers.get("Link"))

        for entry in payload:
            name = entry.get("name", "")
            if entry.get("type") != "file":
                continue
            if not name.lower().endswith(IMAGE_EXTENSIONS):
                continue
            catalog.append((platform, Path(name).stem))

        url = next_url
        if url:
            time.sleep(0.1)

    return catalog


def list_boxart_catalog(opener: urllib.request.OpenerDirector) -> list[tuple[str, str]]:
    catalog: list[tuple[str, str]] = []
    for platform in PLATFORMS:
        catalog.extend(list_platform_catalog(opener, platform))
    catalog.sort()
    return catalog


def sync_all_catalog(
    output_dir: Path,
    *,
    force: bool,
    dry_run: bool,
    tmp_dir: Path,
    stats: FetchStats,
    verbose: bool,
) -> None:
    opener = request_opener()
    catalog = list_boxart_catalog(opener)

    for index, (platform, stem) in enumerate(catalog, start=1):
        write_cover_from_url(
            opener,
            output_dir,
            platform,
            stem,
            force=force,
            dry_run=dry_run,
            tmp_dir=tmp_dir,
            stats=stats,
            verbose=verbose,
        )
        if not dry_run and index % 25 == 0:
            time.sleep(0.2)


def fetch_for_rom_dirs(
    rom_dirs: list[Path],
    output_dir: Path,
    *,
    force: bool,
    dry_run: bool,
    tmp_dir: Path,
    stats: FetchStats,
    verbose: bool,
) -> None:
    stems = collect_rom_stems(rom_dirs)
    if not stems:
        print("No .gb or .gbc ROM files found.", file=sys.stderr)
        return

    opener = request_opener()
    for stem, is_cgb in stems:
        fetch_cover_for_stem(
            opener,
            output_dir,
            stem,
            is_cgb,
            force=force,
            dry_run=dry_run,
            tmp_dir=tmp_dir,
            stats=stats,
            verbose=verbose,
        )
        if not dry_run:
            time.sleep(0.05)


def print_summary(stats: FetchStats, dry_run: bool) -> None:
    action = "planned" if dry_run else "fetched"
    print(
        f"Done: {stats.fetched} {action}, "
        f"{stats.skipped} skipped, "
        f"{stats.missing} missing, "
        f"{stats.failed} failed"
    )


def main() -> int:
    root_dir = SCRIPT_DIR.parent
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--roms-dir",
        action="append",
        default=[],
        metavar="DIR",
        help="Scan DIR for .gb/.gbc files and fetch matching covers",
    )
    parser.add_argument(
        "--sync-all",
        action="store_true",
        help="Sync the full xero/boxart GB + GBC catalog (incremental)",
    )
    parser.add_argument(
        "--output-dir",
        default=str(root_dir / "covers" / "boxart"),
        metavar="DIR",
        help="Write .w555 files under DIR/GB and DIR/GBC",
    )
    parser.add_argument(
        "--tmp-dir",
        default=str(root_dir / ".boxart-fetch"),
        metavar="DIR",
        help="Temporary PNG download directory",
    )
    parser.add_argument(
        "--force",
        action="store_true",
        help="Re-download and convert even when .w555 already exists",
    )
    parser.add_argument(
        "--dry-run",
        action="store_true",
        help="Show what would be fetched without downloading",
    )
    parser.add_argument(
        "-v",
        "--verbose",
        action="store_true",
        help="Log each cover as it is processed",
    )
    args = parser.parse_args()

    if not args.roms_dir and not args.sync_all:
        parser.error("pass --roms-dir and/or --sync-all")

    output_dir = Path(args.output_dir)
    tmp_dir = Path(args.tmp_dir)
    stats = FetchStats()

    output_dir.mkdir(parents=True, exist_ok=True)
    for platform in PLATFORMS:
        (output_dir / platform).mkdir(parents=True, exist_ok=True)

    if args.sync_all:
        sync_all_catalog(
            output_dir,
            force=args.force,
            dry_run=args.dry_run,
            tmp_dir=tmp_dir,
            stats=stats,
            verbose=args.verbose,
        )

    if args.roms_dir:
        rom_dirs = [Path(path) for path in args.roms_dir]
        fetch_for_rom_dirs(
            rom_dirs,
            output_dir,
            force=args.force,
            dry_run=args.dry_run,
            tmp_dir=tmp_dir,
            stats=stats,
            verbose=args.verbose,
        )

    print_summary(stats, args.dry_run)
    print(f"Source: https://github.com/{BOXART_REPO} (CC0)")
    return 1 if stats.failed else 0


if __name__ == "__main__":
    raise SystemExit(main())
