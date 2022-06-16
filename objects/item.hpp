#ifndef _ITEM_
#define _ITEM_

#include <models/sprite.hpp>
#include "../include/contants.hpp"

class Item
{
public:
    Item();
    ~Item();

    /** Block id to render mesh of sprite. */
    u8 id;

    /** Block id to render mesh of sprite. */
    u8 blockId;

    /** Sprite to be renderer. */
    Sprite *sprite;
};

#endif