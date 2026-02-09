# Block Class Template — Copy-Pasteable Scaffolds

> **On-demand reference** for the `block-manager` skill. Provides ready-to-use code templates for creating new block types.

---

## Complete Block Creation Scaffold

### 1. Block Class Definition (Header)

**File**: [inc/entities/blocks/<Category>Blocks.hpp](../../../../inc/entities/blocks/) (choose appropriate category)

```cpp
// Add to existing category file or create new file
// For new files, also add: #include "entities/Block.hpp"

class MyNewBlock : public Block {
 public:
  MyNewBlock() : Block() { collidable = isCollidable(); };
  virtual ~MyNewBlock() = default;

  IMPLEMENT_BLOCK_CLONE(MyNewBlock)

  Blocks getType() override { return Blocks::MY_NEW_BLOCK; }
  
  // Adjust hardness: 0.0f = unbreakable, 0.1–0.6f = soft, 1.0–3.0f = hard
  float getHardness() override { return 1.5f; }
  
  // Texture atlas indices: {Top, Bottom, Left, Right, Back, Front}
  // Replace with actual tile indices from bin/textures/blocks.png (16×16 grid, 0–255)
  std::array<u8, 6> getFacesMap() override { 
    return {10, 10, 10, 10, 10, 10}; 
  }
  
  u8 isBreakable() override { return true; }      // false for bedrock-like
  u8 isCollidable() override { return true; }     // false for plants/liquids
  u8 hasTransparency() override { return false; } // true for glass/water/leaves
  u8 isCrossed() override { return false; }       // true for grass/flowers
};
```

---

### 2. Constants Enum Registration

**File**: [inc/constants.hpp](../../../../inc/constants.hpp)

```cpp
enum class Blocks : u8 {
  // ... existing blocks ...
  MOSSY_STONE_BRICKS_SLAB = 53,
  
  // ADD NEW BLOCK HERE (before TOTAL_OF_BLOCKS)
  MY_NEW_BLOCK = 54,
  
  // UPDATE THIS COUNT
  TOTAL_OF_BLOCKS = 55  // Increment by 1
};
```

⚠️ **Critical**: Always increment `TOTAL_OF_BLOCKS` after adding a new enum value.

---

### 3. StaticBlockRepository Registration

**File**: [src/managers/block/StaticBlockRepository.cpp](../../../../src/managers/block/StaticBlockRepository.cpp)

**Step A**: Add include at top of file:
```cpp
#include "entities/blocks/<Category>Blocks.hpp"  // Adjust category filename
```

**Step B**: Add template creation in `initializeBlocks()`:
```cpp
void StaticBlockRepository::initializeBlocks() {
  // ... existing blocks ...
  
  blockTemplates[static_cast<size_t>(Blocks::MOSSY_STONE_BRICKS_SLAB)] =
      std::make_unique<MossyStoneBricksSlab>();
      
  // ADD THIS
  blockTemplates[static_cast<size_t>(Blocks::MY_NEW_BLOCK)] =
      std::make_unique<MyNewBlock>();
}
```

**Step C**: Add factory case in `createBlock()` switch:
```cpp
Block* StaticBlockRepository::createBlock(const Blocks& blockType) {
  switch (blockType) {
    // ... existing cases ...
    
    case Blocks::MOSSY_STONE_BRICKS_SLAB:
      return new MossyStoneBricksSlab();
      
    // ADD THIS
    case Blocks::MY_NEW_BLOCK:
      return new MyNewBlock();
      
    default:
      return nullptr;
  }
}
```

---

### 4. Mesh Builder Registration

**File**: [src/managers/mesh/mesh_builder.cpp](../../../../src/managers/mesh/mesh_builder.cpp)

Add to **BOTH** `builders` and `light_builders` maps in `MeshBuilder::MeshBuilder()` constructor:

