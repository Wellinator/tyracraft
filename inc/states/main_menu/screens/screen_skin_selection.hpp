#pragma once
#include "states/main_menu/screens/screen_base.hpp"
#include "models/skin_info_model.hpp"
#include "managers/font/font_options.hpp"
#include "managers/language_manager.hpp"
#include "managers/settings_manager.hpp"
#include <tamtypes.h>
#include <tyra>
#include <string>
#include <vector>
#include <fstream>
#include <utils.hpp>

using Tyra::Color;
using Tyra::DynamicMesh;
using Tyra::DynamicPipeline;
using Tyra::DynPipOptions;
using Tyra::FileUtils;
using Tyra::Math;
using Tyra::ObjLoader;
using Tyra::Renderer;
using Tyra::Sprite;
using Tyra::Texture;
using Tyra::TextureRepository;
using Tyra::Vec4;

class StateMainMenu;

class ScreenSkinSelection : public ScreenBase {
 public:
  ScreenSkinSelection(StateMainMenu* t_context);
  ~ScreenSkinSelection();

  void init();
  void update(const float& deltaTime);
  void render();

 private:
  Renderer* t_renderer;

  Sprite btnTriangle;
  Sprite btnCross;
  Sprite arrowLeft;
  Sprite arrowRight;

  u8 isMoving = false;
  u8 isMovingForward = false;
  float interpolation = 0;

  const std::array<float, 3> defaultRotation = {_90DEGINRAD + 0.2f, _90DEGINRAD,
                                                _90DEGINRAD - 0.2f};
  std::array<float, 3> startRotation = defaultRotation;
  std::array<float, 3> tempRotation = defaultRotation;
  std::array<float, 3> endRotation = defaultRotation;

  const std::array<float, 3> defaultScale = {1.0f, 1.3f, 1.0f};
  std::array<float, 3> startScale = defaultScale;
  std::array<float, 3> tempScale = defaultScale;
  std::array<float, 3> endScale = defaultScale;

  const std::array<Vec4, 3> defaultPositions = {Vec4(25.0f, 16.5F, -12.0f),
                                                Vec4(25.0f, 16.5F, 0.0f),
                                                Vec4(25.0f, 16.5F, 12.0f)};
  std::array<Vec4, 3> startPositions = defaultPositions;
  std::array<Vec4, 3> endPositions = defaultPositions;
  std::array<Vec4, 3> tempPositions = defaultPositions;

  const float slotWidth = 246.0F;
  const float slotHeight = 32.0F;

  const std::string Label_Select =
      g_language_repository["gui"]["select"].get<std::string>();
  const std::string Label_Prev =
      g_language_repository["gui"]["prev"].get<std::string>();
  const std::string Label_Next =
      g_language_repository["gui"]["next"].get<std::string>();
  const std::string Label_Back =
      g_language_repository["gui"]["back"].get<std::string>();

  std::vector<SkinInfoModel> skins;
  SkinInfoModel selectedSkin;

  std::array<std::unique_ptr<DynamicMesh>, 3> models;
  std::array<Texture*, 3> textures;

  std::unique_ptr<DynamicMesh> baseMesh;
  DynPipOptions dynpipOptions;
  DynamicPipeline dynpip;
  std::vector<u32> standStillSequence = {0, 1};

  const float TRANSITION_SPEED = 6.0f;

  void handleInput();
  void backToMainMenu();
  void saveSkin();
  void selectPreviousSkin();
  void selectNextSkin();
  void getAvailableSkins();
  void loadSkinTextures();
  void unloadSkinTextures();
  void loadModels();
  void unloadModels();
  void calcLerp(const float& deltaTime);
  void startMoving(u8 _movingForward);
  void moveForward(const float& deltaTime);
  void moveBackward(const float& deltaTime);
  void stopMoving();
};
