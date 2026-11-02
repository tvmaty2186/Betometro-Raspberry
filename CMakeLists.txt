cmake_minimum_required(VERSION 3.16)
project(betometro VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE RelWithDebInfo)
endif()

# Núcleo: C++ puro, sin pantalla ni hardware. Se prueba en PC.
add_library(nucleo STATIC
  app/nucleo/json.cpp
  app/nucleo/motor.cpp
  app/nucleo/modelo.cpp
  app/nucleo/almacen.cpp
  app/nucleo/informe.cpp
  app/nucleo/programador.cpp
  app/nucleo/neo.cpp
  app/nucleo/pulsador.cpp
)
target_include_directories(nucleo PUBLIC app/nucleo)
target_compile_options(nucleo PRIVATE -Wall -Wextra -Wpedantic)
# Mismos resultados que JavaScript: nada de cuentas «rápidas» ni fusionadas por el compilador
target_compile_options(nucleo PUBLIC -fno-fast-math -ffp-contract=off)

# Programa (Qt 6). Si no hay Qt, se compila solo el núcleo y sus pruebas.
option(BETO_PROGRAMA "Compilar el programa con pantalla (necesita Qt 6)" ON)
if(BETO_PROGRAMA)
  find_package(Qt6 6.5 QUIET COMPONENTS Quick Svg Multimedia Network)
  if(Qt6_FOUND)
    qt_standard_project_setup(REQUIRES 6.5)
    qt_add_executable(betometro app/main.cpp app/contexto.h app/datos.h app/datos.cpp app/crono.h app/crono.cpp app/gps.h app/gps.cpp app/qr.h app/qr.cpp app/wifi.h app/wifi.cpp app/sistema.h app/audio.h app/audio.cpp
      app/camara.h app/camara.cpp app/actualizador.h app/actualizador.cpp
      app/terceros/qrcodegen/qrcodegen.cpp)
    target_compile_definitions(betometro PRIVATE BETO_VERSION="${PROJECT_VERSION}")
    add_subdirectory(app/qml)
    install(TARGETS betometro RUNTIME DESTINATION bin)
    target_link_libraries(betometro PRIVATE nucleo Qt6::Quick Qt6::Svg Qt6::Multimedia Qt6::Network betometro_qmlplugin)
    # Ayudante de video y sonido (GStreamer, proceso aparte). Si no hay GStreamer, la cámara no anda.
    find_package(PkgConfig QUIET)
    if(PkgConfig_FOUND)
      pkg_check_modules(GST QUIET IMPORTED_TARGET gstreamer-1.0 gstreamer-app-1.0)
    endif()
    if(GST_FOUND)
      add_executable(betometro-camara app/camara/betometro_camara.cpp)
      target_link_libraries(betometro-camara PRIVATE PkgConfig::GST)
      install(TARGETS betometro-camara RUNTIME DESTINATION bin)
    else()
      message(STATUS "Sin GStreamer: no se arma betometro-camara")
    endif()
    # Voz: se graba con Piper (voz/grabar.py) en build/voz. Se arma a mano: cmake --build build --target voz
    find_package(Python3 COMPONENTS Interpreter)
    if(Python3_FOUND)
      add_custom_target(voz COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/voz/grabar.py ${CMAKE_BINARY_DIR}/voz)
    endif()
    file(GLOB BETO_FUENTES RELATIVE ${CMAKE_SOURCE_DIR} recursos/fuentes/*)
    qt_add_resources(betometro recursos
      PREFIX "/"
      FILES ${BETO_FUENTES} recursos/img/leon.png recursos/img/inicio.jpg recursos/informe-plantilla.html)
  else()
    message(STATUS "Sin Qt 6: no se compila el programa, solo el núcleo")
  endif()
endif()

# Simulador del NEO-M8N (solo desarrollo, nunca en el equipo)
add_executable(simular_neo simulador/simular_neo.cpp)
target_link_libraries(simular_neo PRIVATE nucleo)

option(BETO_PRUEBAS "Compilar las pruebas" ON)
if(BETO_PRUEBAS)
  enable_testing()

  add_executable(correr_motor pruebas/motor/correr_motor.cpp)
  target_link_libraries(correr_motor PRIVATE nucleo)

  add_executable(correr_informe pruebas/informe/correr_informe.cpp)
  target_link_libraries(correr_informe PRIVATE nucleo)

  add_executable(prueba_nucleo pruebas/nucleo/prueba_nucleo.cpp)
  target_link_libraries(prueba_nucleo PRIVATE nucleo)
  target_compile_definitions(prueba_nucleo PRIVATE BETO_REALES="${CMAKE_SOURCE_DIR}/pruebas/reales")
  add_test(NAME nucleo COMMAND prueba_nucleo)

  if(TARGET betometro)
    add_executable(prueba_datos pruebas/programa/prueba_datos.cpp app/datos.cpp app/datos.h)
    qt_add_resources(prueba_datos plantilla PREFIX "/" FILES recursos/informe-plantilla.html)
    target_include_directories(prueba_datos PRIVATE app)
    target_link_libraries(prueba_datos PRIVATE nucleo Qt6::Core)
    add_test(NAME datos COMMAND prueba_datos)
    add_executable(prueba_crono pruebas/programa/prueba_crono.cpp app/datos.cpp app/datos.h app/crono.cpp app/crono.h app/gps.cpp app/gps.h app/camara.cpp app/camara.h)
    qt_add_resources(prueba_crono plantilla PREFIX "/" FILES recursos/informe-plantilla.html)
    target_include_directories(prueba_crono PRIVATE app)
    target_link_libraries(prueba_crono PRIVATE nucleo Qt6::Core)
    add_test(NAME crono COMMAND prueba_crono)
  endif()

  find_program(NODE node)
  if(NODE)
    add_test(NAME motor_igual_a_la_app
             COMMAND ${NODE} ${CMAKE_SOURCE_DIR}/pruebas/motor/comparar.js $<TARGET_FILE:correr_motor> ${CMAKE_BINARY_DIR})
    add_test(NAME informe_igual_a_la_app
             COMMAND ${NODE} ${CMAKE_SOURCE_DIR}/pruebas/informe/comparar.js $<TARGET_FILE:correr_informe>)
  else()
    message(WARNING "Sin Node: no se compara el motor con el de la app")
  endif()
endif()
