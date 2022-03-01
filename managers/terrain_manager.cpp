#include "terrain_manager.hpp"

TerrainManager::TerrainManager()
{
}

TerrainManager::~TerrainManager() {}

void TerrainManager::init(Engine *t_engine)
{
    engine = t_engine;
    texRepo = t_engine->renderer->getTextureRepository();
    this->chunck = new Chunck(engine);
    this->loadBlocks();
    this->generateNewTerrain(WORLD_SIZE);
    this->optimizeTerrain();
}

void TerrainManager::generateNewTerrain(int terrainLength)
{
    PRINT_LOG("Generating terrain");
    int blockIndex = 0;

    PRINT_LOG("Adding Blocks Textures");
    for (int x = 0; x < terrainLength; x++)
    {
        for (int y = 0; y < terrainLength; y++)
        {
            for (int z = 0; z < terrainLength; z++)
            {
                int blockType = y < 3 ? AIR_BLOCK : DIRTY_BLOCK;
                terrain[blockIndex].init(blockType, x * BLOCK_SIZE * 2, y * -(BLOCK_SIZE * 2), z * -(BLOCK_SIZE * 2));
                terrain[blockIndex].mesh.loadFrom(getMeshByBlockType(blockType));
                terrain[blockIndex].mesh.shouldBeFrustumCulled = true;
                terrain[blockIndex].mesh.shouldBeBackfaceCulled = false;
                linkTextureByBlockType(blockType, terrain[blockIndex].mesh.getMaterial(0).getId());
                terrain[blockIndex].xIndex = x;
                terrain[blockIndex].yIndex = y;
                terrain[blockIndex].zIndex = z;
                blockIndex++;
            }
        }
    }
}

void TerrainManager::optimizeTerrain()
{
    printf("Optimizing Terrain\n");
    for (int i = 0; i <= WORLD_SIZE * WORLD_SIZE * WORLD_SIZE; i++)
    {
        // If some nighbor block is AIR_BLOCK set block to visible
        if (
            // Front block
            terrain[i].zIndex < WORLD_SIZE - 1 && terrain[i + 1].blockType == AIR_BLOCK ||
            // Back block
            terrain[i].zIndex > 0 && terrain[i - 1].blockType == AIR_BLOCK ||
            // Down block
            terrain[i].yIndex < WORLD_SIZE - 1 && terrain[i + WORLD_SIZE].blockType == AIR_BLOCK ||
            // Up block
            terrain[i].yIndex > 0 && terrain[i - WORLD_SIZE].blockType == AIR_BLOCK ||
            // Right block
            terrain[i].xIndex < WORLD_SIZE - 1 && terrain[i + WORLD_SIZE * WORLD_SIZE].blockType == AIR_BLOCK ||
            // Left block
            terrain[i].xIndex > 0 && terrain[i - WORLD_SIZE * WORLD_SIZE].blockType == AIR_BLOCK)
        {
            terrain[i].isHidden = false;
        }
    }
}

Block *TerrainManager::getBlockByIndex(int offsetX, int offsetY, int offsetZ)
{
    for (int i = 0; i < WORLD_SIZE * WORLD_SIZE * WORLD_SIZE; i++)
    {
        if (
            terrain[i].xIndex == offsetX &&
            terrain[i].yIndex == offsetY &&
            terrain[i].zIndex == offsetZ)
        {
            return &terrain[i];
        }
    }
}

void TerrainManager::updateChunkByPlayerPosition(Player *player)
{
    if (player->isWalking)
    {
        const Vector3 pos = player->getPosition();
        this->chunck->clear();
        this->buildChunk(
            pos.x / (BLOCK_SIZE * 2),
            pos.y / -(BLOCK_SIZE * 2),
            pos.z / -(BLOCK_SIZE * 2));
        printf("Updating Chunck: %f, %f, %f\n",
               pos.x / (BLOCK_SIZE * 2),
               pos.y / -(BLOCK_SIZE * 2),
               pos.z / -(BLOCK_SIZE * 2));
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
    for (int i = 0; i < WORLD_SIZE * WORLD_SIZE * WORLD_SIZE; i++)
    {
        if (
            // Check if block is inside chunck;
            terrain[i].shouldBeDrawn() &&
            (terrain[i].xIndex >= offsetX && terrain[i].xIndex < offsetX + CHUNCK_SIZE) &&
            (terrain[i].yIndex >= offsetY && terrain[i].yIndex < offsetY + CHUNCK_SIZE) &&
            (terrain[i].zIndex >= offsetZ && terrain[i].zIndex < offsetZ + CHUNCK_SIZE))
        {
            this->chunck->add(&terrain[i]);
        }
    }
}

void TerrainManager::update()
{
    return;
};

void TerrainManager::loadBlocks()
{
    dirtBlock.loadObj("meshes/cube/", "cube", BLOCK_SIZE, false);
    texRepo->addByMesh("meshes/cube/", dirtBlock, PNG);
}

void TerrainManager::linkTextureByBlockType(int blockType, const u32 t_meshId)
{
    texRepo->getBySpriteOrMesh(getMeshByBlockType(blockType).getMaterial(0).getId())->addLink(t_meshId);
}

Mesh &TerrainManager::getMeshByBlockType(int blockType)
{
    if (blockType == DIRTY_BLOCK)
        return dirtBlock;

    // Should return an invalid texture
    return dirtBlock;
}