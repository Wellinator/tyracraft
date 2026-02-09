# TyraCraft — Agent & LLM Context Guide

> **Purpose**: This document is the primary entry point for AI agents, LLMs, and
> automated coding assistants working on the TyraCraft project. It provides the
> essential context needed to understand, navigate, and safely modify the codebase.

---

## Table of Contents

- [Project Overview](#project-overview)
- [Technology Stack](#technology-stack)
- [Repository Layout](#repository-layout)
- [Architecture at a Glance](#architecture-at-a-glance)
- [Critical PS2 Constraints](#critical-ps2-constraints)
- [Key Conventions](#key-conventions)
- [Navigation Map — Detailed Docs](#navigation-map--detailed-docs)
- [PS2 Hardware Skills](#ps2-hardware-skills)
- [Quick Reference Tables](#quick-reference-tables)
- [Agent Guidelines](#agent-guidelines)

---

## Project Overview

| Field | Value |
|---|---|
| **Name** | TyraCraft |
| **Version** | `v0.86.140-pre-alpha` |
| **License** | Apache 2.0 |
| **Language** | C++ (cross-compiled for MIPS R5900 EE core) |
| **Platform** | Sony PlayStation 2™ |
| **Genre** | Voxel sandbox (Minecraft-like) |
| **Engine** | [Tyra](https://github.com/h4570/tyra) by h4570 |
| **SDK** | [PS2SDK](https://github.com/ps2dev/ps2sdk) (ps2dev toolchain) |
| **Author** | [@Wellinator](https://github.com/wellinator) |
| **Website** | https://wellinator.github.io/tyracraft |

TyraCraft is an open-source voxel game that brings a Minecraft-like experience
to the PlayStation 2. The game features creative, survival, and maze game modes
with procedural world generation, block placement/breaking, mob AI, day/night
cycles, liquid physics, and a chunk-based rendering system — all running within
the extreme memory and processing constraints of the PS2 hardware.

---

## Technology Stack

```
┌──────────────────────────────────────────────┐
│              TyraCraft Game                   │
│   (src/, inc/ — this repository)             │
├──────────────────────────────────────────────┤
│          Tyra Engine (v≥2.x)                 │
│   /tyra/engine — rendering, audio, input,    │
│   math, file I/O for PS2                     │
├──────────────────────────────────────────────┤
│             PS2SDK / PS2DEV                   │
│   EE/GS/VU/IOP/DMA low-level access         │
├──────────────────────────────────────────────┤
│        PlayStation 2 Hardware                 │
│   EE Core (MIPS R5900), 32 MB RAM,          │
│   GS (Graphics Synthesizer), 4 MB VRAM      │
└──────────────────────────────────────────────┘
```

### Third-Party Libraries (bundled in `inc/3libs/`)

| Library | Purpose |
|---|---|
| **BVH** (`bvh/`) | Dynamic AABB tree for collision detection |
| **CrossCraft Core** (`CrossCraftCore/`) | World generation algorithms |
| **FastNoiseLite** (`FastNoiseLite/`) | Procedural noise (terrain, caves) |
| **inifile-cpp** (`inifile-cpp/`) | INI config file parsing |
| **mazegen** (`mazegen/`) | Maze generation |
| **nlohmann/json** (`nlohmann/`) | JSON parsing (saves, lang files) |
| **observer** (`observer/`) | Observer pattern utilities |

### External Dependencies (system / Docker image)

- PS2SDK headers & libs (`tamtypes.h`, `audsrv.h`, `dma.h`, `gs_gp.h`, etc.)
- zlib (save compression)
- Tyra Engine (`libtyra.a`)

---

## Repository Layout

```
tyracraft/
├── agents.md               ← YOU ARE HERE — LLM entry point
├── README.md               ← Project overview for humans
├── CHANGELOG.md            ← Detailed version history
├── ROADMAP.TXT             ← Feature roadmap (✅/❌/🔲)
├── Makefile                ← PS2 build (target: tyracraft.elf)
├── Dockerfile              ← Build environment image
├── docker-compose.yml      ← Container orchestration
├── run.ps1                 ← Launch PCSX2 emulator (Windows)
│
├── inc/                    ← C++ HEADER FILES (.hpp)
│   ├── 3libs/              ← Third-party library headers
│   ├── entities/           ← Game entities (Block, Chunk, Player, Mob, World…)
│   │   ├── blocks/         ← Concrete block type definitions
│   │   ├── items/          ← Item definitions
│   │   ├── mob/            ← Mob types + AI state machine + A* pathfinding
│   │   ├── player/         ← Player entity, rendering pipelines
│   │   └── animation/      ← Animation framework
│   ├── managers/           ← Subsystem managers (~20 managers)
│   │   ├── block/          ← Block template repository, UV/vertex data, SFX
│   │   ├── mesh/           ← Mesh builders (cuboid, water, lava, slab, torch…)
│   │   ├── font/           ← Font atlas rendering
│   │   └── clipper/        ← Triangle/frustum clipping
│   ├── services/           ← Game services layer
│   ├── states/             ← Game state machine (two levels)
│   │   ├── splash_screen/
│   │   ├── language_selection/
│   │   ├── main_menu/      ← Menu screens (new game, load, options…)
│   │   ├── loading/        ← World/save loading states
│   │   └── game_play/      ← Main gameplay + sub-states (creative/survival/maze)
│   └── memory-monitor/     ← PS2 memory tracking
│
├── src/                    ← C++ SOURCE FILES (.cpp) — mirrors inc/ structure
│   ├── main.cpp            ← Entry point
│   ├── tyracraft_game.cpp  ← Main game loop
│   ├── entities/
│   ├── managers/
│   ├── services/
│   ├── states/
│   └── post-fx/            ← GS post-processing (fog)
│
├── bin/                    ← BUILD OUTPUT + RUNTIME ASSETS
│   ├── tyracraft.elf       ← Compiled PS2 executable
│   ├── config.ini          ← User settings
│   ├── textures/           ← PNG texture atlases + packs
│   ├── models/             ← OBJ/MD2 3D models
│   ├── sounds/             ← ADPCM audio files
│   ├── lang/               ← JSON translation dictionaries (en_US, pt_BR…)
│   └── saves/              ← .tcw/.mgw save files (zlib compressed)
│
├── res/                    ← Source resource files (copied to bin/ at build)
├── docs/                   ← Technical documentation
│   ├── agents/             ← Detailed docs for AI agents (see below)
│   ├── EE_CORE_MANUAL.md
│   ├── EE_USERS_MANUAL.md
│   ├── GS_USERS_MANUAL.md
│   ├── POSTFX_FOG_INSTRUCTIONS.md
│   └── ps2dev-intellisense/
│
├── obj/                    ← Compiled object files (auto-generated)
├── logs/                   ← Runtime logs
└── .vscode/                ← VS Code workspace config (tasks, settings, C++ props)
```

> **Convention**: Headers in `inc/` mirror sources in `src/` 1:1.
> e.g. `inc/managers/chunk_manager.hpp` ↔ `src/managers/chunk_manager.cpp`

---

## Architecture at a Glance

### Game Loop

```
main.cpp
  └─ Engine engine(options)         // Tyra engine init (512×448, near=0.1, far=5000)
  └─ TyraCraftGame game(&engine)    // Game object creation
  └─ engine.run(game)               // Enters main loop → calls game.update/render
       │
       ├─ timer.update()
       ├─ stateManager.update(dt)       // Logic at variable rate
       ├─ notificationManager.update()
       ├─ stateManager.fixedUpdate(dt)  // Physics at 30 FPS
       └─ beginFrame()
            ├─ stateManager.render()
            ├─ notificationManager.render()
            ├─ FPS/memory overlay
            └─ endFrame()
```

### Two-Level State Machine

```
Level 1 — Global Game States (GameState)
  ┌──────────────────┐     ┌────────────────────┐     ┌────────────────┐
  │ SplashScreen     │ ──▶ │ LanguageSelection  │ ──▶ │ MainMenu       │
  └──────────────────┘     └────────────────────┘     └────────────────┘
                                                            │
                               ┌────────────────────────────┤
                               ▼                            ▼
                        ┌──────────────┐          ┌────────────────┐
                        │ LoadingGame  │          │ LoadingSaved   │
                        └──────┬───────┘          └───────┬────────┘
                               └──────────┬──────────────┘
                                          ▼
                                 ┌─────────────────┐
                                 │  StateGamePlay  │
                                 └────────┬────────┘
                                          │
Level 2 — Playing Sub-States (PlayingStateBase)
                        ┌─────────────────┼──────────────────┐
                        ▼                 ▼                  ▼
                 ┌─────────────┐  ┌──────────────┐  ┌──────────────┐
                 │  Creative   │  │  Survival    │  │  MazeCraft   │
                 └─────────────┘  └──────────────┘  └──────────────┘
                        │                 │                  │
                        └────────┬────────┘                  │
                                 ▼                           │
                        ┌──────────────┐                     │
                        │ GameMenu     │◀────────────────────┘
                        └──────────────┘
```

### Core Patterns

| Pattern | Usage |
|---|---|
| **State** | `GameState` (global) + `PlayingStateBase` (in-game) — classic GoF State pattern via `Context` |
| **Singleton** | Template in `singleton.hpp` — used by `Timer`, `ChunkManager`, `BlockManager`, `SoundManager`, `VisibleFacesManager`, `TaskManager`, `LanguageManager`, `Level`, `FastVoxelTraversalService` |
| **Entity** | Base `Entity` → `Block`, `Player`, `Mob` — virtual `tick()`, collision, physics |
| **Manager** | One manager per subsystem (chunk, block, sound, light, collision, clouds, particles, etc.) |
| **Greedy Meshing** | Chunks merge adjacent coplanar faces to reduce draw calls |
| **BFS Flood Fill** | Light propagation (sunlight + block light) and liquid propagation |
| **A\* Pathfinding** | Mob AI navigation |

---

## Critical PS2 Constraints

> ⚠️ **The PlayStation 2 has only 32 MB of RAM and 4 MB of VRAM.**
> Every byte counts. This is the single most important constraint.

| Resource | Limit | Project Threshold |
|---|---|---|
| Main RAM | 32 MB | `MAX_SAFE_MEMORY_ALLOCATION` = 29 MB |
| VRAM | 4 MB | Managed by Tyra's `textureRepository` |
| CPU | MIPS R5900 @ 294.912 MHz | No SSE/AVX — use `fastmath.h` |
| Audio channels | 24 (SPU2) | `MAX_ADPCM_CH` = 23 |
| Resolution | 512 × 448 fixed | `SCREEN_WIDTH` / `SCREEN_HEIGHT` |
| World size | 128 × 128 × 64 blocks | `OVERWORLD_SIZE` = 1,048,576 |
| Chunk size | 8 × 8 × 8 blocks | `CHUNK_SIZE` = 8 |

### What this means for code changes

1. **No STL containers in hot paths** — `std::map`, `std::unordered_map` allocate heap memory; prefer fixed arrays or pool allocators.
2. **No smart pointers** — manual `new`/`delete` throughout; `memalign()` for DMA-aligned buffers.
3. **Bitfields and packed structs** — `Block` uses compressed coordinates (`s16`), bitfield face masks, and cache-aligned layout.
4. **Greedy meshing is critical** — merging faces drastically reduces polygon count.
5. **LOD and draw distance** — chunks beyond `MAX_DRAW_DISTANCE` (10 chunks) are not rendered.
6. **Texture atlas** — blocks share a single large atlas to avoid texture swaps.

> 📖 Full details: [docs/agents/ps2-constraints.md](docs/agents/ps2-constraints.md)

---

## Key Conventions

### Code Style

- **Namespace**: `TyraCraft` (all game code)
- **File extensions**: `.hpp` (headers), `.cpp` (sources)
- **Header/source mirror**: `inc/<path>/<name>.hpp` ↔ `src/<path>/<name>.cpp`
- **Include guards**: `#pragma once` or traditional `#ifndef`
- **Naming**: PascalCase for classes/types, camelCase for methods/variables, UPPER_SNAKE for constants
- **Enums**: `enum class` with explicit `u8`/`u16` underlying types where applicable

### Memory Rules

- **Never** allocate more than necessary — track with `memory_monitor`
- **Always** `delete`/`free` manually — no RAII wrappers in this codebase
- **DMA buffers** must be 64-byte aligned — use `memalign(64, size)`
- **Texture uploads** go through Tyra's `textureRepository`

### Adding New Features

| Task | Pattern to Follow |
|---|---|
| New block type | Add class in `inc/entities/blocks/`, extend `Block`, use `IMPLEMENT_BLOCK_CLONE` macro, register in `BlockManager` |
| New game state | Extend `GameState` or `PlayingStateBase`, wire in `Context::setState()` |
| New manager | Create `.hpp`/`.cpp` pair, optionally make Singleton via `Singleton<T>` template |
| New mob | Extend `Mob` in `inc/entities/mob/`, add AI states, register in `MobManager` |
| New sound | Convert WAV → ADPCM (docker task), place in `bin/sounds/`, register in `SfxLibrary` |
| New UI screen | Extend `ScreenBase` in `inc/states/main_menu/screens/` |

> 📖 Full details: [docs/agents/conventions.md](docs/agents/conventions.md)

---

## Navigation Map — Detailed Docs

The `docs/agents/` directory contains detailed reference documentation for AI
agents. Each file dives deep into a specific aspect of the project:

| Document | Description |
|---|---|
| [docs/agents/architecture.md](docs/agents/architecture.md) | Full architecture: game loop, state machines, entity hierarchy, manager system, Singleton pattern |
| [docs/agents/ps2-constraints.md](docs/agents/ps2-constraints.md) | PS2 hardware limits, DMA/GIF pipeline, memory management, optimization techniques |
| [docs/agents/codebase-map.md](docs/agents/codebase-map.md) | File-by-file map: all managers, entities, states, blocks, libraries with purpose annotations |
| [docs/agents/build-and-workflow.md](docs/agents/build-and-workflow.md) | Docker build pipeline, VS Code tasks, asset pipeline, testing on PCSX2/real PS2 |
| [docs/agents/conventions.md](docs/agents/conventions.md) | Code style, naming, patterns for adding blocks/states/managers/mobs, memory rules |
| [docs/agents/dma-gif-reference.md](docs/agents/dma-gif-reference.md) | Complete DMA/GIF reference: DMAtag/GIFtag bit layout, PACKED/REGLIST/IMAGE modes, full GS register catalog |

### PS2 Hardware Skills

Distilled, agent-optimized references extracted from the raw PS2 hardware manuals.
**Consult these instead of reading the raw manuals directly** — they contain the
essential information organized for code-generation tasks.

| Skill Document | Source Manual | When to Use |
|---|---|---|
| [docs/agents/skill-ee-cpu.md](docs/agents/skill-ee-cpu.md) | EE_CORE_MANUAL.md | CPU pipeline, FPU quirks (non-IEEE!), cache, SIMD, branch prediction, VPU0 macro mode |
| [docs/agents/skill-ee-system.md](docs/agents/skill-ee-system.md) | EE_USERS_MANUAL.md | DMA channels/tags, GIF packet format, VIF/VPU data path, timers, interrupts |
| [docs/agents/skill-gs-rendering.md](docs/agents/skill-gs-rendering.md) | GS_USERS_MANUAL.md | VRAM layout, primitives, textures, pixel tests, alpha blending formula, fog, drawing contexts |
| [docs/agents/skill-postfx.md](docs/agents/skill-postfx.md) | POSTFX_FOG_INSTRUCTIONS.md | Z-buffer fog trick, CLUT fog curves, depth of field, 32-pixel strip optimization |

### Other Project Documentation

| Document | Description |
|---|---|
| [README.md](README.md) | Human-readable project overview |
| [CHANGELOG.md](CHANGELOG.md) | Detailed version history |
| [ROADMAP.TXT](ROADMAP.TXT) | Feature roadmap with completion status |
| [docs/agents/dma-gif-reference.md](docs/agents/dma-gif-reference.md) | Complete DMA/GIF reference: DMAtag/GIFtag bit layout, PACKED/REGLIST/IMAGE modes, full GS register catalog |
| [docs/EE_CORE_MANUAL.md](docs/EE_CORE_MANUAL.md) | PS2 Emotion Engine core reference (raw — prefer skill-ee-cpu.md) |
| [docs/EE_USERS_MANUAL.md](docs/EE_USERS_MANUAL.md) | PS2 EE system reference (raw — prefer skill-ee-system.md) |
| [docs/GS_USERS_MANUAL.md](docs/GS_USERS_MANUAL.md) | PS2 Graphics Synthesizer reference (raw — prefer skill-gs-rendering.md) |
| [docs/POSTFX_FOG_INSTRUCTIONS.md](docs/POSTFX_FOG_INSTRUCTIONS.md) | Fog post-FX implementation notes (raw — prefer skill-postfx.md) |

---

## PS2 Hardware Skills

The `docs/agents/skill-*.md` files are **distilled, agent-optimized** references
extracted from the full PS2 hardware manuals (`docs/*_MANUAL.md`). They contain
only the information relevant to writing and reviewing code for this project.

> **Rule for agents**: Always consult the appropriate skill file **before** reading
> the raw manual. The raw manuals are 10,000–16,000 lines each; the skills are
> 200–400 lines of actionable knowledge.

### Skill: EE Core CPU (`skill-ee-cpu.md`)

Covers the **MIPS R5900 processor** — the main CPU executing TyraCraft code.

| Topic | Key Facts |
|---|---|
| Pipeline | 6-stage, 2-way superscalar (dual-issue per cycle) |
| FPU | ⚠️ **Non-IEEE 754** — denorms flushed to zero, truncation rounding only, `NaN → 0x7FFFFFFF` |
| Cache | 16 KB I-Cache, 8 KB D-Cache (64-byte lines), 16 KB Scratchpad |
| SIMD | 128-bit multimedia instructions (4×32, 8×16, 16×8 parallel ops) |
| Latencies | Integer ALU=1cy, FPU=4cy, FPU DIV=7cy, RSQRT=14cy, Load-miss=40+cy |
| Branching | BHT + 64-entry BTAC; 3-cycle misprediction; use MOVN/MOVZ for branchless |

📖 [docs/agents/skill-ee-cpu.md](docs/agents/skill-ee-cpu.md)

### Skill: EE System (`skill-ee-system.md`)

Covers **DMA, GIF, VPU, timers, and interrupts** — the data-moving infrastructure.

| Topic | Key Facts |
|---|---|
| DMA | 10 channels, 128-bit transfers, source/destination chain modes |
| DMAtag | 128-bit control word: ID (refe/cnt/next/ref/call/ret/end) + ADDR + QWC |
| GIF | 3 paths (PATH1=VU1, PATH2=VIF1, PATH3=DMA ch2); GIFtag = NLOOP + FLG + REGS |
| Packet formats | PACKED (1 reg/qword), REGLIST (1 reg/dword), IMAGE (raw pixels) |
| VIF | Decompresses vertex data (UNPACK V4-32/16/8, V3-32, V2-*, S-*) into VU memory |
| Timers | 4 × 16-bit, clocked at BUSCLK (147 MHz) / 16 / 256, or H-BLNK |

📖 [docs/agents/skill-ee-system.md](docs/agents/skill-ee-system.md)

### Skill: GS Rendering (`skill-gs-rendering.md`)

Covers the **Graphics Synthesizer** — the PS2's pixel-rendering GPU.

| Topic | Key Facts |
|---|---|
| VRAM | 4 MB total; frame (×2) + Z + textures must all fit |
| Fill rate | 2.4 Gpix/s (no texture), 1.2 Gpix/s (textured) |
| Primitives | Point, Line, LineStrip, Triangle, TriStrip, TriFan, Sprite |
| Textures | Max 1024×1024, power-of-2, formats: RGBA32/16, indexed 8/4-bit |
| Alpha blend | `Output = (A-B)×C + D` where `×Y = (X×Y)>>7`; 0x80 = 1.0 |
| Pixel test | Scissor → Alpha Test → Dest Alpha Test → Depth Test |
| ⚠️ TEXFLUSH | **Must write** after texture/CLUT upload; stale cache otherwise |
| Contexts | 2 independent drawing contexts (switch per primitive) |

📖 [docs/agents/skill-gs-rendering.md](docs/agents/skill-gs-rendering.md)

### Skill: Post-FX (`skill-postfx.md`)

Covers the **Z-buffer fog** and **depth of field** post-processing techniques.

| Topic | Key Facts |
|---|---|
| Z-buffer fog | Reinterpret Z as 16-bit texture → copy Green→Alpha → blend fog color |
| Strip width | 8px for Z→Alpha copy, **32px for fog draw** (page cache thrashing avoidance) |
| Scissor trick | Double height when reinterpreting 32-bit as 16-bit format |
| CLUT curves | 256-entry CLUT maps depth→fog density for non-linear fog (exp, exp²) |
| Depth of field | Downsample frame → upsample with dest alpha blend → variable blur |
| Performance | ~1 ms total (Z copy + fog draw) — essentially free at 33 ms/frame |

📖 [docs/agents/skill-postfx.md](docs/agents/skill-postfx.md)

---

## Quick Reference Tables

### World Constants (`inc/constants.hpp`)

| Constant | Value | Notes |
|---|---|---|
| `OVERWORLD_H_DISTANCE` | 128 | Horizontal world size in blocks |
| `OVERWORLD_V_DISTANCE` | 64 | Vertical world size in blocks |
| `OVERWORLD_SIZE` | 1,048,576 | Total blocks (128×128×64) |
| `CHUNK_SIZE` | 8 | Blocks per chunk axis |
| `BLOCK_SIZE` | 8.0f | Block scale unit |
| `MAX_DRAW_DISTANCE` | 10 | In chunks |
| `HOT_INVENTORY_SIZE` | 9 | Player hotbar slots |
| `MAX_ADPCM_CH` | 23 | Audio channels |
| `MAX_SAFE_MEMORY_ALLOCATION` | 29 MB | Memory safety threshold |

### Game Modes

| Mode | State Class | Description |
|---|---|---|
| Creative | `CreativePlayingState` | Free building, flying, no damage |
| Survival | `SurvivalPlayingState` | Resource gathering, health, mobs |
| MazeCraft | `MazePlayingState` | Navigate generated mazes |

### Block Types (54 total)

Defined in `enum class Blocks` in `inc/constants.hpp`. Ranges from `AIR_BLOCK`
through `TOTAL_OF_BLOCKS`. Concrete implementations in `inc/entities/blocks/`:
`BasicBlocks.hpp`, `ExtendedBlocks.hpp`, `OreBlocks.hpp`, `PlantBlocks.hpp`,
`SlabBlocks.hpp`, `SpecialBlocks.hpp`.

---

## Agent Guidelines

### Before Making Changes

1. **Read this file first** — it's your map of the project.
2. **Check `inc/constants.hpp`** — world limits, enums, and block types live here.
5. **Understand the build system** — changes must compile via `make` inside Docker.

### When Modifying Code

1. **Memory is the #1 concern** — always consider allocation impact.
2. **Mirror `inc/` and `src/`** — headers and sources must stay in sync.
3. **Follow existing patterns** — look at similar implementations before creating new ones.
4. **Test on PCSX2 mentally** — performance matters; avoid O(n²) in hot paths.
5. **Update `CHANGELOG.md`** when adding features or fixing bugs.

### When Creating New Files

1. Place headers in `inc/<subsystem>/` and sources in `src/<subsystem>/`.
2. Use the `TyraCraft` namespace.
3. Add new `.cpp` files to the Makefile's source discovery (automatic via wildcard, but verify).
4. For new block types, use `IMPLEMENT_BLOCK_CLONE` macro.

### Keeping This Documentation Updated

When making significant architectural changes, agents should also update the
relevant files in `docs/agents/`. Key triggers:

- **New manager added** → update `codebase-map.md` manager table
- **New game state added** → update `architecture.md` state diagram and `codebase-map.md`
- **New block type added** → update `codebase-map.md` block table
- **New build step added** → update `build-and-workflow.md`
- **New coding convention** → update `conventions.md`
- **Constants changed** → update quick reference tables in this file
- **New post-FX technique** → update `skill-postfx.md`
- **DMA/GIF packet changes** → verify against `skill-ee-system.md` GIFtag format
- **Rendering pipeline changes** → verify against `skill-gs-rendering.md` pixel test order

---

*Last updated: 2026-02-08 — v0.86.140-pre-alpha*
