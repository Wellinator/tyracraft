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
    this->updateBlocks(t_player->getPosition());
}

void Chunck::applyFOG(Mesh *t_mesh, const Vector3 &originPosition)
{
    float visibility = 255 * this->getVisibityByPosition(originPosition.distanceTo(t_mesh->position));
    for (u16 materialIndex = 0; materialIndex < t_mesh->getMaterialsCount(); materialIndex++)
    {
        t_mesh->getMaterial(materialIndex).color.a = visibility;
    }
}

void Chunck::highLightTargetBlock(Mesh *t_mesh, u8 &isTarget)
{
    for (u16 materialIndex = 0; materialIndex < t_mesh->getMaterialsCount(); materialIndex++)
    {
        t_mesh->getMaterial(materialIndex).color.r = isTarget ? 160 : 128;
        t_mesh->getMaterial(materialIndex).color.g = isTarget ? 160 : 128;
        t_mesh->getMaterial(materialIndex).color.b = isTarget ? 160 : 128;
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
        if (this->blocks[blockIndex]->mesh.getMaterialsCount() > 0)
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

    for (u16 i = 0; i < this->blocks.size(); i++)
        delete meshes[i];

    this->meshes.clear();
    this->meshes.shrink_to_fit();
}

void Chunck::addBlock(Block *t_block)
{
    this->blocks.push_back(t_block);
}

void Chunck::updateBlocks(const Vector3 &playerPosition)
{
    for (u16 blockIndex = 0; blockIndex < this->blocks.size(); blockIndex++)
    {
        if (this->blocks[blockIndex]->mesh.getMaterialsCount() > 0)
        {
            // this->applyFOG(&this->blocks[blockIndex]->mesh, playerPosition);
            this->highLightTargetBlock(&this->blocks[blockIndex]->mesh, this->blocks[blockIndex]->isTarget);
        }
    }
}

void Chunck::sanitize(Vector3 currentPlayerPos)
{
    for (u16 blockIndex = 0; blockIndex < this->blocks.size(); blockIndex++)
    {
        if (this->blocks[blockIndex]->position.distanceTo(currentPlayerPos) > CHUNCK_DISTANCE)
        {
            if (this->blocks[blockIndex]->mesh.getMaterialsCount() > 0)
            {
                for (u16 materialIndex = 0; materialIndex < this->blocks[blockIndex]->mesh.getMaterialsCount(); materialIndex++)
                {
                    this->blockManager->removeTextureLinkByBlockType(this->blocks[blockIndex]->type, this->blocks[blockIndex]->mesh.getMaterial(materialIndex).getId(), materialIndex);
                }
            }
            delete this->blocks[blockIndex];
            this->blocks[blockIndex] = nullptr;
        }
    }

    blocks.erase(std::remove(blocks.begin(), blocks.end(), nullptr), blocks.end());
    blocks.shrink_to_fit();
}
