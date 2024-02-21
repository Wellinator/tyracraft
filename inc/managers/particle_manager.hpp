#pragma once

#include "constants.hpp"
#include "camera.hpp"
#include "entities/Block.hpp"
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

class Particle {
 public:
  PaticleType type;

  int id = 0;

  u8 billboarded = true;
  u8 expired = false;
  u8 collidable = false;
  u8 col;
  u8 row;

  M4x4 model, translation, rotation, scale;

  float _elapsedTime = 0;
  float _lifeTime = 0;
  Vec4 _velocity;
  Vec4 _position;
  Vec4 _direction;

  Color* t_color = nullptr;
  Vec4 uv[6] = {};
  Vec4 vertex[6] = {};
};

class ParticlesManager {
 public:
  ParticlesManager();
  ~ParticlesManager();

  void init(Renderer* renderer, Texture* t_blocksTexture,
            const std::string& texturePack);
  void update(const float deltaTime, Camera* t_camera);
  void tick();

  void render();
  void renderBlocksParticles();

  void createBlockParticle(Block* block);
  void createBlockParticleBatch(Block* block, const u16 size);

  Texture* getParticlesTexture();

  inline uint16_t getParticlesCounter() { return particles.size(); };

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

  StaticPipeline stapip;
  Renderer* t_renderer = nullptr;

  Texture* blocksTexture = nullptr;
  Texture* particlesTexture = nullptr;

  void updateParticles(const float deltaTime, const Vec4* camPos);
  void destroyExpiredParticles();

  void loadParticlesTexture(const std::string& texturePack);
};
