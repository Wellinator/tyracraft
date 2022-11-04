#pragma once

#include <tamtypes.h>
#include <renderer/renderer.hpp>
#include <renderer/core/2d/sprite/sprite.hpp>
#include <renderer/core/texture/models/texture.hpp>
#include "entities/item.hpp"
#include "contants.hpp"
#include "file/file_utils.hpp"

using Tyra::Renderer;
using Tyra::TextureRepository;

class ItemRepository {
 public:
  Renderer* t_renderer;

  ItemRepository();
  ~ItemRepository();
  void init(Renderer* t_renderer);
  Item* getItemById(ItemId& itemId);

  void linkTextureByItemType(ItemId itemType, const u32 t_spriteId);
  void removeTextureLinkByBlockType(ItemId itemType, const u32 t_spriteId);
  Sprite* getSpriteByItemType(ItemId itemType);

 private:
  std::vector<Item*> items;
  void loadItems();

  // Items reg
  //  Items Blocks
  Item dirt;
  Item sand;
  Item stone;
  Item bricks;
  Item glass;

  // Ores and Minerals blocks
  Item coal_ore_block;
  Item diamond_ore_block;
  Item iron_ore_block;
  Item gold_ore_block;
  Item redstone_ore_block;
  Item emerald_ore_block;

  // Wood Planks
  Item oak_planks;
  Item spruce_planks;
  Item birch_planks;
  Item acacia_planks;

  // Stone Bricks
  Item stone_brick;
  Item cracked_stone_bricks;
  Item mossy_stone_bricks;
  Item chiseled_stone_bricks;

  // Stripped woods
  Item stripped_oak_wood;

  // Toolds
  Item wooden_axe;
};
