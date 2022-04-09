#include "terrain_manager.hpp"

TerrainManager::TerrainManager()
{
}

TerrainManager::~TerrainManager()
{
}

void TerrainManager::init(Engine *t_engine)
{
    engine = t_engine;
    texRepo = t_engine->renderer->getTextureRepository();

    int terrainType = 0;
    int testterrain = rand() % 10;
    if (testterrain < 4)
        terrainType = 0;
    if (testterrain >= 4 && testterrain < 7)
        terrainType = 1;
    if (testterrain >= 7)
        terrainType = 2;

    this->chunck = new Chunck(engine);
    this->blockManager->init(texRepo);
    this->generateNewTerrain(terrainType, false, false, false, false);
}

void TerrainManager::generateNewTerrain(int terrainType, bool makeFlat, bool makeTrees, bool makeWater, bool makeCaves)
{
    if (!makeFlat)
    {
        int index = 0;
        for (int z = OVERWORLD_MIN_DISTANCE; z < OVERWORLD_MAX_DISTANCE; z++)
        {
            for (int x = OVERWORLD_MIN_DISTANCE; x < OVERWORLD_MAX_DISTANCE; x++)
            {
                for (int y = OVERWORLD_MIN_HEIGH; y < OVERWORLD_MAX_HEIGH; y++)
                {
                    this->terrain[index] = this->getBlock(x, y, z);
                    index++;
                }
            }
        }
    }
}

int TerrainManager::getBlock(int x, int y, int z)
{
    int octaves = sqrt(OVERWORLD_H_DISTANCE * OVERWORLD_V_DISTANCE);

    double noiseLayer1 = simplex->fractal(octaves, x + seed, z + seed);
    double noiseLayer2 = simplex->fractal(octaves /= 2, x + seed, z + seed);
    double noiseLayer3 = simplex->fractal(octaves /= 2, x + seed, z + seed);
    double noise = floor((((noiseLayer1 + noiseLayer2 + noiseLayer3) / 3) * scale));

    if (y < noise)
    {
        return AIR_BLOCK;
    }
    if (y == noise)
    {
        return GRASS_BLOCK;
    };
    if (y > noise && y <= noise + 2)
    {
        return DIRTY_BLOCK;
    }
    if (y > noise + 2)
    {
        return STONE_BLOCK;
    }

    if (y > 0)
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
    return terrain[this->getIndexByPosition(x, y, z)];
}

