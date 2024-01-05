#include "managers/mob/mob_manager.hpp"
#include "entities/mob/pig/pig.hpp"
#include "managers/tick_manager.hpp"
#include "managers/collision_manager.hpp"

using Tyra::MeshBuilderData;
using Tyra::ObjLoader;
using Tyra::ObjLoaderOptions;

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

  delete pigBaseMesh;
  delete pigMeshBuilderData;

  mobs.clear();
  mobs.shrink_to_fit();
};

void MobManager::init(Renderer* renderer, SoundManager* t_soundManager,
                      WorldLightModel* t_worldLightModel, LevelMap* t_terrain,
                      ChunckManager* t_chunkManager) {
  this->t_renderer = renderer;
  this->t_soundManager = t_soundManager;
  this->t_chunkManager = t_chunkManager;
  this->t_worldLightModel = t_worldLightModel;
  this->t_terrain = t_terrain;

  _loadPigTexture();
  _loadPigMesh();

  dynpip.setRenderer(&this->t_renderer->core);
}

void MobManager::update(const float& deltaTime) {
  for (size_t i = 0; i < mobs.size(); i++) {
    if (mobs[i]->shouldUnspawn) {
      _mobsHasChanged = true;
      continue;
    }

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

  if (isTicksCounterAt(15) && _mobsHasChanged) {
    _destroyUnspownedMobs();
  }
}

void MobManager::render() {
  t_renderer->renderer3D.usePipeline(&dynpip);
  for (size_t i = 0; i < mobs.size(); i++) {
    if (mobs[i]) dynpip.render(mobs[i]->mesh, &dynpipOptions);
  }
}

void MobManager::_loadPigTexture() {
  pigTexture = t_renderer->getTextureRepository().add(
      FileUtils::fromCwd("textures/entity/pig/pig.png"));
}

void MobManager::_loadPigMesh() {
  ObjLoaderOptions options;
  options.scale = 17.0F;
  options.flipUVs = true;
  options.animation.count = 3;

  pigMeshBuilderData =
      ObjLoader::load(FileUtils::fromCwd("models/pig/pig.obj"), options)
          .release();
  pigMeshBuilderData->loadNormals = false;

  pigBaseMesh = new DynamicMesh(pigMeshBuilderData);
}

Vec4 MobManager::_getMobMoviementDirection(Mob* mob) {
  return Vec4(Tyra::Math::randomi(-360, 360), 0, Tyra::Math::randomi(-360, 360))
      .getNormalized();
}

Mob* MobManager::spawnMob(const MobType type) {
  if (mobs.size() >= MAX_MOBS_LIMIT) {
    TYRA_WARN("Reached MOB LIMIT");
    return nullptr;
  }

  switch (type) {
    case MobType::Pig:
      return _createPig();

    default:
      TYRA_ERROR("Invalid MobType!");
      return nullptr;
  }
}

Mob* MobManager::spawnMobAtPosition(const MobType type, const Vec4& position) {
  if (mobs.size() >= MAX_MOBS_LIMIT) {
    TYRA_WARN("Reached MOB LIMIT");
    return nullptr;
  }

  switch (type) {
    case MobType::Pig:
      return _createPigAtPosition(position);

    default:
      TYRA_ERROR("Invalid MobType!");
      return nullptr;
  }
}

Mob* MobManager::_createPig() {
  Pig* mob = new Pig(t_renderer, t_soundManager, t_chunkManager, pigTexture,
                     pigBaseMesh);
  mobs.push_back(mob);
  return mob;
}

Mob* MobManager::_createPigAtPosition(const Vec4& position) {
  Mob* mob = _createPig();
  mob->spawnPosition.set(position);
  mob->setPosition(position);
  return mob;
}

void MobManager::unspawnMob(const uint32_t id) {
  for (size_t i = 0; i < mobs.size(); i++)
    if (mobs[i]->id == id) {
      delete mobs[i];
      break;
    }
}

void MobManager::_destroyUnspownedMobs() {
  mobs.erase(std::remove_if(mobs.begin(), mobs.end(),
                            [](Mob* m) {
                              const auto _shouldUnspawn = m->shouldUnspawn;
                              if (_shouldUnspawn) delete m;

                              return _shouldUnspawn;
                            }),
             mobs.end());

  mobs.shrink_to_fit();
  _mobsHasChanged = false;
}
