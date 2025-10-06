#include "states/main_menu/screens/screen_skin_selection.hpp"
#include "states/main_menu/screens/screen_main.hpp"
#include "managers/font/font_manager.hpp"

ScreenSkinSelection::ScreenSkinSelection(StateMainMenu* t_context)
    : ScreenBase(t_context) {
  t_renderer = &t_context->context->t_engine->renderer;
  init();
}

ScreenSkinSelection::~ScreenSkinSelection() {
  TextureRepository* textureRepo = &t_renderer->getTextureRepository();

  textureRepo->freeBySprite(btnCross);
  textureRepo->freeBySprite(btnTriangle);
  textureRepo->freeBySprite(arrowLeft);
  textureRepo->freeBySprite(arrowRight);

  unloadSkinTextures();
}

void ScreenSkinSelection::update(const float& deltaTime) {
  handleInput();

  if (isMoving)
    isMovingForward ? moveForward(deltaTime) : moveBackward(deltaTime);

  mannequins[0]->update(deltaTime);
  mannequins[1]->update(deltaTime);
  mannequins[2]->update(deltaTime);
}

void ScreenSkinSelection::render() {
  FontManager& fm = FontManager::getInstanceRef();

  t_renderer->renderer3D.usePipeline(&statPip);
  mannequins[0]->render(&statPip, textures[0]);
  mannequins[1]->render(&statPip, textures[1]);
  mannequins[2]->render(&statPip, textures[2]);

  t_renderer->renderer2D.render(btnCross);
  fm.printText(Label_Select, 35, 407);

  t_renderer->renderer2D.render(btnTriangle);
  fm.printText(Label_Back, 160, 407);

  if (!isMoving) {
    t_renderer->renderer2D.render(arrowLeft);
    t_renderer->renderer2D.render(arrowRight);

    FontOptions fontOptions;
    fontOptions.scale = 0.9f;
    fontOptions.position.set(Vec2(250.0F, 365.0F));
    fontOptions.alignment = TextAlignment::Center;
    fontOptions.color.set(Tyra::Color(255, 255, 255));

    fm.printText(selectedSkin.name, fontOptions);
  }
}

void ScreenSkinSelection::init() {
  statPip.setRenderer(&t_renderer->core);

  getAvailableSkins();
  loadSkinTextures();
  loadModels();

  TextureRepository* textureRepo = &t_renderer->getTextureRepository();

  const float HEIGHT = 448.0F;

  btnCross.mode = Tyra::MODE_STRETCH;
  btnCross.size.set(25, 25);
  btnCross.position.set(15, t_renderer->core.getSettings().getHeight() - 40);
  textureRepo->add(FileUtils::fromCwd("textures/gui/btn_cross.png"))
      ->addLink(btnCross.id);

  btnTriangle.mode = Tyra::MODE_STRETCH;
  btnTriangle.size.set(25, 25);
  btnTriangle.position.set(140,
                           t_renderer->core.getSettings().getHeight() - 40);
  textureRepo->add(FileUtils::fromCwd("textures/gui/btn_triangle.png"))
      ->addLink(btnTriangle.id);

  arrowLeft.mode = Tyra::MODE_STRETCH;
  arrowLeft.size.set(32, 32);
  arrowLeft.color.set(250, 250, 250);
  arrowLeft.position.set(165, HEIGHT / 2 + 56);
  textureRepo->add(FileUtils::fromCwd("textures/gui/chevron_grey_left.png"))
      ->addLink(arrowLeft.id);

  arrowRight.mode = Tyra::MODE_STRETCH;
  arrowRight.size.set(32, 32);
  arrowRight.color.set(250, 250, 250);
  arrowRight.position.set(320, HEIGHT / 2 + 56);
  textureRepo->add(FileUtils::fromCwd("textures/gui/chevron_grey_right.png"))
      ->addLink(arrowRight.id);
}

void ScreenSkinSelection::handleInput() {
  auto clickedButtons = context->context->t_engine->pad.getClicked();

  if (clickedButtons.DpadLeft) {
    if (isMoving) return;
    startMoving(true);
  } else if (clickedButtons.DpadRight) {
    if (isMoving) return;
    startMoving(false);
  }

  if (clickedButtons.Triangle) {
    context->playClickSound();
    backToMainMenu();
  } else if (clickedButtons.Cross) {
    context->playClickSound();
    saveSkin();
  }
}

