cmake_minimum_required(VERSION 3.16)
project(betometro VERSION 1.0.46 LANGUAGES CXX)

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
# Datos: la clase está repartida por tema (datos.h dice qué va en cada archivo). La usan el programa y las pruebas.
set(BETO_DATOS app/datos.h app/datos_comun.h app/datos.cpp app/datos_resumen.cpp app/datos_informe.cpp app/datos_videos.cpp
    app/datos_config.cpp app/datos_guardado.cpp app/datos_muestra.cpp)
if(BETO_PROGRAMA)
  find_package(Qt6 6.5 QUIET COMPONENTS Quick Svg Multimedia Network)
  if(Qt6_FOUND)
    qt_standard_project_setup(REQUIRES 6.5)
    qt_add_executable(betometro app/main.cpp app/contexto.h ${BETO_DATOS} app/crono.h app/crono.cpp app/gps.h app/gps.cpp app/qr.h app/qr.cpp app/wifi.h app/wifi.cpp app/servidor.h app/servidor.cpp app/control_remoto.h app/control_remoto.cpp app/pulsador_gpio.h app/pulsador_gpio.cpp app/reportes.h app/reportes.cpp app/salud.h app/salud.cpp app/vigilante.h app/vigilante.cpp app/clima.h app/clima.cpp app/sistema.h app/audio.h app/audio.cpp app/simulacion.h app/simulacion.cpp app/prueba_completa.h app/prueba_completa.cpp app/tienda.h app/tienda.cpp
      app/camara.h app/camara.cpp app/actualizador.h app/actualizador.cpp app/hdmi.h app/hdmi.cpp app/imagen_video.h app/imagen_video.cpp app/video_gst.h app/video_gst.cpp
      app/drive.h app/drive.cpp app/drive_logica.h app/drive_logica.cpp
      app/terceros/qrcodegen/qrcodegen.cpp)
    target_compile_definitions(betometro PRIVATE BETO_VERSION="${PROJECT_VERSION}")
    add_subdirectory(app/qml)
    install(TARGETS betometro RUNTIME DESTINATION bin)
    # Pantallas para eglfs_kms (la de 5" manda; la tele por HDMI a 1280×720): /usr/share/betometro/kms.json
    install(FILES raspios/kms.json DESTINATION share/betometro)
    target_link_libraries(betometro PRIVATE nucleo Qt6::Quick Qt6::Svg Qt6::Multimedia Qt6::Network betometro_qmlplugin)
    target_compile_options(betometro PRIVATE -Wall -Wextra)   # también en el programa (02-2)
    # Colores del video por el chip gráfico (app/shaders, ImagenVideo). Para compilar hace falta Qt Shader
    # Tools (qsb; en Debian qt6-shadertools-dev) y las cabeceras de QRhi (qt6-base-private-dev); al equipo no.
    # Los programas llevan variante GLSL ES 100: la Pi 3 solo tiene OpenGL ES 2. Sin eso: por el procesador.
    find_package(Qt6 QUIET COMPONENTS ShaderTools)
    if(Qt6ShaderTools_FOUND AND EXISTS "${QT6_INSTALL_PREFIX}/${QT6_INSTALL_HEADERS}/QtGui/${Qt6Gui_VERSION}/QtGui/rhi/qrhi.h")
      qt_add_shaders(betometro "video_shaders"
        PREFIX "/shaders" BASE app/shaders BATCHABLE
        GLSL "100es,120,150" HLSL 50 MSL 12
        FILES app/shaders/yuv.vert app/shaders/yuv.frag)
      target_link_libraries(betometro PRIVATE Qt6::GuiPrivate)
      target_compile_definitions(betometro PRIVATE BETO_VIDEO_CHIP)
    else()
      message(STATUS "Sin Qt Shader Tools o sin QRhi: el video pasa a colores por el procesador")
    endif()
    # Ayudante de video y sonido (GStreamer, proceso aparte). Si no hay GStreamer, la cámara no anda.
    find_package(PkgConfig QUIET)
    if(PkgConfig_FOUND)
      pkg_check_modules(GST QUIET IMPORTED_TARGET gstreamer-1.0 gstreamer-app-1.0 gstreamer-video-1.0)
    endif()
    if(GST_FOUND)
      add_executable(betometro-camara app/camara/betometro_camara.cpp)
      target_compile_options(betometro-camara PRIVATE -Wall -Wextra -Wno-deprecated-declarations)
      target_link_libraries(betometro-camara PRIVATE PkgConfig::GST)
      # El reproductor del informe también usa GStreamer (para el descompresor de video de la Raspberry)
      target_link_libraries(betometro PRIVATE PkgConfig::GST)
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
      FILES ${BETO_FUENTES} recursos/img/leon.png recursos/img/inicio.jpg recursos/informe-plantilla.html
            recursos/web/index.html recursos/web/drive.html recursos/web/control.html recursos/web/prueba.html recursos/web/ayuda.html recursos/web/personalizar.html recursos/manual.pdf
            recursos/img/betometro-icono-512.png recursos/img/betometro-icono-redondo-512.png)
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
    add_executable(prueba_datos pruebas/programa/prueba_datos.cpp ${BETO_DATOS})
    qt_add_resources(prueba_datos plantilla PREFIX "/" FILES recursos/informe-plantilla.html)
    target_include_directories(prueba_datos PRIVATE app)
    target_link_libraries(prueba_datos PRIVATE nucleo Qt6::Core)
    add_test(NAME datos COMMAND prueba_datos)
    add_executable(prueba_crono pruebas/programa/prueba_crono.cpp ${BETO_DATOS} app/crono.cpp app/crono.h app/gps.cpp app/gps.h app/camara.cpp app/camara.h)
    qt_add_resources(prueba_crono plantilla PREFIX "/" FILES recursos/informe-plantilla.html)
    target_include_directories(prueba_crono PRIVATE app)
    target_link_libraries(prueba_crono PRIVATE nucleo Qt6::Core)
    add_test(NAME crono COMMAND prueba_crono)
    add_executable(prueba_videos pruebas/programa/prueba_videos.cpp ${BETO_DATOS} app/gps.cpp app/gps.h app/camara.cpp app/camara.h)
    qt_add_resources(prueba_videos plantilla PREFIX "/" FILES recursos/informe-plantilla.html)
    target_include_directories(prueba_videos PRIVATE app)
    target_link_libraries(prueba_videos PRIVATE nucleo Qt6::Core)
    add_test(NAME videos COMMAND prueba_videos)
    add_executable(prueba_drive pruebas/programa/prueba_drive.cpp app/drive_logica.cpp app/drive_logica.h)
    target_include_directories(prueba_drive PRIVATE app)
    target_link_libraries(prueba_drive PRIVATE Qt6::Core)
    add_test(NAME drive COMMAND prueba_drive)
    add_executable(prueba_clima pruebas/programa/prueba_clima.cpp app/clima.cpp app/clima.h)
    target_include_directories(prueba_clima PRIVATE app)
    target_compile_definitions(prueba_clima PRIVATE BETO_VERSION="${PROJECT_VERSION}" BETO_PRUEBA_DIR="${CMAKE_SOURCE_DIR}/pruebas/programa")
    target_link_libraries(prueba_clima PRIVATE Qt6::Core Qt6::Network)
    add_test(NAME clima COMMAND prueba_clima)
    add_executable(prueba_tienda pruebas/programa/prueba_tienda.cpp app/tienda.cpp app/tienda.h)
    target_include_directories(prueba_tienda PRIVATE app)
    target_compile_definitions(prueba_tienda PRIVATE BETO_VERSION="${PROJECT_VERSION}")
    target_link_libraries(prueba_tienda PRIVATE Qt6::Core Qt6::Network)
    add_test(NAME tienda COMMAND prueba_tienda)
    # Carrera de punta a punta: simular_neo por un puerto serie virtual, GpsNeo de verdad y el Crono (09 P3)
    add_executable(prueba_carrera pruebas/programa/prueba_carrera.cpp ${BETO_DATOS} app/crono.cpp app/crono.h app/gps.cpp app/gps.h app/camara.cpp app/camara.h)
    qt_add_resources(prueba_carrera plantilla PREFIX "/" FILES recursos/informe-plantilla.html)
    target_include_directories(prueba_carrera PRIVATE app)
    target_link_libraries(prueba_carrera PRIVATE nucleo Qt6::Core)
    add_test(NAME carrera_simulada COMMAND prueba_carrera $<TARGET_FILE:simular_neo>)
    set_tests_properties(carrera_simulada PROPERTIES TIMEOUT 120)
    # Wifi con un nmcli de mentira (pruebas/dobles) primero en el PATH (09 P4)
    add_executable(prueba_wifi pruebas/programa/prueba_wifi.cpp app/wifi.cpp app/wifi.h)
    target_include_directories(prueba_wifi PRIVATE app)
    target_compile_definitions(prueba_wifi PRIVATE BETO_DOBLES="${CMAKE_SOURCE_DIR}/pruebas/dobles")
    target_link_libraries(prueba_wifi PRIVATE Qt6::Core Qt6::Network)
    add_test(NAME wifi COMMAND prueba_wifi)
    # Capturas comparadas contra pruebas/capturas (09 P2): necesita faketime (la fecha y la hora quedan fijas)
    add_executable(comparar_capturas pruebas/capturas/comparar_capturas.cpp)
    target_link_libraries(comparar_capturas PRIVATE Qt6::Gui)
    find_program(FAKETIME faketime)
    if(FAKETIME)
      add_test(NAME capturas COMMAND bash ${CMAKE_SOURCE_DIR}/herramientas/comparar-capturas.sh $<TARGET_FILE:betometro> $<TARGET_FILE:comparar_capturas> ${CMAKE_BINARY_DIR}/capturas)
      set_tests_properties(capturas PROPERTIES TIMEOUT 600)
    else()
      message(WARNING "Sin faketime: no se comparan las capturas (apt-get install faketime)")
    endif()
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
