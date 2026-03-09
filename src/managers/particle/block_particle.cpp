#include "managers/particle/block_particle.hpp"

BlockParticle::BlockParticle(Block* pBlock)
    : CollidableParticle(ParticleType::Block) {
  // Safety: validate block pointer before dereferencing
  TYRA_ASSERT(pBlock != nullptr, "BlockParticle: pBlock cannot be null!");

  // Define life time
  _lifeTime = Tyra::Math::randomi(8, 40);

  // Define if is collidable
  collidable = true;

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
  const auto facesMap = pBlock->getFacesMap();
  const u8 index = facesMap[4];
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
  if (expired) return;

  // Reset lerp state
  _prevPosition.set(_targetPosition);

  const auto PARTICLE_GRAVITY = GRAVITY * 0.75F * fixedDeltaTime;
  const float particleSpeed = 65.0F;
  const float instantSpeed = particleSpeed * fixedDeltaTime;

  // Decay the ejection impulse so the directional kick fades out over ~0.5 s.
  // Without this, direction * instantSpeed keeps pushing the particle forever.
  _direction *= 0.9F;

  // Damp horizontal velocity to simulate air resistance and surface friction.
  // This dissipates any already-accumulated XZ velocity each tick.
  _velocity.x *= 0.55F;
  _velocity.z *= 0.55F;

  Vec4 nextPosition;
  Utils::IntegrateParticleMotionVU0(&_velocity, &nextPosition, _direction,
                                    instantSpeed, PARTICLE_GRAVITY,
                                    _targetPosition, fixedDeltaTime);

  resolveCollision(nextPosition);
}

void BlockParticle::update(const float deltaTime, const M4x4* billboard) {
  // Expiry is driven by tick() (game-logic countdown via TickManager).
  // Guard here so a particle marked expired between the last tick and this
  // render frame does not produce a stale visual update.
  if (expired) return;

  M4x4 scale;
  scale.identity();
  scale.scaleX(_scale);
  scale.scaleY(_scale);

  _position.lerp(_prevPosition, _targetPosition, TyraCraft::Timer::stateLerp);

  // Apply shared billboard rotation + per-particle scale then translate to
  // world pos
  M4x4 model = *billboard * scale;
  model.translate(_position);

  const Vec4* data = Particle::rawData;
  for (size_t j = 0; j < Particle::DRAW_DATA_COUNT; j++) {
    vertex[j] = model * data[j];
  }
};
