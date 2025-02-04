#include "constants.hpp"
#include "utils.hpp"
#include "managers/particle/smoke_particle.hpp"
#include "entities/level.hpp"
#include "managers/tick_manager.hpp"
#include "timer.hpp"

using Tyra::Color;

SmokeParticle::SmokeParticle(Block* pBlock) : Particle(PaticleType::Flame) {
  billboarded = true;

  // Define life time
  // This will influence how high the particles will go
  _lifeTime = TICK * 17;

  // Define if is collidable
  collidable = false;

  // Define direction
  _direction = Vec4(0.0F, 1.0F, 0.0F);

  // Check if the torch is oriented and fix position
  Vec4 pos, offsetCorrection = Vec4(0, 0, 0);
  pBlock->pLevel->GetXYZFromPos(&pBlock->offset, &pos);
  const BlockOrientation orientation =
      pBlock->pLevel->GetTorchOrientationDataFromMap(pos.x, pos.y, pos.z);

  const float offsetH = BLOCK_SIZE * 0.25F;
  const float offsetVAtSide = BLOCK_SIZE * 0.75F;
  const float offsetVOnTop = BLOCK_SIZE * 0.80F;

  switch (orientation) {
    case BlockOrientation::North:
      offsetCorrection.set(0.0F, offsetVAtSide, offsetH, 1.0f);
      break;
    case BlockOrientation::South:
      offsetCorrection.set(0.0F, offsetVAtSide, -offsetH, 1.0f);
      break;
    case BlockOrientation::West:
      offsetCorrection.set(offsetH, offsetVAtSide, 0.0F, 1.0f);
      break;
    case BlockOrientation::East:
      offsetCorrection.set(-offsetH, offsetVAtSide, 0.0F, 1.0f);
      break;
    case BlockOrientation::Top:
    default:
      offsetCorrection.set(0.0F, offsetVOnTop, 0.0F, 1.0f);
      break;
  }

  // Set position by top of the torch
  Vec4 center =
      pBlock->minCorner + ((pBlock->maxCorner - pBlock->minCorner) / 2);
  _position = center + offsetCorrection;
  _prevPosition.set(_position);
  _targetPosition.set(_position);

  const float colorRGB = Tyra::Math::randomi(35, 150);
  color.set(colorRGB, colorRGB, colorRGB);
};

SmokeParticle::~SmokeParticle() { return; }

void SmokeParticle::fixedUpdate(const float fixedDeltaTime) {
  // Reset lerp state
  _prevPosition.set(_targetPosition);

  // Update position without gravity
  const float particleSpeed = 45.0F;
  const float instantSpeed = particleSpeed * fixedDeltaTime;
  _velocity += _direction * instantSpeed;

  // Define next position based on velocity
  const auto nextPosition = _targetPosition + (_velocity * fixedDeltaTime);
  _targetPosition.set(nextPosition);

  // Updates smoke UV based on lifeTime
  const u8 tempStage = getStage();
  if (tempStage != stageIndex) {
    stageIndex = tempStage;
    updateUV(stageIndex);
  }
}

void SmokeParticle::update(const float deltaTime, const Vec4* camPos) {
  _elapsedTime += deltaTime;
  if (_elapsedTime > _lifeTime) {
    expired = true;
    return;
  }

  _position.lerp(_prevPosition, _targetPosition, TyraCraft::Timer::stateLerp);

  M4x4 model, scale;

  // Set scale
  scale.identity();
  scale.scaleX(size);
  scale.scaleY(size);

  /**
   * Apply billboard rotation to particle of type equals to
   * PaticleType::Block
   */
  M4x4 result;
  M4x4 temp;
  M4x4::lookAt(&temp, _position, *camPos);
  Utils::inverseMatrix(&result, &temp);

  // Set particle model. M = R * S;
  model = result * scale;

  size_t size = Particle::DRAW_DATA_COUNT;
  const Vec4* data = Particle::rawData;

  for (size_t j = 0; j < size; j++) {
    vertex[j] = model * data[j];
  }
};

u8 SmokeParticle::getStage() {
  const float lerp = 1.0F - (_elapsedTime / _lifeTime);
  return static_cast<u8>(std::floor(MAX_UV_INDEX * lerp));
}

void SmokeParticle::updateUV(const u8 _stageIndex) {
  const float colSize = 0.0625F;

  auto xMin = _stageIndex * colSize;
  auto xMax = xMin + colSize;

  auto yMin = 0.0000F;
  auto yMax = 0.0625F;

  uv[0] = Vec4(xMin, yMax, 1.0F, 0.0F);
  uv[1] = Vec4(xMax, yMin, 1.0F, 0.0F);
  uv[2] = Vec4(xMax, yMax, 1.0F, 0.0F);
  uv[3] = Vec4(xMin, yMax, 1.0F, 0.0F);
  uv[4] = Vec4(xMin, yMin, 1.0F, 0.0F);
  uv[5] = Vec4(xMax, yMin, 1.0F, 0.0F);
}