**Standard cuboid block** (most common):
```cpp
MeshBuilder::MeshBuilder() {
  // Regular mesh builders
  builders[Blocks::STONE_BLOCK] = CuboidMeshBuilder_GenerateMesh;
  // ... existing entries ...
  builders[Blocks::MY_NEW_BLOCK] = CuboidMeshBuilder_GenerateMesh;  // ← ADD

  // Light mesh builders
  light_builders[Blocks::STONE_BLOCK] = CuboidMeshBuilder_GenerateLightMesh;
  // ... existing entries ...
  light_builders[Blocks::MY_NEW_BLOCK] = CuboidMeshBuilder_GenerateLightMesh;  // ← ADD
}
```

**Alternative mesh types**:

| Mesh Type | Regular Builder | Light Builder |
|---|---|---|
| Half-height cube | `SlabMeshBuilder_GenerateMesh` | `SlabMeshBuilder_GenerateLightMesh` |
| X-shaped (plants) | `CrossedMeshBuilder_GenerateMesh` | `CrossedMeshBuilder_GenerateLightMesh` |
| Animated water | `WaterMeshBuilder_GenerateMesh` | `WaterMeshBuilder_GenerateLightMesh` |
| Animated lava | `LavaMeshBuilder_GenerateMesh` | `LavaMeshBuilder_GenerateLightMesh` |
| Narrow pillar | `TorchMeshBuilder_GenerateMesh` | `TorchMeshBuilder_GenerateLightMesh` |

⚠️ **Critical**: Missing `light_builders` entry causes visual glitches.

---

### 5. Sound Registration (3 Files)

#### Dig Sound

**File**: [src/managers/block/sound/block_dig_sfx_repository.cpp](../../../../src/managers/block/sound/block_dig_sfx_repository.cpp)

```cpp
void BlockDigSfxRepository::loadModels() {
  // ... existing models ...
  
  // ADD THIS (choose appropriate category and sound)
  models.push_back(new SfxBlockModel(Blocks::MY_NEW_BLOCK, 
                                     &SfxLibrary::stone,  // Category
                                     SoundFX::Stone1));   // Sound
}
```

#### Broken Sound

**File**: [src/managers/block/sound/block_broken_sfx_repository.cpp](../../../../src/managers/block/sound/block_broken_sfx_repository.cpp)

```cpp
void BlockBrokenSfxRepository::loadModels() {
  // ... existing models ...
  
  // ADD THIS (usually same as dig sound)
  models.push_back(new SfxBlockModel(Blocks::MY_NEW_BLOCK, 
                                     &SfxLibrary::stone, 
                                     SoundFX::Stone1));
}
```

#### Step Sound

**File**: [src/managers/block/sound/block_step_sfx_repository.cpp](../../../../src/managers/block/sound/block_step_sfx_repository.cpp)

```cpp
void BlockStepSfxRepository::loadModels() {
  // ... existing models ...
  
  // ADD THIS (usually same as dig sound)
  models.push_back(new SfxBlockModel(Blocks::MY_NEW_BLOCK, 
                                     &SfxLibrary::stone, 
                                     SoundFX::Stone1));
}
```

**Sound category options**:
```cpp
// Choose category + sound combination:
&SfxLibrary::stone,  SoundFX::Stone1  // Stone, ores, bricks
&SfxLibrary::wood,   SoundFX::Wood1   // Logs, planks, leaves
&SfxLibrary::grass,  SoundFX::Grass1  // Grass, dirt, flowers, wool
&SfxLibrary::sand,   SoundFX::Sand1   // Sand
&SfxLibrary::gravel, SoundFX::Gravel1 // Gravel
&SfxLibrary::glass,  SoundFX::Glass1  // Glass
```

⚠️ **Critical**: Pass category as pointer (`&SfxLibrary::stone`), not by value.

---

### 6. (Optional) Light Emission

**File**: [src/managers/block_manager.cpp](../../../../src/managers/block_manager.cpp)

