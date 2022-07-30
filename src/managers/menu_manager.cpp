#include "managers/menu_manager.hpp"
#include "file/file_utils.hpp"
#include <renderer/renderer_settings.hpp>
#include <debug/debug.hpp>
#include "loaders/3d/tyrobj/tyrobj_loader.hpp"

using Tyra::Audio;
using Tyra::FileUtils;
using Tyra::Renderer;
using Tyra::RendererSettings;
using Tyra::TyrobjLoader;

MainMenu::MainMenu() {}

MainMenu::~MainMenu() {
  t_audio->stopSong();
  t_audio->setSongLoop(false);
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
  title[0].setMode(Tyra::MODE_STRETCH);
  title[0].size.set(256, 128);
  title[0].position.set((halfWidth)-256, 64);

  title[1].setMode(Tyra::MODE_STRETCH);
  title[1].size.set(256, 128);
  title[1].position.set(halfWidth, 64);

  this->t_renderer->core.texture.repository
      .add(FileUtils::fromCwd("assets/textures/menu/title_1.png"))
      ->addLink(title[0].getId());
  this->t_renderer->core.texture.repository
      .add(FileUtils::fromCwd("assets/textures/menu/title_2.png"))
      ->addLink(title[1].getId());

  // Buttons
  btnCross.setMode(Tyra::MODE_STRETCH);
  btnCross.size.set(25, 25);
  btnCross.position.set(40,
                        this->t_renderer->core.getSettings().getHeight() - 52);

  this->t_renderer->core.texture.repository
      .add(FileUtils::fromCwd("assets/textures/ui/btn_cross.png"))
      ->addLink(btnCross.getId());

  // Load slots
  slot[0].setMode(Tyra::MODE_STRETCH);
  slot[0].size.set(200, 25);
  slot[0].position.set(halfWidth - 100, 265);
  slot[1].setMode(Tyra::MODE_STRETCH);
  slot[1].size.set(200, 25);
  slot[1].position.set(halfWidth - 100, 265 + 30);
  slot[2].setMode(Tyra::MODE_STRETCH);
  slot[2].size.set(200, 25);
  slot[2].position.set(halfWidth - 100, 265 + 60);

  this->t_renderer->core.texture.repository
      .add(FileUtils::fromCwd("assets/textures/menu/slot.png"))
      ->addLink(slot[0].getId());
  this->t_renderer->core.texture.repository
      .add(FileUtils::fromCwd("assets/textures/menu/hovered_slot.png"))
      ->addLink(slot[1].getId());
  this->t_renderer->core.texture.repository
      .add(FileUtils::fromCwd("assets/textures/menu/selected_slot.png"))
      ->addLink(slot[2].getId());

  // Texts
  textPlayGame.setMode(Tyra::MODE_STRETCH);
  textPlayGame.size.set(80, 15);
  textPlayGame.position.set(halfWidth - 40, 265 + 35);

  if (this->activeOption == PLAY_GAME) {
    textPlayGame.color.r = 255;
    textPlayGame.color.g = 255;
    textPlayGame.color.b = 0;
  }

  this->t_renderer->core.texture.repository
      .add(FileUtils::fromCwd("assets/textures/menu/play_game.png"))
      ->addLink(textPlayGame.getId());

  textSelect.setMode(Tyra::MODE_STRETCH);
  textSelect.size.set(64, 16);
  textSelect.position.set(
      30 + 40, this->t_renderer->core.getSettings().getHeight() - 47);
  this->t_renderer->core.texture.repository
      .add(FileUtils::fromCwd("assets/textures/menu/select.png"))
      ->addLink(textSelect.getId());

  // Load song
  t_audio->loadSong(FileUtils::fromCwd("sounds/menu.wav"));
  t_audio->playSong();
  t_audio->setSongLoop(1);
  t_audio->setSongVolume(100);
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
  this->skyboxOptions->fullClipChecks = true;
  this->skyboxOptions->frustumCulling =
      Tyra::PipelineFrustumCulling::PipelineFrustumCulling_Precise;

  TyrobjLoader loader;
  auto* data = loader.load(
      FileUtils::fromCwd("meshes/menu-skybox/skybox.tyrobj"), 1, 50.0F, true);
  this->menuSkybox = new StaticMesh(*data);
  this->menuSkybox->translation.translate(Vec4(0.0F, 25.0F, 0.0F, 1.0F));
  delete data;

  renderer->core.texture.repository.addByMesh(
      this->menuSkybox, FileUtils::fromCwd("meshes/menu-skybox/"), "png");
}

void MainMenu::unloadTextures() {
  for (u8 i = 0; i < menuSkybox->getMaterialsCount(); i++) {
    t_renderer->core.texture.repository.free(
        t_renderer->core.texture.repository
            .getBySpriteOrMesh(menuSkybox->getMaterials()[i]->getId())
            ->getId());
  }
  for (u8 i = 0; i < 2; i++) {
    t_renderer->core.texture.repository.free(
        t_renderer->core.texture.repository.getBySpriteOrMesh(title[i].getId())
            ->getId());
  }
  for (u8 i = 0; i < 3; i++) {
    t_renderer->core.texture.repository.free(
        t_renderer->core.texture.repository.getBySpriteOrMesh(slot[i].getId())
            ->getId());
  }
  t_renderer->core.texture.repository.free(
      t_renderer->core.texture.repository
          .getBySpriteOrMesh(textPlayGame.getId())
          ->getId());
  t_renderer->core.texture.repository.free(
      t_renderer->core.texture.repository.getBySpriteOrMesh(textSelect.getId())
          ->getId());
  t_renderer->core.texture.repository.free(
      t_renderer->core.texture.repository.getBySpriteOrMesh(btnCross.getId())
          ->getId());
}