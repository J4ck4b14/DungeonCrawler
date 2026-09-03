# DungeonCrawler
 
A terminal roguelike written from scratch in C++20. Runs natively — and in your browser.
 
**▶ Play it now:** https://jackofaltrades.itch.io/dungeoncrawler *(one click, works on mobile too)*
 
<img width="397" height="154" alt="Screenshot 2026-07-05 173819" src="https://github.com/user-attachments/assets/77c28450-c48d-4010-a8af-5fb391ce3cc9" />

## The game
 
You descend a procedurally generated dungeon. There is no final boss and no bottom floor — enemies grow stronger faster than you do, and eventually the dungeon forms a wall your build cannot climb. **Victory is escaping alive.** Every extra floor you descend is greed.
 
- **Reactive combat** — commit to Defend, track falling WASD cues, and catch them at the guard line. Complete the sequence to block; perfect every cue to take no damage and counterattack.
- **Enemy groups** — deeper floors can surround you with two or three independently acting foes. The combat panel shows their stable target labels, intents, and initiative order.
- **Legacy progression** — every completed run awards persistent Legacy XP. New ranks unlock additional relics for future descents while each hero's combat level remains run-specific.
- **Tiered relics** — floor rewards draw from unlocked, depth-appropriate common, uncommon, and rare pools. Relics last for the current run; catalogue unlocks persist in the profile.
- **Perception** — survey rooms, spot hidden passages, break walls (not the bedrock, though — we learned that the hard way).
- **Bestiary** — inspect enemies to learn their stats and weaknesses across runs.
- **Death saves** — at death's door, a heartbeat QTE gives you one last chance. Match the rhythm or flatline.
- **Permadeath.** Obviously.
### Design notes
 
Enemy stats scale **exponentially** per floor (compounding 10%) while XP rewards scale linearly, so player power mathematically cannot keep pace forever. This architecture pass intentionally preserves those existing values; balancing is a separate phase.
 
## Tech
 
- **C++20, no gameplay framework.** Procedural dungeon generation, combat, persistent progression, perception, and terminal presentation remain explicit project-owned systems.
- **Runs in the browser via WebAssembly.** The game is built on blocking console I/O (`std::cin`), which browsers don't allow. The Emscripten layer in [`src/platform/`](src/platform/) redirects the standard streams and supports asynchronous terminal input.
## Building
 
### Native (Windows / Linux / macOS)
 
```sh
# Windows (run from a Visual Studio developer shell)
cmake --preset x64-debug
cmake --build --preset x64-debug
ctest --preset x64-debug

# Optional memory-safety check on Windows
cmake --preset x64-asan
cmake --build --preset x64-asan
ctest --preset x64-asan

# Linux
cmake --preset linux-debug
cmake --build --preset linux-debug
ctest --preset linux-debug
```
 
Requires CMake 3.21+ and a C++20 compiler.
 
### Web (itch.io)
 
The Emscripten compatibility layer is under [`src/platform/`](src/platform/). Web packaging is maintained separately from this native CMake project.
 
## Project structure
 
```
src/
├── audio/      Optional scene music playback
├── core/       Game loop, relics, stats, dev mode
├── combat/     Turn-based combat and testable defense timing rules
├── dungeon/    Procedural floors, rooms, perception
├── entities/   Player, enemies, definitions, factory
├── items/      Inventory
├── platform/   Profile storage, timed input, native/web terminal support
├── presentation/ Terminal menus, panels, and defense animation
├── progression/ Persistent Legacy profile and run rewards
└── utils/      Console helpers, RNG
tests/          Focused gameplay-rule regression tests
```

## Persistent profile

Native builds store the Legacy profile in the user's platform data directory
(`%LOCALAPPDATA%/DungeonCrawler/profile.dat` on Windows). Browser builds use
local storage. The versioned text format uses stable relic keys so catalogue
changes do not depend on enum ordering.
 
---
 
*My first complete C++ project. It kills you on purpose.*
