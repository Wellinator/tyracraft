#include "managers/world_block_interaction.hpp"
#include "managers/block_manager.hpp"
#include "managers/chunk_manager.hpp"
#include "managers/particle/particle_manager.hpp"
#include "managers/world_light_propagation.hpp"
#include "managers/world_liquid_propagation.hpp"
#include "managers/light_manager.hpp"
#include "managers/sound_manager.hpp"
#include "managers/visible_faces_manager.hpp"
#include "managers/model_builder.hpp"
#include "managers/mesh/mesh_builder.hpp"
#include "managers/clipping_manager.hpp"
#include "managers/block/vertex_block_data.hpp"
#include "entities/player/player.hpp"
#include "entities/sfx_config.hpp"
#include "models/sfx_block_model.hpp"
#include "camera.hpp"
#include "debug.hpp"
#include "utils.hpp"
#include <tyra>
#include <cmath>

using Tyra::BBox;
using Tyra::M4x4;
using Tyra::Math;

WorldBlockInteraction::WorldBlockInteraction() {}

WorldBlockInteraction::~WorldBlockInteraction() { clearTargetBlockDrawData(); }

void WorldBlockInteraction::init(
    Level* level, Renderer* renderer, BlockManager* blockManager,
    ChunkManager* chunkManager, ParticlesManager* particlesManager,
    WorldLightPropagation* lightPropagation,
    WorldLiquidPropagation* liquidPropagation,
    WorldLightModel* worldLightModel) {
  pLevel = level;
  t_renderer = renderer;
  pBlockManager = blockManager;
  pChunkManager = chunkManager;
  pParticlesManager = particlesManager;
  pLightPropagation = lightPropagation;
  pLiquidPropagation = liquidPropagation;
  pWorldLightModel = worldLightModel;

  stapip.setRenderer(&t_renderer->core);
}

// ===============================================================
//  Target block
// ===============================================================

void WorldBlockInteraction::updateTargetBlock(Camera* t_camera,
                                              Player* t_player) {
#ifdef DEBUG_MODE
  if (g_debug_menu.enableRenderBlockDamage == false) return;
#endif

  u32 _lastTargetBlockId = 999999;

  if (targetBlock) {
    _lastTargetBlockId = targetBlock->index;
    delete targetBlock;
    targetBlock = nullptr;
  };

  const Vec4 origin =
      *t_player->getPosition() + Vec4(0.0f, t_camera->getCamY(), 0.0f);

  ray.origin = origin;
  ray.direction.set(t_camera->unitCirclePosition.getNormalized());

  // Broad phase raycast
  std::vector<LevelIntersectQueryResult> tempResult = {};
  pLevel->getIntersectedBlocks(ray.origin, ray.at(MAX_RANGE_PICKER),
                               &tempResult);

  // Narrow phase raycast
  for (const auto& result : tempResult) {
    Block* targetTemplate = pBlockManager->getBlockTemplateByType(
        static_cast<Blocks>(result.blockType));
    if (targetTemplate->isBreakable()) {
      BBox* rawBBox = VertexBlockData::getRawBBoxByOffset(
          const_cast<Vec4*>(&result.offset));
      M4x4 model = ModelBuilder_BuildModel(const_cast<Vec4*>(&result.offset));
      BBox blockBBox = rawBBox->getTransformed(model);

      Vec4 min, max;
      blockBBox.getMinMax(&min, &max);

      u8 visibleFaces =
          VisibleFacesManager::getInstance()->getVisibleFacesByOffset(
              result.offset);

      float distance = 0.0f;
      if (ray.intersectBox(min, max, &distance)) {
        targetBlock = targetTemplate->clone();
        targetBlock->index = pLevel->OffsetToIndex(result.offset);
        targetBlock->distance = distance;
        targetBlock->setHitPosition(ray.at(distance));
        targetBlock->setIsTarget(true);
        targetBlock->setVisibleFaces(visibleFaces);
        targetBlock->setVisibleFacesCount(Utils::countSetBits(visibleFaces));
        targetBlock->position = pLevel->offsetToWorldPos(&result.offset);
        targetBlock->bbox = new BBox(blockBBox);
        targetBlock->minCorner.set(min);
        targetBlock->maxCorner.set(max);
        targetBlock->offset = result.offset;
        targetBlock->baseColor = LightManager::GetLightColorAt(
            result.offset, getTargetedFace(),
            pWorldLightModel->sunLightIntensity);
        M4x4::copy(&targetBlock->model, model);

        if (targetBlock->index != _lastTargetBlockId) {
          breaking_time_pessed = 0;
          buildTargetBlockDrawData();
        }

        break;
      }
    }
  }
}

