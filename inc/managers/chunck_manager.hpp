// Based on https://gist.github.com/luuthevinh/42227ad9712e86ab9d5c3ab37a56936c

#pragma once

#include <debug/debug.hpp>
#include <renderer/3d/mesh/mesh.hpp>
#include <renderer/core/2d/sprite/sprite.hpp>
#include <renderer/3d/bbox/bbox.hpp>
#include <math/vec4.hpp>
#include <math.h>
#include "contants.hpp"
#include "entities/chunck.hpp"

using Tyra::BBox;
using Tyra::Mesh;
using Tyra::Vec4;

class ChunckManager {
 public:
  ChunckManager();
  ~ChunckManager();

  Chunck* getChunckByPlayerPosition(Player* t_player);
  Chunck* getChunckById(const u16 id);
  void init(BlockManager* t_blockManager);

 private:
  std::vector<Chunck*> chuncks;
  BlockManager* t_blockManager;

  void generateChunks();
};
