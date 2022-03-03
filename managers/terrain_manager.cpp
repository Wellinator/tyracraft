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
    this->generateNewTerrain(WORLD_SIZE, terrainType, false, false, false, false);
    this->optimizeTerrain();
}

void TerrainManager::generateNewTerrain(int terrainLength, int terrainType, bool makeFlat, bool makeTrees, bool makeWater, bool makeCaves)
{
    PRINT_LOG("Generating terrain");

    if (!makeFlat)
    {
        PRINT_LOG("Adding Blocks Textures");
        int blockIndex = 0;
        for (int z = -(terrainLength / 2); z < terrainLength / 2; z++)
        {
            for (int x = -(terrainLength / 2); x < terrainLength / 2; x++)
            {
                for (int y = -(terrainLength / 2); y < terrainLength / 2; y++)
                {
                    int blockType = this->getBlock(x, y, z);
                    terrain[blockIndex].init(blockType, x, y, z);
                    blockIndex++;
                }
            }
        }
    }
}

int TerrainManager::getBlock(int x, int y, int z)
{
    int octaves = 1;
    const int scale = 3;

    double noiseLayer1 = simplex->fractal(octaves, x, z);
    double noiseLayer2 = simplex->fractal(octaves += 5, x, z);
    double noiseLayer3 = simplex->fractal(octaves += 10, x, z);
    double noise = floor((((noiseLayer1 + noiseLayer2 + noiseLayer3) / 3) * scale));
    printf("Noise n: %f\n", noise);

    if (y == noise)
    {
        printf("Seting block GRASS_BLOCK\n");
        return GRASS_BLOCK;
    };
    if (y > noise && y <= noise + 2)
    {
        printf("Seting block DIRTY_BLOCK\n");
        return DIRTY_BLOCK;
    }
    if (y > noise + 2)
    {
        printf("Seting block STONE_BLOCK\n");
        return STONE_BLOCK;
    }
    printf("Seting block AIR_BLOCK\n");
    return AIR_BLOCK;
}

void TerrainManager::optimizeTerrain()
{
    printf("Optimizing Terrain\n");
    for (int i = 0; i <= WORLD_SIZE * WORLD_SIZE * WORLD_SIZE; i++)
    {
        // If some nighbor block is AIR_BLOCK set block to visible
        if (
            // Front block
            terrain[i].position.z < WORLD_SIZE - 1 && terrain[i + 1].blockType == AIR_BLOCK ||
            // Back block
            terrain[i].position.z > 0 && terrain[i - 1].blockType == AIR_BLOCK ||
            // Down block
            terrain[i].position.y < WORLD_SIZE - 1 && terrain[i + WORLD_SIZE].blockType == AIR_BLOCK ||
            // Up block
            terrain[i].position.y > 0 && terrain[i - WORLD_SIZE].blockType == AIR_BLOCK ||
            // Right block
            terrain[i].position.x < WORLD_SIZE - 1 && terrain[i + WORLD_SIZE * WORLD_SIZE].blockType == AIR_BLOCK ||
            // Left block
            terrain[i].position.x > 0 && terrain[i - WORLD_SIZE * WORLD_SIZE].blockType == AIR_BLOCK)
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
            terrain[i].position.x == offsetX &&
            terrain[i].position.y == offsetY &&
            terrain[i].position.z == offsetZ)
        {
            return &terrain[i];
        }
    }
}

void TerrainManager::updateChunkByPlayerPosition(Player *player)
{
    // Update chunck when player moves three blocks
    if (this->lastPlayerPosition.distanceTo(player->getPosition()) > BLOCK_SIZE * 3)
    {
        printf("Player moved %f\n", this->lastPlayerPosition.distanceTo(player->getPosition()));
        this->lastPlayerPosition = player->getPosition();
        this->chunck->clear();
        this->buildChunk(
            player->getPosition().x / (BLOCK_SIZE * 2),
            player->getPosition().y / -(BLOCK_SIZE * 2),
            player->getPosition().z / -(BLOCK_SIZE * 2));
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
    const int halfChunk = CHUNCK_SIZE / 2;
    for (int i = 0; i < WORLD_SIZE * WORLD_SIZE * WORLD_SIZE; i++)
    {
        if (
            // Check if block is inside chunck;
            terrain[i].shouldBeDrawn() &&
            (terrain[i].position.x >= offsetX - halfChunk && terrain[i].position.x < offsetX + halfChunk) &&
            (terrain[i].position.y >= offsetY - halfChunk && terrain[i].position.y < offsetY + halfChunk) &&
            (terrain[i].position.z >= offsetZ - halfChunk && terrain[i].position.z < offsetZ + halfChunk))
        {
            Node *tempNode = new Node();
            tempNode->mesh.position.set(
                terrain[i].position.x * BLOCK_SIZE * 2,
                terrain[i].position.y * -(BLOCK_SIZE * 2),
                terrain[i].position.z * -(BLOCK_SIZE * 2));
            tempNode->mesh.loadFrom(getMeshByBlockType(terrain[i].blockType));
            tempNode->mesh.shouldBeFrustumCulled = true;
            tempNode->mesh.shouldBeBackfaceCulled = false;
            linkTextureByBlockType(terrain[i].blockType, tempNode->mesh.getMaterial(0).getId());
            this->chunck->add(tempNode);
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

    // Load model's Textures:
    texRepo->addByMesh(TEXTURES_PATH, stoneBlock, PNG);
    texRepo->addByMesh(TEXTURES_PATH, dirtBlock, PNG);
    texRepo->addByMesh(TEXTURES_PATH, grassBlock, PNG);
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
    }
}