#!/usr/bin/env bash
# Import Game Boy / Game Boy Color cover art from xero/boxart (CC0).
#
# Usage:
#   ./scripts/import-boxart.sh
#   BOXART_SRC=/path/to/boxart ./scripts/import-boxart.sh
#
# Requires: git, python3, pillow (pip install pillow)

set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
BOXART_SRC="${BOXART_SRC:-}"
BOXART_TMP="${ROOT_DIR}/.boxart-src"
OUT_DIR="${ROOT_DIR}/covers/boxart"
CONVERT="${ROOT_DIR}/scripts/convert-cover.py"

if ! python3 -c "import PIL" 2>/dev/null; then
	echo "error: install Pillow first (pip install pillow)" >&2
	exit 1
fi

if [[ -n "${BOXART_SRC}" ]]; then
	SRC_DIR="${BOXART_SRC}"
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
rm -f "${OUT_DIR}/GB"/*.w555 "${OUT_DIR}/GBC"/*.w555

GB_COUNT="$(python3 "${CONVERT}" --batch-dir "${SRC_DIR}/GB" --output-dir "${OUT_DIR}/GB" | awk '{print $2}')"
GBC_COUNT="$(python3 "${CONVERT}" --batch-dir "${SRC_DIR}/GBC" --output-dir "${OUT_DIR}/GBC" | awk '{print $2}')"

echo "Imported ${GB_COUNT} GB and ${GBC_COUNT} GBC covers into ${OUT_DIR}"
echo "Source: https://github.com/xero/boxart (CC0)"
