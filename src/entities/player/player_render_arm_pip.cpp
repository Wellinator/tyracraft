#include "constants.hpp"
#include "entities/player/player_render_arm_pip.hpp"
#include "managers/block/vertex_block_data.hpp"
#include "models/block_info_model.hpp"
#include "entities/Block.hpp"
#include "managers/block/block_info_repository.hpp"
#include "managers/mesh/handled_item_mesh_builder.hpp"

PlayerRenderArmPip::PlayerRenderArmPip(Player* t_player)
    : PlayerRenderPip(t_player) {
  stapip.setRenderer(&t_player->t_renderer->core);
  loadItemDrawData();
};

PlayerRenderArmPip::~PlayerRenderArmPip() { unloadItemDrawData(); };


// changed structure & fixed showing up new block in inventory
void PlayerRenderArmPip::update(const float& deltaTime, Camera* t_camera) {
  ItemId currentItem = t_player->getSelectedInventoryItemType();

  if (currentItem != lastSelectedItem) {
    loadItemDrawData();
    lastSelectedItem = currentItem;
  }

  colorBag.single = t_player->getBaseColorAtPlayerPos();

  if (is_playing_break_animation || t_player->isBreaking) {
    updateBreakAnimation(deltaTime);
  } else if (is_playing_put_animation || t_player->isPuting) {
    updatePutAnimation(deltaTime);
  } else if (t_player->isMoving) {
    updateWalkAnimation(t_camera);
  }
};

void PlayerRenderArmPip::render(Renderer* t_render) {
  const ItemId itemId = this->t_player->getSelectedInventoryItemType();

  switch (itemId) {
      // No data
    case ItemId::empty:
      renderArm(t_render);
      break;

      // TODO: implement item mesh
    case ItemId::poppy_flower:
    case ItemId::dandelion_flower:
    case ItemId::water_bucket:
    case ItemId::lava_bucket:
      break;

    // Items with draw data
    default:
      renderItem(t_render);
      break;
  }
}

void PlayerRenderArmPip::renderArm(Renderer* t_render) {
  t_render->renderer3D.usePipeline(stapip);
  stapip.core.render(&bag);
}

void PlayerRenderArmPip::renderItem(Renderer* t_render) {
  if (bag.count > 0) {
    t_render->renderer3D.usePipeline(stapip);
    stapip.core.render(&bag);
  }
}

void PlayerRenderArmPip::loadItemDrawData() {
  ItemId activeItemType = t_player->getSelectedInventoryItemType();

  if (activeItemType == ItemId::empty) {
    scale = armScale;
    rotation.set(armRot);
    translation.set(armPos);

    rawMatrix.identity();
    rawMatrix.rotate(rotation);
    rawMatrix.scale(scale);
    rawMatrix.translate(translation);

    textureBag.texture = t_player->getPlayerTexture();
    textureBag.coordinates = armUVMap.data();

    infoBag.model = &rawMatrix;

    infoBag.transformationType = Tyra::PipelineTransformationType::TyraMP;
    infoBag.textureMappingType = Tyra::PipelineTextureMappingType::TyraNearest;
    infoBag.shadingType = Tyra::PipelineShadingType::TyraShadingFlat;
    infoBag.blendingEnabled = true;
    infoBag.antiAliasingEnabled = false;
    infoBag.fullClipChecks = false;
    infoBag.frustumCulling =
        Tyra::PipelineInfoBagFrustumCulling::PipelineInfoBagFrustumCulling_None;

    colorBag.single = t_player->getBaseColorAtPlayerPos();

    bag.count = 36;
    bag.vertices = armVertices.data();
    bag.color = &colorBag;
    bag.info = &infoBag;
    bag.texture = &textureBag;

    // set animation settings
    breaking_start_rot.set(armRot);
    breaking_end_rot.set(armRot + breaking_animation_rot_offset);
    breaking_start_pos.set(armPos);
    breaking_end_pos.set(armPos + breaking_animation_pos_offset);

    puting_start_rot.set(armRot);
    puting_end_rot.set(armRot + puting_animation_rot_offset);
    puting_start_pos.set(armPos);
    puting_end_pos.set(armPos + puting_animation_pos_offset);

  } else {
    Item* t_item = t_player->t_itemRepository->getItemById(activeItemType);
    BlockInfo* tempInfo =
        t_player->t_blockManager->getBlockInfoByType(t_item->blockId);

    if (tempInfo) {
      Block block = Block(tempInfo);

      block.visibleFaces = 0b111111;
      block.visibleFacesCount = 6;

      block.model.identity();

      if (activeItemType == ItemId::torch) {
        scale = torchScale;
        rotation.set(torchRot);
        translation.set(torchPos);

        // set animation settings
        breaking_start_rot.set(torchRot);
        breaking_end_rot.set(torchRot + breaking_animation_rot_offset);
        breaking_start_pos.set(torchPos);
        breaking_end_pos.set(torchPos + breaking_animation_pos_offset);

        puting_start_rot.set(torchRot);
        puting_end_rot.set(torchRot + puting_animation_rot_offset);
        puting_start_pos.set(torchPos);
        puting_end_pos.set(torchPos + puting_animation_pos_offset);
      } else {
        scale = blockScale;
        rotation.set(blockRot);
        translation.set(blockPos);

        // set animation settings
        breaking_start_rot.set(blockRot);
        breaking_end_rot.set(blockRot + breaking_animation_rot_offset);
        breaking_start_pos.set(blockPos);
        breaking_end_pos.set(blockPos + breaking_animation_pos_offset);

        puting_start_rot.set(blockRot);
        puting_end_rot.set(blockRot + puting_animation_rot_offset);
        puting_start_pos.set(blockPos);
        puting_end_pos.set(blockPos + puting_animation_pos_offset);
      }

      rawMatrix.identity();
      rawMatrix.rotate(rotation);
      rawMatrix.scale(scale);
      rawMatrix.translate(translation);

      HandledItemMeshBuilder_BuildMesh(t_item, &block, &vertices, nullptr,
                                       &uvMap, t_player->t_worldLightModel);

      textureBag.texture = t_player->t_blockManager->getBlocksTexture();
      textureBag.coordinates = uvMap.data();

      infoBag.model = &rawMatrix;

      infoBag.transformationType = Tyra::PipelineTransformationType::TyraMP;
      infoBag.textureMappingType =
          Tyra::PipelineTextureMappingType::TyraNearest;
      infoBag.shadingType = Tyra::PipelineShadingType::TyraShadingGouraud;
      infoBag.blendingEnabled = true;
      infoBag.antiAliasingEnabled = false;
      infoBag.fullClipChecks = false;
      infoBag.frustumCulling = Tyra::PipelineInfoBagFrustumCulling::
          PipelineInfoBagFrustumCulling_None;

      colorBag.single = t_player->getBaseColorAtPlayerPos();

      bag.count = vertices.size();
      bag.vertices = vertices.data();
      bag.color = &colorBag;
      bag.info = &infoBag;
      bag.texture = &textureBag;
    }
  }
};

