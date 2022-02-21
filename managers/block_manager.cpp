#include <modules/texture_repository.hpp>
#include "block_manager.hpp"

BlockManager::BlockManager(TextureRepository *t_texRepo)
{
    PRINT_LOG("Initializing Block Manager");

    texRepo = t_texRepo;
    this->loadBlocks();
}

BlockManager::~BlockManager()
{
}

Mesh BlockManager::getMeshByBlockType(int blockType)
{
    if (blockType == DIRTY_BLOCK)
        return this->dirtBlock;

    //Should return an invalid texture
    return dirtBlock;
}

void BlockManager::linkTextureByBlockType(int blockType, const u32 t_meshId)
{
    this->texRepo->getBySpriteOrMesh(
                     this->getMeshByBlockType(blockType).getMaterial(0).getId())
        ->addLink(t_meshId);
}

void BlockManager::loadBlocks()
{
    dirtBlock.loadObj("meshes/cube/", "cube", BLOCK_SIZE, false);
    texRepo->addByMesh("meshes/cube/", dirtBlock, PNG);
}