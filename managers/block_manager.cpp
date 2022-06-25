#include "./block_manager.hpp"

BlockManager::BlockManager() {}

BlockManager::~BlockManager() 
{
    for (u8 i = 0; i < blockItems.size(); i++)
    {
        delete blockItems[i];
        blockItems[i] = NULL;
    }
    this->blockItems.clear();
}

void BlockManager::init(TextureRepository *t_texRepo)
{
    texRepo = t_texRepo;
    this->loadBlocks();
    this->pushItems();
}

void BlockManager::loadBlocks()
{
    char *MODELS_PATH = "meshes/block/";
    char *TEXTURES_PATH = "assets/textures/block/";

    // Load models:
    // Blocks
    stoneBlock.loadObj(MODELS_PATH, "stone", BLOCK_SIZE, true);
    grassBlock.loadObj(MODELS_PATH, "grass", BLOCK_SIZE, true);
    dirtBlock.loadObj(MODELS_PATH, "dirt", BLOCK_SIZE, true);
    waterBlock.loadObj(MODELS_PATH, "water", BLOCK_SIZE, true);
    bedrockBlock.loadObj(MODELS_PATH, "bedrock", BLOCK_SIZE, true);
    sandBlock.loadObj(MODELS_PATH, "sand", BLOCK_SIZE, true);
    glassBlock.loadObj(MODELS_PATH, "glass", BLOCK_SIZE, true);
    bricksBlock.loadObj(MODELS_PATH, "bricks", BLOCK_SIZE, true);

    // Ores and Minerals
    goldOreBlock.loadObj(MODELS_PATH, "gold_ore", BLOCK_SIZE, true);
    ironOreBlock.loadObj(MODELS_PATH, "iron_ore", BLOCK_SIZE, true);
    coalOreBlock.loadObj(MODELS_PATH, "coal_ore", BLOCK_SIZE, true);
    diamondOreBlock.loadObj(MODELS_PATH, "diamond_ore", BLOCK_SIZE, true);
    redsoneOreBlock.loadObj(MODELS_PATH, "redstone_ore", BLOCK_SIZE, true);
    emeraldOreBlock.loadObj(MODELS_PATH, "emerald_ore", BLOCK_SIZE, true);

    // Wood Planks
    oakPlanksBlock.loadObj(MODELS_PATH, "oak_planks", BLOCK_SIZE, true);
    sprucePlanksBlock.loadObj(MODELS_PATH, "spruce_planks", BLOCK_SIZE, true);
    birchPlanksBlock.loadObj(MODELS_PATH, "birch_planks", BLOCK_SIZE, true);
    acaciaPlanksBlock.loadObj(MODELS_PATH, "acacia_planks", BLOCK_SIZE, true);

    // Stone Bricks
    stoneBrickBlock.loadObj(MODELS_PATH, "stone_bricks", BLOCK_SIZE, true);
    crackedStoneBricksBlock.loadObj(MODELS_PATH, "cracked_stone_bricks", BLOCK_SIZE, true);
    mossyStoneBricksBlock.loadObj(MODELS_PATH, "mossy_stone_bricks", BLOCK_SIZE, true);
    chiseledStoneBricksBlock.loadObj(MODELS_PATH, "chiseled_stone_bricks", BLOCK_SIZE, true);

    // Woods
    strippedOakWoodBlock.loadObj(MODELS_PATH, "stripped_oak_log", BLOCK_SIZE, true);

    // Load model's Textures:
    // Blocks
    texRepo->addByMesh(TEXTURES_PATH, stoneBlock, PNG);
    texRepo->addByMesh(TEXTURES_PATH, grassBlock, PNG);
    texRepo->addByMesh(TEXTURES_PATH, dirtBlock, PNG);
    texRepo->addByMesh(TEXTURES_PATH, waterBlock, PNG);
    texRepo->addByMesh(TEXTURES_PATH, bedrockBlock, PNG);
    texRepo->addByMesh(TEXTURES_PATH, sandBlock, PNG);
    texRepo->addByMesh(TEXTURES_PATH, glassBlock, PNG);
    texRepo->addByMesh(TEXTURES_PATH, bricksBlock, PNG);

    // Ores and Minerals
    texRepo->addByMesh(TEXTURES_PATH, goldOreBlock, PNG);
    texRepo->addByMesh(TEXTURES_PATH, ironOreBlock, PNG);
    texRepo->addByMesh(TEXTURES_PATH, coalOreBlock, PNG);
    texRepo->addByMesh(TEXTURES_PATH, diamondOreBlock, PNG);
    texRepo->addByMesh(TEXTURES_PATH, redsoneOreBlock, PNG);
    texRepo->addByMesh(TEXTURES_PATH, emeraldOreBlock, PNG);

    // Wood Planks
    texRepo->addByMesh(TEXTURES_PATH, oakPlanksBlock, PNG);
    texRepo->addByMesh(TEXTURES_PATH, sprucePlanksBlock, PNG);
    texRepo->addByMesh(TEXTURES_PATH, birchPlanksBlock, PNG);
    texRepo->addByMesh(TEXTURES_PATH, acaciaPlanksBlock, PNG);

    // Stone Bricks
    texRepo->addByMesh(TEXTURES_PATH, stoneBrickBlock, PNG);
    texRepo->addByMesh(TEXTURES_PATH, crackedStoneBricksBlock, PNG);
    texRepo->addByMesh(TEXTURES_PATH, mossyStoneBricksBlock, PNG);
    texRepo->addByMesh(TEXTURES_PATH, chiseledStoneBricksBlock, PNG);

    // Woods
    texRepo->addByMesh(TEXTURES_PATH, strippedOakWoodBlock, PNG);
}

