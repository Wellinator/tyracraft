#include <models/mesh.hpp>
#include <loaders/obj_loader.hpp>
#include "chunck.hpp"

Chunck::Chunck(BlockManager *t_blockManager)
{
    this->blockManager = t_blockManager;
};

Chunck::~Chunck()
{
}

void Chunck::update(Player *t_player)
{
    for (u8 blockTypeIndex = 0; blockTypeIndex < BLOCKS_COUNTER; blockTypeIndex++)
    {
        if (blockTypeIndex != AIR_BLOCK && this->blocksMeshes[blockTypeIndex].size())
        {
            for (u16 meshIndex = 0; meshIndex < this->blocksMeshes[blockTypeIndex].size(); meshIndex++)
            {
                float visibility = 255 * this->getVisibityByPosition(
                                             t_player->getPosition().distanceTo(this->blocksMeshes[blockTypeIndex][meshIndex]->position));
                for (u16 materialIndex = 0; materialIndex < this->blocksMeshes[blockTypeIndex][meshIndex]->getMaterialsCount(); materialIndex++)
                {
                    this->blocksMeshes[blockTypeIndex][meshIndex]->getMaterial(materialIndex).color.a = visibility;
                }
            }
        }
    }
}

void Chunck::renderer(Renderer *t_renderer)
{
    for (u8 blockTypeIndex = 0; blockTypeIndex < BLOCKS_COUNTER; blockTypeIndex++)
    {
        if (blockTypeIndex != AIR_BLOCK && this->blocksMeshes[blockTypeIndex].size())
            t_renderer->draw(this->blocksMeshes[blockTypeIndex].data(), this->blocksMeshes[blockTypeIndex].size());
    }
};

/**
 * Calculate the FOG by distance;
 */
float Chunck::getVisibityByPosition(float d)
{
    return Utils::FOG_EXP_GRAD(d, 0.007F, 3.0F);
}

void Chunck::clear()
{
    for (u8 blockTypeIndex = 0; blockTypeIndex < BLOCKS_COUNTER; blockTypeIndex++)
    {
        if (this->blocksMeshes[blockTypeIndex].size())
        {
            for (u16 meshIndex = 0; meshIndex < this->blocksMeshes[blockTypeIndex].size(); meshIndex++)
            {
                if (this->blocksMeshes[blockTypeIndex][meshIndex]->getMaterialsCount() > 0)
                {
                    for (u16 materialIndex = 0; materialIndex < this->blocksMeshes[blockTypeIndex][meshIndex]->getMaterialsCount(); materialIndex++)
                    {
                        this->blockManager->removeTextureLinkByBlockType(blockTypeIndex, this->blocksMeshes[blockTypeIndex][meshIndex]->getMaterial(materialIndex).getId(), materialIndex);
                    }
                }
                delete this->blocksMeshes[blockTypeIndex][meshIndex];
            }
            this->blocksMeshes[blockTypeIndex].clear();
            this->blocksMeshes[blockTypeIndex].shrink_to_fit();
        }
    }

    delete[] mesheCounters;
    mesheCounters = new u8[BLOCKS_COUNTER];
}

void Chunck::addMeshByBlockType(u8 blockType, Mesh *t_mesh)
{
    this->blocksMeshes[blockType].push_back(t_mesh);
}