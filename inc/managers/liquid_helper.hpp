#pragma once
#include <tamtypes.h>
#include "tyra"
#include "constants.hpp"
#include "models/liquid_quad_map_model.hpp"
#include "entities/level.hpp"

LiquidQuadMapModel LiquidHelper_getQuadMap(LevelMap* t_terrain,
                                           BlockOrientation orientation,
                                           Vec4* offset, u8 liquid_type);

float LiquidHelper_getNW(LevelMap* t_terrain, Vec4* offset, u8 liquid_type);
float LiquidHelper_getNE(LevelMap* t_terrain, Vec4* offset, u8 liquid_type);
float LiquidHelper_getSE(LevelMap* t_terrain, Vec4* offset, u8 liquid_type);
float LiquidHelper_getSW(LevelMap* t_terrain, Vec4* offset, u8 liquid_type);

float LiquidHelper_getWaterHeightByVolume(u8 liquid_volume);
float LiquidHelper_getLavaHeightByVolume(u8 liquid_volume);