TargetedFace WorldBlockInteraction::getTargetedFace() {
  Vec4 targetPos = ray.at(targetBlock->distance);

  float frontFace = targetBlock->bbox->getFrontFace().axisPosition;
  if (std::round(targetPos.z) == frontFace) return TargetedFace::FrontFace;

  float backFace = targetBlock->bbox->getBackFace().axisPosition;
  if (std::round(targetPos.z) == backFace) return TargetedFace::BackFace;

  float leftFace = targetBlock->bbox->getLeftFace().axisPosition;
  if (std::round(targetPos.x) == leftFace) return TargetedFace::LeftFace;

  float rightFace = targetBlock->bbox->getRightFace().axisPosition;
  if (std::round(targetPos.x) == rightFace) return TargetedFace::RightFace;

  float topFace = targetBlock->bbox->getTopFace().axisPosition;
  if (std::round(targetPos.y) == topFace) return TargetedFace::TopFace;

  float bottomFace = targetBlock->bbox->getBottomFace().axisPosition;
  if (std::round(targetPos.y) == bottomFace) return TargetedFace::BottomFace;

  if (g_debug_mode) {
    TYRA_ERROR("Could not determine targeted face!");
    TYRA_ERROR("------ Target bbox faces -----");
    targetPos.print("Target Position: ");
    printf("Front Face: %f\n", frontFace);
    printf("Back Face: %f\n", backFace);
    printf("Left Face: %f\n", leftFace);
    printf("Right Face: %f\n", rightFace);
    printf("Top Face: %f\n", topFace);
    printf("Bottom Face: %f\n", bottomFace);
    TYRA_ERROR("------------------------------");
  }

  return TargetedFace::TopFace;  // Default
}

// ===============================================================
//  Block placement
// ===============================================================

void WorldBlockInteraction::placeBlockAt(const Blocks& blockType,
                                         const Vec4& blockOffset) {
  const Blocks blockTypeAtOffsetPosition = static_cast<Blocks>(
      pLevel->GetBlockFromMap(blockOffset.x, blockOffset.y, blockOffset.z));

  BlockOrientation orientation = BlockOrientation::East;

  if (pBlockManager->isBlockOriented(blockType)) {
    const float cameraYaw = Camera::getInstance()->yaw;

    if (cameraYaw > 315 || cameraYaw < 45)
      orientation = BlockOrientation::North;
    else if (cameraYaw >= 135 && cameraYaw <= 225)
      orientation = BlockOrientation::South;
    else if (cameraYaw >= 45 && cameraYaw <= 135)
      orientation = BlockOrientation::East;
    else
      orientation = BlockOrientation::West;
  }
  pLevel->SetBlockOrientationDataToMap(blockOffset.x, blockOffset.y,
                                       blockOffset.z, orientation);

  pLevel->SetBlockInMap(blockOffset.x, blockOffset.y, blockOffset.z,
                        static_cast<u8>(blockType));
  pLightPropagation->checkSunLightAt(blockOffset.x, blockOffset.y,
                                     blockOffset.z);

  const auto lightValue = pBlockManager->getBlockLightValue(blockType);
  if (lightValue > 0) {
    pLightPropagation->addBlockLight(blockOffset.x, blockOffset.y,
                                     blockOffset.z, lightValue);
  } else {
    pLightPropagation->removeLight(blockOffset.x, blockOffset.y, blockOffset.z);
  }

  pLightPropagation->updateSunlight();
  pLightPropagation->updateBlockLights();

  // Async light reload: only affects chunks within 8-chunk radius
  // Immediate update on player chunk for instant feedback (~1-2ms)
  // Rest propagate smoothly over 1-2 seconds (4 chunks/tick)
  pChunkManager->enqueueAffectedChunksForLightReload(blockOffset, 8.0f, true);

  const Blocks oldTypeBlock = blockTypeAtOffsetPosition;
  const u8 isPlacingLiquid =
      blockType == Blocks::WATER_BLOCK || blockType == Blocks::LAVA_BLOCK;
  const u8 itWasLiquidAtPosition =
      oldTypeBlock == Blocks::WATER_BLOCK || oldTypeBlock == Blocks::LAVA_BLOCK;

  if (isPlacingLiquid) {
    pLiquidPropagation->addLiquid(blockOffset.x, blockOffset.y, blockOffset.z,
                                  (u8)blockType, (u8)LiquidLevel::Percent100,
                                  (u8)orientation);
  } else if (itWasLiquidAtPosition) {
    pLiquidPropagation->removeLiquid(blockOffset.x, blockOffset.y,
                                     blockOffset.z, (u8)oldTypeBlock);
  }

  updateNeighBorsChunksByAddedBlock(const_cast<Vec4*>(&blockOffset));
}

