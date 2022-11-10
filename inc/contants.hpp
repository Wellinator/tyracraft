#pragma once

#include <math/vec4.hpp>

#define OVERWORLD_H_DISTANCE 256  // Total horizontal overworld distance;
#define OVERWORLD_V_DISTANCE 128  // Total vertical overworld distance;
#define HALF_OVERWORLD_H_DISTANCE \
  (OVERWORLD_H_DISTANCE / 2)  // Half horizontal overworld distance;
#define HALF_OVERWORLD_V_DISTANCE \
  (OVERWORLD_V_DISTANCE / 2)  // Half vertical overworld distance;
#define OVERWORLD_MIN_DISTANCE \
  -HALF_OVERWORLD_H_DISTANCE  // Min overworld width;
#define OVERWORLD_MAX_DISTANCE \
  HALF_OVERWORLD_H_DISTANCE  // Max overworld width;
#define OVERWORLD_MIN_HEIGH \
  -HALF_OVERWORLD_V_DISTANCE  // Min overworld layer for badblock;
#define OVERWORLD_MAX_HEIGH \
  HALF_OVERWORLD_V_DISTANCE  // Max overworld heigh layer;
#define OVERWORLD_SIZE \
  ((OVERWORLD_H_DISTANCE * OVERWORLD_H_DISTANCE) * OVERWORLD_V_DISTANCE)

// Define static chunk size CHUNCK_SIZE x CHUNCK_SIZE x OVERWORLD_V_DISTANCE
#define CHUNCK_SIZE 16
#define CHUNCK_HEIGHT OVERWORLD_V_DISTANCE
#define CHUNCK_LENGTH ((CHUNCK_SIZE * CHUNCK_SIZE) * CHUNCK_HEIGHT)
#define HALF_CHUNCK_SIZE (CHUNCK_SIZE / 2)
#define BLOCK_SIZE 8.0F
#define DUBLE_BLOCK_SIZE (BLOCK_SIZE * 2.0F)
#define CHUNCK_DISTANCE (HALF_CHUNCK_SIZE * DUBLE_BLOCK_SIZE)

// Defines how many chunks will be loaded from player position
#define DRAW_DISTANCE_IN_CHUNKS 2

// Define how many blocks will be loaded/unloaded from chunk per step in async
// loading
#define UNLOAD_CHUNK_BATCH 256
#define LOAD_CHUNK_BATCH 128

/**
 * Define blocks IDs
 **/
#define AIR_BLOCK 0

// Blocks
#define STONE_BLOCK 1
#define GRASS_BLOCK 2
#define DIRTY_BLOCK 3
#define WATER_BLOCK 4
#define BEDROCK_BLOCK 5
#define SAND_BLOCK 6
#define GLASS_BLOCK 7
#define BRICKS_BLOCK 8

// Ores and Minerals
#define GOLD_ORE_BLOCK 9
#define IRON_ORE_BLOCK 10
#define COAL_ORE_BLOCK 11
#define DIAMOND_ORE_BLOCK 12
#define REDSTONE_ORE_BLOCK 13
#define EMERALD_ORE_BLOCK 14

// Wood Planks
#define OAK_PLANKS_BLOCK 15
#define SPRUCE_PLANKS_BLOCK 16
#define BIRCH_PLANKS_BLOCK 17
#define ACACIA_PLANKS_BLOCK 18

// Stone bricks
#define STONE_BRICK_BLOCK 19
#define CRACKED_STONE_BRICKS_BLOCK 20
#define MOSSY_STONE_BRICKS_BLOCK 21
#define CHISELED_STONE_BRICKS_BLOCK 22

// Woods
#define STRIPPED_OAK_WOOD_BLOCK 23
#define OAK_LOG_BLOCK 24
#define OAK_LEAVES_BLOCK 25
#define BIRCH_LOG_BLOCK 26
#define BIRCH_LEAVES_BLOCK 27
/*---------------------------------------------*/

#define FIRST_PERSON_CAM 1
#define THIRD_PERSON_CAM 2

#define MAX_RANGE_PICKER \
  (DUBLE_BLOCK_SIZE * 8.0F)  // How far the player can pick a block

// Game states
#define MAIN_MENU 0
#define IN_GAME 1
#define IN_GAME_MENU 2
#define SPLASH_SCREEN 3
#define LOADING_SCREEN 4

// World constants
#define GRAVITY Vec4(0.0f, 850.0f, 0.0f)

enum class GAME_MODE { SURVIVAL, CREATIVE };

enum class ItemId {
  // None
  empty,

  // Blocks
  dirt,
  sand,
  stone,
  bricks,
  glass,

  // Ores and Minerals
  coal_ore_block,
  diamond_ore_block,
  iron_ore_block,
  gold_ore_block,
  redstone_ore_block,
  emerald_ore_block,

  // Wood Planks
  oak_planks,
  spruce_planks,
  birch_planks,
  acacia_planks,

  // Stripped woods
  stripped_oak_wood,

  // Stone Bricks
  stone_brick,
  cracked_stone_bricks,
  mossy_stone_bricks,
  chiseled_stone_bricks,

  // Tools
  wooden_axe,
  stone_axe,
  iron_axe,
  golden_axe,
  diamond_axe
};

enum class ItemType { McPipBlock, ObjBlock, Tool, Food };

#define INVENTORY_SIZE 9

#define MIN_WORLD_POS                            \
  Vec4(OVERWORLD_MIN_DISTANCE* DUBLE_BLOCK_SIZE, \
       OVERWORLD_MIN_HEIGH* DUBLE_BLOCK_SIZE,    \
       OVERWORLD_MIN_DISTANCE* DUBLE_BLOCK_SIZE)
#define MAX_WORLD_POS                            \
  Vec4(OVERWORLD_MAX_DISTANCE* DUBLE_BLOCK_SIZE, \
       OVERWORLD_MAX_HEIGH* DUBLE_BLOCK_SIZE,    \
       OVERWORLD_MAX_DISTANCE* DUBLE_BLOCK_SIZE)

#define MAX_FRAME_MS 0.016667F  // Comes from 1 / 60;
