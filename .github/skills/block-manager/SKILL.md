---
name: block-manager
description: 'Comprehensive guide for managing block types in TyraCraft. Use when: adding/creating new block types, modifying existing blocks, registering blocks in BlockManager or StaticBlockRepository, updating block properties (hardness, transparency, collidability, texture faces, sounds), working with Blocks enum, implementing block classes, configuring mesh builders (cuboid/slab/crossed/water/lava/torch), setting up block sound effects (dig/broken/step), updating block texture atlas mappings, handling light-emitting blocks, managing oriented blocks (pumpkins/jack-o-lanterns), working with slabs/vegetation ranges, or debugging block-related compilation/runtime issues.'
---

# Block Manager Skill — TyraCraft Block System

## Overview

The TyraCraft block system is a memory-optimized, template-based architecture running on PlayStation 2 hardware (32 MB RAM, 4 MB VRAM constraints). The system manages 54 block types (air through slabs) with custom rendering pipelines, sound effects, and physics properties.

**Key components:**
- `BlockManager` — Singleton orchestrator (texture loading, sound registration, block queries)
- `StaticBlockRepository` — Singleton template storage + block instantiation factory
- `Block` — Abstract entity base class with 7 pure virtual methods
- Concrete block classes — One class per block type (header-only in `inc/entities/blocks/`)
- Mesh builders — Dispatched per block type (cuboid, slab, crossed, water, lava, torch)
- Sound repositories — Dig/broken/step sound mappings per block category

**Related documentation:**
- [agents.md](../../../agents.md) — Project overview + navigation map
- [docs/agents/conventions.md](../../../docs/agents/conventions.md) — Coding patterns
- [docs/agents/codebase-map.md](../../../docs/agents/codebase-map.md) — File-by-file reference
- [references/block-registry.md](references/block-registry.md) — Complete block enum + mappings *(on-demand)*
- [references/block-class-template.md](references/block-class-template.md) — Copy-pasteable scaffolds *(on-demand)*

---

## Block Class Hierarchy

```
Entity (inc/entities/entity.hpp)
  └─ Block (inc/entities/Block.hpp) — ABSTRACT BASE CLASS
       ├─ StoneBlock, GrassBlock, DirtyBlock... (inc/entities/blocks/BasicBlocks.hpp)
       ├─ SandBlock, GlassBlock, Lava... (inc/entities/blocks/ExtendedBlocks.hpp)
       ├─ GoldOreBlock, DiamondOre... (inc/entities/blocks/OreBlocks.hpp)
       ├─ GrassPlant, Flowers... (inc/entities/blocks/PlantBlocks.hpp)
       ├─ StoneSlab, BricksSlab... (inc/entities/blocks/SlabBlocks.hpp)
       └─ CrackedStoneBricks, Wool, Logs, Torch... (inc/entities/blocks/SpecialBlocks.hpp)
```

### Required Virtual Methods (Every Block Must Implement)

| Method | Return Type | Purpose | Example Values |
|---|---|---|---|
| `getType()` | `Blocks` | Enum identifier | `Blocks::STONE_BLOCK` |
| `getHardness()` | `float` | Breaking time multiplier | `1.5f` (stone), `0.6f` (dirt), `0.0f` (bedrock) |
| `getFacesMap()` | `std::array<u8, 6>` | Texture atlas indices: `{Top, Bottom, Left, Right, Back, Front}` | `{1, 0, 0, 0, 0, 0}` = grass top |
| `isBreakable()` | `u8` | Can player destroy it? | `true` (most), `false` (bedrock) |
| `isCollidable()` | `u8` | Solid physics collision | `true` (most), `false` (air, water, plants) |
| `hasTransparency()` | `u8` | Alpha blending + face culling | `true` (glass, water, leaves), `false` (stone) |
| `isCrossed()` | `u8` | X-shaped billboard mesh | `true` (grass, flowers), `false` (cubes) |
| `clone()` | `Block*` | Copy constructor | Implemented via `IMPLEMENT_BLOCK_CLONE(ClassName)` macro |

**Memory-optimized packed struct** (`Block.hpp`):
```cpp
struct {
  u16 chunkId, localIndex, drawDataIndex;
  u8  drawDataLength, visibleFaces;  // 6-bit bitmask
  u8  visibleFacesCount : 4, isTarget : 1, reserved : 3;  // bitfields
} packed;
```

