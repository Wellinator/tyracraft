#include "states/main_menu/state_main_menu.hpp"
#include "states/loading/state_loading_game.hpp"
#include "file/file_utils.hpp"
#include <renderer/renderer_settings.hpp>
#include <debug/debug.hpp>
#include "loaders/3d/obj_loader/obj_loader.hpp"

using Tyra::Audio;
using Tyra::FileUtils;
using Tyra::ObjLoader;
using Tyra::ObjLoaderOptions;
using Tyra::Renderer;
using Tyra::RendererSettings;

StateMainMenu::StateMainMenu(Context* t_context) : GameState(t_context) {
  this->init();
}

StateMainMenu::~StateMainMenu() {
  this->context->t_audio->song.stop();
  this->context->t_audio->song.inLoop = false;
  this->unloadTextures();
}

void StateMainMenu::init() {
  /**
   * TODO: ->
   * - Load menu option
   *      - Play Game (DONE!)
   *      - How to play
   *      - About
   * */

  this->stapip.setRenderer(&this->context->t_renderer->core);

  const float halfWidth =
      this->context->t_renderer->core.getSettings().getWidth() / 2;
  const float halfHeight =
      this->context->t_renderer->core.getSettings().getHeight() / 2;

  this->loadSkybox(this->context->t_renderer);

  // Load title
  // Title
  title[0].mode = Tyra::MODE_STRETCH;
  title[0].size.set(256, 128);
  title[0].position.set((halfWidth)-256, 64);

  title[1].mode = Tyra::MODE_STRETCH;
  title[1].size.set(256, 128);
  title[1].position.set(halfWidth, 64);

  this->context->t_renderer->core.texture.repository
      .add(FileUtils::fromCwd("assets/menu/title_1.png"))
      ->addLink(title[0].id);
  this->context->t_renderer->core.texture.repository
      .add(FileUtils::fromCwd("assets/menu/title_2.png"))
      ->addLink(title[1].id);

  // Buttons
  btnCross.mode = Tyra::MODE_STRETCH;
  btnCross.size.set(25, 25);
  btnCross.position.set(
      40, this->context->t_renderer->core.getSettings().getHeight() - 52);

  this->context->t_renderer->core.texture.repository
      .add(FileUtils::fromCwd("assets/textures/ui/btn_cross.png"))
      ->addLink(btnCross.id);

  // Load slots
  slot[0].mode = Tyra::MODE_STRETCH;
  slot[0].size.set(SLOT_WIDTH, 25);
  slot[0].position.set(halfWidth - SLOT_WIDTH / 2, 265);
  slot[1].mode = Tyra::MODE_STRETCH;
  slot[1].size.set(SLOT_WIDTH, 25);
  slot[1].position.set(halfWidth - SLOT_WIDTH / 2, 265 + 30);
  slot[2].mode = Tyra::MODE_STRETCH;
  slot[2].size.set(SLOT_WIDTH, 25);
  slot[2].position.set(halfWidth - SLOT_WIDTH / 2, 265 + 60);

  this->context->t_renderer->core.texture.repository
      .add(FileUtils::fromCwd("assets/menu/slot.png"))
      ->addLink(slot[0].id);
  this->context->t_renderer->core.texture.repository
      .add(FileUtils::fromCwd("assets/menu/hovered_slot.png"))
      ->addLink(slot[1].id);
  this->context->t_renderer->core.texture.repository
      .add(FileUtils::fromCwd("assets/menu/selected_slot.png"))
      ->addLink(slot[2].id);

  // Texts
  textPlayGame.mode = Tyra::MODE_STRETCH;
  textPlayGame.size.set(80, 15);
  textPlayGame.position.set(halfWidth - 40, 265 + 35);

  if (this->activeOption == MainMenuOptions::PlayGame) {
    textPlayGame.color.r = 255;
    textPlayGame.color.g = 255;
    textPlayGame.color.b = 0;
  }

  this->context->t_renderer->core.texture.repository
      .add(FileUtils::fromCwd("assets/menu/play_game.png"))
      ->addLink(textPlayGame.id);

  textSelect.mode = Tyra::MODE_STRETCH;
  textSelect.size.set(64, 16);
  textSelect.position.set(
      30 + 40, this->context->t_renderer->core.getSettings().getHeight() - 47);
  this->context->t_renderer->core.texture.repository
      .add(FileUtils::fromCwd("assets/menu/select.png"))
      ->addLink(textSelect.id);

  // Load song
  this->context->t_audio->song.load(FileUtils::fromCwd("sounds/menu.wav"));
  this->context->t_audio->song.play();
  this->context->t_audio->song.inLoop = true;
  this->context->t_audio->song.setVolume(100);
}

