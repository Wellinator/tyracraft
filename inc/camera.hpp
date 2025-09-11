/*
# ______       ____   ___
#   |     \/   ____| |___|
#   |     |   |   \  |   |
#-----------------------------------------------------------------------
# Copyright 2020, tyra - https://github.com/h4570/tyra
# Licenced under Apache License 2.0
# Sandro Sobczyński <sandro.sobczynski@gmail.com>
*/

#pragma once

#include <tyra>
#include "constants.hpp"
#include "singleton.hpp"
#include <tamtypes.h>
#include <utils.hpp>
#include <fastmath.h>

// Smooth movement configuration
#define SMOOTH_FACTOR_ROTATION 20.0F   // Higher = faster rotation smoothing
#define SMOOTH_FACTOR_POSITION 12.0F   // Higher = faster position smoothing
#define SMOOTH_THRESHOLD 0.01F        // Minimum difference to smooth

using Tyra::CameraInfo3D;
using Tyra::Pad;
using Tyra::Ray;
using Tyra::RendererSettings;

enum class CamType { FirstPerson, ThirdPerson, ThirdPersonInverted };

/** 3D camera which follow by 3D object. Can be rotated via pad */
class Camera : public Singleton<Camera> {
 public:
  Camera(const RendererSettings& t_screen);
  ~Camera();

  Vec4 position, looksAt, unitCirclePosition, camShake;

  float pitch, yaw;
  float hitDistance;

  // Smooth movement variables
  float targetPitch, targetYaw;
  float smoothPitch, smoothYaw;
  Vec4 targetPosition, smoothPosition;
  Vec4 targetLooksAt, smoothLooksAt;

  void update();
  void update(const float& deltaTime, const u8 isWalking);
  void reset();
  void setPosition(Vec4 newPosition);
  void setLookDirectionByPad(Pad* t_pad, const float deltatime);

  void setFirstPerson();
  void setThirdPerson();
  void setThirdPersonInverted();

  // Smooth movement configuration methods
  void setSmoothFactors(float rotationFactor, float positionFactor);
  void resetSmoothMovement();

  // Bob camera configuration methods
  void setBobIntensity(float intensity);
  void setBobEnabled(bool enabled);

  CameraInfo3D getCameraInfo() { return CameraInfo3D(&position, &looksAt); }

  inline const CamType getCamType() const { return camera_type; }
  inline float getCamY() { return CAMERA_Y; };

  const float distanceFromPlayer = 80.0F;

  inline const float getBobPhase() const { return bobPhase; };

 private:
  CamType camera_type = CamType::FirstPerson;
  const float CAMERA_Y = 25.0F;

  // Configurable smooth factors
  float smoothFactorRotation = SMOOTH_FACTOR_ROTATION;
  float smoothFactorPosition = SMOOTH_FACTOR_POSITION;

  // Bob camera configuration
  // Default intensity further reduced for a very subtle feel
  float bobIntensityMultiplier = 0.5f;
  bool bobEnabled = true;
  // Bob internal state (deltaTime based)
  Vec4 bobCurrentOffset = Vec4(0.0F, 0.0F, 0.0F);
  Vec4 bobTargetOffset = Vec4(0.0F, 0.0F, 0.0F);
  float bobPhase = 0.0F;       // phase accumulator
  float bobTilt = 0.0F;        // current applied view tilt (pitch add)
  float bobTargetTilt = 0.0F;  // target tilt for smoothing
  float bobPitchOffset =
      0.0F;  // final additive pitch offset (computed from tilt)

  void shakeCamera(const float deltaTime, const bool isWalking);
  void calculatePitch(Pad* t_pad, const float deltatime);
  void calculateYaw(Pad* t_pad, const float deltatime);
  float calculateHorizontalDistance();
  float calculateVerticalDistance();
  void calculateCameraPosition(Vec4* newPosition,
                               const float horizontalDistance,
                               const float verticalDistance);
  void applySmoothMovement(const float deltaTime);
  void initializeSmoothValues();
};
