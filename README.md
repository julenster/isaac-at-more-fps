# isaac-highfps

**High refresh rate for The Binding of Isaac: Repentance+** — play at 144, 165, 240 Hz or whatever your monitor does, with the game logic running **exactly as in vanilla**.

[Español](README.es.md)

The game normally updates its logic 30 times per second and draws 60 frames. This mod keeps the logic untouched (same speed, damage, i-frames, RNG, input timing) and draws as many frames as your monitor can show, placing every entity, Isaac and the camera where they would be at that exact moment.

## Install (1 minute)

1. **[Download `winmm.dll`](https://github.com/julenster/isaac-highfps/releases/latest)** from the latest release.
2. In Steam: right-click **The Binding of Isaac: Rebirth** → **Manage** → **Browse local files**.
3. Drop `winmm.dll` into that folder.
4. **Linux / Steam Deck only:** right-click the game → **Properties** → **General** → **Launch options**, paste:
   ```
   WINEDLLOVERRIDES="winmm=n,b" %command%
   ```

That's it. Launch the game normally. Make sure **VSync is on** in the game options so the frame rate follows your monitor.

**Uninstall:** delete `winmm.dll` (and clear the launch option on Linux).

## Settings (optional)

Put [`highfps.ini`](highfps.ini) next to `winmm.dll` if you want to change something:

| Key | Default | |
|---|---|---|
| `enabled` | `1` | `0` turns the mod off without uninstalling |
| `max_fps` | `0` | FPS cap. `0` = no cap (VSync decides) |
| `log_fps` | `0` | `1` writes FPS and logic rate to `highfps.log` |

## Compatibility

- **Repentance+ v1.9.7.17** (current Steam version). On any other version the mod detects the mismatch, **disables itself** and writes the reason to `highfps.log` — the game runs normally.
- Tested on Linux (Proton Experimental) with a 240 Hz monitor, plus capped at 90 and 144 FPS. Windows uses the same file and loading method but **hasn't been tested yet** — reports welcome.
- Works alongside Lua mods. Not tested with REPENTOGON.
- **Online play: untested.** The mod doesn't change game logic, so it shouldn't desync, but use it in local runs to be safe.

## How it works

`winmm.dll` is a proxy: it forwards everything to the real Windows `winmm.dll` and, when the game starts, patches it in memory (no game files are modified):

1. The main loop stops capping itself at 60 iterations per second.
2. The original game update runs on a clock, exactly 60 times per real second, regardless of the frame rate.
3. On every drawn frame, entities are placed at the right fraction of the current tick. The mod restores each entity's state from the last tick, calls the game's own `Interpolate()` (which advances half a tick) and scales the result. This works for all 9 entity interpolation variants (tears, lasers, knives, effects/gore...) without patching them.
4. Isaac and the camera are interpolated between their last two real positions, so they never overshoot.
5. Before each real game update, the exact state the game expects is restored.

## Building

```
make        # needs i686-w64-mingw32-gcc (Arch: mingw-w64-gcc, Debian/Ubuntu: gcc-mingw-w64-i686)
make dist   # zip with dll + ini + readme
```

Release binaries are built by GitHub Actions from this source; SHA-256 checksums are attached to each release.

## Credits

- [REPENTOGON](https://github.com/TeamREPENTOGON/REPENTOGON) — their reverse-engineering work was a great reference for locating functions. No REPENTOGON code is included.
- Not affiliated with Edmund McMillen or Nicalis. No game files are included or modified.

## License

[MIT](LICENSE)
