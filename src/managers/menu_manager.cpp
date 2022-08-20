#include "managers/menu_manager.hpp"
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

MainMenu::MainMenu() {}

MainMenu::~MainMenu() {
  t_audio->song.stop();
  t_audio->song.inLoop = false;
  this->unloadTextures();
}

void MainMenu::init(Renderer* t_renderer, Audio* t_audio) {
  /**
   * TODO: ->
   * - Load menu option
   *      - Play Game (DONE!)
   *      - How to play
   *      - About
   * */

  this->stapip.setRenderer(&t_renderer->core);
  this->t_renderer = t_renderer;
  this->t_audio = t_audio;

  const float halfWidth = t_renderer->core.getSettings().getWidth() / 2;
  const float halfHeight = t_renderer->core.getSettings().getHeight() / 2;

  this->loadSkybox(t_renderer);

  // Load title
  // Title
  title[0].mode = Tyra::MODE_STRETCH;
  title[0].size.set(256, 128);
  title[0].position.set((halfWidth)-256, 64);

  title[1].mode = Tyra::MODE_STRETCH;
  title[1].size.set(256, 128);
  title[1].position.set(halfWidth, 64);

  this->t_renderer->core.texture.repository
      .add(FileUtils::fromCwd("assets/menu/title_1.png"))
      ->addLink(title[0].id);
  this->t_renderer->core.texture.repository
      .add(FileUtils::fromCwd("assets/menu/title_2.png"))
      ->addLink(title[1].id);

  // Buttons
  btnCross.mode = Tyra::MODE_STRETCH;
  btnCross.size.set(25, 25);
  btnCross.position.set(40,
                        this->t_renderer->core.getSettings().getHeight() - 52);

  this->t_renderer->core.texture.repository
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

  this->t_renderer->core.texture.repository
      .add(FileUtils::fromCwd("assets/menu/slot.png"))
      ->addLink(slot[0].id);
  this->t_renderer->core.texture.repository
      .add(FileUtils::fromCwd("assets/menu/hovered_slot.png"))
      ->addLink(slot[1].id);
  this->t_renderer->core.texture.repository
      .add(FileUtils::fromCwd("assets/menu/selected_slot.png"))
      ->addLink(slot[2].id);

  // Texts
  textPlayGame.mode = Tyra::MODE_STRETCH;
  textPlayGame.size.set(80, 15);
  textPlayGame.position.set(halfWidth - 40, 265 + 35);

  if (this->activeOption == PLAY_GAME) {
    textPlayGame.color.r = 255;
    textPlayGame.color.g = 255;
    textPlayGame.color.b = 0;
  }

  this->t_renderer->core.texture.repository
      .add(FileUtils::fromCwd("assets/menu/play_game.png"))
      ->addLink(textPlayGame.id);

  textSelect.mode = Tyra::MODE_STRETCH;
  textSelect.size.set(64, 16);
  textSelect.position.set(
      30 + 40, this->t_renderer->core.getSettings().getHeight() - 47);
  this->t_renderer->core.texture.repository
      .add(FileUtils::fromCwd("assets/menu/select.png"))
      ->addLink(textSelect.id);

  // Load song
  t_audio->song.load(FileUtils::fromCwd("sounds/menu.wav"));
  t_audio->song.play();
  t_audio->song.inLoop = true;
  t_audio->song.setVolume(100);
}

void MainMenu::update(Pad& t_pad) {
  if (t_pad.getClicked().Cross) this->selectedOption = this->activeOption;
}

void MainMenu::render() {
  // Meshes
  this->menuSkybox->rotation.rotateY(0.001F);
  this->t_renderer->renderer3D.usePipeline(&stapip);
  { stapip.render(this->menuSkybox, skyboxOptions); }

  /**
   * --------------- Sprites ---------------
   * */

  //   // Title & Subtitle
  this->t_renderer->renderer2D.render(&title[0]);
  this->t_renderer->renderer2D.render(&title[1]);

  //   // Slots
  this->t_renderer->renderer2D.render(&slot[0]);
  this->t_renderer->renderer2D.render(&slot[1]);
  this->t_renderer->renderer2D.render(&slot[2]);

  //   // Texts
  this->t_renderer->renderer2D.render(&textPlayGame);
  this->t_renderer->renderer2D.render(&textSelect);

  //   // Buttons
  this->t_renderer->renderer2D.render(&btnCross);
}

u8 MainMenu::shouldInitGame() { return this->selectedOption == PLAY_GAME; }

void MainMenu::loadSkybox(Renderer* renderer) {
  this->skyboxOptions = new StaPipOptions();
  this->skyboxOptions->fullClipChecks = false;
  this->skyboxOptions->textureMappingType =
      Tyra::PipelineTextureMappingType::TyraNearest;
  this->skyboxOptions->frustumCulling =
      Tyra::PipelineFrustumCulling::PipelineFrustumCulling_None;

  ObjLoader loader;
  ObjLoaderOptions objOptions;
  objOptions.flipUVs = true;
  objOptions.scale = 250.0F;

  auto* data =
      loader.load(FileUtils::fromCwd("assets/menu/skybox.obj"), objOptions);
  data->normalsEnabled = false;
  this->menuSkybox = new StaticMesh(*data);
  delete data;

  renderer->core.texture.repository.addByMesh(
      this->menuSkybox, FileUtils::fromCwd("assets/menu/"), "png");
}

void MainMenu::unloadTextures() {
  for (u8 i = 0; i < menuSkybox->materials.size(); i++) {
    t_renderer->core.texture.repository.free(
        t_renderer->core.texture.repository
            .getBySpriteOrMesh(menuSkybox->materials[i]->id)
            ->id);
  }
  for (u8 i = 0; i < 2; i++) {
    t_renderer->core.texture.repository.free(
        t_renderer->core.texture.repository.getBySpriteOrMesh(title[i].id)->id);
  }
  for (u8 i = 0; i < 3; i++) {
    t_renderer->core.texture.repository.free(
        t_renderer->core.texture.repository.getBySpriteOrMesh(slot[i].id)->id);
  }
  t_renderer->core.texture.repository.free(
      t_renderer->core.texture.repository.getBySpriteOrMesh(textPlayGame.id)
          ->id);
  t_renderer->core.texture.repository.free(
      t_renderer->core.texture.repository.getBySpriteOrMesh(textSelect.id)->id);
  t_renderer->core.texture.repository.free(
      t_renderer->core.texture.repository.getBySpriteOrMesh(btnCross.id)->id);
}