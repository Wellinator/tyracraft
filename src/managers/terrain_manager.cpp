#include "managers/terrain_manager.hpp"

using Tyra::BBox;
using Tyra::Color;
using Tyra::Pad;
using Tyra::PadButtons;
using Tyra::Ray;
using Tyra::Renderer;
using Tyra::Renderer3D;
using Tyra::Vec4;

TerrainManager::TerrainManager() {
  printf("\n\n|-----------SEED---------|");
  printf("\n|           %d         |\n", seed);
  printf("|------------------------|\n\n");

  this->minWorldPos.set(OVERWORLD_MIN_DISTANCE, OVERWORLD_MIN_HEIGH,
                        OVERWORLD_MIN_DISTANCE);
  this->maxWorldPos.set(OVERWORLD_MAX_DISTANCE, OVERWORLD_MAX_HEIGH,
                        OVERWORLD_MAX_DISTANCE);
}

TerrainManager::~TerrainManager() {
  delete this->noise;
  delete this->terrain;
}

void TerrainManager::init(Renderer* t_renderer, ItemRepository* itemRepository,
                          MinecraftPipeline* mcPip, BlockManager* blockManager,
                          SoundManager* t_soundManager) {
  this->t_renderer = t_renderer;
  this->t_itemRepository = itemRepository;
  this->t_mcPip = mcPip;
  this->t_blockManager = blockManager;
  this->t_soundManager = t_soundManager;

  this->calcRawBlockBBox(mcPip);
  this->initNoise();
}

void TerrainManager::update(Player* t_player, Camera* t_camera, Pad* t_pad,
                            std::vector<Chunck*> chuncks,
                            const float& deltaTime) {
  this->t_player = t_player;
  this->t_camera = t_camera;
  this->updateTargetBlock(*t_player->getPosition(), t_camera, chuncks);
  this->handlePadControls(t_pad, deltaTime);
};

void TerrainManager::generateNewTerrain(const NewGameOptions& options) {
  // TODO: refactor to terrain generation pipeline by terrain type
  this->generateTerrainBase(options.makeFlat);
  if (options.enableCaves) this->generateCaves();
  if (options.enableWater) this->generateWater();
  if (options.enableTrees) this->generateTrees();

  // TODO: refactor -> use local noise variable in each function;
}

void TerrainManager::generateTerrainBase(const bool& makeFlat) {
  this->noise->SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
  this->noise->SetFractalType(FastNoiseLite::FractalType_DomainWarpProgressive);

  u32 index = 0;
  u32 noise = 0;

  for (int z = OVERWORLD_MIN_DISTANCE; z < OVERWORLD_MAX_DISTANCE; z++) {
    for (int x = OVERWORLD_MIN_DISTANCE; x < OVERWORLD_MAX_DISTANCE; x++) {
      noise = getNoise(x, z);
      for (int y = OVERWORLD_MIN_HEIGH; y < OVERWORLD_MAX_HEIGH; y++) {
        if (y == OVERWORLD_MIN_HEIGH)
          this->terrain[index] = (u8)Blocks::BEDROCK_BLOCK;
        else if (makeFlat) {
          this->terrain[index] =
              y <= 0 ? (u8)Blocks::GRASS_BLOCK : (u8)Blocks::AIR_BLOCK;
        } else {
          this->terrain[index] = this->getBlock(noise, y);
        }
        index++;
      }
    }
  }
}

void TerrainManager::generateCaves() {
  // TODO: refactor to terrain generation pipeline by terrain type
  this->noise->SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2S);
  this->noise->SetFractalType(FastNoiseLite::FractalType_None);
  this->noise->SetFrequency(0.04F);
  this->noise->SetFractalLacunarity(1.0F);
  this->noise->SetFractalOctaves(24);

  u32 index = 0;
  for (int z = OVERWORLD_MIN_DISTANCE; z < OVERWORLD_MAX_DISTANCE; z++) {
    for (int x = OVERWORLD_MIN_DISTANCE; x < OVERWORLD_MAX_DISTANCE; x++) {
      for (int y = OVERWORLD_MIN_HEIGH; y < OVERWORLD_MAX_HEIGH; y++) {
        if (y == OVERWORLD_MIN_HEIGH) {
          index++;
          continue;
        }

        const float density = getDensity(x, y, z);
        if (density <= -0.2F) this->terrain[index] = (u8)Blocks::AIR_BLOCK;

        index++;
      }
    }
  }
}

