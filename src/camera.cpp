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
  if (camera_type == CamType::FirstPerson) {
    unitCirclePosition.x = Math::cos(Utils::degreesToRadian(yaw)) *
                           Math::cos(Utils::degreesToRadian(pitch));
    unitCirclePosition.y = Math::sin(Utils::degreesToRadian(pitch));
    unitCirclePosition.z = Math::sin(Utils::degreesToRadian(yaw)) *
                           Math::cos(Utils::degreesToRadian(pitch));
  } else {
    unitCirclePosition.x = Math::cos(Utils::degreesToRadian(yaw)) *
                           Math::cos(Utils::degreesToRadian(pitch));
    unitCirclePosition.y = Math::sin(Utils::degreesToRadian(-pitch));
    unitCirclePosition.z = Math::sin(Utils::degreesToRadian(yaw)) *
                           Math::cos(Utils::degreesToRadian(pitch));
  }
  lookPos.set(unitCirclePosition + position);
}

void Camera::setPositionByMesh(Mesh* t_mesh) {
  if (camera_type == CamType::FirstPerson) {
    position.set(*t_mesh->getPosition() -
                 (unitCirclePosition.getNormalized() * 4.5F));
    position.y += CAMERA_Y;
  } else {
    // position.set(
    //     *t_mesh->getPosition() -
    //     (unitCirclePosition.getNormalized() * 4.5F - distanceFromPlayer));
    // position.y += CAMERA_Y + 10.0F;
    const float hDistance = calculateHorizontalDistance();
    const float vDistance = calculateVerticalDistance();
    calculateCameraPosition(t_mesh, hDistance, vDistance);
  }
}

void Camera::setLookDirectionByPad(Pad* t_pad) {
  calculatePitch(t_pad);
  calculateYaw(t_pad);
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

void Camera::calculatePitch(Pad* t_pad) {
  const auto& rightJoy = t_pad->getRightJoyPad();
  const auto _v = (rightJoy.v - 128.0F) / 128.0F;
  const auto tempPitch = std::abs(_v) > g_settings.r_stick_V ? _v : 0.0F;

  pitch += camSpeedV * -tempPitch;

  if (pitch > 89.0F) pitch = 89.0F;
  if (pitch < -89.0F) pitch = -89.0F;
}

void Camera::calculateYaw(Pad* t_pad) {
  const auto& rightJoy = t_pad->getRightJoyPad();
  const auto _h = (rightJoy.h - 128.0F) / 128.0F;
  const auto tempYaw = std::abs(_h) > g_settings.r_stick_H ? _h : 0.0F;

  yaw += camSpeedH * tempYaw;
}

float Camera::calculateHorizontalDistance() {
  return distanceFromPlayer * Tyra::Math::cos(Tyra::Math::ANG2RAD * pitch);
}

float Camera::calculateVerticalDistance() {
  return distanceFromPlayer * Tyra::Math::sin(Tyra::Math::ANG2RAD * pitch);
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
