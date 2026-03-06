#pragma once

#include "constants.hpp"
#include "camera.hpp"
#include "entities/Block.hpp"
#include "particle.hpp"
#include "block_particle.hpp"
#include "flame_particle.hpp"
#include "smoke_particle.hpp"
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
  u8 particlesHasChanged = false;

  StaticPipeline stapip;
  Renderer* t_renderer = nullptr;

  Texture* blocksTexture = nullptr;
  Texture* particlesTexture = nullptr;

  // Pre-computed billboard rotation matrix — updated once per frame
  M4x4 billboardRotation;

  // Batch buffers for block particles (blocksTexture)
  // Capacity pre-reserved to MAX_BLOCK_PARTICLES * 6 in init()
  std::vector<Vec4>  blockBatchVerts;
  std::vector<Vec4>  blockBatchUVs;
  std::vector<Color> blockBatchColors;

  // Batch buffers for fx particles: flame + smoke (particlesTexture)
  // Capacity pre-reserved to (MAX_FLAME_PARTICLES + MAX_SMOKE_PARTICLES) * 6
  std::vector<Vec4>  fxBatchVerts;
  std::vector<Vec4>  fxBatchUVs;
  std::vector<Color> fxBatchColors;

  // Persistent bags reused every frame (pointers fixed after init)
  M4x4              blockBagMatrix;
  StaPipTextureBag  blockTexBag;
  StaPipInfoBag     blockInfoBag;
  StaPipColorBag    blockColorBag;
  StaPipBag         blockBag;

  M4x4              fxBagMatrix;
  StaPipTextureBag  fxTexBag;
  StaPipInfoBag     fxInfoBag;
  StaPipColorBag    fxColorBag;
  StaPipBag         fxBag;

  void destroyExpiredParticles();
  void destroyAllParticles();

  void loadParticlesTexture(const std::string& texturePack);
  void initBatchBags();
};
