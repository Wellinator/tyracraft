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
  this->init();
}

StateMainMenu::~StateMainMenu() {
  TYRA_LOG("Stopping menu song");
  context->t_engine->audio.song.stop();

  this->unloadTextures();
}

void StateMainMenu::init() {
  /**
   * TODO: Add menu actions sfx;
   * */

  stapip.setRenderer(&this->context->t_engine->renderer.core);

  const float halfWidth =
      this->context->t_engine->renderer.core.getSettings().getWidth() / 2;

  this->loadSkybox(&this->context->t_engine->renderer);
  this->context->t_camera->reset();

  // Load title
  // Title
  title[0].mode = Tyra::MODE_STRETCH;
  title[0].size.set(256, 128);
  title[0].position.set((halfWidth)-256, 64);

  title[1].mode = Tyra::MODE_STRETCH;
  title[1].size.set(256, 128);
  title[1].position.set(halfWidth, 64);

  this->context->t_engine->renderer.getTextureRepository()
      .add(FileUtils::fromCwd("textures/gui/menu/title_1.png"))
      ->addLink(title[0].id);
  this->context->t_engine->renderer.getTextureRepository()
      .add(FileUtils::fromCwd("textures/gui/menu/title_2.png"))
      ->addLink(title[1].id);

  this->loadMenuSong();
  this->setScreen(new ScreenMain(this));
}

void StateMainMenu::update(const float& deltaTime) {
  this->context->t_camera->setPosition(*menuSkybox->getPosition());
  this->context->t_camera->update();

  this->menuSkybox->rotation.rotateY(0.0001F);

  // Update current screen state
  this->screen->update(deltaTime);
}

void StateMainMenu::render() {
  // Meshes
  this->context->t_engine->renderer.renderer3D.usePipeline(&stapip);
  stapip.render(this->menuSkybox, skyboxOptions);

  /**
   * --------------- Sprites ---------------
   * */

  // Title & Subtitle
  this->context->t_engine->renderer.renderer2D.render(title[0]);
  this->context->t_engine->renderer.renderer2D.render(title[1]);

  this->screen->render();
}

void StateMainMenu::loadSkybox(Renderer* renderer) {
  this->skyboxOptions = new StaPipOptions();
  this->skyboxOptions->fullClipChecks = false;
  this->skyboxOptions->textureMappingType =
      Tyra::PipelineTextureMappingType::TyraNearest;
  this->skyboxOptions->frustumCulling =
      Tyra::PipelineFrustumCulling::PipelineFrustumCulling_None;

  ObjLoaderOptions options;
  options.flipUVs = true;
  options.scale = 500.0F;

  auto data =
      ObjLoader::load(FileUtils::fromCwd("models/skybox/skybox.obj"), options);
  // data->normalsEnabled = false;
  this->menuSkybox = new StaticMesh(data.get());

  renderer->core.texture.repository.addByMesh(
      this->menuSkybox, FileUtils::fromCwd("textures/entity/skybox/menu/1/"),
      "png");
}

void StateMainMenu::unloadTextures() {
  this->context->t_engine->renderer.getTextureRepository().freeByMesh(
      menuSkybox);

  for (u8 i = 0; i < 2; i++)
    this->context->t_engine->renderer.getTextureRepository().freeBySprite(
        title[i]);

  delete this->menuSkybox;
  delete this->skyboxOptions;
}

void StateMainMenu::loadGame(const NewGameOptions& options) {
  this->context->setState(new StateLoadingGame(this->context, options));
  delete this->screen;
}

void StateMainMenu::loadSavedGame(const std::string save_file_full_path) {
  this->context->setState(
      new StateLoadingSavedGame(this->context, save_file_full_path));
  delete this->screen;
}

void StateMainMenu::createMiniGame(const NewGameOptions& options) {
  switch (options.gameMode) {
    case GameMode::Maze:
      this->context->setState(new StateCreateMazeCraft(this->context, options));
      break;

    default:
      return TYRA_ERROR("Invalis mini game creation!");
      break;
  }

  delete this->screen;
}

void StateMainMenu::loadSavedMiniGame(const std::string save_file_full_path) {
  this->context->setState(
      new StateLoadMazeCraft(this->context, save_file_full_path));
  delete this->screen;
}

void StateMainMenu::playClickSound() {
  this->context->t_engine->audio.adpcm.setVolume(50, MENU_SFX_CH);
  this->context->t_soundManager->playSfx(SoundFxCategory::Random,
                                         SoundFX::WoodClick, MENU_SFX_CH);
  Tyra::Threading::switchThread();
}

void StateMainMenu::loadMenuSong() {
  const std::string randSong =
      SoundManager::GetRandonSongFromPath(FileUtils::fromCwd("sounds/menu/"));
  if (randSong.size() > 0) {
    this->context->t_engine->audio.song.load(randSong.c_str());
    this->context->t_engine->audio.song.inLoop = true;
    this->context->t_engine->audio.song.setVolume(65);
    this->context->t_engine->audio.song.play();
  }
}

void StateMainMenu::setScreen(ScreenBase* screen) {
  if (this->screen) delete this->screen;
  this->screen = screen;
}

void StateMainMenu::goToLanguageSelectioScreen() {
  if (this->screen) delete this->screen;
  context->setState(new StateLanguageSelectionScreen(context));
}