void StateMainMenu::update(const float& deltaTime) {
  this->context->t_camera->update(*this->context->t_pad, *this->menuSkybox);
  this->handleInput();
  if (this->shouldInitGame()) return this->loadGame();
}

void StateMainMenu::render() {
  // Meshes
  this->menuSkybox->rotation.rotateY(0.001F);
  this->context->t_renderer->renderer3D.usePipeline(&stapip);
  { stapip.render(this->menuSkybox, skyboxOptions); }

  /**
   * --------------- Sprites ---------------
   * */

  //   // Title & Subtitle
  this->context->t_renderer->renderer2D.render(&title[0]);
  this->context->t_renderer->renderer2D.render(&title[1]);

  //   // Slots
  this->context->t_renderer->renderer2D.render(&slot[0]);
  this->context->t_renderer->renderer2D.render(&slot[1]);
  this->context->t_renderer->renderer2D.render(&slot[2]);

  //   // Texts
  this->context->t_renderer->renderer2D.render(&textPlayGame);
  this->context->t_renderer->renderer2D.render(&textSelect);

  //   // Buttons
  this->context->t_renderer->renderer2D.render(&btnCross);
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
  options.scale = 250.0F;

  auto data =
      ObjLoader::load(FileUtils::fromCwd("assets/menu/skybox.obj"), options);
  // data->normalsEnabled = false;
  this->menuSkybox = new StaticMesh(data.get());

  renderer->core.texture.repository.addByMesh(
      this->menuSkybox, FileUtils::fromCwd("assets/menu/"), "png");
}

void StateMainMenu::unloadTextures() {
  for (u8 i = 0; i < menuSkybox->materials.size(); i++) {
    this->context->t_renderer->core.texture.repository.free(
        this->context->t_renderer->core.texture.repository
            .getBySpriteId(menuSkybox->materials[i]->id)
            ->id);
  }
  for (u8 i = 0; i < 2; i++) {
    this->context->t_renderer->core.texture.repository.free(
        this->context->t_renderer->core.texture.repository
            .getBySpriteId(title[i].id)
            ->id);
  }
  for (u8 i = 0; i < 3; i++) {
    this->context->t_renderer->core.texture.repository.free(
        this->context->t_renderer->core.texture.repository
            .getBySpriteId(slot[i].id)
            ->id);
  }
  this->context->t_renderer->core.texture.repository.free(
      this->context->t_renderer->core.texture.repository
          .getBySpriteId(textPlayGame.id)
          ->id);
  this->context->t_renderer->core.texture.repository.free(
      this->context->t_renderer->core.texture.repository
          .getBySpriteId(textSelect.id)
          ->id);
  this->context->t_renderer->core.texture.repository.free(
      this->context->t_renderer->core.texture.repository
          .getBySpriteId(btnCross.id)
          ->id);
}

void StateMainMenu::handleInput() {
  if (this->context->t_pad->getClicked().Cross)
    this->selectedOption = this->activeOption;
}

u8 StateMainMenu::shouldInitGame() {
  return this->selectedOption == MainMenuOptions::PlayGame;
}

void StateMainMenu::loadGame() {
  this->context->setState(new StateLoadingGame(this->context));
}
