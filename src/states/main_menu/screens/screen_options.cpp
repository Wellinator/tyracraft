#include "states/main_menu/screens/screen_options.hpp"
#include "states/main_menu/screens/screen_main.hpp"
#include "managers/font/font_manager.hpp"

ScreenOptions::ScreenOptions(StateMainMenu* t_context) : ScreenBase(t_context) {
  t_renderer = &t_context->context->t_engine->renderer;
  init();
}

ScreenOptions::~ScreenOptions() {
  t_renderer->getTextureRepository().free(raw_slot_texture->id);
  t_renderer->getTextureRepository().freeBySprite(active_slot);
  t_renderer->getTextureRepository().freeBySprite(btnTriangle);
  t_renderer->getTextureRepository().freeBySprite(btnCross);
}

void ScreenOptions::update() {
  handleInput();
  hightLightActiveOption();
}

void ScreenOptions::render() {
  t_renderer->renderer2D.render(background);

  t_renderer->renderer2D.render(raw_slot[0]);
  t_renderer->renderer2D.render(raw_slot[1]);
  t_renderer->renderer2D.render(raw_slot[2]);
  t_renderer->renderer2D.render(raw_slot[3]);
  t_renderer->renderer2D.render(raw_slot[4]);
  t_renderer->renderer2D.render(raw_slot[5]);

  t_renderer->renderer2D.render(active_slot);

  auto baseX = 248;
  auto baseY = 106;

  // Vsync
  {
    FontOptions fontOptions;
    fontOptions.scale = 0.8F;
    fontOptions.position.set(Vec2(baseX, baseY));
    fontOptions.alignment = TextAlignment::Center;
    fontOptions.color.set(activeOption == OptionsScreenOptions::VSyncOnOff
                              ? Tyra::Color(255, 255, 0)
                              : Tyra::Color(255, 255, 255));

    std::string _state = tempSettings.vsync ? Label_On : Label_Off;
    FontManager_printText(Label_UseVsync + std::string(": ") + _state,
                          fontOptions);
  }

  // Language
  {
    baseY += 40;
    FontOptions fontOptions;
    fontOptions.scale = 0.8F;
    fontOptions.position.set(Vec2(baseX, baseY));
    fontOptions.alignment = TextAlignment::Center;
    fontOptions.color.set(activeOption == OptionsScreenOptions::ChangeLanguage
                              ? Tyra::Color(255, 255, 0)
                              : Tyra::Color(255, 255, 255));
    FontManager_printText(Label_Language, fontOptions);
  }

  // LStickH
  {
    baseY += 40;
    FontOptions fontOptions;
    fontOptions.scale = 0.8F;
    fontOptions.position.set(Vec2(baseX, baseY));
    fontOptions.alignment = TextAlignment::Center;
    fontOptions.color.set(activeOption == OptionsScreenOptions::LStickDeadZoneH
                              ? Tyra::Color(255, 255, 0)
                              : Tyra::Color(255, 255, 255));

    std::string _value = std::to_string(tempSettings.l_stick_H);
    std::string _state = _value.substr(0, _value.find(".") + 3);

    FontManager_printText(Label_LStickH + _state, fontOptions);
  }

  // LStickV
  {
    baseY += 40;
    FontOptions fontOptions;
    fontOptions.scale = 0.8F;
    fontOptions.position.set(Vec2(baseX, baseY));
    fontOptions.alignment = TextAlignment::Center;
    fontOptions.color.set(activeOption == OptionsScreenOptions::LStickDeadZoneV
                              ? Tyra::Color(255, 255, 0)
                              : Tyra::Color(255, 255, 255));

    std::string _value = std::to_string(tempSettings.l_stick_V);
    std::string _state = _value.substr(0, _value.find(".") + 3);
    FontManager_printText(Label_LStickV + _state, fontOptions);
  }

  // RStickH
  {
    baseY += 40;
    FontOptions fontOptions;
    fontOptions.scale = 0.8F;
    fontOptions.position.set(Vec2(baseX, baseY));
    fontOptions.alignment = TextAlignment::Center;
    fontOptions.color.set(activeOption == OptionsScreenOptions::RStickDeadZoneH
                              ? Tyra::Color(255, 255, 0)
                              : Tyra::Color(255, 255, 255));

    std::string _value = std::to_string(tempSettings.r_stick_H);
    std::string _state = _value.substr(0, _value.find(".") + 3);
    FontManager_printText(Label_RStickH + _state, fontOptions);
  }

  // RStickV
  {
    baseY += 40;
    FontOptions fontOptions;
    fontOptions.scale = 0.8F;
    fontOptions.position.set(Vec2(baseX, baseY));
    fontOptions.alignment = TextAlignment::Center;
    fontOptions.color.set(activeOption == OptionsScreenOptions::RStickDeadZoneV
                              ? Tyra::Color(255, 255, 0)
                              : Tyra::Color(255, 255, 255));

    std::string _value = std::to_string(tempSettings.r_stick_V);
    std::string _state = _value.substr(0, _value.find(".") + 3);
    FontManager_printText(Label_RStickV + _state, fontOptions);
  }

  t_renderer->renderer2D.render(btnCross);
  FontManager_printText(Label_Save, 35, 407);

  t_renderer->renderer2D.render(btnTriangle);
  FontManager_printText(Label_Back, 235, 407);
}

