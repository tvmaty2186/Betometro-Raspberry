#!/bin/sh
# Después de armar las imágenes: partición de datos vacía y la tarjeta completa.
set -e
BOARD_DIR="$(dirname "$0")"
GENIMAGE_TMP="${BUILD_DIR}/genimage.tmp"
rm -f "${BINARIES_DIR}/datos.ext4"
"${HOST_DIR}/sbin/mkfs.ext4" -q -L datos -O ^metadata_csum_seed "${BINARIES_DIR}/datos.ext4" 64M
rm -rf "${GENIMAGE_TMP}"
genimage --rootpath "${TARGET_DIR}" --tmppath "${GENIMAGE_TMP}" --inputpath "${BINARIES_DIR}" \
	--outputpath "${BINARIES_DIR}" --config "${BOARD_DIR}/genimage.cfg"
xz -T0 -k -f -9 "${BINARIES_DIR}/betometro.img"
