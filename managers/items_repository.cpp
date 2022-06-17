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

    // Dirt block item
    dirt.id = ITEM_TYPES::DIRT;
    dirt.blockId = DIRTY_BLOCK;
    dirt.sprite.setMode(MODE_STRETCH);
    dirt.sprite.size.set(20.0f, 20.0f);
    texRepo->add(TEXTURES_PATH, "dirt", PNG)->addLink(dirt.sprite.getId());
    this->items.push_back(&dirt);

    // Stone
    stone.id = ITEM_TYPES::STONE;
    stone.blockId = STONE_BLOCK;
    stone.sprite.setMode(MODE_STRETCH);
    stone.sprite.size.set(20.0f, 20.0f);
    texRepo->add(TEXTURES_PATH, "stone", PNG)->addLink(stone.sprite.getId());
    this->items.push_back(&stone);

    // Stripped Oak Wood
    stripped_oak_wood.id = ITEM_TYPES::STRIPPED_OAK_WOOD;
    stripped_oak_wood.blockId = STRIPPED_OAK_WOOD_BLOCK;
    stripped_oak_wood.sprite.setMode(MODE_STRETCH);
    stripped_oak_wood.sprite.size.set(20.0f, 20.0f);
    texRepo->add(TEXTURES_PATH, "stripped_oak_wood", PNG)->addLink(stripped_oak_wood.sprite.getId());
    this->items.push_back(&stripped_oak_wood);
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