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

#include <pad/pad.hpp>
#include <renderer/3d/mesh/mesh.hpp>
#include <math/vec4.hpp>
#include <renderer/renderer_settings.hpp>
#include <renderer/core/3d/camera_info_3d.hpp>
#include <tamtypes.h>
#include <utils.hpp>
#include <math/math.hpp>
#include "constants.hpp"
#include <debug/debug.hpp>
#include <fastmath.h>
#include "renderer/renderer_settings.hpp"
#include <tyra>

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

  Vec4 position, lookPos, unitCirclePosition;

  float pitch, yaw;
  float hitDistance;

  void update();
  void reset();
  void setPositionByMesh(Mesh* t_mesh);
  void setLookDirectionByPad(Pad* t_pad, const float deltatime);

  void setFirstPerson();
  void setThirdPerson();
  void setThirdPersonInverted();

  CameraInfo3D getCameraInfo() { return CameraInfo3D(&position, &lookPos); }

  inline const CamType getCamType() const { return camera_type; }
  inline float getCamY() { return CAMERA_Y; };

  const float distanceFromPlayer = 80.0F;

 private:
  CamType camera_type = CamType::FirstPerson;
  const float CAMERA_Y = 25.0F;

  Ray revRay;

  void calculatePitch(Pad* t_pad, const float deltatime);
  void calculateYaw(Pad* t_pad, const float deltatime);
  float calculateHorizontalDistance();
  float calculateVerticalDistance();
  void calculateCameraPosition(Mesh* t_mesh, const float horizontalDistance,
                               const float verticalDistance);
};
