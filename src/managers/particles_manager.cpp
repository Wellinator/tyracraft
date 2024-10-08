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

void ParticlesManager::tick() { destroyExpiredParticles(); }

void ParticlesManager::updateParticles(const float deltaTime,
                                       const Vec4* camPos) {
  colors.clear();
  vertex.clear();
  uv.clear();

  const auto PARTICLE_GRAVITY = GRAVITY * 0.9F * deltaTime;
  const float instantSpeed = particleSpeed * deltaTime;

  for (size_t i = 0; i < particles.size(); i++) {
    if (particles[i].expired == true) {
      particlesHasChanged = true;
      continue;
    }

    particles[i]._elapsedTime += deltaTime;
    if (particles[i]._elapsedTime > particles[i]._lifeTime) {
      particles[i].expired = true;
    } else {
      // Update position
      particles[i]._velocity += particles[i]._direction * instantSpeed;

      // Reduce gravity to 85%, it was too huge for particles
      particles[i]._velocity += PARTICLE_GRAVITY;

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
        Utils::inverseMatrix(&result, &temp);

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

      for (size_t j = 0; j < 6; j++) {
        particles[i].vertex[j] = particles[i].model * rawData[j];
      }

      colors.insert(colors.end(), 6, *particles[i].t_color);
      vertex.insert(vertex.end(), std::begin(particles[i].vertex),
                    std::end(particles[i].vertex));
      uv.insert(uv.end(), std::begin(particles[i].uv),
                std::end(particles[i].uv));
    }
  }
};

void ParticlesManager::destroyExpiredParticles() {
  if (particlesHasChanged) {
    particles.erase(std::remove_if(particles.begin(), particles.end(),
                                   [](const Particle& p) { return p.expired; }),
                    particles.end());

    particles.shrink_to_fit();
    particlesHasChanged = false;

    colors.shrink_to_fit();
    vertex.shrink_to_fit();
    uv.shrink_to_fit();
  }
}

void ParticlesManager::createBlockParticleBatch(Block* block, const u16 size) {
  particles.reserve(size);
  for (size_t i = 0; i < size; i++) createBlockParticle(block);
}

void ParticlesManager::createBlockParticle(Block* block) {
  particles.emplace_back();
  auto& particle = particles.back();

  // Set type
  particle.type = PaticleType::Block;

  // Define life time
  particle._lifeTime = Tyra::Math::randomf(0.3F, 0.7F);

  // Define if is collidable
  particle.collidable = Utils::Probability(0.1);

  // Set particle rotation
  particle.rotation.identity();

  // Set particle initial velocity
  // Initiate with a random value from 5 to 15 to lift it on spawn
  particle._velocity.y = Tyra::Math::randomf(5.0F, 15.0F);

  if (block->isTarget) {
    particle._position =
        Vec4(block->hitPosition.x + (Tyra::Math::randomf(-8.0F, 8.0F)),
             block->hitPosition.y + (Tyra::Math::randomf(-8.0F, 8.0F)),
             block->hitPosition.z + (Tyra::Math::randomf(-8.0F, 8.0F)));
    particle._direction = particle._position - block->hitPosition;
  } else {
    particle._position =
        Vec4(block->position.x + (Tyra::Math::randomf(-4.5F, 4.5F)),
             block->position.y + (Tyra::Math::randomf(-4.5F, 4.5F)),
             block->position.z + (Tyra::Math::randomf(-4.5F, 4.5F)));
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

  particle.uv[0] = Vec4(xMin, yMax, 1.0F, 0.0F) * scaleVec;
  particle.uv[1] = Vec4(xMax, yMin, 1.0F, 0.0F) * scaleVec;
  particle.uv[2] = Vec4(xMax, yMax, 1.0F, 0.0F) * scaleVec;
  particle.uv[3] = Vec4(xMin, yMax, 1.0F, 0.0F) * scaleVec;
  particle.uv[4] = Vec4(xMin, yMin, 1.0F, 0.0F) * scaleVec;
  particle.uv[5] = Vec4(xMax, yMin, 1.0F, 0.0F) * scaleVec;

  particle.t_color = &block->baseColor;
};

void ParticlesManager::renderBlocksParticles() {
  t_renderer->renderer3D.usePipeline(stapip);

  M4x4 rawMatrix;
  rawMatrix.identity();

  StaPipTextureBag textureBag;
  textureBag.texture = blocksTexture;
  textureBag.coordinates = uv.data();

  StaPipInfoBag infoBag;
  infoBag.model = &rawMatrix;
  infoBag.textureMappingType = Tyra::PipelineTextureMappingType::TyraNearest;

  StaPipColorBag colorBag;
  colorBag.single = colors.data();

  StaPipBag bag;
  bag.count = vertex.size();
  bag.vertices = vertex.data();

  bag.color = &colorBag;
  bag.info = &infoBag;
  bag.texture = &textureBag;

  stapip.core.render(&bag);
}

void ParticlesManager::render() {
  if (particles.size() > 0) renderBlocksParticles();
};
