#include "constants.hpp"
#include "utils.hpp"
#include "managers/particle/smoke_particle.hpp"
#include "entities/level.hpp"
#include "managers/tick_manager.hpp"
#include "timer.hpp"
#include "managers/model_builder.hpp"
#include "managers/block/vertex_block_data.hpp"

using Tyra::Color;

// Static UV LUT: 8 stages × 6 vertices — built once in initUVLUT()
Vec4** SmokeParticle::uvLUT = nullptr;

void SmokeParticle::initUVLUT() {
  if (uvLUT) {
#ifdef DEBUG_MODE
    TYRA_WARN("[SmokeParticle] uvLUT already initialized, skipping");
#endif
    return;  // Already initialised
  }
  const float colSize = 0.0625F;

  uvLUT = new Vec4*[8];
  for (u8 stage = 0; stage < 8; stage++) {
    uvLUT[stage] = new Vec4[6];
    const float xMin = stage * colSize;
    const float xMax = xMin + colSize;
    const float yMin = 0.0000F;
    const float yMax = 0.0625F;

    uvLUT[stage][0] = Vec4(xMin, yMax, 1.0F, 0.0F);
    uvLUT[stage][1] = Vec4(xMax, yMin, 1.0F, 0.0F);
    uvLUT[stage][2] = Vec4(xMax, yMax, 1.0F, 0.0F);
    uvLUT[stage][3] = Vec4(xMin, yMax, 1.0F, 0.0F);
    uvLUT[stage][4] = Vec4(xMin, yMin, 1.0F, 0.0F);
    uvLUT[stage][5] = Vec4(xMax, yMin, 1.0F, 0.0F);
  }
}

void SmokeParticle::destroyUVLUT() {
  if (!uvLUT) return;
  for (u8 stage = 0; stage < 8; stage++) {
    delete[] uvLUT[stage];
  }
  delete[] uvLUT;
  uvLUT = nullptr;
}

SmokeParticle::SmokeParticle(Vec4* offset) : Particle(ParticleType::Smoke) {
  billboarded = true;

  // Define life time
  // This will influence how high the particles will go
  _lifeTime = TICK * 17;

  // Define if is collidable
  collidable = false;

  // Define direction
  _direction = Vec4(0.0F, 1.0F, 0.0F);

  // Check if the torch is oriented and fix position
  Level* pLevel = Level::getInstance();
  Vec4 offsetCorrection = Vec4(0, 0, 0);
  const BlockOrientation orientation =
      pLevel->GetTorchOrientationDataFromMap(offset->x, offset->y, offset->z);

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
  Vec4 min, max;
  BBox* rawBBox = VertexBlockData::getTorchRawBBox();
  rawBBox->getMinMax(&min, &max);

  M4x4 model = ModelBuilder_TorchModel(offset);
  min = model * min;
  max = model * max;

  // Set position in the middle of the bounding box
  Vec4 center = min + ((max - min) / 2);
  _position = center + offsetCorrection;
  _prevPosition.set(_position);
  _targetPosition.set(_position);

  const float colorRGB = Tyra::Math::randomi(35, 150);
  color.set(colorRGB, colorRGB, colorRGB);

  // Init UV from LUT stage 0 (uvLUT must be built before any smoke particle)
  if (uvLUT) {
    memcpy(uv, uvLUT[0], sizeof(Vec4) * 6);
  }
}

SmokeParticle::~SmokeParticle() {}

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

  // Updates smoke UV based on lifeTime — only when stage changes
  const u8 tempStage = getStage();
  if (tempStage != stageIndex) {
    stageIndex = tempStage;
    updateUV(stageIndex);
  }
}

void SmokeParticle::update(const float deltaTime, const M4x4* billboard) {
  _elapsedTime += deltaTime;
  if (_elapsedTime > _lifeTime) {
    expired = true;
    return;
  }

  _position.lerp(_prevPosition, _targetPosition, TyraCraft::Timer::stateLerp);

  M4x4 scale;
  scale.identity();
  scale.scaleX(size);
  scale.scaleY(size);

  // Apply shared billboard rotation + per-particle scale, then translate to world pos
  M4x4 model = *billboard * scale;
  model.translate(_position);

  const Vec4* data = Particle::rawData;
  for (size_t j = 0; j < Particle::DRAW_DATA_COUNT; j++) {
    vertex[j] = model * data[j];
  }
}

u8 SmokeParticle::getStage() {
  const float lerp = 1.0F - (_elapsedTime / _lifeTime);
  return static_cast<u8>(std::floor(MAX_UV_INDEX * lerp));
}

void SmokeParticle::updateUV(const u8 _stageIndex) {
  // Fast memcpy from pre-built LUT — no Vec4 construction at runtime
  if (uvLUT) {
    memcpy(uv, uvLUT[_stageIndex], sizeof(Vec4) * 6);
    return;
  }
  // Fallback (LUT not yet initialised)
  const float colSize = 0.0625F;
  const float xMin = _stageIndex * colSize;
  const float xMax = xMin + colSize;
  uv[0] = Vec4(xMin, 0.0625F, 1.0F, 0.0F);
  uv[1] = Vec4(xMax, 0.0000F, 1.0F, 0.0F);
  uv[2] = Vec4(xMax, 0.0625F, 1.0F, 0.0F);
  uv[3] = Vec4(xMin, 0.0625F, 1.0F, 0.0F);
  uv[4] = Vec4(xMin, 0.0000F, 1.0F, 0.0F);
  uv[5] = Vec4(xMax, 0.0000F, 1.0F, 0.0F);
}
