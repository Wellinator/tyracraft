#pragma once

#include <math/vec4.hpp>

#ifndef VERSION
#define VERSION "v0.83.120-pre-alpha"  // Software version
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
#define HALF_BLOCK_SIZE (BLOCK_SIZE / 2.0F)
#define CHUNCK_DISTANCE (HALF_CHUNCK_SIZE * DUBLE_BLOCK_SIZE)

#define OVERWORLD_V_DISTANCE_IN_CHUNKS \
  (OVERWORLD_V_DISTANCE /              \
   CHUNCK_SIZE)  // Total vertical overworld distance in chunks;
#define OVERWORLD_H_DISTANCE_IN_CHUNKS \
  (OVERWORLD_H_DISTANCE /              \
   CHUNCK_SIZE)  // Total horizontal overworld distance in chunks;
#define OVERWORLD_H_DISTANCE_IN_CHUNKS_SQRD \
  (OVERWORLD_H_DISTANCE_IN_CHUNKS *         \
   OVERWORLD_H_DISTANCE_IN_CHUNKS)  // Total horizontal overworld distance in
                                    // chunks squared;
#define OVERWORLD_V_DISTANCE_IN_CHUNKS \
  (OVERWORLD_V_DISTANCE /              \
   CHUNCK_SIZE)  // Total vertical overworld distance in chunks;
#define OVERWORLD_PAGE_IN_CHUNKS    \
  (OVERWORLD_V_DISTANCE_IN_CHUNKS * \
   OVERWORLD_H_DISTANCE_IN_CHUNKS)  // Overworld V * H distance in
                                    // chunks;

// Defines how many chunks will be loaded from player position
#define MIN_DRAW_DISTANCE 2
#define MAX_DRAW_DISTANCE 4

// Define how many blocks will be loaded/unloaded from chunk per step in async
// loading
#define UNLOAD_CHUNK_BATCH 8
#define LOAD_CHUNK_BATCH 6

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

  // Wool
  YELLOW_WOOL,
  BLUE_WOOL,
  GREEN_WOOL,
  ORANGE_WOOL,
  PURPLE_WOOL,
  RED_WOOL,
  WHITE_WOOL,
  BLACK_WOOL,

  // Light Emissors
  GLOWSTONE_BLOCK,
  JACK_O_LANTERN_BLOCK,
  PUMPKIN_BLOCK,
  LAVA_BLOCK,

  // Woods
  OAK_LOG_BLOCK,
  OAK_LEAVES_BLOCK,
  BIRCH_LOG_BLOCK,
  BIRCH_LEAVES_BLOCK,

  // Items
  TORCH,

  // Slabs
  STONE_SLAB,
  BRICKS_SLAB,
  OAK_PLANKS_SLAB,
  SPRUCE_PLANKS_SLAB,
  BIRCH_PLANKS_SLAB,
  ACACIA_PLANKS_SLAB,
  STONE_BRICK_SLAB,
  CRACKED_STONE_BRICKS_SLAB,
  MOSSY_STONE_BRICKS_SLAB,

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

  // Wools
  yellow_wool,
  blue_wool,
  green_wool,
  orange_wool,
  purple_wool,
  red_wool,
  white_wool,
  black_wool,

  // Light Emissors
  glowstone,
  jack_o_lantern,

  // Items
  water_bucket,
  lava_bucket,
  torch,

  // Slabs
  stone_slab,
  bricks_slab,
  oak_slab,
  spruce_slab,
  birch_slab,
  acacia_slab,
  stone_brick_slab,
  cracked_stone_bricks_slab,
  mossy_stone_bricks_slab,

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

#define WATER_PROPAGATION_PER_TICKS 5
#define LAVA_PROPAGATION_PER_TICKS 10

/**
 * https://minecraft.fandom.com/wiki/Water
 * 1	block	  1
 * 2	blocks	0.75-1
 * 3	blocks	0.625-0.75
 * 4	blocks	0.5-0.625
 * 5	blocks	0.375-0.5
 * 6	blocks	0.25-0.375
 * 7	blocks	0.125-0.25
 */
typedef enum {
  Percent0,
  Percent12,
  Percent25,
  Percent37,
  Percent50,
  Percent62,
  Percent75,
  Percent100,
} LiquidLevel;

enum class BlockOrientation {
  East = 0,
  North = 1,
  West = 2,
  South = 3,
  Top = 4,
};

enum class LiquidOrientation {
  East,
  NorthEast,
  North,
  NorthWest,
  West,
  SouthEast,
  South,
  SouthWest
};

enum class SlabOrientation {
  Top = 0,
  Bottom = 1,
};

enum class PlacementDirection {
  Top = 0,
  Bottom = 1,
  Left = 2,
  Right = 3,
  Front = 4,
  Back = 5,
};

// Entities types based on https://minecraft.fandom.com/wiki/Entity
enum class EntityType {
  Block,
  Falling_Block,
  Player,
  Mob,
  Item,
  Experience_Orb,
  Arrow,
  Painting
};

// Three bits for liquid volume
#define LIQUID_LEVEL_MASK 0b00011100

// Three bits for orientation
#define LIQUID_ORIENTATION_MASK 0b11100000

// Three bits for orientation for torch
#define TORCH_ORIENTATION_MASK 0b00000111

// Two bits for orientation
#define BLOCK_ORIENTATION_MASK 0b00000011

// Two bits for slab orientation (Top or Bottom)
#define SLAB_ORIENTATION_MASK 0b00000100

#define _90DEGINRAD Tyra::Math::ANG2RAD * 90
#define _180DEGINRAD Tyra::Math::ANG2RAD * 180
#define _270DEGINRAD Tyra::Math::ANG2RAD * 270
#define SIN_90 1.000000f
#define COS_90 0.000000f
#define SIN_180 0.000000f
#define COS_180 -1.00000f
#define SIN_270 -1.00000f
#define COS_270 0.000000f
#define SIN_360 0.000000f
#define COS_360 1.000000f

enum class ItemType { McPipBlock, ObjBlock, Tool, Food };

enum class MobCategory { Passive, Neutral, Hostile };

enum class MobType { Pig };

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

#define TORCH_UV_COUNTER 20

#define MAX_SAFE_MEMORY_ALLOCATION 29