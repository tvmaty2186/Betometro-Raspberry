if(QT_KNOWN_POLICY_QTP0004)
  qt_policy(SET QTP0004 NEW)
endif()
set_source_files_properties(Tema.qml Navegacion.qml Formato.qml PROPERTIES QT_QML_SINGLETON_TYPE TRUE)

qt_add_library(betometro_qml STATIC)
qt_add_qml_module(betometro_qml
  URI Betometro
  VERSION 1.0
  QML_FILES
    Main.qml
    Tema.qml
    Navegacion.qml
    Formato.qml
    componentes/Aviso.qml
    componentes/Banda.qml
    componentes/BandaSeccion.qml
    componentes/BotonAmarillo.qml
    componentes/BotonCromo.qml
    componentes/Cabecera.qml
    componentes/CabeceraPantalla.qml
    componentes/Campo.qml
    componentes/Chip.qml
    componentes/ChipEstado.qml
    componentes/Digito.qml
    componentes/Disco.qml
    componentes/DobleToque.qml
    componentes/Etiqueta.qml
    componentes/FechaRedonda.qml
    componentes/Fila.qml
    componentes/Fondo.qml
    componentes/FotoInicio.qml
    componentes/Numero101.qml
    componentes/Rayas.qml
    componentes/Segmentado.qml
    componentes/Sub.qml
    componentes/Swoosh.qml
    componentes/Tarjeta.qml
    componentes/Teclado.qml
    componentes/TextoCromo.qml
    componentes/TituloTarjeta.qml
    componentes/Velocimetro.qml
    componentes/Volver.qml
    pantallas/Carreras.qml
    pantallas/EnConstruccion.qml
    pantallas/Ficha.qml
    pantallas/Inicio.qml
    pantallas/Practica.qml
    widgets/Barra.qml
    widgets/Caja.qml
    widgets/Etiq.qml
    widgets/Numero.qml
    widgets/WCuenta.qml
    widgets/WMarcar.qml
    widgets/WNumero.qml
    widgets/WTipo.qml
    widgets/WVelocidad.qml
    widgets/WVelocimetro.qml
    pantallas/Carrera.qml
  RESOURCES
    iconos/ajustes.svg
    iconos/carreras.svg
    iconos/chequeo.svg
    iconos/pantalla.svg
    iconos/practica.svg
)
