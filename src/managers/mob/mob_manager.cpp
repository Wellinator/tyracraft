#include "managers/mob/mob_manager.hpp"
#include "entities/mob/pig/pig.hpp"

MobManager::MobManager() {
  dynpipOptions.antiAliasingEnabled = false;
  dynpipOptions.frustumCulling =
      Tyra::PipelineFrustumCulling::PipelineFrustumCulling_None;
  dynpipOptions.shadingType = Tyra::PipelineShadingType::TyraShadingFlat;
  dynpipOptions.textureMappingType =
      Tyra::PipelineTextureMappingType::TyraNearest;
};

MobManager::~MobManager() {
  t_renderer->getTextureRepository().free(pigTexture);

  for (size_t i = 0; i < mobs.size(); i++) {
    delete mobs[i];
    mobs[i] = nullptr;
  }

  mobs.clear();
  mobs.shrink_to_fit();
};

void MobManager::init(Renderer* renderer, SoundManager* t_soundManager,
                      WorldLightModel* t_worldLightModel, LevelMap* t_terrain) {
  this->t_renderer = renderer;
  this->t_soundManager = t_soundManager;
  this->t_worldLightModel = t_worldLightModel;
  this->t_terrain = t_terrain;

  _loadPigTexture();

  dynpip.setRenderer(&this->t_renderer->core);
}

void MobManager::update(const float& deltaTime) {
  for (size_t i = 0; i < mobs.size(); i++) {
    const auto isTimeToChangeDir = changeDirectionTimer > changeDirectionLimit;
    if (isTimeToChangeDir) {
      const Vec4 start = mobs[i]->moviemntDirection;
      const Vec4 end = _getMobMoviementDirection(mobs[i]);

      Vec4::setLerp(&mobs[i]->moviemntDirection, start, end, 0.1f);
      mobs[i]->moviemntDirection.normalize();

      changeDirectionTimer = 0;
      changeDirectionLimit = Tyra::Math::randomf(2, 3);
    } else {
      changeDirectionTimer += deltaTime;
    }

    mobs[i]->update(deltaTime, mobs[i]->moviemntDirection, t_terrain);
  }
}

void MobManager::render() {
  t_renderer->renderer3D.usePipeline(&dynpip);
  for (size_t i = 0; i < mobs.size(); i++) {
    dynpip.render(mobs[i]->mesh.get(), &dynpipOptions);
  }
}

void MobManager::_loadPigTexture() {
  pigTexture = t_renderer->getTextureRepository().add(
      FileUtils::fromCwd("textures/entity/pig/pig.png"));
}

Vec4 MobManager::_getMobMoviementDirection(Mob* mob) {
  return Vec4(Tyra::Math::randomi(-360, 360), 0, Tyra::Math::randomi(-360, 360))
      .getNormalized();
}

Mob* MobManager::spawnMob(const MobType type) {
  switch (type) {
    case MobType::Pig:
      return _createPig();

    default:
      TYRA_ERROR("Invalid MobType!");
      return nullptr;
  }
}

Mob* MobManager::spawnMobAtPosition(const MobType type, const Vec4& position) {
  switch (type) {
    case MobType::Pig:
      return _createPigAtPosition(position);

    default:
      TYRA_ERROR("Invalid MobType!");
      return nullptr;
  }
}

Mob* MobManager::_createPig() {
  Mob* mob = new Pig(t_renderer, t_soundManager, pigTexture);
  mobs.emplace_back(mob);
  return mob;
}

Mob* MobManager::_createPigAtPosition(const Vec4& position) {
  Mob* mob = _createPig();
  mob->spawnPosition.set(position);
  mob->position.set(mob->spawnPosition);
  return mob;
}

void MobManager::unspawnMob(const uint32_t id) {
  for (size_t i = 0; i < mobs.size(); i++)
    if (mobs[i]->id == id) {
      mobs.erase(mobs.begin() + i);
      mobs.shrink_to_fit();
      return;
    }
}