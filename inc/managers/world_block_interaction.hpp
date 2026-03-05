#pragma once

#include <tyra>
#include <tamtypes.h>
#include <vector>
#include <physics/ray.hpp>
#include <math/vec4.hpp>
#include "constants.hpp"
#include "entities/Block.hpp"
#include "entities/level.hpp"
#include "entities/edit_context.hpp"
#include "camera.hpp"
#include "models/world_light_model.hpp"
#include "managers/tick_scheduler.hpp"

using Tyra::Color;
using Tyra::Ray;
using Tyra::Renderer;
using Tyra::StaticPipeline;
using Tyra::Vec4;

class BlockManager;
class ChunkManager;
class ParticlesManager;
class WorldLightPropagation;
class WorldLiquidPropagation;
class Player;
struct Chunk;

/**
 * @brief Manages block targeting, placement, removal, breaking, and related
 *        sound effects.
 *
 * Extracted from World to isolate all player ↔ block interaction logic.
 */
class WorldBlockInteraction {
 public:
  WorldBlockInteraction();
  ~WorldBlockInteraction();

  void init(Level* level, Renderer* renderer, BlockManager* blockManager,
            ChunkManager* chunkManager, ParticlesManager* particlesManager,
            WorldLightPropagation* lightPropagation,
            WorldLiquidPropagation* liquidPropagation,
            WorldLightModel* worldLightModel);

  /** Register tick callbacks for async light propagation continuation. */
  void registerTickCallbacks(TickScheduler& scheduler);

  /** The block currently targeted by the player's crosshair (nullable). */
  Block* targetBlock = nullptr;

  // --- Targeting ---
  void updateTargetBlock(Camera* t_camera, Player* t_player);
  TargetedFace getTargetedFace();

  // --- Placement ---
  bool putBlock(const Blocks& blockType, Player* t_player);
  void placeBlockAt(const Blocks& blockType, const Vec4& blockOffset);
  void mergeSlabs(const Blocks& slabToPlace, Player* t_player,
                  const Vec4& offsetToMerge);

  // --- Removal ---
  void removeBlock(Block* blockToRemove);

  // --- Breaking ---
  void breakTargetBlock(const float& deltaTime);
  void breakTargetBlockInCreativeMode(const float& deltaTime);
  void stopBreakTargetBlock();
  inline u8 isBreakingBlock() const { return _isBreakingBlock; };

  // --- Validation ---
  inline u8 validTargetBlock() const {
    return targetBlock != nullptr && targetBlock->isBreakable();
  };

  // --- Rendering ---
  void renderBlockDamageOverlay();
  void buildTargetBlockDrawData();
  void updateBlockDamage();
  void clearTargetBlockDrawData();

  // --- Chunk neighbors ---
  void rebuildChunkNeighbors(Chunk* t_chunk, const TyraCraft::EditContext& ctx);

  // --- EditContext factory helpers ---
  TyraCraft::EditContext makeEditContextForPlacement(const Blocks& newBlockType,
                                                      const Vec4& blockPos,
                                                      const Blocks& oldBlockType);
  TyraCraft::EditContext makeEditContextForRemoval(const Blocks& oldBlockType,
                                                    const Vec4& blockPos);

  // --- Geometry impact detection (private helper) ---
  bool doesBlockChangeAffectNeighbor(const Blocks& oldType, const Blocks& newType);

  /** Static pipeline used for rendering — shared with World for chunk renders */
  StaticPipeline stapip;

 private:
  Level* pLevel = nullptr;
  Renderer* t_renderer = nullptr;
  BlockManager* pBlockManager = nullptr;
  ChunkManager* pChunkManager = nullptr;
  ParticlesManager* pParticlesManager = nullptr;
  WorldLightPropagation* pLightPropagation = nullptr;
  WorldLiquidPropagation* pLiquidPropagation = nullptr;
  WorldLightModel* pWorldLightModel = nullptr;

  Ray ray;

  // Breaking state
  u8 _isBreakingBlock = false;
  float breaking_time_pessed = 0.0F;
  float lastTimePlayedBreakingSfx = 0.0F;
  float lastTimeCreatedParticle = 0.0F;

  // Light propagation state (Phase 1: budget-bounded incremental propagation)
  bool pendingLightPropagation = false;
  TyraCraft::EditContext pendingEditContext;
  TickTaskHandle lightPropagationTickHandle;

  // Target block draw data
  std::vector<Vec4> _targetBlockVertices;
  std::vector<Vec4> _targetBlockUVMap;
  std::vector<Color> _targetBlockColors;

  // Block placement helpers
  bool putTorchBlock();
  bool putSlab(const Blocks& slabToPlace, Player* t_player);
  bool putDefaultBlock(const Blocks blockToPlace, Player* t_player);

  // Sound helpers
  void playPutBlockSound(const Blocks& blockType);
  void playDestroyBlockSound(const Blocks& blockType);
  void playBreakingBlockSound(const Blocks& blockType);

  // Light propagation continuation (Phase 1)
  void continueLightPropagation();
};