void WorldBlockInteraction::removeBlock(Block* blockToRemove) {
  // Generate particles right before block gets destroyed
  pParticlesManager->createBlockParticleBatch(blockToRemove, 48);

  Vec4 offsetToRemove = blockToRemove->offset;
  pLevel->SetBlockInMapByIndex(blockToRemove->index, (u8)Blocks::AIR_BLOCK);
  pLevel->SetLiquidDataToMap(offsetToRemove.x, offsetToRemove.y,
                             offsetToRemove.z, (u8)LiquidLevel::Percent0);

  // Update sunlight and block light at position
  pLightPropagation->removeLight(offsetToRemove.x, offsetToRemove.y,
                                 offsetToRemove.z);
  pLightPropagation->checkSunLightAt(offsetToRemove.x, offsetToRemove.y,
                                     offsetToRemove.z);
  pLightPropagation->updateSunlight();
  pLightPropagation->updateBlockLights();
  
  // Async light reload: spatially filtered to affected area
  pChunkManager->enqueueAffectedChunksForLightReload(offsetToRemove, 8.0f, true);

  // Update liquid at position
  pLiquidPropagation->checkLiquidPropagation(offsetToRemove.x,
                                             offsetToRemove.y,
                                             offsetToRemove.z);

  playDestroyBlockSound(blockToRemove->getType());

  Chunk* chunkToRebuild = pChunkManager->getChunkByBlockOffset(offsetToRemove);
  rebuildChunkNeighbors(chunkToRebuild, const_cast<Vec4*>(&offsetToRemove));

  // Remove up block if it's vegetation
  const Vec4 upBlockOffset =
      Vec4(offsetToRemove.x, offsetToRemove.y + 1, offsetToRemove.z);

  if (pLevel->BoundCheckMap(upBlockOffset.x, upBlockOffset.y,
                            upBlockOffset.z)) {
    const Blocks upperBlockType = static_cast<Blocks>(pLevel->GetBlockFromMap(
        upBlockOffset.x, upBlockOffset.y, upBlockOffset.z));

    if (pBlockManager->isVegetation(upperBlockType) ||
        upperBlockType == Blocks::TORCH) {
      Block* _template = pBlockManager->getBlockTemplateByType(upperBlockType);
      Block* upperBlock = _template->clone();

      u8 visibleFaces =
          VisibleFacesManager::getInstance()->getVisibleFacesByOffset(
              upBlockOffset);

      upperBlock->offset = upBlockOffset;
      upperBlock->index = pLevel->OffsetToIndex(upBlockOffset);
      upperBlock->setVisibleFaces(visibleFaces);
      upperBlock->setVisibleFacesCount(Utils::countSetBits(visibleFaces));
      upperBlock->position = pLevel->offsetToWorldPos(&upperBlock->offset);
      upperBlock->baseColor = LightManager::GetLightColorAt(
          upperBlock->offset, getTargetedFace(),
          pWorldLightModel->sunLightIntensity);

      removeBlock(upperBlock);
      delete upperBlock;
    }
  }
}

bool WorldBlockInteraction::putBlock(const Blocks& blockToPlace,
                                     Player* t_player) {
  Vec4 blockOffset = targetBlock->offset;

  if (!pLevel->BoundCheckMap(blockOffset.x, blockOffset.y, blockOffset.z))
    return false;

  t_player->playPutBlockAnimation();
  bool blockPlaced = false;

  switch (blockToPlace) {
    case Blocks::TORCH:
      blockPlaced = putTorchBlock();
      break;

    case Blocks::STONE_SLAB:
    case Blocks::BRICKS_SLAB:
    case Blocks::OAK_PLANKS_SLAB:
    case Blocks::SPRUCE_PLANKS_SLAB:
    case Blocks::BIRCH_PLANKS_SLAB:
    case Blocks::ACACIA_PLANKS_SLAB:
    case Blocks::STONE_BRICK_SLAB:
    case Blocks::CRACKED_STONE_BRICKS_SLAB:
    case Blocks::MOSSY_STONE_BRICKS_SLAB:
      blockPlaced = putSlab(blockToPlace, t_player);
      break;

    default:
      blockPlaced = putDefaultBlock(blockToPlace, t_player);
      break;
  }

  if (blockPlaced) playPutBlockSound(blockToPlace);
  return blockPlaced;
}

