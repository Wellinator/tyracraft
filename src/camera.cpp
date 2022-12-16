/*
# ______       ____   ___
#   |     \/   ____| |___|
#   |     |   |   \  |   |
#-----------------------------------------------------------------------
# Copyright 2020, tyra - https://github.com/h4570/tyra
# Licenced under Apache License 2.0
# Sandro Sobczyński <sandro.sobczynski@gmail.com>
*/
#include "camera.hpp"
#include "math/math.hpp"

using Tyra::CameraInfo3D;
using Tyra::Math;
using Tyra::Mesh;
using Tyra::Pad;
using Tyra::Vec4;

// ----
// Constructors/Destructors
// ----

Camera::Camera(const RendererSettings& t_screen) {}

Camera::~Camera() {}

// ----
// Methods
// ----

void Camera::update(Pad& pad, Mesh& t_mesh) {
  const auto& rightJoy = pad.getRightJoyPad();
  position.set(*t_mesh.getPosition() -
               (unitCirclePosition.getNormalized() * BLOCK_SIZE));
  position.y += CAMERA_Y;

  Vec4 sensibility = Vec4((rightJoy.h - 128.0F) / 128.0F, 0.0F,
                          (rightJoy.v - 128.0F) / 128.0F);

  if (rightJoy.isMoved && sensibility.length() >= R_JOYPAD_DEAD_ZONE) {
    yaw += camSpeed * sensibility.x;
    pitch += camSpeed * (-sensibility.z);

    // if (rightJoy.h <= 100)
    //   yaw -= camSpeed * sensibility.x;
    // else if (rightJoy.h >= 200)
    //   yaw += camSpeed * sensibility.x;
    // else if (rightJoy.v <= 100)
    //   pitch += camSpeed * sensibility.z;
    // else if (rightJoy.v >= 200)
    //   pitch -= camSpeed * sensibility.z;

    if (pitch > 89.0F) pitch = 89.0F;
    if (pitch < -89.0F) pitch = -89.0F;
  }

  unitCirclePosition.x = Math::cos(Utils::degreesToRadian(yaw)) *
                         Math::cos(Utils::degreesToRadian(pitch));
  unitCirclePosition.y = Math::sin(Utils::degreesToRadian(pitch));
  unitCirclePosition.z = Math::sin(Utils::degreesToRadian(yaw)) *
                         Math::cos(Utils::degreesToRadian(pitch));

  lookPos.set(unitCirclePosition + position);
}