void TerrainManager::initNoise() {
  // Fast Noise Lite
  this->noise = new FastNoiseLite();
  this->noise->SetSeed(this->seed);
  this->noise->SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
  this->noise->SetFractalType(FastNoiseLite::FractalType_DomainWarpProgressive);
  this->noise->SetFractalOctaves(this->octaves);
  this->noise->SetFractalLacunarity(this->lacunarity);
  this->noise->SetFrequency(this->frequency);
}

int TerrainManager::getNoise(int x, int z) {
  float xNoiseOffset = (float)((x + seed));
  float zNoiseOffset = (float)((z + seed));
  this->noise->SetFrequency(this->frequency);
  this->noise->SetFractalOctaves(this->octaves);
  float y1 = this->noise->GetNoise(xNoiseOffset, zNoiseOffset);

  // xNoiseOffset += 1;
  // zNoiseOffset += 1;
  this->noise->SetFrequency(this->frequency + 0.01f);
  this->noise->SetFractalOctaves(this->octaves / 4);
  float y2 = this->noise->GetNoise(xNoiseOffset, zNoiseOffset);

  // xNoiseOffset += 2;
  // zNoiseOffset += 2;
  this->noise->SetFrequency(this->frequency + 0.02f);
  this->noise->SetFractalOctaves(this->octaves / 16);
  float y3 = this->noise->GetNoise(xNoiseOffset, zNoiseOffset);

  float scale = getHeightScale(x, z);

  return (int)floor((((y1 + y2 + y3) / 3) * scale));
}

u8 TerrainManager::getBlock(int noise, int y) {
  if (y > noise) {
    return (u8)Blocks::AIR_BLOCK;
  }
  if (y == noise) {
    return (u8)Blocks::GRASS_BLOCK;
  };
  if (y >= noise - 2) {
    return (u8)Blocks::DIRTY_BLOCK;
  }
  if (y < noise - 2) {
    return (u8)Blocks::STONE_BLOCK;
  }

  if (y < 0) {
    return (u8)Blocks::WATER_BLOCK;
  }

  return (u8)Blocks::AIR_BLOCK;
}

float TerrainManager::getHeightScale(int x, int z) {
  float continentalnes = this->getContinentalness(x, z);
  float erosion = this->getErosion(x, z);
  float pv = this->getPeaksAndValleys(x, z);

  float noise = continentalnes + pv - erosion;
  // Clamp between -1 and 1
  if (noise < -2.0f) noise = -1.0;
  if (noise > 2.0f) noise = 1.0;

  // Scale based on noise;
  if (noise <= -0.9f) return 30.0f;
  if (noise > -0.9f && noise <= -0.8f) return 27.0f;
  if (noise > -0.8f && noise <= -0.7f) return 23.0f;
  if (noise > -0.7f && noise <= -0.5f) return 19.0f;
  if (noise > -0.5f && noise <= -0.4f) return 15.0f;
  if (noise > -0.4f && noise <= -0.3f) return 5.0f;
  if (noise > -0.3f && noise <= -0.2f) return 2.0f;
  if (noise > -0.2f && noise <= 0.2f) return 1.0f;
  if (noise > 0.2f && noise <= 0.3f) return 2.0f;
  if (noise > 0.4f && noise <= 0.5f) return 5.0f;
  if (noise > 0.6f && noise <= 0.7f) return 7.0f;
  if (noise > 0.8f && noise <= 0.9f) return 10.0f;
  if (noise > 1.0f && noise <= 1.3f) return 15.0f;
  if (noise > 1.3f && noise <= 1.4f) return 17.0f;
  if (noise > 1.4f && noise <= 1.5f) return 18.0f;
  if (noise > 1.5f && noise <= 1.6f) return 25.0f;
  if (noise > 1.6f && noise <= 1.9f) return 30.0f;
  if (noise > 1.9f) return 30.0f;

  return 0.0F;
}

