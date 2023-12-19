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

    // case ItemId::torch:
    //   TorchMeshBuilder_GenerateMesh(t_item, t_vertices, t_vertices_colors,
    //                                 t_uv_map, t_worldLightModel);
    //   break;

    // // Cuboid
    // case ItemId::acacia_planks:
    // case ItemId::birch_log:
    // case ItemId::dirt:
    // case ItemId::gravel:
    // case ItemId::sand:
    // case ItemId::stone:
    // case ItemId::bricks:
    // case ItemId::glass:
    // case ItemId::pumpkin:
    // case ItemId::oak_planks:
    // case ItemId::spruce_planks:
    // case ItemId::birch_planks:
    // case ItemId::oak_log:
    // case ItemId::stone_brick:
    // case ItemId::cracked_stone_bricks:
    // case ItemId::mossy_stone_bricks:
    // case ItemId::chiseled_stone_bricks:
    // case ItemId::coal_ore_block:
    // case ItemId::diamond_ore_block:
    // case ItemId::iron_ore_block:
    // case ItemId::gold_ore_block:
    // case ItemId::redstone_ore_block:
    // case ItemId::emerald_ore_block:
    // case ItemId::yellow_concrete:
    // case ItemId::blue_concrete:
    // case ItemId::green_concrete:
    // case ItemId::orange_concrete:
    // case ItemId::purple_concrete:
    // case ItemId::red_concrete:
    // case ItemId::white_concrete:
    // case ItemId::black_concrete:
    // case ItemId::glowstone:
    // case ItemId::jack_o_lantern:
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

    rawMatrix.scale(BLOCK_SIZE / 3.5F);
    rawMatrix.rotateY(Tyra::Math::ANG2RAD * -25);
    rawMatrix.rotateX(Tyra::Math::ANG2RAD * 10);
    rawMatrix.translate(Vec4(4.5F, -5.0F, -10.0F));

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