**Face visibility bitmask** (6 bits):
- `FRONT_VISIBLE (0b100000)`, `BACK_VISIBLE (0b010000)`, `LEFT_VISIBLE (0b001000)`
- `RIGHT_VISIBLE (0b000100)`, `TOP_VISIBLE (0b000010)`, `BOTTOM_VISIBLE (0b000001)`

---

## Step-by-Step: Adding a New Block Type

### Prerequisites

1. **Choose a block category** — determines which file to edit:
   - Basic blocks (stone, grass, dirt, water, bedrock) → `BasicBlocks.hpp`
   - Ores (coal, iron, gold, diamond, redstone, emerald) → `OreBlocks.hpp`
   - Plants (grass, flowers) → `PlantBlocks.hpp`
   - Slabs (half-height) → `SlabBlocks.hpp`
   - Everything else (wool, logs, glass, bricks, lava, glowstone, torch) → `ExtendedBlocks.hpp` or `SpecialBlocks.hpp`

2. **Determine block properties**:
   - Hardness (0.0f = unbreakable, 0.1–0.6f = soft, 1.0–3.0f = hard)
   - Texture faces (6 `u8` indices into 16×16 atlas at [bin/textures/blocks.png](../../../bin/textures/blocks.png))
   - Transparency (glass, water, leaves = true; most = false)
   - Mesh type (cuboid, slab, crossed, water, lava, torch)
   - Sound category (stone, wood, grass, sand, gravel, glass)
   - Collidability (false for plants/water/lava)
   - Light emission (0 = none, 14 = torch, 15 = glowstone/lava/jack-o-lantern)

### Full Registration Checklist (10 Files)

#### 1. Add to `Blocks` Enum

**File**: [inc/constants.hpp](../../../inc/constants.hpp)

**Action**: Insert new enum value **before** `TOTAL_OF_BLOCKS`. Preserve numeric ordering if possible.

```cpp
enum class Blocks : u8 {
  // ... existing blocks ...
  MOSSY_STONE_BRICKS_SLAB = 53,
  MY_NEW_BLOCK = 54,              // ← ADD HERE
  TOTAL_OF_BLOCKS = 55            // ← INCREMENT THIS
};
```

⚠️ **Critical**: Update `TOTAL_OF_BLOCKS` count.

⚠️ **Fragile ranges**: If adding a **slab**, insert between `STONE_SLAB (45)` and the next non-slab block OR update `BlockManager::isSlab()` range check. Same for **vegetation** (`GRASS (17)` through `DANDELION_FLOWER (19)`).

---

#### 2. Create Block Class

**File**: [inc/entities/blocks/<Category>Blocks.hpp](../../../inc/entities/blocks/) (choose appropriate category)

**Action**: Add new class definition. **Pattern** (copy-paste from [references/block-class-template.md](references/block-class-template.md)):

```cpp
class MyNewBlock : public Block {
 public:
  MyNewBlock() : Block() { collidable = isCollidable(); };
  virtual ~MyNewBlock() = default;

  IMPLEMENT_BLOCK_CLONE(MyNewBlock)

  Blocks getType() override { return Blocks::MY_NEW_BLOCK; }
  float getHardness() override { return 1.5f; }  // Adjust value
  
  // Texture atlas indices: {Top, Bottom, Left, Right, Back, Front}
  std::array<u8, 6> getFacesMap() override { return {10, 10, 10, 10, 10, 10}; }
  
  u8 isBreakable() override { return true; }
  u8 isCollidable() override { return true; }  // false for plants/liquids
  u8 hasTransparency() override { return false; }  // true for glass/water/leaves
  u8 isCrossed() override { return false; }  // true for grass/flowers
};
```

⚠️ **No `.cpp` file needed** — blocks are header-only.

---

#### 3. Register in StaticBlockRepository

**File**: [src/managers/block/StaticBlockRepository.cpp](../../../src/managers/block/StaticBlockRepository.cpp)

**Action A**: Add template creation in `initializeBlocks()`:

