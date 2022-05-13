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

    this->applyFOG(t_player->getPosition());
}

void Chunck::applyFOG(Vector3 originPosition)
{
    for (u16 blockIndex = 0; blockIndex < this->blocks.size(); blockIndex++)
    {
        if (this->blocks[blockIndex]->type != AIR_BLOCK &&
            !this->blocks[blockIndex]->isHidden &&
            this->blocks[blockIndex]->mesh.getMaterialsCount() > 0)
        {
            float visibility = 255 * this->getVisibityByPosition(originPosition.distanceTo(this->blocks[blockIndex]->position));
            for (u16 materialIndex = 0; materialIndex < this->blocks[blockIndex]->mesh.getMaterialsCount(); materialIndex++)
            {
                this->blocks[blockIndex]->mesh.getMaterial(materialIndex).color.a = visibility;
            }
        }
    }
}

void Chunck::renderer(Renderer *t_renderer)
{
    for (u16 blockIndex = 0; blockIndex < this->blocks.size(); blockIndex++)
        if (this->blocks[blockIndex]->type != AIR_BLOCK && !this->blocks[blockIndex]->isHidden)
            t_renderer->draw(this->blocks[blockIndex]->mesh, NULL, 0);
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
    // Clear chunck data
    for (u16 blockIndex = 0; blockIndex < this->blocks.size(); blockIndex++)
    {
        if (this->blocks[blockIndex]->type != AIR_BLOCK &&
            !this->blocks[blockIndex]->isHidden &&
            this->blocks[blockIndex]->mesh.getMaterialsCount() > 0)
        {
            for (u16 materialIndex = 0; materialIndex < this->blocks[blockIndex]->mesh.getMaterialsCount(); materialIndex++)
            {
                this->blockManager->removeTextureLinkByBlockType(this->blocks[blockIndex]->type, this->blocks[blockIndex]->mesh.getMaterial(materialIndex).getId(), materialIndex);
            }
        }
        delete this->blocks[blockIndex];
    }

    this->blocks.clear();
    this->blocks.shrink_to_fit();
}

void Chunck::addBlock(Block *t_block)
{
    this->blocks.push_back(t_block);
}
