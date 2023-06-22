#include "states/main_menu/screens/screen_main.hpp"
#include "states/main_menu/screens/screen_how_to_play.hpp"
#include "states/main_menu/screens/screen_options.hpp"
#include "states/main_menu/screens/screen_about.hpp"
#include "states/main_menu/screens/screen_new_game.hpp"
#include "states/main_menu/screens/screen_load_game.hpp"
#include "managers/font/font_manager.hpp"

ScreenMain::ScreenMain(StateMainMenu* t_context) : ScreenBase(t_context) {
  t_renderer = &t_context->context->t_engine->renderer;
  init();
}

ScreenMain::~ScreenMain() {
  t_renderer->getTextureRepository().free(raw_slot_texture->id);
  t_renderer->getTextureRepository().freeBySprite(active_slot);
  t_renderer->getTextureRepository().freeBySprite(btnCross);
}

void ScreenMain::update() {
  handleInput();
  hightLightActiveOption();
  navigate();
}

void ScreenMain::render() {
  t_renderer->renderer2D.render(raw_slot[0]);
  t_renderer->renderer2D.render(raw_slot[1]);
  t_renderer->renderer2D.render(raw_slot[2]);
  t_renderer->renderer2D.render(raw_slot[3]);
  t_renderer->renderer2D.render(active_slot);

  auto baseX = 248;
  auto baseY = 206;

  // New Game
  {
    FontOptions fontOptions;
    fontOptions.position.set(Vec2(baseX, baseY));
    fontOptions.alignment = TextAlignment::Center;
    fontOptions.color.set(activeOption == ScreenMainOptions::PlayGame
                              ? Tyra::Color(255, 255, 0)
                              : Tyra::Color(255, 255, 255));

    FontManager_printText(Label_PlayGame, fontOptions);
  }

  // Options
  {
    baseY += 40;
    FontOptions fontOptions;
    fontOptions.position.set(Vec2(baseX, baseY));
    fontOptions.alignment = TextAlignment::Center;
    fontOptions.color.set(activeOption == ScreenMainOptions::Options
                              ? Tyra::Color(255, 255, 0)
                              : Tyra::Color(255, 255, 255));

    FontManager_printText(Label_Options, fontOptions);
  }

  // How To Play
  {
    baseY += 40;
    FontOptions fontOptions;
    fontOptions.position.set(Vec2(baseX, baseY));
    fontOptions.alignment = TextAlignment::Center;
    fontOptions.color.set(activeOption == ScreenMainOptions::HowToPlay
                              ? Tyra::Color(255, 255, 0)
                              : Tyra::Color(255, 255, 255));
    FontManager_printText(Label_HowToPlay, fontOptions);
  }

  // About
  {
    baseY += 40;
    FontOptions fontOptions;
    fontOptions.position.set(Vec2(baseX, baseY));
    fontOptions.alignment = TextAlignment::Center;
    fontOptions.color.set(activeOption == ScreenMainOptions::About
                              ? Tyra::Color(255, 255, 0)
                              : Tyra::Color(255, 255, 255));
    FontManager_printText(Label_About, fontOptions);
  }

  t_renderer->renderer2D.render(btnCross);
  FontManager_printText(Label_Select, 35, 407);
}

void ScreenMain::init() {
  const float halfWidth = t_renderer->core.getSettings().getWidth() / 2;
  auto baseX = halfWidth - SLOT_WIDTH / 2;
  auto baseY = 200;

  // Load slots
  raw_slot[0].mode = Tyra::MODE_STRETCH;
  raw_slot[0].size.set(SLOT_WIDTH, 35);
  raw_slot[0].position.set(baseX, baseY);

  baseY += 40;
  raw_slot[1].mode = Tyra::MODE_STRETCH;
  raw_slot[1].size.set(SLOT_WIDTH, 35);
  raw_slot[1].position.set(baseX, baseY);

  baseY += 40;
  raw_slot[2].mode = Tyra::MODE_STRETCH;
  raw_slot[2].size.set(SLOT_WIDTH, 35);
  raw_slot[2].position.set(baseX, baseY);

  baseY += 40;
  raw_slot[3].mode = Tyra::MODE_STRETCH;
  raw_slot[3].size.set(SLOT_WIDTH, 35);
  raw_slot[3].position.set(baseX, baseY);

  raw_slot_texture = t_renderer->getTextureRepository().add(
      FileUtils::fromCwd("textures/gui/slot.png"));

  raw_slot_texture->addLink(raw_slot[0].id);
  raw_slot_texture->addLink(raw_slot[1].id);
  raw_slot_texture->addLink(raw_slot[2].id);
  raw_slot_texture->addLink(raw_slot[3].id);

  active_slot.mode = Tyra::MODE_STRETCH;
  active_slot.size.set(SLOT_WIDTH, 35);
  active_slot.position.set(halfWidth - SLOT_WIDTH / 2, 230);

  t_renderer->getTextureRepository()
      .add(FileUtils::fromCwd("textures/gui/slot_active.png"))
      ->addLink(active_slot.id);

  // Buttons
  btnCross.mode = Tyra::MODE_STRETCH;
  btnCross.size.set(25, 25);
  btnCross.position.set(15, t_renderer->core.getSettings().getHeight() - 40);

  t_renderer->getTextureRepository()
      .add(FileUtils::fromCwd("textures/gui/btn_cross.png"))
      ->addLink(btnCross.id);
}

void ScreenMain::handleInput() {
  // Change active option
  {
    if (context->context->t_engine->pad.getClicked().DpadDown) {
      int nextOption = (int)activeOption + 1;
      if (nextOption > static_cast<uint8_t>(ScreenMainOptions::About))
        activeOption = ScreenMainOptions::PlayGame;
      else
        activeOption = static_cast<ScreenMainOptions>(nextOption);
    } else if (context->context->t_engine->pad.getClicked().DpadUp) {
      int nextOption = (int)activeOption - 1;
      if (nextOption < 0)
        activeOption = ScreenMainOptions::About;
      else
        activeOption = static_cast<ScreenMainOptions>(nextOption);
    }
  }

  if (context->context->t_engine->pad.getClicked().Cross) {
    context->playClickSound();
    selectedOption = activeOption;
  }
}

void ScreenMain::hightLightActiveOption() {
  u8 option = (int)activeOption;
  active_slot.position.y =
      (option * SLOT_HIGHT_OPTION_OFFSET) + SLOT_HIGHT_OFFSET;
}

void ScreenMain::navigate() {
  if (selectedOption == ScreenMainOptions::None) return;

  if (selectedOption == ScreenMainOptions::PlayGame)
    context->setScreen(new ScreenLoadGame(context));
  else if (selectedOption == ScreenMainOptions::Options)
    context->setScreen(new ScreenOptions(context));
  else if (selectedOption == ScreenMainOptions::HowToPlay)
    context->setScreen(new ScreenHowToPlay(context));
  else if (selectedOption == ScreenMainOptions::About)
    context->setScreen(new ScreenAbout(context));
}
