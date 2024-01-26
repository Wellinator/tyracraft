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
#include <tamtypes.h>
#include <utils.hpp>
#include <fastmath.h>

using Tyra::CameraInfo3D;
using Tyra::Pad;
using Tyra::Ray;
using Tyra::RendererSettings;

enum class CamType { FirstPerson, ThirdPerson, ThirdPersonInverted };

/** 3D camera which follow by 3D object. Can be rotated via pad */
class Camera {
 public:
  Camera(const RendererSettings& t_screen);
  ~Camera();

  Vec4 position, lookPos, unitCirclePosition, camShake;

  float pitch, yaw;
  float hitDistance;

  void update();
  void update(const float& deltaTime, const u8 isWalking);
  void reset();
  void setPosition(Vec4 newPosition);
  void setLookDirectionByPad(Pad* t_pad, const float deltatime);

  void setFirstPerson();
  void setThirdPerson();
  void setThirdPersonInverted();

  CameraInfo3D getCameraInfo() { return CameraInfo3D(&position, &lookPos); }

  inline const CamType getCamType() const { return camera_type; }
  inline float getCamY() { return CAMERA_Y; };

  const float distanceFromPlayer = 80.0F;

  inline const float getCamTime() const { return camera_time; };

 private:
  CamType camera_type = CamType::FirstPerson;
  const float CAMERA_Y = 25.0F;

  float camera_time = 0;

  Ray revRay;

  void shakeCamera();
  void calculatePitch(Pad* t_pad, const float deltatime);
  void calculateYaw(Pad* t_pad, const float deltatime);
  float calculateHorizontalDistance();
  float calculateVerticalDistance();
  void calculateCameraPosition(Vec4* newPosition,
                               const float horizontalDistance,
                               const float verticalDistance);
};
