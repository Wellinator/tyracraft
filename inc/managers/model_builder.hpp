#pragma once

#include "constants.hpp"
#include "entities/Block.hpp"
#include <tyra>
#include <math.h>
#include <vector>
#include "entities/level.hpp"

using Tyra::M4x4;
using Tyra::Vec4;

M4x4 ModelBuilder_BuildModel(Vec4* offset);

M4x4 ModelBuilder_DefaultModel(Vec4* offset);

M4x4 ModelBuilder_NoRotationModel(Vec4* offset);

M4x4 ModelBuilder_TorchModel(Vec4* offset);
