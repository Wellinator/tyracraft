#pragma once

#include <vector>
#include <tamtypes.h>
#include <renderer/renderer.hpp>
#include <renderer/3d/mesh/mesh.hpp>
#include <renderer/core/texture/models/texture.hpp>
#include "contants.hpp"

using Tyra::Mesh;
using Tyra::Renderer;
using Tyra::Texture;

struct BlockTexOffset {
  BlockTexOffset(u8 type, u8 isSingle, const float& texOffssetX,
                 const float& texOffssetY) {
    _texOffssetX = texOffssetX;
    _texOffssetY = texOffssetY;
    blockId = type;
    _isSingle = isSingle;
  };

  ~BlockTexOffset(){};

  float _texOffssetX;
  float _texOffssetY;
  u8 _isSingle;
  u8 blockId;
};

class BlockManager {
 public:
  BlockManager();
  ~BlockManager();
  void init(Renderer* t_renderer);
  BlockTexOffset* getBlockTexOffsetByType(u8 blockType);

 private:
  void registerBlocksTextureCoordinates();
  void loadBlocksTextures();

  Texture* blocksTexAtlas;
  Renderer* t_renderer;
  std::vector<BlockTexOffset*> blockItems;
};
