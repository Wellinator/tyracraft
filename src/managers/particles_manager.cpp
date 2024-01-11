#include "managers/particle_manager.hpp"
#include "managers/tick_manager.hpp"
#include "math3d.h"
#include "utils.hpp"
#include "managers/collision_manager.hpp"

using bvh::AABB;
using bvh::AABBTree;
using bvh::Bvh_Node;
using bvh::index_t;

ParticlesManager::ParticlesManager() {}

ParticlesManager::~ParticlesManager() {
  t_renderer->getTextureRepository().free(particlesTexture->id);

  delete[] rawData;

  particles.clear();
  particles.shrink_to_fit();

  particlesUVMap.clear();
  particlesUVMap.shrink_to_fit();

  particlesColors.clear();
  particlesColors.shrink_to_fit();

  particlesVertexData.clear();
  particlesVertexData.shrink_to_fit();
}

void ParticlesManager::init(Renderer* renderer, Texture* t_blocksTexture,
                            const std::string& texturePack) {
  t_renderer = renderer;
  blocksTexture = t_blocksTexture;
  stapip.setRenderer(&renderer->core);
  loadParticlesTexture(texturePack);
}

void ParticlesManager::loadParticlesTexture(const std::string& texturePack) {
  const std::string path =
      "textures/texture_packs/" + texturePack + "/particle/particles.png";

  particlesTexture =
      t_renderer->core.texture.repository.add(FileUtils::fromCwd(path.c_str()));
}

Texture* ParticlesManager::getParticlesTexture() { return particlesTexture; }

void ParticlesManager::update(const float deltaTime, Camera* t_camera) {
  updateParticles(deltaTime, &t_camera->position);
};

void ParticlesManager::tick() {
  if (particlesHasChanged && isTicksCounterAt(15)) {
    destroyExpiredParticles();
  }
}

void ParticlesManager::updateParticles(const float deltaTime,
                                       const Vec4* camPos) {
  particlesVertexData.clear();

  for (size_t i = 0; i < particles.size(); i++) {
    if (particles[i].expired == true) {
      particlesHasChanged = true;
      continue;
    }

    u16 relativeIndex = i * 6;
    if (relativeIndex > 0) relativeIndex -= 1;

    particles[i]._elapsedTime += deltaTime;
    if (particles[i]._elapsedTime > particles[i]._lifeTime) {
      particles[i].expired = true;

      particlesUVMap.erase(particlesUVMap.begin() + relativeIndex,
                           particlesUVMap.begin() + relativeIndex + 6);
      particlesColors.erase(particlesColors.begin() + relativeIndex,
                            particlesColors.begin() + relativeIndex + 6);
    } else {
      // Update position
      particles[i]._velocity +=
          particles[i]._direction * particleSpeed * deltaTime;

      // Reduce gravity to 85%, it was too huge for particles
      particles[i]._velocity += GRAVITY * 0.9F * deltaTime;

      // Define next position based on velocity
      const auto nextPosition =
          particles[i]._position + (particles[i]._velocity * deltaTime);

      if (particles[i].collidable) {
        float closestHitDistance = -1.0f;
        const float maxCollidableDistance =
            particles[i]._position.distanceTo(nextPosition);
        u8 willCollide = false;
        Vec4 finalHitPosition;

        // Broad phase
        const Vec4 segmentStart = particles[i]._position;
        const Vec4 segmentEnd = nextPosition;

        std::vector<index_t> ni;
        g_AABBTree->intersectLine(segmentStart, segmentEnd, ni);

        Ray ray = Ray(particles[i]._position, particles[i]._direction);

        for (u16 i = 0; i < ni.size(); i++) {
          Entity* entity = (Entity*)g_AABBTree->user_data(ni[i]);
          if (!entity->isCollidable) continue;

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
          particles[i]._position.set(finalHitPosition);
          particles[i]._direction = -particles[i]._direction;
        } else {
          particles[i]._position = nextPosition;
        }
      } else {
        particles[i]._position = nextPosition;
      }

      /**
       * Apply billboard rotation to particle of type equals to
       * PaticleType::Block
       */
      if (particles[i].billboarded) {
        M4x4 result;
        M4x4 temp;
        M4x4::lookAt(&temp, particles[i]._position, *camPos);
        matrix_inverse(result.data, temp.data);

        // Set particle model. M = R * S;
        particles[i].model = result * particles[i].scale;
      } else {
        // Set particle position
        particles[i].translation.identity();
        reinterpret_cast<Vec4*>(&particles[i].translation.data[3 * 4])
            ->set(particles[i]._position);

        // Set particle model. M = T * R * S;
        particles[i].model = particles[i].translation * particles[i].rotation *
                             particles[i].scale;
      }

      u16 count = 0;
      particlesVertexData.emplace_back(particles[i].model * rawData[count++]);
      particlesVertexData.emplace_back(particles[i].model * rawData[count++]);
      particlesVertexData.emplace_back(particles[i].model * rawData[count++]);
      particlesVertexData.emplace_back(particles[i].model * rawData[count++]);
      particlesVertexData.emplace_back(particles[i].model * rawData[count++]);
      particlesVertexData.emplace_back(particles[i].model * rawData[count++]);
    }
  }
};

