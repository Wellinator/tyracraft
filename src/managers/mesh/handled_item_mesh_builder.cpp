#include "managers/mesh/handled_item_mesh_builder.hpp"
#include "managers/mesh/cuboid/cuboid_hand_item_mesh_builder.hpp"
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

    // Cuboid
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
    // No data
    // TODO: implement item mesh
    case ItemId::empty:
    case ItemId::poppy_flower:
    case ItemId::dandelion_flower:
    case ItemId::water_bucket:
    case ItemId::lava_bucket:
      break;

    case ItemId::torch:
      TorchHandItemMeshBuilder_loadLightData(t_block, t_vertices_colors,
                                             t_worldLightModel);
      break;

    // Cuboid
    default:
      CuboidHandItemMeshBuilder_loadLightData(t_block, t_vertices_colors,
                                              t_worldLightModel);
      break;
  }
}