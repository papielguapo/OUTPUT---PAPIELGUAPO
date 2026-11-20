# Auditoría 0.9.4

Comparación reproducible contra el ejecutable 0.9.3 conservado: tono 997 Hz durante 8 s seguido de ruido/señal residual de amplitud 1e-6 durante 2 s, Output −12 dB, Clip OFF, COMP VOL ON. Maquillaje retenido medido en el último cuarto de segundo: antes +11.9863 dB; después −0.0137 dB (diferencia residual del motor, prácticamente neutro). Archivo Validation/autolevel-before-after.json.

Con COMP VOL apagado, 16 renders estéreo a 48 kHz continúan idénticos bit a bit a rc7 (PRE/POST × Band Split × Clip × controles neutros/activos). Interfaz y persistencia de las tres skins aprobadas; mediciones y OS 1/2/4/8 aprobados. El resultado no equivale a una prueba en el DAW ni descarta un bucle acústico externo.

Prueba específica COMP VOL: activación con 500 ms sin maquillaje, límite de subida 2 dB/s, convergencia, retorno a neutro sobre ruido −120 dBFS, nueva espera al volver la señal y cero asignaciones C++ durante callbacks. Combinaciones 44.1/48/96/192 kHz × bloques 17/128/1024 aprobadas. También 24 grupos funcionales aprobados. No se atribuye al plugin una protección frente a un bucle acústico externo: esta corrección evita la subida automática rápida y la amplificación retenida de ruido.

# Auditoría 0.9.3

Solo nueva skin y estado visual; no se modifica PluginProcessor. Se comprueba restauración de las tres skins, límites de ventana y controles, con capturas Ads a 900×540 y 450×270. La foto incorporada conserva los bytes del archivo suministrado.

# Auditoría 0.9.2

Cambio de tamaño/posición de controles únicamente. Motor y cálculos de medición sin cambios respecto a 0.9.1. Capturas reales BLACK y DIAMOND II comprobadas a 900×540 y 450×270. Pruebas de interfaz y rueda Hz aprobadas. SCULP ON/INFLATE ON recolocados junto a la ruta abreviada IN/OUT.

# Auditoría 0.9.1

Cambio de geometría y etiquetas de correlación/Side exclusivamente en pintura. El motor y los cálculos de medición son los de 0.9.0. Capturas reales en ambas skins y tamaño mínimo.

# Auditoría 0.9.0

pluginval nivel 10: SUCCESS. VST3 instalado en Common Files/VST3 y verificado por SHA-256.

24 grupos funcionales aprobados; pruebas de interfaz y rueda Hz aprobadas. Los 16 renders estéreo de referencia siguen idénticos bit a bit al motor rc7 a 4×. La captura de densidad activa/desactivada también produce muestras idénticas en PRE y POST y cero asignaciones C++ en el callback medido. Densidad L/R aislada, etapa desactivada y editor cerrado verificados. Correlación/Side comprobados con mono, antifase, cuadratura, un canal y silencio. Oversampling 1/2/4/8: salida finita, latencia dentro del buffer, cambio en vivo y restauración de sesión comprobados. Las capturas provienen del editor JUCE real, en ambas skins.

# Auditoría 0.8.0

Interfaz simplificada: selector de escalas directo y selector de skin adyacente, sin TOOLS, RESET, gestor de presets ni botones UNDO/REDO. Los atajos internos de undo/redo se han retirado. Los tests de APIs heredadas se conservan para compatibilidad de estado; no significan que esas funciones estén expuestas en la interfaz.

Se comprueban ambas skins, límites de componentes a 450/900/1350 px, persistencia por instancia/sesión, recursos gráficos cargados, clic en números de pico para borrar avisos y lectura de las siete escalas. La migración de estados antiguos evita quedarse en un Solo/bypass global retirado de la interfaz y conserva la escala de I/O.

La skin DIAMOND II reutiliza el mismo recorte de mando (508,91,74,74) y la misma extracción alfa de firma (1517,108,162,51) que el proyecto original. Textura, mando y firmas se cachean en la interfaz; no se procesa ningún recurso gráfico en audio. Se mantienen los colores sólidos de barras en ambos temas.

Los registros skins-*.txt e io-scales-*.txt son de esta revisión; los apartados siguientes son históricos y pueden mencionar controles ya retirados.

# Auditoría 0.7.0

24 grupos funcionales PASS. Pruebas nuevas: cambios de Hz Air/Sub en ambos sentidos a 48/192 kHz sobre señal continua, con salto entre muestras inferior al de coeficientes cambiados abruptamente en los ocho ensayos. No se deduce ausencia universal de clics para cualquier material o automatización.

Automatización rápida a 44.1/192 kHz: cero asignaciones C++ en callback y destino final con coeficientes activos exactamente iguales a la referencia, sin transición pendiente. Diferencia RMS final de los ensayos: 0.000134 y 0.000483 dB. Las historias de estados de los filtros originales en coma flotante no terminan necesariamente en muestras idénticas después de trayectorias distintas: máximo residual observado 0.00001447 a 44.1 kHz y 0.000117136 a 192 kHz. Son resultados de esa señal de prueba, no especificaciones de precisión general.

