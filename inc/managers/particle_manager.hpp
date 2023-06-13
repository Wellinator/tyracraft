#pragma once

#include "constants.hpp"
#include "camera.hpp"
#include "entities/particle.hpp"
#include "entities/Block.hpp"
#include <tyra>
#include <math.h>

using Tyra::Color;
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

  void init(Renderer* renderer, Texture* t_blocksTexture);
  void update(const float deltaTime, Camera* t_camera);

  void render();
  void renderBlocksParticles();

  void createBlockParticle(Block* targetBlock);
  void createBlockParticleBatch(Block* targetBlock, const u16 size);
  void destroyBlockParticles();

 private:
  Vec4 camPos;
  const float particleSpeed = 80.0F;

  const u8 DRAW_DATA_COUNT = 6;
  Vec4* rawData = new Vec4[DRAW_DATA_COUNT]{
      Vec4(1.0F, -1.0F, -1.0),  Vec4(-1.0F, 1.0F, -1.0),
      Vec4(-1.0F, -1.0F, -1.0), Vec4(1.0F, -1.0F, -1.0),
      Vec4(1.0F, 1.0F, -1.0),   Vec4(-1.0F, 1.0F, -1.0)};

  std::vector<Particle> blocksParticles;
  std::vector<Vec4> blocksParticlesUVMap;
  std::vector<Vec4> blocksParticlesVertexData;

  Color particleBaseColor = Color(120, 120, 120);

  StaticPipeline stapip;
  Renderer* t_renderer = nullptr;

  Texture* blocksTexture = nullptr;

  void updateParticles(const float deltaTime, const Vec4* camPos);
  void destroyExpiredParticles();
};
