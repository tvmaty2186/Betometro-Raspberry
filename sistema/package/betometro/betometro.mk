################################################################################
#
# betometro: el programa, compilado desde este mismo repositorio
#
################################################################################

BETOMETRO_VERSION = 1.0.0
BETOMETRO_SITE = $(BR2_EXTERNAL_BETOMETRO_PATH)/..
BETOMETRO_SITE_METHOD = local
BETOMETRO_LICENSE = Propietaria
BETOMETRO_DEPENDENCIES = qt6base qt6declarative qt6svg qt6multimedia gstreamer1 gst1-plugins-base host-pkgconf
BETOMETRO_CONF_OPTS = -DBETO_PRUEBAS=OFF -DBETO_PROGRAMA=ON
# Lo pesado del repo no hace falta para compilar
BETOMETRO_OVERRIDE_SRCDIR_RSYNC_EXCLUSIONS = \
	--exclude .git --exclude referencia --exclude docs --exclude build \
	--exclude sistema/salida --exclude sistema/cache

# La voz grabada con Piper (voz/grabar.py) se arma antes, fuera de Buildroot, en sistema/voz-grabada
define BETOMETRO_INSTALAR_VOZ
	if [ -d $(BR2_EXTERNAL_BETOMETRO_PATH)/voz-grabada ]; then \
		mkdir -p $(TARGET_DIR)/usr/share/betometro/voz && \
		cp $(BR2_EXTERNAL_BETOMETRO_PATH)/voz-grabada/*.wav $(TARGET_DIR)/usr/share/betometro/voz/; \
	else \
		echo "OJO: falta sistema/voz-grabada (python3 voz/grabar.py sistema/voz-grabada): la imagen queda sin voz"; \
	fi
endef
BETOMETRO_POST_INSTALL_TARGET_HOOKS += BETOMETRO_INSTALAR_VOZ

$(eval $(cmake-package))
