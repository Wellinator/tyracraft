#include "entities/entity.hpp"
#include "managers/block/vertex_block_data.hpp"
#include "managers/model_builder.hpp"
#include "managers/block/StaticBlockRepository.hpp"

class Block;

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
  Vec4 selfMin, selfMax;
  entityBB.getMinMax(&selfMin, &selfMax);
  terrainHeight.reset();

  // Padding used to fix the position offset
  const float EPSLON = 0.0001f;

  // Prepate the raycast
  // Should I inflate by Y velocity?
  const Vec4 offset = Vec4(0, 40, 0);
  Vec4 upperPosition = selfMax + offset;
  Vec4 lowerPosition = selfMin - offset;
  Vec4 upperOffset = pLevel->worldPosToOffset(upperPosition);
  Vec4 lowerOffset = pLevel->worldPosToOffset(lowerPosition);
  Vec4 currentOffset(upperOffset);

  // Loop from upper to lower position until we find the first solid block
  uint8_t currentBlockID = pLevel->GetBlockFromMap(&currentOffset);
  Blocks currentBlockType = static_cast<Blocks>(currentBlockID);
  StaticBlockRepository* staticBlockRepo = StaticBlockRepository::getInstance();
  Block* templateBlock = nullptr;

  while (true) {
    if (currentBlockType != Blocks::AIR_BLOCK) {
      templateBlock = staticBlockRepo->getBlockTemplate(currentBlockType);
      if (templateBlock->isCollidable()) {
        M4x4 model = ModelBuilder_BuildModel(&currentOffset);

        Vec4 min, max;
        BBox blockBBox = VertexBlockData::getRawBBoxByOffset(&currentOffset)
                             ->getTransformed(model);
        blockBBox.getMinMax(&min, &max);

        // is under or above entity
        const float minHeight = max.y + EPSLON;
        if (selfMin.y >= minHeight && minHeight > terrainHeight.minHeight) {
          terrainHeight.minHeight = minHeight;
          terrainHeight.lowerBlockType = currentBlockID;
        }

        const float maxHeight = min.y - EPSLON;
        if (selfMax.y < maxHeight && maxHeight < terrainHeight.maxHeight) {
          terrainHeight.maxHeight = maxHeight;
          terrainHeight.upperBlockType = currentBlockID;
        }
      }
    }

    // Move down by one block
    currentOffset.y--;
    if (currentOffset.y < lowerOffset.y) {
      break;
    }

    currentBlockID = pLevel->GetBlockFromMap(&currentOffset);
    currentBlockType = static_cast<Blocks>(currentBlockID);
  }

  // std::vector<int32_t> ni;
  // g_AABBTree->intersectLine(segmentStart, segmentEnd, ni);

  // for (u16 i = 0; i < ni.size(); i++) {
  //   Entity* entity = (Entity*)g_AABBTree->user_data(ni[i]);
  //   if (!entity->collidable) continue;

  //   // is under or above entity
  //   if (selfMin.x <= entity->maxCorner.x && selfMax.x >= entity->minCorner.x
  //   &&
  //       selfMin.z <= entity->maxCorner.z && selfMax.z >= entity->minCorner.z)
  //       {
  //     const float minHeight = entity->maxCorner.y + EPSLON;
  //     if (selfMin.y >= minHeight && minHeight > terrainHeight.minHeight) {
  //       terrainHeight.minHeight = minHeight;
  //     }

  //     const float maxHeight = entity->minCorner.y - EPSLON;
  //     if (selfMax.y < maxHeight && maxHeight < terrainHeight.maxHeight) {
  //       terrainHeight.maxHeight = maxHeight;
  //     }
  //   }
  // }
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
