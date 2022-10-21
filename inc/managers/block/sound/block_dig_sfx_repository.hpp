#pragma once

#include "managers/block/sound/block_sfx_base_repository.hpp"
#include "contants.hpp"
#include <tyra>

using Tyra::Audio;
using Tyra::FileUtils;

class BlockDigSfxRepository : public BlockSfxBaseRepository {
 public:
  BlockDigSfxRepository();
  ~BlockDigSfxRepository();

  void loadModels();
};
