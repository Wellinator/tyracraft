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

  unloadModels();
  unloadSkinTextures();
}

void ScreenSkinSelection::update(const float& deltaTime) {
  handleInput();
  if (isMoving)
    isMovingForward ? moveForward(deltaTime) : moveBackward(deltaTime);
  for (size_t i = 0; i < models.size(); i++) models[i].get()->update();
}

void ScreenSkinSelection::render() {
  t_renderer->renderer3D.usePipeline(&dynpip);
  for (size_t i = 0; i < models.size(); i++)
    dynpip.render(models[i].get(), &dynpipOptions);

  t_renderer->renderer2D.render(btnCross);
  FontManager_printText(Label_Select, 35, 407);

  t_renderer->renderer2D.render(btnTriangle);
  FontManager_printText(Label_Back, 160, 407);

  if (!isMoving) {
    t_renderer->renderer2D.render(arrowLeft);
    t_renderer->renderer2D.render(arrowRight);

    FontOptions fontOptions;
    fontOptions.scale = 0.9f;
    fontOptions.position.set(Vec2(250.0F, 365.0F));
    fontOptions.alignment = TextAlignment::Center;
    fontOptions.color.set(Tyra::Color(255, 255, 255));

    FontManager_printText(selectedSkin.name, fontOptions);
  }
}

void ScreenSkinSelection::init() {
  dynpip.setRenderer(&t_renderer->core);

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
  dynpipOptions.antiAliasingEnabled = true;
  dynpipOptions.frustumCulling =
      Tyra::PipelineFrustumCulling::PipelineFrustumCulling_Precise;
  dynpipOptions.shadingType = Tyra::PipelineShadingType::TyraShadingFlat;
  dynpipOptions.textureMappingType =
      Tyra::PipelineTextureMappingType::TyraNearest;

  ObjLoaderOptions options;
  options.scale = 3.5F;
  options.flipUVs = true;
  options.animation.count = 2;

  auto data = ObjLoader::load(
      FileUtils::fromCwd("models/player/stand_still/player.obj"), options);
  data.get()->loadNormals = false;
  baseMesh = std::make_unique<DynamicMesh>(data.get());

  for (size_t i = 0; i < models.size(); i++) {
    models[i] = std::make_unique<DynamicMesh>(*baseMesh.get());
    models[i]->rotation.identity();
    models[i]->scale.identity();
    models[i]->translation.identity();
    models[i]->animation.loop = true;
    models[i]->animation.setSequence(standStillSequence);
    models[i]->animation.speed = 0.005F;

    auto& materials = models[i].get()->materials;
    for (size_t j = 0; j < materials.size(); j++)
      textures[i]->addLink(materials[j]->id);
  }

  models[1]->scale.scale(1.5f);

  models[0]->rotation.rotateY(defaultRotation[0]);
  models[1]->rotation.rotateY(defaultRotation[1]);
  models[2]->rotation.rotateY(defaultRotation[2]);

  models[0]->translation.translate(defaultPositions[0]);
  models[1]->translation.translate(defaultPositions[1]);
  models[2]->translation.translate(defaultPositions[2]);
}

void ScreenSkinSelection::unloadModels() {
  for (size_t i = 0; i < models.size(); i++) {
    for (size_t j = 0; j < models[i]->materials.size(); j++) {
      textures[i]->removeLinkById(models[i]->materials[j]->id);
    }
  }
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
        1.5f,
        1.0f,
        1.0f,
    };

    startPositions = defaultPositions;
    endPositions = {
        Vec4(25.0f, 15.0F, 0.0f),
        Vec4(25.0f, 15.0F, 12.0f),
        Vec4(25.0f, 15.0F, 24.0f),
    };

  } else {
    startRotation = defaultRotation;
    endRotation = {
        _90DEGINRAD + 0.2f,
        _90DEGINRAD - 0.2f,
        _90DEGINRAD,
    };

    startScale = defaultScale;
    endScale = {
        1.0f,
        1.0f,
        1.5f,
    };

    startPositions = defaultPositions;
    endPositions = {
        Vec4(25.0f, 15.0F, -24.0f),
        Vec4(25.0f, 15.0F, -12.0f),
        Vec4(25.0f, 15.0F, 0.0f),
    };
  }
}

