#include "utils.hpp"
#include "managers/particle/flame_particle.hpp"
#include "entities/level.hpp"
#include "managers/tick_manager.hpp"
#include "managers/model_builder.hpp"
#include "managers/block/vertex_block_data.hpp"

// Static UV shared by all flame particles — built once, never changes
const Vec4 FlameParticle::flameUV[6] = {
    Vec4(0.0000F, 0.2421F, 1.0F, 0.0F),  // xMin, yMax
    Vec4(0.0546F, 0.1875F, 1.0F, 0.0F),  // xMax, yMin
    Vec4(0.0546F, 0.2421F, 1.0F, 0.0F),  // xMax, yMax
    Vec4(0.0000F, 0.2421F, 1.0F, 0.0F),  // xMin, yMax
    Vec4(0.0000F, 0.1875F, 1.0F, 0.0F),  // xMin, yMin
    Vec4(0.0546F, 0.1875F, 1.0F, 0.0F),  // xMax, yMin
};

FlameParticle::FlameParticle(Vec4* offset) : Particle(ParticleType::Flame) {
  billboarded = true;

  // Define life time
  _lifeTime = 20;

  // Define if is collidable
  collidable = false;

  Level* pLevel = Level::getInstance();

  Vec4 offsetCorrection = Vec4(0, 0, 0);
  const BlockOrientation orientation =
      pLevel->GetTorchOrientationDataFromMap(offset->x, offset->y, offset->z);

  const float offsetH = BLOCK_SIZE * 0.25F;
  const float offsetVAtSide = BLOCK_SIZE * 0.670F;
  const float offsetVOnTop = BLOCK_SIZE * 0.700F;

  switch (orientation) {
    case BlockOrientation::North:
      offsetCorrection.set(0.0F, offsetVAtSide, offsetH);
      break;
    case BlockOrientation::South:
      offsetCorrection.set(0.0F, offsetVAtSide, -offsetH);
      break;
    case BlockOrientation::West:
      offsetCorrection.set(offsetH, offsetVAtSide, 0.0F);
      break;
    case BlockOrientation::East:
      offsetCorrection.set(-offsetH, offsetVAtSide, 0.0F);
      break;
    case BlockOrientation::Top:
    default:
      offsetCorrection.set(0.0F, offsetVOnTop, 0.0F);
      break;
  }

  // Set position by top of the torch
  Vec4 min, max;
  BBox* rawBBox = VertexBlockData::getTorchRawBBox();
  rawBBox->getMinMax(&min, &max);

  M4x4 model = ModelBuilder_TorchModel(offset);
  min = model * min;
  max = model * max;

  Vec4 center = min + ((max - min) / 2);
  _position = center + offsetCorrection;

  // Copy from shared static UV — no per-instance computation
  memcpy(uv, flameUV, sizeof(flameUV));

  color = Color(120, 120, 120);
}

void FlameParticle::fixedUpdate(const float fixedDeltaTime) {}

void FlameParticle::renew() {
  // Reset time and expired flag (base class behavior)
  Particle::renew();
  // Reset lifetime to initial value for proper scaling
  _lifeTime = 20;
}

void FlameParticle::update(const float deltaTime, const M4x4* billboard) {
  if (expired) return;

  const float scaleVal = START_SIZE * (_lifeTime / 20.0F);
  M4x4 scale;
  scale.identity();
  scale.scaleX(scaleVal);
  scale.scaleY(scaleVal);

  // Apply shared billboard rotation + per-particle scale, then translate to
  // world pos
  M4x4 model = *billboard * scale;
  model.translate(_position);

  const Vec4* data = Particle::rawData;
  for (size_t j = 0; j < Particle::DRAW_DATA_COUNT; j++) {
    vertex[j] = model * data[j];
  }
}