```cpp
void StaticBlockRepository::initializeBlocks() {
  // ... existing blocks ...
  blockTemplates[static_cast<size_t>(Blocks::MOSSY_STONE_BRICKS_SLAB)] =
      std::make_unique<MossyStoneBricksSlab>();
  blockTemplates[static_cast<size_t>(Blocks::MY_NEW_BLOCK)] =
      std::make_unique<MyNewBlock>();  // ← ADD THIS
}
```

**Action B**: Add factory case in `createBlock()` switch:

```cpp
Block* StaticBlockRepository::createBlock(const Blocks& blockType) {
  switch (blockType) {
    // ... existing cases ...
    case Blocks::MOSSY_STONE_BRICKS_SLAB:
      return new MossyStoneBricksSlab();
    case Blocks::MY_NEW_BLOCK:
      return new MyNewBlock();  // ← ADD THIS
    default:
      return nullptr;
  }
}
```

⚠️ **Must include header**: Add `#include "entities/blocks/<Category>Blocks.hpp"` at top of file.

---

#### 4. Register Mesh Builder

**File**: [src/managers/mesh/mesh_builder.cpp](../../../src/managers/mesh/mesh_builder.cpp)

**Action**: Add entries in **BOTH** `builders` and `light_builders` maps in constructor.

**Standard cuboid example**:
```cpp
MeshBuilder::MeshBuilder() {
  // Regular mesh builders
  builders[Blocks::STONE_BLOCK] = CuboidMeshBuilder_GenerateMesh;
  builders[Blocks::MY_NEW_BLOCK] = CuboidMeshBuilder_GenerateMesh;  // ← ADD

  // Light mesh builders
  light_builders[Blocks::STONE_BLOCK] = CuboidMeshBuilder_GenerateLightMesh;
  light_builders[Blocks::MY_NEW_BLOCK] = CuboidMeshBuilder_GenerateLightMesh;  // ← ADD
}
```

**Mesh builder options**:
- `CuboidMeshBuilder_GenerateMesh` / `_GenerateLightMesh` — standard cube (most blocks)
- `SlabMeshBuilder_GenerateMesh` / `_GenerateLightMesh` — half-height cube
- `CrossedMeshBuilder_GenerateMesh` / `_GenerateLightMesh` — X-shaped (grass, flowers)
- `WaterMeshBuilder_GenerateMesh` / `_GenerateLightMesh` — animated water
- `LavaMeshBuilder_GenerateMesh` / `_GenerateLightMesh` — animated lava
- `TorchMeshBuilder_GenerateMesh` / `_GenerateLightMesh` — narrow pillar

⚠️ **Critical**: Missing `light_builders` entry causes rendering bugs.

---

#### 5–7. Register Sound Effects (3 Files)

**Files**:
- [src/managers/block/sound/block_dig_sfx_repository.cpp](../../../src/managers/block/sound/block_dig_sfx_repository.cpp)
- [src/managers/block/sound/block_broken_sfx_repository.cpp](../../../src/managers/block/sound/block_broken_sfx_repository.cpp)
- [src/managers/block/sound/block_step_sfx_repository.cpp](../../../src/managers/block/sound/block_step_sfx_repository.cpp)

**Action**: Add `SfxBlockModel` entry in each `loadModels()` method:

```cpp
void BlockDigSfxRepository::loadModels() {
  // ... existing models ...
  models.push_back(new SfxBlockModel(Blocks::MY_NEW_BLOCK, 
                                     &SfxLibrary::stone, 
                                     SoundFX::Stone1));
}
```

**Sound categories** (from `SfxLibrary`):
- `SfxLibrary::stone` → `SoundFX::Stone1` (stone, ores, bricks)
- `SfxLibrary::wood` → `SoundFX::Wood1` (logs, planks)
- `SfxLibrary::grass` → `SoundFX::Grass1` (grass, dirt, flowers)
- `SfxLibrary::sand` → `SoundFX::Sand1` (sand)
- `SfxLibrary::gravel` → `SoundFX::Gravel1` (gravel)
- `SfxLibrary::glass` → `SoundFX::Glass1` (glass)

⚠️ **Must add to all 3 repos** (dig, broken, step) or block will be silent.

---

#### 8. (Optional) Add Light Emission

**File**: [src/managers/block_manager.cpp](../../../src/managers/block_manager.cpp)

