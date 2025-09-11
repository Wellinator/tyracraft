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
#include "managers/block/vertex_block_data.hpp"
#include "managers/block_manager.hpp"
#include "managers/model_builder.hpp"
#include "entities/level.hpp"
#include "entities/entity.hpp"
#include "entities/Block.hpp"
#include "3libs/bvh/bvh.h"
#include "utils.hpp"

#define TWO_PI 6.28318530717958647692F

using bvh::AABB;
using bvh::AABBTree;
using bvh::Bvh_Node;
using bvh::index_t;
using Tyra::BBox;
using Tyra::CameraInfo3D;
using Tyra::Math;
using Tyra::Mesh;
using Tyra::Pad;
using Tyra::Vec4;

// ----
// Constructors/Destructors
// ----

Camera::Camera(const RendererSettings& t_screen) : Singleton<Camera>() {
  // Initialize with safe default values first
  pitch = yaw = targetPitch = targetYaw = smoothPitch = smoothYaw = 0.0f;

  position = Vec4(0.0f, CAMERA_Y, 0.0f);
  targetPosition.set(position);
  smoothPosition.set(position);

  looksAt = Vec4(0.0f, CAMERA_Y, 1.0f);
  targetLooksAt.set(looksAt);
  smoothLooksAt.set(looksAt);

  // Then call initialization to ensure consistency
  initializeSmoothValues();
}

Camera::~Camera() {}

// ----
// Methods
// ----

void Camera::update() {
  const float effectivePitch =
      smoothPitch + bobPitchOffset;  // apply bob pitch only to view calc
  unitCirclePosition.x = Math::cos(Utils::degreesToRadian(smoothYaw)) *
                         Math::cos(Utils::degreesToRadian(effectivePitch));
  unitCirclePosition.z = Math::sin(Utils::degreesToRadian(smoothYaw)) *
                         Math::cos(Utils::degreesToRadian(effectivePitch));

  if (g_settings.invert_cam_y) {
    unitCirclePosition.y = Math::sin(Utils::degreesToRadian(-effectivePitch));
  } else {
    unitCirclePosition.y = Math::sin(Utils::degreesToRadian(effectivePitch));
  }

  unitCirclePosition.normalize();

  looksAt.set(unitCirclePosition + smoothPosition + bobCurrentOffset);
}

void Camera::update(const float& deltaTime, const u8 isWalking) {
  // Apply smooth movement (base rotation + base position)
  applySmoothMovement(deltaTime);

  const float effectivePitch = smoothPitch + bobPitchOffset;
  unitCirclePosition.x = Math::cos(Utils::degreesToRadian(smoothYaw)) *
                         Math::cos(Utils::degreesToRadian(effectivePitch));
  unitCirclePosition.z = Math::sin(Utils::degreesToRadian(smoothYaw)) *
                         Math::cos(Utils::degreesToRadian(effectivePitch));

  if (g_settings.invert_cam_y) {
    unitCirclePosition.y = Math::sin(Utils::degreesToRadian(-effectivePitch));
  } else {
    unitCirclePosition.y = Math::sin(Utils::degreesToRadian(effectivePitch));
  }

  unitCirclePosition.normalize();

  // Handle camera bob (first & third person) with new system
  shakeCamera(deltaTime, isWalking);

  looksAt.set(unitCirclePosition + smoothPosition + bobCurrentOffset);
}