bool TerrainManager::isBlockHidden(const Vec4& blockOffset) {
  u8 upBlock = this->getBlockTypeByOffset(blockOffset.x, blockOffset.y - 1,
                                          blockOffset.z);
  u8 downBlock = this->getBlockTypeByOffset(blockOffset.x, blockOffset.y + 1,
                                            blockOffset.z);
  u8 frontBlock = this->getBlockTypeByOffset(blockOffset.x, blockOffset.y,
                                             blockOffset.z + 1);
  u8 backBlock = this->getBlockTypeByOffset(blockOffset.x, blockOffset.y,
                                            blockOffset.z - 1);
  u8 rightBlock = this->getBlockTypeByOffset(blockOffset.x + 1, blockOffset.y,
                                             blockOffset.z);
  u8 leftBlock = this->getBlockTypeByOffset(blockOffset.x - 1, blockOffset.y,
                                            blockOffset.z);

  // If some nighbor block is transparent set block to visible
  if (
      // Up block
      upBlock == (u8)Blocks::AIR_BLOCK || upBlock == (u8)Blocks::GLASS_BLOCK ||
      // Down block
      downBlock == (u8)Blocks::AIR_BLOCK ||
      downBlock == (u8)Blocks::GLASS_BLOCK ||
      // Front block
      frontBlock == (u8)Blocks::AIR_BLOCK ||
      frontBlock == (u8)Blocks::GLASS_BLOCK ||
      // Back block
      backBlock == (u8)Blocks::AIR_BLOCK ||
      backBlock == (u8)Blocks::GLASS_BLOCK ||
      // Right block
      rightBlock == (u8)Blocks::AIR_BLOCK ||
      rightBlock == (u8)Blocks::GLASS_BLOCK ||
      // Left block
      leftBlock == (u8)Blocks::AIR_BLOCK ||
      leftBlock == (u8)Blocks::GLASS_BLOCK) {
    return false;
  } else
    return true;
}

u8 TerrainManager::getBlockTypeByOffset(const int& x, const int& y,
                                        const int& z) {
  return terrain[this->getIndexByOffset(x, y, z)];
}

unsigned int TerrainManager::getIndexByPosition(Vec4* normalizedWorldPosition) {
  int offsetZ = (normalizedWorldPosition->z / DUBLE_BLOCK_SIZE) +
                HALF_OVERWORLD_H_DISTANCE;
  int offsetX = (normalizedWorldPosition->x / DUBLE_BLOCK_SIZE) +
                HALF_OVERWORLD_H_DISTANCE;
  int offsetY = (normalizedWorldPosition->y / DUBLE_BLOCK_SIZE) +
                HALF_OVERWORLD_V_DISTANCE;

  int _z = offsetZ >= 0 && offsetZ < OVERWORLD_H_DISTANCE
               ? offsetZ * OVERWORLD_H_DISTANCE * OVERWORLD_V_DISTANCE
               : 0;
  int _x = offsetX >= 0 && offsetX < OVERWORLD_H_DISTANCE
               ? offsetX * OVERWORLD_V_DISTANCE
               : 0;
  int _y = offsetY >= 0 && offsetY < OVERWORLD_V_DISTANCE ? offsetY : 0;

  return _z + _x + _y;
}

unsigned int TerrainManager::getIndexByOffset(int x, int y, int z) {
  int offsetZ = z + HALF_OVERWORLD_H_DISTANCE;
  int offsetX = x + HALF_OVERWORLD_H_DISTANCE;
  int offsetY = y + HALF_OVERWORLD_V_DISTANCE;

  int _z = offsetZ > 0 && z < OVERWORLD_H_DISTANCE
               ? offsetZ * OVERWORLD_H_DISTANCE * OVERWORLD_V_DISTANCE
               : 0;
  int _x = offsetX > 0 && x < OVERWORLD_H_DISTANCE
               ? offsetX * OVERWORLD_V_DISTANCE
               : 0;
  int _y = offsetY > 0 && y < OVERWORLD_V_DISTANCE ? offsetY : 0;

  return _z + _x + _y;
}

const Vec4 TerrainManager::getPositionByIndex(const u16& index) {
  int mod = index;
  int z = OVERWORLD_MIN_DISTANCE;
  int x = OVERWORLD_MIN_DISTANCE;
  int y = OVERWORLD_MIN_HEIGH;

  if (mod >= OVERWORLD_H_DISTANCE * OVERWORLD_V_DISTANCE) {
    z = floor(mod / (OVERWORLD_H_DISTANCE * OVERWORLD_V_DISTANCE)) -
        HALF_OVERWORLD_H_DISTANCE;
    mod = mod % (OVERWORLD_H_DISTANCE * OVERWORLD_V_DISTANCE);
  }

  if (mod >= OVERWORLD_V_DISTANCE) {
    x = floor(mod / OVERWORLD_V_DISTANCE) - HALF_OVERWORLD_H_DISTANCE;
    mod = mod % OVERWORLD_H_DISTANCE;
  }

  if (mod < OVERWORLD_V_DISTANCE) {
    y = mod - HALF_OVERWORLD_V_DISTANCE;
  }

  return Vec4(x, y, z) * DUBLE_BLOCK_SIZE;
}

