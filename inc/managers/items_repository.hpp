#pragma once

#include <tamtypes.h>
#include <renderer/renderer.hpp>
#include <renderer/core/2d/sprite/sprite.hpp>
#include <renderer/core/texture/models/texture.hpp>
#include "entities/item.hpp"
#include "constants.hpp"
#include "file/file_utils.hpp"

using Tyra::Renderer;
using Tyra::TextureRepository;

class ItemRepository {
 public:
  Renderer* t_renderer;

  ItemRepository();
  ~ItemRepository();
  void init(Renderer* t_renderer, const std::string& texturePack);
  Item* getItemById(ItemId& itemId);

  u8 linkTextureByItemType(const ItemId& itemType, const u32& t_spriteId);
  void removeTextureLinkByBlockType(const ItemId& itemType,
                                    const u32& t_spriteId);
  Sprite* getSpriteByItemType(const ItemId& itemType);

 private:
  std::vector<Item*> items;
  void loadItems(const std::string& texturePack);

  // Items reg
  //  Items Blocks
  Item dirt;
  Item sand;
  Item stone;
  Item bricks;
  Item gravel;
  Item glass;
  Item pumpkin;

  // Ores and Minerals blocks
  Item coal_ore_block;
  Item diamond_ore_block;
  Item iron_ore_block;
  Item gold_ore_block;
  Item redstone_ore_block;
  Item emerald_ore_block;

  // Flowers
  Item poppy_flower;
  Item dandelion_flower;

  // Wood Planks
  Item oak_planks;
  Item spruce_planks;
  Item birch_planks;
  Item acacia_planks;

  // Wood log
  Item oak_log;
  Item birch_log;

  // Light Emissor
  Item glowstone;
  Item jack_o_lantern;

  // Concretes
  Item yellow_concrete;
  Item blue_concrete;
  Item green_concrete;
  Item orange_concrete;
  Item purple_concrete;
  Item red_concrete;
  Item white_concrete;
  Item black_concrete;

  // Stone Bricks
  Item stone_brick;
  Item cracked_stone_bricks;
  Item mossy_stone_bricks;
  Item chiseled_stone_bricks;

  // Toolds
  // Item wooden_axe;
};