Only add this if your block emits light (torch, glowstone, lava, etc.):

```cpp
u8 BlockManager::getBlockLightValue(Blocks blockType) const {
  switch (blockType) {
    case Blocks::GLOWSTONE_BLOCK:
    case Blocks::JACK_O_LANTERN_BLOCK:
    case Blocks::LAVA_BLOCK:
      return 15;  // Brightest
      
    case Blocks::TORCH:
      return 14;
      
    // ADD THIS IF YOUR BLOCK EMITS LIGHT
    case Blocks::MY_NEW_BLOCK:
      return 12;  // 0–15 scale (0=no light, 15=max)
      
    default:
      return 0;
  }
}
```

---

### 7. (Optional) Oriented Block

**File**: [src/managers/block_manager.cpp](../../../../src/managers/block_manager.cpp)

Only add this if your block has directional faces (like pumpkin/jack-o-lantern):

```cpp
u8 BlockManager::isBlockOriented(const Blocks& blockId) const {
  return blockId == Blocks::JACK_O_LANTERN_BLOCK || 
         blockId == Blocks::PUMPKIN_BLOCK ||
         blockId == Blocks::MY_NEW_BLOCK;  // ← ADD IF ORIENTED
}
```

**Note**: Oriented blocks typically have different texture indices for front vs. sides:
```cpp
// Example getFacesMap() for oriented block:
std::array<u8, 6> getFacesMap() override { 
  return {102, 102, 118, 118, 118, 119};  // Front (119) differs from sides (118)
}
```

---

### 8. (Optional) Inventory Item

#### Constants Enum

**File**: [inc/constants.hpp](../../../../inc/constants.hpp)

```cpp
enum class ItemId : u8 {
  // ... existing items ...
  MOSSY_STONE_BRICKS_SLAB_ITEM = 53,
  
  MY_NEW_ITEM = 54,  // ADD THIS (usually same ID as Blocks enum)
};
```

#### Item Repository Header

**File**: [inc/managers/items_repository.hpp](../../../../inc/managers/items_repository.hpp)

```cpp
class ItemRepository {
 public:
  // ... existing items ...
  Item mossyStoneBricksSlabItem;
  
  Item myNewItem;  // ADD THIS
  
  // ... methods ...
};
```

#### Item Repository Source

**File**: [src/managers/items_repository.cpp](../../../../src/managers/items_repository.cpp)

```cpp
void ItemRepository::loadItems() {
  // ... existing items ...
  
  mossyStoneBricksSlabItem = Item(ItemId::MOSSY_STONE_BRICKS_SLAB_ITEM,
                                   Blocks::MOSSY_STONE_BRICKS_SLAB,
                                   "mossy_stone_bricks_slab",
                                   t_renderer);
  items.push_back(&mossyStoneBricksSlabItem);
  
  // ADD THIS
  myNewItem = Item(ItemId::MY_NEW_ITEM,
                   Blocks::MY_NEW_BLOCK,
                   "my_new_block",  // Translation key (see lang/*.json)
                   t_renderer);
  items.push_back(&myNewItem);
}
```

---

## Specialized Templates

### Slab Block Template

For half-height blocks (already handled by `SlabMeshBuilder`):

```cpp
class MyCustomSlab : public Block {
 public:
  MyCustomSlab() : Block() { collidable = isCollidable(); };
  virtual ~MyCustomSlab() = default;
  
  IMPLEMENT_BLOCK_CLONE(MyCustomSlab)
  
  Blocks getType() override { return Blocks::MY_CUSTOM_SLAB; }
  float getHardness() override { return 2.0f; }  // Slabs usually harder
  
  // Same texture on all faces for simple slabs
  std::array<u8, 6> getFacesMap() override { 
    return {54, 54, 54, 54, 54, 54}; 
  }
  
  u8 isBreakable() override { return true; }
  u8 isCollidable() override { return true; }
  u8 hasTransparency() override { return false; }
  u8 isCrossed() override { return false; }
};
```