void TerrainManager::buildChunk(Chunck* t_chunck) {
  for (int z = t_chunck->minOffset->z; z < t_chunck->maxOffset->z; z++) {
    for (int x = t_chunck->minOffset->x; x < t_chunck->maxOffset->x; x++) {
      for (int y = OVERWORLD_MIN_DISTANCE; y < OVERWORLD_MAX_DISTANCE; y++) {
        unsigned int blockIndex = this->getIndexByOffset(x, y, z);
        u8 block_type = this->terrain[blockIndex];
        if (block_type <= (u8)Blocks::AIR_BLOCK) continue;

        Vec4 tempBlockOffset = Vec4(x, y, z);
        Vec4 blockPosition = (tempBlockOffset * DUBLE_BLOCK_SIZE);

        u8 isHidden = this->isBlockHidden(tempBlockOffset);

        // Are block's coordinates in world range?
        if (!isHidden &&
            tempBlockOffset.collidesBox(minWorldPos, maxWorldPos)) {
          BlockInfo* blockInfo = this->t_blockManager->getBlockTexOffsetByType(
              static_cast<Blocks>(block_type));
          if (blockInfo != nullptr) {
            Block* block = new Block(blockInfo);
            block->index = blockIndex;

            float bright = this->getBlockLuminosity(tempBlockOffset.y);
            block->color = Color(bright, bright, bright, 128.0F);

            block->setPosition(blockPosition);
            block->scale.scale(BLOCK_SIZE);
            block->updateModelMatrix();

            // Calc min and max corners
            {
              BBox tempBBox = this->rawBlockBbox->getTransformed(block->model);
              block->bbox = new BBox(tempBBox);
              block->bbox->getMinMax(&block->minCorner, &block->maxCorner);
            }

            t_chunck->addBlock(block);
          }
        }
      }
    }
  }

  t_chunck->state = ChunkState::Loaded;
  t_chunck->updateDrawData();
}

void TerrainManager::updateTargetBlock(const Vec4& playerPosition,
                                       Camera* t_camera,
                                       std::vector<Chunck*> chuncks) {
  u8 hitedABlock = 0;
  float tempTargetDistance = -1.0f;
  float tempPlayerDistance = -1.0f;
  Block* tempTargetBlock = nullptr;

  // Reset the current target block;
  this->targetBlock = nullptr;

  // Prepate the raycast
  Vec4 rayDir = t_camera->lookPos - t_camera->position;
  rayDir.normalize();
  ray.origin.set(t_camera->position);
  ray.direction.set(rayDir);

  for (u16 h = 0; h < chuncks.size(); h++) {
    // Isn't loaded or is out of frustum
    if (chuncks[h]->state != ChunkState::Loaded || !chuncks[h]->isVisible())
      continue;

    for (u16 i = 0; i < chuncks[h]->blocks.size(); i++) {
      float distanceFromCurrentBlockToPlayer =
          playerPosition.distanceTo(*chuncks[h]->blocks[i]->getPosition());

      if (distanceFromCurrentBlockToPlayer <= MAX_RANGE_PICKER) {
        // Reset block state
        chuncks[h]->blocks[i]->isTarget = 0;
        chuncks[h]->blocks[i]->distance = -1.0f;

        float intersectionPoint;
        if (ray.intersectBox(chuncks[h]->blocks[i]->minCorner,
                             chuncks[h]->blocks[i]->maxCorner,
                             &intersectionPoint)) {
          hitedABlock = 1;
          if (tempTargetDistance == -1.0f ||
              (distanceFromCurrentBlockToPlayer < tempPlayerDistance)) {
            tempTargetBlock = chuncks[h]->blocks[i];
            tempTargetDistance = intersectionPoint;
            tempPlayerDistance = distanceFromCurrentBlockToPlayer;
          }
        }
      }
    }
  }

  if (hitedABlock) {
    this->targetBlock = tempTargetBlock;
    this->targetBlock->isTarget = 1;
    this->targetBlock->distance = tempTargetDistance;
  }
}

