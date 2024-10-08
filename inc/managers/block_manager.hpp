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
#include "entities/Block.hpp"
#include "entities/sfx_library_category.hpp"
#include "entities/sfx_library_sound.hpp"
#include "managers/block/sound/block_sfx_base_repository.hpp"
#include "managers/block/sound/block_dig_sfx_repository.hpp"
#include "managers/block/sound/block_broken_sfx_repository.hpp"
#include "managers/block/sound/block_step_sfx_repository.hpp"
#include "managers/block/block_info_repository.hpp"
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
  void init(Renderer* t_renderer, const std::string& texturePack);

  BlockInfo* getBlockInfoByType(const Blocks& blockType);
  const u8 isBlockTransparent(const Blocks& blockType);
  const u8 isBlockOriented(const Blocks& blockType);

  SfxBlockModel* getBrokenSoundByBlockType(const Blocks& blockType);
  SfxBlockModel* getDigSoundByBlockType(const Blocks& blockType);
  SfxBlockModel* getStepSoundByBlockType(const Blocks& blockType);
  inline Texture* getBlocksTexture() { return this->blocksTexAtlas; };
  inline Texture* getBlocksTextureLowRes() {
    return this->blocksTexAtlasLowRes;
  };

  float getBlockBreakingTime(Block* targetBlock);
  u8 getBlockLightValue(Blocks blockType);

 private:
  void registerBlockSoundsEffects();
  void registerDamageOverlayBlocks(MinecraftPipeline* mcPip);
  void loadBlocksTextures(const std::string& texturePack);
  void loadBlocksTexturesLowRes(const std::string& texturePack);

  bool canHarvestWithCurrentTool(const Blocks blockType);
  bool isBestTool(const Blocks blockType);

  Texture* blocksTexAtlas;
  Texture* blocksTexAtlasLowRes;
  Renderer* t_renderer;
  BlockInfoRepository* t_BlockInfoRepository;

  std::vector<BlockSfxBaseRepository*> blockSfxRepositories;
};
