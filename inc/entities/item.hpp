#pragma once

#include <renderer/core/2d/sprite/sprite.hpp>
#include "contants.hpp"

using Tyra::Sprite;

class Item {
 public:
  Item();
  ~Item();

  /** Item id */
  ItemType itemType = ItemType::McPipBlock;

  /** Item id */
  ItemId id;

  /** Block id to render mesh of sprite. */
  int blockId;

  /** Sprite to be renderer. */
  Sprite sprite;
};
