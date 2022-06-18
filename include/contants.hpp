#ifndef _CONSTANTS_
#define _CONSTANTS_

#define OVERWORLD_H_DISTANCE 256                             // Total horizontal overworld distance;
#define OVERWORLD_V_DISTANCE 128                             // Total vertical overworld distance;
#define HALF_OVERWORLD_H_DISTANCE (OVERWORLD_H_DISTANCE / 2) // Half horizontal overworld distance;
#define HALF_OVERWORLD_V_DISTANCE (OVERWORLD_V_DISTANCE / 2) // Half vertical overworld distance;
#define OVERWORLD_MIN_DISTANCE -HALF_OVERWORLD_H_DISTANCE    // Min overworld width;
#define OVERWORLD_MAX_DISTANCE HALF_OVERWORLD_H_DISTANCE     // Max overworld width;
#define OVERWORLD_MIN_HEIGH -HALF_OVERWORLD_V_DISTANCE       // Min overworld layer for badblock;
#define OVERWORLD_MAX_HEIGH HALF_OVERWORLD_V_DISTANCE        // Max overworld heigh layer;

#define OVERWORLD_SIZE ((OVERWORLD_H_DISTANCE * OVERWORLD_H_DISTANCE) * OVERWORLD_V_DISTANCE)
#define CHUNCK_SIZE 24
#define HALF_CHUNCK_SIZE (CHUNCK_SIZE / 2)
#define BLOCK_SIZE 7.0F
#define DUBLE_BLOCK_SIZE (BLOCK_SIZE * 2.0F)
#define CHUNCK_DISTANCE (HALF_CHUNCK_SIZE * DUBLE_BLOCK_SIZE)

// Define blocks ids baseds on https://minecraft.fandom.com/wiki/;
#define AIR_BLOCK 0
#define STONE_BLOCK 1
#define GRASS_BLOCK 2
#define DIRTY_BLOCK 3
#define WATER_BLOCK 4
#define STRIPPED_OAK_WOOD_BLOCK 5

// Should be updated avery time a new block is added;
#define BLOCKS_COUNTER 5

#define FIRST_PERSON_CAM 1
#define THIRD_PERSON_CAM 2

#define MAX_RANGE_PICKER (DUBLE_BLOCK_SIZE * 8.0F) // How far the player can pick a block

// Game states
#define MAIN_MENU 0
#define IN_GAME 1
#define IN_GAME_MENU 2
#define SPLASH_SCREEN 3

// World constants
#define GRAVITY 0.5F;

enum GAME_MODE
{
    SURVIVAL,
    CREATIVE
};

enum ITEM_TYPES
{
    // None
    empty,

    // Blocks
    bedrock,
    dirt,
    sand,
    stone,
    bricks,
    glass,

    // Ores and Minerals
    coal_ore,
    diamond_ore,
    iron_ore,
    gold_ore,
    redstone_ore,
    emerald_ore,

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
    chiseled_stone_bricks
};

#define INVENTORY_SIZE 9

#endif