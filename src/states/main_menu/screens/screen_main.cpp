#include "states/main_menu/screens/screen_main.hpp"
#include "states/main_menu/screens/screen_how_to_play.hpp"
#include "states/main_menu/screens/screen_options.hpp"
#include "states/main_menu/screens/screen_about.hpp"
#include "states/main_menu/screens/screen_new_game.hpp"
#include "states/main_menu/screens/screen_load_game.hpp"
#include "states/main_menu/screens/screen_skin_selection.hpp"
#include "managers/font/font_manager.hpp"
#include "managers/save_manager.hpp"

using Tyra::MeshBuilderData;
using Tyra::ObjLoaderOptions;
using Tyra::StaPipBag;
using Tyra::StaPipColorBag;
using Tyra::StaPipInfoBag;
using Tyra::StaPipTextureBag;

ScreenMain::ScreenMain(StateMainMenu* t_context) : ScreenBase(t_context) {
  t_renderer = &t_context->context->t_engine->renderer;
  init();
}

ScreenMain::~ScreenMain() {
  t_renderer->getTextureRepository().free(raw_slot_texture->id);
  t_renderer->getTextureRepository().freeBySprite(active_slot);
  t_renderer->getTextureRepository().freeBySprite(btnCross);
  t_renderer->getTextureRepository().freeBySprite(beacon_button_default);
  t_renderer->getTextureRepository().freeBySprite(beacon_button_hover);
  t_renderer->getTextureRepository().freeBySprite(glyph_skin_pack);
  t_renderer->getTextureRepository().free(skinTexture);
}

void ScreenMain::update(const float& deltaTime) {
  animator.update(deltaTime);

  handleInput();
  hightLightActiveOption();
  if (shouldNavigate) navigate();
}

void ScreenMain::render() {
  FontManager& fm = FontManager::getInstanceRef();

  t_renderer->renderer2D.render(raw_slot[0]);
  t_renderer->renderer2D.render(raw_slot[1]);
  t_renderer->renderer2D.render(raw_slot[2]);
  t_renderer->renderer2D.render(raw_slot[3]);
  t_renderer->renderer2D.render(beacon_button_default);

  if (activeOption == ScreenMainOptions::SkinSelection) {
    t_renderer->renderer2D.render(beacon_button_hover);
  } else {
    t_renderer->renderer2D.render(active_slot);
  }

  t_renderer->renderer2D.render(glyph_skin_pack);

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

    fm.printText(Label_PlayGame, fontOptions);
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

    fm.printText(Label_Options, fontOptions);
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
    fm.printText(Label_HowToPlay, fontOptions);
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
    fm.printText(Label_About, fontOptions);
  }

  t_renderer->renderer2D.render(btnCross);
  fm.printText(Label_Select, 35, 407);

  // Draw player skin
  renderPlayerPreview();

  // Draw player name
  // About
  {
    FontOptions fontOptions;
    fontOptions.scale = 0.9f;
    fontOptions.position.set(Vec2(baseX + 160, baseY - 20));
    fontOptions.alignment = TextAlignment::Center;
    fontOptions.color.set(activeOption == ScreenMainOptions::SkinSelection
                              ? Tyra::Color(255, 255, 0)
                              : Tyra::Color(255, 255, 255));
    fm.printText(g_settings.skin, fontOptions);
  }
}

void ScreenMain::renderPlayerPreview() {
  std::vector<Vec4> vertices = {};
  std::vector<Color> verticesColors = {};
  std::vector<Vec4> uvMap = {};

  // Calc draw data by frames interpolation
  const AnimationOptions currentAnimationOptions =
      animator.getCurrentAnimation();
  if (currentAnimationOptions.framesIndices.size() > 1) {
    animator.fillDrawDataByLerp(&vertices, &verticesColors, &uvMap);
  } else {
    animator.fillDrawDataByFrame(&vertices, &verticesColors, &uvMap);
  }

  StaPipTextureBag textureBag;
  StaPipInfoBag infoBag;
  StaPipColorBag colorBag;
  StaPipBag bag;

  textureBag.coordinates = uvMap.data();
  textureBag.texture = skinTexture;

  infoBag.textureMappingType = Tyra::PipelineTextureMappingType::TyraNearest;
  infoBag.shadingType = Tyra::PipelineShadingType::TyraShadingGouraud;
  infoBag.blendingEnabled = true;
  infoBag.antiAliasingEnabled = false;
  infoBag.fullClipChecks = false;
  infoBag.frustumCulling =
      Tyra::PipelineInfoBagFrustumCulling::PipelineInfoBagFrustumCulling_None;

  Color tempColor = Color(128, 128, 128);
  colorBag.single = &tempColor;

  infoBag.model = &playerPreviewModelMatrix;

  bag.count = vertices.size();
  bag.vertices = vertices.data();
  bag.color = &colorBag;
  bag.info = &infoBag;
  bag.texture = &textureBag;

  t_renderer->renderer3D.usePipeline(statPip);
  statPip.core.render(&bag);
}