**Action**: Update `getBlockLightValue()` if block emits light:

```cpp
u8 BlockManager::getBlockLightValue(Blocks blockType) const {
  switch (blockType) {
    case Blocks::GLOWSTONE_BLOCK:
    case Blocks::JACK_O_LANTERN_BLOCK:
    case Blocks::LAVA_BLOCK:
      return 15;
    case Blocks::TORCH:
      return 14;
    case Blocks::MY_NEW_BLOCK:  // ← ADD IF LIGHT-EMITTING
      return 12;  // 0–15 scale
    default:
      return 0;
  }
}
```

---

#### 9. (Optional) Add Oriented Block Support

**File**: [src/managers/block_manager.cpp](../../../src/managers/block_manager.cpp)

**Action**: Update `isBlockOriented()` if block has directional faces (like pumpkin/jack-o-lantern):

```cpp
u8 BlockManager::isBlockOriented(const Blocks& blockId) const {
  return blockId == Blocks::JACK_O_LANTERN_BLOCK || 
         blockId == Blocks::PUMPKIN_BLOCK ||
         blockId == Blocks::MY_NEW_BLOCK;  // ← ADD IF ORIENTED
}
```

---

#### 10. (Optional) Add Inventory Item

**Files**:
- [inc/constants.hpp](../../../inc/constants.hpp) — Add `ItemId` enum value
- [inc/managers/items_repository.hpp](../../../inc/managers/items_repository.hpp) — Add `Item` member
- [src/managers/items_repository.cpp](../../../src/managers/items_repository.cpp) — Initialize in `loadItems()`

**Pattern**:
```cpp
// inc/constants.hpp
enum class ItemId : u8 {
  // ...
  MY_NEW_ITEM = 54,
};

// inc/managers/items_repository.hpp
class ItemRepository {
  Item myNewItem;
  // ...
};

// src/managers/items_repository.cpp
void ItemRepository::loadItems() {
  myNewItem = Item(ItemId::MY_NEW_ITEM, Blocks::MY_NEW_BLOCK, 
                   "my_new_block", t_renderer);
  items.push_back(&myNewItem);
}
```

---

## Step-by-Step: Modifying an Existing Block

| Property to Change | Files to Edit | Method/Location |
|---|---|---|
| **Hardness** (breaking speed) | `inc/entities/blocks/<Category>Blocks.hpp` | `getHardness()` return value |
| **Texture faces** | `inc/entities/blocks/<Category>Blocks.hpp` | `getFacesMap()` array |
| **Transparency** | `inc/entities/blocks/<Category>Blocks.hpp` | `hasTransparency()` return value |
| **Collidability** | `inc/entities/blocks/<Category>Blocks.hpp` | `isCollidable()` return value |
| **Crossed mesh** (X-shape) | `inc/entities/blocks/<Category>Blocks.hpp` | `isCrossed()` return value |
| **Mesh type** | `src/managers/mesh/mesh_builder.cpp` | Update `builders[]` + `light_builders[]` maps |
| **Dig sound** | `src/managers/block/sound/block_dig_sfx_repository.cpp` | Update `SfxBlockModel` entry |
| **Broken sound** | `src/managers/block/sound/block_broken_sfx_repository.cpp` | Update `SfxBlockModel` entry |
| **Step sound** | `src/managers/block/sound/block_step_sfx_repository.cpp` | Update `SfxBlockModel` entry |
| **Light emission** | `src/managers/block_manager.cpp` | Add/update case in `getBlockLightValue()` |
| **Orientation** | `src/managers/block_manager.cpp` | Add/update condition in `isBlockOriented()` |

---

## Edge Cases & Gotchas

### 1. Enum Ordering and Range Checks

**Problem**: `BlockManager::isSlab()` and `BlockManager::isVegetation()` use **range comparisons** on enum values:

```cpp
bool BlockManager::isSlab(const Blocks& blockId) const {
  return blockId >= Blocks::STONE_SLAB && blockId <= Blocks::MOSSY_STONE_BRICKS_SLAB;
}
```

**Impact**: If you insert a non-slab block between `STONE_SLAB (45)` and `MOSSY_STONE_BRICKS_SLAB (53)`, it will incorrectly be treated as a slab.