void ScreenSkinSelection::backToMainMenu() {
  context->setScreen(new ScreenMain(context));
}

void ScreenSkinSelection::getAvailableSkins() {
  std::string pathPrefix = "textures/skin/";
  std::string fullPath = FileUtils::fromCwd(pathPrefix);
  std::vector<UtilDirectory> fileList = Utils::listDir(fullPath);

  u8 tempId = 1;
  for (size_t i = 0; i < fileList.size(); i++) {
    const UtilDirectory dir = fileList.at(i);
    const std::string fileExtension =
        FileUtils::getExtensionOfFilename(dir.name);

    if (strncmp(fileExtension.c_str(), "png", 3) == 0) {
      SkinInfoModel model;

      model.id = tempId++;
      model.name = FileUtils::getFilenameWithoutExtension(dir.name);
      model.path = std::string(fullPath).append(dir.name);

      skins.push_back(model);

      if (model.name.compare(g_settings.skin.c_str()) == 0) {
        selectedSkin = model;
      }
    }
  }

  if (selectedSkin.id <= 0) selectedSkin = skins[0];
}

void ScreenSkinSelection::selectPreviousSkin() {
  u16 idx = 0;
  for (size_t i = 0; i < skins.size(); i++)
    if (skins[i].id == selectedSkin.id) idx = i;

  if (idx == 0)
    selectedSkin = skins[skins.size() - 1];
  else
    selectedSkin = skins[idx - 1];
}

void ScreenSkinSelection::selectNextSkin() {
  u16 idx = 0;
  for (size_t i = 0; i < skins.size(); i++)
    if (skins[i].id == selectedSkin.id) idx = i;

  if (idx == skins.size() - 1)
    selectedSkin = skins[0];
  else
    selectedSkin = skins[idx + 1];
}

void ScreenSkinSelection::saveSkin() {
  g_settings.skin = selectedSkin.name;
  SettingsManager::Save();
  backToMainMenu();
}

void ScreenSkinSelection::loadSkinTextures() {
  TextureRepository* textureRepo = &t_renderer->getTextureRepository();

  const u8 index = selectedSkin.id - 1;
  const u8 prevIndex = index - 1 < 0 ? skins.size() - 1 : index - 1;
  const u8 nextIndex = (index + 1) % (skins.size());

  textures[0] = textureRepo->add(skins[prevIndex].path);
  textures[1] = textureRepo->add(skins[index].path);
  textures[2] = textureRepo->add(skins[nextIndex].path);
}

void ScreenSkinSelection::unloadSkinTextures() {
  TextureRepository* textureRepo = &t_renderer->getTextureRepository();

  if (textures[0]) textureRepo->free(textures[0]->id);
  if (textures[1]) textureRepo->free(textures[1]->id);
  if (textures[2]) textureRepo->free(textures[2]->id);
}

void ScreenSkinSelection::loadModels() {
  // Load animation frames
  ObjLoaderOptions options;
  options.scale = 3.5F;
  options.flipUVs = true;
  options.animation.count = 1;

  // Load frames used for animation
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

  std::vector<u8> standStillSequence = {0, 1};
  AnimationOptions idleAnimation;
  idleAnimation.animationId = IDLE_ANIMATION;
  idleAnimation.framesIndices = standStillSequence;
  idleAnimation.durationInMs = 1000.0f;
  idleAnimation.loop = true;
  idleAnimation.wrapFrames = false;

  // Create the mannequins
  for (size_t i = 0; i < mannequins.size(); i++) {
    mannequins[i] = std::make_unique<Mannequin>();
    mannequins[i]->setFrames(rawFrames, 2);
    mannequins[i]->addAnimation(idleAnimation);
    mannequins[i]->setAnimation(IDLE_ANIMATION);
  }

  // The middle is larger
  mannequins[0]->scale.scale(defaultScale[0]);
  mannequins[1]->scale.scale(defaultScale[1]);
  mannequins[2]->scale.scale(defaultScale[2]);

  mannequins[0]->rotation.rotateY(defaultRotation[0]);
  mannequins[1]->rotation.rotateY(defaultRotation[1]);
  mannequins[2]->rotation.rotateY(defaultRotation[2]);

  mannequins[0]->translation.translate(defaultPositions[0]);
  mannequins[1]->translation.translate(defaultPositions[1]);
  mannequins[2]->translation.translate(defaultPositions[2]);
}

