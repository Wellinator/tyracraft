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

void ParticlesManager::update(const float deltaTime, Camera* t_camera) {
  updateParticles(deltaTime, &t_camera->position);
};

void ParticlesManager::tick() { destroyExpiredParticles(); }

void ParticlesManager::updateParticles(const float deltaTime,
                                       const Vec4* camPos) {
  // colors.clear();
  // vertex.clear();
  // uv.clear();

  aliveParticlesCounter = ParticlesManager::Particles.size();

  for (size_t i = 0; i < ParticlesManager::Particles.size(); i++) {
    Particle* p = ParticlesManager::Particles[i];

    // Prevent to update an expired particle
    if (p->expired == true) {
      particlesHasChanged = true;
      aliveParticlesCounter--;
      continue;
    }

    p->update(deltaTime, camPos);

    // if (particle.isAllive()) {
    //   colors.insert(colors.end(), 6, *particle.t_color);
    //   vertex.insert(vertex.end(), std::begin(particle.vertex),
    //                 std::end(particle.vertex));
    //   uv.insert(uv.end(), std::begin(particle.uv), std::end(particle.uv));
    // }
  }
};

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

    // colors.shrink_to_fit();
    // vertex.shrink_to_fit();
    // uv.shrink_to_fit();
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

    M4x4 rawMatrix;
    rawMatrix.identity();

    StaPipTextureBag textureBag;
    textureBag.coordinates = p->uv;

    if (p->type == PaticleType::Block) {
      textureBag.texture = blocksTexture;
    } else {
      textureBag.texture = particlesTexture;
    }

    StaPipInfoBag infoBag;
    infoBag.model = &rawMatrix;
    infoBag.textureMappingType = Tyra::PipelineTextureMappingType::TyraNearest;

    StaPipColorBag colorBag;
    colorBag.single = p->t_color;

    StaPipBag bag;
    bag.count = Particle::DRAW_DATA_COUNT;
    bag.vertices = p->vertex;

    bag.color = &colorBag;
    bag.info = &infoBag;
    bag.texture = &textureBag;

    stapip.core.render(&bag);
  }
};
