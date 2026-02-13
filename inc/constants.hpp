#pragma once

#include <math/vec4.hpp>

#ifndef VERSION
#define VERSION "v0.86.140-pre-alpha"  // Software version
#endif

// Total number of chunks in the overworld
#define OVERWORLD_SIZE_IN_CHUNKS (OVERWORLD_SIZE / (CHUNK_SIZE * CHUNK_SIZE * CHUNK_SIZE))  
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

// Define static chunk size CHUNK_SIZE x CHUNK_SIZE x OVERWORLD_V_DISTANCE
#define CHUNK_SIZE 8
#define CHUNK_LENGTH (CHUNK_SIZE * CHUNK_SIZE * CHUNK_SIZE)
#define HALF_CHUNK_SIZE (CHUNK_SIZE / 2)
#define BLOCK_SIZE 8.0F
#define DOUBLE_BLOCK_SIZE (BLOCK_SIZE * 2.0F)
#define HALF_BLOCK_SIZE (BLOCK_SIZE / 2.0F)
#define CHUNK_DISTANCE (HALF_CHUNK_SIZE * DOUBLE_BLOCK_SIZE)
#define HALF_CHUNK_SIZE_SCALED HALF_CHUNK_SIZE* DOUBLE_BLOCK_SIZE

#define OVERWORLD_V_DISTANCE_IN_CHUNKS \
  (OVERWORLD_V_DISTANCE /              \
   CHUNK_SIZE)  // Total vertical overworld distance in chunks;
#define OVERWORLD_H_DISTANCE_IN_CHUNKS \
  (OVERWORLD_H_DISTANCE /              \
   CHUNK_SIZE)  // Total horizontal overworld distance in chunks;
#define OVERWORLD_H_DISTANCE_IN_CHUNKS_SQRD \
  (OVERWORLD_H_DISTANCE_IN_CHUNKS *         \
   OVERWORLD_H_DISTANCE_IN_CHUNKS)  // Total horizontal overworld distance in
                                    // chunks squared;
#define OVERWORLD_V_DISTANCE_IN_CHUNKS \
  (OVERWORLD_V_DISTANCE /              \
   CHUNK_SIZE)  // Total vertical overworld distance in chunks;
#define OVERWORLD_PAGE_IN_CHUNKS    \
  (OVERWORLD_V_DISTANCE_IN_CHUNKS * \
   OVERWORLD_H_DISTANCE_IN_CHUNKS)  // Overworld V * H distance in
                                    // chunks;

// Defines how many chunks will be loaded from player position
#define MIN_DRAW_DISTANCE 2
#define MAX_DRAW_DISTANCE 16
#define DRAW_DISTANCE_SAFETY_MARGIN_MB 5
#define DRAW_DISTANCE_BACKWARD_RATIO 0.4f
#define DRAW_DISTANCE_SIDE_RATIO 0.7f

// Define how many blocks will be loaded/unloaded from chunk per step in async
// loading
#define UNLOAD_CHUNK_BATCH 32
#define LOAD_CHUNK_BATCH 32

// Texture atlas info
#define MAX_TEX_COLS 16
#define MAX_TEX_ROWS 16

// Block face visibility flags
#define FRONT_VISIBLE 0b100000
#define BACK_VISIBLE 0b010000
#define LEFT_VISIBLE 0b001000
#define RIGHT_VISIBLE 0b000100
#define TOP_VISIBLE 0b000010
#define BOTTOM_VISIBLE 0b000001
#define HIDDEN_BLOCK 0b000000

/**
 * Define blocks IDs
 **/
enum class Blocks {
  VOID,       // 0
  AIR_BLOCK,  // 1

  // Blocks
  STONE_BLOCK,    // 2
  GRASS_BLOCK,    // 3
  DIRTY_BLOCK,    // 4
  WATER_BLOCK,    // 5
  BEDROCK_BLOCK,  // 6
  SAND_BLOCK,     // 7
  GLASS_BLOCK,    // 8
  BRICKS_BLOCK,   // 9
  GRAVEL_BLOCK,   // 10

  // Ores and Minerals
  GOLD_ORE_BLOCK,      // 11
  IRON_ORE_BLOCK,      // 12
  COAL_ORE_BLOCK,      // 13
  DIAMOND_ORE_BLOCK,   // 14
  REDSTONE_ORE_BLOCK,  // 15
  EMERALD_ORE_BLOCK,   // 16

  // Flowers
  GRASS,             // 17
  POPPY_FLOWER,      // 18
  DANDELION_FLOWER,  // 19

  // Wood Planks
  OAK_PLANKS_BLOCK,     // 20
  SPRUCE_PLANKS_BLOCK,  // 21
  BIRCH_PLANKS_BLOCK,   // 22
  ACACIA_PLANKS_BLOCK,  // 23

  // Stone bricks
  STONE_BRICK_BLOCK,            // 24
  CRACKED_STONE_BRICKS_BLOCK,   // 25
  MOSSY_STONE_BRICKS_BLOCK,     // 26
  CHISELED_STONE_BRICKS_BLOCK,  // 27

  // Wool
  YELLOW_WOOL,  // 28
  BLUE_WOOL,    // 29
  GREEN_WOOL,   // 30
  ORANGE_WOOL,  // 31
  PURPLE_WOOL,  // 32
  RED_WOOL,     // 33
  WHITE_WOOL,   // 34
  BLACK_WOOL,   // 35

