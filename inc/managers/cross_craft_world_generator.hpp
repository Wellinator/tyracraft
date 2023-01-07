#pragma once

#include "3libs/FastNoiseLite/ModdedFastNoiseLite.h"
#include <constants.hpp>
#include <stdint.h>
#include <stdbool.h>
#include "entities/level.hpp"
#include <math.h>
#include <stdlib.h>
#include <time.h>
#include <tyra>

void CrossCraft_WorldGenerator_Init(int32_t seed);
void CrossCraft_WorldGenerator_Generate_Original(LevelMap* map);
void CrossCraft_WorldGenerator_Generate_Flat(LevelMap* map);
void CrossCraft_WorldGenerator_Generate_Woods(LevelMap* map);
void CrossCraft_WorldGenerator_Generate_Island(LevelMap* map);;
void CrossCraft_WorldGenerator_Generate_Floating(LevelMap* map);