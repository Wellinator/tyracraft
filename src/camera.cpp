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
#include "managers/collision_manager.hpp"
#include "3libs/bvh/bvh.h"

// TODO: move to Entity class
#include "entities/Block.hpp"

using bvh::AABB;
using bvh::AABBTree;
using bvh::Bvh_Node;
using bvh::index_t;
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

  unitCirclePosition.normalize();

  lookPos.set(unitCirclePosition + position);
}

void Camera::update(const float& deltaTime, const u8 isWalking) {
  camera_time += deltaTime * 5.0F;

  unitCirclePosition.x = Math::cos(Utils::degreesToRadian(yaw)) *
                         Math::cos(Utils::degreesToRadian(pitch));
  unitCirclePosition.z = Math::sin(Utils::degreesToRadian(yaw)) *
                         Math::cos(Utils::degreesToRadian(pitch));

  if (g_settings.invert_cam_y) {
    unitCirclePosition.y = Math::sin(Utils::degreesToRadian(-pitch));
  } else {
    unitCirclePosition.y = Math::sin(Utils::degreesToRadian(pitch));
  }

  unitCirclePosition.normalize();

  if (isWalking) {
    shakeCamera();
  } else {
    camera_time = 0;
  }

  lookPos.set(unitCirclePosition + position);
}

// TODO: refactore method signature to receive the position directly
void Camera::setPosition(Vec4 newPosition) {
  if (camera_type == CamType::FirstPerson) {
    position.set(newPosition - (unitCirclePosition.getNormalized() * 4.5F));
    position.y += CAMERA_Y;
  } else {
    hitDistance = distanceFromPlayer;

    const Vec4 segmentStart = newPosition + Vec4(0.0f, getCamY(), 0.0f);
    const Vec4 segmentEnd = -unitCirclePosition * distanceFromPlayer;

    revRay.origin.set(segmentStart);
    revRay.direction.set(-unitCirclePosition);

    std::vector<index_t> ni;
    g_AABBTree->intersectLine(segmentStart, segmentEnd, ni);

    for (u16 i = 0; i < ni.size(); i++) {
      Entity* entity = (Entity*)g_AABBTree->user_data(ni[i]);

      if (!entity->isCollidable) continue;

      float intersectionPoint;
      if (revRay.intersectBox(entity->minCorner, entity->maxCorner,
                              &intersectionPoint) &&
          intersectionPoint < hitDistance) {
        hitDistance = intersectionPoint * 0.95F;
      }
    }

    const float hDistance = calculateHorizontalDistance();
    const float vDistance = calculateVerticalDistance();

    if (g_settings.invert_cam_y) {
      calculateCameraPosition(&newPosition, hDistance, vDistance);
    } else {
      calculateCameraPosition(&newPosition, hDistance, -vDistance);
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
  const auto tempPitch = Utils::Abs(_v) > g_settings.r_stick_V ? _v : 0.0F;

  pitch += g_settings.cam_v_sensitivity * deltatime * -tempPitch;

  if (pitch > 89.0F) pitch = 89.0F;
  if (pitch < -89.0F) pitch = -89.0F;
}

void Camera::calculateYaw(Pad* t_pad, const float deltatime) {
  const auto& rightJoy = t_pad->getRightJoyPad();
  const auto _h = (rightJoy.h - 128.0F) / 128.0F;
  const auto tempYaw = Utils::Abs(_h) > g_settings.r_stick_H ? _h : 0.0F;

  yaw += g_settings.cam_h_sensitivity * deltatime * tempYaw;

  if (yaw < 0.0f) {
    yaw = 360.0F - yaw;
  } else if (yaw > 360.0f) {
    yaw = yaw - 360.0F;
  }
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

void Camera::calculateCameraPosition(Vec4* newPosition,
                                     const float horizontalDistance,
                                     const float verticalDistance) {
  const float theta = yaw;
  const float offsetX =
      horizontalDistance * Tyra::Math::cos(Tyra::Math::ANG2RAD * theta);
  const float offsetZ =
      horizontalDistance * Tyra::Math::sin(Tyra::Math::ANG2RAD * theta);

  position.set(newPosition->x - offsetX,
               newPosition->y + CAMERA_Y + verticalDistance,
               newPosition->z - offsetZ);
}

void Camera::shakeCamera() {
  M4x4 rot = M4x4::Identity;
  rot.rotateY(_90DEGINRAD);

  Vec4 orientatioFix = Vec4(Math::cos(Utils::degreesToRadian(yaw)), 1.0f,
                            Math::sin(Utils::degreesToRadian(yaw)));

  const float dH = Math::sin(camera_time);
  const float dV = Math::sin(Math::HALF_PI - 2 * camera_time);
  Vec4 offset_factor = Vec4(dH, dV, dH) * (rot * orientatioFix);

  const auto _result =
      Vec4::getByLerp(position + offset_factor, position, 0.4f);
  position.set(_result);
}
