#pragma once

#include <math/vec4.hpp>

#ifndef VERSION
#define VERSION "v0.43.71-pre-alpha"  // Software version
#endif

#define OVERWORLD_H_DISTANCE 128  // Total horizontal overworld distance;
#define OVERWORLD_V_DISTANCE 64   // Total vertical overworld distance;

#define HALF_OVERWORLD_H_DISTANCE \
  (OVERWORLD_H_DISTANCE / 2)  // Half horizontal overworld distance;
#define HALF_OVERWORLD_V_DISTANCE \
  (OVERWORLD_V_DISTANCE / 2)  // Half vertical overworld distance;

#define OVERWORLD_MIN_DISTANCE 0                     // Min overworld width;
#define OVERWORLD_MAX_DISTANCE OVERWORLD_H_DISTANCE  // Max overworld width;

#define OVERWORLD_MIN_HEIGH 0  // Min overworld layer for badblock;
#define OVERWORLD_MAX_HEIGH OVERWORLD_V_DISTANCE  // Max overworld heigh layer;

#define OVERWORLD_SIZE \
  (OVERWORLD_H_DISTANCE * OVERWORLD_H_DISTANCE * OVERWORLD_V_DISTANCE)

// Define static chunk size CHUNCK_SIZE x CHUNCK_SIZE x OVERWORLD_V_DISTANCE
#define CHUNCK_SIZE 8
#define CHUNCK_LENGTH (CHUNCK_SIZE * CHUNCK_SIZE * CHUNCK_SIZE)
#define HALF_CHUNCK_SIZE (CHUNCK_SIZE / 2)
#define BLOCK_SIZE 8.0F
#define DUBLE_BLOCK_SIZE (BLOCK_SIZE * 2.0F)
#define CHUNCK_DISTANCE (HALF_CHUNCK_SIZE * DUBLE_BLOCK_SIZE)

// Defines how many chunks will be loaded from player position
#define MIN_DRAW_DISTANCE 2
#define MAX_DRAW_DISTANCE 4

// Define how many blocks will be loaded/unloaded from chunk per step in async
// loading
#define UNLOAD_CHUNK_BATCH 256
#define LOAD_CHUNK_BATCH 32

// Texture atlas info
#define MAX_TEX_COLS 16
#define MAX_TEX_ROWS 16

/**
 * Define blocks IDs
 **/
enum class Blocks {
  VOID,
  AIR_BLOCK,

  // Blocks
  STONE_BLOCK,
  GRASS_BLOCK,
  DIRTY_BLOCK,
  WATER_BLOCK,
  BEDROCK_BLOCK,
  SAND_BLOCK,
  GLASS_BLOCK,
  BRICKS_BLOCK,
  GRAVEL_BLOCK,

  // Ores and Minerals
  GOLD_ORE_BLOCK,
  IRON_ORE_BLOCK,
  COAL_ORE_BLOCK,
  DIAMOND_ORE_BLOCK,
  REDSTONE_ORE_BLOCK,
  EMERALD_ORE_BLOCK,

  // Flowers
  GRASS,
  POPPY_FLOWER,
  DANDELION_FLOWER,

  // Wood Planks
  OAK_PLANKS_BLOCK,
  SPRUCE_PLANKS_BLOCK,
  BIRCH_PLANKS_BLOCK,
  ACACIA_PLANKS_BLOCK,

  // Stone bricks
  STONE_BRICK_BLOCK,
  CRACKED_STONE_BRICKS_BLOCK,
  MOSSY_STONE_BRICKS_BLOCK,
  CHISELED_STONE_BRICKS_BLOCK,

  // Concretes
  YELLOW_CONCRETE,
  BLUE_CONCRETE,
  GREEN_CONCRETE,
  ORANGE_CONCRETE,
  PURPLE_CONCRETE,
  RED_CONCRETE,
  WHITE_CONCRETE,
  BLACK_CONCRETE,

  // Light Emissors
  GLOWSTONE_BLOCK,
  JACK_O_LANTERN_BLOCK,
  PUMPKIN_BLOCK,

