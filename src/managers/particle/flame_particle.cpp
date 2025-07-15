#include "utils.hpp"
#include "managers/particle/flame_particle.hpp"
#include "entities/level.hpp"
#include "managers/tick_manager.hpp"
#include "managers/model_builder.hpp"
#include "managers/block/vertex_block_data.hpp"

FlameParticle::FlameParticle(Vec4* offset) : Particle(ParticleType::Flame) {
  billboarded = true;

  // Define life time
  _lifeTime = TICK * 20;

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
  delete rawBBox;

  M4x4 model = ModelBuilder_TorchModel(offset);
  min = model * min;
  max = model * max;

  Vec4 center = min + ((max - min) / 2);
  _position = center + offsetCorrection;

  // TODO: apply correct flame UV and move to static property
  // Calc rand offset between row and col;
  auto xMin = 0.0000F;
  auto xMax = 0.0546F;
  auto yMin = 0.1875F;
  auto yMax = 0.2421F;

  uv[0] = Vec4(xMin, yMax, 1.0F, 0.0F);
  uv[1] = Vec4(xMax, yMin, 1.0F, 0.0F);
  uv[2] = Vec4(xMax, yMax, 1.0F, 0.0F);
  uv[3] = Vec4(xMin, yMax, 1.0F, 0.0F);
  uv[4] = Vec4(xMin, yMin, 1.0F, 0.0F);
  uv[5] = Vec4(xMax, yMin, 1.0F, 0.0F);

  // TODO: refactor lightmanager to use block offset instead block reference
  // and get color in realtime based on block position
  color = Color(120, 120, 120);
};

void FlameParticle::fixedUpdate(const float fixedDeltaTime) { return; }

void FlameParticle::update(const float deltaTime, const Vec4* camPos) {
  _elapsedTime += deltaTime;

  if (_elapsedTime > _lifeTime) {
    expired = true;
    return;
  }

  M4x4 model, translation, scale;

  float _scale = START_SIZE * (1.0F - (_elapsedTime / _lifeTime));
  scale.identity();
  scale.scaleX(_scale);
  scale.scaleY(_scale);

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