bool WorldBlockInteraction::putTorchBlock() {
  TargetedFace placementDirection = getTargetedFace();
  Vec4 blockOffset = targetBlock->offset;

  if (placementDirection == TargetedFace::FrontFace)
    blockOffset.z++;
  else if (placementDirection == TargetedFace::BackFace)
    blockOffset.z--;
  else if (placementDirection == TargetedFace::RightFace)
    blockOffset.x++;
  else if (placementDirection == TargetedFace::LeftFace)
    blockOffset.x--;
  else if (placementDirection == TargetedFace::TopFace)
    blockOffset.y++;
  else if (placementDirection == TargetedFace::BottomFace)
    blockOffset.y--;

  const bool canReplace =
      pLevel->isPositionEmpty(blockOffset.x, blockOffset.y, blockOffset.z) ||
      pLevel->isGrassAtPosition(blockOffset.x, blockOffset.y, blockOffset.z);

  if (targetBlock->getType() == Blocks::TORCH &&
      placementDirection == TargetedFace::TopFace)
    return false;

  if (canReplace) {
    BlockOrientation orientation;

    if (targetBlock->getType() == Blocks::TORCH) {
      orientation = BlockOrientation::Top;
    } else {
      switch (placementDirection) {
        case TargetedFace::TopFace:
          orientation = BlockOrientation::Top;
          break;
        case TargetedFace::LeftFace:
          orientation = BlockOrientation::East;
          break;
        case TargetedFace::RightFace:
          orientation = BlockOrientation::West;
          break;
        case TargetedFace::FrontFace:
          orientation = BlockOrientation::North;
          break;
        case TargetedFace::BackFace:
          orientation = BlockOrientation::South;
          break;
        case TargetedFace::BottomFace:
        default:
          return false;
      }
    }

    pLevel->SetBlockInMap(blockOffset.x, blockOffset.y, blockOffset.z,
                          static_cast<u8>(Blocks::TORCH));
    pLevel->SetTorchOrientationDataToMap(blockOffset.x, blockOffset.y,
                                         blockOffset.z, orientation);
    pLightPropagation->checkSunLightAt(blockOffset.x, blockOffset.y,
                                       blockOffset.z);

    const auto lightValue = pBlockManager->getBlockLightValue(Blocks::TORCH);
    pLightPropagation->addBlockLight(blockOffset.x, blockOffset.y,
                                     blockOffset.z, lightValue);

    pLightPropagation->updateSunlight();
    pLightPropagation->updateBlockLights();

    // Async light reload for torch placement
    pChunkManager->enqueueAffectedChunksForLightReload(blockOffset, 8.0f, true);
    updateNeighBorsChunksByAddedBlock(&blockOffset);

    return true;
  }

  return false;
}

