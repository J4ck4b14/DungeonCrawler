# DungeonCrawler
 
A terminal roguelike written from scratch in C++20. Runs natively — and in your browser.
 
**▶ Play it now:** https://jackofaltrades.itch.io/dungeoncrawler *(one click, works on mobile too)*
 
<img width="397" height="154" alt="Screenshot 2026-07-05 173819" src="https://github.com/user-attachments/assets/77c28450-c48d-4010-a8af-5fb391ce3cc9" />

## The game
 
You descend a procedurally generated dungeon. There is no final boss and no bottom floor — enemies grow stronger faster than you do, and eventually the dungeon forms a wall your build cannot climb. **Victory is escaping alive.** Every extra floor you descend is greed.
 
- **Directional combat** — three attack styles (Slash / Thrust / Bash), four defensive stances, and real parries. Read your enemy, guess right, and their attack becomes your counter.
- **Relics** — after every floor you clear, choose one of three ancient relics. Most carry a price. All are permanent. Runs build differently every time.
- **Perception** — survey rooms, spot hidden passages, break walls (not the bedrock, though — we learned that the hard way).
- **Bestiary** — inspect enemies to learn their stats and weaknesses across runs.
- **Death saves** — at death's door, a heartbeat QTE gives you one last chance. Match the rhythm or flatline.
- **Permadeath.** Obviously.
### Design notes
 
Enemy stats scale **exponentially** per floor (compounding 10%) while XP rewards scale linearly, so player power mathematically cannot keep pace forever. The balance was tuned with automated bot playtesting: a bot that spams a single attack and never defends dies around floor 13. A player who actually parries, uses items, and picks relics wisely gets further. That gap is the game.
 
## Tech
 
- **C++20, no frameworks.** ~4,500 lines: procedural dungeon generation, combat system, entity hierarchy, perception system.
- **Runs in the browser via WebAssembly.** The game is built on blocking console I/O (`std::cin`), which browsers don't allow. The port (Emscripten + Asyncify) swaps the standard stream buffers for custom `streambuf`s that suspend the C++ call stack while an [xterm.js](https://xtermjs.org/) terminal waits for input — so all game logic runs unmodified. See [`src/platform/`](src/platform/) and [`BUILD-WEB.md`](BUILD-WEB.md).
- **Bot playtesting.** Balance was validated with Node.js drivers that parse the game's menus and play autonomously (30,000+ inputs, zero crashes). One of them found a memory corruption bug that had been silently lurking in the native build.
## Building
 
### Native (Windows / Linux / macOS)
 
```
cmake --preset default
cmake --build build
```
 
Requires CMake 3.15+ and a C++20 compiler.
 
### Web (itch.io)
 
See [`BUILD-WEB.md`](BUILD-WEB.md) for the full Emscripten build command, the list of web-specific patches, and the packaging steps for itch.io.
 
## Project structure
 
```
src/
├── core/       Game loop, relics, stats, dev mode
├── combat/     Turn-based combat, parries, death saves
├── dungeon/    Procedural floors, rooms, perception
├── entities/   Player, enemies, enemy factory
├── items/      Inventory
├── platform/   Web (Emscripten) console layer
└── utils/      Console helpers, RNG
```
 
---
 
*My first complete C++ project. It kills you on purpose.*
