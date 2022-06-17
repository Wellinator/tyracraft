#ifndef _ITEM_REPOSITORY_
#define _ITEM_REPOSITORY_

#include <tamtypes.h>
#include <modules/texture_repository.hpp>
#include <models/sprite.hpp>
#include <models/texture.hpp>
#include "../objects/item.hpp"
#include "../include/contants.hpp"

class ItemRepository
{
public:
    TextureRepository *texRepo;

    ItemRepository();
    ~ItemRepository();
    void init(TextureRepository *t_texRepo);
    Item *getItemById(ITEM_TYPES &itemId);

    void linkTextureByItemType(ITEM_TYPES itemType, const u32 t_spriteId);
    void removeTextureLinkByBlockType(ITEM_TYPES itemType, const u32 t_spriteId);
    Sprite &getSpriteByItemType(ITEM_TYPES itemType);

private:
    std::vector<Item *> items;
    void loadItems();

    //Items reg
    Item dirt;
    Item stone;
    Item stripped_oak_wood;
};

#endif