bool WorldBlockInteraction::putSlab(const Blocks& slabToPlace,
                                    Player* t_player) {
  TargetedFace targeted_face = getTargetedFace();
  Vec4 targetPos = ray.at(targetBlock->distance);
  Vec4 newSlabPos = targetBlock->offset;
  SlabOrientation slabOrientation = SlabOrientation::Top;

  // When the target block IS a slab
  if (pBlockManager->isSlab(targetBlock->getType())) {
    if (targetBlock->getType() == slabToPlace) {
      const auto existing_slab_orientation =
          pLevel->GetSlabOrientationDataFromMap(newSlabPos.x, newSlabPos.y,
                                                newSlabPos.z);
      if ((targeted_face == TargetedFace::TopFace &&
           existing_slab_orientation == SlabOrientation::Bottom) ||
          (targeted_face == TargetedFace::BottomFace &&
           existing_slab_orientation == SlabOrientation::Top)) {
        mergeSlabs(slabToPlace, t_player, newSlabPos);
        return true;
      } else if (targeted_face == TargetedFace::FrontFace ||
                 targeted_face == TargetedFace::BackFace ||
                 targeted_face == TargetedFace::RightFace ||
                 targeted_face == TargetedFace::LeftFace) {
        if (targeted_face == TargetedFace::FrontFace)
          newSlabPos.z++;
        else if (targeted_face == TargetedFace::BackFace)
          newSlabPos.z--;
        else if (targeted_face == TargetedFace::RightFace)
          newSlabPos.x++;
        else if (targeted_face == TargetedFace::LeftFace)
          newSlabPos.x--;

        const float heightOffset =
            std::fmod(targetPos.y - BLOCK_SIZE, DOUBLE_BLOCK_SIZE);
        slabOrientation = heightOffset > BLOCK_SIZE ? SlabOrientation::Top
                                                    : SlabOrientation::Bottom;

        goto placeSlabOnEmptyOrCombine;
      }
    } else {
      if (targeted_face == TargetedFace::TopFace) {
        newSlabPos.y++;
        slabOrientation = SlabOrientation::Bottom;
      } else if (targeted_face == TargetedFace::BottomFace) {
        newSlabPos.y--;
        slabOrientation = SlabOrientation::Top;
      } else if (targeted_face == TargetedFace::FrontFace) {
        newSlabPos.z++;
      } else if (targeted_face == TargetedFace::BackFace) {
        newSlabPos.z--;
      } else if (targeted_face == TargetedFace::RightFace) {
        newSlabPos.x++;
      } else if (targeted_face == TargetedFace::LeftFace) {
        newSlabPos.x--;
      }

      const float heightOffset =
          std::fmod(targetPos.y - BLOCK_SIZE, DOUBLE_BLOCK_SIZE);
      slabOrientation = heightOffset > BLOCK_SIZE ? SlabOrientation::Top
                                                  : SlabOrientation::Bottom;
      goto placeSlabOnEmptyOrCombine;
    }

    return false;
  }

  // When the target block IS NOT a slab
  if (targeted_face == TargetedFace::TopFace) {
    newSlabPos.y++;
    slabOrientation = SlabOrientation::Bottom;
  } else if (targeted_face == TargetedFace::BottomFace) {
    newSlabPos.y--;
    slabOrientation = SlabOrientation::Top;
  } else if (targeted_face == TargetedFace::FrontFace ||
             targeted_face == TargetedFace::BackFace ||
             targeted_face == TargetedFace::RightFace ||
             targeted_face == TargetedFace::LeftFace) {
    if (targeted_face == TargetedFace::FrontFace)
      newSlabPos.z++;
    else if (targeted_face == TargetedFace::BackFace)
      newSlabPos.z--;
    else if (targeted_face == TargetedFace::RightFace)
      newSlabPos.x++;
    else if (targeted_face == TargetedFace::LeftFace)
      newSlabPos.x--;

    const float heightOffset =
        std::fmod(targetPos.y - BLOCK_SIZE, DOUBLE_BLOCK_SIZE);
    slabOrientation = heightOffset > BLOCK_SIZE ? SlabOrientation::Top
                                                : SlabOrientation::Bottom;
  }

placeSlabOnEmptyOrCombine:

  // Prevent putting a block at the player position
  Vec4 newBlockPos = pLevel->offsetToWorldPos(newSlabPos);

  M4x4 tempModel = M4x4();
  tempModel.identity();
  tempModel.scaleX(BLOCK_SIZE);
  tempModel.scaleZ(BLOCK_SIZE);
  tempModel.scaleY(HALF_BLOCK_SIZE);
  tempModel.translate(newBlockPos);

  BBox* rawBBox = VertexBlockData::getRawBBoxByOffset(&newSlabPos);
  BBox tempBBox = rawBBox->getTransformed(tempModel);
  BBox newBlockBBox = BBox(tempBBox.vertices, tempBBox.getVertexCount());
  BBox playerBBox = t_player->getHitBox();

  if (Utils::AABBCollides(&newBlockBBox, &playerBBox)) return false;

  if (pLevel->isReplaceableBySolidBlock(newSlabPos.x, newSlabPos.y,
                                        newSlabPos.z)) {
    pLevel->SetSlabOrientationDataToMap(newSlabPos.x, newSlabPos.y,
                                        newSlabPos.z, slabOrientation);
    placeBlockAt(slabToPlace, newSlabPos);
    return true;
  } else {
    const Blocks existing_block = static_cast<Blocks>(
        pLevel->GetBlockFromMap(newSlabPos.x, newSlabPos.y, newSlabPos.z));
    if (existing_block == slabToPlace) {
      const auto existing_slab_orientation =
          pLevel->GetSlabOrientationDataFromMap(newSlabPos.x, newSlabPos.y,
                                                newSlabPos.z);

      if ((slabOrientation == SlabOrientation::Top &&
           existing_slab_orientation == SlabOrientation::Bottom)) {
        mergeSlabs(slabToPlace, t_player, newSlabPos);
        return true;
      } else if ((slabOrientation == SlabOrientation::Bottom &&
                  existing_slab_orientation == SlabOrientation::Top)) {
        mergeSlabs(slabToPlace, t_player, newSlabPos);
        return true;
      }
      return false;
    }
  }

  return false;
}

void WorldBlockInteraction::mergeSlabs(const Blocks& slabToPlace,
                                       Player* t_player,
                                       const Vec4& offsetToMerge) {
  Blocks newBlock;

  switch (slabToPlace) {
    case Blocks::STONE_SLAB:
      newBlock = Blocks::STONE_BLOCK;
      break;
    case Blocks::BRICKS_SLAB:
      newBlock = Blocks::BRICKS_BLOCK;
      break;
    case Blocks::OAK_PLANKS_SLAB:
      newBlock = Blocks::OAK_PLANKS_BLOCK;
      break;
    case Blocks::SPRUCE_PLANKS_SLAB:
      newBlock = Blocks::SPRUCE_PLANKS_BLOCK;
      break;
    case Blocks::BIRCH_PLANKS_SLAB:
      newBlock = Blocks::BIRCH_PLANKS_BLOCK;
      break;
    case Blocks::ACACIA_PLANKS_SLAB:
      newBlock = Blocks::ACACIA_PLANKS_BLOCK;
      break;
    case Blocks::STONE_BRICK_SLAB:
      newBlock = Blocks::STONE_BRICK_BLOCK;
      break;
    case Blocks::CRACKED_STONE_BRICKS_SLAB:
      newBlock = Blocks::CRACKED_STONE_BRICKS_BLOCK;
      break;
    case Blocks::MOSSY_STONE_BRICKS_SLAB:
      newBlock = Blocks::MOSSY_STONE_BRICKS_BLOCK;
      break;
    default:
      newBlock = Blocks::AIR_BLOCK;
      TYRA_ERROR("Not valid double slab!");
      break;
  }

  placeBlockAt(newBlock, offsetToMerge);
  buildTargetBlockDrawData();
}

