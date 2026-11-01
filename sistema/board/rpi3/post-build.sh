#!/bin/sh
# Después de armar el sistema: puntos de montaje y lo que tiene que estar en /etc.
set -e
TARGET_DIR="$1"
mkdir -p "$TARGET_DIR/datos" "$TARGET_DIR/boot"
# fstab: el sistema es de solo lectura; los datos van en la partición 4
grep -q '/datos' "$TARGET_DIR/etc/fstab" || cat >> "$TARGET_DIR/etc/fstab" <<'FSTAB'
/dev/mmcblk0p1	/boot	vfat	ro,noauto,nofail	0	0
/dev/mmcblk0p4	/datos	ext4	noatime,commit=5,nofail,x-systemd.requires=agrandar-datos.service	0	2
FSTAB
# El programa arranca solo
ln -sf ../betometro.service "$TARGET_DIR/etc/systemd/system/multi-user.target.wants/betometro.service"
ln -sf ../agrandar-datos.service "$TARGET_DIR/etc/systemd/system/multi-user.target.wants/agrandar-datos.service"
chmod +x "$TARGET_DIR/usr/libexec/betometro/"*
