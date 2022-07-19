#ifndef _BLOCK_MANAGER_
#define _BLOCK_MANAGER_

#include <vector>
#include <tamtypes.h>
#include <renderer/renderer.hpp>
#include <renderer/3d/mesh/mesh.hpp>
#include "contants.hpp"

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
    void linkTextureByBlockType(u8 blockType, const u32 t_meshId, u8 materialIndex);
    void removeTextureLinkByBlockType(u8 blockType, const u32 t_meshId, u8 materialIndex);
    Mesh *getMeshByBlockType(u8 blockType);

private:
    void pushItems();
    void loadBlocks();

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
};

#endif
