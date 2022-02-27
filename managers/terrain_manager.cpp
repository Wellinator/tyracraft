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
                terrain[blockIndex].init(DIRTY_BLOCK, x * BLOCK_SIZE * 2, y * -(BLOCK_SIZE * 2), z * -(BLOCK_SIZE * 2));
                terrain[blockIndex].mesh.loadFrom(getMeshByBlockType(DIRTY_BLOCK));
                terrain[blockIndex].mesh.shouldBeFrustumCulled = false;
                terrain[blockIndex].mesh.shouldBeBackfaceCulled = false;
                linkTextureByBlockType(DIRTY_BLOCK, terrain[blockIndex].mesh.getMaterial(0).getId());
                terrain[blockIndex].isHidden = false;
                terrain[blockIndex].xIndex = x;
                terrain[blockIndex].yIndex = y;
                terrain[blockIndex].zIndex = z;
                blockIndex++;
            }
        }
    }
}

void TerrainManager::updateChunkByPlayerPosition(Player *player)
{
    player->getPosition();
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
            (terrain[i].xIndex >= offsetX && terrain[i].xIndex < CHUNCK_SIZE) &&
            (terrain[i].yIndex >= offsetY && terrain[i].yIndex < CHUNCK_SIZE) && 
            (terrain[i].zIndex >= offsetZ && terrain[i].zIndex < CHUNCK_SIZE))
        {
            printf("Adding block to chunck %d \n", i);
            this->chunck->add(&terrain[i]);
        }
    }
}

void TerrainManager::update(){
    // TODO: If player move, then update chunck by player position;
    // TODO: Render the chunck;
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