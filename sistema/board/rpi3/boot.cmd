# Arranque A/B de Betómetro (U-Boot). Lo maneja RAUC:
# BOOT_ORDER = orden de las copias; BOOT_A_LEFT / BOOT_B_LEFT = intentos que le quedan a cada una.
# Cada arranque gasta un intento; cuando Betómetro anda bien, RAUC lo vuelve a 3 («mark-good»).
# Si una copia gasta los 3 sin andar, se pasa sola a la otra.
test -n "${BOOT_ORDER}" || setenv BOOT_ORDER "A B"
test -n "${BOOT_A_LEFT}" || setenv BOOT_A_LEFT 3
test -n "${BOOT_B_LEFT}" || setenv BOOT_B_LEFT 3

setenv bootargs
for BOOT_SLOT in "${BOOT_ORDER}"; do
  if test "x${bootargs}" != "x"; then
    # ya se eligió una copia
  elif test "x${BOOT_SLOT}" = "xA"; then
    if test ${BOOT_A_LEFT} -gt 0; then
      setexpr BOOT_A_LEFT ${BOOT_A_LEFT} - 1
      setenv carpeta a
      setenv bootargs "root=/dev/mmcblk0p2 rauc.slot=A"
    fi
  elif test "x${BOOT_SLOT}" = "xB"; then
    if test ${BOOT_B_LEFT} -gt 0; then
      setexpr BOOT_B_LEFT ${BOOT_B_LEFT} - 1
      setenv carpeta b
      setenv bootargs "root=/dev/mmcblk0p3 rauc.slot=B"
    fi
  fi
done

if test -n "${bootargs}"; then
  saveenv
else
  # Ninguna copia con intentos: se les devuelven y se prueba de nuevo
  setenv BOOT_A_LEFT 3
  setenv BOOT_B_LEFT 3
  saveenv
  reset
fi

setenv bootargs "${bootargs} rootfstype=squashfs ro rootwait fsck.mode=skip console=tty3 quiet loglevel=3 logo.nologo vt.global_cursor_default=0 consoleblank=0"
fatload mmc 0:1 ${kernel_addr_r} ${carpeta}/Image
# El árbol de dispositivos lo arma el firmware (con la pantalla y el GPS) y lo deja en fdt_addr
booti ${kernel_addr_r} - ${fdt_addr}
