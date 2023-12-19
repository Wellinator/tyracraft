#include "managers/mesh/handled_item_mesh_builder.hpp"
#include "managers/mesh/cuboid/cuboid_hand_item_mesh_builder.hpp"
#include "managers/light_manager.hpp"
#include "managers/block/vertex_block_data.hpp"

void HandledItemMeshBuilder_BuildMesh(Item* t_item, Block* t_block,
                                      std::vector<Vec4>* t_vertices,
                                      std::vector<Color>* t_vertices_colors,
                                      std::vector<Vec4>* t_uv_map,
                                      WorldLightModel* t_worldLightModel) {
  switch (t_item->id) {
    // case ItemId::torch:
    //   TorchMeshBuilder_GenerateMesh(t_item, t_vertices, t_vertices_colors,
    //                                 t_uv_map, t_worldLightModel);
    //   break;

    // TODO: implement item mesh
    // No data
    case ItemId::empty:
    case ItemId::poppy_flower:
    case ItemId::dandelion_flower:
    case ItemId::water_bucket:
    case ItemId::lava_bucket:
      break;

    // Cuboid
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
      CuboidHandItemMeshBuilder_GenerateMesh(
          t_block, t_vertices, t_vertices_colors, t_uv_map, t_worldLightModel);
      break;
  }
}

void HandledItemMeshBuilder_BuildLightData(
    Item* t_item, Block* t_block, std::vector<Color>* t_vertices_colors,
    WorldLightModel* t_worldLightModel) {
  switch (t_item->id) {
    // case ItemId::torch:
    //   TorchMeshBuilder_GenerateMesh(t_item, t_vertices, t_vertices_colors,
    //                                 t_uv_map, t_worldLightModel, t_terrain);
    //   break;

    // No data
    case ItemId::empty:
    // TODO: implement item mesh
    case ItemId::poppy_flower:
    case ItemId::dandelion_flower:
    case ItemId::water_bucket:
    case ItemId::lava_bucket:
      break;
    // Cuboid
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
      CuboidHandItemMeshBuilder_loadLightData(t_block, t_vertices_colors,
                                              t_worldLightModel);
      break;
  }
}