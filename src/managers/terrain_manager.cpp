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

TerrainManager::~TerrainManager() {}

void TerrainManager::init(Renderer* t_renderer, ItemRepository* itemRepository,
                          MinecraftPipeline* mcPip,
                          BlockManager* blockManager) {
  printf("Iniating Terrain manger\n");
  this->t_renderer = t_renderer;
  this->t_itemRepository = itemRepository;
  this->t_mcPip = mcPip;
  this->t_blockManager = blockManager;

  int terrainType = 0;
  int testterrain = rand() % 10;
  if (testterrain < 4) terrainType = 0;
  if (testterrain >= 4 && testterrain < 7) terrainType = 1;
  if (testterrain >= 7) terrainType = 2;

  this->calcRawBlockBBox(mcPip);
  this->initNoise();
  this->generateNewTerrain(terrainType, false, true);
}

void TerrainManager::update(Player* t_player, Camera* t_camera, Pad* t_pad,
                            std::vector<Chunck*> chuncks) {
  this->t_player = t_player;
  this->t_camera = t_camera;
  this->getTargetBlock(*t_player->getPosition(), t_camera, chuncks);
  this->handlePadControls(t_pad);
};

void TerrainManager::generateNewTerrain(int terrainType, bool makeFlat,
                                        bool makeCaves) {
  int index = 0;
  int noise = 0;
  float density;

  for (int z = OVERWORLD_MIN_DISTANCE; z < OVERWORLD_MAX_DISTANCE; z++) {
    for (int x = OVERWORLD_MIN_DISTANCE; x < OVERWORLD_MAX_DISTANCE; x++) {
      noise = getNoise(x, z);
      for (int y = OVERWORLD_MIN_HEIGH; y < OVERWORLD_MAX_HEIGH; y++) {
        if (makeFlat) {
          this->terrain[index] = y <= 0 ? GRASS_BLOCK : AIR_BLOCK;
        } else {
          density = getDensity(x, y, z);
          if (density <= 0) {
            this->terrain[index] = AIR_BLOCK;
          } else {
            this->terrain[index] = this->getBlock(noise, y);
          }
        }
        index++;
      }
    }
  }

  this->generateWater();
  this->generateTrees();
}

void TerrainManager::initNoise() {
  // Fast Noise Lite
  this->noise = new FastNoiseLite(seed);
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
    return AIR_BLOCK;
  }
  if (y == noise) {
    return GRASS_BLOCK;
  };
  if (y >= noise - 2) {
    return DIRTY_BLOCK;
  }
  if (y < noise - 2) {
    return STONE_BLOCK;
  }

  if (y < 0) {
    return WATER_BLOCK;
  }

  return AIR_BLOCK;
}

float TerrainManager::getHeightScale(int x, int z) {
  float continentalnes = this->getContinentalness(x, z);
  float erosion = this->getErosion(x, z);
  float pv = this->getPeaksAndValleys(x, z);

  float noise = continentalnes + pv - erosion;
  // Clamp between -1 and 1
  if (noise < -1.0f) noise = -1.0;
  if (noise > 1.0f) noise = 1.0;

  // Scale based on noise;
  if (noise <= -0.9f) return 30.0f;
  if (noise > -0.9f && noise <= -0.7f) return 2.0f;
  if (noise > -0.7f && noise <= -0.6f) return 9.0f;
  if (noise > -0.6f && noise <= -0.2f) return 11.0f;
  if (noise > -0.2f && noise <= -0.0f) return 12.0f;
  if (noise > 0.0f && noise <= 0.8f) return 20.0f;
  if (noise > 0.8f && noise <= 0.9f) return 25.0f;
  if (noise <= 1.0f) return 30.0f;
}

