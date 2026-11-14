# isaac-highfps

**Alta tasa de refresco para The Binding of Isaac: Repentance+**: juega a 144, 165, 240 Hz o a lo que dé tu monitor, con la lógica del juego funcionando **exactamente igual que el original**.

[English](README.md)

El juego actualiza su lógica 30 veces por segundo y dibuja 60 frames. Este mod deja la lógica intacta (misma velocidad, daño, invencibilidad, RNG y respuesta a los controles) y dibuja tantos frames como pueda mostrar tu monitor, colocando cada entidad, a Isaac y la cámara donde estarían en ese instante exacto.

## Instalación (1 minuto)

1. **[Descarga `winmm.dll`](https://github.com/julenster/isaac-highfps/releases/latest)** de la última versión.
2. En Steam: clic derecho en **The Binding of Isaac: Rebirth** → **Administrar** → **Ver archivos locales**.
3. Copia `winmm.dll` en esa carpeta.
4. **Solo Linux / Steam Deck:** clic derecho en el juego → **Propiedades** → **General** → **Opciones de lanzamiento**, y pega:
   ```
   WINEDLLOVERRIDES="winmm=n,b" %command%
   ```

Ya está. Abre el juego como siempre. Ten el **VSync activado** en las opciones del juego para que los FPS sigan a tu monitor.

**Desinstalar:** borra `winmm.dll` (y en Linux, la opción de lanzamiento).

## Ajustes (opcional)

Pon [`highfps.ini`](highfps.ini) junto a `winmm.dll` si quieres cambiar algo:

| Clave | Por defecto | |
|---|---|---|
| `enabled` | `1` | `0` desactiva el mod sin desinstalarlo |
| `max_fps` | `0` | Límite de FPS. `0` = sin límite (decide el VSync) |
| `log_fps` | `0` | `1` escribe los FPS y la velocidad de la lógica en `highfps.log` |

## Compatibilidad

- **Repentance+ v1.9.7.17** (la versión actual de Steam). Con cualquier otra versión el mod lo detecta, **se desactiva solo** y apunta el motivo en `highfps.log`; el juego funciona con normalidad.
- Probado en Linux (Proton Experimental) con un monitor de 240 Hz y limitado a 90 y 144 FPS. En Windows se usa el mismo archivo y el mismo método de carga, pero **aún no está probado**: se agradecen los informes.
- Compatible con mods de Lua. No probado junto a REPENTOGON.
- **Online: sin probar.** El mod no cambia la lógica del juego, así que no debería desincronizar, pero por si acaso úsalo en partidas locales.

## Cómo funciona

`winmm.dll` es un proxy: reenvía todo a la `winmm.dll` real de Windows y, al arrancar el juego, lo parchea en memoria (no modifica ningún archivo del juego):

1. El bucle principal deja de limitarse a 60 vueltas por segundo.
2. El update original del juego se ejecuta por reloj, exactamente 60 veces por segundo real, sea cual sea la tasa de frames.
3. En cada frame dibujado, las entidades se colocan en la fracción correcta del tick actual. El mod restaura el estado de cada entidad tras el último tick, llama al `Interpolate()` del propio juego (que avanza medio tick) y escala el resultado. Funciona para las 9 variantes de interpolación (lágrimas, láseres, cuchillos, efectos y gore...) sin parchearlas.
4. Isaac y la cámara se interpolan entre sus dos últimas posiciones reales, así que nunca se pasan.
5. Antes de cada update real del juego se restaura exactamente el estado que el juego espera.

## Compilar

```
make        # necesita i686-w64-mingw32-gcc (Arch: mingw-w64-gcc, Debian/Ubuntu: gcc-mingw-w64-i686)
make dist   # zip con la dll, el ini y el readme
```

Los binarios de las releases los compila GitHub Actions a partir de este código; cada release incluye las sumas SHA-256.

## Créditos

- [REPENTOGON](https://github.com/TeamREPENTOGON/REPENTOGON): su trabajo de ingeniería inversa fue una gran referencia para localizar funciones. No incluye código de REPENTOGON.
- Sin relación con Edmund McMillen ni Nicalis. No incluye ni modifica archivos del juego.

## Licencia

[MIT](LICENSE)