**Solutions**:
- **Option A**: Always insert new slabs **within** the existing slab range (45–53)
- **Option B**: Insert new blocks **after** `MOSSY_STONE_BRICKS_SLAB` and update `TOTAL_OF_BLOCKS`
- **Option C** (refactor): Replace range checks with a `std::unordered_set` or bitfield

Same issue applies to `isVegetation()` (range 17–19).

---

### 2. VOID and AIR_BLOCK are nullptr

**Behavior**: `StaticBlockRepository::blockTemplates[0]` and `blockTemplates[1]` are **not initialized** (remain `nullptr`).

**Impact**: Calling `getBlockTemplate(Blocks::VOID)` or `getBlockTemplate(Blocks::AIR_BLOCK)` returns `nullptr`. Code must null-check before dereferencing.

**Why**: Air/void have no mesh, no texture, no physics — they're sentinel values.

---

### 3. Mesh Builder Requires BOTH Maps

**Problem**: Forgetting to add entry to `light_builders[]` map causes visual glitches (shadows, light bleed).

**Verification**: After adding a block, search `mesh_builder.cpp` for **two** occurrences of `Blocks::MY_NEW_BLOCK` — one in `builders[]`, one in `light_builders[]`.

---

### 4. Texture Atlas is 16×16 Grid

**Layout**: [bin/textures/blocks.png](../../../bin/textures/blocks.png) is a 256×256 image divided into 16×16 cells = 256 tiles.

**Indexing**: Tiles are numbered 0–255, row-major order (left-to-right, top-to-bottom).

**Example**:
- Tile 0 = top-left corner (grass side)
- Tile 1 = (1,0) in grid (grass top)
- Tile 16 = (0,1) in grid (second row, first column)

**Face map order**: `{Top, Bottom, Left, Right, Back, Front}` — **not** XYZ order.

**Debugging**: If textures appear wrong, verify atlas indices match visual tile positions in `blocks.png`.

---

### 5. Sound Repos Use Pointer Addresses

**Pattern**: `SfxBlockModel` constructor takes `SfxLibraryCategory* category` (pointer to `SfxLibrary::stone`, etc.).

**Common error**: Passing `SfxLibrary::stone` by value instead of `&SfxLibrary::stone`.

**Correct**:
```cpp
models.push_back(new SfxBlockModel(Blocks::MY_BLOCK, &SfxLibrary::stone, SoundFX::Stone1));
```

---

### 6. PS2 Memory Constraints

**Context**: PlayStation 2 has only 32 MB RAM. Every block instance matters.

**Implications**:
- `Block` uses packed structs and compressed coordinates (`s16` instead of `float`)
- `StaticBlockRepository` uses **templates** (single instance per type) + `clone()` for copies
- Greedy meshing merges adjacent coplanar faces to reduce draw calls
- Avoid allocating temporary blocks in hot paths (chunk meshing, collision detection)

**Rule**: Prefer `BlockManager::getBlockTemplateByType()` for read-only queries over creating new instances.

---

### 7. Transparency Affects Face Culling

**Behavior**: If `Block::hasTransparency()` returns `true`, adjacent solid blocks **will** render faces touching this block.

**Use case**: Glass, water, leaves need this so you can see through them.

**Bug**: Setting `hasTransparency() = true` on opaque blocks causes overdraw (performance hit) and visual artifacts (z-fighting).

---

## File Quick-Reference

