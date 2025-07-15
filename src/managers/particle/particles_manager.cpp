#include "managers/particle/particle_manager.hpp"
#include "managers/tick_manager.hpp"
#include "math3d.h"
#include "utils.hpp"

// Static values init
std::vector<Particle*> ParticlesManager::Particles = {};

ParticlesManager::ParticlesManager() {}

ParticlesManager::~ParticlesManager() {
  t_renderer->getTextureRepository().free(particlesTexture->id);
  destroyAllParticles();
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

void ParticlesManager::fixedUpdate(const float& fixedDeltaTime) {
  auto counter = ParticlesManager::Particles.size();
  for (size_t i = 0; i < counter; i++) {
    Particle* p = ParticlesManager::Particles[i];

    // Prevent to update an expired particle
    if (p->expired) continue;
    p->fixedUpdate(fixedDeltaTime);
  }
};

void ParticlesManager::update(const float deltaTime, Camera* t_camera) {
  aliveParticlesCounter = ParticlesManager::Particles.size();
  for (size_t i = 0; i < ParticlesManager::Particles.size(); i++) {
    Particle* p = ParticlesManager::Particles[i];
    // Prevent to update an expired particle
    if (p->expired) {
      particlesHasChanged = true;
      aliveParticlesCounter--;
      continue;
    }
    p->update(deltaTime, &t_camera->position);
  }
};

void ParticlesManager::tick() { destroyExpiredParticles(); }

void ParticlesManager::destroyExpiredParticles() {
  if (particlesHasChanged) {
    auto& part = ParticlesManager::Particles;
    part.erase(std::remove_if(part.begin(), part.end(),
                              [](Particle* p) {
                                const auto hasExpired = p->expired;
                                if (hasExpired) delete p;
                                return hasExpired;
                              }),
               part.end());

    part.shrink_to_fit();
    particlesHasChanged = false;
  }
}

void ParticlesManager::destroyAllParticles() {
  Particle** pData = ParticlesManager::Particles.data();
  size_t size = ParticlesManager::Particles.size();

  for (size_t i = 0; i < size; i++) {
    delete pData[i];
  }

  ParticlesManager::Particles.clear();
  ParticlesManager::Particles.shrink_to_fit();

  colors.clear();
  colors.shrink_to_fit();
  vertex.clear();
  vertex.shrink_to_fit();
  uv.clear();
  uv.shrink_to_fit();

  particlesHasChanged = false;
}

Particle* ParticlesManager::GetParticleById(const u32 id) {
  Particle** pData = ParticlesManager::Particles.data();
  size_t size = ParticlesManager::Particles.size();

  for (size_t i = 0; i < size; i++) {
    if (pData[i]->id == id) return pData[i];
  }

  return nullptr;
}

void ParticlesManager::EmitParticle(Particle* particle) {
  ParticlesManager::Particles.emplace_back(particle);
}

void ParticlesManager::createBlockParticleBatch(Block* block, const u16 size) {
  ParticlesManager::Particles.reserve(ParticlesManager::Particles.size() +
                                      size);
  for (size_t i = 0; i < size; i++) createBlockParticle(block);
}

void ParticlesManager::createBlockParticle(Block* pBlock) {
  Particle* p = new BlockParticle(pBlock);
  ParticlesManager::EmitParticle(p);
};

void ParticlesManager::render() {
  Particle** pData = ParticlesManager::Particles.data();
  size_t size = ParticlesManager::Particles.size();

  t_renderer->renderer3D.usePipeline(stapip);

  for (size_t i = 0; i < size; i++) {
    Particle* p = pData[i];

    if (p->expired) continue;

    M4x4 rawMatrix = M4x4::Identity;

    StaPipTextureBag textureBag;
    textureBag.coordinates = p->uv;

    if (p->type == ParticleType::Block) {
      textureBag.texture = blocksTexture;
    } else {
      textureBag.texture = particlesTexture;
    }

    StaPipInfoBag infoBag;
    infoBag.model = &rawMatrix;
    infoBag.textureMappingType = Tyra::PipelineTextureMappingType::TyraNearest;
    infoBag.blendingEnabled = true;

    StaPipColorBag colorBag;
    colorBag.single = &p->color;

    StaPipBag bag;
    bag.count = Particle::DRAW_DATA_COUNT;
    bag.vertices = p->vertex;

    bag.color = &colorBag;
    bag.info = &infoBag;
    bag.texture = &textureBag;

    stapip.core.render(&bag);
  }
};
