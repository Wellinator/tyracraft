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

// TWO_PI now defined in header as TWO_PI_CONST

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
  const float effectivePitch = smoothPitch + bobPitchOffset;

  // Cache trigonometric calculations
  const float yawRad = Utils::degreesToRadian(smoothYaw);
  const float pitchRad = Utils::degreesToRadian(effectivePitch);
  const float cosYaw = Math::cos(yawRad);
  const float sinYaw = Math::sin(yawRad);
  const float cosPitch = Math::cos(pitchRad);
  const float sinPitch = Math::sin(pitchRad);

  unitCirclePosition.x = cosYaw * cosPitch;
  unitCirclePosition.z = sinYaw * cosPitch;

  if (g_settings.invert_cam_y) {
    unitCirclePosition.y = -sinPitch;
  } else {
    unitCirclePosition.y = sinPitch;
  }

  // Unit circle position is already normalized by construction
  // unitCirclePosition.normalize(); // Removed redundant normalization

  looksAt.set(unitCirclePosition + smoothPosition + bobCurrentOffset);
}

void Camera::update(const float& deltaTime, const u8 isWalking) {
  // Clamp deltatime to prevent death loop when FPS drops
  const float clampedDeltaTime = (deltaTime > MAX_DELTA_TIME) ? MAX_DELTA_TIME : deltaTime;
  
  // Apply smooth movement (base rotation + base position)
  applySmoothMovement(clampedDeltaTime);

  // Handle camera bob before calculating unit circle position
  shakeCamera(clampedDeltaTime, isWalking);

  const float effectivePitch = smoothPitch + bobPitchOffset;

  // Cache trigonometric calculations
  const float yawRad = Utils::degreesToRadian(smoothYaw);
  const float pitchRad = Utils::degreesToRadian(effectivePitch);
  const float cosYaw = Math::cos(yawRad);
  const float sinYaw = Math::sin(yawRad);
  const float cosPitch = Math::cos(pitchRad);
  const float sinPitch = Math::sin(pitchRad);

  unitCirclePosition.x = cosYaw * cosPitch;
  unitCirclePosition.z = sinYaw * cosPitch;

  if (g_settings.invert_cam_y) {
    unitCirclePosition.y = -sinPitch;
  } else {
    unitCirclePosition.y = sinPitch;
  }

  // Unit circle position is already normalized by construction
  // unitCirclePosition.normalize(); // Removed redundant normalization

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
  // Clamp deltatime to prevent death loop when FPS drops
  const float clampedDeltaTime = (deltatime > MAX_DELTA_TIME) ? MAX_DELTA_TIME : deltatime;
  calculatePitch(t_pad, clampedDeltaTime);
  calculateYaw(t_pad, clampedDeltaTime);
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

  position = Vec4(0.0f, CAMERA_Y, 0.0f);
  targetPosition.set(position);
  smoothPosition.set(position);

  looksAt = Vec4(0.0f, CAMERA_Y, 1.0f);
  targetLooksAt.set(looksAt);
  smoothLooksAt.set(looksAt);

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
  const float normalizedV = (rightJoy.v - 128.0F) * INV_128;

  // Early return if input is below threshold
  if (Utils::Abs(normalizedV) <= g_settings.r_stick_V) return;

  targetPitch += g_settings.cam_v_sensitivity * deltatime * -normalizedV;

  // Clamp pitch efficiently
  if (targetPitch > 89.0F) {
    targetPitch = 89.0F;
  } else if (targetPitch < -89.0F) {
    targetPitch = -89.0F;
  }
}

