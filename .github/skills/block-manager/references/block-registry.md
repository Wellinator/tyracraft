# Block Registry — Complete Reference Tables

> **On-demand reference** for the `block-manager` skill. Contains all 54 block types with their properties, sound mappings, and mesh builder assignments.

---

## Complete Blocks Enum

From [inc/constants.hpp](../../../../inc/constants.hpp):

| Enum Value | Numeric ID | Category |
|---|---|---|
| `VOID` | 0 | Sentinel (nullptr in templates) |
| `AIR_BLOCK` | 1 | Sentinel (nullptr in templates) |
| `STONE_BLOCK` | 2 | Basic |
| `GRASS_BLOCK` | 3 | Basic |
| `DIRTY_BLOCK` | 4 | Basic |
| `WATER_BLOCK` | 5 | Basic (liquid) |
| `BEDROCK_BLOCK` | 6 | Basic (unbreakable) |
| `SAND_BLOCK` | 7 | Extended |
| `GLASS_BLOCK` | 8 | Extended (transparent) |
| `BRICKS_BLOCK` | 9 | Extended |
| `GRAVEL_BLOCK` | 10 | Extended |
| `GOLD_ORE_BLOCK` | 11 | Ore |
| `REDSTONE_ORE_BLOCK` | 12 | Ore |
| `IRON_ORE_BLOCK` | 13 | Ore |
| `EMERALD_ORE_BLOCK` | 14 | Ore |
| `DIAMOND_ORE_BLOCK` | 15 | Ore |
| `COAL_ORE_BLOCK` | 16 | Ore |
| `GRASS` | 17 | Plant (crossed) |
| `POPPY_FLOWER` | 18 | Plant (crossed) |
| `DANDELION_FLOWER` | 19 | Plant (crossed) |
| `OAK_PLANKS_BLOCK` | 20 | Extended |
| `SPRUCE_PLANKS_BLOCK` | 21 | Extended |
| `BIRCH_PLANKS_BLOCK` | 22 | Extended |
| `ACACIA_PLANKS_BLOCK` | 23 | Extended |
| `STONE_BRICK_BLOCK` | 24 | Special |
| `CRACKED_STONE_BRICKS_BLOCK` | 25 | Special |
| `MOSSY_STONE_BRICKS_BLOCK` | 26 | Special |
| `CHISELED_STONE_BRICKS_BLOCK` | 27 | Special |
| `YELLOW_WOOL` | 28 | Special |
| `WHITE_WOOL` | 29 | Special |
| `RED_WOOL` | 30 | Special |
| `PURPLE_WOOL` | 31 | Special |
| `PINK_WOOL` | 32 | Special |
| `ORANGE_WOOL` | 33 | Special |
| `LIGHT_BLUE_WOOL` | 34 | Special |
| `BLACK_WOOL` | 35 | Special |
| `GLOWSTONE_BLOCK` | 36 | Extended (light=15) |
| `JACK_O_LANTERN_BLOCK` | 37 | Extended (light=15, oriented) |
| `PUMPKIN_BLOCK` | 38 | Extended (oriented) |
| `LAVA_BLOCK` | 39 | Extended (liquid, light=15) |
| `OAK_LOG_BLOCK` | 40 | Special |
| `OAK_LEAVES_BLOCK` | 41 | Special (transparent) |
| `BIRCH_LOG_BLOCK` | 42 | Special |
| `BIRCH_LEAVES_BLOCK` | 43 | Special (transparent) |
| `TORCH` | 44 | Special (light=14) |
| `STONE_SLAB` | 45 | Slab (half-height) |
| `BRICKS_SLAB` | 46 | Slab |
| `OAK_PLANKS_SLAB` | 47 | Slab |
| `SPRUCE_PLANKS_SLAB` | 48 | Slab |
| `ACACIA_PLANKS_SLAB` | 49 | Slab |
| `BIRCH_PLANKS_SLAB` | 50 | Slab |
| `CRACKED_STONE_BRICKS_SLAB` | 51 | Slab |
| `STONE_BRICK_SLAB` | 52 | Slab |
| `MOSSY_STONE_BRICKS_SLAB` | 53 | Slab |
| `TOTAL_OF_BLOCKS` | 54 | Sentinel (count) |

