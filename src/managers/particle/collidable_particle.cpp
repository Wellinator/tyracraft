#include "managers/particle/collidable_particle.hpp"

CollidableParticle::CollidableParticle(const ParticleType& _type)
    : Particle(_type) {}

u8 CollidableParticle::isBlockCollidableAtWorldPos(Level* level,
                                                   const Vec4& worldPos) {
  Vec4 offset = level->worldPosToOffset(worldPos);
  const u16 ox = (u16)offset.x;
  const u16 oy = (u16)offset.y;
  const u16 oz = (u16)offset.z;

  // Out-of-bounds is treated as solid to keep particles inside the world.
  if (!level->BoundCheckMap(ox, oy, oz)) return 1;

  const u8 blockType = level->GetBlockFromMap(ox, oy, oz);
  if (blockType == (u8)Blocks::AIR_BLOCK) return 0;

  Block* tmpl =
      StaticBlockRepository::getInstance()->getBlockTemplate(blockType);
  return tmpl ? tmpl->isCollidable() : 0;
}

void CollidableParticle::resolveCollision(Vec4& nextPosition) {
  if (expired) return;

  if (!collidable) {
    _targetPosition = nextPosition;
    return;
  }

  Level* level = Level::getInstance();
  if (!level) {
    _targetPosition = nextPosition;
    return;
  }

  // Axis-separated collision: test each axis independently so the particle
  // slides along surfaces rather than stopping dead on contact.
  Vec4 resolved = _targetPosition;

  // --- X axis ---
  Vec4 testX(nextPosition.x, resolved.y, resolved.z);
  if (isBlockCollidableAtWorldPos(level, testX)) {
    _velocity.x = 0.0F;
  } else {
    resolved.x = nextPosition.x;
  }

  // --- Y axis ---
  Vec4 testY(resolved.x, nextPosition.y, resolved.z);
  if (isBlockCollidableAtWorldPos(level, testY)) {
    _velocity.y = 0.0F;
  } else {
    resolved.y = nextPosition.y;
  }

  // --- Z axis ---
  Vec4 testZ(resolved.x, resolved.y, nextPosition.z);
  if (isBlockCollidableAtWorldPos(level, testZ)) {
    _velocity.z = 0.0F;
  } else {
    resolved.z = nextPosition.z;
  }

  _targetPosition = resolved;
}
