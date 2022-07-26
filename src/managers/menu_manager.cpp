#include "managers/menu_manager.hpp"
#include "file/file_utils.hpp"
#include <renderer/renderer_settings.hpp>
#include <debug/debug.hpp>
#include <thread/threading.hpp>

using Tyra::Audio;
using Tyra::FileUtils;
using Tyra::Renderer;
using Tyra::RendererSettings;
using Tyra::Threading;

MainMenu::MainMenu() {}

MainMenu::~MainMenu() {
  t_audio->stopSong();
  t_audio->setSongLoop(1);
}

void MainMenu::init(Renderer* t_renderer, Audio* t_audio) {
  /**
   * TODO: ->
   * - Load menu option
   *      - Play Game (DONE!)
   *      - How to play
   *      - About
   * */
  this->t_renderer = t_renderer;
  this->t_audio = t_audio;

  const float halfWidth = t_renderer->core.getSettings().getWidth() / 2;
  const float halfHeight =
      t_renderer->core.getSettings().getInterlacedHeightUI() / 2;

  // Background
  background[0].setMode(Tyra::MODE_STRETCH);
  background[0].size.set(halfWidth, halfHeight);
  background[0].position.set(0.0F, 0.0F);
  background[1].setMode(Tyra::MODE_STRETCH);
  background[1].size.set(halfWidth, halfHeight);
  background[1].position.set(halfWidth, 0.0F);
  background[2].setMode(Tyra::MODE_STRETCH);
  background[2].size.set(halfWidth, halfHeight);
  background[2].position.set(0.0F, halfHeight);
  background[3].setMode(Tyra::MODE_STRETCH);
  background[3].size.set(halfWidth, halfHeight);
  background[3].position.set(halfWidth, halfHeight);

  this->t_renderer->core.texture.repository
      .add(FileUtils::fromCwd("assets/textures/menu/background_tl.png"))
      ->addLink(background[0].getId());
  this->t_renderer->core.texture.repository
      .add(FileUtils::fromCwd("assets/textures/menu/background_tr.png"))
      ->addLink(background[1].getId());
  this->t_renderer->core.texture.repository
      .add(FileUtils::fromCwd("assets/textures/menu/background_bl.png"))
      ->addLink(background[2].getId());
  this->t_renderer->core.texture.repository
      .add(FileUtils::fromCwd("assets/textures/menu/background_br.png"))
      ->addLink(background[3].getId());

  // Load title
  // Title
  title[0].setMode(Tyra::MODE_STRETCH);
  title[0].size.set(128, 48);
  title[0].position.set((halfWidth)-256, 32);

  title[1].setMode(Tyra::MODE_STRETCH);
  title[1].size.set(128, 48);
  title[1].position.set(halfWidth - 128, 32);

  title[2].setMode(Tyra::MODE_STRETCH);
  title[2].size.set(128, 48);
  title[2].position.set(halfWidth, 32);

  title[3].setMode(Tyra::MODE_STRETCH);
  title[3].size.set(128, 48);
  title[3].position.set(halfWidth + 128, 32);

  this->t_renderer->core.texture.repository
      .add(FileUtils::fromCwd("assets/textures/menu/title_1.png"))
      ->addLink(title[0].getId());
  this->t_renderer->core.texture.repository
      .add(FileUtils::fromCwd("assets/textures/menu/title_2.png"))
      ->addLink(title[1].getId());
  this->t_renderer->core.texture.repository
      .add(FileUtils::fromCwd("assets/textures/menu/title_3.png"))
      ->addLink(title[2].getId());
  this->t_renderer->core.texture.repository
      .add(FileUtils::fromCwd("assets/textures/menu/title_4.png"))
      ->addLink(title[3].getId());

  // Alpha Version
  subtitle.setMode(Tyra::MODE_STRETCH);
  subtitle.size.set(130, 12);
  subtitle.position.set(halfWidth - 65, 82);

  this->t_renderer->core.texture.repository
      .add(FileUtils::fromCwd("assets/textures/menu/sub_title.png"))
      ->addLink(subtitle.getId());

  // Buttons
  btnCross.setMode(Tyra::MODE_STRETCH);
  btnCross.size.set(48, 27);
  btnCross.position.set(
      30, this->t_renderer->core.getSettings().getInterlacedHeightUI() - 30);

  this->t_renderer->core.texture.repository
      .add(FileUtils::fromCwd("assets/textures/ui/btn_cross.png"))
      ->addLink(btnCross.getId());

  // Load slots
  slot[0].setMode(Tyra::MODE_STRETCH);
  slot[0].size.set(200, 15);
  slot[0].position.set(halfWidth - 100, 132);
  slot[1].setMode(Tyra::MODE_STRETCH);
  slot[1].size.set(200, 15);
  slot[1].position.set(halfWidth - 100, 132 + 18);
  slot[2].setMode(Tyra::MODE_STRETCH);
  slot[2].size.set(200, 15);
  slot[2].position.set(halfWidth - 100, 132 + 36);

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
  textPlayGame.size.set(96, 8);
  textPlayGame.position.set(halfWidth - 40, 132 + 22);

  if (this->activeOption == PLAY_GAME) {
    textPlayGame.color.r = 255;
    textPlayGame.color.g = 255;
    textPlayGame.color.b = 0;
  }

  this->t_renderer->core.texture.repository
      .add(FileUtils::fromCwd("assets/textures/menu/play_game.png"))
      ->addLink(textPlayGame.getId());

  textSelect.setMode(Tyra::MODE_STRETCH);
  textSelect.size.set(64, 8);
  textSelect.position.set(
      30 + 40,
      this->t_renderer->core.getSettings().getInterlacedHeightUI() - 20);
  this->t_renderer->core.texture.repository
      .add(FileUtils::fromCwd("assets/textures/menu/select.png"))
      ->addLink(textSelect.getId());

  // TODO: Refactore to tyrav2 (no obj loader and BMP image format)
  //   menuSkybox.loadObj("meshes/menu-skybox/", "skybox", 400.0F, false);
  //   menuSkybox.shouldBeFrustumCulled = false;
  //   this->t_renderer->core.texture.repository.addByMesh("meshes/menu-skybox/",
  //   menuSkybox, BMP);

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
  // menuSkybox.rotation.y += .001F;
  // this->t_renderer->renderer2D.render(&menuSkybox);

  /**
   * --------------- Sprites ---------------
   * */
  // Background
  this->t_renderer->renderer2D.render(&background[0]);
  this->t_renderer->renderer2D.render(&background[1]);
  this->t_renderer->renderer2D.render(&background[2]);
  this->t_renderer->renderer2D.render(&background[3]);
  Threading::switchThread();

  // Title & Subtitle
  this->t_renderer->renderer2D.render(&title[0]);
  this->t_renderer->renderer2D.render(&title[1]);
  this->t_renderer->renderer2D.render(&title[2]);
  this->t_renderer->renderer2D.render(&title[3]);
  this->t_renderer->renderer2D.render(&subtitle);
  Threading::switchThread();

  // Slots
  this->t_renderer->renderer2D.render(&slot[0]);
  this->t_renderer->renderer2D.render(&slot[1]);
  this->t_renderer->renderer2D.render(&slot[2]);
  Threading::switchThread();

  // Texts
  this->t_renderer->renderer2D.render(&textPlayGame);
  this->t_renderer->renderer2D.render(&textSelect);

  // Buttons
  this->t_renderer->renderer2D.render(&btnCross);
}

u8 MainMenu::shouldInitGame() { return this->selectedOption == PLAY_GAME; }