void ScreenSkinSelection::moveForward(const float& deltaTime) {
  calcLerp(deltaTime);

  if (interpolation == 1.0f) {
    TextureRepository* textureRepo = &t_renderer->getTextureRepository();

    stopMoving();
    selectPreviousSkin();

    textureRepo->freeByMesh(models[0].get());
    textureRepo->freeByMesh(models[1].get());
    textureRepo->freeByMesh(models[2].get());

    loadSkinTextures();

    for (size_t i = 0; i < models.size(); i++) {
      models[i].reset();

      models[i] = std::make_unique<DynamicMesh>(*baseMesh.get());

      models[i]->animation.loop = true;
      models[i]->animation.setSequence(standStillSequence);
      models[i]->animation.speed = 0.005F;

      models[i]->rotation.identity();
      models[i]->rotation.rotateY(defaultRotation[i]);

      models[i]->scale.identity();
      models[i]->scale.scale(defaultScale[i]);

      models[i]->translation.identity();
      models[i]->translation.translate(defaultPositions[i]);

      auto& materials = models[i].get()->materials;
      for (size_t j = 0; j < materials.size(); j++)
        textures[i]->addLink(materials[j]->id);
    }
  }
}

void ScreenSkinSelection::moveBackward(const float& deltaTime) {
  calcLerp(deltaTime);

  if (interpolation == 1.0f) {
    TextureRepository* textureRepo = &t_renderer->getTextureRepository();

    stopMoving();
    selectNextSkin();

    textureRepo->freeByMesh(models[0].get());
    textureRepo->freeByMesh(models[1].get());
    textureRepo->freeByMesh(models[2].get());

    loadSkinTextures();

    for (size_t i = 0; i < models.size(); i++) {
      models[i].reset();

      models[i] = std::make_unique<DynamicMesh>(*baseMesh.get());

      models[i]->animation.loop = true;
      models[i]->animation.setSequence(standStillSequence);
      models[i]->animation.speed = 0.005F;

      models[i]->rotation.identity();
      models[i]->rotation.rotateY(defaultRotation[i]);

      models[i]->scale.identity();
      models[i]->scale.scale(defaultScale[i]);

      models[i]->translation.identity();
      models[i]->translation.translate(defaultPositions[i]);

      auto& materials = models[i].get()->materials;
      for (size_t j = 0; j < materials.size(); j++)
        textures[i]->addLink(materials[j]->id);
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

  // rotation
  tempRotation[0] =
      Utils::lerp(startRotation[0], endRotation[0], interpolation);
  tempRotation[1] =
      Utils::lerp(startRotation[1], endRotation[1], interpolation);
  tempRotation[2] =
      Utils::lerp(startRotation[2], endRotation[2], interpolation);

  // scale
  tempScale[0] = Utils::lerp(startScale[0], endScale[0], interpolation);
  tempScale[1] = Utils::lerp(startScale[1], endScale[1], interpolation);
  tempScale[2] = Utils::lerp(startScale[2], endScale[2], interpolation);

  // position
  tempPositions[0].lerp(startPositions[0], endPositions[0], interpolation);
  tempPositions[1].lerp(startPositions[1], endPositions[1], interpolation);
  tempPositions[2].lerp(startPositions[2], endPositions[2], interpolation);

  for (size_t i = 0; i < models.size(); i++) {
    models[i]->rotation.identity();
    models[i]->rotation.rotateY(tempRotation[i]);

    models[i]->scale.identity();
    models[i]->scale.scale(tempScale[i]);

    models[i]->translation.identity();
    models[i]->translation.translate(tempPositions[i]);
  }
}
