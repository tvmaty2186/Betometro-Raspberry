# Historial de versiones y decisiones

Lo que pidió Matías y cómo se resolvió. Lo más nuevo está arriba.

| Versión | Qué cambió |
|---|---|
| **2.1.1** | Al actualizar arranca otra vez en Avispón Verde, una sola vez (bandera `avispon21`). |
| **2.1.0** | Inicio del Avispón con cabecera (león y título cromado) y la foto de los pilotos con el auto. La foto es fija, no se puede cambiar. |
| **2.1.0** | Foto de la app en el inicio (el piloto y el copiloto con el 404), la misma en **todos los temas**. Es fija: **no se puede cambiar** desde la app (decisión de Matías). En Avispón: cabecera verde con león, rayita, título cromado y rayas; la foto se funde en verde y el 101 va montado sobre la banda. |
| **2.0.0** | **Tema Avispón Verde** (Peugeot 404 #101, león de Matías, cromo, banda 101, velocímetro crema). Al actualizar pasa a este tema **una sola vez**; después se respeta el tema elegido. Nuevo ajuste «Número del auto». |
| 1.9.1 | Sin pitido del cero, la cuenta dice «cero» en voz. |
| 1.9.0 | Abrir al prender el teléfono (apagado de fábrica). Clic mecánico al marcar. Pitido del cero apagado de fábrica. |
| 1.8.2 | Se sacan las exportaciones CSV/JSON. Desde el informe y el detalle, atrás vuelve siempre a la lista de Carreras. Las carreras terminadas pasan a «Corridas». Flujo completo probado. |
| 1.8.1 | Informe: scroll cómodo, barra a la derecha, favicon, imagen de vista previa y nombre «🏁 nombre dd-mm-aaaa». |
| 1.8.0 | Grabación del recorrido GPS por vuelta. Informe con mapa armado con todas las vueltas, velocidad, aceleración y tabla de vueltas. Visor dentro de la app y compartir en HTML. |
| 1.7.4–1.7.5 | Pantallas de fábrica con Última vuelta, alineada justo entre la cuenta y MARCAR (como en las fotos de Matías). |
| 1.7.3 | El widget Última vuelta muestra el tiempo medido. |
| 1.7.2 | Cuenta regresiva hablada de 20 s de fábrica. |
| 1.7.1 | Velocímetro cuadrado digital con barra contra la sugerida (±10 %, mínimo 5 km/h). El centro de la barra es dinámico (sigue a la sugerida). |
| 1.7.0 | Pantalla horizontal con su propio armado. |
| 1.6.0 | Errores en h:mm:ss. Las puntas de la barra equivalen al 10 % de la vuelta. |
| 1.5.9 | Barra de adelanto/atraso en la cuenta (proporcional al tiempo y la distancia; adelantado a la derecha). Velocímetro hasta 160. |
| 1.5.6–1.5.8 | Pantalla protegida en carrera: los botones de volumen no hacen nada y la pantalla vuelve sola. Fijar la app: agregado y **sacado** a pedido. |
| 1.5.5 | Editor: distancia antes que tiempo, velocidad promedio en vivo, sin «lugar». Ficha simétrica. |
| 1.5.3 | «· Actualizar» en amarillo al lado de la versión. «Necesaria» pasa a llamarse **Velocidad sugerida**. La cuenta muestra los metros que faltan. El menú del editor va dentro del panel ✎. |
| 1.5.2–1.5.4 | La práctica toma el tiempo editado. Los segundos de más se normalizan (90 s pasa a 1:30). |
| 1.5.1 | Instalación directa desde el cartel del inicio. Aviso diario en segundo plano. |
| 1.5.0 | Ícono propio, compartir la app, reglas de largada. |
| 1.4.x | Nueva línea de diseño en todos los menús. Ajustes de fábrica elegidos por Matías. |
| 1.3.x | Panel de instrumentos realista, editor libre de widgets, pantallas a la medida del teléfono. |
| 1.2.x | Temas, widgets configurables, sonido al máximo. |
| 1.1.x | Inicio, carreras y base de datos. Práctica libre. |
| 1.0 | App nativa Android con Capacitor (antes era una versión web de prueba). |

## Decisiones que conviene recordar

- **Fijar la app:** no. Se probó y Matías lo hizo sacar.
- **Exportar CSV/JSON:** no. Solo el informe HTML.
- **Play Protect:** no apelar ni sacar permisos. Se instala apagando el análisis un momento.
- **Foto del inicio:** fija (`www/img/inicio.jpg`), igual en todos los temas; no se puede elegir otra.
- **Pitido del cero:** apagado de fábrica. El cero se dice en voz.
- **Auriculares Bluetooth:** por ahora sin cambios en la app.
- **ESP32:** por ahora no.
- **Caja con Galaxy A37:** cancelada (01/11/2026). El equipo dedicado sigue por la Raspberry Pi.
- **Antes de construir algo visual:** mostrar una imagen de muestra.
