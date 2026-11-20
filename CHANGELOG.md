# 0.9.5 (Última actualización)

- Menú contextual de Presets para `SCULP` al hacer clic derecho sobre `OUTPUT` con los 6 presets 1:1 de `TMC Fobos` (`TMC_Mixbus`, `Clarity`, `Density`, `MachineHead`, `MixHead`, `TMC_Trans_sienten`), `Default (Reset Sculp)` y gestión completa de presets de usuario (guardar, cargar, sobrescribir y borrar).
- Selector de 18 Skins activable con clic derecho sobre la firma `papielguapo` (añadidas `ATARI PC` como skin por defecto, `FL STUDIO`, `FL DARK`, `ABLETON LIVE`, `LOGIC PRO X`, `PRO TOOLS`, `MAGIX SEQUOIA`, `ADOBE AUDITION`, `CUBASE 14`, `REAPER 7`, `WINAMP CLASSIC`, `WINDOWS 98`, `MAC OS X AQUA`, `BLACK STEALTH` y `WHITE STUDIO`).
- Motor de Resampling Quality de FL Studio (`2-point linear` a `512-point sinc`) + opción `Native Daw` e integración del selector de Oversampling (`OS 1x..8x`) a su derecha en la barra superior.
- `COMP VOL` reubicado detrás de `CLIP 0` en la cadena de audio (`IN > SCULP > INFLATE > OUT / CLIP > COMP VOL`), orden de botones actualizado (`MONO · DELTA · CLIP 0 · COMP VOL · [COMP OFF] · BYPASS`) y botón `BYPASS` junto a la fila principal.

# 0.9.4

- COMP VOL: corregida la subida rápida de maquillaje al activar la compensación y la ganancia retenida sobre ruido/silencio.
- Espera inicial de señal 500 ms; subida limitada a 2 dB/s, reducción 20 dB/s y vuelta a neutro en señal bajo −60 dBFS RMS.
- Detector antes de la compensación y del clip final, evitando compensar la pérdida por ese recorte.
- Restauración segura tras pausas, sin asignaciones de memoria en el callback probado. Skins y motor con COMP VOL desactivado conservados.

# 0.9.3

- Skin Ads con la foto completa suministrada, sin recorte ni distorsión de proporciones.
- Seleccionable junto a BLACK y DIAMOND II, guardada con la sesión.
- Fondo ajustado en caché y paneles semitransparentes para legibilidad; sin cambios de audio.

# 0.9.2

- SCULP ON e INFLATE ON junto a la ruta de señal; nombres IN y OUT abreviados en esa ruta.

- SUB y AIR al mismo tamaño que los demás knobs, alineados con sus filas.
- Cajas Hz recolocadas bajo los valores editables, conservando la rueda del ratón.
- Cambio visual; sin modificaciones del motor ni cálculos de medición.

# 0.9.1

- Correlación y Side compactos: barras de 94×7, paso vertical de 10, alineadas con los L/R pequeños de MOD %.
- Etiquetas C/S y valores numéricos a la derecha; sin cambios de cálculo ni de audio.

# 0.9.0

- Densidad L/R entre Even y Odd; cálculo por bloques, solo con interfaz abierta y etapa activa.
- Correlación de fase y porcentaje de energía Side entre Mid y Side, calculados en la interfaz con rechazo de DC a 5 Hz.
- Oversampling 1×/2×/4×/8× abajo a la derecha, guardado con la sesión; 4× por defecto conserva el motor anterior.
- Firma después de OUTPUT; capturas nuevas de BLACK y DIAMOND II en tamaño normal y mínimo.
- Verificados 24 grupos funcionales, 16 renders estéreo idénticos al motor rc7 a 4× y pruebas específicas de medidores/oversampling.

# 0.8.0

- I/O SCALES directo en cabecera, como en BASS; sin botón/panel TOOLS.
- Segundo desplegable SKIN al lado: BLACK / DIAMOND II, guardado por instancia.
- Metal cepillado, mandos morados y firma gráfica original de DIAMOND II; firma clara equivalente en BLACK.
- Retirados controles de presets, opciones avanzadas, RESET, UNDO/REDO y atajos internos.
- Clic en las lecturas de pico borra retenciones/OVER.
- Conservados los controles de audio, rueda Hz, transiciones y las cuatro barras físicas de I/O.

# 0.7.0

- Una opción por evento de rueda convencional en Hz de Sub/Air.
- Acumulación de movimientos pequeños de trackpad, reinicio al invertir/pausar e inercia ignorada.
- Transiciones de 20 ms entre dos filtros estables al cambiar la frecuencia.
- Cambios rápidos agrupados hacia el último destino; coeficientes nominales y sobremuestreo conservados.
- Ventana y controles sin cambios; sin nuevos analizadores.

# 0.6.1

- Rueda del ratón habilitada sobre las cajas Hz de Sub/Air, con límites en las opciones existentes.
- Desactivada junto con SCULP; usa los parámetros/attachments existentes.
- Sin cambios en procesamiento de audio ni diseño de ventana.

# 0.6.0

- Bypass SCULP/INFLATE independiente y automatizable, con transición de 5 ms, ajustes y latencia conservados.
- Controles atenuados cuando su etapa está apagada; etiquetas ON/OFF explícitas.
- OVER L/R retenido hasta RESET, con números/marcos rojos y fondo neutro.
- Estados activos en verde; escucha y compensación activa/suspendida identificadas en la línea inferior.
- Presets/sesiones anteriores activan por defecto los parámetros nuevos.
- Ventana compacta y ausencia de analizadores superiores conservadas.

# 0.5.0

- Retirados panel TOP y todos sus analizadores y medidores, junto con EXPAND, SHOW/HIDE TOP, ECO/HQ y selector de pares.
- Ventana única compacta 450×270..1350×810; escala de los medidores laterales trasladada a TOOLS.
- Fila MONO · DELTA · COMP VOL · CLIP 0.
- Etapas renombradas SCULP e INFLATE; SCULP SWEET SPOT conserva la misma escala nominal.
- Eliminados FFT y cálculos asociados del editor; retirada dependencia y worker de loudness.
- Motor, sobremuestreo, parámetros/UID y ganancias sin cambios intencionados.
