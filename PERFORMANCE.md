# Rendimiento 0.9.3

Ads decodifica el JPEG y lo ajusta a un panel en caché de 900×540 una sola vez, al elegir la skin. La pintura reutiliza ese panel. No añade trabajo al hilo de audio; el coste adicional es gráfico y memoria de imagen en la interfaz.

# Rendimiento 0.9.0

Densidad: sumas de potencias por muestra y suavizado una vez por bloque, sin FFT, solo con editor abierto y Even/Odd activos. Correlación/Side reutilizan el FIFO visual y se calculan en la interfaz. No se han añadido asignaciones C++ al callback en las pruebas. Las imágenes de skin siguen almacenadas en caché.

OS 1× y 2× reducen el trabajo de la ruta SCULP frente a 4×, con más aliasing potencial y diferente límite de frecuencia Air. 8× aumenta ese trabajo y reduce aliasing en esa ruta. INFLATE continúa a frecuencia nativa. El ajuste de OS reconstruye los filtros fuera del callback con el audio suspendido brevemente; no se cambia continuamente durante reproducción. No se promete un porcentaje fijo de CPU del DAW.

# Rendimiento 0.8.0

Las skins se cargan y dibujan solo en el hilo de interfaz. El panel metálico se genera una vez y se guarda como imagen; el mando y la firma se extraen una vez y se reutilizan. No hay recursos gráficos ni cálculos de skin en processBlock. La pintura de DIAMOND II puede costar más que BLACK por textura y mandos rasterizados; no se promete un porcentaje de CPU total. El procesamiento de sonido y el sobremuestreo siguen siendo los de 0.7.0.

# Rendimiento OUTPUT 0.7.0

La rueda se procesa en la interfaz. Los cambios de Hz añaden una segunda ruta del filtro afectado únicamente durante su transición de 20 ms, sin FFT ni asignaciones de memoria. No se promete un porcentaje de CPU de DAW.

## Histórico 0.5.0

El panel superior se ha eliminado, no solo ocultado. El editor ya no construye objetos FFT ni buffers de historial/espectro/transferencia/armónicos. No calcula correlación, goniometría, fase en graves ni loudness. libebur128 y su worker ya no se enlazan; el callback deja de enviar cada muestra a ese medidor.

Se conservan cuatro acumuladores de pico/RMS de las muestras nuevas de I/O. La interfaz los consume a 20 Hz; VU mantiene envolvente de 300 ms, los picos tienen caída visual y retención numérica. La medición RMS ahora abarca el intervalo entre refrescos, en lugar de reconstruir las últimas 4096 muestras de un buffer histórico. Estos cambios afectan a la presentación, no al sonido.

El motor D2F y el sobremuestreo permanecen. Los renders de referencia se comparan byte a byte y se comprueban asignaciones C++ dentro de processBlock. No se afirma un porcentaje total de CPU en un DAW: depende del host, dispositivo, frecuencia y parámetros. Los registros anteriores de analizadores corresponden a versiones antiguas.

## 0.6.0

Los bypass añaden dos rampas de 5 ms y mezclas ligeras en las posiciones de etapa existentes. No hay nuevos analizadores ni FFT. Se conservan estados de filtro durante bypass: apagar una etapa no garantiza menor CPU. Ensayos de transición y compensación confirman cero asignaciones C++ en processBlock. La lectura de estados y retención de OVER ocurre en la interfaz a 20 Hz.

## Rueda 0.6.1
Solo se activa el manejo de eventos de rueda de las dos cajas de frecuencia. No hay nuevos cálculos ni cambios en processBlock. Los knobs de ganancia mantienen la rueda desactivada.

## 0.7.0

Se conservan las mismas operaciones de filtro cuando no hay transición. Durante 20 ms se ejecuta también el filtro anterior de la sección afectada y se mezcla su salida; el coste aumenta temporalmente en los cambios de Hz. No hay interpolación de coeficientes ni nuevas FFT. Las copias de estados son estructuras fijas, sin reservas de memoria en audio; comprobado durante automatización rápida. No se promete un porcentaje de CPU del DAW.

