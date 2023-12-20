#pragma once

#include "managers/block/sound/block_sfx_base_repository.hpp"
#include "constants.hpp"
#include <tyra>

using Tyra::Audio;
using Tyra::FileUtils;

class BlockBrokenSfxRepository : public BlockSfxBaseRepository {
 public:
  BlockBrokenSfxRepository();
  ~BlockBrokenSfxRepository();

  void loadModels();
};
