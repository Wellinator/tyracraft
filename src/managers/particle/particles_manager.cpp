#include "managers/particle/particle_manager.hpp"
#include "managers/tick_manager.hpp"
#include "math3d.h"
#include "utils.hpp"
#include "debug.hpp"

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
#ifdef DEBUG_MODE
  if (g_debug_menu.enableRenderParticles == false) return;
#endif  // DEBUG_MODE

  auto counter = ParticlesManager::Particles.size();
  for (size_t i = 0; i < counter; i++) {
    Particle* p = ParticlesManager::Particles[i];

    // Prevent to update an expired particle
    if (p->expired) continue;
    p->fixedUpdate(fixedDeltaTime);
  }
};

void ParticlesManager::update(const float deltaTime, Camera* t_camera) {
#ifdef DEBUG_MODE
  if (g_debug_menu.enableRenderParticles == false) return;
#endif  // DEBUG_MODE

  aliveParticlesCounter = ParticlesManager::Particles.size();
  particleBags.clear();

  for (size_t i = 0; i < ParticlesManager::Particles.size(); i++) {
    Particle* p = ParticlesManager::Particles[i];
    // Prevent to update an expired particle
    if (p->expired) {
      particlesHasChanged = true;
      aliveParticlesCounter--;
      continue;
    }
    p->update(deltaTime, &t_camera->position);

    // Pre-build draw bag for this particle
    ParticleBagCache cache;
    cache.matrix = M4x4::Identity;

    cache.textureBag.coordinates = p->uv;
    cache.textureBag.texture = (p->type == ParticleType::Block)
                                   ? blocksTexture
                                   : particlesTexture;

    cache.infoBag.model = &cache.matrix;
    cache.infoBag.textureMappingType =
        Tyra::PipelineTextureMappingType::TyraNearest;
    cache.infoBag.blendingEnabled = true;

    cache.colorBag.single = &p->color;

    cache.bag.count = Particle::DRAW_DATA_COUNT;
    cache.bag.vertices = p->vertex;
    cache.bag.color = &cache.colorBag;
    cache.bag.info = &cache.infoBag;
    cache.bag.texture = &cache.textureBag;

    particleBags.emplace_back(std::move(cache));
  }
};

void ParticlesManager::tick() {
#ifdef DEBUG_MODE
  if (g_debug_menu.enableRenderParticles == false) return;
#endif  // DEBUG_MODE

  destroyExpiredParticles();
}

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
#ifdef DEBUG_MODE
  if (g_debug_menu.enableRenderParticles == false) return;
#endif  // DEBUG_MODE

  if (particleBags.empty()) return;

  t_renderer->renderer3D.usePipeline(stapip);

  // Bags were pre-built in update() — just submit them
  for (size_t i = 0; i < particleBags.size(); i++) {
    // Re-bind the info model pointer (points into the cache struct itself)
    // This is necessary because StaPipInfoBag::model is a raw pointer.
    particleBags[i].infoBag.model = &particleBags[i].matrix;
    particleBags[i].bag.info = &particleBags[i].infoBag;
    particleBags[i].bag.color = &particleBags[i].colorBag;
    particleBags[i].bag.texture = &particleBags[i].textureBag;
    stapip.core.render(&particleBags[i].bag);
  }
};
