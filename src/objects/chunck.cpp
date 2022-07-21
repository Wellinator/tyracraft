#include <renderer/3d/mesh/mesh.hpp>
// #include <loaders/obj_loader.hpp>
#include "entities/chunck.hpp"

Chunck::Chunck(BlockManager *t_blockManager)
{
    this->blockManager = t_blockManager;
};

Chunck::~Chunck()
{
}

void Chunck::update(Player *t_player)
{
    this->updateBlocks(*t_player->getPosition());
}

void Chunck::applyFOG(Mesh *t_mesh, const Vec4 &originPosition)
{
    // float visibility = 255 * this->getVisibityByPosition(originPosition.distanceTo(*t_mesh->getPosition()));
    // for (u16 materialIndex = 0; materialIndex < t_mesh->getMaterialsCount(); materialIndex++)
    // {
    //     t_mesh->getMaterial(materialIndex).color.a = visibility;
    // }
}

void Chunck::highLightTargetBlock(Mesh *t_mesh, u8 &isTarget)
{
    // for (u16 materialIndex = 0; materialIndex < t_mesh->getMaterialsCount(); materialIndex++)
    // {
    //     t_mesh->getMaterial(materialIndex).color.r = isTarget ? 160 : 128;
    //     t_mesh->getMaterial(materialIndex).color.g = isTarget ? 160 : 128;
    //     t_mesh->getMaterial(materialIndex).color.b = isTarget ? 160 : 128;
    // }
}

void Chunck::renderer(Renderer *t_renderer)
{
    // TODO: refactor to minecraft pipeline
    // for (u16 blockIndex = 0; blockIndex < this->blocks.size(); blockIndex++)
    //     if (this->blocks[blockIndex]->mesh->isDataLoaded())
    //         t_renderer->draw(this->blocks[blockIndex]->mesh, NULL, 0);
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
        if (this->blocks[blockIndex]->mesh->getMaterialsCount() > 0)
        {
            for (u16 materialIndex = 0; materialIndex < this->blocks[blockIndex]->mesh->getMaterialsCount(); materialIndex++)
            {
                this->blockManager->removeTextureLinkByBlockType(this->blocks[blockIndex]->type, this->blocks[blockIndex]->mesh->getMaterial(materialIndex)->getId(), materialIndex);
            }
        }

        if (this->blocks[blockIndex] != NULL && this->blocks[blockIndex] != nullptr)
        {
            delete this->blocks[blockIndex];
            this->blocks[blockIndex] = NULL;
        }
    }

    this->blocks.clear();
    this->blocks.shrink_to_fit();
}

void Chunck::addBlock(Block *t_block)
{
    this->blocks.push_back(t_block);
}

void Chunck::updateBlocks(const Vec4 &playerPosition)
{
    for (u16 blockIndex = 0; blockIndex < this->blocks.size(); blockIndex++)
    {
        if (this->blocks[blockIndex]->mesh->getMaterialsCount() > 0)
        {
            this->applyFOG(this->blocks[blockIndex]->mesh, playerPosition);
            this->highLightTargetBlock(this->blocks[blockIndex]->mesh, this->blocks[blockIndex]->isTarget);
        }
    }
}
