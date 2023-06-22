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
#include "managers/settings_manager.hpp"

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

void Camera::update() {
  unitCirclePosition.x = Math::cos(Utils::degreesToRadian(yaw)) *
                         Math::cos(Utils::degreesToRadian(pitch));
  unitCirclePosition.y = Math::sin(Utils::degreesToRadian(pitch));
  unitCirclePosition.z = Math::sin(Utils::degreesToRadian(yaw)) *
                         Math::cos(Utils::degreesToRadian(pitch));

  lookPos.set(unitCirclePosition + position);
}

void Camera::setPositionByMesh(Mesh* t_mesh) {
  position.set(*t_mesh->getPosition() -
               (unitCirclePosition.getNormalized() * 4.5F));
  position.y += CAMERA_Y;
}

void Camera::setLookDirectionByPad(Pad* t_pad) {
  const auto& rightJoy = t_pad->getRightJoyPad();

  const auto _h = (rightJoy.h - 128.0F) / 128.0F;
  const auto _v = (rightJoy.v - 128.0F) / 128.0F;
  Vec4 sensibility = Vec4(std::abs(_h) > g_settings.r_stick_H ? _h : 0.0F, 0.0F,
                          std::abs(_v) > g_settings.r_stick_V ? _v : 0.0F);

  if (sensibility.length() > 0) {
    yaw += camSpeed * sensibility.x;
    pitch += camSpeed * (-sensibility.z);

    if (pitch > 89.0F) pitch = 89.0F;
    if (pitch < -89.0F) pitch = -89.0F;
  }
}

void Camera::reset() {
  yaw = 0;
  pitch = 0;
  update();
}
