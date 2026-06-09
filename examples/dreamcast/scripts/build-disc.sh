#!/usr/bin/env bash
# Build a self-bootable Dreamcast disc image for Walnut-DC.
#
# Prerequisites:
#   - KOS environ.sh sourced
#   - makeip from $KOS_BASE/utils/makeip (or in PATH)
#   - scramble from $KOS_BASE/utils/scramble (or in PATH)
#   - sh-elf-objcopy (from the KOS toolchain; $KOS_OBJCOPY)
#   - genisoimage or mkisofs
#   - cdi4dc (optional, for .cdi output)
#
# Usage:
#   ./scripts/build-disc.sh

set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
BUILD_DIR="${ROOT_DIR}/disc-build"
ISO_DIR="${BUILD_DIR}/iso"
ROM_DIR="${ISO_DIR}/roms"
MAKEIP="${MAKEIP:-makeip}"
OBJCOPY="${KOS_OBJCOPY:-sh-elf-objcopy}"
SCRAMBLE="${SCRAMBLE:-scramble}"

if [[ -z "${KOS_BASE:-}" ]]; then
	echo "error: source KOS environ.sh before running this script" >&2
	exit 1
fi

# Fall back to the in-tree KOS utilities if they are not already on PATH.
if ! command -v "${MAKEIP}" >/dev/null 2>&1; then
	MAKEIP="${KOS_BASE}/utils/makeip/makeip"
fi
if ! command -v "${SCRAMBLE}" >/dev/null 2>&1; then
	SCRAMBLE="${KOS_BASE}/utils/scramble/scramble"
fi

cd "${ROOT_DIR}"
make clean >/dev/null 2>&1 || true
make

# A Dreamcast boot binary is a flat, scrambled image named 1ST_READ.BIN —
# the raw ELF will not boot. Strip to a flat binary, then scramble it.
mkdir -p "${ROM_DIR}"
"${OBJCOPY}" -R .stack -O binary "${ROOT_DIR}/walnut-dc.elf" "${BUILD_DIR}/walnut-dc.bin"
"${SCRAMBLE}" "${BUILD_DIR}/walnut-dc.bin" "${ISO_DIR}/1ST_READ.BIN"

# IP.BIN (boot sector / metadata) is supplied to the ISO tool via -G and is
# not placed inside the data filesystem.
"${MAKEIP}" "${ROOT_DIR}/meta/ip.txt" "${BUILD_DIR}/IP.BIN"

if command -v genisoimage >/dev/null 2>&1; then
	ISO_TOOL=genisoimage
elif command -v mkisofs >/dev/null 2>&1; then
	ISO_TOOL=mkisofs
else
	echo "error: genisoimage or mkisofs required" >&2
	exit 1
fi

"${ISO_TOOL}" -G "${BUILD_DIR}/IP.BIN" -C 0,11702 -V WALNUTDC -r -J -l \
	-o "${BUILD_DIR}/walnut-dc.iso" "${ISO_DIR}"

echo "Created ${BUILD_DIR}/walnut-dc.iso"

if command -v cdi4dc >/dev/null 2>&1; then
	cdi4dc "${BUILD_DIR}/walnut-dc.iso" "${BUILD_DIR}/walnut-dc.cdi"
	echo "Created ${BUILD_DIR}/walnut-dc.cdi"
fi

echo "Place .gb/.gbc ROM files in ${ROM_DIR} before burning."
