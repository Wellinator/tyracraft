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
  SmokeParticle::destroyUVLUT();
  destroyAllParticles();
}

void ParticlesManager::init(Renderer* renderer, Texture* t_blocksTexture,
                            const std::string& texturePack) {
  t_renderer = renderer;
  blocksTexture = t_blocksTexture;
  stapip.setRenderer(&renderer->core);
  loadParticlesTexture(texturePack);

  // Pre-reserve batch buffers to avoid per-frame heap allocation
  const u32 blockCap = MAX_BLOCK_PARTICLES * Particle::DRAW_DATA_COUNT;
  blockBatchVerts.reserve(blockCap);
  blockBatchUVs.reserve(blockCap);
  blockBatchColors.reserve(blockCap);

  const u32 fxCap = (MAX_FLAME_PARTICLES + MAX_SMOKE_PARTICLES) * Particle::DRAW_DATA_COUNT;
  fxBatchVerts.reserve(fxCap);
  fxBatchUVs.reserve(fxCap);
  fxBatchColors.reserve(fxCap);

  // Pre-reserve legacy vector (avoids reallocations at runtime)
  Particles.reserve(MAX_PARTICLES);

  // Build static parts of the render bags (pointers stay valid after reserve)
  initBatchBags();

  // Init smoke UV look-up table (static, built once)
  SmokeParticle::initUVLUT();
}

void ParticlesManager::initBatchBags() {
  blockBagMatrix.identity();

  blockInfoBag.model = &blockBagMatrix;
  blockInfoBag.textureMappingType = Tyra::PipelineTextureMappingType::TyraNearest;
  blockInfoBag.blendingEnabled = true;

  blockTexBag.texture = blocksTexture;
  // .coordinates set in update() to batch buffer data pointer

  blockColorBag.many = nullptr;  // set in update()

  blockBag.count = 0;
  blockBag.vertices = nullptr;   // set in update()
  blockBag.color = &blockColorBag;
  blockBag.info = &blockInfoBag;
  blockBag.texture = &blockTexBag;

  // --- FX bag ---
  fxBagMatrix.identity();

  fxInfoBag.model = &fxBagMatrix;
  fxInfoBag.textureMappingType = Tyra::PipelineTextureMappingType::TyraNearest;
  fxInfoBag.blendingEnabled = true;

  fxTexBag.texture = particlesTexture;

  fxColorBag.many = nullptr;  // set in update()

  fxBag.count = 0;
  fxBag.vertices = nullptr;   // set in update()
  fxBag.color = &fxColorBag;
  fxBag.info = &fxInfoBag;
  fxBag.texture = &fxTexBag;
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
#endif

  const size_t counter = ParticlesManager::Particles.size();
  for (size_t i = 0; i < counter; i++) {
    Particle* p = ParticlesManager::Particles[i];
    if (p->expired) continue;
    p->fixedUpdate(fixedDeltaTime);
  }
}

void ParticlesManager::update(const float deltaTime, Camera* t_camera) {
#ifdef DEBUG_MODE
  if (g_debug_menu.enableRenderParticles == false) return;
#endif

  // --- Compute billboard rotation matrix ONCE per frame ---
  // All billboarded particles can reuse this instead of calling lookAt+inverse each.
  // IMPORTANT: this matrix must contain only rotation. Translation is applied
  // per particle later via model.translate(_position).
  {
    const Vec4 origin = Vec4(0.0F, 0.0F, 0.0F, 1.0F);
    const Vec4 toCamera = t_camera->position - t_camera->looksAt;
    M4x4 temp;
    M4x4::lookAt(&temp, origin, toCamera);
    Utils::inverseMatrix(&billboardRotation, &temp);
  }

  // --- Clear batch buffers (capacity retained by reserve) ---
  blockBatchVerts.clear();
  blockBatchUVs.clear();
  blockBatchColors.clear();
  fxBatchVerts.clear();
  fxBatchUVs.clear();
  fxBatchColors.clear();

  aliveParticlesCounter = 0;

  const size_t total = ParticlesManager::Particles.size();
  for (size_t i = 0; i < total; i++) {
    Particle* p = ParticlesManager::Particles[i];
    if (p->expired) {
      particlesHasChanged = true;
      continue;
    }

    // Distance culling: disable grid-based collision for block particles far
    // from the camera. The collidable flag is read in
    // CollidableParticle::resolveCollision() on the next physics tick —
    // disabling it here is intentionally one tick early and avoids any extra
    // branch inside the physics hot path.
    if (p->type == ParticleType::Block && p->collidable) {
      static constexpr float kCullDistSq =
          (MAX_DRAW_DISTANCE * BLOCK_SIZE * 2.0F) *
          (MAX_DRAW_DISTANCE * BLOCK_SIZE * 2.0F);
      const float dx = p->_position.x - t_camera->position.x;
      const float dy = p->_position.y - t_camera->position.y;
      const float dz = p->_position.z - t_camera->position.z;
      if ((dx * dx + dy * dy + dz * dz) > kCullDistSq) p->collidable = false;
    }

    p->update(deltaTime, &billboardRotation);
    if (p->expired) {
      particlesHasChanged = true;
      continue;
    }
    aliveParticlesCounter++;

    const bool isBlock = (p->type == ParticleType::Block);

    std::vector<Vec4>*  verts  = isBlock ? &blockBatchVerts  : &fxBatchVerts;
    std::vector<Vec4>*  uvs    = isBlock ? &blockBatchUVs    : &fxBatchUVs;
    std::vector<Color>* colors = isBlock ? &blockBatchColors : &fxBatchColors;

    // Expand 6 verts/uvs per particle into flat batch buffer
    for (u8 j = 0; j < Particle::DRAW_DATA_COUNT; j++) {
      verts->push_back(p->vertex[j]);
      uvs->push_back(p->uv[j]);
      colors->push_back(p->color);
    }
  }

#ifdef DEBUG_MODE
  if (aliveParticlesCounter > 0 && blockBatchVerts.empty() && fxBatchVerts.empty()) {
    TYRA_LOG("[Particles] Alive particles but render batches are empty");
  }
#endif
}

