#include "terrain_manager.hpp"

TerrainManager::TerrainManager()
{
    printf("\n\n|-----------SEED---------|");
    printf("\n|           %d         |\n", seed);
    printf("|------------------------|\n\n");
}

TerrainManager::~TerrainManager()
{
}

void TerrainManager::init(TextureRepository *t_texRepo, Player *t_player)
{
    texRepo = t_texRepo;
    t_player = t_player;

    int terrainType = 0;
    int testterrain = rand() % 10;
    if (testterrain < 4)
        terrainType = 0;
    if (testterrain >= 4 && testterrain < 7)
        terrainType = 1;
    if (testterrain >= 7)
        terrainType = 2;

    this->initNoise();
    this->blockManager->init(texRepo);
    this->chunck = new Chunck(this->blockManager);
    this->generateNewTerrain(terrainType, false, false, false, false);
    this->defineSpawnArea();
}

void TerrainManager::generateNewTerrain(int terrainType, bool makeFlat, bool makeTrees, bool makeWater, bool makeCaves)
{
    int index = 0;
    for (int z = OVERWORLD_MIN_DISTANCE; z < OVERWORLD_MAX_DISTANCE; z++)
    {
        for (int x = OVERWORLD_MIN_DISTANCE; x < OVERWORLD_MAX_DISTANCE; x++)
        {
            for (int y = OVERWORLD_MIN_HEIGH; y < OVERWORLD_MAX_HEIGH; y++)
            {
                if (makeFlat)
                {
                    this->terrain[index] = y <= 0 ? GRASS_BLOCK : AIR_BLOCK;
                }
                else
                {
                    this->terrain[index] = this->getBlock(x, y, z);
                }
                index++;
            }
        }
    }
}

void TerrainManager::initNoise()
{
    // Fast Noise Lite
    this->noise = new FastNoiseLite(seed);
    this->noise->SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    this->noise->SetFractalType(FastNoiseLite::FractalType_None);
    this->noise->SetFractalOctaves(this->octaves);
    this->noise->SetFractalLacunarity(this->lacunarity);
    this->noise->SetFrequency(this->frequency);
}

u8 TerrainManager::getBlock(int x, int y, int z)
{
    float xNoiseOffset = (float)((x + seed));
    float zNoiseOffset = (float)((z + seed));
    this->noise->SetFractalOctaves(this->octaves);
    double noiseLayer1 = this->noise->GetNoise(xNoiseOffset, zNoiseOffset);

    xNoiseOffset += 10;
    zNoiseOffset += 10;
    this->noise->SetFractalOctaves(this->octaves * 2);
    double noiseLayer2 = this->noise->GetNoise(xNoiseOffset, zNoiseOffset);

    xNoiseOffset += 20;
    zNoiseOffset += 20;
    this->noise->SetFractalOctaves(this->octaves * 8);
    double noiseLayer3 = this->noise->GetNoise(xNoiseOffset, zNoiseOffset);

    double noise = floor((((noiseLayer1 + noiseLayer2 + noiseLayer3) / 3) * scale));

    if (y > noise)
    {
        return AIR_BLOCK;
    }
    if (y == noise)
    {
        return GRASS_BLOCK;
    };
    if (y >= noise - 2)
    {
        return DIRTY_BLOCK;
    }
    if (y < noise - 2)
    {
        return STONE_BLOCK;
    }

    if (y < 0)
    {
        return WATER_BLOCK;
    }

    return AIR_BLOCK;
}

bool TerrainManager::isBlockHidden(int x, int y, int z)
{
    // If some nighbor block is AIR_BLOCK set block to visible
    if (
        // Front block
        z < OVERWORLD_MAX_DISTANCE - 1 && this->getBlockTypeByPosition(x, y, z + 1) == AIR_BLOCK ||
        // Back block
        z > OVERWORLD_MIN_DISTANCE && this->getBlockTypeByPosition(x, y, z - 1) == AIR_BLOCK ||
        // Down block
        y < OVERWORLD_MAX_HEIGH - 1 && this->getBlockTypeByPosition(x, y + 1, z) == AIR_BLOCK ||
        // Up block
        y > OVERWORLD_MIN_HEIGH && this->getBlockTypeByPosition(x, y - 1, z) == AIR_BLOCK ||
        // Right block
        x < OVERWORLD_MAX_DISTANCE - 1 && this->getBlockTypeByPosition(x + 1, y, z) == AIR_BLOCK ||
        // Left block
        x > OVERWORLD_MIN_DISTANCE && this->getBlockTypeByPosition(x - 1, y, z) == AIR_BLOCK)
    {
        return false;
    }
    return true;
}

int TerrainManager::getBlockTypeByPosition(int x, int y, int z)
{
    return terrain[this->getIndexByOffset(x, y, z)];
}