void Camera::setPosition(Vec4 newPosition) {
  if (camera_type == CamType::FirstPerson) {
    targetPosition.set(newPosition);
    targetPosition.y += CAMERA_Y;
  } else {
    hitDistance = distanceFromPlayer;

    const Vec4 segmentStart = newPosition + Vec4(0.0f, getCamY(), 0.0f);
    const Vec4 segmentEnd =
        segmentStart + (-unitCirclePosition * distanceFromPlayer);

    Ray revRay;
    revRay.origin.set(segmentStart);
    revRay.direction.set(-unitCirclePosition);

    // Broad phase raycast
    std::vector<LevelIntersectQueryResult> tempResult = {};
    Level::getInstance()->getIntersectedBlocks(segmentStart, segmentEnd,
                                               &tempResult);

    // Narrow phase raycast
    for (const auto& result : tempResult) {
      Block* targetTemplate =
          BlockManager::getInstance()->getBlockTemplateByType(
              static_cast<Blocks>(result.blockType));

      if (targetTemplate->isCollidable()) {
        BBox* rawBBox = VertexBlockData::getRawBBoxByOffset(
            const_cast<Vec4*>(&result.offset));
        M4x4 model = ModelBuilder_BuildModel(const_cast<Vec4*>(&result.offset));
        BBox blockBBox = rawBBox->getTransformed(model);

        Vec4 min, max;
        blockBBox.getMinMax(&min, &max);

        float intersectionPoint;
        if (revRay.intersectBox(min, max, &intersectionPoint)) {
          if (intersectionPoint < hitDistance) {
            hitDistance = intersectionPoint * 0.95F;
          }
        }
      }
    }

    const float hDistance = calculateHorizontalDistance();
    const float vDistance = calculateVerticalDistance();

    Vec4 calculatedPosition;
    if (g_settings.invert_cam_y) {
      const float theta = smoothYaw;
      const float offsetX =
          hDistance * Tyra::Math::cos(Tyra::Math::ANG2RAD * theta);
      const float offsetZ =
          hDistance * Tyra::Math::sin(Tyra::Math::ANG2RAD * theta);
      calculatedPosition.set(newPosition.x - offsetX,
                             newPosition.y + CAMERA_Y + vDistance,
                             newPosition.z - offsetZ);
    } else {
      const float theta = smoothYaw;
      const float offsetX =
          hDistance * Tyra::Math::cos(Tyra::Math::ANG2RAD * theta);
      const float offsetZ =
          hDistance * Tyra::Math::sin(Tyra::Math::ANG2RAD * theta);
      calculatedPosition.set(newPosition.x - offsetX,
                             newPosition.y + CAMERA_Y + (-vDistance),
                             newPosition.z - offsetZ);
    }

    // Set target position directly to calculated position to avoid teleport
    targetPosition.set(calculatedPosition);
    position.set(calculatedPosition);  // Keep for compatibility
  }
}

void Camera::setLookDirectionByPad(Pad* t_pad, const float deltatime) {
  calculatePitch(t_pad, deltatime);
  calculateYaw(t_pad, deltatime);
}

void Camera::reset() {
  yaw = 0;
  pitch = 0;
  targetYaw = 0;
  targetPitch = 0;
  smoothYaw = 0;
  smoothPitch = 0;
  bobPhase = 0.0F;
  bobTilt = 0.0F;
  bobTargetTilt = 0.0F;
  bobPitchOffset = 0.0F;
  bobCurrentOffset.set(0.0F, 0.0F, 0.0F);
  bobTargetOffset.set(0.0F, 0.0F, 0.0F);
  update();
}

void Camera::setFirstPerson() {
  camera_type = CamType::FirstPerson;
  // Only sync current position, don't reset smooth values
  targetPosition = position;
}

void Camera::setThirdPerson() {
  camera_type = CamType::ThirdPerson;
  // Only sync current position, don't reset smooth values
  targetPosition = position;
}

void Camera::setThirdPersonInverted() {
  camera_type = CamType::ThirdPersonInverted;
  // Only sync current position, don't reset smooth values
  targetPosition = position;
}

void Camera::calculatePitch(Pad* t_pad, const float deltatime) {
  const auto& rightJoy = t_pad->getRightJoyPad();
  const auto _v = (rightJoy.v - 128.0F) / 128.0F;
  const auto tempPitch = Utils::Abs(_v) > g_settings.r_stick_V ? _v : 0.0F;

  targetPitch += g_settings.cam_v_sensitivity * deltatime * -tempPitch;

  if (targetPitch > 89.0F) targetPitch = 89.0F;
  if (targetPitch < -89.0F) targetPitch = -89.0F;
}

