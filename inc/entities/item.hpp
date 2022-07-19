#ifndef _ITEM_
#define _ITEM_

#include <renderer/core/2d/sprite/sprite.hpp>
#include "contants.hpp"

class Item
{
public:
    Item();
    ~Item();

    /** Block id to render mesh of sprite. */
    ITEM_TYPES id;

    /** Block id to render mesh of sprite. */
    int blockId;

    /** Sprite to be renderer. */
    Sprite sprite;
};

#endif