#pragma once

#include "managers/block/sound/block_sfx_base_repository.hpp"
#include "constants.hpp"
#include <tyra>

using Tyra::Audio;
using Tyra::FileUtils;

class BlockStepSfxRepository : public BlockSfxBaseRepository {
 public:
  BlockStepSfxRepository();
  ~BlockStepSfxRepository();

  void loadModels();
};
