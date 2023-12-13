#include "managers/mesh/mesh_builder.hpp"
#include "managers/mesh/cuboid/cuboid_mesh_builder.hpp"
#include "managers/mesh/crossed/crossed_mesh_builder.hpp"
#include "managers/mesh/water/water_mesh_builder.hpp"
#include "managers/mesh/lava/lava_mesh_builder.hpp"
#include "managers/mesh/torch/torch_mesh_builder.hpp"
#include "managers/light_manager.hpp"
#include "managers/block/vertex_block_data.hpp"

void MeshBuilder_BuildMesh(Block* t_block, std::vector<Vec4>* t_vertices,
                           std::vector<Color>* t_vertices_colors,
                           std::vector<Vec4>* t_uv_map,
                           WorldLightModel* t_worldLightModel,
                           LevelMap* t_terrain) {
  switch (t_block->type) {
    // Crossed mesh blocks
    case Blocks::GRASS:
    case Blocks::POPPY_FLOWER:
    case Blocks::DANDELION_FLOWER:
      CrossedMeshBuilder_GenerateMesh(t_block, t_vertices, t_vertices_colors,
                                      t_uv_map, t_worldLightModel, t_terrain);
      break;

    case Blocks::WATER_BLOCK:
      WaterMeshBuilder_GenerateMesh(t_block, t_vertices, t_vertices_colors,
                                    t_uv_map, t_worldLightModel, t_terrain);
      break;
    case Blocks::LAVA_BLOCK:
      LavaMeshBuilder_GenerateMesh(t_block, t_vertices, t_vertices_colors,
                                   t_uv_map, t_worldLightModel, t_terrain);
      break;
    case Blocks::TORCH:
      TorchMeshBuilder_GenerateMesh(t_block, t_vertices, t_vertices_colors,
                                    t_uv_map, t_worldLightModel, t_terrain);
      break;

    // Cuboid mesh blocks
    default:
      CuboidMeshBuilder_GenerateMesh(t_block, t_vertices, t_vertices_colors,
                                     t_uv_map, t_worldLightModel, t_terrain);
      break;
  }
}

void MeshBuilder_BuildLightData(Block* t_block,
                                std::vector<Color>* t_vertices_colors,
                                WorldLightModel* t_worldLightModel,
                                LevelMap* t_terrain) {
  switch (t_block->type) {
    // Crossed mesh blocks
    case Blocks::GRASS:
    case Blocks::POPPY_FLOWER:
    case Blocks::DANDELION_FLOWER:
      CrossedMeshBuilder_loadCroosedLightData(t_block, t_vertices_colors,
                                              t_worldLightModel, t_terrain);
      break;

    case Blocks::WATER_BLOCK:
      WaterMeshBuilder_loadLightData(t_block, t_vertices_colors,
                                     t_worldLightModel, t_terrain);
      break;
    case Blocks::LAVA_BLOCK:
      LavaMeshBuilder_loadLightData(t_block, t_vertices_colors,
                                    t_worldLightModel, t_terrain);
      break;

    case Blocks::TORCH:
      TorchMeshBuilder_loadLightData(t_block, t_vertices_colors,
                                          t_worldLightModel, t_terrain);
      break;

    // Cuboid mesh blocks
    default:
      CuboidMeshBuilder_loadLightData(t_block, t_vertices_colors,
                                      t_worldLightModel, t_terrain);
      break;
  }
}