unsigned int TerrainManager::getIndexByPosition(int x, int y, int z)
{
    int offsetZ = z + HALF_OVERWORLD_H_DISTANCE;
    int offsetX = x + HALF_OVERWORLD_H_DISTANCE;
    int offsetY = y + HALF_OVERWORLD_V_DISTANCE;

    int _z = offsetZ > 0 && z < HALF_OVERWORLD_H_DISTANCE ? offsetZ * OVERWORLD_H_DISTANCE * OVERWORLD_V_DISTANCE : 0;
    int _x = offsetX > 0 && x < HALF_OVERWORLD_H_DISTANCE ? offsetX * OVERWORLD_V_DISTANCE : 0;
    int _y = offsetY > 0 && y < HALF_OVERWORLD_V_DISTANCE ? offsetY : 0;

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
        this->lastPlayerPosition = player->getPosition();
        this->clearTempBlocks();
        this->chunck->clear();
        this->buildChunk(
            floor(player->getPosition().x / (BLOCK_SIZE * 2)),
            floor(player->getPosition().y / -(BLOCK_SIZE * 2)),
            floor(player->getPosition().z / -(BLOCK_SIZE * 2)));
        this->lastPlayerPosition = player->getPosition();
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
    for (int z = -HALF_CHUNCK_SIZE; z < HALF_CHUNCK_SIZE; z++)
    {
        for (int x = -HALF_CHUNCK_SIZE; x < HALF_CHUNCK_SIZE; x++)
        {
            for (int y = -HALF_CHUNCK_SIZE; y < HALF_CHUNCK_SIZE; y++)
            {
                if (
                    // Are block's coordinates in world range?
                    (offsetX + x >= OVERWORLD_MIN_DISTANCE && offsetX + x < OVERWORLD_MAX_DISTANCE) &&
                    (offsetY + y >= OVERWORLD_MIN_HEIGH && offsetY + y < OVERWORLD_MAX_HEIGH) &&
                    (offsetZ + z >= OVERWORLD_MIN_DISTANCE && offsetZ + z < OVERWORLD_MAX_DISTANCE))
                {
                    Vector3 *tempBlockPos = new Vector3(offsetX + x,
                                                        offsetY + y,
                                                        offsetZ + z);

                    int block_type = this->getBlockTypeByPosition(
                        tempBlockPos->x,
                        tempBlockPos->y,
                        tempBlockPos->z);

                    if (
                        block_type != AIR_BLOCK &&
                        this->isBlockVisible(offsetX + x, offsetY + y, offsetZ + z))
                    {
                        Block *tempBlock = new Block(block_type);

                        tempBlock->mesh.position.set((tempBlockPos->x) * BLOCK_SIZE * 2,
                                                     (tempBlockPos->y) * -(BLOCK_SIZE * 2),
                                                     (tempBlockPos->z) * -(BLOCK_SIZE * 2));
                        tempBlock->mesh.loadFrom(this->blockManager->getMeshByBlockType(block_type));
                        tempBlock->mesh.shouldBeFrustumCulled = true;
                        tempBlock->mesh.shouldBeBackfaceCulled = false;
                        this->blockManager->linkTextureByBlockType(block_type, tempBlock->mesh.getMaterial(0).getId());
                        this->chunck->add(tempBlock);
                        this->tempBlocks.push_back(tempBlock);
                    }

                    delete tempBlockPos;
                }
            }
        }
    }
}

/**
 * @brief Clear temp blocks ref before rebuild chunck to prevent memory leak;
 */
void TerrainManager::clearTempBlocks()
{
    for (size_t i = 0; i < this->tempBlocks.size(); i++)
    {
        delete this->tempBlocks[i];
    }
    this->tempBlocks.clear();
    this->tempBlocks.shrink_to_fit();
};

void TerrainManager::update(Player *t_player, Camera *t_camera)
{
    this->chunck->update(t_player);
    this->updateTargetBlock(t_player, t_camera);
};

void TerrainManager::updateTargetBlock(Player *t_player, Camera *t_camera)
{
    Vector3 currentBoxIntersection;
    Vector3 targetPos;
    Block *targetBlock;

    ray.set(
        t_camera->position,
        t_camera->lookPos - t_camera->position);

    for (u16 i = 0; i < this->tempBlocks.size(); i++)
    {
        this->tempBlocks[i]->mesh.getMaterial(0).color.g = 128;
        if (
            t_player->getPosition().distanceTo(this->tempBlocks[i]->mesh.position) <= MAX_RANGE_PICKER)
        {
            Vector3 min;
            Vector3 max;
            this->tempBlocks[i]->mesh.getMinMaxBoundingBox(&min, &max);

            currentBoxIntersection = ray.intersectBox(&min, &max);

            if (
                t_player->getPosition().distanceTo(currentBoxIntersection) <
                t_player->getPosition().distanceTo(targetPos))
            {
                targetPos.set(currentBoxIntersection);
                targetBlock = this->tempBlocks[i];
            }
        }
    }

    if (targetPos.length() > 0)
    {
        targetBlock->mesh.getMaterial(0).color.g = 200;
    }
};

void TerrainManager::removeBlock(Vector3 *position)
{
    terrain[this->getIndexByPosition(position->x, position->y, position->z)] = AIR_BLOCK;
    this->shouldUpdateChunck = 1;
}

void TerrainManager::putBlock(Vector3 *position, u8 &blockType)
{
    terrain[this->getIndexByPosition(position->x, position->y, position->z)] = AIR_BLOCK;
    this->shouldUpdateChunck = 1;
}
