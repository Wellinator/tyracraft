#include "states/main_menu/state_main_menu.hpp"
#include "states/loading/state_loading_game.hpp"
#include "file/file_utils.hpp"
#include <renderer/renderer_settings.hpp>
#include <debug/debug.hpp>
#include "loaders/3d/obj_loader/obj_loader.hpp"

using Tyra::Audio;
using Tyra::FileUtils;
using Tyra::Math;
using Tyra::ObjLoader;
using Tyra::ObjLoaderOptions;
using Tyra::Renderer;
using Tyra::RendererSettings;

StateMainMenu::StateMainMenu(Context* t_context) : GameState(t_context) {
  fontManager.init(&t_context->t_engine->renderer);
  this->init();
}

StateMainMenu::~StateMainMenu() { this->unloadTextures(); }

void StateMainMenu::init() {
  /**
   * TODO: Add menu actions sfx;
   * */

  this->stapip.setRenderer(&this->context->t_engine->renderer.core);

  const float halfWidth =
      this->context->t_engine->renderer.core.getSettings().getWidth() / 2;

  this->loadSkybox(&this->context->t_engine->renderer);

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
  // Switch to audio thread
  Tyra::Threading::switchThread();

  // Update skybox and camera;
  {
    this->context->t_camera->update(this->context->t_engine->pad,
                                    *this->menuSkybox);
    this->menuSkybox->rotation.rotateY(0.0001F);
  }

  // Update current screen state
  this->screen->update();

  // Switch to audio thread
  Tyra::Threading::switchThread();
}

void StateMainMenu::render() {
  // Meshes
  this->context->t_engine->renderer.renderer3D.usePipeline(&stapip);
  { stapip.render(this->menuSkybox, skyboxOptions); }

  /**
   * --------------- Sprites ---------------
   * */

  // Title & Subtitle
  this->context->t_engine->renderer.renderer2D.render(&title[0]);
  this->context->t_engine->renderer.renderer2D.render(&title[1]);

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
  options.scale = 300.0F;

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

  for (u8 i = 0; i < 2; i++) {
    this->context->t_engine->renderer.getTextureRepository().free(
        this->context->t_engine->renderer.getTextureRepository()
            .getBySpriteId(title[i].id)
            ->id);
  }

  delete this->menuSkybox;
  delete this->skyboxOptions;
}

void StateMainMenu::loadGame(const NewGameOptions& options) {
  this->context->setState(new StateLoadingGame(this->context, options));
  delete this->screen;
}

void StateMainMenu::playClickSound() {
  this->context->t_engine->audio.adpcm.setVolume(50, MENU_SFX_CH);
  this->context->t_soundManager->playSfx(SoundFxCategory::Random,
                                         SoundFX::Click, MENU_SFX_CH);
  Tyra::Threading::switchThread();
}

void StateMainMenu::loadMenuSong() {
  // Load song
  this->context->t_engine->audio.song.load(
      FileUtils::fromCwd(this->getRandonMenuSongName()));
  this->context->t_engine->audio.song.inLoop = true;
  this->context->t_engine->audio.song.setVolume(80);
  this->context->t_engine->audio.song.play();
}

const std::string StateMainMenu::getRandonMenuSongName() {
  const u8 MAX_MENU_SONGS = 5;
  std::string result = "sounds/menu/menu";
  std::string songExtension = ".wav";

  int randSongId = Math::randomi(1, MAX_MENU_SONGS);
  result.append(std::to_string(randSongId));
  result.append(songExtension);
  return result;
}

void StateMainMenu::setScreen(ScreenBase* screen) {
  if (this->screen) delete this->screen;
  this->screen = screen;
}