⚠️ **Critical**: Insert slab enum value **within** the existing slab range (45–53) OR update `BlockManager::isSlab()` range check.

---

### Plant/Flower Template (Crossed Mesh)

For vegetation with X-shaped billboards:

```cpp
class MyCustomFlower : public Block {
 public:
  MyCustomFlower() : Block() { collidable = isCollidable(); };
  virtual ~MyCustomFlower() = default;
  
  IMPLEMENT_BLOCK_CLONE(MyCustomFlower)
  
  Blocks getType() override { return Blocks::MY_CUSTOM_FLOWER; }
  float getHardness() override { return 0.0f; }  // Instant break
  
  // All faces use same texture (crossed mesh only shows 2 faces)
  std::array<u8, 6> getFacesMap() override { 
    return {40, 40, 40, 40, 40, 40}; 
  }
  
  u8 isBreakable() override { return true; }
  u8 isCollidable() override { return false; }   // Walk through
  u8 hasTransparency() override { return true; }  // See through stems
  u8 isCrossed() override { return true; }        // X-shaped mesh
};
```

⚠️ **Critical**: Insert vegetation enum value **within** the existing vegetation range (17–19) OR update `BlockManager::isVegetation()` range check.

Use `CrossedMeshBuilder_GenerateMesh` in mesh builder registration.

---

### Transparent Block Template (Glass/Leaves)

For see-through blocks:

```cpp
class MyTransparentBlock : public Block {
 public:
  MyTransparentBlock() : Block() { collidable = isCollidable(); };
  virtual ~MyTransparentBlock() = default;
  
  IMPLEMENT_BLOCK_CLONE(MyTransparentBlock)
  
  Blocks getType() override { return Blocks::MY_TRANSPARENT_BLOCK; }
  float getHardness() override { return 0.3f; }  // Glass-like
  
  std::array<u8, 6> getFacesMap() override { 
    return {60, 60, 60, 60, 60, 60}; 
  }
  
  u8 isBreakable() override { return true; }
  u8 isCollidable() override { return true; }     // Solid collision
  u8 hasTransparency() override { return true; }  // CRITICAL FOR TRANSPARENCY
  u8 isCrossed() override { return false; }
};
```

⚠️ **Impact**: Setting `hasTransparency() = true` causes adjacent solid blocks to render their faces (allowing you to see through this block). Don't set on opaque blocks (causes overdraw + z-fighting).

---

### Light-Emitting Block Template

For blocks that glow (torch, glowstone, lava):

```cpp
class MyGlowingBlock : public Block {
 public:
  MyGlowingBlock() : Block() { collidable = isCollidable(); };
  virtual ~MyGlowingBlock() = default;
  
  IMPLEMENT_BLOCK_CLONE(MyGlowingBlock)
  
  Blocks getType() override { return Blocks::MY_GLOWING_BLOCK; }
  float getHardness() override { return 0.3f; }
  
  std::array<u8, 6> getFacesMap() override { 
    return {105, 105, 105, 105, 105, 105};  // Bright texture
  }
  
  u8 isBreakable() override { return true; }
  u8 isCollidable() override { return true; }
  u8 hasTransparency() override { return false; }
  u8 isCrossed() override { return false; }
};
```

⚠️ **Don't forget**: Update `BlockManager::getBlockLightValue()` (see section 6 above) to return 14–15 for this block.

---

### Liquid Block Template (Water/Lava)

For flowing liquids (requires custom mesh builder):

