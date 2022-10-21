#pragma once
#include "contants.hpp"
#include "models/block_info_model.hpp"
#include <tamtypes.h>
#include <vector>
#include <tyra>

using Tyra::MinecraftPipeline;

class BlockTextureRepository {
 public:
  BlockTextureRepository(MinecraftPipeline* t_mcPip);
  ~BlockTextureRepository();

  BlockInfo* getTextureInfo(const u8& blockType);

 private:
  void loadTextures();

  MinecraftPipeline* t_mcPip;
  std::vector<BlockInfo*> models;
};