void Camera::calculateYaw(Pad* t_pad, const float deltatime) {
  const auto& rightJoy = t_pad->getRightJoyPad();
  const float normalizedH = (rightJoy.h - 128.0F) * INV_128;

  // Early return if input is below threshold
  if (Utils::Abs(normalizedH) <= g_settings.r_stick_H) return;

  targetYaw += g_settings.cam_h_sensitivity * deltatime * normalizedH;

  // Normalize yaw efficiently
  if (targetYaw < 0.0f) {
    targetYaw += 360.0F;
  } else if (targetYaw >= 360.0f) {
    targetYaw -= 360.0F;
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
  // Use cached trigonometric values if available, otherwise calculate
  const float thetaRad = Tyra::Math::ANG2RAD * smoothYaw;
  const float offsetX = horizontalDistance * Tyra::Math::cos(thetaRad);
  const float offsetZ = horizontalDistance * Tyra::Math::sin(thetaRad);

  position.set(newPosition->x - offsetX,
               newPosition->y + CAMERA_Y + verticalDistance,
               newPosition->z - offsetZ);
}

void Camera::shakeCamera(const float deltaTime, const bool isWalking) {
  // Early returns for performance
  if (!bobEnabled || !isWalking) {
    // Fast fade out path
    const float fadeSpeed = !bobEnabled ? 10.0F : 8.0F;
    const float f = fadeSpeed * deltaTime;

    // Direct lerp without temporary variables
    bobCurrentOffset.x += (0.0F - bobCurrentOffset.x) * f;
    bobCurrentOffset.y += (0.0F - bobCurrentOffset.y) * f;
    bobCurrentOffset.z += (0.0F - bobCurrentOffset.z) * f;

    bobTilt += (0.0F - bobTilt) * f;
    const float targetPitchOffset = bobTilt * 0.02F;
    bobPitchOffset += (targetPitchOffset - bobPitchOffset) * f;
    return;
  }

  // Advance phase with cached speed calculation
  const float baseSpeed = (camera_type == CamType::FirstPerson) ? 6.2F : 5.2F;
  bobPhase += deltaTime * baseSpeed;
  if (bobPhase > TWO_PI_CONST) bobPhase -= TWO_PI_CONST;

  // Cache intensity calculation
  const float modeScale = (camera_type == CamType::FirstPerson) ? 0.9F : 0.55F;
  const float intensity = bobIntensityMultiplier * modeScale;

  // Calculate waves once and reuse
  const float w1 = Math::sin(bobPhase);
  const float w2 = Math::sin(bobPhase * 2.0F);
  const float w3 = Math::sin(bobPhase * 1.5F);

  // Pre-calculate motion components
  const float side = w1 * 0.015F * intensity;
  const float up = (w2 * 0.5F + 0.5F) * 0.01F * intensity;
  const float depth = w3 * 0.008F * intensity;

  // Use cached yaw calculations from update function
  const float yawRad = Utils::degreesToRadian(smoothYaw);
  const float sY = Math::sin(yawRad);
  const float cY = Math::cos(yawRad);

  // Calculate target offset directly
  bobTargetOffset.x = (-sY * side) + (cY * depth);
  bobTargetOffset.y = up;
  bobTargetOffset.z = (cY * side) + (sY * depth);

  // Optimized lerp without Vec4::getByLerp overhead
  const float offLerp = 14.0F * deltaTime;
  bobCurrentOffset.x += (bobTargetOffset.x - bobCurrentOffset.x) * offLerp;
  bobCurrentOffset.y += (bobTargetOffset.y - bobCurrentOffset.y) * offLerp;
  bobCurrentOffset.z += (bobTargetOffset.z - bobCurrentOffset.z) * offLerp;

  // Tilt calculation with early exit for third person
  if (camera_type == CamType::FirstPerson) {
    bobTargetTilt = w1 * 0.35F * intensity;
    const float tiltLerp = 12.0F * deltaTime;
    bobTilt += (bobTargetTilt - bobTilt) * tiltLerp;

    // Update pitch offset with clamping
    const float targetPitchOffset = bobTilt * 0.02F;
    const float pitchLerp = 16.0F * deltaTime;
    bobPitchOffset += (targetPitchOffset - bobPitchOffset) * pitchLerp;

    // Clamp pitch offset
    if (bobPitchOffset > 1.0F)
      bobPitchOffset = 1.0F;
    else if (bobPitchOffset < -1.0F)
      bobPitchOffset = -1.0F;
  } else {
    // Fade out tilt for third person
    const float tiltFade = 12.0F * deltaTime;
    bobTilt += (0.0F - bobTilt) * tiltFade;
    bobPitchOffset += (0.0F - bobPitchOffset) * tiltFade;
  }
}

void Camera::applySmoothMovement(const float deltaTime) {
  // Clamp lerp factors to [0, 1] to prevent overshoot on low FPS
  float rotationLerpFactor = smoothFactorRotation * deltaTime;
  if (rotationLerpFactor > 1.0f) rotationLerpFactor = 1.0f;

  // Optimize yaw smoothing with threshold check
  float yawDiff = targetYaw - smoothYaw;
  if (yawDiff > 180.0f) {
    yawDiff -= 360.0f;
  } else if (yawDiff < -180.0f) {
    yawDiff += 360.0f;
  }

  // Only update if difference is significant
  if (Utils::Abs(yawDiff) > SMOOTH_THRESHOLD) {
    smoothYaw += yawDiff * rotationLerpFactor;
    if (smoothYaw < 0.0f) {
      smoothYaw += 360.0f;
    } else if (smoothYaw >= 360.0f) {
      smoothYaw -= 360.0f;
    }
  }

  // Optimize pitch smoothing with threshold and validation
  const float pitchDiff = targetPitch - smoothPitch;
  if (Utils::Abs(pitchDiff) > SMOOTH_THRESHOLD) {
    smoothPitch += pitchDiff * rotationLerpFactor;
    // Safety clamp for pitch extremes
    if (smoothPitch > 89.0f) smoothPitch = 89.0f;
    if (smoothPitch < -89.0f) smoothPitch = -89.0f;
  }

  // Optimize position smoothing
  float positionLerpFactor = smoothFactorPosition * deltaTime;
  if (positionLerpFactor > 1.0f) positionLerpFactor = 1.0f;

  // Fast squared distance check to avoid sqrt
  const Vec4 positionDiff = targetPosition - smoothPosition;
  const float distanceSqr = positionDiff.x * positionDiff.x +
                            positionDiff.y * positionDiff.y +
                            positionDiff.z * positionDiff.z;

  // Use pre-calculated constant from header
  const float MIN_SMOOTH_DISTANCE_SQR = SMOOTH_THRESHOLD * SMOOTH_THRESHOLD;

  if (distanceSqr > MAX_SMOOTH_DISTANCE_SQR) {
    // Teleport case - snap to target
    smoothPosition = targetPosition;
  } else if (distanceSqr > MIN_SMOOTH_DISTANCE_SQR) {
    // Normal smoothing - direct lerp without Vec4::getByLerp overhead
    smoothPosition.x +=
        (targetPosition.x - smoothPosition.x) * positionLerpFactor;
    smoothPosition.y +=
        (targetPosition.y - smoothPosition.y) * positionLerpFactor;
    smoothPosition.z +=
        (targetPosition.z - smoothPosition.z) * positionLerpFactor;
  }

  // Final safety validation of extremes to catch any edge cases
  if (smoothPitch > 89.0f) smoothPitch = 89.0f;
  if (smoothPitch < -89.0f) smoothPitch = -89.0f;
  if (smoothYaw < 0.0f) smoothYaw += 360.0f;
  if (smoothYaw >= 360.0f) smoothYaw -= 360.0f;

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

void Camera::setBobEnabled(bool enabled) { bobEnabled = enabled; }
