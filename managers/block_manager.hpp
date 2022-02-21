#ifndef _BLOCK_MANAGER_
#define _BLOCK_MANAGER_

#include <engine.hpp>
#include <tamtypes.h>
#include <modules/texture_repository.hpp>
#include <models/texture.hpp>
#include <models/mesh.hpp>
#include "../include/contants.hpp"

class BlockManager
{
public:
    Mesh dirtBlock;
    TextureRepository *texRepo;

    BlockManager(TextureRepository *t_texRepo);
    ~BlockManager();
    Mesh getMeshByBlockType(int blockType);
    void linkTextureByBlockType(int blockType, const u32 t_meshId);
    void loadBlocks();
};

#endif