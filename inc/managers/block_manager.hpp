#pragma once

#include <vector>
#include <tamtypes.h>
#include <renderer/renderer.hpp>
#include <renderer/3d/mesh/mesh.hpp>
#include <renderer/core/texture/models/texture.hpp>
#include "renderer/3d/pipeline/minecraft/minecraft_pipeline.hpp"
#include "constants.hpp"
#include "renderer/3d/pipeline/minecraft/mcpip_block.hpp"
#include "models/sfx_block_model.hpp"
#include "entities/sfx_library_category.hpp"
#include "entities/sfx_library_sound.hpp"
#include "managers/block/sound/block_sfx_base_repository.hpp"
#include "managers/block/sound/block_dig_sfx_repository.hpp"
#include "managers/block/sound/block_step_sfx_repository.hpp"
#include "managers/block/texture/block_texture_info_repository.hpp"
#include "models/block_info_model.hpp"
#include <tyra>

using Tyra::Audio;
using Tyra::McpipBlock;
using Tyra::Mesh;
using Tyra::MinecraftPipeline;
using Tyra::Renderer;
using Tyra::Texture;

class BlockManager {
 public:
  BlockManager();
  ~BlockManager();
  void init(Renderer* t_renderer, MinecraftPipeline* mcPip);

  BlockInfo* getBlockTexOffsetByType(const u8& blockType);
  SfxBlockModel* getDigSoundByBlockType(const u8& blockType);
  SfxBlockModel* getStepSoundByBlockType(const u8& blockType);
  inline Texture* getBlocksTexture() { return this->blocksTexAtlas; };
  float getBlockBreakingTime();
  McpipBlock* getDamageOverlay(const float& damage_percentage);

 private:
  void registerBlockSoundsEffects();
  void registerDamageOverlayBlocks(MinecraftPipeline* mcPip);
  void loadBlocksTextures(Renderer* t_renderer);

  Texture* blocksTexAtlas;
  Renderer* t_renderer;
  BlockTextureRepository* t_blockTextureRepository;

  std::vector<McpipBlock*> damage_overlay;
  std::vector<BlockSfxBaseRepository*> blockSfxRepositories;
};
