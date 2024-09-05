#pragma once

#include <tamtypes.h>
#include "tyra"
#include "constants.hpp"
#include "models/liquid_quad_map_model.hpp"
#include "entities/level.hpp"

LiquidQuadMapModel LiquidHelper_getQuadMap(Level* pLevel,
                                           LiquidOrientation orientation,
                                           Vec4* offset, u8 liquid_type);

float LiquidHelper_getNW(Level* pLevel, Vec4* offset, u8 liquid_type);
float LiquidHelper_getNE(Level* pLevel, Vec4* offset, u8 liquid_type);
float LiquidHelper_getSE(Level* pLevel, Vec4* offset, u8 liquid_type);
float LiquidHelper_getSW(Level* pLevel, Vec4* offset, u8 liquid_type);

float LiquidHelper_getWaterHeightByVolume(u8 liquid_volume);
float LiquidHelper_getLavaHeightByVolume(u8 liquid_volume);
