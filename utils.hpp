/*
# ______       ____   ___
#   |     \/   ____| |___|
#   |     |   |   \  |   |
#-----------------------------------------------------------------------
# Copyright 2020, tyra - https://github.com/h4570/tyra
# Licenced under Apache License 2.0
# Sandro Sobczyński <sandro.sobczynski@gmail.com>
*/

#ifndef _TYRA_UTILS_
#define _TYRA_UTILS_

#include <tamtypes.h>
#include <models/mesh.hpp>
#include <models/math/vector3.hpp>
#include <models/math/vector3.hpp>
#include <utils/math.hpp>

class Utils
{

public:
    static float degreesToRadian(float degress);
    static float expoEaseInOut(float t, float b, float c, float d);
    static void getMinMax(const Mesh &t_mesh, Vector3 &t_min, Vector3 &t_max);
    static float clamp(const float value, float min, float max);
    static float FOG_LINEAR(float d, float start, float end, float offset);
    static float FOG_EXP(float d, float density);
    static float FOG_EXP2(float d, float density);
    static float FOG_EXP_GRAD(float d, float density, float gradient);
};

#endif
