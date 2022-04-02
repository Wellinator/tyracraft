#ifndef _BLOCK_MANAGER_
#define _BLOCK_MANAGER_

#include <tamtypes.h>
#include <modules/texture_repository.hpp>
#include <models/mesh.hpp>
#include "../include/contants.hpp"

class BlockManager
{
public:
    TextureRepository *texRepo;

    BlockManager();
    ~BlockManager();
    void init(TextureRepository *t_texRepo);
    void linkTextureByBlockType(int blockType, const u32 t_meshId);
    Mesh &getMeshByBlockType(int blockType);
private:
    // Blocks meshes
    Mesh stoneBlock;
    Mesh dirtBlock;
    Mesh grassBlock;
    Mesh waterBlock;

    void loadBlocks();
};

#endif