bool WorldBlockInteraction::putDefaultBlock(const Blocks blockToPlace,
                                            Player* t_player) {
  TargetedFace placementDirection = getTargetedFace();
  Vec4 blockOffset = targetBlock->offset;

  if (placementDirection == TargetedFace::FrontFace)
    blockOffset.z++;
  else if (placementDirection == TargetedFace::BackFace)
    blockOffset.z--;
  else if (placementDirection == TargetedFace::RightFace)
    blockOffset.x++;
  else if (placementDirection == TargetedFace::LeftFace)
    blockOffset.x--;
  else if (placementDirection == TargetedFace::TopFace)
    blockOffset.y++;
  else if (placementDirection == TargetedFace::BottomFace)
    blockOffset.y--;

  Vec4 newBlockPos = blockOffset * DOUBLE_BLOCK_SIZE;

  M4x4 tempModel = M4x4();
  tempModel.identity();
  tempModel.scale(BLOCK_SIZE);
  tempModel.translate(newBlockPos);

  BBox* rawBBox = VertexBlockData::getRawBBoxByOffset(&blockOffset);
  BBox tempBBox = rawBBox->getTransformed(tempModel);
  BBox finalBBox = BBox(tempBBox.vertices, tempBBox.getVertexCount());

  Vec4 newBlockPosMin;
  Vec4 newBlockPosMax;
  finalBBox.getMinMax(&newBlockPosMin, &newBlockPosMax);

  Vec4 minPlayerCorner;
  Vec4 maxPlayerCorner;
  t_player->getHitBox().getMinMax(&minPlayerCorner, &maxPlayerCorner);

  // Collision check
  if (newBlockPosMax.x > minPlayerCorner.x &&
      newBlockPosMin.x < maxPlayerCorner.x &&
      newBlockPosMax.z > minPlayerCorner.z &&
      newBlockPosMin.z < maxPlayerCorner.z &&
      newBlockPosMax.y > minPlayerCorner.y &&
      newBlockPosMin.y < maxPlayerCorner.y) {
    return false;
  }

  const u8 canReplace = pLevel->isReplaceableBySolidBlock(
      blockOffset.x, blockOffset.y, blockOffset.z);

  if (canReplace) {
    placeBlockAt(blockToPlace, blockOffset);
    return true;
  }

  return false;
}

// ===============================================================
//  Breaking
// ===============================================================

void WorldBlockInteraction::stopBreakTargetBlock() {
  _isBreakingBlock = false;
  breaking_time_pessed = 0;
  if (targetBlock) {
    targetBlock->damage = 0;
    buildTargetBlockDrawData();
  }
}

void WorldBlockInteraction::breakTargetBlock(const float& deltaTime) {
  if (_isBreakingBlock) {
    breaking_time_pessed += deltaTime;
    const auto breakingTime = pBlockManager->getBlockBreakingTime(targetBlock);
    if (breaking_time_pessed >= breakingTime) {
      removeBlock(targetBlock);
      delete targetBlock;
      targetBlock = nullptr;
      breaking_time_pessed = 0;
    } else {
      targetBlock->damage = breaking_time_pessed / breakingTime * 100;

      if (lastTimeCreatedParticle > 0.2) {
        pParticlesManager->createBlockParticleBatch(targetBlock, 4);
        lastTimeCreatedParticle = 0;
      } else {
        lastTimeCreatedParticle += deltaTime;
      }

      if (lastTimePlayedBreakingSfx > 0.3F) {
        playBreakingBlockSound(targetBlock->getType());
        lastTimePlayedBreakingSfx = 0;
      } else {
        lastTimePlayedBreakingSfx += deltaTime;
      }
    }

    return;
  }

  breaking_time_pessed = 0;
  _isBreakingBlock = true;
}

void WorldBlockInteraction::breakTargetBlockInCreativeMode(
    const float& deltaTime) {
  if (_isBreakingBlock) {
    breaking_time_pessed += deltaTime;
    const auto breakingTime = BREAKING_TIME_IN_CREATIVE_MODE;
    if (breaking_time_pessed >= breakingTime) {
      removeBlock(targetBlock);
      delete targetBlock;
      targetBlock = nullptr;
      breaking_time_pessed = 0;
    } else {
      targetBlock->damage = breaking_time_pessed / breakingTime * 100;

      if (lastTimeCreatedParticle > 0.2) {
        pParticlesManager->createBlockParticleBatch(targetBlock, 3);
        lastTimeCreatedParticle = 0;
      } else {
        lastTimeCreatedParticle += deltaTime;
      }

      if (lastTimePlayedBreakingSfx > 0.3F) {
        playBreakingBlockSound(targetBlock->getType());
        lastTimePlayedBreakingSfx = 0;
      } else {
        lastTimePlayedBreakingSfx += deltaTime;
      }
    }

    return;
  }

  breaking_time_pessed = 0;
  _isBreakingBlock = true;
}

