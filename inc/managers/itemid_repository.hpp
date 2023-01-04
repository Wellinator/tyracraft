#pragma once

#include "entities/item.hpp"
#include "items_repository.hpp"
#include "contants.hpp"
#include "file/file_utils.hpp"

using Tyra::Renderer;
using Tyra::TextureRepository;

using Tyra::Sprite;

class ItemIdRepository {
 public:
 ItemId();
 ~ItemId();


  // Item Id

  // Blocks
  Item dirt 1;
  Item sand 3;
  Item stone 2;
  Item bricks 4;
  Item glass 5;

  // Ores and Minerals blocks
  Item coal_ore_block 6;
  Item diamond_ore_block 9;
  Item iron_ore_block 7;
  Item gold_ore_block 8;
  Item redstone_ore_block 10;
  Item emerald_ore_block 11;

  // Wood Planks
  Item oak_planks 12;
  Item spruce_planks 13;
  Item birch_planks 14;
  Item acacia_planks 15;

  // Stone Bricks
  Item stone_brick 16;
  Item cracked_stone_bricks 17;
  Item mossy_stone_bricks 18;
  Item chiseled_stone_bricks 19;

  // Stripped woods
  Item stripped_oak_wood 20;

  // Toolds
  Item wooden_axe 21;
};
