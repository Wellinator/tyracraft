#pragma once

#include "constants.hpp"
#include "camera.hpp"
#include "entities/Block.hpp"
#include <tyra>
#include <math.h>

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

struct Particle {
  PaticleType type;

  u8 billboarded = true;
  u8 expired = false;
  u8 col;
  u8 row;

  M4x4 model, translation, rotation, scale;

  float _elapsedTime = 0;
  float _lifeTime;
  Vec4 _velocity;
  Vec4 _position;
  Vec4 _direction;
};

class ParticlesManager {
 public:
  ParticlesManager();
  ~ParticlesManager();

  void init(Renderer* renderer, Texture* t_blocksTexture,
            const std::string& texturePack);
  void update(const float deltaTime, Camera* t_camera);

  void render();
  void renderBlocksParticles();

  void createBlockParticle(Block* block);
  void createBlockParticleBatch(Block* block, const u16 size);

  Texture* getParticlesTexture();

 private:
  Vec4 camPos;
  const float particleSpeed = 65.0F;

  u8 particlesHasChanged = false;

  const u8 DRAW_DATA_COUNT = 6;
  Vec4* rawData = new Vec4[DRAW_DATA_COUNT]{
      Vec4(1.0F, -1.0F, -1.0),  Vec4(-1.0F, 1.0F, -1.0),
      Vec4(-1.0F, -1.0F, -1.0), Vec4(1.0F, -1.0F, -1.0),
      Vec4(1.0F, 1.0F, -1.0),   Vec4(-1.0F, 1.0F, -1.0)};

  std::vector<Particle> particles;
  std::vector<Color> particlesColors;
  std::vector<Vec4> particlesUVMap;
  std::vector<Vec4> particlesVertexData;

  StaticPipeline stapip;
  Renderer* t_renderer = nullptr;

  Texture* blocksTexture = nullptr;
  Texture* particlesTexture = nullptr;

  void updateParticles(const float deltaTime, const Vec4* camPos);
  void destroyExpiredParticles();

  void loadParticlesTexture(const std::string& texturePack);
};