  // Light Emissors
  GLOWSTONE_BLOCK,       // 36
  JACK_O_LANTERN_BLOCK,  // 37
  PUMPKIN_BLOCK,         // 38
  LAVA_BLOCK,            // 39

  // Woods
  OAK_LOG_BLOCK,       // 40
  OAK_LEAVES_BLOCK,    // 41
  BIRCH_LOG_BLOCK,     // 42
  BIRCH_LEAVES_BLOCK,  // 43

  // Items
  TORCH,  // 44

  // Slabs
  STONE_SLAB,                 // 45
  BRICKS_SLAB,                // 46
  OAK_PLANKS_SLAB,            // 47
  SPRUCE_PLANKS_SLAB,         // 48
  BIRCH_PLANKS_SLAB,          // 49
  ACACIA_PLANKS_SLAB,         // 50
  STONE_BRICK_SLAB,           // 51
  CRACKED_STONE_BRICKS_SLAB,  // 52
  MOSSY_STONE_BRICKS_SLAB,    // 53

  // Helper index
  TOTAL_OF_BLOCKS  // 54
};
/*---------------------------------------------*/

#define FIRST_PERSON_CAM 1
#define THIRD_PERSON_CAM 2

#define MAX_RANGE_PICKER \
  (DOUBLE_BLOCK_SIZE * 6.0F)  // How far the player can pick a block

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
#define DEFAULT_TICK_SPEED 1

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

enum class TargetedFace {
  TopFace,
  BottomFace,
  LeftFace,
  RightFace,
  FrontFace,
  BackFace,
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

enum class MobCategory { Passive, Neutral, Hostile, Invalid };

enum class MobType { Pig, Cow, Invalid };

enum class ParticleType { Block, Flame, Smoke };

#define HOT_INVENTORY_SIZE 9

#define MIN_WORLD_POS                             \
  Vec4(OVERWORLD_MIN_DISTANCE* DOUBLE_BLOCK_SIZE, \
       OVERWORLD_MIN_HEIGH* DOUBLE_BLOCK_SIZE,    \
       OVERWORLD_MIN_DISTANCE* DOUBLE_BLOCK_SIZE)

#define MAX_WORLD_POS                                    \
  Vec4((OVERWORLD_MAX_DISTANCE - 1) * DOUBLE_BLOCK_SIZE, \
       (OVERWORLD_MAX_HEIGH - 1) * DOUBLE_BLOCK_SIZE,    \
       (OVERWORLD_MAX_DISTANCE - 1) * DOUBLE_BLOCK_SIZE)

#define CENTER_WORLD_POS (MAX_WORLD_POS + MIN_WORLD_POS) / 2

#define FIXED_15_FRAME_MS 0.06666666667F
#define FIXED_30_FRAME_MS 0.03333333334F
#define FIXED_60_FRAME_MS 0.01666666667F
#define FIXED_120_FRAME_MS 0.00833333334F

#define MAX_ADPCM_CH 23

typedef enum {
  WORLD_TYPE_ORIGINAL = 0,
  WORLD_TYPE_FLAT = 1,
  WORLD_TYPE_ISLAND = 2,
  WORLD_TYPE_WOODS = 3,
  WORLD_TYPE_FLOATING = 4,
  WORLD_MINI_GAME_MAZECRAFT = 5
} WorldType;

#define SAVE_FILE_EXTENSION "tcw"
#define MINIGAME_FILE_EXTENSION "mgw"

// Blocks params
#define BREAKING_TIME_IN_CREATIVE_MODE 0.150F

#define TORCH_UV_COUNTER 20

#define MAX_SAFE_MEMORY_ALLOCATION 29

// ===== Post-FX VRAM Constants =====
// PS2 total VRAM: 4MB
#define PS2_VRAM_SIZE_BYTES (4 * 1024 * 1024)

// Fog CLUT size: 8KB reserved at end of VRAM
// (256 colors × 4 bytes = 1KB actual, but 8KB allocated for alignment/safety)
#define FOG_CLUT_SIZE_BYTES (8 * 1024)

// Half-resolution buffer size for bloom temp buffers
// 256×224×4 bytes/pixel = 229,376 bytes = 57,344 words (32-bit word units)
// Used for tempBufA and tempBufB in bloom effect
#define HALF_RES_BUFFER_SIZE_WORDS ((256 * 224 * 4) / 4)
// =====================================

#define DOWN_VEC Vec4(0, -1, 0)
#define UP_VEC Vec4(0, 1, 0)
#define RIGHT_VEC Vec4(-1, 0, 0)
#define LEFT_VEC Vec4(1, 0, 0)
#define FRONT_VEC Vec4(0, 0, -1)
#define BACK_VEC Vec4(0, 0, 1)

#define BLOCK_SIZE_VEC Vec4(BLOCK_SIZE, BLOCK_SIZE, BLOCK_SIZE)
#define DOUBLE_BLOCK_SIZE_VEC \
  Vec4(DOUBLE_BLOCK_SIZE, DOUBLE_BLOCK_SIZE, DOUBLE_BLOCK_SIZE)

enum class GameMode { Survival, Creative, Maze };

enum class MenuAction { Save };

// Day/Night cycle global settings
#define DAY_NIGHT_TICKS_UPDATE 100
#define CLOUDS_TICKS_UPDATE 150