void TerrainManager::removeBlock() {
  if (this->targetBlock == nullptr) return;

  this->_modifiedPosition.set(*this->targetBlock->getPosition());
  terrain[this->targetBlock->index] = (u8)Blocks::AIR_BLOCK;
  this->_shouldUpdateChunck = 1;
  this->playDestroyBlockSound(this->targetBlock->type);
}

void TerrainManager::putBlock(const Blocks& blockToPlace) {
  if (this->targetBlock == nullptr) return;

  int terrainIndex = this->targetBlock->index;
  Vec4 targetPos = ray.at(this->targetBlock->distance);
  Vec4 newBlockPos = *this->targetBlock->getPosition();

  // Front
  if (std::round(targetPos.z) ==
      this->targetBlock->bbox->getFrontFace().axisPosition) {
    terrainIndex += OVERWORLD_H_DISTANCE * OVERWORLD_V_DISTANCE;
    newBlockPos.z += DUBLE_BLOCK_SIZE;
    // Back
  } else if (std::round(targetPos.z) ==
             this->targetBlock->bbox->getBackFace().axisPosition) {
    terrainIndex -= OVERWORLD_H_DISTANCE * OVERWORLD_V_DISTANCE;
    newBlockPos.z -= DUBLE_BLOCK_SIZE;
    // Right
  } else if (std::round(targetPos.x) ==
             this->targetBlock->bbox->getRightFace().axisPosition) {
    terrainIndex += OVERWORLD_V_DISTANCE;
    newBlockPos.x += DUBLE_BLOCK_SIZE;
    // Left
  } else if (std::round(targetPos.x) ==
             this->targetBlock->bbox->getLeftFace().axisPosition) {
    terrainIndex -= OVERWORLD_V_DISTANCE;
    newBlockPos.x -= DUBLE_BLOCK_SIZE;
    // Up
  } else if (std::round(targetPos.y) ==
             this->targetBlock->bbox->getTopFace().axisPosition) {
    terrainIndex++;
    newBlockPos.y += DUBLE_BLOCK_SIZE;
    // Down
  } else if (std::round(targetPos.y) ==
             this->targetBlock->bbox->getBottomFace().axisPosition) {
    terrainIndex--;
    newBlockPos.y -= DUBLE_BLOCK_SIZE;
  }

  if (terrainIndex <= OVERWORLD_SIZE &&
      terrainIndex != this->targetBlock->index) {
    // Prevent to put a block at the player position;
    {
      M4x4 tempModel = M4x4();
      tempModel.identity();
      tempModel.scale(BLOCK_SIZE);
      tempModel.translate(newBlockPos);

      BBox tempBBox = this->rawBlockBbox->getTransformed(tempModel);
      Vec4 newBlockPosMin;
      Vec4 newBlockPosMax;
      tempBBox.getMinMax(&newBlockPosMin, &newBlockPosMax);

      Vec4 minPlayerCorner;
      Vec4 maxPlayerCorner;
      this->t_player->getHitBox().getMinMax(&minPlayerCorner, &maxPlayerCorner);

      // Will Collide to player?
      if (newBlockPosMax.x > minPlayerCorner.x &&
          newBlockPosMin.x < maxPlayerCorner.x &&
          newBlockPosMax.z > minPlayerCorner.z &&
          newBlockPosMin.z < maxPlayerCorner.z &&
          newBlockPosMax.y > minPlayerCorner.y &&
          newBlockPosMin.y < maxPlayerCorner.y)
        return;  // Return on collision
    }

    this->_modifiedPosition.set(newBlockPos);
    this->terrain[terrainIndex] = (u8)blockToPlace;
    this->_shouldUpdateChunck = 1;
    this->playPutBlockSound(blockToPlace);
  }
}

void TerrainManager::handlePadControls(Pad* t_pad, const float& deltaTime) {
  if (t_pad->getPressed().L2 && this->targetBlock != nullptr) {
    if (!this->targetBlock->isBreakable) return;
    this->breakBlock(this->targetBlock, deltaTime);
  } else if (this->_isBreakingBlock) {
    this->_isBreakingBlock = false;
    if (this->targetBlock) this->targetBlock->damage = 0;
  }

  if (t_pad->getClicked().R2) {
    ItemId activeItemType = this->t_player->getSelectedInventoryItemType();
    if (activeItemType != ItemId::empty) {
      const Blocks blockid =
          this->t_itemRepository->getItemById(activeItemType)->blockId;
      if (blockid != Blocks::AIR_BLOCK) this->putBlock(blockid);
    }
  }
}

