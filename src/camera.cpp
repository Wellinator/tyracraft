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
  unitCirclePosition.z = Math::sin(Utils::degreesToRadian(yaw)) *
                         Math::cos(Utils::degreesToRadian(pitch));

  if (g_settings.invert_cam_y) {
    unitCirclePosition.y = Math::sin(Utils::degreesToRadian(-pitch));
  } else {
    unitCirclePosition.y = Math::sin(Utils::degreesToRadian(pitch));
  }

  // if (camera_type == CamType::FirstPerson) {
  //   unitCirclePosition.y = Math::sin(Utils::degreesToRadian(pitch));
  // } else {
  //   unitCirclePosition.y = Math::sin(Utils::degreesToRadian(pitch));
  // }

  lookPos.set(unitCirclePosition + position);
}

void Camera::setPositionByMesh(Mesh* t_mesh) {
  if (camera_type == CamType::FirstPerson) {
    position.set(*t_mesh->getPosition() -
                 (unitCirclePosition.getNormalized() * 4.5F));
    position.y += CAMERA_Y;
  } else {
    const float hDistance = calculateHorizontalDistance();
    const float vDistance = calculateVerticalDistance();

    if (g_settings.invert_cam_y) {
      calculateCameraPosition(t_mesh, hDistance, vDistance);
    } else {
      calculateCameraPosition(t_mesh, hDistance, -vDistance);
    }
  }
}

void Camera::setLookDirectionByPad(Pad* t_pad, const float deltatime) {
  calculatePitch(t_pad, deltatime);
  calculateYaw(t_pad, deltatime);
}

void Camera::reset() {
  yaw = 0;
  pitch = 0;
  update();
}

void Camera::setFirstPerson() { camera_type = CamType::FirstPerson; }

void Camera::setThirdPerson() { camera_type = CamType::ThirdPerson; }

void Camera::setThirdPersonInverted() {
  camera_type = CamType::ThirdPersonInverted;
}

void Camera::calculatePitch(Pad* t_pad, const float deltatime) {
  const auto& rightJoy = t_pad->getRightJoyPad();
  const auto _v = (rightJoy.v - 128.0F) / 128.0F;
  const auto tempPitch = std::abs(_v) > g_settings.r_stick_V ? _v : 0.0F;

  pitch += g_settings.cam_v_sensitivity * deltatime * -tempPitch;

  if (pitch > 89.0F) pitch = 89.0F;
  if (pitch < -89.0F) pitch = -89.0F;
}

void Camera::calculateYaw(Pad* t_pad, const float deltatime) {
  const auto& rightJoy = t_pad->getRightJoyPad();
  const auto _h = (rightJoy.h - 128.0F) / 128.0F;
  const auto tempYaw = std::abs(_h) > g_settings.r_stick_H ? _h : 0.0F;

  yaw += g_settings.cam_h_sensitivity * deltatime * tempYaw;
}

float Camera::calculateHorizontalDistance() {
  const float maxDist = camera_type == CamType::FirstPerson
                            ? distanceFromPlayer
                            : std::min(distanceFromPlayer, hitDistance);
  return maxDist * Tyra::Math::cos(Tyra::Math::ANG2RAD * pitch);
}

float Camera::calculateVerticalDistance() {
  const float maxDist = camera_type == CamType::FirstPerson
                            ? distanceFromPlayer
                            : std::min(distanceFromPlayer, hitDistance);
  return maxDist * Tyra::Math::sin(Tyra::Math::ANG2RAD * pitch);
}

void Camera::calculateCameraPosition(Mesh* t_mesh,
                                     const float horizontalDistance,
                                     const float verticalDistance) {
  const float theta = yaw;
  const float offsetX =
      horizontalDistance * Tyra::Math::cos(Tyra::Math::ANG2RAD * theta);
  const float offsetZ =
      horizontalDistance * Tyra::Math::sin(Tyra::Math::ANG2RAD * theta);

  const Vec4 playerPos = *t_mesh->getPosition();
  position.set(playerPos.x - offsetX, playerPos.y + CAMERA_Y + verticalDistance,
               playerPos.z - offsetZ);
}
