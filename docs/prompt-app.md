# Prompt para la app del celular (02/11/2026)

Para pegar en una sesión nueva de Claude Code con el repo **tvmaty2186/Betometro**. Si algún punto no lo querés en la app, borralo antes de pegar.

---

Hola. Quiero llevar a la app Betómetro unos cambios que ya están hechos y probados en la versión para Raspberry (repo tvmaty2186/Betometro-Raspberry, rama ccr-e460edcc-aowt5f; se puede leer para copiar, no se toca). Leé primero el CLAUDE.md de este repo y seguí sus reglas (español rioplatense, corto, commits a mi nombre, subir versión e historial).

Antes de programar, mostrame una imagen de muestra de cada pantalla que cambie y esperá mi OK.

1. **Sin cantidad de vueltas.** Sacar la opción «Vueltas» de cada carrera. En la carrera pueden anunciar vueltas extra, así que no hay límite: se termina a mano. Sin «Última vuelta» ni «Carrera completa». La carrera pasa a «corridas» cuando tiene vueltas y su fecha ya pasó; el mismo día se puede volver a largar. Las carreras viejas con `vueltasTotal` se siguen leyendo, pero el número no se usa.

2. **MEDIDA / LIBRE en el informe.** En la tabla «Todas las vueltas», cada vuelta muestra una etiqueta: MEDIDA (roja) o LIBRE (verde). Se toca y cambia. Las interrumpidas no cambian. Se guarda al instante. También en el informe HTML que se comparte.

3. **Las libres nunca suman error.** Ni las del patrón ni las marcadas a mano. El error total, el promedio y la mejor se recalculan enseguida.

4. **Unir una vuelta con la siguiente (marca de más).** Caso real: en una práctica, dos marcas seguidas partieron una vuelta en una de 4,1 s y otra de 106,3 s. En el informe se toca una vuelta y arriba de la tabla aparece «Unir V5 con V6»; pide tocar dos veces («¿Seguro? Tocá de nuevo»). Al unir: tiempo y distancia se suman, el recorrido (traza) de la segunda sigue al de la primera (tiempo y distancia corridos), dif = real − ideal, velocidad promedio = distancia / tiempo, máxima = la mayor; queda medida/libre de la primera; las siguientes de esa largada se renumeran. No se une si son de distinta largada o alguna es interrumpida. Copiá la lógica de `Carrera::unirConSiguiente` en `app/nucleo/modelo.cpp` del repo de la Raspberry. Prueba: el informe real `pruebas/reales/practica-2026-10-31-beto.html` de ese repo; al unir la V5 con la V6 tiene que dar 1:50,5 (+0,5 s) y 8 vueltas.

5. **Cronómetro de la vuelta: progresiva o regresiva.** El widget de la cuenta regresiva pasa a llamarse «Cronómetro de la vuelta» y tiene dos modos: **progresiva** (de fábrica: cuenta para arriba desde la marca, 0:00.0 → 1:50.0, etiqueta «Vuelta»; al pasar el ideal se pone rojo y dice «Pasado el ideal») o **regresiva** (como hoy). Se cambia en el editor de pantalla con un botón «Cambiar a cuenta regresiva / progresiva». Mismas funciones en los dos: dato extra («faltan X m») y barra ±. Sin modo guardado = progresiva.

6. **Tiempo ideal siempre a la vista.** Chiquito, en la esquina de arriba a la derecha del cronómetro: «IDEAL 1:50,0», en color suave.

7. **«Libre» en vez de «práctica» para las vueltas no medidas.** En la pantalla de carrera (redondel, «Próxima vuelta»), en la voz («Vuelta 2, libre») y en el informe. La «Práctica libre» (la sesión sin carrera) queda igual.

No hagas aviso en voz de «marca doble» durante la carrera: no lo quiero.

Al terminar: pruebas, versión nueva, historial, y pasame el APK para instalar.