void BlockManager::linkTextureByBlockType(int blockType, const u32 t_meshId, u8 materialIndex)
{

    texRepo->getBySpriteOrMesh(getMeshByBlockType(blockType).getMaterial(materialIndex).getId())->addLink(t_meshId);
}

void BlockManager::removeTextureLinkByBlockType(int blockType, const u32 t_meshId, u8 materialIndex)
{
    texRepo->getBySpriteOrMesh(getMeshByBlockType(blockType).getMaterial(materialIndex).getId())->removeLinkById(t_meshId);
}

void BlockManager::pushItems()
{
    // Blocks
    this->blockItems.push_back(new BlockItem(&stoneBlock, STONE_BLOCK));
    this->blockItems.push_back(new BlockItem(&grassBlock, GRASS_BLOCK));
    this->blockItems.push_back(new BlockItem(&dirtBlock, DIRTY_BLOCK));
    this->blockItems.push_back(new BlockItem(&waterBlock, WATER_BLOCK));
    this->blockItems.push_back(new BlockItem(&bedrockBlock, BEDROCK_BLOCK));
    this->blockItems.push_back(new BlockItem(&sandBlock, SAND_BLOCK));
    this->blockItems.push_back(new BlockItem(&glassBlock, GLASS_BLOCK));
    this->blockItems.push_back(new BlockItem(&bricksBlock, BRICKS_BLOCK));

    // Ores and Minerals
    this->blockItems.push_back(new BlockItem(&goldOreBlock, GOLD_ORE_BLOCK));
    this->blockItems.push_back(new BlockItem(&ironOreBlock, IRON_ORE_BLOCK));
    this->blockItems.push_back(new BlockItem(&coalOreBlock, COAL_ORE_BLOCK));
    this->blockItems.push_back(new BlockItem(&diamondOreBlock, DIAMOND_ORE_BLOCK));
    this->blockItems.push_back(new BlockItem(&redsoneOreBlock, REDSTONE_ORE_BLOCK));
    this->blockItems.push_back(new BlockItem(&emeraldOreBlock, EMERALD_ORE_BLOCK));

    // Wood Planks
    this->blockItems.push_back(new BlockItem(&oakPlanksBlock, OAK_PLANKS_BLOCK));
    this->blockItems.push_back(new BlockItem(&sprucePlanksBlock, SPRUCE_PLANKS_BLOCK));
    this->blockItems.push_back(new BlockItem(&birchPlanksBlock, BIRCH_PLANKS_BLOCK));
    this->blockItems.push_back(new BlockItem(&acaciaPlanksBlock, ACACIA_PLANKS_BLOCK));

    // Stone bricks
    this->blockItems.push_back(new BlockItem(&stoneBrickBlock, STONE_BRICK_BLOCK));
    this->blockItems.push_back(new BlockItem(&crackedStoneBricksBlock, CRACKED_STONE_BRICKS_BLOCK));
    this->blockItems.push_back(new BlockItem(&mossyStoneBricksBlock, MOSSY_STONE_BRICKS_BLOCK));
    this->blockItems.push_back(new BlockItem(&chiseledStoneBricksBlock, CHISELED_STONE_BRICKS_BLOCK));

    // Woods
    this->blockItems.push_back(new BlockItem(&strippedOakWoodBlock, STRIPPED_OAK_WOOD_BLOCK));
}

Mesh &BlockManager::getMeshByBlockType(int blockType)
{
    for (u8 i = 0; i < blockItems.size(); i++)
        if (blockItems[i]->blockId == blockType)
            return *blockItems[i]->t_mesh;
}