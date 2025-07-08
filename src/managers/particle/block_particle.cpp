#include "managers/particle/block_particle.hpp"

BlockParticle::BlockParticle(Block* pBlock) : Particle(PaticleType::Block) {
  // Define life time
  _lifeTime = Tyra::Math::randomf(0.6F, 1.0F);

  // Define if is collidable
  collidable = Utils::Probability(0.5);

  // Set particle initial velocity
  // Initiate with a random value from 5 to 15 to lift it on spawn
  _velocity.y = Tyra::Math::randomf(5.0F, 15.0F);

  if (pBlock->getIsTarget()) {
    _position =
        Vec4(pBlock->getHitPosition().x + (Tyra::Math::randomf(-8.0F, 8.0F)),
             pBlock->getHitPosition().y + (Tyra::Math::randomf(-8.0F, 8.0F)),
             pBlock->getHitPosition().z + (Tyra::Math::randomf(-8.0F, 8.0F)));
    _direction = _position - pBlock->getHitPosition();
  } else {
    _position = Vec4(pBlock->position.x + (Tyra::Math::randomf(-4.5F, 4.5F)),
                     pBlock->position.y + (Tyra::Math::randomf(-4.5F, 4.5F)),
                     pBlock->position.z + (Tyra::Math::randomf(-4.5F, 4.5F)));
    _direction = _position - pBlock->position;
  }
  _prevPosition.set(_position);
  _targetPosition.set(_position);
  _direction.normalize();

  const float UVSscale = 1.0F / 16.0F;
  const Vec4 scaleVec = Vec4(UVSscale, UVSscale, 1.0F, 0.0F);

  // Calc rand offset between row and col;
  const u8 index = pBlock->getFacesMap()->data()[4];
  const u8 X = index < MAX_TEX_COLS ? index : index % MAX_TEX_COLS;
  const u8 Y = index < MAX_TEX_COLS ? 0 : std::floor(index / MAX_TEX_COLS);

  auto xMin = Tyra::Math::randomf(X, X + 0.5F);
  auto xMax = Tyra::Math::randomf(X, X + 0.5F);
  auto yMin = Tyra::Math::randomf(Y, Y + 0.5F);
  auto yMax = Tyra::Math::randomf(Y, Y + 0.5F);

  uv[0] = Vec4(xMin, yMax, 1.0F, 0.0F) * scaleVec;
  uv[1] = Vec4(xMax, yMin, 1.0F, 0.0F) * scaleVec;
  uv[2] = Vec4(xMax, yMax, 1.0F, 0.0F) * scaleVec;
  uv[3] = Vec4(xMin, yMax, 1.0F, 0.0F) * scaleVec;
  uv[4] = Vec4(xMin, yMin, 1.0F, 0.0F) * scaleVec;
  uv[5] = Vec4(xMax, yMin, 1.0F, 0.0F) * scaleVec;

  color.set(pBlock->baseColor);
};

void BlockParticle::fixedUpdate(const float fixedDeltaTime) {
  // Reset lerp state
  _prevPosition.set(_targetPosition);

  const auto PARTICLE_GRAVITY = GRAVITY * 0.9F * fixedDeltaTime;
  const float particleSpeed = 65.0F;
  const float instantSpeed = particleSpeed * fixedDeltaTime;

  // Update position
  _velocity += _direction * instantSpeed;

  // Reduce gravity to 85%, it was too huge for particles
  _velocity += PARTICLE_GRAVITY;

  // Define next position based on velocity
  const auto nextPosition = _targetPosition + (_velocity * fixedDeltaTime);

  if (collidable) {
    float closestHitDistance = -1.0f;
    const float maxCollidableDistance =
        _targetPosition.distanceTo(nextPosition);
    u8 willCollide = false;
    Vec4 finalHitPosition;

    // Broad phase
    std::vector<index_t> ni;
    g_AABBTree->intersectLine(_targetPosition, nextPosition, ni);

    Ray ray = Ray(_targetPosition, _direction);

    for (u16 i = 0; i < ni.size(); i++) {
      Entity* entity = static_cast<Entity*>(g_AABBTree->user_data(ni[i]));
      if (!entity->collidable) continue;

      // Narrow Phase
      float hitDistance;
      if (ray.intersectBox(entity->minCorner, entity->maxCorner,
                           &hitDistance) &&
          hitDistance < maxCollidableDistance) {
        if (closestHitDistance == -1.0F || hitDistance < closestHitDistance) {
          closestHitDistance = hitDistance;
          willCollide = true;
          finalHitPosition.set(ray.at(hitDistance));
        }
      }
    }

    if (willCollide) {
      _targetPosition.set(finalHitPosition);
      _direction = -_direction;
    } else {
      _targetPosition = nextPosition;
    }
  } else {
    _targetPosition = nextPosition;
  }
}

void BlockParticle::update(const float deltaTime, const Vec4* camPos) {
  _elapsedTime += deltaTime;

  if (_elapsedTime > _lifeTime) {
    expired = true;
    return;
  }

  M4x4 model, scale;
  model.identity();

  _position.lerp(_prevPosition, _targetPosition, TyraCraft::Timer::stateLerp);

  // Set scale matrix
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

  for (size_t j = 0; j < Particle::DRAW_DATA_COUNT; j++) {
    vertex[j] = model * Particle::rawData[j];
  }
};