void PlayerRenderArmPip::unloadItemDrawData() {
  vertices.clear();
  vertices.shrink_to_fit();
  uvMap.clear();
  uvMap.shrink_to_fit();
};

void PlayerRenderArmPip::updateWalkAnimation(Camera* t_camera) {
  const float d = t_camera->getCamTime();
  const float dH = Math::sin(d);
  const float dV = -Math::sin(Math::HALF_PI - 2 * d);
  Vec4 offset_factor = Vec4(dH, dV, 0.0f) / 2;

  Vec4 _translation =
      Vec4::getByLerp(translation + offset_factor, translation, 0.4f);

  rawMatrix.identity();
  rawMatrix.rotate(rotation);
  rawMatrix.scale(scale);
  rawMatrix.translate(_translation);
};

void PlayerRenderArmPip::updateBreakAnimation(const float& deltaTime) {
  const float d = Math::cos(animantion_time);

  interpolation = 1.0f - Utils::reRangeScale(0.0f, 1.0f, -1.0f, 1.0f, d);

  const Vec4 dy_translation = Vec4(0.0f, Math::sin(animantion_time), 0.0f);
  Vec4 _rotation =
      Vec4::getByLerp(breaking_start_rot, breaking_end_rot, interpolation);
  Vec4 _translation =
      Vec4::getByLerp(breaking_start_pos, breaking_end_pos, interpolation) +
      dy_translation;

  rawMatrix.identity();
  rawMatrix.rotate(_rotation);
  rawMatrix.scale(scale);
  rawMatrix.translate(_translation);

  animantion_time += deltaTime;

  is_playing_break_animation = animantion_time < animantion_time_limit;
  if (is_playing_break_animation) {
    animantion_time += deltaTime * animantion_speed;
  } else {
    interpolation = 0;
    animantion_time = default_animantion_time;
  }
};

void PlayerRenderArmPip::updatePutAnimation(const float& deltaTime) {
  const float d = Math::cos(animantion_time);

  interpolation = 1.0f - Utils::reRangeScale(0.0f, 1.0f, -1.0f, 1.0f, d);

  const Vec4 dy_translation = Vec4(0.0f, Math::sin(animantion_time), 0.0f);
  Vec4 _rotation =
      Vec4::getByLerp(puting_start_rot, puting_end_rot, interpolation);
  Vec4 _translation =
      Vec4::getByLerp(puting_start_pos, puting_end_pos, interpolation) +
      dy_translation;

  rawMatrix.identity();
  rawMatrix.rotate(_rotation);
  rawMatrix.scale(scale);
  rawMatrix.translate(_translation);

  animantion_time += deltaTime;

  is_playing_put_animation = animantion_time < animantion_time_limit;
  if (is_playing_put_animation) {
    animantion_time += deltaTime * animantion_speed;
  } else {
    interpolation = 0;
    animantion_time = default_animantion_time;
  }
};