// ===============================================================
//  Rendering
// ===============================================================

void WorldBlockInteraction::renderBlockDamageOverlay() {
#ifdef DEBUG_MODE
  if (g_debug_menu.enableRenderBlockDamage == false) return;
#endif

  if (!targetBlock) return;

  t_renderer->renderer3D.usePipeline(stapip);

  StaPipTextureBag textureBag;
  StaPipInfoBag infoBag;
  StaPipColorBag colorBag;
  StaPipBag bag;
  M4x4 model = M4x4::Identity;

  infoBag.model = &model;
  infoBag.textureMappingType = Tyra::PipelineTextureMappingType::TyraNearest;
  infoBag.shadingType = Tyra::PipelineShadingType::TyraShadingGouraud;
  infoBag.blendingEnabled = true;
  infoBag.antiAliasingEnabled = false;
  infoBag.fullClipChecks = false;
  infoBag.frustumCulling =
      Tyra::PipelineInfoBagFrustumCulling::PipelineInfoBagFrustumCulling_None;

  colorBag.many = _targetBlockColors.data();
  bag.color = &colorBag;

  textureBag.coordinates = _targetBlockUVMap.data();
  textureBag.texture = pBlockManager->getBlocksTexture();
  bag.texture = &textureBag;

  bag.count = _targetBlockVertices.size();
  bag.vertices = _targetBlockVertices.data();
  bag.info = &infoBag;

  ClippingManager_ClipAndRenderBag(&bag, &stapip, t_renderer,
                                   Camera::getInstance()->looksAt);

#ifdef DEBUG_MODE
  if (g_debug_menu.showTargetedBlockBoundingBox) {
    BBox* rawBBox = VertexBlockData::getRawBBoxByOffset(&targetBlock->offset);
    M4x4 model = ModelBuilder_BuildModel(&targetBlock->offset);
    BBox blockBBox = rawBBox->getTransformed(model);
    t_renderer->renderer3D.utility.drawBBox(blockBBox, Color(100, 100, 50));
  }
#endif
}

void WorldBlockInteraction::buildTargetBlockDrawData() {
  TYRA_ASSERT(targetBlock != nullptr, "No target block to build draw data!");

  clearTargetBlockDrawData();

  const u8 size = Utils::countSetBits(targetBlock->getVisibleFaces()) *
                  VertexBlockData::FACES_COUNT;
  if (_targetBlockVertices.capacity() < size) {
    _targetBlockVertices.reserve(size);
    _targetBlockColors.reserve(size);
    _targetBlockUVMap.reserve(size);
  }

  CustomMeshOptions options = {.scale = 1.01f};
  MeshBuilder_BuildMesh(&targetBlock->offset, targetBlock->getVisibleFaces(), 0,
                        &_targetBlockVertices, &_targetBlockColors,
                        &_targetBlockUVMap, pWorldLightModel, pLevel, &options);

  for (size_t i = 0; i < size; i++) {
    LightManager::IntensifyColor(&_targetBlockColors[i], 1.35f);
  }
}

void WorldBlockInteraction::updateBlockDamage() {
  _targetBlockUVMap.clear();

  const u8 V = 15;
  const u8 U = floor(targetBlock->damage / 10);
  const float _scale = 1.0F / 16.0F;
  const Vec4 UVScale = Vec4(_scale, _scale, 1.0F, 0.0F);
  const u8 size =
      targetBlock->packed.visibleFacesCount * VertexBlockData::FACES_COUNT;

  u8 idx = 0;
  const u8 faces = size / 6;

  for (size_t i = 0; i < faces; i++) {
    _targetBlockUVMap[idx++] = (Vec4(U, (V + 1.0F), 1.0F, 0.0F) * UVScale);
    _targetBlockUVMap[idx++] = (Vec4((U + 1.0F), V, 1.0F, 0.0F) * UVScale);
    _targetBlockUVMap[idx++] =
        (Vec4((U + 1.0F), (V + 1.0F), 1.0F, 0.0F) * UVScale);

    _targetBlockUVMap[idx++] = (Vec4(U, (V + 1.0F), 1.0F, 0.0F) * UVScale);
    _targetBlockUVMap[idx++] = (Vec4(U, V, 1.0F, 0.0F) * UVScale);
    _targetBlockUVMap[idx++] = (Vec4((U + 1.0F), V, 1.0F, 0.0F) * UVScale);
  }

  for (size_t i = 0; i < size; i++) {
    _targetBlockColors[i].a = 70.0f;
  };
}

void WorldBlockInteraction::clearTargetBlockDrawData() {
  _targetBlockVertices.clear();
  _targetBlockColors.clear();
  _targetBlockUVMap.clear();
}

// ===============================================================
//  Chunk neighbor rebuilding
// ===============================================================

