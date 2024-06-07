#include "managers/mesh/mesh_builder.hpp"
#include "managers/mesh/cuboid/cuboid_mesh_builder.hpp"
#include "managers/mesh/slab/slab_mesh_builder.hpp"
#include "managers/mesh/crossed/crossed_mesh_builder.hpp"
#include "managers/mesh/water/water_mesh_builder.hpp"
#include "managers/mesh/lava/lava_mesh_builder.hpp"
#include "managers/mesh/torch/torch_mesh_builder.hpp"
#include "managers/light_manager.hpp"
#include "managers/block/vertex_block_data.hpp"

std::map<Blocks,
         std::function<void(
             Block* t_block, std::vector<Vec4>* t_vertices,
             std::vector<Color>* t_vertices_colors, std::vector<Vec4>* t_uv_map,
             WorldLightModel* t_worldLightModel, LevelMap* t_terrain)>>
    builders;

std::map<Blocks, std::function<void(
                     Block* t_block, std::vector<Color>* t_vertices_colors,
                     WorldLightModel* t_worldLightModel, LevelMap* t_terrain)>>
    light_builders;

void MeshBuilder_RegisterBuilders() {
  builders = {
      // Vegetation
      {Blocks::GRASS, CrossedMeshBuilder_GenerateMesh},
      {Blocks::POPPY_FLOWER, CrossedMeshBuilder_GenerateMesh},
      {Blocks::DANDELION_FLOWER, CrossedMeshBuilder_GenerateMesh},

      // Liquids
      {Blocks::WATER_BLOCK, WaterMeshBuilder_GenerateMesh},
      {Blocks::LAVA_BLOCK, LavaMeshBuilder_GenerateMesh},

      // Items
      {Blocks::TORCH, TorchMeshBuilder_GenerateMesh},

      // Slabs
      {Blocks::STONE_SLAB, SlabMeshBuilder_GenerateMesh},
      {Blocks::BRICKS_SLAB, SlabMeshBuilder_GenerateMesh},
      {Blocks::OAK_PLANKS_SLAB, SlabMeshBuilder_GenerateMesh},
      {Blocks::SPRUCE_PLANKS_SLAB, SlabMeshBuilder_GenerateMesh},
      {Blocks::BIRCH_PLANKS_SLAB, SlabMeshBuilder_GenerateMesh},
      {Blocks::ACACIA_PLANKS_SLAB, SlabMeshBuilder_GenerateMesh},
      {Blocks::STONE_BRICK_SLAB, SlabMeshBuilder_GenerateMesh},
      {Blocks::CRACKED_STONE_BRICKS_SLAB, SlabMeshBuilder_GenerateMesh},
      {Blocks::MOSSY_STONE_BRICKS_SLAB, SlabMeshBuilder_GenerateMesh},

      // Cuboids
      {Blocks::STONE_BLOCK, CuboidMeshBuilder_GenerateMesh},
      {Blocks::GRASS_BLOCK, CuboidMeshBuilder_GenerateMesh},
      {Blocks::DIRTY_BLOCK, CuboidMeshBuilder_GenerateMesh},
      {Blocks::WATER_BLOCK, CuboidMeshBuilder_GenerateMesh},
      {Blocks::BEDROCK_BLOCK, CuboidMeshBuilder_GenerateMesh},
      {Blocks::SAND_BLOCK, CuboidMeshBuilder_GenerateMesh},
      {Blocks::GLASS_BLOCK, CuboidMeshBuilder_GenerateMesh},
      {Blocks::BRICKS_BLOCK, CuboidMeshBuilder_GenerateMesh},
      {Blocks::GRAVEL_BLOCK, CuboidMeshBuilder_GenerateMesh},
      {Blocks::GOLD_ORE_BLOCK, CuboidMeshBuilder_GenerateMesh},
      {Blocks::IRON_ORE_BLOCK, CuboidMeshBuilder_GenerateMesh},
      {Blocks::COAL_ORE_BLOCK, CuboidMeshBuilder_GenerateMesh},
      {Blocks::DIAMOND_ORE_BLOCK, CuboidMeshBuilder_GenerateMesh},
      {Blocks::REDSTONE_ORE_BLOCK, CuboidMeshBuilder_GenerateMesh},
      {Blocks::EMERALD_ORE_BLOCK, CuboidMeshBuilder_GenerateMesh},
      {Blocks::OAK_PLANKS_BLOCK, CuboidMeshBuilder_GenerateMesh},
      {Blocks::SPRUCE_PLANKS_BLOCK, CuboidMeshBuilder_GenerateMesh},
      {Blocks::BIRCH_PLANKS_BLOCK, CuboidMeshBuilder_GenerateMesh},
      {Blocks::ACACIA_PLANKS_BLOCK, CuboidMeshBuilder_GenerateMesh},
      {Blocks::STONE_BRICK_BLOCK, CuboidMeshBuilder_GenerateMesh},
      {Blocks::CRACKED_STONE_BRICKS_BLOCK, CuboidMeshBuilder_GenerateMesh},
      {Blocks::MOSSY_STONE_BRICKS_BLOCK, CuboidMeshBuilder_GenerateMesh},
      {Blocks::CHISELED_STONE_BRICKS_BLOCK, CuboidMeshBuilder_GenerateMesh},
      {Blocks::YELLOW_WOOL, CuboidMeshBuilder_GenerateMesh},
      {Blocks::BLUE_WOOL, CuboidMeshBuilder_GenerateMesh},
      {Blocks::GREEN_WOOL, CuboidMeshBuilder_GenerateMesh},
      {Blocks::ORANGE_WOOL, CuboidMeshBuilder_GenerateMesh},
      {Blocks::PURPLE_WOOL, CuboidMeshBuilder_GenerateMesh},
      {Blocks::RED_WOOL, CuboidMeshBuilder_GenerateMesh},
      {Blocks::WHITE_WOOL, CuboidMeshBuilder_GenerateMesh},
      {Blocks::BLACK_WOOL, CuboidMeshBuilder_GenerateMesh},
      {Blocks::GLOWSTONE_BLOCK, CuboidMeshBuilder_GenerateMesh},
      {Blocks::JACK_O_LANTERN_BLOCK, CuboidMeshBuilder_GenerateMesh},
      {Blocks::PUMPKIN_BLOCK, CuboidMeshBuilder_GenerateMesh},
      {Blocks::LAVA_BLOCK, CuboidMeshBuilder_GenerateMesh},
      {Blocks::OAK_LOG_BLOCK, CuboidMeshBuilder_GenerateMesh},
      {Blocks::OAK_LEAVES_BLOCK, CuboidMeshBuilder_GenerateMesh},
      {Blocks::BIRCH_LOG_BLOCK, CuboidMeshBuilder_GenerateMesh},
      {Blocks::BIRCH_LEAVES_BLOCK, CuboidMeshBuilder_GenerateMesh},
  };

  light_builders = {
      // Vegetation
      {Blocks::GRASS, CrossedMeshBuilder_loadCroosedLightData},
      {Blocks::POPPY_FLOWER, CrossedMeshBuilder_loadCroosedLightData},
      {Blocks::DANDELION_FLOWER, CrossedMeshBuilder_loadCroosedLightData},

      // Liquids
      {Blocks::WATER_BLOCK, WaterMeshBuilder_loadLightData},
      {Blocks::LAVA_BLOCK, LavaMeshBuilder_loadLightData},

      // Items
      {Blocks::TORCH, TorchMeshBuilder_loadLightData},

      // Slabs
      {Blocks::STONE_SLAB, SlabMeshBuilder_loadLightData},
      {Blocks::BRICKS_SLAB, SlabMeshBuilder_loadLightData},
      {Blocks::OAK_PLANKS_SLAB, SlabMeshBuilder_loadLightData},
      {Blocks::SPRUCE_PLANKS_SLAB, SlabMeshBuilder_loadLightData},
      {Blocks::BIRCH_PLANKS_SLAB, SlabMeshBuilder_loadLightData},
      {Blocks::ACACIA_PLANKS_SLAB, SlabMeshBuilder_loadLightData},
      {Blocks::STONE_BRICK_SLAB, SlabMeshBuilder_loadLightData},
      {Blocks::CRACKED_STONE_BRICKS_SLAB, SlabMeshBuilder_loadLightData},
      {Blocks::MOSSY_STONE_BRICKS_SLAB, SlabMeshBuilder_loadLightData},

      // Cuboids
      {Blocks::STONE_BLOCK, CuboidMeshBuilder_loadLightData},
      {Blocks::GRASS_BLOCK, CuboidMeshBuilder_loadLightData},
      {Blocks::DIRTY_BLOCK, CuboidMeshBuilder_loadLightData},
      {Blocks::WATER_BLOCK, CuboidMeshBuilder_loadLightData},
      {Blocks::BEDROCK_BLOCK, CuboidMeshBuilder_loadLightData},
      {Blocks::SAND_BLOCK, CuboidMeshBuilder_loadLightData},
      {Blocks::GLASS_BLOCK, CuboidMeshBuilder_loadLightData},
      {Blocks::BRICKS_BLOCK, CuboidMeshBuilder_loadLightData},
      {Blocks::GRAVEL_BLOCK, CuboidMeshBuilder_loadLightData},
      {Blocks::GOLD_ORE_BLOCK, CuboidMeshBuilder_loadLightData},
      {Blocks::IRON_ORE_BLOCK, CuboidMeshBuilder_loadLightData},
      {Blocks::COAL_ORE_BLOCK, CuboidMeshBuilder_loadLightData},
      {Blocks::DIAMOND_ORE_BLOCK, CuboidMeshBuilder_loadLightData},
      {Blocks::REDSTONE_ORE_BLOCK, CuboidMeshBuilder_loadLightData},
      {Blocks::EMERALD_ORE_BLOCK, CuboidMeshBuilder_loadLightData},
      {Blocks::OAK_PLANKS_BLOCK, CuboidMeshBuilder_loadLightData},
      {Blocks::SPRUCE_PLANKS_BLOCK, CuboidMeshBuilder_loadLightData},
      {Blocks::BIRCH_PLANKS_BLOCK, CuboidMeshBuilder_loadLightData},
      {Blocks::ACACIA_PLANKS_BLOCK, CuboidMeshBuilder_loadLightData},
      {Blocks::STONE_BRICK_BLOCK, CuboidMeshBuilder_loadLightData},
      {Blocks::CRACKED_STONE_BRICKS_BLOCK, CuboidMeshBuilder_loadLightData},
      {Blocks::MOSSY_STONE_BRICKS_BLOCK, CuboidMeshBuilder_loadLightData},
      {Blocks::CHISELED_STONE_BRICKS_BLOCK, CuboidMeshBuilder_loadLightData},
      {Blocks::YELLOW_WOOL, CuboidMeshBuilder_loadLightData},
      {Blocks::BLUE_WOOL, CuboidMeshBuilder_loadLightData},
      {Blocks::GREEN_WOOL, CuboidMeshBuilder_loadLightData},
      {Blocks::ORANGE_WOOL, CuboidMeshBuilder_loadLightData},
      {Blocks::PURPLE_WOOL, CuboidMeshBuilder_loadLightData},
      {Blocks::RED_WOOL, CuboidMeshBuilder_loadLightData},
      {Blocks::WHITE_WOOL, CuboidMeshBuilder_loadLightData},
      {Blocks::BLACK_WOOL, CuboidMeshBuilder_loadLightData},
      {Blocks::GLOWSTONE_BLOCK, CuboidMeshBuilder_loadLightData},
      {Blocks::JACK_O_LANTERN_BLOCK, CuboidMeshBuilder_loadLightData},
      {Blocks::PUMPKIN_BLOCK, CuboidMeshBuilder_loadLightData},
      {Blocks::LAVA_BLOCK, CuboidMeshBuilder_loadLightData},
      {Blocks::OAK_LOG_BLOCK, CuboidMeshBuilder_loadLightData},
      {Blocks::OAK_LEAVES_BLOCK, CuboidMeshBuilder_loadLightData},
      {Blocks::BIRCH_LOG_BLOCK, CuboidMeshBuilder_loadLightData},
      {Blocks::BIRCH_LEAVES_BLOCK, CuboidMeshBuilder_loadLightData},
  };
};

void MeshBuilder_UnregisterBuilders() {
  builders.clear();
  light_builders.clear();
};

void MeshBuilder_BuildMesh(Block* t_block, std::vector<Vec4>* t_vertices,
                           std::vector<Color>* t_vertices_colors,
                           std::vector<Vec4>* t_uv_map,
                           WorldLightModel* t_worldLightModel,
                           LevelMap* t_terrain) {
  builders[t_block->type](t_block, t_vertices, t_vertices_colors, t_uv_map,
                          t_worldLightModel, t_terrain);
}

void MeshBuilder_BuildLightData(Block* t_block,
                                std::vector<Color>* t_vertices_colors,
                                WorldLightModel* t_worldLightModel,
                                LevelMap* t_terrain) {
  light_builders[t_block->type](t_block, t_vertices_colors, t_worldLightModel,
                                t_terrain);
}