void ScreenOptions::init() {
  TextureRepository* textureRepo = &t_renderer->getTextureRepository();
  const float halfWidth = t_renderer->core.getSettings().getWidth() / 2;

  background.mode = Tyra::MODE_STRETCH;
  background.size.set(512, 512);
  background.position.set(0, 0);
  textureRepo->add(FileUtils::fromCwd("textures/gui/options/background.png"))
      ->addLink(background.id);

  // Load slots
  auto baseX = halfWidth - SLOT_WIDTH / 2;
  auto baseY = 100;

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

  baseY += 40;
  raw_slot[4].mode = Tyra::MODE_STRETCH;
  raw_slot[4].size.set(SLOT_WIDTH, 35);
  raw_slot[4].position.set(baseX, baseY);

  baseY += 40;
  raw_slot[5].mode = Tyra::MODE_STRETCH;
  raw_slot[5].size.set(SLOT_WIDTH, 35);
  raw_slot[5].position.set(baseX, baseY);

  raw_slot_texture =
      textureRepo->add(FileUtils::fromCwd("textures/gui/slot.png"));

  raw_slot_texture->addLink(raw_slot[0].id);
  raw_slot_texture->addLink(raw_slot[1].id);
  raw_slot_texture->addLink(raw_slot[2].id);
  raw_slot_texture->addLink(raw_slot[3].id);
  raw_slot_texture->addLink(raw_slot[4].id);
  raw_slot_texture->addLink(raw_slot[5].id);

  active_slot.mode = Tyra::MODE_STRETCH;
  active_slot.size.set(SLOT_WIDTH, 35);
  active_slot.position.set(halfWidth - SLOT_WIDTH / 2, 230);

  textureRepo->add(FileUtils::fromCwd("textures/gui/slot_input.png"))
      ->addLink(active_slot.id);

  // Buttons
  btnCross.mode = Tyra::MODE_STRETCH;
  btnCross.size.set(25, 25);
  btnCross.position.set(15, t_renderer->core.getSettings().getHeight() - 40);
  textureRepo->add(FileUtils::fromCwd("textures/gui/btn_cross.png"))
      ->addLink(btnCross.id);

  btnTriangle.mode = Tyra::MODE_STRETCH;
  btnTriangle.size.set(25, 25);
  btnTriangle.position.set(215,
                           t_renderer->core.getSettings().getHeight() - 40);
  textureRepo->add(FileUtils::fromCwd("textures/gui/btn_triangle.png"))
      ->addLink(btnTriangle.id);
}

void ScreenOptions::handleInput() {
  // Change active option
  {
    if (context->context->t_engine->pad.getClicked().DpadDown) {
      int nextOption = (u8)activeOption + 1;
      if (nextOption > static_cast<u8>(OptionsScreenOptions::RStickDeadZoneV))
        activeOption = OptionsScreenOptions::VSyncOnOff;
      else
        activeOption = static_cast<OptionsScreenOptions>(nextOption);
    } else if (context->context->t_engine->pad.getClicked().DpadUp) {
      int nextOption = (u8)activeOption - 1;
      if (nextOption < 0)
        activeOption = OptionsScreenOptions::VSyncOnOff;
      else
        activeOption = static_cast<OptionsScreenOptions>(nextOption);
    }
  }

  if (context->context->t_engine->pad.getClicked().Triangle) {
    context->playClickSound();
    context->setScreen(new ScreenMain(context));
  } else if (context->context->t_engine->pad.getClicked().Cross) {
    context->playClickSound();
    SettingsManager::Save(tempSettings);

    if (activeOption == OptionsScreenOptions::ChangeLanguage) {
      context->goToLanguageSelectioScreen();
    } else {
      context->setScreen(new ScreenMain(context));
    }

  } else if (context->context->t_engine->pad.getClicked().DpadLeft) {
    if (activeOption == OptionsScreenOptions::VSyncOnOff) {
      tempSettings.vsync = !tempSettings.vsync;
    } else if (activeOption == OptionsScreenOptions::LStickDeadZoneH) {
      tempSettings.l_stick_H -= 0.05F;
      if (tempSettings.l_stick_H < 0) tempSettings.l_stick_H = 0;
    } else if (activeOption == OptionsScreenOptions::LStickDeadZoneV) {
      tempSettings.l_stick_V -= 0.05F;
      if (tempSettings.l_stick_V < 0) tempSettings.l_stick_V = 0;
    } else if (activeOption == OptionsScreenOptions::RStickDeadZoneH) {
      tempSettings.r_stick_H -= 0.05F;
      if (tempSettings.r_stick_H < 0) tempSettings.r_stick_H = 0;
    } else if (activeOption == OptionsScreenOptions::RStickDeadZoneV) {
      tempSettings.r_stick_V -= 0.05F;
      if (tempSettings.r_stick_V < 0) tempSettings.r_stick_V = 0;
    }
  } else if (context->context->t_engine->pad.getClicked().DpadRight) {
    if (activeOption == OptionsScreenOptions::VSyncOnOff) {
      tempSettings.vsync = !tempSettings.vsync;
    } else if (activeOption == OptionsScreenOptions::LStickDeadZoneH) {
      tempSettings.l_stick_H += 0.05F;
    } else if (activeOption == OptionsScreenOptions::LStickDeadZoneV) {
      tempSettings.l_stick_V += 0.05F;
    } else if (activeOption == OptionsScreenOptions::RStickDeadZoneH) {
      tempSettings.r_stick_H += 0.05F;
    } else if (activeOption == OptionsScreenOptions::RStickDeadZoneV) {
      tempSettings.r_stick_V += 0.05F;
    }
  }
}

void ScreenOptions::hightLightActiveOption() {
  u8 option = (int)activeOption;
  active_slot.position.y =
      (option * SLOT_HIGHT_OPTION_OFFSET) + SLOT_HIGHT_OFFSET;
}