unsigned int TerrainManager::getIndexByPosition(Vector3 *normalizedWorldPosition)
{
    int offsetZ = (normalizedWorldPosition->z / DUBLE_BLOCK_SIZE) + HALF_OVERWORLD_H_DISTANCE;
    int offsetX = (normalizedWorldPosition->x / DUBLE_BLOCK_SIZE) + HALF_OVERWORLD_H_DISTANCE;
    int offsetY = (normalizedWorldPosition->y / DUBLE_BLOCK_SIZE) + HALF_OVERWORLD_V_DISTANCE;

    int _z = offsetZ >= 0 && offsetZ < OVERWORLD_H_DISTANCE ? offsetZ * OVERWORLD_H_DISTANCE * OVERWORLD_V_DISTANCE : 0;
    int _x = offsetX >= 0 && offsetX < OVERWORLD_H_DISTANCE ? offsetX * OVERWORLD_V_DISTANCE : 0;
    int _y = offsetY >= 0 && offsetY < OVERWORLD_V_DISTANCE ? offsetY : 0;

    return _z + _x + _y;
}

unsigned int TerrainManager::getIndexByOffset(int x, int y, int z)
{
    int offsetZ = z + HALF_OVERWORLD_H_DISTANCE;
    int offsetX = x + HALF_OVERWORLD_H_DISTANCE;
    int offsetY = y + HALF_OVERWORLD_V_DISTANCE;

    int _z = offsetZ > 0 && z < OVERWORLD_H_DISTANCE ? offsetZ * OVERWORLD_H_DISTANCE * OVERWORLD_V_DISTANCE : 0;
    int _x = offsetX > 0 && x < OVERWORLD_H_DISTANCE ? offsetX * OVERWORLD_V_DISTANCE : 0;
    int _y = offsetY > 0 && y < OVERWORLD_V_DISTANCE ? offsetY : 0;

    return _z + _x + _y;
}

Vector3 *TerrainManager::getPositionByIndex(unsigned int index)
{
    Vector3 *pos = new Vector3();
    int mod = index;
    int z = OVERWORLD_MIN_DISTANCE;
    int x = OVERWORLD_MIN_DISTANCE;
    int y = OVERWORLD_MIN_HEIGH;

    if (mod >= OVERWORLD_H_DISTANCE * OVERWORLD_V_DISTANCE)
    {
        z = floor(mod / (OVERWORLD_H_DISTANCE * OVERWORLD_V_DISTANCE)) - HALF_OVERWORLD_H_DISTANCE;
        mod = mod % (OVERWORLD_H_DISTANCE * OVERWORLD_V_DISTANCE);
    }

    if (mod >= OVERWORLD_V_DISTANCE)
    {
        x = floor(mod / OVERWORLD_V_DISTANCE) - HALF_OVERWORLD_H_DISTANCE;
        mod = mod % OVERWORLD_H_DISTANCE;
    }

    if (mod < OVERWORLD_V_DISTANCE)
    {
        y = mod - HALF_OVERWORLD_V_DISTANCE;
    }

    pos->set(x, y, z);
    return pos;
}

void TerrainManager::updateChunkByPlayerPosition(Player *player)
{
    // Update chunck when player moves a quarter chunck
    if (this->lastPlayerPosition.distanceTo(player->getPosition()) > BLOCK_SIZE * CHUNCK_SIZE / 4)
    {
        this->lastPlayerPosition.set(player->getPosition());
        this->buildChunk(
            floor(player->getPosition().x / DUBLE_BLOCK_SIZE),
            floor(player->getPosition().y / DUBLE_BLOCK_SIZE),
            floor(player->getPosition().z / DUBLE_BLOCK_SIZE));
        this->lastPlayerPosition = player->getPosition();
    }

    if (shouldUpdateChunck)
    {
        this->shouldUpdateChunck = 0;
        this->buildChunk(
            floor(this->lastPlayerPosition.x / DUBLE_BLOCK_SIZE),
            floor(this->lastPlayerPosition.y / DUBLE_BLOCK_SIZE),
            floor(this->lastPlayerPosition.z / DUBLE_BLOCK_SIZE));
    }
}

Chunck *TerrainManager::getChunck(int offsetX, int offsetY, int offsetZ)
{
    this->buildChunk(offsetX, offsetY, offsetZ);
    return this->chunck;
}

