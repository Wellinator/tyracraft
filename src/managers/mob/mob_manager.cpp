#include "managers/mob/mob_manager.hpp"
#include "entities/mob/pig/pig.hpp"
#include "entities/mob/cow/cow.hpp"
#include "managers/tick_manager.hpp"
#include "managers/collision_manager.hpp"
#include "debug.hpp"

using Tyra::MeshBuilderData;
using Tyra::ObjLoader;
using Tyra::ObjLoaderOptions;

MobManager::MobManager() {
  tickHandles = new TickTaskHandles();
};

MobManager::~MobManager() {
  delete tickHandles;
  tickHandles = nullptr;
  for (size_t i = 0; i < mobs.size(); i++) {
    delete mobs[i];
    mobs[i] = nullptr;
  }

  // delete pigBaseMesh;
  // delete pigMeshBuilderData;

  t_renderer->getTextureRepository().free(pigTexture);
  t_renderer->getTextureRepository().free(cowTexture);

  mobs.clear();
  mobs.shrink_to_fit();
};

void MobManager::init(Renderer* renderer, WorldLightModel* t_worldLightModel,
                      Level* level, ChunkManager* t_chunkManager) {
  this->t_renderer = renderer;
  this->t_chunkManager = t_chunkManager;
  this->t_worldLightModel = t_worldLightModel;
  pLevel = level;

  _loadMobTextures();
  _loadMobFrames();

  // Initialize shared rendering pipeline once
  sharedStatPip.setRenderer(&t_renderer->core);
  pipelineInitialized = true;
}

void MobManager::fixedUpdate(const float& fixedDeltaTime) {
  for (size_t i = 0; i < mobs.size(); i++) {
    if (mobs[i]->shouldUnspawn) continue;
    mobs[i]->fixedUpdate(fixedDeltaTime);
  }
}

void MobManager::update(const float& deltaTime) {
  for (size_t i = 0; i < mobs.size(); i++) {
    if (mobs[i]->shouldUnspawn) {
      _mobsHasChanged = true;
      continue;
    }

    mobs[i]->update(deltaTime);
  }
}

void MobManager::tick() {
  for (size_t i = 0; i < mobs.size(); i++) mobs[i]->tick();
}

void MobManager::registerTickCallbacks(TickScheduler& scheduler) {
  tickHandles->add(scheduler.everyHandle(5, [this]() {
    for (size_t i = 0; i < mobs.size(); i++)
      mobs[i]->updateStateInWater();
  }));

  tickHandles->add(scheduler.everyHandle(15, [this]() {
    if (_mobsHasChanged) _destroyUnspownedMobs();
  }));
}

void MobManager::render() {
  for (size_t i = 0; i < mobs.size(); i++) {
    if (mobs[i]->shouldUnspawn) continue;

#ifdef DEBUG_MODE
    if (g_debug_menu.enableRenderMobs == false) continue;
#endif  // DEBUG_MODE

    mobs[i]->render();

#ifdef DEBUG_MODE
    if (g_debug_menu.showCollisionBoxes) {
      t_renderer->renderer3D.utility.drawBBox(mobs[i]->getHitBox(),
                                              Color(100, 50, 50));
    }

    if (g_debug_menu.showMobPathfinding) {
      auto currentBBox = mobs[i]->getHitBox();
      t_renderer->renderer3D.utility.drawBBox(currentBBox, Color(200, 20, 200));

      if (mobs[i]->currentPath != nullptr) {
        auto& waypoints = mobs[i]->currentPath->waypoints;

        // Start Red
        const Vec4 start = waypoints[0];
        t_renderer->renderer3D.utility.drawBox(start, 0.5f, Color(200, 0, 0));

        //  Cursor Blue
        for (size_t j = 1; j < waypoints.size() - 1; j++) {
          const Vec4 p = waypoints[j];
          t_renderer->renderer3D.utility.drawBox(p, 0.5f, Color(50, 50, 200));
        }

        // Goal Green
        const Vec4 goal = waypoints[waypoints.size() - 1];
        t_renderer->renderer3D.utility.drawBox(goal, 0.5f, Color(0, 200, 0));
      }

      // Current Yellow
      t_renderer->renderer3D.utility.drawBox(mobs[i]->position, 0.5f,
                                             Color(200, 200, 50));
    }
#endif
  }
}

void MobManager::_loadMobTextures() {
  pigTexture = t_renderer->getTextureRepository().add(
      FileUtils::fromCwd("textures/entity/pig/pig.png"));
  cowTexture = t_renderer->getTextureRepository().add(
      FileUtils::fromCwd("textures/entity/cow/cow.png"));
}

void MobManager::_loadMobFrames() {
  ObjLoaderOptions options;
  options.scale = 16.5F;
  options.flipUVs = true;
  options.animation.count = 1;

  for (size_t i = 0; i < pigFrames.size(); i++) {
    std::unique_ptr<MeshBuilderData> tempFrameData =
        ObjLoader::load(FileUtils::fromCwd("models/pig/pig_frame_" +
                                           std::to_string(i + 1) + ".obj"),
                        options);
    tempFrameData->loadNormals = false;
    tempFrameData->loadLightmap = false;
    pigFrames[i] = std::make_unique<Tyra::Mesh>(tempFrameData.get());
  }

  for (size_t i = 0; i < cowFrames.size(); i++) {
    std::unique_ptr<MeshBuilderData> tempFrameData =
        ObjLoader::load(FileUtils::fromCwd("models/cow/cow_frame_" +
                                           std::to_string(i + 1) + ".obj"),
                        options);
    tempFrameData->loadNormals = false;
    tempFrameData->loadLightmap = false;
    cowFrames[i] = std::make_unique<Tyra::Mesh>(tempFrameData.get());
  }
}