void Camera::calculateYaw(Pad* t_pad, const float deltatime) {
  const auto& rightJoy = t_pad->getRightJoyPad();
  const auto _h = (rightJoy.h - 128.0F) / 128.0F;
  const auto tempYaw = Utils::Abs(_h) > g_settings.r_stick_H ? _h : 0.0F;

  targetYaw += g_settings.cam_h_sensitivity * deltatime * tempYaw;

  if (targetYaw < 0.0f) {
    targetYaw = 360.0F + targetYaw;
  } else if (targetYaw > 360.0f) {
    targetYaw = targetYaw - 360.0F;
  }
}

float Camera::calculateHorizontalDistance() {
  const float maxDist = camera_type == CamType::FirstPerson
                            ? distanceFromPlayer
                            : std::min(distanceFromPlayer, hitDistance);
  return maxDist * Tyra::Math::cos(Tyra::Math::ANG2RAD * smoothPitch);
}

float Camera::calculateVerticalDistance() {
  const float maxDist = camera_type == CamType::FirstPerson
                            ? distanceFromPlayer
                            : std::min(distanceFromPlayer, hitDistance);
  return maxDist * Tyra::Math::sin(Tyra::Math::ANG2RAD * smoothPitch);
}

void Camera::calculateCameraPosition(Vec4* newPosition,
                                     const float horizontalDistance,
                                     const float verticalDistance) {
  const float theta = smoothYaw;
  const float offsetX =
      horizontalDistance * Tyra::Math::cos(Tyra::Math::ANG2RAD * theta);
  const float offsetZ =
      horizontalDistance * Tyra::Math::sin(Tyra::Math::ANG2RAD * theta);

  position.set(newPosition->x - offsetX,
               newPosition->y + CAMERA_Y + verticalDistance,
               newPosition->z - offsetZ);
}

void Camera::shakeCamera(const float deltaTime, const bool isWalking) {
  // Smooth deactivation path
  auto fadeOutAll = [&](const float lerpBase) {
    const float f = 1.0F - exp(-lerpBase * deltaTime);
    bobTargetOffset.set(0.0F, 0.0F, 0.0F);
    bobCurrentOffset = Vec4::getByLerp(bobCurrentOffset, bobTargetOffset, f);
    bobTargetTilt = 0.0F;
    bobTilt = bobTilt + (bobTargetTilt - bobTilt) * f;
    bobPitchOffset = bobPitchOffset + ((bobTilt * 0.02F) - bobPitchOffset) * f;
  };

  if (!bobEnabled) {
    fadeOutAll(10.0F);
    return;
  }
  if (!isWalking) {
    fadeOutAll(8.0F);
    return;
  }

  // Advance independent phase
  // Slightly slower base speed for softer motion
  const float baseSpeed = (camera_type == CamType::FirstPerson) ? 6.2F : 5.2F;
  bobPhase += deltaTime * baseSpeed;
  if (bobPhase > TWO_PI) bobPhase -= TWO_PI;

  // Intensity scaling
  const float modeScale = (camera_type == CamType::FirstPerson) ? 0.9F : 0.55F;
  const float intensity = bobIntensityMultiplier * modeScale;

  // Waves (no ABS; use remap for vertical)
  const float w1 = Math::sin(bobPhase);         // sway & tilt
  const float w2 = Math::sin(bobPhase * 2.0F);  // step pattern
  const float w3 = Math::sin(bobPhase * 1.5F);  // subtle depth

  // Vertical: remap [-1,1] -> [0,1] smoothly to avoid cusp: (x*0.5 + 0.5)
  const float verticalNorm = (w2 * 0.5F + 0.5F);

  // Target offsets
  // Further reduced amplitudes for subtle motion
  const float side = w1 * 0.015F * intensity;    // horizontal sway
  const float up = verticalNorm * 0.01F * intensity;   // vertical bounce
  const float depth = w3 * 0.008F * intensity;   // depth motion

  // Orientation basis
  const float yawRad = Utils::degreesToRadian(smoothYaw);
  const float sY = Math::sin(yawRad);
  const float cY = Math::cos(yawRad);
  Vec4 right(-sY, 0.0F, cY);
  Vec4 forward(cY, 0.0F, sY);
  Vec4 upVec(0.0F, 1.0F, 0.0F);

  bobTargetOffset = (right * side) + (upVec * up) + (forward * depth);

  // Smooth offset
  const float offLerp = 1.0F - exp(-14.0F * deltaTime);
  bobCurrentOffset =
      Vec4::getByLerp(bobCurrentOffset, bobTargetOffset, offLerp);

  // Tilt (first person only)
  float desiredTiltDeg = 0.0F;
  if (camera_type == CamType::FirstPerson)
    desiredTiltDeg = w1 * 0.35F * intensity; // further reduced tilt amplitude
  bobTargetTilt = desiredTiltDeg;
  const float tiltLerp = 1.0F - exp(-12.0F * deltaTime);
  bobTilt = bobTilt + (bobTargetTilt - bobTilt) * tiltLerp;

  // Pitch offset derived from tilt (decoupled from base smoothing)
  const float targetPitchOffset = bobTilt * 0.02F; // further reduce pitch effect
  const float pitchLerp = 1.0F - exp(-16.0F * deltaTime);
  bobPitchOffset =
      bobPitchOffset + (targetPitchOffset - bobPitchOffset) * pitchLerp;
  const float maxBobPitch = 1.0F; // tighter clamp for very subtle feel
  if (bobPitchOffset > maxBobPitch) bobPitchOffset = maxBobPitch;
  if (bobPitchOffset < -maxBobPitch) bobPitchOffset = -maxBobPitch;
}

