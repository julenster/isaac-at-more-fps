# Isaac at More FPS

**Hace que The Binding of Isaac: Repentance+ se vea tan fluido como permita tu monitor (144, 165, 240 Hz…) sin cambiar cómo se juega.**

[Read in English](README.md)

## ¿Qué es esto? (versión corta)

Isaac solo dibuja 60 imágenes por segundo. Aunque tengas un monitor de 144 o 240 Hz, el juego se sigue viendo a 60.

Este mod hace que el juego dibuje tantas imágenes por segundo como pueda mostrar tu pantalla. Todo se mueve más suave: Isaac, las lágrimas, los enemigos, la cámara.

**El juego en sí no cambia nada.** Misma velocidad, mismo daño, mismos controles, mismas semillas. Solo se ve más fluido.

## Instalación

**[⬇ Descarga isaac-at-more-fps.zip](https://github.com/julenster/isaac-at-more-fps/releases/latest/download/isaac-at-more-fps.zip)** y descomprímelo.

### Windows
Doble clic en **`install.bat`**. Listo, a jugar.

> Windows puede mostrar un aviso de seguridad porque el archivo viene de internet. Pulsa *Más información → Ejecutar de todas formas*. El código es público y GitHub compila el archivo automáticamente a partir de él (ver [¿Es seguro?](#es-seguro)).

### Linux / Steam Deck
1. Abre una terminal en la carpeta descomprimida (Steam Deck: modo escritorio, clic derecho en la carpeta → *Abrir terminal aquí*) y ejecuta:
   ```
   sh install-linux.sh
   ```
2. En Steam: clic derecho en el juego → **Propiedades** → **General** → **Opciones de lanzamiento**, y pega:
   ```
   WINEDLLOVERRIDES="winmm=n,b" %command%
   ```

### Instalación manual (cualquier sistema)
Copia `winmm.dll` en la carpeta del juego (Steam → clic derecho en el juego → **Administrar** → **Ver archivos locales**). En Linux / Steam Deck añade también la opción de lanzamiento de arriba.

**Consejo:** activa el **VSync** en las opciones del juego para que los FPS sigan a tu monitor.

### Desinstalar
Windows: doble clic en `uninstall.bat`. Linux: `sh install-linux.sh uninstall` y borra la opción de lanzamiento. O simplemente borra `winmm.dll` de la carpeta del juego.

## Preguntas frecuentes

**¿Cambia la jugabilidad?**
No. La lógica del juego funciona exactamente igual que en el original y a la misma velocidad. El mod solo cambia lo que ves entre los cálculos del propio juego.

**¿Funciona con mis mods? ¿Y los logros?**
No es un mod de Lua: el juego no lo ve en la lista de mods y funciona junto a tus mods habituales. Los logros no se tocan.

**¿Y el online?**
Aún no está probado. No debería desincronizar porque no cambia la lógica del juego, pero por si acaso úsalo en partidas locales.

**El juego se ha actualizado y ya no funciona.**
El mod comprueba la versión del juego al arrancar. Si no coincide, **se desactiva solo** y el juego funciona con normalidad. Espera a que salga una actualización del mod.

**¿Y si mi monitor es de 60 Hz?**
Entonces no ganas nada, pero tampoco se rompe nada.

<a name="es-seguro"></a>**¿Es seguro?**
Todo el código está en este repositorio. La descarga la compila GitHub Actions directamente a partir de ese código, y cada release incluye sus sumas SHA-256. El mod no modifica ningún archivo del juego ni se conecta a internet.

## Cómo funciona

### Versión sencilla
Cada segundo, el juego calcula 60 veces dónde está cada cosa: eso es "la lógica del juego". Entre esos momentos no sabe dónde está nada, así que no puede dibujar más imágenes.

El mod no toca esos 60 cálculos. Cuando tu monitor pide una imagen nueva entre dos de ellos, el mod calcula dónde debería estar cada cosa en ese instante exacto, usando el propio código de movimiento del juego, lo dibuja ahí y lo deja todo como estaba antes del siguiente cálculo real. El juego nunca nota que haya cambiado nada.

### Versión técnica
`winmm.dll` es un proxy. Reenvía todas las llamadas a la `winmm.dll` real de Windows y, al arrancar, parchea `isaac-ng.exe` en memoria (no modifica ningún archivo):

1. **Bucle principal sin límite.** `IsaacMain` lee `1/60` en tres sitios para marcar su ritmo. Esas lecturas se redirigen a un valor del mod, así que el ritmo lo marca el VSync o `max_fps`.
2. **Lógica por reloj.** Se redirige la llamada al `IsaacUpdate` original. El mod lo llama exactamente 60 veces por segundo real (recuperando retrasos y tolerando pequeñas variaciones), sea cual sea la tasa de frames.
3. **Interpolación genérica de entidades.** Repentance+ ejecuta la lógica a 30 Hz y dibuja un frame extra de medio paso con el `Interpolate()` virtual de cada entidad. El juego tiene 9 implementaciones distintas (lágrimas, láseres, cuchillos, efectos y gore...). En vez de parchearlas, el mod guarda el estado de cada entidad tras cada tick de lógica (posición, posición de respaldo, flags, altura). En cada frame dibujado restaura ese estado, llama al `Interpolate()` del propio juego (que avanza medio tick) y escala el desplazamiento resultante a la fracción exacta que toca. Esto corrige también campos acumulativos, como la altura del gore.
4. **Jugador y cámara.** El jugador recibe un `Update()` real en el frame de medio paso, así que no se puede extrapolar sin efectos secundarios (el movimiento y la invencibilidad irían más rápido). En su lugar, la posición del jugador y el desplazamiento de render de la sala (la cámara) se interpolan entre sus dos últimos valores reales de 60 Hz.
5. **Restauración exacta.** Antes de cada update real del juego, el mod restaura exactamente lo que el juego original tendría en ese momento, así que la lógica solo ve el estado original.
6. **Comprobación de seguridad.** Antes de parchear, el mod verifica los bytes exactos que espera en cada sitio. Con cualquier otra versión del juego se desactiva y apunta el motivo en `morefps.log`.

## Hecho con Claude (IA)

Este mod lo he desarrollado junto a **Claude**, la IA de Anthropic, usando Claude Code. Yo dirigí el proyecto, tomé las decisiones y probé cada versión en el juego: detecté los fallos (Isaac moviéndose demasiado rápido, golpes dobles, temblores en el gore y en la cámara) y confirmé los arreglos. Claude hizo la ingeniería inversa del código del juego y escribió el código del mod.

Lo digo desde el principio porque creo que merece la pena ser transparente, y porque es un buen ejemplo de lo que se puede construir con ayuda de la IA.

## Ajustes (opcional)

`morefps.ini` está junto a `winmm.dll` en la carpeta del juego:

| Clave | Por defecto | |
|---|---|---|
| `enabled` | `1` | `0` desactiva el mod sin desinstalarlo |
| `max_fps` | `0` | Límite de FPS. `0` = sin límite (decide el VSync) |
| `log_fps` | `0` | `1` escribe los FPS y la velocidad de la lógica en `morefps.log` cada 2 s |

## Compatibilidad

- **The Binding of Isaac: Repentance+ v1.9.7.17**, la versión actual de Steam.
- **Probado:** Linux (Proton Experimental), monitor de 240 Hz, y limitado a 90 y 144 FPS.
- **Aún sin probar:** Windows (mismo archivo y mismo método de carga, así que debería funcionar; se agradecen los informes), Steam Deck, online, REPENTOGON.

¿Algún problema? [Abre un issue](https://github.com/julenster/isaac-at-more-fps/issues) y adjunta `morefps.log` (pon antes `log_fps=1`).

## Compilar desde el código

```
make        # necesita i686-w64-mingw32-gcc (Arch: mingw-w64-gcc · Debian/Ubuntu: gcc-mingw-w64-i686)
make dist   # genera dist/isaac-at-more-fps.zip
```

## Créditos

- [REPENTOGON](https://github.com/TeamREPENTOGON/REPENTOGON): su trabajo de ingeniería inversa fue una referencia muy útil para localizar funciones. No incluye código de REPENTOGON.
- Sin relación con Edmund McMillen, Nicalis ni Valve. No incluye ni modifica archivos del juego.

## Licencia

[MIT](LICENSE)