void TerrainManager::breakBlock(Block* blockToBreak, const float& deltaTime) {
  if (this->targetBlock == nullptr) return;

  if (this->_isBreakingBlock) {
    this->breaking_time_pessed += deltaTime;

    if (breaking_time_pessed >= this->t_blockManager->getBlockBreakingTime()) {
      // Remove block;
      this->removeBlock();

      // Target block has changed, reseting the pressed time;
      this->breaking_time_pessed = 0;
    } else {
      // Update damage overlay
      this->targetBlock->damage = breaking_time_pessed /
                                  this->t_blockManager->getBlockBreakingTime() *
                                  100;
      this->playBreakingBlockSound(blockToBreak->type);
    }
  } else {
    this->breaking_time_pessed = 0;
    this->_isBreakingBlock = true;
  }
}

Vec4* TerrainManager::normalizeWorldBlockPosition(Vec4* worldPosition) {
  return new Vec4((worldPosition->x == 0
                       ? 0
                       : std::trunc((worldPosition->x + DUBLE_BLOCK_SIZE) /
                                    DUBLE_BLOCK_SIZE) *
                             DUBLE_BLOCK_SIZE),
                  (worldPosition->y == 0
                       ? 0
                       : std::trunc((worldPosition->y + DUBLE_BLOCK_SIZE) /
                                    DUBLE_BLOCK_SIZE) *
                             DUBLE_BLOCK_SIZE),
                  (worldPosition->z == 0
                       ? 0
                       : std::trunc((worldPosition->z + DUBLE_BLOCK_SIZE) /
                                    DUBLE_BLOCK_SIZE) *
                             DUBLE_BLOCK_SIZE));
}

const Vec4 TerrainManager::defineSpawnArea() {
  Vec4 spawPos = this->calcSpawOffset();
  return spawPos;
}

const Vec4 TerrainManager::calcSpawOffset(int bias) {
  u8 found = 0;
  u8 airBlockCounter = 0;
  // Pick a X and Z coordinates based on the seed;
  int posX =
      ((seed + bias) % HALF_OVERWORLD_H_DISTANCE) - HALF_OVERWORLD_H_DISTANCE;
  int posZ =
      ((seed - bias) % HALF_OVERWORLD_H_DISTANCE) - HALF_OVERWORLD_H_DISTANCE;
  Vec4 result;

  for (int posY = OVERWORLD_MAX_HEIGH; posY >= OVERWORLD_MIN_HEIGH; posY--) {
    int index = this->getIndexByOffset(posX, posY, posZ);
    u8 type = this->terrain[index];
    if (type != (u8)Blocks::AIR_BLOCK && airBlockCounter >= 4) {
      found = 1;
      result = Vec4(posX, posY + 2, posZ);
      break;
    }

    if (type == (u8)Blocks::AIR_BLOCK)
      airBlockCounter++;
    else
      airBlockCounter = 0;
  }

  if (found)
    return result * DUBLE_BLOCK_SIZE;
  else
    return calcSpawOffset(bias + 1);
}

float TerrainManager::getContinentalness(int x, int z) {
  this->noise->SetFrequency(this->frequency);
  this->noise->SetFractalOctaves(1);

  return this->noise->GetNoise((float)(x + seed), (float)(z + seed));
}

float TerrainManager::getErosion(int x, int z) {
  this->noise->SetFrequency(this->frequency);
  this->noise->SetFractalOctaves(2);

  return this->noise->GetNoise((float)(x + seed), (float)(z + seed));
}

float TerrainManager::getPeaksAndValleys(int x, int z) {
  this->noise->SetFrequency(this->frequency);
  this->noise->SetFractalOctaves(1);

  return this->noise->GetNoise((float)(x + seed), (float)(z + seed));
}

float TerrainManager::getDensity(int x, int y, int z) {
  return this->noise->GetNoise((float)(x + seed), (float)(y + seed),
                               (float)(z + seed));
}

float TerrainManager::getTemperature(int x, int z) {
  this->noise->SetFrequency(this->frequency);
  this->noise->SetFractalOctaves(1);

  return this->noise->GetNoise((float)(x + seed), (float)(z + seed));
}

float TerrainManager::getHumidity(int x, int z) {
  this->noise->SetFrequency(this->frequency);
  this->noise->SetFractalOctaves(1);

  return this->noise->GetNoise((float)(x + seed), (float)(z + seed));
}