void WorldBlockInteraction::rebuildChunkNeighbors(Chunk* t_chunk,
                                                  Vec4* moddedOffset) {
  if (g_debug_mode) t_chunk->buildingTimeStart = clock();

  Vec4 bottom = *moddedOffset + DOWN_VEC;
  if (!t_chunk->containsBlock(&bottom)) {
    Chunk* bottomChunk = pChunkManager->getChunkByBlockOffset(bottom);
    if (bottomChunk && bottomChunk->isLoaded()) bottomChunk->rebuild();
  }

  Vec4 top = *moddedOffset + UP_VEC;
  if (!t_chunk->containsBlock(&top)) {
    Chunk* topChunk = pChunkManager->getChunkByBlockOffset(top);
    if (topChunk && topChunk->isLoaded()) topChunk->rebuild();
  }

  Vec4 right = *moddedOffset + RIGHT_VEC;
  if (t_chunk->containsBlock(&right)) {
    Chunk* rightChunk = pChunkManager->getChunkByBlockOffset(right);
    if (rightChunk && rightChunk->isLoaded()) rightChunk->rebuild();
  }

  Vec4 left = *moddedOffset + LEFT_VEC;
  if (t_chunk->containsBlock(&left)) {
    Chunk* leftChunk = pChunkManager->getChunkByBlockOffset(left);
    if (leftChunk && leftChunk->isLoaded()) leftChunk->rebuild();
  }

  Vec4 front = *moddedOffset + FRONT_VEC;
  if (t_chunk->containsBlock(&front)) {
    Chunk* frontChunk = pChunkManager->getChunkByBlockOffset(front);
    if (frontChunk && frontChunk->isLoaded()) frontChunk->rebuild();
  }

  Vec4 back = *moddedOffset + BACK_VEC;
  if (t_chunk->containsBlock(&back)) {
    Chunk* backChunk = pChunkManager->getChunkByBlockOffset(back);
    if (backChunk && backChunk->isLoaded()) backChunk->rebuild();
  }

  t_chunk->rebuild();

  if (g_debug_mode) {
    t_chunk->timeToBuild =
        ((float)(clock() - t_chunk->buildingTimeStart)) / CLOCKS_PER_SEC;
    printf("Time to sync build chunk fragment %i: %f\n", t_chunk->id,
           t_chunk->timeToBuild);
  }
}

void WorldBlockInteraction::updateNeighBorsChunksByAddedBlock(Vec4* offset) {
  Chunk* currentChunk = pChunkManager->getChunkByBlockOffset(*offset);
  rebuildChunkNeighbors(currentChunk, offset);
}

// ===============================================================
//  Sound helpers
// ===============================================================

void WorldBlockInteraction::playPutBlockSound(const Blocks& blockType) {
  if (blockType != Blocks::AIR_BLOCK) {
    SfxBlockModel* blockSfxModel =
        pBlockManager->getDigSoundByBlockType(blockType);
    if (blockSfxModel) {
      SoundManager* pSoundManager = SoundManager::getInstance();
      const int ch = pSoundManager->getAvailableChannel();
      SfxLibrarySound* sound = pSoundManager->getSound(blockSfxModel);
      auto config = SfxConfig::getPlaceSoundConfig(blockType);
      sound->_sound->pitch = config->_pitch;
      pSoundManager->setSfxVolume(config->_volume, ch);
      pSoundManager->playSfx(sound, ch);
      delete config;
    }
  }
}

void WorldBlockInteraction::playDestroyBlockSound(const Blocks& blockType) {
  if (blockType != Blocks::AIR_BLOCK) {
    SfxBlockModel* blockSfxModel =
        pBlockManager->getBrokenSoundByBlockType(blockType);
    if (blockSfxModel) {
      SoundManager* pSoundManager = SoundManager::getInstance();
      const int ch = pSoundManager->getAvailableChannel();
      SfxLibrarySound* sound = pSoundManager->getSound(blockSfxModel);
      auto config = SfxConfig::getBrokenSoundConfig(blockType);
      sound->_sound->pitch = config->_pitch;
      pSoundManager->setSfxVolume(config->_volume, ch);
      pSoundManager->playSfx(sound, ch);
      delete config;
    }
  }
}

void WorldBlockInteraction::playBreakingBlockSound(const Blocks& blockType) {
  if (blockType != Blocks::AIR_BLOCK) {
    SfxBlockModel* blockSfxModel =
        pBlockManager->getDigSoundByBlockType(blockType);
    if (blockSfxModel) {
      SoundManager* pSoundManager = SoundManager::getInstance();
      const int ch = pSoundManager->getAvailableChannel();
      SfxLibrarySound* sound = pSoundManager->getSound(blockSfxModel);
      auto config = SfxConfig::getBreakingSoundConfig(blockType);
      sound->_sound->pitch = config->_pitch;
      pSoundManager->setSfxVolume(config->_volume, ch);
      pSoundManager->playSfx(sound, ch);
      delete config;
    }
  }
}
