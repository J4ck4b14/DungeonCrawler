# Building DungeonCrawler for the web (itch.io)

## v1.1 — Roguelike update

**New: Relic system.** After clearing each floor you're offered a choice of
3 random relics (out of 10) you don't own yet. Take one or walk away.
Relics persist for the run and die with the hero. See `src/core/Relic.h`.

**Bug fixes:**
- AntiThrust stance can now actually parry Thrust (it was dead code:
  the pierce check ran before the parry check). Thrust's identity is now
  "unblockable by a WRONG stance" instead of "unblockable, period."
- XP curve fixed to the documented quadratic (5L^2+25L = 30/70/120/180...).
  It was accidentally linear (30/60/90), making late levels twice as fast
  as designed.
- Combat stats no longer overcount damage on parried/halved hits.
- CRITICAL: breaking a wall on the map edge moved the player off the grid
  (out-of-bounds memory access -- silent corruption on Windows, a hard
  crash on wasm). Edge walls are now unbreakable bedrock.

**Balance changes:**
- Speed rework: a 2nd action per round now requires DOUBLE the opponent's
  speed, and 2 is the cap (was: unlimited actions via integer division,
  so speed builds got 3-4 free turns vs slow enemies).
- Enemy tiers retire: once you're 5+ floors past a tier's unlock it stops
  spawning (no more free Rats on floor 10), and picks are weighted toward
  the newest tier (weight = minLevel^2).
- Enemy stats now scale EXPONENTIALLY (compounding 10%/floor) while XP
  rewards stay linear. Player power cannot keep pace forever: the dungeon
  forms a wall somewhere past floor 10. Escaping alive is the victory --
  every floor deeper is greed. (Baseline: a bot that only spams Slash and
  never defends or uses items dies around floor 13.)


## What changed vs. your original source

- `src/platform/WebConsole.h / .cpp` (NEW): redirects `std::cin` / `std::cout`
  through the browser terminal. Uses Emscripten **Asyncify** so all your
  blocking `std::cin` code runs completely unmodified.
- `src/utils/Console.h`: added `__EMSCRIPTEN__` branches:
  - `Clear()` emits ANSI escape codes (xterm.js understands them)
  - New `Console::Sleep(ms)` helper — uses `emscripten_sleep` on web so the
    browser tab never freezes
  - `WaitForEnter()` web variant (avoids a double-Enter issue with the
    stream buffer)
  - `HeartbeatQTE()` now has a REAL-TIME web version (same rules as your
    Windows `_kbhit` version — the d20 fallback is only used on Linux/macOS
    native builds now)
- `src/core/Game.cpp`: added missing `#include <sstream>` and `<algorithm>`
  (MSVC includes them transitively; clang/emscripten does not).
- Renamed `src/Utils/` -> `src/utils/` to match your `#include "utils/..."`
  lines (Windows is case-insensitive, everything else is not).
- Converted 3 files from Windows-1252 to UTF-8 encoding.

## Rebuilding on Windows

1. Install emsdk (you already have it in Downloads):
   ```
   cd emsdk-main
   emsdk install latest
   emsdk activate latest
   emsdk_env.bat
   ```

2. From the project root:
   ```
   cd src
   em++ -std=c++20 -O2 -I. ^
     main.cpp core/Game.cpp core/DevMode.cpp ^
     combat/CombatSystem.cpp ^
     dungeon/Dungeon.cpp dungeon/Perception.cpp ^
     entities/Entity.cpp entities/Enemy.cpp entities/EnemyFactory.cpp entities/Player.cpp ^
     utils/RNG.cpp platform/WebConsole.cpp ^
     -sASYNCIFY -sASYNCIFY_STACK_SIZE=65536 ^
     -sALLOW_MEMORY_GROWTH=1 ^
     -sEXPORTED_RUNTIME_METHODS=UTF8ToString,lengthBytesUTF8,stringToUTF8 ^
     -sEXPORTED_FUNCTIONS=_main,_malloc,_free ^
     -sENVIRONMENT=web ^
     -o ../web/game/game.js
   ```

3. Copy the new `game.js` + `game.wasm` next to `index.html`, re-zip, upload.

## Uploading to itch.io

1. Create a new project -> **Kind of project: HTML**.
2. Upload `DungeonCrawler-itch.zip` and check **"This file will be played
   in the browser"**.
3. Recommended embed settings:
   - Viewport: **800 x 600** minimum (or "Click to launch in fullscreen")
   - Check **Mobile friendly** (there's an on-screen input bar for phones)
   - No SharedArrayBuffer support needed (leave that unchecked)
4. Save & view. The `index.html` must stay at the root of the zip — it does.

## Local testing

Browsers block wasm from `file://`. Serve it:
```
cd dist
python -m http.server 8000
```
then open http://localhost:8000