void Camera::applySmoothMovement(const float deltaTime) {
  // Smooth rotation interpolation
  const float rotationLerpFactor =
      1.0f - exp(-smoothFactorRotation * deltaTime);

  // Handle yaw wrapping for shortest rotation path
  float yawDiff = targetYaw - smoothYaw;
  if (yawDiff > 180.0f) {
    yawDiff -= 360.0f;
  } else if (yawDiff < -180.0f) {
    yawDiff += 360.0f;
  }

  smoothYaw += yawDiff * rotationLerpFactor;
  if (smoothYaw < 0.0f) {
    smoothYaw += 360.0f;
  } else if (smoothYaw >= 360.0f) {
    smoothYaw -= 360.0f;
  }

  // Smooth pitch interpolation
  smoothPitch += (targetPitch - smoothPitch) * rotationLerpFactor;

  // Smooth position interpolation with teleport prevention
  const float positionLerpFactor =
      1.0f - exp(-smoothFactorPosition * deltaTime);

  // Calculate distance between current smooth position and target
  Vec4 positionDiff = targetPosition - smoothPosition;
  float distance = positionDiff.length();

  // If distance is too large (indicating a potential teleport), snap to target
  const float MAX_SMOOTH_DISTANCE = 200.0f;  // Adjust based on game scale
  if (distance > MAX_SMOOTH_DISTANCE) {
    smoothPosition = targetPosition;
  } else {
    smoothPosition =
        Vec4::getByLerp(smoothPosition, targetPosition, positionLerpFactor);
  }

  // Update actual values for compatibility
  yaw = smoothYaw;
  pitch = smoothPitch;
  position = smoothPosition;
}

void Camera::initializeSmoothValues() {
  // Sync smooth values with current values
  targetPitch = pitch;
  targetYaw = yaw;
  smoothPitch = pitch;
  smoothYaw = yaw;

  targetPosition = position;
  smoothPosition = position;

  targetLooksAt = looksAt;
  smoothLooksAt = looksAt;
}

void Camera::setSmoothFactors(float rotationFactor, float positionFactor) {
  smoothFactorRotation = rotationFactor;
  smoothFactorPosition = positionFactor;
}

void Camera::resetSmoothMovement() { initializeSmoothValues(); }

void Camera::setBobIntensity(float intensity) {
  bobIntensityMultiplier = intensity;
}

void Camera::setBobEnabled(bool enabled) {
  bobEnabled = enabled;
}
