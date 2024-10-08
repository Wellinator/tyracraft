#include "managers/mesh/handled_item_mesh_builder.hpp"
#include "managers/mesh/cuboid/cuboid_hand_item_mesh_builder.hpp"
#include "managers/mesh/slab/slab_hand_item_mesh_builder.hpp"
#include "managers/mesh/torch/torch_hand_item_mesh_builder.hpp"
#include "managers/light_manager.hpp"
#include "managers/block/vertex_block_data.hpp"

void HandledItemMeshBuilder_BuildMesh(Item* t_item, Block* t_block,
                                      std::vector<Vec4>* t_vertices,
                                      std::vector<Color>* t_vertices_colors,
                                      std::vector<Vec4>* t_uv_map,
                                      WorldLightModel* t_worldLightModel) {
  switch (t_item->id) {
    // No data
    // TODO: implement item mesh
    case ItemId::empty:
    case ItemId::poppy_flower:
    case ItemId::dandelion_flower:
    case ItemId::water_bucket:
    case ItemId::lava_bucket:
      break;

    case ItemId::torch:
      TorchHandItemMeshBuilder_GenerateMesh(
          t_block, t_vertices, t_vertices_colors, t_uv_map, t_worldLightModel);
      break;

    case ItemId::stone_slab:
    case ItemId::bricks_slab:
    case ItemId::oak_slab:
    case ItemId::spruce_slab:
    case ItemId::birch_slab:
    case ItemId::acacia_slab:
    case ItemId::stone_brick_slab:
    case ItemId::cracked_stone_bricks_slab:
    case ItemId::mossy_stone_bricks_slab:
      SlabHandItemMeshBuilder_GenerateMesh(
          t_block, t_vertices, t_vertices_colors, t_uv_map, t_worldLightModel);
      break;

    // Cuboid
    default:
      CuboidHandItemMeshBuilder_GenerateMesh(
          t_block, t_vertices, t_vertices_colors, t_uv_map, t_worldLightModel);
      break;
  }
}
