# Nuevas mediciones 0.9.0

DENSITY es 100*r/(1+r), con r=RMS(contribución no lineal de Even/Odd)/RMS(componente lineal), promediando potencias durante 300 ms. Incluye el fundacional y DC de la contribución y no es THD. Cero con SCULP desactivado. No representa armónicos añadidos por etapas posteriores.

CORR OUT y Side usan la salida final con rechazo DC a 5 Hz. CORR requiere ambos canales con energía; Side=100*E(S)/(E(M)+E(S)). Sin energía se muestra --. Side 50% puede ser decorrelación o audio de un solo canal. Los colores son guías visuales; no certifican calidad de fase.

# Referencias de los medidores laterales

dBFS: pico de muestra, rango visual −60..+9, caída 30 dB/s y retención numérica 2 s. LINEAR: amplitud RMS. VU: envolvente RMS 300 ms y cero −18 dBFS RMS. K-12/14/20: referencia RMS compensada para seno; un seno con pico −12/−14/−20 dBFS marca cero nominal respectivamente.

SCULP SWEET SPOT conserva escalones nominales −15/−12/−9/−6/−3/0/+3/+6/+9 dBFS, SP en −12. No reproduce exactamente el detector ni la balística de Fobos. MOD % conserva una fórmula propia de modificación de Inflate, no una calibración Sonnox ni ganancia reducida.

No existen agujas ni analizadores superiores en 0.5.0. Los medidores no alteran el audio. COMP VOL sí cambia la ganancia cuando se activa; su referencia es RMS, no LUFS.