| File | Purpose |
|---|---|
| [inc/constants.hpp](../../../inc/constants.hpp) | `Blocks` enum (54 values), `ItemId` enum, world constants |
| [inc/entities/Block.hpp](../../../inc/entities/Block.hpp) | Abstract `Block` base class, face bitmasks, `IMPLEMENT_BLOCK_CLONE` macro |
| [inc/entities/blocks/*.hpp](../../../inc/entities/blocks/) | Concrete block class definitions (6 files, header-only) |
| [inc/managers/block_manager.hpp](../../../inc/managers/block_manager.hpp) | `BlockManager` singleton (texture, sound, queries) |
| [src/managers/block_manager.cpp](../../../src/managers/block_manager.cpp) | Light emission, oriented blocks, slab/vegetation checks |
| [inc/managers/block/StaticBlockRepository.hpp](../../../inc/managers/block/StaticBlockRepository.hpp) | Template storage + block factory singleton |
| [src/managers/block/StaticBlockRepository.cpp](../../../src/managers/block/StaticBlockRepository.cpp) | **Register here**: `initializeBlocks()` + `createBlock()` switch |
| [src/managers/mesh/mesh_builder.cpp](../../../src/managers/mesh/mesh_builder.cpp) | **Register here**: `builders[]` + `light_builders[]` maps |
| [src/managers/block/sound/block_dig_sfx_repository.cpp](../../../src/managers/block/sound/block_dig_sfx_repository.cpp) | **Register here**: Dig sound mappings |
| [src/managers/block/sound/block_broken_sfx_repository.cpp](../../../src/managers/block/sound/block_broken_sfx_repository.cpp) | **Register here**: Broken sound mappings |
| [src/managers/block/sound/block_step_sfx_repository.cpp](../../../src/managers/block/sound/block_step_sfx_repository.cpp) | **Register here**: Step sound mappings |
| [inc/managers/block/uv_block_data.hpp](../../../inc/managers/block/uv_block_data.hpp) | Static UV coordinate data for mesh generation |
| [inc/managers/block/vertex_block_data.hpp](../../../inc/managers/block/vertex_block_data.hpp) | Vertex positions for cuboid/slab/torch/crossed meshes |
| [inc/managers/items_repository.hpp](../../../inc/managers/items_repository.hpp) | Inventory item definitions (optional link to blocks) |
| [inc/entities/level.hpp](../../../inc/entities/level.hpp) | World storage: `map.blocks[OVERWORLD_SIZE]` array |

---

## Detailed Reference Tables

For complete listings of all 54 blocks with their properties, sound mappings, mesh builder assignments, and face maps, see:

**[references/block-registry.md](references/block-registry.md)** — Full `Blocks` enum table, concrete class properties, sound/mesh mappings

For copy-pasteable code templates when creating new blocks, see:

**[references/block-class-template.md](references/block-class-template.md)** — Scaffolds for block class + all registration snippets

---

## Debugging Checklist

**Compilation errors after adding a block:**
- [ ] Did you include the block header in `StaticBlockRepository.cpp`?
- [ ] Did you increment `TOTAL_OF_BLOCKS` in `constants.hpp`?
- [ ] Did you add the block class to the correct category header file?

**Block not appearing in game:**
- [ ] Is the block registered in `StaticBlockRepository::initializeBlocks()`?
- [ ] Is the block registered in `StaticBlockRepository::createBlock()` switch?
- [ ] Did you add an inventory `Item` entry in `ItemRepository`?

**Block renders with wrong texture:**
- [ ] Check `getFacesMap()` return values (0–255 atlas indices)
- [ ] Verify texture pack loaded: [bin/textures/blocks.png](../../../bin/textures/blocks.png)
- [ ] Face map order is `{Top, Bottom, Left, Right, Back, Front}`, not XYZ

**Block has no sound:**
- [ ] Did you add entries to **all 3** sound repos (dig, broken, step)?
- [ ] Did you pass `&SfxLibrary::categoryName` (with `&`) to `SfxBlockModel`?

**Block renders with wrong mesh shape:**
- [ ] Check `builders[]` map in `mesh_builder.cpp` — points to correct builder function?
- [ ] Did you add **both** `builders[]` and `light_builders[]` entries?

**Block doesn't emit light / has wrong lighting:**
- [ ] For light sources: added case in `BlockManager::getBlockLightValue()`?
- [ ] For transparent blocks: `hasTransparency()` returns `true`?
- [ ] Did you add `light_builders[]` entry in `mesh_builder.cpp`?

**Block breaks range checks (slabs/vegetation):**
- [ ] If adding a slab: inserted within `STONE_SLAB (45)` – `MOSSY_STONE_BRICKS_SLAB (53)` range?
- [ ] If adding vegetation: inserted within `GRASS (17)` – `DANDELION_FLOWER (19)` range?
- [ ] Otherwise: check `BlockManager::isSlab()` / `isVegetation()` logic

---

*Last updated: 2026-02-08 (TyraCraft v0.86.140-pre-alpha)*