void TerrainManager::generateTrees() {
  this->noise->SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
  this->noise->SetFractalType(FastNoiseLite::FractalType_DomainWarpIndependent);
  this->noise->SetFractalOctaves(this->octaves);
  this->noise->SetFractalLacunarity(10.0f);
  this->noise->SetFrequency(1.0f);

  for (int z = OVERWORLD_MIN_DISTANCE; z < OVERWORLD_MAX_DISTANCE; z++) {
    for (int x = OVERWORLD_MIN_DISTANCE; x < OVERWORLD_MAX_DISTANCE; x++) {
      int noise = floor(
          this->noise->GetNoise((float)(x + seed), (float)(z + seed)) * 7.6);
      int modX = noise % 3;
      int modZ = noise % 9;
      u8 treeHeight;
      u8 shouldPutATree = noise > 5 && (modX == 1 || modZ == 7) ? 1 : 0;

      if (noise < 10 && noise >= 4)
        treeHeight = noise;
      else if (noise < 3)
        treeHeight = 4;
      else if (noise > 9)
        treeHeight = 9;

      if (shouldPutATree) {
        // TODO: Refactor: Sould generate tree based on biome type
        TreeType treeType = this->getTreeType(x, z);

        if (treeType == TreeType::Oak)
          this->placeOakTreeAt(x, z, treeHeight);
        else if (treeType == TreeType::Birch)
          this->placeBirchTreeAt(x, z, treeHeight);
      }
    }
  }
}

TreeType TerrainManager::getTreeType(const int& x, const int& z) {
  FastNoiseLite fnl = FastNoiseLite(this->seed);
  fnl.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2S);
  fnl.SetFractalType(FastNoiseLite::FractalType_DomainWarpProgressive);
  fnl.SetFrequency(0.005f);

  const float noise = fnl.GetNoise((float)(x), (float)(z));

  if (noise > 0.5f) return TreeType::Birch;

  return TreeType::Oak;
}

void TerrainManager::placeTreeAt(const int& x, const int& z,
                                 const u8& treeHeight, const Blocks& logType,
                                 const Blocks& leaveType) {
  for (int y = OVERWORLD_MAX_HEIGH - 1; y >= OVERWORLD_MIN_HEIGH; y--) {
    int index = this->getIndexByOffset(x, y, z);
    u8 type = this->terrain[index];

    if (y >= 0 && type != (u8)Blocks::AIR_BLOCK &&
        (type == (u8)Blocks::GRASS_BLOCK || type == (u8)Blocks::DIRTY_BLOCK)) {
      for (int j = 0; j <= (treeHeight + 1); j++) {
        u32 treeBlockIndex = this->getIndexByOffset(x, y + j, z);

        // Place logs
        if (j <= treeHeight &&
            this->terrain[treeBlockIndex] == (u8)Blocks::AIR_BLOCK)
          this->terrain[treeBlockIndex] = (u8)logType;

        // Place leaves
        if (j >= treeHeight - 3) {
          Vec4 center = Vec4(x, y + j, z);

          int radianOffset = 0;
          if (j == (treeHeight - 3) || j == (treeHeight - 2))
            radianOffset = 3;
          else if (j == (treeHeight - 1))
            radianOffset = 2;
          else if (j == treeHeight)
            radianOffset = 1;

          for (int k = -2; k < 3; k++) {
            for (int l = -2; l < 3; l++) {
              u32 treeLeaveBlockIndex =
                  this->getIndexByOffset(x + k, y + j, z + l);
              Vec4 leafPos = Vec4(x + k, y + j, z + l);

              if (this->terrain[treeLeaveBlockIndex] == (u8)Blocks::AIR_BLOCK &&
                  center.distanceTo(leafPos) <= radianOffset) {
                this->terrain[treeLeaveBlockIndex] = (u8)leaveType;
              }
            }
          }
        }
      }
      break;
    }
  }
}

void TerrainManager::placeOakTreeAt(const int& x, const int& z,
                                    const u8& treeHeight) {
  this->placeTreeAt(x, z, treeHeight, Blocks::OAK_LOG_BLOCK,
                    Blocks::OAK_LEAVES_BLOCK);
}

void TerrainManager::placeBirchTreeAt(const int& x, const int& z,
                                      const u8& treeHeight) {
  this->placeTreeAt(x, z, treeHeight, Blocks::BIRCH_LOG_BLOCK,
                    Blocks::BIRCH_LEAVES_BLOCK);
}