---

## Block Properties by Class

### BasicBlocks.hpp

| Class | Hardness | Faces {T,B,L,R,Bk,F} | Transparent | Collidable | Breakable | Crossed |
|---|---|---|---|---|---|---|
| `StoneBlock` | 1.5f | {2,2,2,2,2,2} | false | true | true | false |
| `GrassBlock` | 0.6f | {1,0,0,0,0,0} | false | true | true | false |
| `DirtyBlock` | 0.5f | {0,0,0,0,0,0} | false | true | true | false |
| `WaterBlock` | 0.0f | {205,205,205,205,205,205} | true | false | false | false |
| `BedrockBlock` | 0.0f | {10,10,10,10,10,10} | false | true | false | false |

### ExtendedBlocks.hpp

| Class | Hardness | Faces | Transparent | Collidable | Breakable | Crossed |
|---|---|---|---|---|---|---|
| `SandBlock` | 0.5f | {18,18,18,18,18,18} | false | true | true | false |
| `GlassBlock` | 0.3f | {49,49,49,49,49,49} | true | true | true | false |
| `BricksBlock` | 2.0f | {7,7,7,7,7,7} | false | true | true | false |
| `GravelBlock` | 0.6f | {19,19,19,19,19,19} | false | true | true | false |
| `PumpkinBlock` | 1.0f | {102,102,118,118,118,119} | false | true | true | false |
| `OakPlanksBlock` | 2.0f | {4,4,4,4,4,4} | false | true | true | false |
| `SprucePlanksBlock` | 2.0f | {198,198,198,198,198,198} | false | true | true | false |
| `AcaciaPlanksBlock` | 2.0f | {214,214,214,214,214,214} | false | true | true | false |
| `BirchPlanksBlock` | 2.0f | {197,197,197,197,197,197} | false | true | true | false |
| `GlowstoneBlock` | 0.3f | {105,105,105,105,105,105} | false | true | true | false |
| `JackOLanternBlock` | 1.0f | {102,102,118,118,118,120} | false | true | true | false |
| `LavaBlock` | 0.0f | {237,237,237,237,237,237} | false | false | false | false |

### OreBlocks.hpp

| Class | Hardness | Faces | Transparent | Collidable | Breakable | Crossed |
|---|---|---|---|---|---|---|
| `GoldOreBlock` | 3.0f | {32,32,32,32,32,32} | false | true | true | false |
| `RedstoneOreBlock` | 3.0f | {51,51,51,51,51,51} | false | true | true | false |
| `IronOreBlock` | 3.0f | {33,33,33,33,33,33} | false | true | true | false |
| `EmeraldOreBlock` | 3.0f | {186,186,186,186,186,186} | false | true | true | false |
| `DiamondOreBlock` | 3.0f | {50,50,50,50,50,50} | false | true | true | false |
| `CoalOreBlock` | 3.0f | {34,34,34,34,34,34} | false | true | true | false |

### PlantBlocks.hpp (Vegetation)

| Class | Hardness | Faces | Transparent | Collidable | Breakable | Crossed |
|---|---|---|---|---|---|---|
| `GrassPlant` | 0.0f | {39,39,39,39,39,39} | true | false | true | true |
| `PoppyFlower` | 0.0f | {12,12,12,12,12,12} | true | false | true | true |
| `DandelionFlower` | 0.0f | {13,13,13,13,13,13} | true | false | true | true |

### SlabBlocks.hpp

