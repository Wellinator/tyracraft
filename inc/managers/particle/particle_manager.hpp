#pragma once

#include "constants.hpp"
#include "camera.hpp"
#include "entities/Block.hpp"
#include "particle.hpp"
#include "block_particle.hpp"
#include "flame_particle.hpp"
#include <tyra>
#include <math.h>
#include <vector>

using Tyra::Color;
using Tyra::FileUtils;
using Tyra::M4x4;
using Tyra::Renderer;
using Tyra::Renderer3D;
using Tyra::StaPipBag;
using Tyra::StaPipColorBag;
using Tyra::StaPipInfoBag;
using Tyra::StaPipTextureBag;
using Tyra::StaticPipeline;
using Tyra::Texture;
using Tyra::Vec4;

class ParticlesManager {
 public:
  ParticlesManager();
  ~ParticlesManager();

  void init(Renderer* renderer, Texture* t_blocksTexture,
            const std::string& texturePack);
  void fixedUpdate(const float& fixedDeltaTime);
  void update(const float deltaTime, Camera* t_camera);
  void tick();
  void render();

  Texture* getParticlesTexture();

  static std::vector<Particle*> Particles;

  uint16_t aliveParticlesCounter = 0;
  inline uint16_t getParticlesCounter() {
    return ParticlesManager::Particles.size();
  };

  static Particle* GetParticleById(const u32 id);
  static void EmitParticle(Particle* particle);

  void createBlockParticle(Block* pBlock);
  void createBlockParticleBatch(Block* pBlock, const u16 size);

 private:
  Vec4 camPos;

  u8 particlesHasChanged = false;

  StaticPipeline stapip;
  Renderer* t_renderer = nullptr;

  Texture* blocksTexture = nullptr;
  Texture* particlesTexture = nullptr;

  std::vector<Color> colors;
  std::vector<Vec4> vertex;
  std::vector<Vec4> uv;

  void destroyExpiredParticles();
  void destroyAllParticles();

  void loadParticlesTexture(const std::string& texturePack);
};
