#include "states/main_menu/state_main_menu.hpp"
#include "states/loading/state_loading_game.hpp"
#include "states/loading/mini_games/state_create_maze_craft.hpp"
#include "states/loading/mini_games/state_load_maze_craft.hpp"
#include "states/loading/state_loading_saved_game.hpp"
#include "states/language_selection/language_selection_screen.hpp"
#include "file/file_utils.hpp"
#include <renderer/renderer_settings.hpp>
#include <debug/debug.hpp>
#include "loaders/3d/obj_loader/obj_loader.hpp"
#include <managers/settings_manager.hpp>

using Tyra::Audio;
using Tyra::FileUtils;
using Tyra::Math;
using Tyra::Renderer;
using Tyra::RendererSettings;

StateMainMenu::StateMainMenu(Context* t_context) : GameState(t_context) {
  init();
}

StateMainMenu::~StateMainMenu() {
  TYRA_LOG("Stopping menu song");
  context->t_engine->audio.song.stop();

  unloadTextures();
}

void StateMainMenu::init() {
  /**
   * TODO: Add menu actions sfx;
   * */

  stapip.setRenderer(&context->t_engine->renderer.core);

  const float halfWidth =
      context->t_engine->renderer.core.getSettings().getWidth() / 2;

  loadSkybox(&context->t_engine->renderer);
  context->t_camera->reset();

  // Load title
  // Title
  title[0].mode = Tyra::MODE_STRETCH;
  title[0].size.set(256, 128);
  title[0].position.set((halfWidth)-256, 64);

  title[1].mode = Tyra::MODE_STRETCH;
  title[1].size.set(256, 128);
  title[1].position.set(halfWidth, 64);

  context->t_engine->renderer.getTextureRepository()
      .add(FileUtils::fromCwd("textures/gui/menu/title_1.png"))
      ->addLink(title[0].id);
  context->t_engine->renderer.getTextureRepository()
      .add(FileUtils::fromCwd("textures/gui/menu/title_2.png"))
      ->addLink(title[1].id);

  loadMenuSong();
  setScreen(new ScreenMain(this));
}

void StateMainMenu::update(const float& deltaTime) {
  context->t_camera->setPosition(*menuSkybox->getPosition());
  context->t_camera->update();

  menuSkybox->rotation.rotateY(0.001F * deltaTime);

  // Update current screen state
  screen->update(deltaTime);
}

void StateMainMenu::render() {
  // Meshes
  context->t_engine->renderer.renderer3D.usePipeline(&stapip);
  stapip.render(menuSkybox, skyboxOptions);

  /**
   * --------------- Sprites ---------------
   * */

  // Title & Subtitle
  context->t_engine->renderer.renderer2D.render(title[0]);
  context->t_engine->renderer.renderer2D.render(title[1]);

  screen->render();
}

void StateMainMenu::loadSkybox(Renderer* renderer) {
  skyboxOptions = new StaPipOptions();
  skyboxOptions->fullClipChecks = false;
  skyboxOptions->textureMappingType =
      Tyra::PipelineTextureMappingType::TyraLinear;
  skyboxOptions->frustumCulling =
      Tyra::PipelineFrustumCulling::PipelineFrustumCulling_None;

  ObjLoaderOptions options;
  options.flipUVs = true;
  options.scale = 500.0F;

  auto data =
      ObjLoader::load(FileUtils::fromCwd("models/skybox/skybox.obj"), options);
  // data->normalsEnabled = false;
  menuSkybox = new StaticMesh(data.get());

  renderer->core.texture.repository.addByMesh(
      menuSkybox, FileUtils::fromCwd("textures/entity/skybox/menu/1/"), "png");
}

void StateMainMenu::unloadTextures() {
  context->t_engine->renderer.getTextureRepository().freeByMesh(menuSkybox);

  for (u8 i = 0; i < 2; i++)
    context->t_engine->renderer.getTextureRepository().freeBySprite(title[i]);

  delete menuSkybox;
  delete skyboxOptions;
}

void StateMainMenu::loadGame(const NewGameOptions& options) {
  context->setState(new StateLoadingGame(context, options));
  delete screen;
}

void StateMainMenu::loadSavedGame(const std::string save_file_full_path) {
  context->setState(new StateLoadingSavedGame(context, save_file_full_path));
  delete screen;
}

void StateMainMenu::createMiniGame(const NewGameOptions& options) {
  switch (options.gameMode) {
    case GameMode::Maze:
      context->setState(new StateCreateMazeCraft(context, options));
      break;

    default:
      return TYRA_ERROR("Invalis mini game creation!");
      break;
  }

  delete screen;
}

void StateMainMenu::loadSavedMiniGame(GameMode gameMode,
                                      const std::string save_file_full_path) {
  switch (gameMode) {
    case GameMode::Maze:
      context->setState(new StateLoadMazeCraft(context, save_file_full_path));
      break;

    default:
      return TYRA_ERROR("Invalid mini game loading!");
      break;
  }

  delete screen;
}

void StateMainMenu::playClickSound() {
  SoundManager* pSoundManager = SoundManager::getInstance();

  context->t_engine->audio.adpcm.setVolume(50, MENU_SFX_CH);
  pSoundManager->playSfx(SoundFxCategory::Random, SoundFX::WoodClick,
                         MENU_SFX_CH);
}

void StateMainMenu::loadMenuSong() {
  const std::string randSong =
      SoundManager::GetRandonSongFromPath(FileUtils::fromCwd("sounds/menu/"));
  if (randSong.size() > 0) {
    context->t_engine->audio.song.load(randSong.c_str());
    context->t_engine->audio.song.inLoop = true;
    context->t_engine->audio.song.setVolume(65);
    context->t_engine->audio.song.play();
  }
}

void StateMainMenu::setScreen(ScreenBase* screen) {
  if (this->screen) delete this->screen;
  this->screen = screen;
}

void StateMainMenu::goToLanguageSelectioScreen() {
  if (screen) delete screen;
  context->setState(new StateLanguageSelectionScreen(context));
}
