#include "managers/block_manager.hpp"
#include "file/file_utils.hpp"
#include <math/vec4.hpp>
#include "constants.hpp"

using Tyra::FileUtils;
using Tyra::McpipBlock;
using Tyra::MinecraftPipeline;
using Tyra::Renderer;
using Tyra::Vec4;

BlockManager::BlockManager() {}

BlockManager::~BlockManager() {
  delete this->t_BlockInfoRepository;

  this->t_renderer->getTextureRepository().free(this->blocksTexAtlas->id);
  this->t_renderer->getTextureRepository().free(this->blocksTexAtlasLowRes->id);

  for (u8 i = 0; i < this->blockSfxRepositories.size(); i++) {
    delete this->blockSfxRepositories[i];
    this->blockSfxRepositories[i] = nullptr;
  }
  this->blockSfxRepositories.clear();
  this->blockSfxRepositories.shrink_to_fit();
}

void BlockManager::init(Renderer* t_renderer, const std::string& texturePack) {
  this->t_renderer = t_renderer;
  this->t_BlockInfoRepository = new BlockInfoRepository();
  this->loadBlocksTextures(texturePack);
  // this->loadBlocksTexturesLowRes(texturePack);
  this->registerBlockSoundsEffects();
}

void BlockManager::loadBlocksTextures(const std::string& texturePack) {
  const std::string pathPrefix = "textures/texture_packs/";
  const std::string path =
      pathPrefix + texturePack + "/block/texture_atlas.png";

  blocksTexAtlas =
      t_renderer->core.texture.repository.add(FileUtils::fromCwd(path.c_str()));
}

void BlockManager::loadBlocksTexturesLowRes(const std::string& texturePack) {
  const std::string pathPrefix = "textures/texture_packs/";
  const std::string path =
      pathPrefix + texturePack + "/block/texture_atlas_lower_res.png";

  blocksTexAtlasLowRes =
      t_renderer->core.texture.repository.add(FileUtils::fromCwd(path.c_str()));
}

void BlockManager::registerBlockSoundsEffects() {
  this->blockSfxRepositories.push_back(new BlockDigSfxRepository());
  this->blockSfxRepositories.push_back(new BlockBrokenSfxRepository());
  this->blockSfxRepositories.push_back(new BlockStepSfxRepository());
}

BlockInfo* BlockManager::getBlockInfoByType(const Blocks& blockType) {
  return this->t_BlockInfoRepository->getBlockInfo(blockType);
}

const u8 BlockManager::isBlockTransparent(const Blocks& blockType) {
  return this->t_BlockInfoRepository->isBlockTransparent(blockType);
}

const u8 BlockManager::isBlockOriented(const Blocks& blockType) {
  return blockType == Blocks::JACK_O_LANTERN_BLOCK ||
         blockType == Blocks::PUMPKIN_BLOCK;
}

SfxBlockModel* BlockManager::getBrokenSoundByBlockType(
    const Blocks& blockType) {
  for (size_t i = 0; i < this->blockSfxRepositories.size(); i++)
    if (blockSfxRepositories[i]->id == SoundFxCategory::Broken)
      return blockSfxRepositories[i]->getModel((u8)blockType);

  TYRA_WARN("Block sound not found for type: ",
            std::to_string((u8)blockType).c_str());
  return nullptr;
}

SfxBlockModel* BlockManager::getDigSoundByBlockType(const Blocks& blockType) {
  for (size_t i = 0; i < this->blockSfxRepositories.size(); i++)
    if (blockSfxRepositories[i]->id == SoundFxCategory::Dig)
      return blockSfxRepositories[i]->getModel((u8)blockType);

  TYRA_WARN("Block sound not found for type: ",
            std::to_string((u8)blockType).c_str());
  return nullptr;
}

SfxBlockModel* BlockManager::getStepSoundByBlockType(const Blocks& blockType) {
  for (size_t i = 0; i < this->blockSfxRepositories.size(); i++)
    if (this->blockSfxRepositories[i]->id == SoundFxCategory::Step)
      return this->blockSfxRepositories[i]->getModel((u8)blockType);

  TYRA_WARN("Block sound not found for type: ",
            std::to_string((u8)blockType).c_str());
  return nullptr;
}

// ref: https://minecraft.fandom.com/wiki/Breaking
float BlockManager::getBlockBreakingTime(Block* targetBlock) {
  // TODO: apply in survival mode only
  const auto baseSpeed = isBestTool(targetBlock->type) ? 1.5F : 5.0F;

  const float baseBreakingTime = baseSpeed * targetBlock->getHardness();
  return baseBreakingTime;
}

// TODO: implements canHarvestWithCurrentTool, it must receive the current
// player tool
// See: https://minecraft.fandom.com/wiki/Breaking#Blocks_by_hardness
bool BlockManager::canHarvestWithCurrentTool(const Blocks blockType) {
  return true;
}

bool BlockManager::isBestTool(const Blocks blockType) {
  if ((blockType == Blocks::STONE_BLOCK || blockType == Blocks::BRICKS_BLOCK) ||
      // Ores
      ((u8)blockType >= (u8)Blocks::GOLD_ORE_BLOCK &&
       (u8)blockType >= (u8)Blocks::EMERALD_ORE_BLOCK) ||
      // Bricks
      ((u8)blockType >= (u8)Blocks::STONE_BRICK_BLOCK &&
       (u8)blockType >= (u8)Blocks::CHISELED_STONE_BRICKS_BLOCK) ||
      // Wools
      ((u8)blockType >= (u8)Blocks::YELLOW_WOOL &&
       (u8)blockType >= (u8)Blocks::BLACK_WOOL)) {
    return false;
  }

  return true;
}
// TODO: implement best tool calculation

u8 BlockManager::getBlockLightValue(Blocks blockType) {
  switch (blockType) {
    case Blocks::GLOWSTONE_BLOCK:
      return 15;
    case Blocks::JACK_O_LANTERN_BLOCK:
      return 15;
    case Blocks::LAVA_BLOCK:
      return 15;
    case Blocks::TORCH:
      return 14;

    default:
      return 0;
  }
}