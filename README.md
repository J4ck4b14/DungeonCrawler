# DungeonCrawler
 
A terminal roguelike written from scratch in C++20. Runs natively — and in your browser.
 
**▶ Play it now:** https://jackofaltrades.itch.io/dungeoncrawler *(one click, works on mobile too)*
 
<img width="397" height="154" alt="Screenshot 2026-07-05 173819" src="https://github.com/user-attachments/assets/77c28450-c48d-4010-a8af-5fb391ce3cc9" />

## The game

Characters are built from four core stats: **Health, Strength, Speed, and Intelligence**.
 
You descend a procedurally generated dungeon. There is no final boss and no bottom floor — enemies grow stronger faster than you do, and eventually the dungeon forms a wall your build cannot climb. **Victory is escaping alive.** Every extra floor you descend is greed.
 
- **Reactive combat** — commit to Defend, read species-specific four-lane A/W/S/D patterns, and catch cues at the guard line. Complete the sequence to block; perfect every cue to take no damage and counterattack.
- **Enemy groups** — deeper floors can surround you with two or three independently acting foes. The combat panel shows their stable target labels, intents, and initiative order.
- **Equipment-driven builds** — weapons have visible Weapon Rank (WR), distinct STR/SPD/INT scaling, permanent sharpening, and up to two visible ranked enchantments. Apparel occupies five fixed slots; replacing gear is a real choice, not backpack accumulation. Loot rarity exists only behind the scenes so the game presents what an item does, not a color label.
- **Rest-site choices** — each site is spent on one recovery, permanent weapon/apparel improvement, or uncapped STR/SPD/INT training decision.
- **Statuses and spell identity** — Burn, Poison, Bleed, Stun, Slow, and Regeneration use shared lifecycle rules. Enemy spellbooks and casting behavior remain species-specific.
- **Legacy 0–50** — every completed run awards persistent Legacy XP. Six relics are available at Rank 0, then one catalogue relic unlocks at every five-rank milestone through Rank 50. Heroes still begin each descent with run-specific combat progression.
- **Tiered relic offers** — floor rewards draw from the profile's unlocked, depth-appropriate pool. Relics last for the current run; catalogue unlocks persist.
- **Systemic dungeon topology** — floors begin as sparse connected graphs with branches, dead ends, chokepoints, junctions, and selective loops. Finding the stairs early is a valid reason to leave; ordinary movement and map coverage award no XP.
- **Perception and breaching** — deliberate surveys can mark suspicious masonry without revealing hidden geometry. High INT gains clues; high STR can force a genuinely generated route, but striking solid masonry can never create a room.
- **Persistent Bestiary** — encounters, victories, observed spells/statuses, weaknesses, behavior notes, and the best knowledge tier survive between runs. Knowledge helps interpret tells without making every future action certain.
- **Death saves** — at death's door, a heartbeat QTE gives you one last chance. Match the rhythm or flatline.
- **Permadeath.** Obviously.
### Design notes
 
Each enemy specimen has a **Rank from 1–50** driven by dungeon depth, species threat, and small specimen variance—not player power. Per-species growth keeps Giants slow and crushing, Werewolves fast and burst-heavy, and magical enemies focused on Intelligence and mana. XP grows sublinearly with Rank to reward danger without creating runaway leveling.

Equipment names show their useful structure directly, for example `Sword IV — Fire-form II — Snake-Tongue III`, followed by the resulting damage. Internal Overall Rank and Common/Rare/Epic/Legendary bands are generation tools and are intentionally absent from the normal player UI.
 
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

`DungeonCrawlerValidation` is also built when testing is enabled. It emits seeded loot distributions, aggregate topology metrics, and several debug-only truth maps. Internal categories and probabilities printed there are validation data, not player-facing information.
 
### Web (itch.io)
 
The Emscripten compatibility layer is under [`src/platform/`](src/platform/). Web packaging is maintained separately from this native CMake project.
 
## Project structure
 
```
src/
├── audio/      Optional scene music playback
├── core/       Game loop, relics, stats, dev mode
├── combat/     Turn-based combat and testable defense timing rules
├── dungeon/    Procedural floors, rooms, perception, rest-site rules
├── equipment/  Weapon, enchantment, apparel, and equipment-slot models
├── entities/   Player, enemies, definitions, factory
├── items/      Consumables and their inventory
├── loot/       Seedable equipment generation and Legacy-weighted rank rolls
├── platform/   Profile storage, timed input, native/web terminal support
├── presentation/ Terminal menus, panels, and defense animation
├── progression/ Persistent Legacy profile and run rewards
├── status/     Shared status-effect state and lifecycle rules
└── utils/      Console helpers, RNG
tests/          Focused gameplay-rule regression tests
```

## Persistent profile

Native builds store the Legacy profile in the user's platform data directory
(`%LOCALAPPDATA%/DungeonCrawler/profile.dat` on Windows). Browser builds use
local storage. The versioned text format uses stable relic keys so catalogue
changes do not depend on enum ordering. Version 2 adds persistent Bestiary
records and the Legacy 0–50 curve; supported version-1 profiles are migrated
while preserving their old rank and fractional progress where possible.

## Developer tuning points

The deliberately asymmetric systems are centralized rather than flattened:
weapon scaling and enchantment behavior live under `equipment/`, loot pull and
category weights under `loot/`, armor mitigation in equipment rules, species
Rank growth and XP in enemy definitions/factory, spell behavior in spell rules,
status potency in status rules, reactive-defense windows and SPD assistance in
defense rules, topology density/loops/walls in dungeon topology, and Legacy XP
in the player profile. These are the intended balance levers for future passes.
 
---
 
*My first complete C++ project. It kills you on purpose.*