void ScreenMain::init() {
  statPip.setRenderer(&t_renderer->core);

  // Load animation frames
  ObjLoaderOptions options;
  options.scale = 3.4F;
  options.flipUVs = true;
  options.animation.count = 1;

  for (size_t i = 0; i < animationFrames.size(); i++) {
    std::unique_ptr<MeshBuilderData> tempFrameData = ObjLoader::load(
        FileUtils::fromCwd("models/player/stand_still/player_frame_" +
                           std::to_string(i + 1) + ".obj"),
        options);
    tempFrameData->loadNormals = false;
    tempFrameData->loadLightmap = false;
    animationFrames[i] = std::make_unique<Tyra::Mesh>(tempFrameData.get());
  }

  Tyra::Mesh* rawFrames[2] = {
      animationFrames[0].get(),
      animationFrames[1].get(),
  };
  animator.setFrames(rawFrames, 2);

  playerPreviewModelMatrix.identity();
  playerPreviewModelMatrix.rotateY(_90DEGINRAD - 0.25f);
  playerPreviewModelMatrix.translate(Vec4(25.0f, 19.5F, 10.0f));

  std::vector<u8> standStillSequence = {0, 1};
  AnimationOptions idleAnimation;
  idleAnimation.animationId = IDLE_ANIMATION;
  idleAnimation.framesIndices = standStillSequence;
  idleAnimation.durationInMs = 1000.0f;
  idleAnimation.loop = true;
  idleAnimation.wrapFrames = false;

  animator.addAnimation(idleAnimation);
  animator.setAnimation(IDLE_ANIMATION);

  loadSkinTexture(t_renderer);

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

  beacon_button_default.mode = Tyra::MODE_STRETCH;
  beacon_button_default.size.set(32, 32);
  beacon_button_default.position.set(baseX + 225, baseY + 20);

  t_renderer->getTextureRepository()
      .add(FileUtils::fromCwd("textures/gui/beacon_button_default.png"))
      ->addLink(beacon_button_default.id);

  beacon_button_hover.mode = Tyra::MODE_STRETCH;
  beacon_button_hover.size.set(32, 32);
  beacon_button_hover.position.set(baseX + 225, baseY + 20);

  t_renderer->getTextureRepository()
      .add(FileUtils::fromCwd("textures/gui/beacon_button_hover.png"))
      ->addLink(beacon_button_hover.id);

  glyph_skin_pack.mode = Tyra::MODE_STRETCH;
  glyph_skin_pack.size.set(24, 24);
  glyph_skin_pack.position.set(baseX + 229, baseY + 24);

  t_renderer->getTextureRepository()
      .add(FileUtils::fromCwd("textures/gui/glyph_skin_pack.png"))
      ->addLink(glyph_skin_pack.id);
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
    } else if (context->context->t_engine->pad.getClicked().DpadRight) {
      if (activeOption != ScreenMainOptions::SkinSelection) {
        activeOption = ScreenMainOptions::SkinSelection;
      }
    } else if (context->context->t_engine->pad.getClicked().DpadLeft) {
      if (activeOption == ScreenMainOptions::SkinSelection) {
        activeOption = ScreenMainOptions::PlayGame;
      }
    }
  }

  if (context->context->t_engine->pad.getClicked().Cross) {
    context->playClickSound();
    selectedOption = activeOption;
    shouldNavigate = true;
  }
}

void ScreenMain::hightLightActiveOption() {
  u8 option = (int)activeOption;
  active_slot.position.y =
      (option * SLOT_HIGHT_OPTION_OFFSET) + SLOT_HIGHT_OFFSET;
}

void ScreenMain::navigate() {
  shouldNavigate = 0;

  if (selectedOption == ScreenMainOptions::None) return;

  if (selectedOption == ScreenMainOptions::PlayGame) {
    // Open Load tab if saves exist, otherwise open Create tab
    if (SaveManager::HasAvailableSaves())
      context->setScreen(new ScreenLoadGame(context));
    else
      context->setScreen(new ScreenNewGame(context));
  } else if (selectedOption == ScreenMainOptions::Options)
    context->setScreen(new ScreenOptions(context));
  else if (selectedOption == ScreenMainOptions::HowToPlay)
    context->setScreen(new ScreenHowToPlay(context));
  else if (selectedOption == ScreenMainOptions::About)
    context->setScreen(new ScreenAbout(context));
  else if (selectedOption == ScreenMainOptions::SkinSelection)
    context->setScreen(new ScreenSkinSelection(context));
}

void ScreenMain::loadSkinTexture(Renderer* renderer) {
  const auto skinPath =
      std::string("textures/skin/").append(g_settings.skin).append(".png");

  skinTexture = renderer->getTextureRepository().add(
      FileUtils::fromCwd(skinPath.c_str()));
}
