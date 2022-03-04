#ifndef _CONSTANTS_
#define _CONSTANTS_

#define OVERWORLD_H_DISTANCE 256                           // 863 // Max horizontal overworld distance (Same as PS3 Edition);
#define OVERWORLD_V_DISTANCE 64                            // Max vertical overworld distance (Same as 1.17 Edition);
#define HALF_OVERWORLD_H_DISTANCE OVERWORLD_H_DISTANCE / 2 // 256 // 863 // Max horizontal overworld distance (Same as PS3 Edition);
#define HALF_OVERWORLD_V_DISTANCE OVERWORLD_V_DISTANCE / 2 // Max vertical overworld distance (Same as 1.17 Edition);
#define OVERWORLD_MIN_DISTANCE -HALF_OVERWORLD_H_DISTANCE  // Min overworld width;
#define OVERWORLD_MAX_DISTANCE HALF_OVERWORLD_H_DISTANCE   // Max overworld width;
#define OVERWORLD_MIN_HEIGH -HALF_OVERWORLD_V_DISTANCE     // Min overworld layer for badblock;
#define OVERWORLD_MAX_HEIGH HALF_OVERWORLD_V_DISTANCE      // Max overworld heigh layer;

#define OVERWORLD_SIZE OVERWORLD_H_DISTANCE *OVERWORLD_H_DISTANCE *OVERWORLD_V_DISTANCE
#define CHUNCK_SIZE 8
#define HALF_CHUNCK_SIZE CHUNCK_SIZE / 2
#define BLOCK_SIZE 7.0F

#define AIR_BLOCK 0
#define DIRTY_BLOCK 1
#define GRASS_BLOCK 2
#define STONE_BLOCK 3
#define WATER_BLOCK 4

#endif