void TerrainManager::buildChunk(int offsetX, int offsetY, int offsetZ)
{
    this->chunck->clear();
    for (int z = -HALF_CHUNCK_SIZE; z < HALF_CHUNCK_SIZE; z++)
    {
        for (int x = -HALF_CHUNCK_SIZE; x < HALF_CHUNCK_SIZE; x++)
        {
            for (int y = -HALF_CHUNCK_SIZE; y < HALF_CHUNCK_SIZE; y++)
            {
                Vector3 *tempBlockOffset = new Vector3(offsetX + x,
                                                       offsetY + y,
                                                       offsetZ + z);
                if (
                    // Are block's coordinates in world range?
                    (tempBlockOffset->x >= OVERWORLD_MIN_DISTANCE && tempBlockOffset->x < OVERWORLD_MAX_DISTANCE) &&
                    (tempBlockOffset->y >= OVERWORLD_MIN_HEIGH && tempBlockOffset->y < OVERWORLD_MAX_HEIGH) &&
                    (tempBlockOffset->z >= OVERWORLD_MIN_DISTANCE && tempBlockOffset->z < OVERWORLD_MAX_DISTANCE))
                {
                    int blockIndex = this->getIndexByOffset(tempBlockOffset->x,
                                                            tempBlockOffset->y,
                                                            tempBlockOffset->z);
                    u8 block_type = terrain[blockIndex];

                    Block *block = new Block(block_type);
                    block->index = blockIndex;
                    block->position.set((tempBlockOffset->x) * DUBLE_BLOCK_SIZE,
                                        (tempBlockOffset->y) * DUBLE_BLOCK_SIZE,
                                        (tempBlockOffset->z) * DUBLE_BLOCK_SIZE);

                    block->isHidden = this->isBlockHidden(tempBlockOffset->x,
                                                          tempBlockOffset->y,
                                                          tempBlockOffset->z);

                    if (block_type != AIR_BLOCK && !block->isHidden)
                    {
                        block->mesh.position.set((tempBlockOffset->x) * DUBLE_BLOCK_SIZE,
                                                 (tempBlockOffset->y) * DUBLE_BLOCK_SIZE,
                                                 (tempBlockOffset->z) * DUBLE_BLOCK_SIZE);

                        block->mesh.loadFrom(this->blockManager->getMeshByBlockType(block_type));
                        block->mesh.shouldBeFrustumCulled = true;
                        block->mesh.shouldBeLighted = true;
                        block->mesh.shouldBeBackfaceCulled = false;

                        if (block->mesh.getMaterialsCount() > 0)
                        {
                            for (u16 materialIndex = 0; materialIndex < block->mesh.getMaterialsCount(); materialIndex++)
                            {
                                this->blockManager->linkTextureByBlockType(block_type, block->mesh.getMaterial(materialIndex).getId(), materialIndex);
                            }
                        }
                    }

                    this->chunck->addBlock(block);
                    delete tempBlockOffset;
                }
            }
        }
    }
}

void TerrainManager::update(Player *t_player, Camera *t_camera, const Pad &t_pad)
{
    this->t_player = t_player;
    this->t_camera = t_camera;
    this->handlePadControls(t_pad);
    this->updateChunkByPlayerPosition(t_player);
    this->chunck->update(t_player);
};

void TerrainManager::removeBlock(Player *t_player, Camera *t_camera)
{
    Vector3 playerPosition = t_player->getPosition();
    Vector3 minCorner;
    Vector3 maxCorner;
    Block *targetBlock;
    Vector3 rayDir = t_camera->lookPos - t_camera->position;
    rayDir.normalize();
    ray.set(t_camera->position, rayDir);
    u8 hitedABlock = 0;
    float distance = -1.0f;

    for (u16 blockIndex = 0; blockIndex < this->chunck->blocks.size(); blockIndex++)
    {
        if (this->chunck->blocks[blockIndex]->type != AIR_BLOCK &&
            !this->chunck->blocks[blockIndex]->isHidden &&
            playerPosition.distanceTo(this->chunck->blocks[blockIndex]->position) <= MAX_RANGE_PICKER)
        {
            this->chunck->blocks[blockIndex]->mesh.getMinMaxBoundingBox(&minCorner, &maxCorner);
            if (ray.intersectBox(&minCorner, &maxCorner, distance))
            {
                hitedABlock = 1;
                if (distance == -1.0f ||
                    playerPosition.distanceTo(this->chunck->blocks[blockIndex]->position) < playerPosition.distanceTo(targetBlock->position))
                {
                    targetBlock = this->chunck->blocks[blockIndex];
                }
            }
        }
    }

    if (hitedABlock)
    {
        printf("Removed index %d\n", targetBlock->index);
        terrain[targetBlock->index] = AIR_BLOCK;
        this->shouldUpdateChunck = 1;
    }
}