  // Woods
  OAK_LOG_BLOCK,
  OAK_LEAVES_BLOCK,
  BIRCH_LOG_BLOCK,
  BIRCH_LEAVES_BLOCK,

  // Helper index
  TOTAL_OF_BLOCKS
};
/*---------------------------------------------*/

#define FIRST_PERSON_CAM 1
#define THIRD_PERSON_CAM 2

#define MAX_RANGE_PICKER \
  (DUBLE_BLOCK_SIZE * 6.0F)  // How far the player can pick a block

// Game states
#define MAIN_MENU 0
#define IN_GAME 1
#define IN_GAME_MENU 2
#define SPLASH_SCREEN 3
#define LOADING_SCREEN 4

// World constants
#define GRAVITY Vec4(0.0f, -340.0f, 0.0f)
#define GRAVITY_ON_WATER_FACTOR 0.85F
#define GRAVITY_UNDER_WATER_FACTOR 0.85F
#define IN_WATER_FRICTION 0.25F

enum class GAME_MODE { SURVIVAL, CREATIVE };

// TODO: Move to liht manager
#define MAX_AO_VALUE 3
#define AO_BASE_COLOR Color()



enum class ItemId {
  // None
  empty,

  // Blocks
  dirt,
  gravel,
  sand,
  stone,
  bricks,
  glass,
  pumpkin,

  // Wood Planks
  oak_planks,
  spruce_planks,
  birch_planks,
  acacia_planks,

  // Wood Log
  oak_log,
  birch_log,

  // Stone Bricks
  stone_brick,
  cracked_stone_bricks,
  mossy_stone_bricks,
  chiseled_stone_bricks,

  // Ores and Minerals
  coal_ore_block,
  diamond_ore_block,
  iron_ore_block,
  gold_ore_block,
  redstone_ore_block,
  emerald_ore_block,

  // Flowers
  poppy_flower,
  dandelion_flower,

  // Flowers
  yellow_concrete,
  blue_concrete,
  green_concrete,
  orange_concrete,
  purple_concrete,
  red_concrete,
  white_concrete,
  black_concrete,

  // Light Emissors
  glowstone,
  jack_o_lantern,

  // TODO: move total_of_items to the end of enum this is temp!
  // Helper
  total_of_items,

  // Tools
  wooden_axe,
  stone_axe,
  iron_axe,
  golden_axe,
  diamond_axe,
};

enum class ItemType { McPipBlock, ObjBlock, Tool, Food };

enum class PaticleType { Block };

#define HOT_INVENTORY_SIZE 9

#define MIN_WORLD_POS                            \
  Vec4(OVERWORLD_MIN_DISTANCE* DUBLE_BLOCK_SIZE, \
       OVERWORLD_MIN_HEIGH* DUBLE_BLOCK_SIZE,    \
       OVERWORLD_MIN_DISTANCE* DUBLE_BLOCK_SIZE)

#define MAX_WORLD_POS                                   \
  Vec4((OVERWORLD_MAX_DISTANCE - 1) * DUBLE_BLOCK_SIZE, \
       (OVERWORLD_MAX_HEIGH - 1) * DUBLE_BLOCK_SIZE,    \
       (OVERWORLD_MAX_DISTANCE - 1) * DUBLE_BLOCK_SIZE)

#define CENTER_WORLD_POS (MAX_WORLD_POS + MIN_WORLD_POS) / 2

#define MAX_FRAME_MS 0.016667F  // Comes from 1 / 60;
#define FIXED_FRAME_MS 0.016667F
#define FIXED_30_FRAME_MS 0.0344F  // Comes from 1 / 29;

#define MAX_ADPCM_CH 23

typedef enum {
  WORLD_TYPE_ORIGINAL = 0,
  WORLD_TYPE_FLAT = 1,
  WORLD_TYPE_ISLAND = 2,
  WORLD_TYPE_WOODS = 3,
  WORLD_TYPE_FLOATING = 4
} WorldType;

#define SAVE_FILE_EXTENSION "tcw"

// Blocks params
#define BREAKING_TIME_IN_CREATIVE_MODE 0.150F