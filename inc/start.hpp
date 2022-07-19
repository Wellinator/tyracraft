#pragma once

#include <debug/debug.hpp>
#include <tamtypes.h>
#include <game.hpp>
#include <engine.hpp>
#include "renderer/3d/mesh/mesh.hpp"
#include "entities/chunck.hpp"
#include "entities/Block.hpp"
#include "camera.hpp"
#include "managers/terrain_manager.hpp"
#include "managers/state_manager.hpp"

class Start : public Game, AudioListener {
 private:
  Mesh skybox;
  Camera camera;
  TextureRepository* texRepo;
  u32 audioTicks;
  u8 skip1Beat;
  StateManager stateManager;

 public:
  Start(Engine* t_engine);
  ~Start();

  void onInit();
  void onUpdate();
  void onAudioTick();
  void onAudioFinish();

  Engine* engine;
};