void ScreenSkinSelection::startMoving(u8 _isMovingForward) {
  isMoving = true;
  isMovingForward = _isMovingForward;

  if (isMovingForward) {
    startRotation = defaultRotation;
    endRotation = {
        _90DEGINRAD,
        _90DEGINRAD - 0.2f,
        _90DEGINRAD + 0.2f,
    };

    startScale = defaultScale;
    endScale = {
        1.35f,
        1.0f,
        1.0f,
    };

    startPositions = defaultPositions;
    endPositions = {
        Vec4(25.0f, 16.5F, 0.0f),
        Vec4(25.0f, 16.5F, 12.0f),
        Vec4(25.0f, 16.5F, 24.0f),
    };

  } else {
    startRotation = defaultRotation;
    endRotation = {
        _90DEGINRAD - 0.2f,
        _90DEGINRAD + 0.2f,
        _90DEGINRAD,
    };

    startScale = defaultScale;
    endScale = {
        1.0f,
        1.0f,
        1.35f,
    };

    startPositions = defaultPositions;
    endPositions = {
        Vec4(25.0f, 16.5F, -24.0f),
        Vec4(25.0f, 16.5F, -12.0f),
        Vec4(25.0f, 16.5F, 0.0f),
    };
  }
}

void ScreenSkinSelection::moveForward(const float& deltaTime) {
  calcLerp(deltaTime);

  if (interpolation == 1.0f) {
    stopMoving();
    selectPreviousSkin();
    unloadSkinTextures();
    loadSkinTextures();

    for (size_t i = 0; i < mannequins.size(); i++) {
      mannequins[i]->rotation.identity();
      mannequins[i]->rotation.rotateY(defaultRotation[i]);

      mannequins[i]->scale.identity();
      mannequins[i]->scale.scale(defaultScale[i]);

      mannequins[i]->translation.identity();
      mannequins[i]->translation.translate(defaultPositions[i]);
    }
  }
}

void ScreenSkinSelection::moveBackward(const float& deltaTime) {
  calcLerp(deltaTime);

  if (interpolation == 1.0f) {
    stopMoving();
    selectNextSkin();
    unloadSkinTextures();
    loadSkinTextures();

    for (size_t i = 0; i < mannequins.size(); i++) {
      mannequins[i]->rotation.identity();
      mannequins[i]->rotation.rotateY(defaultRotation[i]);

      mannequins[i]->scale.identity();
      mannequins[i]->scale.scale(defaultScale[i]);

      mannequins[i]->translation.identity();
      mannequins[i]->translation.translate(defaultPositions[i]);
    }
  }
}

void ScreenSkinSelection::stopMoving() {
  isMoving = false;
  interpolation = 0.0f;
}

void ScreenSkinSelection::calcLerp(const float& deltaTime) {
  float nextVal = interpolation + (TRANSITION_SPEED * deltaTime);
  if (nextVal > 1.0f) nextVal = 1.0f;
  interpolation = nextVal;

  // Apply ease-out easing to interpolation for a quick start and smooth finish
  // easeOutQuad: t * (2 - t)
  const float t = interpolation;
  const float easedT = t * (2.0f - t);

  // rotation
  tempRotation[0] = Utils::lerp(startRotation[0], endRotation[0], easedT);
  tempRotation[1] = Utils::lerp(startRotation[1], endRotation[1], easedT);
  tempRotation[2] = Utils::lerp(startRotation[2], endRotation[2], easedT);

  // scale
  tempScale[0] = Utils::lerp(startScale[0], endScale[0], easedT);
  tempScale[1] = Utils::lerp(startScale[1], endScale[1], easedT);
  tempScale[2] = Utils::lerp(startScale[2], endScale[2], easedT);

  // position
  tempPositions[0].lerp(startPositions[0], endPositions[0], easedT);
  tempPositions[1].lerp(startPositions[1], endPositions[1], easedT);
  tempPositions[2].lerp(startPositions[2], endPositions[2], easedT);

  for (size_t i = 0; i < mannequins.size(); i++) {
    mannequins[i]->rotation.identity();
    mannequins[i]->rotation.rotateY(tempRotation[i]);

    mannequins[i]->scale.identity();
    mannequins[i]->scale.scale(tempScale[i]);

    mannequins[i]->translation.identity();
    mannequins[i]->translation.translate(tempPositions[i]);
  }
}
