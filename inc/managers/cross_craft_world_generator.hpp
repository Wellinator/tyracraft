#pragma once

#include <constants.hpp>
#include <stdint.h>
#include <stdbool.h>
#include "entities/level.hpp"
#include <math.h>
#include <stdlib.h>
#include <time.h>
#include <tyra>

void CrossCraft_WorldGenerator_Init(int32_t seed);
void CrossCraft_WorldGenerator_Generate_Original(Level* pLevel);
void CrossCraft_WorldGenerator_Generate_Flat(Level* pLevel);
void CrossCraft_WorldGenerator_Generate_Woods(Level* pLevel);
void CrossCraft_WorldGenerator_Generate_Island(Level* pLevel);
void CrossCraft_WorldGenerator_Generate_Floating(Level* pLevel);
void CrossCraft_WorldGenerator_Generate_Maze(Level* pLevel);