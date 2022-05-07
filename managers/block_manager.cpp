#include "./block_manager.hpp"

BlockManager::BlockManager() {}
BlockManager::~BlockManager() {}

void BlockManager::init(TextureRepository *t_texRepo)
{
    texRepo = t_texRepo;
    this->loadBlocks();
}

void BlockManager::loadBlocks()
{
    char *MODELS_PATH = "meshes/block/";
    char *TEXTURES_PATH = "assets/textures/block/";

    // Load models:
    stoneBlock.loadObj(MODELS_PATH, "stone", BLOCK_SIZE, false);
    dirtBlock.loadObj(MODELS_PATH, "dirt", BLOCK_SIZE, false);
    waterBlock.loadObj(MODELS_PATH, "water", BLOCK_SIZE, false);
    grassBlock.loadObj(MODELS_PATH, "grass", BLOCK_SIZE, true);

    // Load model's Textures:
    texRepo->addByMesh(TEXTURES_PATH, stoneBlock, PNG);
    texRepo->addByMesh(TEXTURES_PATH, dirtBlock, PNG);
    texRepo->addByMesh(TEXTURES_PATH, waterBlock, PNG);
    texRepo->addByMesh(TEXTURES_PATH, grassBlock, PNG);
}

void BlockManager::linkTextureByBlockType(int blockType, const u32 t_meshId, u8 materialIndex)
{
    
    texRepo->getBySpriteOrMesh(getMeshByBlockType(blockType).getMaterial(materialIndex).getId())->addLink(t_meshId);
}

Mesh &BlockManager::getMeshByBlockType(int blockType)
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