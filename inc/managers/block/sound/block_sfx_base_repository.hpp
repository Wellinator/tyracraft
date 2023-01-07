#pragma once
#include "entities/sfx_library_category.hpp"
#include "entities/sfx_library_sound.hpp"
#include "models/sfx_block_model.hpp"
#include "constants.hpp"
#include <tyra>

using Tyra::Audio;

class BlockSfxBaseRepository {
 public:
  BlockSfxBaseRepository(SoundFxCategory _id) { id = _id; };
  virtual ~BlockSfxBaseRepository(){};

  virtual void loadModels() = 0;

  SfxBlockModel* getModel(const u8& blockType) {
    for (size_t i = 0; i < this->models.size(); i++)
      if (this->models[i]->getType() == blockType) return this->models[i];

    return nullptr;
  }

  SoundFxCategory id;
  std::vector<SfxBlockModel*> models;
  Audio* t_audio;
};
