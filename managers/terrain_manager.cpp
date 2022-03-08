#include "terrain_manager.hpp"

TerrainManager::TerrainManager()
{
}

TerrainManager::~TerrainManager() {}

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
    this->loadBlocks();
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
    int octaves = 1;
    const int scale = 5;

    double noiseLayer1 = simplex->fractal(octaves, x * this->seed, z * this->seed);
    double noiseLayer2 = simplex->fractal(octaves += 5, x * this->seed, z * this->seed);
    double noiseLayer3 = simplex->fractal(octaves += 10, x * this->seed, z * this->seed);
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
    // Update chunck when player moves three blocks
    if (this->lastPlayerPosition.distanceTo(player->getPosition()) > BLOCK_SIZE * 3)
    {
        printf("(Re)Building chunck...\n");
        this->lastPlayerPosition = player->getPosition();
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
                    // Is chunck node coordinates in world range?
                    (offsetX + x >= OVERWORLD_MIN_DISTANCE && offsetX + x < OVERWORLD_MAX_DISTANCE) &&
                    (offsetY + y >= OVERWORLD_MIN_HEIGH && offsetY + y < OVERWORLD_MAX_HEIGH) &&
                    (offsetZ + z >= OVERWORLD_MIN_DISTANCE && offsetZ + z < OVERWORLD_MAX_DISTANCE))
                {
                    int block_type = this->getBlockTypeByPosition(
                        offsetX + x,
                        offsetY + y,
                        offsetZ + z);

                    if (
                        block_type != AIR_BLOCK &&
                        this->isBlockVisible(offsetX + x, offsetY + y, offsetZ + z))
                    {
                        Node *tempNode = new Node();
                        tempNode->mesh.position.set(
                            (offsetX + x) * BLOCK_SIZE * 2,
                            (offsetY + y) * -(BLOCK_SIZE * 2),
                            (offsetZ + z) * -(BLOCK_SIZE * 2));
                        tempNode->mesh.loadFrom(getMeshByBlockType(block_type));
                        tempNode->mesh.shouldBeFrustumCulled = true;
                        tempNode->mesh.shouldBeBackfaceCulled = false;
                        linkTextureByBlockType(block_type, tempNode->mesh.getMaterial(0).getId());
                        this->chunck->add(tempNode);
                    }
                }
            }
        }
    }
}

void TerrainManager::update(){};

void TerrainManager::loadBlocks()
{
    char *MODELS_PATH = "meshes/block/";
    char *TEXTURES_PATH = "assets/textures/block/";

    // Load models:
    stoneBlock.loadObj(MODELS_PATH, "stone", BLOCK_SIZE, false);
    dirtBlock.loadObj(MODELS_PATH, "dirt", BLOCK_SIZE, false);
    grassBlock.loadObj(MODELS_PATH, "grass", BLOCK_SIZE, false);
    waterBlock.loadObj(MODELS_PATH, "water", BLOCK_SIZE, false);

    // Load model's Textures:
    texRepo->addByMesh(TEXTURES_PATH, stoneBlock, PNG);
    texRepo->addByMesh(TEXTURES_PATH, dirtBlock, PNG);
    texRepo->addByMesh(TEXTURES_PATH, grassBlock, PNG);
    texRepo->addByMesh(TEXTURES_PATH, waterBlock, PNG);
}

void TerrainManager::linkTextureByBlockType(int blockType, const u32 t_meshId)
{
    texRepo->getBySpriteOrMesh(getMeshByBlockType(blockType).getMaterial(0).getId())->addLink(t_meshId);
}

Mesh &TerrainManager::getMeshByBlockType(int blockType)
{
    switch (blockType)
    {
    case DIRTY_BLOCK:
        return dirtBlock;
    case STONE_BLOCK:
        return stoneBlock;
    case GRASS_BLOCK:
        return grassBlock;
    case WATER_BLOCK:
        return waterBlock;
    }
}