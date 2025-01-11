#pragma once

#include "entities/Block.hpp"
#include "managers/particle/particle.hpp"
#include "managers/collision_manager.hpp"
#include "utils.hpp"

using bvh::AABB;
using bvh::AABBTree;
using bvh::Bvh_Node;
using bvh::index_t;

class BlockParticle : public Particle {
 public:
  const float _scale = Tyra::Math::randomf(0.5F, 1.0F);

  BlockParticle(Block* pBlock) : Particle(PaticleType::Block) {
    // Define life time
    _lifeTime = Tyra::Math::randomf(0.3F, 0.7F);

    // Define if is collidable
    collidable = Utils::Probability(0.1);

    // Set particle initial velocity
    // Initiate with a random value from 5 to 15 to lift it on spawn
    _velocity.y = Tyra::Math::randomf(5.0F, 15.0F);

    if (pBlock->isTarget) {
      _position =
          Vec4(pBlock->hitPosition.x + (Tyra::Math::randomf(-8.0F, 8.0F)),
               pBlock->hitPosition.y + (Tyra::Math::randomf(-8.0F, 8.0F)),
               pBlock->hitPosition.z + (Tyra::Math::randomf(-8.0F, 8.0F)));
      _direction = _position - pBlock->hitPosition;
    } else {
      _position = Vec4(pBlock->position.x + (Tyra::Math::randomf(-4.5F, 4.5F)),
                       pBlock->position.y + (Tyra::Math::randomf(-4.5F, 4.5F)),
                       pBlock->position.z + (Tyra::Math::randomf(-4.5F, 4.5F)));
      _direction = _position - pBlock->position;
    }

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

    t_color = &pBlock->baseColor;
  };

  void update(const float deltaTime, const Vec4* camPos) {
    M4x4 model, translation, rotation, scale;
    const auto PARTICLE_GRAVITY = GRAVITY * 0.9F * deltaTime;
    const float particleSpeed = 65.0F;
    const float instantSpeed = particleSpeed * deltaTime;

    _elapsedTime += deltaTime;

    if (_elapsedTime > _lifeTime) {
      expired = true;
    } else {
      // Update position
      _velocity += _direction * instantSpeed;

      // Reduce gravity to 85%, it was too huge for particles
      _velocity += PARTICLE_GRAVITY;

      // Define next position based on velocity
      const auto nextPosition = _position + (_velocity * deltaTime);

      if (collidable) {
        float closestHitDistance = -1.0f;
        const float maxCollidableDistance = _position.distanceTo(nextPosition);
        u8 willCollide = false;
        Vec4 finalHitPosition;

        // Broad phase
        const Vec4 segmentStart = _position;
        const Vec4 segmentEnd = nextPosition;

        std::vector<index_t> ni;
        g_AABBTree->intersectLine(segmentStart, segmentEnd, ni);

        Ray ray = Ray(_position, _direction);

        for (u16 i = 0; i < ni.size(); i++) {
          Entity* entity = (Entity*)g_AABBTree->user_data(ni[i]);
          if (!entity->collidable) continue;

          // Narrow Phase
          float hitDistance;
          if (ray.intersectBox(entity->minCorner, entity->maxCorner,
                               &hitDistance) &&
              hitDistance < maxCollidableDistance) {
            if (closestHitDistance == -1.0F ||
                hitDistance < closestHitDistance) {
              closestHitDistance = hitDistance;
              willCollide = true;
              finalHitPosition.set(ray.at(hitDistance));
            }
          }
        }

        if (willCollide) {
          _position.set(finalHitPosition);
          _direction = -_direction;
        } else {
          _position = nextPosition;
        }
      } else {
        _position = nextPosition;
      }

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

      // else {
      //   // Set particle position
      //   translation.identity();
      //   reinterpret_cast<Vec4*>(&translation.data[3 * 4])->set(_position);

      //   // Set particle model. M = T * R * S;
      //   // model = translation * rotation * scale;
      //   model = translation * scale;
      // }

      for (size_t j = 0; j < Particle::DRAW_DATA_COUNT; j++) {
        vertex[j] = model * Particle::rawData[j];
      }
    }
  };
};