void TerrainManager::putBlock(Player *t_player, Camera *t_camera, u8 blockToPlace)
{
    Vector3 playerPosition = t_player->getPosition();
    Vector3 minCorner;
    Vector3 maxCorner;
    Vector3 rayDir = t_camera->lookPos - t_camera->position;
    rayDir.normalize();
    ray.set(t_camera->position, rayDir);
    u8 hitedABlock = 0;
    Block *targetBlock;
    float targetDistance;
    float distance = -1.0f;

    for (u16 blockIndex = 0; blockIndex < this->chunck->blocks.size(); blockIndex++)
    {
        if (this->chunck->blocks[blockIndex]->type != AIR_BLOCK &&
            !this->chunck->blocks[blockIndex]->isHidden &&
            playerPosition.distanceTo(this->chunck->blocks[blockIndex]->position) <= MAX_RANGE_PICKER)
        {
            this->chunck->blocks[blockIndex]->mesh.getMinMaxBoundingBox(&minCorner, &maxCorner);
            if (ray.intersectBox(&minCorner, &maxCorner, distance))
            {
                hitedABlock = 1;
                if (distance == -1.0f ||
                    playerPosition.distanceTo(this->chunck->blocks[blockIndex]->position) < playerPosition.distanceTo(targetBlock->position))
                {
                    targetBlock = this->chunck->blocks[blockIndex];
                    targetDistance = distance;
                }
            }
        }
    }

    if (hitedABlock)
    {
        // Detect face
        const BoundingBox *blockCenterPos = targetBlock->mesh.getCurrentBoundingBox();
        Vector3 hitPosition = ray.at(targetDistance);
        Vector3 targetPos = targetBlock->position - hitPosition;
        unsigned int terrainIndex = targetBlock->index;

        if (std::round(targetPos.z) == blockCenterPos->getFrontFace().axisPosition)
        {
            terrainIndex -= OVERWORLD_H_DISTANCE * OVERWORLD_V_DISTANCE;
        }
        if (std::round(targetPos.z) == blockCenterPos->getBackFace().axisPosition)
        {
            terrainIndex += OVERWORLD_H_DISTANCE * OVERWORLD_V_DISTANCE;
        }
        if (std::round(targetPos.x) == blockCenterPos->getRightFace().axisPosition)
        {
            terrainIndex -= OVERWORLD_V_DISTANCE;
        }
        if (std::round(targetPos.x) == blockCenterPos->getLeftFace().axisPosition)
        {
            terrainIndex += OVERWORLD_V_DISTANCE;
        }
        if (std::round(targetPos.y) == blockCenterPos->getTopFace().axisPosition)
        {
            terrainIndex -= 1;
        }
        if (std::round(targetPos.y) == blockCenterPos->getBottomFace().axisPosition)
        {
            terrainIndex += 1;
        }

        if (terrainIndex <= OVERWORLD_SIZE)
        {
            terrain[terrainIndex] = blockToPlace;
            this->shouldUpdateChunck = 1;
        }
    }
}

void TerrainManager::handlePadControls(const Pad &t_pad)
{
    if (t_pad.isL2Clicked)
    {
        this->removeBlock(this->t_player, this->t_camera);
    }
    if (t_pad.isR2Clicked)
    {
        this->putBlock(this->t_player, this->t_camera, STONE_BLOCK);
    }
}

Vector3 *TerrainManager::normalizeWorldBlockPosition(Vector3 *worldPosition)
{
    return new Vector3((worldPosition->x == 0 ? 0 : std::trunc((worldPosition->x + DUBLE_BLOCK_SIZE) / DUBLE_BLOCK_SIZE) * DUBLE_BLOCK_SIZE),
                       (worldPosition->y == 0 ? 0 : std::trunc((worldPosition->y + DUBLE_BLOCK_SIZE) / DUBLE_BLOCK_SIZE) * DUBLE_BLOCK_SIZE),
                       (worldPosition->z == 0 ? 0 : std::trunc((worldPosition->z + DUBLE_BLOCK_SIZE) / DUBLE_BLOCK_SIZE) * DUBLE_BLOCK_SIZE));
}

void TerrainManager::defineSpawnArea()
{
    // Pick a X and Z coordinates based on the seed;
    float offsetX;
    float offsetZ;
    Vector3 spawPos = this->calcSpawOffset();
    this->worldSpawnArea.set(spawPos);
    this->t_player->setSpawnArea(this->worldSpawnArea);
}

Vector3 TerrainManager::calcSpawOffset(int bias)
{
    u8 found = 0;
    u8 airBlockCounter = 0;
    int posX = ((seed + bias) % HALF_OVERWORLD_H_DISTANCE) - HALF_OVERWORLD_H_DISTANCE;
    int posZ = ((seed - bias) % HALF_OVERWORLD_H_DISTANCE) - HALF_OVERWORLD_H_DISTANCE;
    Vector3 result;

    for (float posY = OVERWORLD_MAX_HEIGH; posY >= OVERWORLD_MIN_HEIGH; posY--)
    {
        int index = this->getIndexByOffset(posX, posY, posZ);
        u8 type = this->terrain[index];
        if (type == GRASS_BLOCK && airBlockCounter >= 2)
        {
            found = 1;
            result = Vector3(posX, posY + 32.0f, posZ);
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