```cpp
class MyCustomLiquid : public Block {
 public:
  MyCustomLiquid() : Block() { collidable = isCollidable(); };
  virtual ~MyCustomLiquid() = default;
  
  IMPLEMENT_BLOCK_CLONE(MyCustomLiquid)
  
  Blocks getType() override { return Blocks::MY_CUSTOM_LIQUID; }
  float getHardness() override { return 0.0f; }  // Unbreakable
  
  // All faces same (animated texture)
  std::array<u8, 6> getFacesMap() override { 
    return {220, 220, 220, 220, 220, 220}; 
  }
  
  u8 isBreakable() override { return false; }     // Cannot break liquids
  u8 isCollidable() override { return false; }    // Walk through
  u8 hasTransparency() override { return true; }  // See through
  u8 isCrossed() override { return false; }
};
```

⚠️ **Special mesh builder**: Use `WaterMeshBuilder_GenerateMesh` or `LavaMeshBuilder_GenerateMesh` (both handle animated textures). If creating new liquid, you may need to implement a custom mesh builder.

---

### Ore Block Template

For underground resources:

```cpp
class MyCustomOre : public Block {
 public:
  MyCustomOre() : Block() { collidable = isCollidable(); };
  virtual ~MyCustomOre() = default;
  
  IMPLEMENT_BLOCK_CLONE(MyCustomOre)
  
  Blocks getType() override { return Blocks::MY_CUSTOM_ORE; }
  float getHardness() override { return 3.0f; }  // All ores are 3.0f
  
  std::array<u8, 6> getFacesMap() override { 
    return {80, 80, 80, 80, 80, 80};  // Stone with ore texture
  }
  
  u8 isBreakable() override { return true; }
  u8 isCollidable() override { return true; }
  u8 hasTransparency() override { return false; }
  u8 isCrossed() override { return false; }
};
```

⚠️ **Sound**: Use `&SfxLibrary::stone` + `SoundFX::Stone1` for consistency with existing ores.

---

### Wool Block Template

For decorative colored blocks:

```cpp
class MyColorWool : public Block {
 public:
  MyColorWool() : Block() { collidable = isCollidable(); };
  virtual ~MyColorWool() = default;
  
  IMPLEMENT_BLOCK_CLONE(MyColorWool)
  
  Blocks getType() override { return Blocks::MY_COLOR_WOOL; }
  float getHardness() override { return 0.8f; }  // All wool is 0.8f
  
  std::array<u8, 6> getFacesMap() override { 
    return {200, 200, 200, 200, 200, 200};  // Solid color texture
  }
  
  u8 isBreakable() override { return true; }
  u8 isCollidable() override { return true; }
  u8 hasTransparency() override { return false; }
  u8 isCrossed() override { return false; }
};
```

⚠️ **Sound**: Use `&SfxLibrary::grass` + `SoundFX::Grass1` (wool is soft like grass).

---

## Quick Checklist

After copying templates, verify:

- [ ] Enum value added to `Blocks` in [inc/constants.hpp](../../../../inc/constants.hpp)
- [ ] `TOTAL_OF_BLOCKS` incremented
- [ ] Block class created in [inc/entities/blocks/<Category>Blocks.hpp](../../../../inc/entities/blocks/)
- [ ] `IMPLEMENT_BLOCK_CLONE(ClassName)` macro used
- [ ] `StaticBlockRepository::initializeBlocks()` has `make_unique` entry
- [ ] `StaticBlockRepository::createBlock()` has `case` + `new` entry
- [ ] `MeshBuilder` constructor has **both** `builders[]` and `light_builders[]` entries
- [ ] All 3 sound repos updated (dig, broken, step)
- [ ] (If light-emitting) `BlockManager::getBlockLightValue()` updated
- [ ] (If oriented) `BlockManager::isBlockOriented()` updated
- [ ] (If slab) Inserted within slab range or updated `isSlab()` check
- [ ] (If vegetation) Inserted within vegetation range or updated `isVegetation()` check
- [ ] (If item) Added to `ItemId` enum + `ItemRepository` header/source

---

*Last updated: 2026-02-08 (TyraCraft v0.86.140-pre-alpha)*
