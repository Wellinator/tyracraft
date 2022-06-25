#ifndef _BLOCK_MANAGER_
#define _BLOCK_MANAGER_

#include <vector>
#include <tamtypes.h>
#include <modules/texture_repository.hpp>
#include <models/mesh.hpp>
#include "../include/contants.hpp"

struct BlockItem {
    BlockItem(Mesh *mesh, u8 type)
    {
        t_mesh = mesh;
        blockId = type;
    };

    ~BlockItem()
    {
        delete t_mesh;
        t_mesh = NULL;
    };

    Mesh *t_mesh;
    u8 blockId;
};

class BlockManager
{
public:
    TextureRepository *texRepo;

    BlockManager();
    ~BlockManager();
    void init(TextureRepository *t_texRepo);
    void linkTextureByBlockType(int blockType, const u32 t_meshId, u8 materialIndex);
    void removeTextureLinkByBlockType(int blockType, const u32 t_meshId, u8 materialIndex);
    Mesh &getMeshByBlockType(int blockType);

private:
    void pushItems();

    std::vector<BlockItem *> blockItems;

    /**
     * -----------BLocks meshes-----------
     */    
    // Blocks
    Mesh stoneBlock;
    Mesh grassBlock;
    Mesh dirtBlock;
    Mesh waterBlock;
    Mesh bedrockBlock;
    Mesh sandBlock;
    Mesh glassBlock;
    Mesh bricksBlock;

    // Ores and Minerals
    Mesh goldOreBlock;
    Mesh ironOreBlock;
    Mesh coalOreBlock;
    Mesh diamondOreBlock;
    Mesh redsoneOreBlock;
    Mesh emeraldOreBlock;

    // Wood Planks
    Mesh oakPlanksBlock;
    Mesh sprucePlanksBlock;
    Mesh birchPlanksBlock;
    Mesh acaciaPlanksBlock;

    // Stone Bricks
    Mesh stoneBrickBlock;
    Mesh crackedStoneBricksBlock;
    Mesh mossyStoneBricksBlock;
    Mesh chiseledStoneBricksBlock;

    // Woods
    Mesh strippedOakWoodBlock;
    Mesh oakLogBlock;
    Mesh oakLeavesBlock;
    /*-------------------------------*/

    void loadBlocks();
};

#endif
