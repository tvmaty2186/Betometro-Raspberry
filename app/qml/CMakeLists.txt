set_source_files_properties(Tema.qml Navegacion.qml PROPERTIES QT_QML_SINGLETON_TYPE TRUE)

qt_add_library(betometro_qml STATIC)
qt_add_qml_module(betometro_qml
  URI Betometro
  VERSION 1.0
  QML_FILES
    Main.qml
    Tema.qml
    Navegacion.qml
    componentes/Banda.qml
    componentes/Cabecera.qml
    componentes/Chip.qml
    componentes/Disco.qml
    componentes/Fondo.qml
    componentes/FotoInicio.qml
    componentes/Numero101.qml
    componentes/Rayas.qml
    componentes/Swoosh.qml
    componentes/TextoCromo.qml
    componentes/Velocimetro.qml
    pantallas/Inicio.qml
    pantallas/EnConstruccion.qml
  RESOURCES
    iconos/ajustes.svg
    iconos/carreras.svg
    iconos/chequeo.svg
    iconos/pantalla.svg
    iconos/practica.svg
)