Vec4 MobManager::_getMobMoviementDirection(Mob* mob) {
  return Vec4(Tyra::Math::randomi(-360, 360), 0, Tyra::Math::randomi(-360, 360))
      .getNormalized();
}

Mob* MobManager::spawnMob(const MobType type) {
  if (mobs.size() >= GLOBAL_MOB_CAP) {
    return nullptr;
  }

  switch (type) {
    case MobType::Pig:
    case MobType::Cow:
      return _createMob(type);

    default:
      TYRA_ERROR("Invalid MobType!");
      return nullptr;
  }
}
Mob* MobManager::trySpawningMobAtPosition(const MobCategory category,
                                          const MobType type,
                                          const Vec4& position) {
  bool hasReachedGlobalCap = mobs.size() >= GLOBAL_MOB_CAP;
  bool hasReachedCategoryCap = false;
  int categoryCap = getMobCapByCategory(category);
  if (categoryCap > 0) {
    int currentCategoryCount = MOB_CAPS[static_cast<int>(category)];
    hasReachedCategoryCap = currentCategoryCount >= categoryCap;
  } else {
    // If category cap is 0 or less, we consider it reached
    hasReachedCategoryCap = true;
  }

  if (hasReachedGlobalCap || hasReachedCategoryCap) {
    return nullptr;
  }

  Mob* mob = nullptr;

  switch (type) {
    case MobType::Pig:
    case MobType::Cow:
      mob = _createMobAtPosition(type, position);
      break;
    default:
      TYRA_TRAP("Invalid MobType!");
      return nullptr;
  }

  if (mob) {
    MOB_CAPS[static_cast<int>(category)]++;
  }
  return mob;
}

Mob* MobManager::spawnMobAtPosition(const MobType type, const Vec4& position) {
  switch (type) {
    case MobType::Pig:
    case MobType::Cow:
      return _createMobAtPosition(type, position);

    default:
      TYRA_ERROR("Invalid MobType!");
      return nullptr;
  }
}

Mob* MobManager::_createMob(const MobType type) {
  Mob* mob = nullptr;
  std::vector<Tyra::Mesh*> rawFrames = {};
  rawFrames.reserve(5);  // Reserve space for up to 5 frames

  switch (type) {
    case MobType::Pig:
      // Convert smart pointers to raw pointers for Animated constructor
      for (size_t i = 0; i < pigFrames.size(); i++)
        rawFrames.emplace_back(pigFrames[i].get());

      mob = new Pig(pLevel, t_renderer, pigTexture, rawFrames.data(),
                    rawFrames.size(), &sharedStatPip);
      break;
    case MobType::Cow:
      // Convert smart pointers to raw pointers for Animated constructor
      for (size_t i = 0; i < cowFrames.size(); i++)
        rawFrames.emplace_back(cowFrames[i].get());
      mob = new Cow(pLevel, t_renderer, cowTexture, rawFrames.data(),
                    rawFrames.size(), &sharedStatPip);
      break;

    default:
      TYRA_TRAP("Invalid MobType!");
      return nullptr;
  }

  mobs.emplace_back(mob);
  return mob;
}

Mob* MobManager::_createMobAtPosition(const MobType type,
                                      const Vec4& position) {
  Mob* mob = _createMob(type);
  mob->spawnPosition.set(position);
  mob->setPosition(position);
  return mob;
}

void MobManager::unspawnMob(const uint32_t id) {
  for (size_t i = 0; i < mobs.size(); i++) {
    if (mobs[i]->id == id) {
      MobCategory cat = mobs[i]->getCategory();
      if (cat != MobCategory::Invalid) {
        int idx = static_cast<int>(cat);
        if (MOB_CAPS[idx] > 0) MOB_CAPS[idx]--;
      }
      delete mobs[i];
      mobs.erase(mobs.begin() + i);
      break;
    }
  }
}

const int MobManager::getMobCountByCategory(const MobCategory category) {
  if (category == MobCategory::Invalid) {
    return -1;
  }

  return MOB_CAPS[static_cast<int>(category)];
}

const int MobManager::getMobCapByCategory(const MobCategory category) {
  switch (category) {
    case MobCategory::Hostile:
      return 30;
    case MobCategory::Passive:
      return 10;
    default:
      return 0;
  }
}

void MobManager::_destroyUnspownedMobs() {
  mobs.erase(std::remove_if(mobs.begin(), mobs.end(),
                            [this](Mob* m) {
                              const auto _shouldUnspawn = m->shouldUnspawn;
                              if (_shouldUnspawn) {
                                MobCategory cat = m->getCategory();
                                if (cat != MobCategory::Invalid) {
                                  int idx = static_cast<int>(cat);
                                  if (MOB_CAPS[idx] > 0) MOB_CAPS[idx]--;
                                }
                                delete m;
                              }

                              return _shouldUnspawn;
                            }),
             mobs.end());

  mobs.shrink_to_fit();
  _mobsHasChanged = false;
}
