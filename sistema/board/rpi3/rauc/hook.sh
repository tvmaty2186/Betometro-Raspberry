#!/bin/sh
# Gancho del paquete de actualización: después de escribir la copia nueva del sistema,
# copia su kernel a la partición de arranque (carpeta a/ o b/), que es de donde lo lee U-Boot.
set -e
case "$1" in
slot-post-install)
	[ "$RAUC_SLOT_CLASS" = "rootfs" ] || exit 0
	case "$RAUC_SLOT_BOOTNAME" in A) dir=a ;; B) dir=b ;; *) exit 1 ;; esac
	m=$(mktemp -d)
	mount -t squashfs -o ro "$RAUC_SLOT_DEVICE" "$m"
	mkdir -p "/arranque/$dir"
	cp "$m/boot/Image" "/arranque/$dir/Image.nuevo"
	sync
	mv "/arranque/$dir/Image.nuevo" "/arranque/$dir/Image"
	sync
	umount "$m"
	rmdir "$m"
	;;
esac