| Class | Hardness | Faces | Transparent | Collidable | Breakable | Crossed |
|---|---|---|---|---|---|---|
| `StoneSlab` | 2.0f | {6,6,6,6,6,6} | false | true | true | false |
| `BricksSlab` | 2.0f | {7,7,7,7,7,7} | false | true | true | false |
| `OakPlanksSlab` | 2.0f | {4,4,4,4,4,4} | false | true | true | false |
| `SprucePlanksSlab` | 2.0f | {198,198,198,198,198,198} | false | true | true | false |
| `AcaciaPlanksSlab` | 2.0f | {214,214,214,214,214,214} | false | true | true | false |
| `BirchPlanksSlab` | 2.0f | {197,197,197,197,197,197} | false | true | true | false |
| `CrackedStoneBricksSlab` | 2.0f | {101,101,101,101,101,101} | false | true | true | false |
| `StoneBrickSlab` | 2.0f | {54,54,54,54,54,54} | false | true | true | false |
| `MossyStoneBricksSlab` | 2.0f | {100,100,100,100,100,100} | false | true | true | false |

### SpecialBlocks.hpp

| Class | Hardness | Faces | Transparent | Collidable | Breakable | Crossed |
|---|---|---|---|---|---|---|
| `CrackedStoneBricksBlock` | 1.5f | {101,101,101,101,101,101} | false | true | true | false |
| `StoneBrickBlock` | 1.5f | {54,54,54,54,54,54} | false | true | true | false |
| `MossyStoneBricksBlock` | 1.5f | {100,100,100,100,100,100} | false | true | true | false |
| `ChiseledStoneBricksBlock` | 1.5f | {53,53,53,53,53,53} | false | true | true | false |
| `YellowWool` | 0.8f | {162,162,162,162,162,162} | false | true | true | false |
| `WhiteWool` | 0.8f | {64,64,64,64,64,64} | false | true | true | false |
| `RedWool` | 0.8f | {161,161,161,161,161,161} | false | true | true | false |
| `PurpleWool` | 0.8f | {178,178,178,178,178,178} | false | true | true | false |
| `PinkWool` | 0.8f | {179,179,179,179,179,179} | false | true | true | false |
| `OrangeWool` | 0.8f | {177,177,177,177,177,177} | false | true | true | false |
| `LightBlueWool` | 0.8f | {163,163,163,163,163,163} | false | true | true | false |
| `BlackWool` | 0.8f | {176,176,176,176,176,176} | false | true | true | false |
| `OakLogBlock` | 2.0f | {21,21,20,20,20,20} | false | true | true | false |
| `BirchLogBlock` | 2.0f | {213,213,212,212,212,212} | false | true | true | false |
| `BirchLeavesBlock` | 0.2f | {196,196,196,196,196,196} | true | true | true | false |
| `OakLeavesBlock` | 0.2f | {52,52,52,52,52,52} | true | true | true | false |
| `Torch` | 0.0f | {80,80,80,80,80,80} | true | false | true | false |

---

## Sound Mappings

### Dig Sounds (`block_dig_sfx_repository.cpp`)

| Sound Category | SoundFX | Block Types |
|---|---|---|
| `SfxLibrary::stone` | `SoundFX::Stone1` | STONE, all ores, BRICKS, STONE_BRICK variants, GLOWSTONE, BEDROCK, all stone slabs |
| `SfxLibrary::grass` | `SoundFX::Grass1` | GRASS_BLOCK, DIRTY, GRASS (plant), flowers, PUMPKIN, JACK_O_LANTERN |
| `SfxLibrary::sand` | `SoundFX::Sand1` | SAND |
| `SfxLibrary::gravel` | `SoundFX::Gravel1` | GRAVEL |
| `SfxLibrary::wood` | `SoundFX::Wood1` | All PLANKS, all LOG blocks, all LEAVES, all plank slabs |
| `SfxLibrary::glass` | `SoundFX::Glass1` | GLASS |

**Wool blocks** use `SfxLibrary::grass` + `SoundFX::Grass1`.  
**Torch** uses `SfxLibrary::wood` + `SoundFX::Wood1`.  
**WATER / LAVA** have no dig sounds (cannot be broken).