Frecuencias fijas: 16 renders estéreo idénticos byte a byte al rc7 en los casos habituales. Wheel/UI PASS: pasos discretos, acumulación de trackpad, inversión, extremos, inercia y SCULP apagado. Registros frequency-*.txt de esta entrega; las siguientes secciones son históricas.

# Auditoría 0.6.1

Cambio de interfaz: scroll nativo de JUCE ComboBox en Sub/Air. Se comprueba que los eventos de rueda cambian tanto la opción visible como el parámetro del plugin en ambos sentidos, que respetan los extremos y que no actúan con SCULP apagado. Motor sin cambios. Los registros wheel-*.txt pertenecen a esta entrega; las pruebas detalladas de bypass que siguen son de 0.6.0.

# Auditoría 0.6.0

22 grupos funcionales aprobados. Nuevos ensayos: aislamiento del bypass de cada etapa en PRE y POST, conservación de ajustes, transiciones con latencia estable y cero asignaciones C++ en callback, y presets antiguos sin flags que reactivan ambas etapas. Mayor variación entre muestras consecutivas en la prueba de DC/transiciones: 0.000183012; es evidencia del ensayo, no garantía de ausencia de cualquier artefacto con cualquier material.

Pruebas UI: avisos de sobrecarga retenidos y RESET, controles de etapas apagadas atenuados/inactivos sin cambiar ajustes, Input/Output disponibles y compensación indicada como suspendida en Delta/Solo. 16 renders estéreo con ambas etapas activas idénticos byte a byte al rc7 de referencia; no se cambia sobremuestreo ni ruta original cuando los mixes están a 1.

La mezcla de bypass conserva el sobremuestreo y mantiene los filtros internos actualizados. Los avisos de OVER son de pico de muestra y solo se capturan con editor abierto. No equivalen a true peak ni a una medición LUFS. Los registros stages-*.txt son de esta versión; los siguientes documentan 0.5.0 y versiones anteriores.

# Auditoría 0.5.0

Cambio de alcance solicitado: versión simple sin analizadores superiores ni EXPAND. Se conservan los cuatro medidores físicos de I/O y MOD % bajo los controles. El código de FFT/análisis superior y la dependencia libebur128 se han retirado de la compilación.

Se verifica tamaño compacto y reescalado, ausencia de controles de TOP/EXPAND, límites de TOOLS, orden de botones, escalas con barras de colores, pico en todo el intervalo de refresco a 192 kHz, RMS independiente, undo/redo y restauración de sesiones en formato compacto. La comparación contra rc7 y las pruebas funcionales cubren el motor.

Los registros simple-*.txt son los de esta entrega. Otros logs conservados en Validation documentan versiones anteriores y sus funciones ya retiradas; no describen la interfaz actual.

Limitaciones: no hay LUFS, true peak ni otros analizadores superiores en esta versión. VU/K y Sweet Spot laterales son referencias visuales sin certificación; MOD % es una fórmula propia. Clip genera armónicos. No se ha validado esta entrega dentro de una sesión real de tu DAW.

Resultados finales: ventana/medidores/orden de botones PASS; 19 grupos funcionales PASS; 16 renders estéreo idénticos byte a byte al rc7 con funciones nuevas apagadas; pluginval nivel 10 SUCCESS (salida 0).

Instalación 0.5.0 verificada en Common Files/VST3 por SHA-256 coincidente con el binario entregado. La versión anterior queda guardada fuera de la carpeta escaneada.

Entrega final 0.6.0: UI y 22 grupos funcionales PASS; 16 renders estéreo idénticos al rc7 con ambas etapas activas y funciones adicionales apagadas; pluginval nivel 10 SUCCESS (salida 0). Instalación Common Files/VST3 verificada por versión 0.6.0 y SHA-256 coincidente con el binario entregado.

Entrega 0.6.1: rueda y UI PASS; 16 renders de referencia idénticos byte a byte con ajustes equivalentes; pluginval nivel 10 SUCCESS. Instalado en Common Files/VST3, versión 0.6.1 y SHA-256 coincidente con la entrega.

Entrega 0.7.0: rueda, UI y 24 grupos funcionales PASS; 16 renders de referencia con frecuencias fijas idénticos byte a byte; pluginval nivel 10 SUCCESS. Instalado en Common Files/VST3, versión 0.7.0 y SHA-256 coincidente con la entrega.

Entrega 0.8.0: ambas skins y su persistencia, selector directo, ausencia de TOOLS/RESET/UNDO/REDO/presets y clic de limpieza PASS; rueda PASS; 24 grupos funcionales PASS; 16 renders de referencia idénticos byte a byte con frecuencias fijas; pluginval nivel 10 SUCCESS (salida 0).

Instalación verificada en Common Files/VST3: versión 0.8.0 y SHA-256 coincidente con el binario de la entrega. Versión anterior guardada fuera de la carpeta escaneada.
