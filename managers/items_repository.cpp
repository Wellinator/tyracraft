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
    // Dirt block item
    Item *dirt = new Item();
    dirt->id = ITEM_TYPES::DIRT;
    dirt->blockId = DIRTY_BLOCK;
    dirt->sprite->setMode(MODE_STRETCH);
    dirt->sprite->size.set(20.0f, 20.0f);
    texRepo->add(TEXTURES_PATH, "dirt", PNG)->addLink(dirt->sprite->getId());
    this->items.push_back(dirt);

    // Stone
    Item *stone = new Item();
    stone->id = ITEM_TYPES::STONE;
    stone->blockId = DIRTY_BLOCK;
    stone->sprite->setMode(MODE_STRETCH);
    stone->sprite->size.set(20.0f, 20.0f);
    texRepo->add(TEXTURES_PATH, "stone", PNG)->addLink(stone->sprite->getId());
    this->items.push_back(stone);

    // Stripped Oak Wood
    Item *stripped_oak_wood = new Item();
    stripped_oak_wood->id = ITEM_TYPES::STRIPPED_OAK_WOOD;
    stripped_oak_wood->blockId = DIRTY_BLOCK;
    stripped_oak_wood->sprite->setMode(MODE_STRETCH);
    stripped_oak_wood->sprite->size.set(20.0f, 20.0f);
    texRepo->add(TEXTURES_PATH, "stripped_oak_wood", PNG)->addLink(stripped_oak_wood->sprite->getId());
    this->items.push_back(stripped_oak_wood);
}

Item *ItemRepository::getItemById(u16 itemId)
{
    for (int i = 0; i < items.size(); i++)
        if (items[i]->id == itemId)
            return items[i];
}