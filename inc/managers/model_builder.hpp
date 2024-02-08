#pragma once

#include "constants.hpp"
#include "entities/Block.hpp"
#include <tyra>
#include <math.h>
#include <vector>
#include "entities/level.hpp"

using Tyra::M4x4;
using Tyra::Vec4;

void ModelBuilder_BuildModel(Block* t_block, LevelMap* t_terrain);

void ModelBuilder_DefaultModel(Block* t_block, LevelMap* t_terrain);

void ModelBuilder_NoRotationModel(Block* t_block);

void ModelBuilder_TorchModel(Block* t_block, LevelMap* t_terrain);
