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
};

PlayerRenderArmPip::~PlayerRenderArmPip() { unloadItemDrawData(); };

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
  t_render->renderer3D.usePipeline(&this->t_player->dynpip);
  this->t_player->dynpip.render(this->t_player->armMesh.get(),
                                &this->t_player->armDynpipOptions);
}
void PlayerRenderArmPip::renderItem(Renderer* t_render) {
  if (vertices.size() > 0) {
    t_render->renderer3D.usePipeline(stapip);
    stapip.core.render(&bag);
  }
}

void PlayerRenderArmPip::loadItemDrawData() {
  ItemId activeItemType = t_player->getSelectedInventoryItemType();
  Item* t_item = t_player->t_itemRepository->getItemById(activeItemType);
  BlockInfo* tempInfo =
      t_player->t_blockManager->getBlockInfoByType(t_item->blockId);

  if (tempInfo) {
    Block block = Block(tempInfo);

    block.visibleFaces = 0x111111;
    block.visibleFacesCount = 6;

    rawMatrix.identity();
    block.model.identity();

    if (activeItemType == ItemId::torch) {
      rawMatrix.scale(BLOCK_SIZE / 2.5F);
      rawMatrix.rotateY(Tyra::Math::ANG2RAD * -50);
      rawMatrix.rotateX(Tyra::Math::ANG2RAD * -16);
      rawMatrix.translate(Vec4(5.7F, -4.6F, -10.0F));
    } else {
      rawMatrix.scale(BLOCK_SIZE / 3.7F);
      rawMatrix.rotateY(Tyra::Math::ANG2RAD * -25);
      rawMatrix.rotateX(Tyra::Math::ANG2RAD * 10);
      rawMatrix.translate(Vec4(4.5F, -5.0F, -10.0F)); 
    }

    HandledItemMeshBuilder_BuildMesh(t_item, &block, &vertices, &verticesColors,
                                     &uvMap, t_player->t_worldLightModel);

    textureBag.texture = t_player->t_blockManager->getBlocksTexture();
    textureBag.coordinates = uvMap.data();

    infoBag.model = &rawMatrix;

    infoBag.transformationType = Tyra::PipelineTransformationType::TyraMP;
    infoBag.textureMappingType = Tyra::PipelineTextureMappingType::TyraNearest;
    infoBag.shadingType = Tyra::PipelineShadingType::TyraShadingGouraud;
    infoBag.blendingEnabled = true;
    infoBag.antiAliasingEnabled = false;
    infoBag.fullClipChecks = false;
    infoBag.frustumCulling =
        Tyra::PipelineInfoBagFrustumCulling::PipelineInfoBagFrustumCulling_None;

    colorBag.many = verticesColors.data();

    bag.count = vertices.size();
    bag.vertices = vertices.data();
    bag.color = &colorBag;
    bag.info = &infoBag;
    bag.texture = &textureBag;
  }
};

void PlayerRenderArmPip::unloadItemDrawData() {
  vertices.clear();
  vertices.shrink_to_fit();
  verticesColors.clear();
  verticesColors.shrink_to_fit();
  uvMap.clear();
  uvMap.shrink_to_fit();
};
