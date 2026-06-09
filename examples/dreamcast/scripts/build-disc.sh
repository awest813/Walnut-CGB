#!/usr/bin/env bash
# Build a self-bootable Dreamcast disc image for Walnut-DC.
#
# Prerequisites:
#   - KOS environ.sh sourced
#   - makeip from $KOS_BASE/utils/makeip (or in PATH)
#   - genisoimage or mkisofs
#   - cdi4dc (optional, for .cdi output)
#
# Usage:
#   ./scripts/build-disc.sh

set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
BUILD_DIR="${ROOT_DIR}/disc-build"
ROM_DIR="${BUILD_DIR}/roms"
MAKEIP="${MAKEIP:-makeip}"

if [[ -z "${KOS_BASE:-}" ]]; then
	echo "error: source KOS environ.sh before running this script" >&2
	exit 1
fi

cd "${ROOT_DIR}"
make clean >/dev/null 2>&1 || true
make

mkdir -p "${ROM_DIR}"
cp "${ROOT_DIR}/walnut-dc.elf" "${BUILD_DIR}/1ST_READ.BIN"
cp "${ROOT_DIR}/meta/ip.txt" "${BUILD_DIR}/ip.txt"

(
	cd "${BUILD_DIR}"
	"${MAKEIP}" ip.txt IP.BIN
)

if command -v genisoimage >/dev/null 2>&1; then
	ISO_TOOL=genisoimage
elif command -v mkisofs >/dev/null 2>&1; then
	ISO_TOOL=mkisofs
else
	echo "error: genisoimage or mkisofs required" >&2
	exit 1
fi

"${ISO_TOOL}" -G "${BUILD_DIR}/IP.BIN" -C 0,11702 -V WALNUTDC -r -J -l \
	-o "${BUILD_DIR}/walnut-dc.iso" "${BUILD_DIR}"

echo "Created ${BUILD_DIR}/walnut-dc.iso"

if command -v cdi4dc >/dev/null 2>&1; then
	cdi4dc "${BUILD_DIR}/walnut-dc.iso" "${BUILD_DIR}/walnut-dc.cdi"
	echo "Created ${BUILD_DIR}/walnut-dc.cdi"
fi

echo "Place .gb/.gbc ROM files in ${ROM_DIR} before burning."
