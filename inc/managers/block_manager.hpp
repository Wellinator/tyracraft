#pragma once

#include <vector>
#include <tamtypes.h>
#include <renderer/renderer.hpp>
#include <renderer/3d/mesh/mesh.hpp>
#include <renderer/core/texture/models/texture.hpp>
#include "renderer/3d/pipeline/minecraft/minecraft_pipeline.hpp"
#include "contants.hpp"

using Tyra::Mesh;
using Tyra::Renderer;
using Tyra::Texture;
using Tyra::MinecraftPipeline;

struct BlockInfo {
  BlockInfo(u8 type, u8 isSingle, const float& texOffssetX,
            const float& texOffssetY) {
    _texOffssetX = texOffssetX;
    _texOffssetY = texOffssetY;
    blockId = type;
    _isSingle = isSingle;
  };

  ~BlockInfo(){};

  float _texOffssetX;
  float _texOffssetY;
  u8 _isSingle;
  u8 blockId;
};

class BlockManager {
 public:
  BlockManager();
  ~BlockManager();
  void init(Renderer* t_renderer, MinecraftPipeline* mcPip);
  BlockInfo* getBlockTexOffsetByType(u8 blockType);
  inline Texture* getBlocksTexture() { return blocksTexAtlas; };

 private:
  void registerBlocksTextureCoordinates(MinecraftPipeline* mcPip);
  void loadBlocksTextures();

  Texture* blocksTexAtlas;
  Renderer* t_renderer;
  std::vector<BlockInfo*> blockItems;
};
