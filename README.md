# isaac-highfps

**Makes The Binding of Isaac: Repentance+ look as smooth as your monitor allows — 144, 165, 240 Hz… — without changing how the game plays.**

[Leer en español](README.es.md)

## What is this? (short version)

Isaac only draws 60 images per second. If you have a 144 Hz or 240 Hz monitor, the game still looks like 60.

This mod makes the game draw as many images per second as your screen can show. Everything moves more smoothly: Isaac, tears, enemies, the camera.

**The game itself doesn't change at all.** Same speed, same damage, same controls, same seeds. It only looks smoother.

## Install

**[⬇ Download isaac-highfps.zip](https://github.com/julenster/isaac-highfps/releases/latest/download/isaac-highfps.zip)** and extract it.

### Windows
Double-click **`install.bat`**. Done, play as usual.

> Windows may show a security warning because the file comes from the internet. Click *More info → Run anyway*. The code is public and the file is built automatically by GitHub from it (see [Is it safe?](#is-it-safe)).

### Linux / Steam Deck
1. Open a terminal in the extracted folder (Steam Deck: desktop mode, right-click the folder → *Open Terminal Here*) and run:
   ```
   sh install-linux.sh
   ```
2. In Steam: right-click the game → **Properties** → **General** → **Launch options**, paste:
   ```
   WINEDLLOVERRIDES="winmm=n,b" %command%
   ```

### Manual install (any system)
Copy `winmm.dll` into the game folder (Steam → right-click the game → **Manage** → **Browse local files**). On Linux / Steam Deck also add the launch option above.

**Tip:** turn **VSync on** in the game options so the frame rate matches your monitor.

### Uninstall
Windows: double-click `uninstall.bat`. Linux: `sh install-linux.sh uninstall` and clear the launch option. Or just delete `winmm.dll` from the game folder.

## FAQ

**Does it change gameplay?**
No. The game logic runs exactly as in the original, at exactly the same speed. The mod only changes what you see between the game's own updates.

**Does it work with my mods? Achievements?**
It's not a Lua mod: the game doesn't see it in the mods list, and it works alongside your usual mods. Achievements aren't touched.

**Online co-op?**
Not tested yet. It shouldn't cause desyncs because it doesn't change the game logic, but play local runs to be safe.

**The game updated and now it doesn't work.**
The mod checks the game version at startup. If it doesn't match, it **turns itself off** and the game runs normally. Wait for an update of the mod.

**What if my monitor is 60 Hz?**
Then there's nothing to gain, but nothing breaks either.

<a name="is-it-safe"></a>**Is it safe?**
The full source code is in this repository. The download is built by GitHub Actions directly from that code, and every release lists its SHA-256 checksums. The mod doesn't modify any game files and doesn't connect to the internet.

## How it works

### Simple version
Every second, the game calculates where everything is 60 times; that's "the game logic". Between those moments it doesn't know where anything is, so it can't draw more frames.

The mod leaves those 60 calculations alone. When your monitor wants a new frame between two of them, the mod works out where each thing should be at that exact moment, using the game's own movement code, draws it there, and puts everything back before the game's next real calculation. The game never notices anything changed.

### Technical version
`winmm.dll` is a proxy. It forwards every call to the real Windows `winmm.dll` and, at startup, patches `isaac-ng.exe` in memory (no files are modified):

1. **Main loop uncapped.** `IsaacMain` reads `1/60` in three places to pace itself. Those reads are redirected to the mod's own value, so the loop is paced by VSync or `max_fps`.
2. **Logic on a clock.** The call to the original `IsaacUpdate` is redirected. The mod calls it exactly 60 times per real second (with catch-up and jitter tolerance), regardless of the frame rate.
3. **Generic entity interpolation.** Repentance+ runs logic at 30 Hz and renders an extra half-step frame using each entity's virtual `Interpolate()`. The game has 9 different implementations (tears, lasers, knives, effects/gore...). Instead of patching them, the mod snapshots each entity after every logic tick (position, backup position, flags, height). On every drawn frame it restores that snapshot, calls the game's own `Interpolate()` (which advances half a tick), and scales the resulting displacement to the exact fraction needed. This also fixes accumulative fields such as gore height.
4. **Player and camera.** The player gets a real `Update()` on the half-step frame, so it can't be extrapolated without side effects (movement and invincibility frames would speed up). Instead, the player position and the room's render scroll offset (the camera) are interpolated between their last two real 60 Hz values.
5. **Exact state restore.** Before every real game update, the mod restores exactly what the original game would have at that point, so the logic only ever sees vanilla state.
6. **Safety check.** Before patching, the mod verifies the exact bytes it expects at each patch site. On any other game version it disables itself and logs why to `highfps.log`.

## Made with Claude (AI)

This mod was developed together with **Claude**, Anthropic's AI, using Claude Code. I directed the project, made the decisions and tested every version in game: spotting the bugs (Isaac moving too fast, double hits, gore and camera jitter) and confirming the fixes. Claude did the reverse engineering of the game's code and wrote the mod's code.

I'm saying it upfront because I think it's worth being transparent about, and because it's a good example of what AI can help build.

## Settings (optional)

`highfps.ini` sits next to `winmm.dll` in the game folder:

| Key | Default | |
|---|---|---|
| `enabled` | `1` | `0` turns the mod off without uninstalling |
| `max_fps` | `0` | FPS cap. `0` = no cap (VSync decides) |
| `log_fps` | `0` | `1` writes FPS and logic rate to `highfps.log` every 2 s |

## Compatibility

- **The Binding of Isaac: Repentance+ v1.9.7.17**, the current Steam version.
- **Tested:** Linux (Proton Experimental), 240 Hz monitor, and capped at 90 and 144 FPS.
- **Not tested yet:** Windows (same file and loading method, so it should work; reports welcome), Steam Deck, online co-op, REPENTOGON.

Found a problem? [Open an issue](https://github.com/julenster/isaac-highfps/issues) and attach `highfps.log` (set `log_fps=1` first).

## Building from source

```
make        # needs i686-w64-mingw32-gcc (Arch: mingw-w64-gcc · Debian/Ubuntu: gcc-mingw-w64-i686)
make dist   # builds dist/isaac-highfps.zip
```

## Credits

- [REPENTOGON](https://github.com/TeamREPENTOGON/REPENTOGON): their reverse-engineering work was a very useful reference for locating functions. No REPENTOGON code is included.
- Not affiliated with Edmund McMillen, Nicalis or Valve. No game files are included or modified.

## License

[MIT](LICENSE)
