#include "managers/particle_manager.hpp"
#include "math3d.h"

ParticlesManager::ParticlesManager() {}

ParticlesManager::~ParticlesManager() {
  delete[] rawData;

  particles.clear();
  particles.shrink_to_fit();
  particlesUVMap.clear();
  particlesUVMap.shrink_to_fit();
  particlesVertexData.clear();
  particlesVertexData.shrink_to_fit();
}

void ParticlesManager::init(Renderer* renderer, Texture* t_blocksTexture) {
  t_renderer = renderer;
  blocksTexture = t_blocksTexture;
  stapip.setRenderer(&renderer->core);
}

void ParticlesManager::update(const float deltaTime, Camera* t_camera) {
  updateParticles(deltaTime, &t_camera->position);
  if (particlesHasChanged) {
    destroyExpiredParticles();
  }
};

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
    } else {
      // Update position
      particles[i]._velocity +=
          particles[i]._direction * particleSpeed * deltaTime;

      // Reduce gravity to 85%, it was too huge for particles
      particles[i]._velocity += GRAVITY * 0.9F * deltaTime;
      particles[i]._position += particles[i]._velocity * deltaTime;

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

  particlesUVMap.shrink_to_fit();
  particlesVertexData.shrink_to_fit();
};

void ParticlesManager::destroyExpiredParticles() {
  particles.erase(std::remove_if(particles.begin(), particles.end(),
                                 [](const Particle& p) { return p.expired; }),
                  particles.end());

  particles.shrink_to_fit();
  particlesVertexData.shrink_to_fit();

  particlesHasChanged = false;
}

void ParticlesManager::createBlockParticleBatch(Block* targetBlock,
                                                const u16 size) {
  particles.reserve(size);
  particlesUVMap.reserve(size * DRAW_DATA_COUNT);
  particlesVertexData.reserve(size * DRAW_DATA_COUNT);

  for (size_t i = 0; i < size; i++) createBlockParticle(targetBlock);
}

void ParticlesManager::createBlockParticle(Block* targetBlock) {
  particles.emplace_back();
  auto& particle = particles.back();

  // Set type
  particle.type = PaticleType::Block;

  particle._lifeTime = Tyra::Math::randomf(0.6F, 1.2F);

  // Set particle rotation
  particle.rotation.identity();

  // Set particle initial velocity
  // Initiate with a random value from 2 to 5 to lift it on spawn
  particle._velocity.y = Tyra::Math::randomf(10.0F, 15.0F);

  particle._position =
      Vec4(targetBlock->hitPosition.x + (Tyra::Math::randomf(-8.0F, 8.0F)),
           targetBlock->hitPosition.y + (Tyra::Math::randomf(-8.0F, 8.0F)),
           targetBlock->hitPosition.z + (Tyra::Math::randomf(-8.0F, 8.0F)));

  // Set particle size
  particle.scale.identity();
  const auto _scale = Tyra::Math::randomf(0.5F, 1.0F);
  particle.scale.scaleX(_scale);
  particle.scale.scaleY(_scale);

  particle._direction = particle._position - targetBlock->hitPosition;
  particle._direction.normalize();

  const float UVSscale = 1.0F / 16.0F;
  const Vec4 scaleVec = Vec4(UVSscale, UVSscale, 1.0F, 0.0F);

  // Calc rand offset between row and col;
  auto xMin = Tyra::Math::randomf(targetBlock->frontMapX(),
                                  targetBlock->frontMapX() + 0.5F);
  auto xMax = Tyra::Math::randomf(targetBlock->frontMapX(),
                                  targetBlock->frontMapX() + 0.5F);
  auto yMin = Tyra::Math::randomf(targetBlock->frontMapY(),
                                  targetBlock->frontMapY() + 0.5F);
  auto yMax = Tyra::Math::randomf(targetBlock->frontMapY(),
                                  targetBlock->frontMapY() + 0.5F);

  particlesUVMap.emplace_back(Vec4(xMin, yMax, 1.0F, 0.0F) * scaleVec);
  particlesUVMap.emplace_back(Vec4(xMax, yMin, 1.0F, 0.0F) * scaleVec);
  particlesUVMap.emplace_back(Vec4(xMax, yMax, 1.0F, 0.0F) * scaleVec);

  particlesUVMap.emplace_back(Vec4(xMin, yMax, 1.0F, 0.0F) * scaleVec);
  particlesUVMap.emplace_back(Vec4(xMin, yMin, 1.0F, 0.0F) * scaleVec);
  particlesUVMap.emplace_back(Vec4(xMax, yMin, 1.0F, 0.0F) * scaleVec);
};

void ParticlesManager::destroyBlockParticles() {
  particles.clear();
  particlesUVMap.clear();
  particlesVertexData.clear();
}

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
    colorBag.single = &particleBaseColor;

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