void ParticlesManager::destroyExpiredParticles() {
  particles.erase(std::remove_if(particles.begin(), particles.end(),
                                 [](const Particle& p) { return p.expired; }),
                  particles.end());

  particles.shrink_to_fit();
  particlesColors.shrink_to_fit();
  particlesUVMap.shrink_to_fit();
  particlesVertexData.shrink_to_fit();

  particlesHasChanged = false;
}

void ParticlesManager::createBlockParticleBatch(Block* block, const u16 size) {
  particles.reserve(size);
  particlesUVMap.reserve(size * DRAW_DATA_COUNT);
  particlesColors.reserve(size * DRAW_DATA_COUNT);
  particlesVertexData.reserve(size * DRAW_DATA_COUNT);

  for (size_t i = 0; i < size; i++) createBlockParticle(block);
}

void ParticlesManager::createBlockParticle(Block* block) {
  particles.emplace_back();
  auto& particle = particles.back();

  // Set type
  particle.type = PaticleType::Block;

  // Define life time
  particle._lifeTime = Tyra::Math::randomf(0.6F, 1.2F);

  // Define if is collidable
  particle.collidable = (Tyra::Math::randomi(1, 100) % 5) == 0;

  // Set particle rotation
  particle.rotation.identity();

  // Set particle initial velocity
  // Initiate with a random value from 2 to 5 to lift it on spawn
  particle._velocity.y = Tyra::Math::randomf(10.0F, 15.0F);

  if (block->isTarget) {
    particle._position =
        Vec4(block->hitPosition.x + (Tyra::Math::randomf(-8.0F, 8.0F)),
             block->hitPosition.y + (Tyra::Math::randomf(-8.0F, 8.0F)),
             block->hitPosition.z + (Tyra::Math::randomf(-8.0F, 8.0F)));
    particle._direction = particle._position - block->hitPosition;
  } else {
    particle._position =
        Vec4(block->position.x + (Tyra::Math::randomf(-4.0F, 4.0F)),
             block->position.y + (Tyra::Math::randomf(-4.0F, 4.0F)),
             block->position.z + (Tyra::Math::randomf(-4.0F, 4.0F)));
    particle._direction = particle._position - block->position;
  }

  particle._direction.normalize();

  // Set particle size
  particle.scale.identity();
  const auto _scale = Tyra::Math::randomf(0.5F, 1.0F);
  particle.scale.scaleX(_scale);
  particle.scale.scaleY(_scale);

  const float UVSscale = 1.0F / 16.0F;
  const Vec4 scaleVec = Vec4(UVSscale, UVSscale, 1.0F, 0.0F);

  // Calc rand offset between row and col;
  const u8 index = block->facesMapIndex[4];
  const u8 X = index < MAX_TEX_COLS ? index : index % MAX_TEX_COLS;
  const u8 Y = index < MAX_TEX_COLS ? 0 : std::floor(index / MAX_TEX_COLS);

  auto xMin = Tyra::Math::randomf(X, X + 0.5F);
  auto xMax = Tyra::Math::randomf(X, X + 0.5F);
  auto yMin = Tyra::Math::randomf(Y, Y + 0.5F);
  auto yMax = Tyra::Math::randomf(Y, Y + 0.5F);

  particlesUVMap.emplace_back(Vec4(xMin, yMax, 1.0F, 0.0F) * scaleVec);
  particlesUVMap.emplace_back(Vec4(xMax, yMin, 1.0F, 0.0F) * scaleVec);
  particlesUVMap.emplace_back(Vec4(xMax, yMax, 1.0F, 0.0F) * scaleVec);

  particlesUVMap.emplace_back(Vec4(xMin, yMax, 1.0F, 0.0F) * scaleVec);
  particlesUVMap.emplace_back(Vec4(xMin, yMin, 1.0F, 0.0F) * scaleVec);
  particlesUVMap.emplace_back(Vec4(xMax, yMin, 1.0F, 0.0F) * scaleVec);

  // Load particles color based in block color average
  particlesColors.emplace_back(block->baseColor);
  particlesColors.emplace_back(block->baseColor);
  particlesColors.emplace_back(block->baseColor);

  particlesColors.emplace_back(block->baseColor);
  particlesColors.emplace_back(block->baseColor);
  particlesColors.emplace_back(block->baseColor);
};

void ParticlesManager::renderBlocksParticles() {
  if (particlesVertexData.size() > 0) {
    t_renderer->renderer3D.usePipeline(stapip);

    M4x4 rawMatrix;
    rawMatrix.identity();

    StaPipTextureBag textureBag;
    textureBag.texture = blocksTexture;
    textureBag.coordinates = particlesUVMap.data();

    StaPipInfoBag infoBag;
    infoBag.model = &rawMatrix;
    infoBag.textureMappingType = Tyra::PipelineTextureMappingType::TyraNearest;

    StaPipColorBag colorBag;
    colorBag.many = particlesColors.data();

    StaPipBag bag;
    bag.count = particlesVertexData.size();
    bag.vertices = particlesVertexData.data();
    bag.color = &colorBag;
    bag.info = &infoBag;
    bag.texture = &textureBag;

    stapip.core.render(&bag);
  }
}

void ParticlesManager::render() {
  t_renderer->renderer3D.usePipeline(stapip);

  if (particles.size() > 0) renderBlocksParticles();
};
