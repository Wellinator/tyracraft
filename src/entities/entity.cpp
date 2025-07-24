#include "entities/entity.hpp"

Entity::Entity(Level* level, const EntityType type) : entity_type(type) {
  pLevel = level;
};

Entity::~Entity() {};

void Entity::tick() {};

const BBox Entity::getHitBox() {
  M4x4 translation = M4x4::Identity;
  translation.translate(_targetPosition);
  return bbox->getTransformed(translation);
};

const BBox Entity::getHitBox(const M4x4& model) {
  return bbox->getTransformed(model);
};

const float Entity::getHeight() { return 0.0f; }

void Entity::resolveOutOfWorldBoundaries() {
  TYRA_TRAP("resolveOutOfWorldBoundaries not implemented!");
};

void Entity::updateTerrainHeightAtEntityPosition() {
  BBox entityBB = getHitBox();
  Vec4 minEntity, maxEntity;
  entityBB.getMinMax(&minEntity, &maxEntity);

  // Padding used to fix the position offset
  const float EPSLON = 0.0001f;

  terrainHeight.reset();
  underEntity = nullptr;

  // Prepate the raycast
  const Vec4 offset = Vec4(0, 40, 0);
  Vec4 segmentStart = maxEntity + offset;
  Vec4 segmentEnd = minEntity - offset;

  std::vector<int32_t> ni;
  g_AABBTree->intersectLine(segmentStart, segmentEnd, ni);

  for (u16 i = 0; i < ni.size(); i++) {
    Entity* entity = (Entity*)g_AABBTree->user_data(ni[i]);
    if (!entity->collidable) continue;

    // is under or above entity
    if (minEntity.x <= entity->maxCorner.x &&
        maxEntity.x >= entity->minCorner.x &&
        minEntity.z <= entity->maxCorner.z &&
        maxEntity.z >= entity->minCorner.z) {
      const float minHeight = entity->maxCorner.y + EPSLON;
      if (minEntity.y >= minHeight && minHeight > terrainHeight.minHeight) {
        terrainHeight.minHeight = minHeight;
        underEntity = entity;
      }

      const float maxHeight = entity->minCorner.y - EPSLON;
      if (maxEntity.y < maxHeight && maxHeight < terrainHeight.maxHeight) {
        terrainHeight.maxHeight = maxHeight;
      }
    }
  }
}

/** Update entity position by gravity and update index of current block */
void Entity::updateYPosition(const float nextYPos) {
  float resultY = nextYPos;
  const float worldMinHeight = OVERWORLD_MIN_HEIGH * DOUBLE_BLOCK_SIZE;
  const float worldMaxHeight = OVERWORLD_MAX_HEIGH * DOUBLE_BLOCK_SIZE;
  const float enityHeight = getHeight();

  if (nextYPos + enityHeight > worldMaxHeight || nextYPos < worldMinHeight) {
    return resolveOutOfWorldBoundaries();
  }

  const float heightLimit = terrainHeight.maxHeight - enityHeight;

  if (resultY < terrainHeight.minHeight) {
    resultY = terrainHeight.minHeight;
    velocity.y = 0.0f;
    isOnGround = true;
  } else if (resultY >= heightLimit) {
    resultY = heightLimit;
    velocity.y = -velocity.y;
    isOnGround = false;
  }

  // Finally updates gravity after checks
  _targetPosition.y = resultY;
}
