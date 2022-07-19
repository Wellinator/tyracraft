/*
# ______       ____   ___
#   |     \/   ____| |___|    
#   |     |   |   \  |   |       
#-----------------------------------------------------------------------
# Copyright 2020, tyra - https://github.com/h4570/tyra
# Licenced under Apache License 2.0
# Sandro Sobczyński <sandro.sobczynski@gmail.com>
*/

#ifndef _CAMERA_
#define _CAMERA_

#include <pad/pad.hpp>
#include <renderer/3d/mesh/mesh.hpp>
#include <math/vec4.hpp>
#include <renderer/renderer_settings.hpp>
#include <renderer/core/3d/camera_info_3d.hpp>
#include <tamtypes.h>
#include <utils.hpp>
#include <math/math.hpp>
#include "contants.hpp"
#include <debug/debug.hpp>
#include <fastmath.h>

/** 3D camera which follow by 3D object. Can be rotated via pad */
class Camera : public CameraInfo3D
{

public:
    Vec4 up, position, unitCirclePosition, lookPos;
    float horizontalLevel, verticalLevel, pitch, yaw;
    float const _sensitivity = 4.58425F;
    u8 camera_type = FIRST_PERSON_CAM;

    Camera(ScreenSettings *t_screen);
    ~Camera();

    void update(Pad &t_pad, Mesh &t_mesh);
    void rotate(Pad &t_pad);
    void updateUnitCirclePosition();
    void followBy(Mesh &t_mesh);
    void pointCamera(Pad &t_pad, Mesh &t_mesh);

protected:
    Vec4 *getPosition() { return &position; };
    ScreenSettings screen;
};

#endif