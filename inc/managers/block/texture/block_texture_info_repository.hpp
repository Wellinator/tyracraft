#pragma once
#include "constants.hpp"
#include "models/block_info_model.hpp"
#include <tamtypes.h>
#include <vector>
#include <tyra>

using Tyra::MinecraftPipeline;

class BlockTextureRepository {
 public:
  BlockTextureRepository();
  ~BlockTextureRepository();

  BlockInfo* getTextureInfo(const Blocks& blockType);

  const u8 isBlockTransparent(const Blocks& blockType);

 private:
  /**
   * @brief Load blocks textures
   * @warning Should load textures in @enum Blocks order to load info faster
   * (haky)
   *
   */
  void loadTextures();

  std::vector<BlockInfo> models;
};