void ParticlesManager::tick() {
#ifdef DEBUG_MODE
  if (g_debug_menu.enableRenderParticles == false) return;
#endif
  // Advance per-particle game logic (lifetime, state changes, etc.).
  // This runs inside TickManager so it is naturally frozen when ticks pause
  // and scales with any tick-rate changes.
  const size_t counter = ParticlesManager::Particles.size();
  for (size_t i = 0; i < counter; i++) {
    Particle* p = ParticlesManager::Particles[i];
    if (p->expired) continue;
    p->tick();
  }
  destroyExpiredParticles();
}

void ParticlesManager::destroyExpiredParticles() {
  if (!particlesHasChanged) return;

  auto& part = ParticlesManager::Particles;
  part.erase(std::remove_if(part.begin(), part.end(),
                            [](Particle* p) {
                              const bool hasExpired = p->expired;
                              if (hasExpired) delete p;
                              return hasExpired;
                            }),
             part.end());
  // No shrink_to_fit — capacity stays at MAX_PARTICLES, avoids realloc
  particlesHasChanged = false;
}

void ParticlesManager::destroyAllParticles() {
  Particle** pData = ParticlesManager::Particles.data();
  const size_t size = ParticlesManager::Particles.size();
  for (size_t i = 0; i < size; i++) delete pData[i];

  ParticlesManager::Particles.clear();
  blockBatchVerts.clear();
  blockBatchUVs.clear();
  blockBatchColors.clear();
  fxBatchVerts.clear();
  fxBatchUVs.clear();
  fxBatchColors.clear();

  particlesHasChanged = false;
}

Particle* ParticlesManager::GetParticleById(const u32 id) {
  Particle** pData = ParticlesManager::Particles.data();
  const size_t size = ParticlesManager::Particles.size();
  for (size_t i = 0; i < size; i++) {
    // Safety: don't return expired particles (use-after-free prevention)
    if (pData[i]->id == id && !pData[i]->expired) return pData[i];
  }
  return nullptr;
}

void ParticlesManager::EmitParticle(Particle* particle) {
  if (ParticlesManager::Particles.size() >= MAX_PARTICLES) {
    // Cap reached: discard oldest non-flame particle to make room
    // For simplicity, just delete and discard the incoming one to avoid stutter
#ifdef DEBUG_MODE
    TYRA_WARN("[Particles] Pool full (", MAX_PARTICLES, "), discarding particle type=", static_cast<u8>(particle->type));
#endif
    delete particle;
    return;
  }
  ParticlesManager::Particles.emplace_back(particle);
}

void ParticlesManager::createBlockParticleBatch(Block* block, const u16 size) {
  const size_t current = ParticlesManager::Particles.size();
  const u16 available = (current >= MAX_PARTICLES) ? 0
                      : static_cast<u16>(MAX_PARTICLES - current);
  const u16 spawnCount = std::min(size, available);

  ParticlesManager::Particles.reserve(current + spawnCount);
  for (u16 i = 0; i < spawnCount; i++) createBlockParticle(block);
}

void ParticlesManager::createBlockParticle(Block* pBlock) {
  Particle* p = new BlockParticle(pBlock);
  ParticlesManager::Particles.emplace_back(p);
}

void ParticlesManager::render() {
#ifdef DEBUG_MODE
  if (g_debug_menu.enableRenderParticles == false) return;
#endif

  const bool hasBlock = !blockBatchVerts.empty();
  const bool hasFx    = !fxBatchVerts.empty();
  if (!hasBlock && !hasFx) return;

  t_renderer->renderer3D.usePipeline(stapip);

  // --- Block particles: 1 draw call ---
  if (hasBlock) {
    blockBag.count    = static_cast<u32>(blockBatchVerts.size());
    blockBag.vertices = blockBatchVerts.data();
    blockTexBag.coordinates = blockBatchUVs.data();
    blockColorBag.many      = blockBatchColors.data();
    stapip.core.render(&blockBag);
  }

  // --- FX particles (flame + smoke): 1 draw call ---
  if (hasFx) {
    fxBag.count    = static_cast<u32>(fxBatchVerts.size());
    fxBag.vertices = fxBatchVerts.data();
    fxTexBag.coordinates = fxBatchUVs.data();
    fxColorBag.many      = fxBatchColors.data();
    stapip.core.render(&fxBag);
  }
}