bool TerrainManager::isBlockHidden(int x, int y, int z) {
  u8 upBlock = this->getBlockTypeByOffset(x, y - 1, z);
  u8 downBlock = this->getBlockTypeByOffset(x, y + 1, z);
  u8 frontBlock = this->getBlockTypeByOffset(x, y, z + 1);
  u8 backBlock = this->getBlockTypeByOffset(x, y, z - 1);
  u8 rightBlock = this->getBlockTypeByOffset(x + 1, y, z);
  u8 leftBlock = this->getBlockTypeByOffset(x - 1, y, z);

  // If some nighbor block is AIR_BLOCK set block to visible
  if (
      // Up block
      upBlock == AIR_BLOCK || upBlock == GLASS_BLOCK ||
      // Down block
      downBlock == AIR_BLOCK || downBlock == GLASS_BLOCK ||
      // Front block
      frontBlock == AIR_BLOCK || frontBlock == GLASS_BLOCK ||
      // Back block
      backBlock == AIR_BLOCK || backBlock == GLASS_BLOCK ||
      // Right block
      rightBlock == AIR_BLOCK || rightBlock == GLASS_BLOCK ||
      // Left block
      leftBlock == AIR_BLOCK || leftBlock == GLASS_BLOCK) {
    return false;
  }
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

Vec4* TerrainManager::getPositionByIndex(unsigned int index) {
  Vec4* pos = new Vec4();
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

  pos->set(x, y, z);
  return pos;
}

void TerrainManager::buildChunk(Chunck* t_chunck) {
  t_chunck->clear();

  for (int z = t_chunck->minCorner->z; z < t_chunck->maxCorner->z; z++) {
    for (int x = t_chunck->minCorner->x; x < t_chunck->maxCorner->x; x++) {
      for (int y = OVERWORLD_MIN_DISTANCE; y < OVERWORLD_MAX_DISTANCE; y++) {
        Vec4* tempBlockOffset = new Vec4(x, y, z);
        Vec4 blockPosition = (*tempBlockOffset * DUBLE_BLOCK_SIZE);

        unsigned int blockIndex = this->getIndexByOffset(
            tempBlockOffset->x, tempBlockOffset->y, tempBlockOffset->z);
        u8 block_type = this->terrain[blockIndex];
        u8 isHidden = this->isBlockHidden(
            tempBlockOffset->x, tempBlockOffset->y, tempBlockOffset->z);

        // Are block's coordinates in world range?
        if (block_type != AIR_BLOCK && !isHidden &&
            tempBlockOffset->collidesBox(minWorldPos, maxWorldPos)) {
          BlockInfo* blockInfo =
              this->t_blockManager->getBlockTexOffsetByType(block_type);
          if (blockInfo != nullptr) {
            Block* block = new Block(blockInfo);
            block->index = blockIndex;
            block->isHidden = isHidden;
            block->color = Color(116.0F, 116.0F, 116.0F, 128.0F);

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

        delete tempBlockOffset;
        tempBlockOffset = NULL;
      }
    }
  }

  t_chunck->isLoaded = 1;
}

void TerrainManager::getTargetBlock(const Vec4& playerPosition,
                                    Camera* t_camera,
                                    std::vector<Chunck*> chuncks) {
  u8 hitedABlock = 0;
  float distance = -1.0f;
  float tempTargetDistance = -1.0f;
  float tempPlayerDistance = -1.0f;
  Block* tempTargetBlock;
  const auto* frustumPlanes =
      this->t_renderer->core.renderer3D.frustumPlanes.getAll();

  // Reset the current target block;
  this->targetBlock = NULL;

  // Prepate the raycast
  Vec4 rayDir = t_camera->lookPos - t_camera->position;
  rayDir.normalize();
  ray.set(t_camera->position, rayDir);

  for (u16 h = 0; h < chuncks.size(); h++) {
    if (!chuncks[h]->isLoaded) continue;
    for (u16 i = 0; i < chuncks[h]->blocks.size(); i++) {
      float distanceFromCurrentBlockToPlayer =
          playerPosition.distanceTo(*chuncks[h]->blocks[i]->getPosition());

      if (distanceFromCurrentBlockToPlayer <= MAX_RANGE_PICKER) {
        // Reset block state
        chuncks[h]->blocks[i]->isTarget = 0;
        chuncks[h]->blocks[i]->distance = -1.0f;

        // Check if is in frustum
        {
          u8 isInFrustum = chuncks[h]->blocks[i]->bbox->isInFrustum(
              frustumPlanes, chuncks[h]->blocks[i]->model, 0);
          if (!isInFrustum) {
            return;
          }
        }

        float intersectionPoint = Utils::Raycast(
            &t_camera->position, &rayDir, &chuncks[h]->blocks[i]->minCorner,
            &chuncks[h]->blocks[i]->maxCorner);
        if (intersectionPoint > -1) {
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
  if (this->targetBlock == NULL) return;

  terrain[this->targetBlock->index] = AIR_BLOCK;
  this->_shouldUpdateChunck = 1;
}

void TerrainManager::putBlock(u8 blockToPlace) {
  printf("Put block\n");
  if (this->targetBlock == NULL || this->targetBlock == nullptr) return;

  // Prevent to put a block at the player position;
  if (this->t_player->getPosition()->collidesBox(this->targetBlock->minCorner,
                                                 this->targetBlock->maxCorner))
    return;

  // Detect face
  Vec4 targetPos = ray.at(this->targetBlock->distance);
  int terrainIndex = this->targetBlock->index;

  if (std::round(targetPos.z) ==
      this->targetBlock->bbox->getFrontFace().axisPosition) {
    terrainIndex += OVERWORLD_H_DISTANCE * OVERWORLD_V_DISTANCE;
  } else if (std::round(targetPos.z) ==
             this->targetBlock->bbox->getBackFace().axisPosition) {
    terrainIndex -= OVERWORLD_H_DISTANCE * OVERWORLD_V_DISTANCE;
  } else if (std::round(targetPos.x) ==
             this->targetBlock->bbox->getRightFace().axisPosition) {
    terrainIndex += OVERWORLD_V_DISTANCE;
  } else if (std::round(targetPos.x) ==
             this->targetBlock->bbox->getLeftFace().axisPosition) {
    terrainIndex -= OVERWORLD_V_DISTANCE;
  } else if (std::round(targetPos.y) ==
             this->targetBlock->bbox->getTopFace().axisPosition) {
    terrainIndex += 1;
  } else if (std::round(targetPos.y) ==
             this->targetBlock->bbox->getBottomFace().axisPosition) {
    terrainIndex -= 1;
  }

  if (terrainIndex <= OVERWORLD_SIZE &&
      terrainIndex != this->targetBlock->index) {
    this->terrain[terrainIndex] = blockToPlace;
    this->_shouldUpdateChunck = 1;
  }
}

void TerrainManager::handlePadControls(Pad* t_pad) {
  PadButtons clickedButton = t_pad->getClicked();
  if (clickedButton.L2) {
    this->removeBlock();
  }
  if (clickedButton.R2) {
    ITEM_TYPES activeItemType = this->t_player->getSelectedInventoryItemType();
    if (activeItemType != ITEM_TYPES::empty) {
      int blockid =
          this->t_itemRepository->getItemById(activeItemType)->blockId;
      if (blockid != AIR_BLOCK) this->putBlock(blockid);
    }
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
    if (type != AIR_BLOCK && airBlockCounter >= 4) {
      found = 1;
      result = Vec4(posX, posY + 2, posZ);
      break;
    }

    if (type == AIR_BLOCK)
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
  this->noise->SetFractalType(FastNoiseLite::FractalType_DomainWarpProgressive);
  this->noise->SetFrequency(0.0001);
  this->noise->SetFractalLacunarity(1.5);
  this->noise->SetFractalOctaves(8);
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

      if (noise < 10 && noise > 5)
        treeHeight = noise;
      else if (noise < 6)
        treeHeight = 6;
      else if (noise > 9)
        treeHeight = 9;

      if (shouldPutATree) this->placeTreeAt(x, z, treeHeight);
    }
  }
}

void TerrainManager::placeTreeAt(int x, int z, u8 treeHeight) {
  for (int y = OVERWORLD_MAX_HEIGH - 1; y >= OVERWORLD_MIN_HEIGH; y--) {
    int index = this->getIndexByOffset(x, y, z);
    u8 type = this->terrain[index];

    if (y >= 0 && type != AIR_BLOCK &&
        (type == GRASS_BLOCK || type == DIRTY_BLOCK)) {
      for (int j = 0; j <= treeHeight; j++) {
        u32 treeBlockIndex = this->getIndexByOffset(x, y + j, z);

        // Place logs
        if (this->terrain[treeBlockIndex] == AIR_BLOCK)
          this->terrain[treeBlockIndex] = OAK_LOG_BLOCK;

        // Place leaves
        if (j >= treeHeight - 2) {
          for (int k = -2; k < 3; k++) {
            u32 treeLeaveBlockIndex;

            treeLeaveBlockIndex = this->getIndexByOffset(x + k, y + j, z);
            if (this->terrain[treeLeaveBlockIndex] == AIR_BLOCK)
              this->terrain[treeLeaveBlockIndex] = OAK_LEAVES_BLOCK;

            treeLeaveBlockIndex = this->getIndexByOffset(x, y + j, z + k);
            if (this->terrain[treeLeaveBlockIndex] == AIR_BLOCK)
              this->terrain[treeLeaveBlockIndex] = OAK_LEAVES_BLOCK;
          }
        }
      }
      break;
    }
  }
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
        if (this->terrain[index] == AIR_BLOCK) {
          // If Y is lower than 0, then fill with water;
          if (y < 0) this->terrain[index] = WATER_BLOCK;

          // Place sand arround the watter;
          u8 leftIndex = this->getBlockTypeByOffset(x - 1, y, z);
          u8 rightIndex = this->getBlockTypeByOffset(x + 1, y, z);
          u8 frontIndex = this->getBlockTypeByOffset(x, y, z - 1);
          u8 backIndex = this->getBlockTypeByOffset(x, y, z + 1);

          if (this->terrain[leftIndex] == DIRTY_BLOCK ||
              this->terrain[leftIndex] == GRASS_BLOCK) {
            this->terrain[leftIndex] = SAND_BLOCK;
          }
          if (this->terrain[rightIndex] == DIRTY_BLOCK ||
              this->terrain[rightIndex] == GRASS_BLOCK) {
            this->terrain[rightIndex] = SAND_BLOCK;
          }
          if (this->terrain[frontIndex] == DIRTY_BLOCK ||
              this->terrain[frontIndex] == GRASS_BLOCK) {
            this->terrain[frontIndex] = SAND_BLOCK;
          }
          if (this->terrain[backIndex] == DIRTY_BLOCK ||
              this->terrain[backIndex] == GRASS_BLOCK) {
            this->terrain[backIndex] = SAND_BLOCK;
          }
        };

        index++;
      }
    }
  }
}
