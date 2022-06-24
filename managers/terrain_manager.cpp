#include "terrain_manager.hpp"

TerrainManager::TerrainManager()
{
    printf("\n\n|-----------SEED---------|");
    printf("\n|           %d         |\n", seed);
    printf("|------------------------|\n\n");

    this->minWorldPos.set(OVERWORLD_MIN_DISTANCE, OVERWORLD_MIN_HEIGH, OVERWORLD_MIN_DISTANCE);
    this->maxWorldPos.set(OVERWORLD_MAX_DISTANCE, OVERWORLD_MAX_HEIGH, OVERWORLD_MAX_DISTANCE);
}

TerrainManager::~TerrainManager()
{
}

void TerrainManager::init(TextureRepository *t_texRepo, ItemRepository *itemRepository)
{
    texRepo = t_texRepo;
    t_itemRepository = itemRepository;

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

void TerrainManager::update(Player *t_player, Camera *t_camera, const Pad &t_pad)
{
    this->t_player = t_player;
    this->t_camera = t_camera;
    this->updateChunkByPlayerPosition(t_player);
    this->getTargetBlock(t_player->getPosition(), t_camera);
    this->handlePadControls(t_pad);
    this->chunck->update(t_player);
};

void TerrainManager::generateNewTerrain(int terrainType, bool makeFlat, bool makeTrees, bool makeWater, bool makeCaves)
{
    int index = 0;
    int noise = 0;
    float density;

    for (int z = OVERWORLD_MIN_DISTANCE; z < OVERWORLD_MAX_DISTANCE; z++)
    {
        for (int x = OVERWORLD_MIN_DISTANCE; x < OVERWORLD_MAX_DISTANCE; x++)
        {
            noise = getNoise(x, z);
            for (int y = OVERWORLD_MIN_HEIGH; y < OVERWORLD_MAX_HEIGH; y++)
            {
                if (makeFlat)
                {
                    this->terrain[index] = y <= 0 ? GRASS_BLOCK : AIR_BLOCK;
                }
                else
                {
                    density = getDensity(x, y, z);
                    if (density <= 0)
                    {
                        this->terrain[index] = AIR_BLOCK;
                    }
                    else
                    {
                        this->terrain[index] = this->getBlock(noise, y);
                    }
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
    this->noise->SetFractalType(FastNoiseLite::FractalType_DomainWarpProgressive);
    this->noise->SetFractalOctaves(this->octaves);
    this->noise->SetFractalLacunarity(this->lacunarity);
    this->noise->SetFrequency(this->frequency);
}

int TerrainManager::getNoise(int x, int z)
{
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

u8 TerrainManager::getBlock(int noise, int y)
{
    // if (y <= noise)
    //     return STONE_BLOCK;

    // if (y < 0)
    //     return WATER_BLOCK;

    // return AIR_BLOCK;

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

float TerrainManager::getHeightScale(int x, int z)
{
    float continentalnes = this->getContinentalness(x, z);
    float erosion = this->getErosion(x, z);
    float pv = this->getPeaksAndValleys(x, z);

    float noise = continentalnes + pv - erosion;
    // Clamp between -1 and 1
    if (noise < -1.0f)
        noise = -1.0;
    if (noise > 1.0f)
        noise = 1.0;

    // Scale based on noise;
    if (noise <= -0.9f)
        return 30.0f;
    if (noise > -0.9f && noise <= -0.7f)
        return 2.0f;
    if (noise > -0.7f && noise <= -0.6f)
        return 9.0f;
    if (noise > -0.6f && noise <= -0.2f)
        return 11.0f;
    if (noise > -0.2f && noise <= -0.0f)
        return 12.0f;
    if (noise > 0.0f && noise <= 0.8f)
        return 20.0f;
    if (noise > 0.8f && noise <= 0.9f)
        return 25.0f;
    if (noise <= 1.0f)
        return 30.0f;
}

bool TerrainManager::isBlockHidden(int x, int y, int z)
{
    u8 upBlock = this->getBlockTypeByPosition(x, y - 1, z);
    u8 downBlock = this->getBlockTypeByPosition(x, y + 1, z);
    u8 frontBlock = this->getBlockTypeByPosition(x, y, z + 1);
    u8 backBlock = this->getBlockTypeByPosition(x, y, z - 1);
    u8 rightBlock = this->getBlockTypeByPosition(x + 1, y, z);
    u8 leftBlock = this->getBlockTypeByPosition(x - 1, y, z);

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
        leftBlock == AIR_BLOCK || leftBlock == GLASS_BLOCK)
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
    if (this->lastPlayerPosition == NULL)
        this->lastPlayerPosition = new Vector3();

    // Update chunck when player moves a quarter chunck
    if (this->lastPlayerPosition->distanceTo(player->getPosition()) > CHUNCK_DISTANCE / 3)
    {
        this->lastPlayerPosition->set(player->getPosition());
        this->buildChunk(floor(player->getPosition().x / DUBLE_BLOCK_SIZE),
                         floor(player->getPosition().y / DUBLE_BLOCK_SIZE),
                         floor(player->getPosition().z / DUBLE_BLOCK_SIZE));
    }

    if (shouldUpdateChunck)
    {
        this->buildChunk(
            floor(this->lastPlayerPosition->x / DUBLE_BLOCK_SIZE),
            floor(this->lastPlayerPosition->y / DUBLE_BLOCK_SIZE),
            floor(this->lastPlayerPosition->z / DUBLE_BLOCK_SIZE));
        this->shouldUpdateChunck = 0;
    }
}

Chunck *TerrainManager::getChunck(int offsetX, int offsetY, int offsetZ)
{
    this->chunck->clear();
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

                Vector3 blockPosition = (*tempBlockOffset * DUBLE_BLOCK_SIZE);

                int blockIndex = this->getIndexByOffset(tempBlockOffset->x,
                                                        tempBlockOffset->y,
                                                        tempBlockOffset->z);
                u8 block_type = terrain[blockIndex];
                u8 isHidden = this->isBlockHidden(tempBlockOffset->x,
                                                  tempBlockOffset->y,
                                                  tempBlockOffset->z);

                // Are block's coordinates in world range?
                if (tempBlockOffset->collidesBox(minWorldPos, maxWorldPos) && block_type != AIR_BLOCK && !isHidden)
                {

                    Block *block = new Block(block_type);
                    block->index = blockIndex;
                    block->position.set(blockPosition);
                    block->isHidden = isHidden;
                    block->mesh.position.set(blockPosition);
                    block->mesh.loadFrom(this->blockManager->getMeshByBlockType(block_type));
                    block->mesh.shouldBeFrustumCulled = true;
                    block->mesh.shouldBeLighted = false;
                    block->mesh.shouldBeBackfaceCulled = false;

                    if (block->mesh.getMaterialsCount() > 0)
                    {
                        for (u16 materialIndex = 0; materialIndex < block->mesh.getMaterialsCount(); materialIndex++)
                        {
                            this->blockManager->linkTextureByBlockType(block_type, block->mesh.getMaterial(materialIndex).getId(), materialIndex);
                        }
                    }

                    // this->chunck->meshes.push_back(&block->mesh);
                    this->chunck->addBlock(block);
                }

                delete tempBlockOffset;
            }
        }
    }
}

void TerrainManager::getTargetBlock(const Vector3 &playerPosition, Camera *t_camera)
{
    Vector3 minCorner;
    Vector3 maxCorner;
    Vector3 rayDir = t_camera->lookPos - t_camera->position;
    rayDir.normalize();
    ray.set(t_camera->position, rayDir);
    u8 hitedABlock = 0;
    float distance = -1.0f;
    float tempDistance;
    Block *tempTargetBlock;

    for (u16 blockIndex = 0; blockIndex < this->chunck->blocks.size(); blockIndex++)
    {
        if (playerPosition.distanceTo(this->chunck->blocks[blockIndex]->position) <= MAX_RANGE_PICKER)
        {
            // Reset block state
            this->chunck->blocks[blockIndex]->isTarget = 0;
            this->chunck->blocks[blockIndex]->distance = 0.0f;

            this->chunck->blocks[blockIndex]->mesh.getMinMaxBoundingBox(&minCorner, &maxCorner);
            if (ray.intersectBox(&minCorner, &maxCorner, distance))
            {
                hitedABlock = 1;
                if (distance == -1.0f ||
                    playerPosition.distanceTo(this->chunck->blocks[blockIndex]->position) < playerPosition.distanceTo(tempTargetBlock->position))
                {
                    tempTargetBlock = this->chunck->blocks[blockIndex];
                    tempDistance = distance;
                }
            }
        }
    }

    if (hitedABlock)
    {
        this->targetBlock = tempTargetBlock;
        this->targetBlock->isTarget = 1;
        this->targetBlock->distance = tempDistance;
    }
    else
    {
        this->targetBlock = NULL;
    }
}

void TerrainManager::removeBlock()
{
    if (this->targetBlock != NULL)
    {
        terrain[this->targetBlock->index] = AIR_BLOCK;
        this->shouldUpdateChunck = 1;
    }
}

void TerrainManager::putBlock(u8 blockToPlace)
{
    if (this->targetBlock != NULL)
    {
        // Detect face
        const BoundingBox *blockCenterPos = this->targetBlock->mesh.getCurrentBoundingBox();
        Vector3 hitPosition = ray.at(this->targetBlock->distance);
        Vector3 targetPos = this->targetBlock->position - hitPosition;
        int terrainIndex = this->targetBlock->index;

        if (std::round(targetPos.z) == blockCenterPos->getFrontFace().axisPosition)
        {
            terrainIndex -= OVERWORLD_H_DISTANCE * OVERWORLD_V_DISTANCE;
        }
        else if (std::round(targetPos.z) == blockCenterPos->getBackFace().axisPosition)
        {
            terrainIndex += OVERWORLD_H_DISTANCE * OVERWORLD_V_DISTANCE;
        }
        else if (std::round(targetPos.x) == blockCenterPos->getRightFace().axisPosition)
        {
            terrainIndex -= OVERWORLD_V_DISTANCE;
        }
        else if (std::round(targetPos.x) == blockCenterPos->getLeftFace().axisPosition)
        {
            terrainIndex += OVERWORLD_V_DISTANCE;
        }
        else if (std::round(targetPos.y) == blockCenterPos->getTopFace().axisPosition)
        {
            terrainIndex -= 1;
        }
        else if (std::round(targetPos.y) == blockCenterPos->getBottomFace().axisPosition)
        {
            terrainIndex += 1;
        }

        if (terrainIndex <= OVERWORLD_SIZE && terrainIndex != this->targetBlock->index)
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
        this->removeBlock();
    }
    if (t_pad.isR2Clicked)
    {
        ITEM_TYPES activeItemType = this->t_player->getSelectedInventoryItemType();
        if (activeItemType != ITEM_TYPES::empty)
        {
            int blockid = this->t_itemRepository->getItemById(activeItemType)->blockId;
            if (blockid != AIR_BLOCK)
                this->putBlock(blockid);
        }
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
    Vector3 spawPos = this->calcSpawOffset();
    printf("\nCalculated spawn area:\n");
    spawPos.print();
    worldSpawnArea.set(spawPos);
    spawnArea.set(spawPos);
}

const Vector3 TerrainManager::calcSpawOffset(int bias)
{
    u8 found = 0;
    u8 airBlockCounter = 0;
    // Pick a X and Z coordinates based on the seed;
    int posX = ((seed + bias) % HALF_OVERWORLD_H_DISTANCE) - HALF_OVERWORLD_H_DISTANCE;
    int posZ = ((seed - bias) % HALF_OVERWORLD_H_DISTANCE) - HALF_OVERWORLD_H_DISTANCE;
    Vector3 result;

    for (float posY = OVERWORLD_MAX_HEIGH; posY >= OVERWORLD_MIN_HEIGH; posY--)
    {
        int index = this->getIndexByOffset(posX, posY, posZ);
        u8 type = this->terrain[index];
        if (type != AIR_BLOCK && airBlockCounter >= 4)
        {
            found = 1;
            result = Vector3(posX, posY + 2, posZ);
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

float TerrainManager::getContinentalness(int x, int z)
{
    this->noise->SetFrequency(this->frequency);
    this->noise->SetFractalOctaves(1);

    return this->noise->GetNoise((float)(x + seed), (float)(z + seed));
}

float TerrainManager::getErosion(int x, int z)
{
    this->noise->SetFrequency(this->frequency);
    this->noise->SetFractalOctaves(2);

    return this->noise->GetNoise((float)(x + seed), (float)(z + seed));
}

float TerrainManager::getPeaksAndValleys(int x, int z)
{
    this->noise->SetFrequency(this->frequency);
    this->noise->SetFractalOctaves(1);

    return this->noise->GetNoise((float)(x + seed), (float)(z + seed));
}

float TerrainManager::getDensity(int x, int y, int z)
{
    this->noise->SetFractalType(FastNoiseLite::FractalType_DomainWarpProgressive);
    this->noise->SetFrequency(0.0001);
    this->noise->SetFractalLacunarity(1.5);
    this->noise->SetFractalOctaves(8);
    return this->noise->GetNoise((float)(x + seed), (float)(y + seed), (float)(z + seed));
}

float TerrainManager::getTemperature(int x, int z)
{
    this->noise->SetFrequency(this->frequency);
    this->noise->SetFractalOctaves(1);

    return this->noise->GetNoise((float)(x + seed), (float)(z + seed));
}

float TerrainManager::getHumidity(int x, int z)
{
    this->noise->SetFrequency(this->frequency);
    this->noise->SetFractalOctaves(1);

    return this->noise->GetNoise((float)(x + seed), (float)(z + seed));
}
