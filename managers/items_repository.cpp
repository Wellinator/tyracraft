#include "./items_repository.hpp"

ItemRepository::ItemRepository() {}
ItemRepository::~ItemRepository() {}

void ItemRepository::init(TextureRepository *t_texRepo)
{
    texRepo = t_texRepo;
    this->loadItems();
}

void ItemRepository::loadItems()
{
    char *TEXTURES_PATH = "assets/textures/items/";
    consoleLog("\n\nLoading items repository...\n\n");

    //----------------Blocks------------------------------
    dirt.id = ITEM_TYPES::dirt;
    dirt.blockId = DIRTY_BLOCK;
    this->items.push_back(&dirt);
    texRepo->add(TEXTURES_PATH, "dirt", PNG)->addLink(dirt.sprite.getId());
    
    sand.id = ITEM_TYPES::sand;
    sand.blockId = SAND_BLOCK;
    this->items.push_back(&sand);
    texRepo->add(TEXTURES_PATH, "sand", PNG)->addLink(sand.sprite.getId());

    stone.id = ITEM_TYPES::stone;
    stone.blockId = STONE_BLOCK;
    this->items.push_back(&stone);
    texRepo->add(TEXTURES_PATH, "stone", PNG)->addLink(stone.sprite.getId());
    
    bricks.id = ITEM_TYPES::bricks;
    bricks.blockId = BRICKS_BLOCK;
    this->items.push_back(&bricks);
    texRepo->add(TEXTURES_PATH, "bricks", PNG)->addLink(bricks.sprite.getId());

    glass.id = ITEM_TYPES::glass;
    glass.blockId = BRICKS_BLOCK;
    this->items.push_back(&glass);
    texRepo->add(TEXTURES_PATH, "glass", PNG)->addLink(glass.sprite.getId());

    //------------------------------Ores and Minerals blocks----------------------
    coal_ore_block.id = ITEM_TYPES::coal_ore_block;
    coal_ore_block.blockId = COAL_ORE_BLOCK;
    this->items.push_back(&coal_ore_block);
    texRepo->add(TEXTURES_PATH, "coal_ore_block", PNG)->addLink(coal_ore_block.sprite.getId());

    diamond_ore_block.id = ITEM_TYPES::diamond_ore_block;
    diamond_ore_block.blockId = DIAMOND_ORE_BLOCK;
    this->items.push_back(&diamond_ore_block);
    texRepo->add(TEXTURES_PATH, "diamond_ore_block", PNG)->addLink(diamond_ore_block.sprite.getId());

    iron_ore_block.id = ITEM_TYPES::iron_ore_block;
    iron_ore_block.blockId = IRON_ORE_BLOCK;
    this->items.push_back(&iron_ore_block);
    texRepo->add(TEXTURES_PATH, "iron_ore_block", PNG)->addLink(iron_ore_block.sprite.getId());

    gold_ore_block.id = ITEM_TYPES::gold_ore_block;
    gold_ore_block.blockId = GOLD_ORE_BLOCK;
    this->items.push_back(&gold_ore_block);
    texRepo->add(TEXTURES_PATH, "gold_ore_block", PNG)->addLink(gold_ore_block.sprite.getId());

    redstone_ore_block.id = ITEM_TYPES::redstone_ore_block;
    redstone_ore_block.blockId = REDSTONE_ORE_BLOCK;
    this->items.push_back(&redstone_ore_block);
    texRepo->add(TEXTURES_PATH, "redstone_ore_block", PNG)->addLink(redstone_ore_block.sprite.getId());

    emerald_ore_block.id = ITEM_TYPES::emerald_ore_block;
    emerald_ore_block.blockId = EMERALD_ORE_BLOCK;
    this->items.push_back(&emerald_ore_block);
    texRepo->add(TEXTURES_PATH, "emerald_ore_block", PNG)->addLink(emerald_ore_block.sprite.getId());

    // -------------- Wood Planks ------------------------
    oak_planks.id = ITEM_TYPES::oak_planks;
    oak_planks.blockId = OAK_PLANKS_BLOCK;
    this->items.push_back(&oak_planks);
    texRepo->add(TEXTURES_PATH, "oak_planks", PNG)->addLink(oak_planks.sprite.getId());

    spruce_planks.id = ITEM_TYPES::spruce_planks;
    spruce_planks.blockId = SPRUCE_PLANKS_BLOCK;
    this->items.push_back(&spruce_planks);
    texRepo->add(TEXTURES_PATH, "spruce_planks", PNG)->addLink(spruce_planks.sprite.getId());

    birch_planks.id = ITEM_TYPES::birch_planks;
    birch_planks.blockId = BIRCH_PLANKS_BLOCK;
    this->items.push_back(&birch_planks);
    texRepo->add(TEXTURES_PATH, "birch_planks", PNG)->addLink(birch_planks.sprite.getId());

    acacia_planks.id = ITEM_TYPES::acacia_planks;
    acacia_planks.blockId = ACACIA_PLANKS_BLOCK;
    this->items.push_back(&acacia_planks);
    texRepo->add(TEXTURES_PATH, "acacia_planks", PNG)->addLink(acacia_planks.sprite.getId());

    // --------------------------Stone Bricks-------------------------
    stone_brick.id = ITEM_TYPES::stone_brick;
    stone_brick.blockId = STONE_BRICK_BLOCK;
    this->items.push_back(&stone_brick);
    texRepo->add(TEXTURES_PATH, "stone_brick", PNG)->addLink(stone_brick.sprite.getId());

    cracked_stone_bricks.id = ITEM_TYPES::cracked_stone_bricks;
    cracked_stone_bricks.blockId = CRACKED_STONE_BRICKS_BLOCK;
    this->items.push_back(&cracked_stone_bricks);
    texRepo->add(TEXTURES_PATH, "cracked_stone_bricks", PNG)->addLink(cracked_stone_bricks.sprite.getId());

    mossy_stone_bricks.id = ITEM_TYPES::mossy_stone_bricks;
    mossy_stone_bricks.blockId = MOSSY_STONE_BRICKS_BLOCK;
    this->items.push_back(&mossy_stone_bricks);
    texRepo->add(TEXTURES_PATH, "mossy_stone_bricks", PNG)->addLink(mossy_stone_bricks.sprite.getId());

    chiseled_stone_bricks.id = ITEM_TYPES::chiseled_stone_bricks;
    chiseled_stone_bricks.blockId = CHISELED_STONE_BRICKS_BLOCK;
    this->items.push_back(&chiseled_stone_bricks);
    texRepo->add(TEXTURES_PATH, "chiseled_stone_bricks", PNG)->addLink(chiseled_stone_bricks.sprite.getId());



    // --------------------Stripped Oak Wood----------------
    stripped_oak_wood.id = ITEM_TYPES::stripped_oak_wood;
    stripped_oak_wood.blockId = STRIPPED_OAK_WOOD_BLOCK;
    this->items.push_back(&stripped_oak_wood);
    texRepo->add(TEXTURES_PATH, "stripped_oak_wood", PNG)->addLink(stripped_oak_wood.sprite.getId());
    
    
}

Item *ItemRepository::getItemById(ITEM_TYPES &itemId)
{
    for (int i = 0; i < items.size(); i++)
        if (items[i]->id == itemId)
            return items[i];
    return NULL;
}

void ItemRepository::linkTextureByItemType(ITEM_TYPES itemType, const u32 t_spriteId)
{

    texRepo->getBySpriteOrMesh(getSpriteByItemType(itemType).getId())->addLink(t_spriteId);
}

void ItemRepository::removeTextureLinkByBlockType(ITEM_TYPES itemType, const u32 t_spriteId)
{
    Texture *tex = texRepo->getBySpriteOrMesh(getSpriteByItemType(itemType).getId());
    s32 index = tex->getIndexOfLink(t_spriteId);
    if (index != -1)
        tex->removeLinkByIndex(index);
}

Sprite &ItemRepository::getSpriteByItemType(ITEM_TYPES itemType)
{
    for (int i = 0; i < items.size(); i++)
        if (items[i]->id == itemType)
            return items[i]->sprite;
}