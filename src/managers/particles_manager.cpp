#include "managers/particle_manager.hpp"
#include "math3d.h"

ParticlesManager::ParticlesManager() {}

ParticlesManager::~ParticlesManager() {
  delete[] rawData;

  blocksParticles.clear();
  blocksParticles.shrink_to_fit();
  blocksParticlesUVMap.clear();
  blocksParticlesUVMap.shrink_to_fit();
  blocksParticlesVertexData.clear();
  blocksParticlesVertexData.shrink_to_fit();
}

void ParticlesManager::init(Renderer* renderer, Texture* t_blocksTexture) {
  t_renderer = renderer;
  blocksTexture = t_blocksTexture;
  stapip.setRenderer(&renderer->core);
}

void ParticlesManager::update(const float deltaTime, Camera* t_camera) {
  updateParticles(deltaTime, &t_camera->position);
  destroyExpiredParticles();
};

void ParticlesManager::updateParticles(const float deltaTime,
                                       const Vec4* camPos) {
  blocksParticlesVertexData.clear();

  for (size_t i = 0; i < blocksParticles.size(); i++) {
    if (blocksParticles[i].expired == true) continue;

    u16 relativeIndex = i * 6;
    if (relativeIndex > 0) relativeIndex -= 1;

    blocksParticles[i]._elapsedTime += deltaTime;
    if (blocksParticles[i]._elapsedTime > blocksParticles[i]._lifeTime) {
      blocksParticles[i].expired = true;

      blocksParticlesUVMap.erase(
          blocksParticlesUVMap.begin() + relativeIndex,
          blocksParticlesUVMap.begin() + relativeIndex + 6);
    } else {
      // Update position
      // Vec4 nextPosition =
      //     blocksParticles[i]._direction * particleSpeed * deltaTime;
      blocksParticles[i]._velocity +=
          blocksParticles[i]._direction * particleSpeed * deltaTime;

      // Reduce gravity to 85%, it was too huge for particles
      blocksParticles[i]._velocity += GRAVITY * 0.9F * deltaTime;
      blocksParticles[i]._position += blocksParticles[i]._velocity * deltaTime;

      // Apply billboard rotation
      M4x4 result;
      M4x4 temp;
      M4x4::lookAt(&temp, blocksParticles[i]._position, *camPos);
      matrix_inverse(result.data, temp.data);

      blocksParticles[i].model = result * blocksParticles[i].scale;

      u16 count = 0;
      blocksParticlesVertexData.emplace_back(blocksParticles[i].model *
                                             rawData[count++]);
      blocksParticlesVertexData.emplace_back(blocksParticles[i].model *
                                             rawData[count++]);
      blocksParticlesVertexData.emplace_back(blocksParticles[i].model *
                                             rawData[count++]);
      blocksParticlesVertexData.emplace_back(blocksParticles[i].model *
                                             rawData[count++]);
      blocksParticlesVertexData.emplace_back(blocksParticles[i].model *
                                             rawData[count++]);
      blocksParticlesVertexData.emplace_back(blocksParticles[i].model *
                                             rawData[count++]);
    }
  }

  blocksParticlesUVMap.shrink_to_fit();
  blocksParticlesVertexData.shrink_to_fit();
};

void ParticlesManager::destroyExpiredParticles() {
  blocksParticles.erase(
      std::remove_if(blocksParticles.begin(), blocksParticles.end(),
                     [](const Particle& p) { return p.expired; }),
      blocksParticles.end());

  blocksParticles.shrink_to_fit();
  blocksParticlesVertexData.shrink_to_fit();
}

void ParticlesManager::createBlockParticleBatch(Block* targetBlock,
                                                const u16 size) {
  blocksParticles.reserve(size);
  blocksParticlesUVMap.reserve(size * DRAW_DATA_COUNT);
  blocksParticlesVertexData.reserve(size * DRAW_DATA_COUNT);

  for (size_t i = 0; i < size; i++) createBlockParticle(targetBlock);
}

void ParticlesManager::createBlockParticle(Block* targetBlock) {
  blocksParticles.emplace_back();
  auto& particle = blocksParticles.back();

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

  // Set particle position
  particle.translation.identity();
  reinterpret_cast<Vec4*>(&particle.translation.data[3 * 4])
      ->set(particle._position);

  // Set particle size
  particle.scale.identity();
  const auto _scale = Tyra::Math::randomf(0.5F, 1.0F);
  particle.scale.scaleX(_scale);
  particle.scale.scaleY(_scale);

  // Set particle model. M = T * R * S;
  particle.model = particle.translation * particle.rotation * particle.scale;

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

  blocksParticlesUVMap.emplace_back(Vec4(xMin, yMax, 1.0F, 0.0F) * scaleVec);
  blocksParticlesUVMap.emplace_back(Vec4(xMax, yMin, 1.0F, 0.0F) * scaleVec);
  blocksParticlesUVMap.emplace_back(Vec4(xMax, yMax, 1.0F, 0.0F) * scaleVec);

  blocksParticlesUVMap.emplace_back(Vec4(xMin, yMax, 1.0F, 0.0F) * scaleVec);
  blocksParticlesUVMap.emplace_back(Vec4(xMin, yMin, 1.0F, 0.0F) * scaleVec);
  blocksParticlesUVMap.emplace_back(Vec4(xMax, yMin, 1.0F, 0.0F) * scaleVec);

  auto vert = 0;
  blocksParticlesVertexData.emplace_back(particle.model * rawData[vert++]);
  blocksParticlesVertexData.emplace_back(particle.model * rawData[vert++]);
  blocksParticlesVertexData.emplace_back(particle.model * rawData[vert++]);
  blocksParticlesVertexData.emplace_back(particle.model * rawData[vert++]);
  blocksParticlesVertexData.emplace_back(particle.model * rawData[vert++]);
  blocksParticlesVertexData.emplace_back(particle.model * rawData[vert++]);
};

void ParticlesManager::destroyBlockParticles() {
  blocksParticles.clear();
  blocksParticlesUVMap.clear();
  blocksParticlesVertexData.clear();
}

void ParticlesManager::renderBlocksParticles() {
  if (blocksParticlesVertexData.size() > 0) {
    t_renderer->renderer3D.usePipeline(stapip);

    M4x4 rawMatrix;
    rawMatrix.identity();

    StaPipTextureBag textureBag;
    textureBag.texture = blocksTexture;
    textureBag.coordinates = blocksParticlesUVMap.data();

    StaPipInfoBag infoBag;
    infoBag.model = &rawMatrix;
    infoBag.textureMappingType = Tyra::PipelineTextureMappingType::TyraNearest;

    StaPipColorBag colorBag;
    colorBag.single = &particleBaseColor;

    StaPipBag bag;
    bag.count = blocksParticlesVertexData.size();
    bag.vertices = blocksParticlesVertexData.data();
    bag.color = &colorBag;
    bag.info = &infoBag;
    bag.texture = &textureBag;

    stapip.core.render(&bag);
  }
}

void ParticlesManager::render() {
  t_renderer->renderer3D.usePipeline(stapip);

  if (blocksParticles.size() > 0) renderBlocksParticles();
};
