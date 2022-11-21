#pragma once
#include "constants.hpp"
#include "models/block_info_model.hpp"
#include <tamtypes.h>
#include <vector>
#include <tyra>

using Tyra::MinecraftPipeline;

class BlockTextureRepository {
 public:
  BlockTextureRepository(MinecraftPipeline* t_mcPip);
  ~BlockTextureRepository();

  BlockInfo* getTextureInfo(const Blocks& blockType);

 private:
  /**
   * @brief Load blocks textures
   * @warning Should load textures in @enum Blocks order to load info faster
   * (haky)
   *
   */
  void loadTextures();

  MinecraftPipeline* t_mcPip;
  std::vector<BlockInfo*> models;
};
