#ifndef _ITEM_REPOSITORY_
#define _ITEM_REPOSITORY_

#include <tamtypes.h>
#include <modules/texture_repository.hpp>
#include <models/sprite.hpp>
#include "../include/contants.hpp"
#include "../objects/item.hpp"

enum ITEM_TYPES
{
    DIRT,
    STONE,
    STRIPPED_OAK_WOOD
};

class ItemRepository
{
public:
    TextureRepository *texRepo;

    ItemRepository();
    ~ItemRepository();
    void init(TextureRepository *t_texRepo);
    Item *getItemById(u16 itemId);

private:
    const char *TEXTURES_PATH = "assets/textures/items/";
    std::vector<Item *> items;
    void loadItems();
};

#endif
