#!/bin/sh
# Después de armar el sistema: puntos de montaje y lo que tiene que estar en /etc.
set -e
TARGET_DIR="$1"
BOARD_DIR="$(dirname "$0")"
mkdir -p "$TARGET_DIR/datos" "$TARGET_DIR/arranque"
# fstab: el sistema es de solo lectura; los datos van en la partición 4
grep -q '/datos' "$TARGET_DIR/etc/fstab" || cat >> "$TARGET_DIR/etc/fstab" <<'FSTAB'
/dev/mmcblk0p1	/arranque	vfat	rw,noatime,flush,nofail	0	0
/dev/mmcblk0p4	/datos	ext4	noatime,commit=5,nofail,x-systemd.requires=agrandar-datos.service	0	2
FSTAB
# El programa arranca solo
ln -sf ../betometro.service "$TARGET_DIR/etc/systemd/system/multi-user.target.wants/betometro.service"
ln -sf ../agrandar-datos.service "$TARGET_DIR/etc/systemd/system/multi-user.target.wants/agrandar-datos.service"
ln -sf ../marcar-bueno.service "$TARGET_DIR/etc/systemd/system/multi-user.target.wants/marcar-bueno.service"
chmod +x "$TARGET_DIR/usr/libexec/betometro/"*
# Actualizaciones (RAUC): configuración y certificado para verificar la firma
install -D -m 0644 "$BOARD_DIR/rauc/system.conf" "$TARGET_DIR/etc/rauc/system.conf"
install -D -m 0644 "$BOARD_DIR/rauc/certificado.pem" "$TARGET_DIR/etc/rauc/certificado.pem"
# Versión del sistema (la lee Betómetro para saber si hay una nueva)
grep -oP 'project\(betometro VERSION \K[0-9.]+' "$BOARD_DIR/../../../CMakeLists.txt" > "$TARGET_DIR/etc/betometro-version" || true