void TerrainManager::calcRawBlockBBox(MinecraftPipeline* mcPip) {
  const auto& blockData = mcPip->getBlockData();
  this->rawBlockBbox = new BBox(blockData.vertices, blockData.count);
}

void TerrainManager::generateWater() {
  int index = 0;
  for (int z = OVERWORLD_MIN_DISTANCE; z < OVERWORLD_MAX_DISTANCE; z++) {
    for (int x = OVERWORLD_MIN_DISTANCE; x < OVERWORLD_MAX_DISTANCE; x++) {
      for (int y = OVERWORLD_MIN_HEIGH; y < OVERWORLD_MAX_HEIGH; y++) {
        if (this->terrain[index] == (u8)Blocks::AIR_BLOCK) {
          // If Y is lower than 0, then fill with water;
          if (y < 0) this->terrain[index] = (u8)Blocks::WATER_BLOCK;

          // Place sand arround the watter;
          u8 leftIndex = this->getBlockTypeByOffset(x - 1, y, z);
          u8 rightIndex = this->getBlockTypeByOffset(x + 1, y, z);
          u8 frontIndex = this->getBlockTypeByOffset(x, y, z - 1);
          u8 backIndex = this->getBlockTypeByOffset(x, y, z + 1);

          if (this->terrain[leftIndex] == (u8)Blocks::DIRTY_BLOCK ||
              this->terrain[leftIndex] == (u8)Blocks::GRASS_BLOCK) {
            this->terrain[leftIndex] = (u8)Blocks::SAND_BLOCK;
          }
          if (this->terrain[rightIndex] == (u8)Blocks::DIRTY_BLOCK ||
              this->terrain[rightIndex] == (u8)Blocks::GRASS_BLOCK) {
            this->terrain[rightIndex] = (u8)Blocks::SAND_BLOCK;
          }
          if (this->terrain[frontIndex] == (u8)Blocks::DIRTY_BLOCK ||
              this->terrain[frontIndex] == (u8)Blocks::GRASS_BLOCK) {
            this->terrain[frontIndex] = (u8)Blocks::SAND_BLOCK;
          }
          if (this->terrain[backIndex] == (u8)Blocks::DIRTY_BLOCK ||
              this->terrain[backIndex] == (u8)Blocks::GRASS_BLOCK) {
            this->terrain[backIndex] = (u8)Blocks::SAND_BLOCK;
          }
        };

        index++;
      }
    }
  }
}

u8 TerrainManager::shouldUpdateTargetBlock() {
  return this->framesCounter == this->UPDATE_TARGET_LIMIT;
}

float TerrainManager::getBlockLuminosity(const float& yPosition) {
  float luminosity =
      (yPosition * 110.0F * (1 / (float)OVERWORLD_MAX_HEIGH)) + 100.0F;
  if (luminosity < 90)
    return 90.0F;
  else if (luminosity > 128)
    return 128.0F;
  else
    return luminosity;
}

void TerrainManager::playPutBlockSound(const Blocks& blockType) {
  if (blockType != Blocks::AIR_BLOCK) {
    SfxBlockModel* blockSfxModel =
        this->t_blockManager->getDigSoundByBlockType(blockType);
    if (blockSfxModel != nullptr)
      this->t_soundManager->playSfx(blockSfxModel->category,
                                    blockSfxModel->sound,
                                    BLOCK_PLACEMENT_SFX_CH);
  }
}

void TerrainManager::playDestroyBlockSound(const Blocks& blockType) {
  if (blockType != Blocks::AIR_BLOCK) {
    SfxBlockModel* blockSfxModel =
        this->t_blockManager->getDigSoundByBlockType(blockType);

    if (blockSfxModel != nullptr)
      this->t_soundManager->playSfx(blockSfxModel->category,
                                    blockSfxModel->sound,
                                    BLOCK_PLACEMENT_SFX_CH);
  }
}

void TerrainManager::playBreakingBlockSound(const Blocks& blockType) {
  if (blockType != Blocks::AIR_BLOCK) {
    SfxBlockModel* blockSfxModel =
        this->t_blockManager->getDigSoundByBlockType(blockType);

    if (blockSfxModel != nullptr)
      this->t_soundManager->playSfx(blockSfxModel->category,
                                    blockSfxModel->sound,
                                    BLOCK_PLACEMENT_SFX_CH);
  }
}