### Broken Sounds (`block_broken_sfx_repository.cpp`)

Same mappings as dig sounds (categories + SoundFX values match 1:1).

### Step Sounds (`block_step_sfx_repository.cpp`)

Same mappings as dig sounds (categories + SoundFX values match 1:1).

---

## Mesh Builder Assignments

From [src/managers/mesh/mesh_builder.cpp](../../../../src/managers/mesh/mesh_builder.cpp):

| Mesh Builder Function | Block Types |
|---|---|
| `CuboidMeshBuilder_GenerateMesh` | STONE, GRASS_BLOCK, DIRTY, BEDROCK, SAND, GLASS, BRICKS, GRAVEL, all ores, all PLANKS, all STONE_BRICK variants, all WOOL, GLOWSTONE, PUMPKIN, JACK_O_LANTERN, all LOGS, all LEAVES |
| `SlabMeshBuilder_GenerateMesh` | All 9 slab types (STONE_SLAB through MOSSY_STONE_BRICKS_SLAB) |
| `CrossedMeshBuilder_GenerateMesh` | GRASS (plant), POPPY_FLOWER, DANDELION_FLOWER |
| `WaterMeshBuilder_GenerateMesh` | WATER_BLOCK |
| `LavaMeshBuilder_GenerateMesh` | LAVA_BLOCK |
| `TorchMeshBuilder_GenerateMesh` | TORCH |

**Critical**: Each block must have entries in **both** `builders` and `light_builders` maps, using the corresponding `_GenerateMesh` and `_GenerateLightMesh` variants.

---

## Light Emission Values

From [src/managers/block_manager.cpp](../../../../src/managers/block_manager.cpp) `getBlockLightValue()`:

| Block Type | Light Value (0–15) |
|---|---|
| `GLOWSTONE_BLOCK` | 15 |
| `JACK_O_LANTERN_BLOCK` | 15 |
| `LAVA_BLOCK` | 15 |
| `TORCH` | 14 |
| *(all others)* | 0 |

---

## Oriented Blocks

From [src/managers/block_manager.cpp](../../../../src/managers/block_manager.cpp) `isBlockOriented()`:

Only **2 blocks** are directional (face orientation changes based on player rotation when placed):
- `JACK_O_LANTERN_BLOCK`
- `PUMPKIN_BLOCK`

These blocks have different texture atlas indices for front vs. sides in their `getFacesMap()`.

---

## Range Check Groups

### Slabs (`BlockManager::isSlab()`)

**Range**: `Blocks::STONE_SLAB (45)` through `Blocks::MOSSY_STONE_BRICKS_SLAB (53)`

⚠️ **Fragile**: Uses `>=` and `<=` comparison. Do not insert non-slab blocks within this range.

### Vegetation (`BlockManager::isVegetation()`)

**Range**: `Blocks::GRASS (17)` through `Blocks::DANDELION_FLOWER (19)`

⚠️ **Fragile**: Uses `>=` and `<=` comparison. Do not insert non-plant blocks within this range.

---

## Face Map Order

The `getFacesMap()` return value is a 6-element array with this exact order:

```cpp
std::array<u8, 6> { Top, Bottom, Left, Right, Back, Front }
```

**Not** XYZ order. Indices are 0–255 (texture atlas tile IDs).

**Texture atlas layout**: [bin/textures/blocks.png](../../../../bin/textures/blocks.png) is 256×256 pixels = 16×16 grid of 16×16 tiles.

Tile indexing is row-major (left-to-right, top-to-bottom):
- Tile 0 = (0,0) top-left
- Tile 15 = (15,0) top-right
- Tile 16 = (0,1) second row, first column
- Tile 255 = (15,15) bottom-right

---

*Last updated: 2026-02-08 (TyraCraft v0.86.140-pre-alpha)*
