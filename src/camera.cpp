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

const float CAMERA_Y = 25.0F;

// ----
// Constructors/Destructors
// ----

Camera::Camera(const RendererSettings& t_screen)
    : Tyra::CameraInfo3D(&position, &lookPos) {
  verticalLevel = DUBLE_BLOCK_SIZE * 2;
  pitch = 0.0F;
  yaw = 270.0F;
}

Camera::~Camera() {}

// ----
// Methods
// ----

void Camera::update(Pad& t_pad, Mesh& t_mesh) {
  rotate(t_pad);
  updateUnitCirclePosition();
  followBy(t_mesh);
  pointCamera(t_pad, t_mesh);
}

/** Set camera rotation by pad right joy and update unit circle
 * https://en.wikipedia.org/wiki/Unit_circle
 * https://www.desmos.com/calculator/3e7iypw4ow
 */
void Camera::rotate(Pad& t_pad) {
  if (t_pad.getRightJoyPad().h <= 100)
    horizontalLevel += 0.08F;
  else if (t_pad.getRightJoyPad().h >= 200)
    horizontalLevel -= 0.08F;
}

void Camera::updateUnitCirclePosition() {
  unitCirclePosition.x = (Math::sin(horizontalLevel) * verticalLevel);
  unitCirclePosition.y = CAMERA_Y;
  unitCirclePosition.z = (Math::cos(horizontalLevel) * verticalLevel);
}

/** Rotate camera around 3D object */
void Camera::followBy(Mesh& t_mesh) {
  position.y = unitCirclePosition.y + t_mesh.getPosition()->y;
  if (camera_type == FIRST_PERSON_CAM) {
    position.x = t_mesh.getPosition()->x;
    position.z = t_mesh.getPosition()->z;
  } else {
    position.x = t_mesh.getPosition()->x + unitCirclePosition.x;
    position.z = t_mesh.getPosition()->z + unitCirclePosition.z;
  }
}

/** Point the camera to a specific position */
void Camera::pointCamera(Pad& t_pad, Mesh& t_mesh) {
  if (t_pad.getRightJoyPad().h <= 100)
    yaw -= _sensitivity;
  else if (t_pad.getRightJoyPad().h >= 200)
    yaw += _sensitivity;
  else if (t_pad.getRightJoyPad().v <= 100)
    pitch += _sensitivity / 3;
  else if (t_pad.getRightJoyPad().v >= 200)
    pitch -= _sensitivity / 3;

  if (pitch > 89.0F) pitch = 89.0F;
  if (pitch < -89.0F) pitch = -89.0F;

  lookPos.x = Math::cos(Utils::degreesToRadian(yaw)) *
              Math::cos(Utils::degreesToRadian(pitch));
  lookPos.y = Math::sin(Utils::degreesToRadian(pitch));
  lookPos.z = Math::sin(Utils::degreesToRadian(yaw)) *
              Math::cos(Utils::degreesToRadian(pitch));

  lookPos += position;

  // t_mesh.getPosition()->print("Mesh");
  // position.print("Position");
  // lookPos.print("Look at");

  // lookAt(lookPos);
}
