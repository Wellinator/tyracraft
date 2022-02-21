#include "terrain_manager.hpp"

TerrainManager::TerrainManager()
{
    this->chunck = new Chunck();
}

TerrainManager::~TerrainManager() {}

void TerrainManager::init(TextureRepository *t_repository)
{
    this->texRepo = t_repository;
    this->blockManager = new BlockManager(t_repository);
    this->generateNewTerrain(WORLD_SIZE);
    this->initChunk();
}

void TerrainManager::generateNewTerrain(int terrainLength)
{
    PRINT_LOG("Generating terrain");
    int blockIndex = 1;

    PRINT_LOG("Loading Blocks");
    terrain[0].loadObj("meshes/cube/", "cube", BLOCK_SIZE, false);
    terrain[0].shouldBeBackfaceCulled = false;
    terrain[0].shouldBeFrustumCulled = false;
    terrain[0].position.set(0, 0, 0);
    texRepo->addByMesh("meshes/cube/", terrain[0], PNG);

    PRINT_LOG("Adding Blocks Textures");
    for (int x = 0; x < terrainLength; x++)
    {
        for (int y = 0; y < terrainLength; y++)
        {
            for (int z = 0; z < terrainLength; z++)
            {

                terrain[blockIndex].loadFrom(terrain[0]);
                terrain[blockIndex].shouldBeFrustumCulled = false;
                terrain[blockIndex].shouldBeBackfaceCulled = false;
                texRepo->getBySpriteOrMesh(terrain[0].getId())->addLink(terrain[blockIndex].getId());
                terrain[0].position.set(x * BLOCK_SIZE * 2,
                    y * -(BLOCK_SIZE * 2),
                    z * -(BLOCK_SIZE * 2));
                // // terrain[blockIndex].init(
                // //     DIRTY_BLOCK,
                // //     x * BLOCK_SIZE * 2,
                // //     y * -(BLOCK_SIZE * 2),
                // //     z * -(BLOCK_SIZE * 2));

                // terrain[blockIndex].isHidden = false;
                // terrain[blockIndex].xIndex = x;
                // terrain[blockIndex].yIndex = y;
                // terrain[blockIndex].zIndex = z;

                // bool const isDataLoaded = terrain[blockIndex].isDataLoaded();
                // bool const isMother = terrain[blockIndex].isMother();
                // printf("Is data loaded: %d | Is Mother %d\n", isDataLoaded, isMother);
                printf("Primary mesh ID: %d;\nCurrent mesh ID: %d\n", terrain[0].getMaterial(0).getId(), terrain[blockIndex].getMaterial(0).getId());

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
    return this->chunck;
}

void TerrainManager::initChunk()
{
    for (size_t index = 0; index < WORLD_SIZE * WORLD_SIZE * WORLD_SIZE; index++)
    {
        if (terrain[blockIndex].xIndex == index || terrain[blockIndex].yIndex == index || terrain[blockIndex].zIndex == index)
        {
            this->chunck->pushBlock(terrain[blockIndex]);
        }
    }
}

void TerrainManager::render(Engine *t_engine)
{
    for (size_t index = 0; index < WORLD_SIZE * WORLD_SIZE * WORLD_SIZE; index++)
    {
        if (terrain[blockIndex].xIndex == index || terrain[blockIndex].yIndex == index || terrain[blockIndex].zIndex == index)
        {
            t_engine->renderer->draw(terrain[blockIndex]);
        }
    }
};