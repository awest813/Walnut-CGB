#!/usr/bin/env bash
# Import Game Boy / Game Boy Color cover art from xero/boxart (CC0).
#
# Usage:
#   ./scripts/import-boxart.sh              # full catalog via sparse clone
#   ./scripts/import-boxart.sh --roms-dir disc-build/roms
#   ./scripts/import-boxart.sh --sync-remote   # incremental HTTP sync
#   BOXART_SRC=/path/to/boxart ./scripts/import-boxart.sh
#
# Requires: git (full import), python3, pillow (pip install pillow)

set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
BOXART_SRC="${BOXART_SRC:-}"
BOXART_TMP="${ROOT_DIR}/.boxart-src"
OUT_DIR="${ROOT_DIR}/covers/boxart"
CONVERT="${ROOT_DIR}/scripts/convert-cover.py"
FETCH="${ROOT_DIR}/scripts/fetch-covers.py"
ROMS_DIRS=()
SYNC_REMOTE=0
FORCE=0
DRY_RUN=0

usage() {
	sed -n '2,9p' "$0"
}

while [[ $# -gt 0 ]]; do
	case "$1" in
	--roms-dir)
		ROMS_DIRS+=("$2")
		shift 2
		;;
	--sync-remote)
		SYNC_REMOTE=1
		shift
		;;
	--force)
		FORCE=1
		shift
		;;
	--dry-run)
		DRY_RUN=1
		shift
		;;
	-h|--help)
		usage
		exit 0
		;;
	*)
		echo "error: unknown option '$1'" >&2
		usage >&2
		exit 1
		;;
	esac
done

if ! python3 -c "import PIL" 2>/dev/null; then
	echo "error: install Pillow first (pip install pillow)" >&2
	exit 1
fi

fetch_args=()
[[ "${FORCE}" -eq 1 ]] && fetch_args+=(--force)
[[ "${DRY_RUN}" -eq 1 ]] && fetch_args+=(--dry-run)

if [[ "${#ROMS_DIRS[@]}" -gt 0 ]]; then
	for rom_dir in "${ROMS_DIRS[@]}"; do
		fetch_args+=(--roms-dir "${rom_dir}")
	done
	python3 "${FETCH}" "${fetch_args[@]}"
	exit $?
fi

if [[ "${SYNC_REMOTE}" -eq 1 ]]; then
	python3 "${FETCH}" --sync-all "${fetch_args[@]}"
	exit $?
fi

if [[ -n "${BOXART_SRC}" ]]; then
	SRC_DIR="${BOXART_SRC}"
elif [[ -d "${BOXART_TMP}/.git" ]]; then
	echo "Updating cached xero/boxart checkout..."
	git -C "${BOXART_TMP}" pull --ff-only
	SRC_DIR="${BOXART_TMP}"
elif [[ -d "${BOXART_TMP}/GB" && -d "${BOXART_TMP}/GBC" ]]; then
	SRC_DIR="${BOXART_TMP}"
else
	echo "Cloning xero/boxart (GB + GBC only)..."
	rm -rf "${BOXART_TMP}"
	git clone --depth 1 --filter=blob:none --sparse https://github.com/xero/boxart.git "${BOXART_TMP}"
	git -C "${BOXART_TMP}" sparse-checkout set GB GBC
	SRC_DIR="${BOXART_TMP}"
fi

mkdir -p "${OUT_DIR}/GB" "${OUT_DIR}/GBC"

GB_COUNT="$(python3 "${CONVERT}" --batch-dir "${SRC_DIR}/GB" --output-dir "${OUT_DIR}/GB" --skip-existing | awk '{print $2}')"
GBC_COUNT="$(python3 "${CONVERT}" --batch-dir "${SRC_DIR}/GBC" --output-dir "${OUT_DIR}/GBC" --skip-existing | awk '{print $2}')"

echo "Imported ${GB_COUNT} GB and ${GBC_COUNT} GBC covers into ${OUT_DIR}"
echo "Source: https://github.com/xero/boxart (CC0)"
