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

void Chunck::updateBlocks(const Vector3 &playerPosition)
{
    for (u16 blockIndex = 0; blockIndex < this->blocks.size(); blockIndex++)
    {
        if (this->blocks[blockIndex]->type != AIR_BLOCK &&
            !this->blocks[blockIndex]->isHidden &&
            this->blocks[blockIndex]->mesh.getMaterialsCount() > 0)
        {
            // this->applyFOG(&this->blocks[blockIndex]->mesh, playerPosition);
            this->highLightTargetBlock(&this->blocks[blockIndex]->mesh, this->blocks[blockIndex]->isTarget);
        }
    }
}

void Chunck::sanitize(Vector3 playerPos)
{
    u16 counter = 0;

    // Clip blocks out of the chunck;
    Vector3 minChunckPos = Vector3(playerPos.x - (HALF_CHUNCK_SIZE * DUBLE_BLOCK_SIZE),
                                   playerPos.y - (HALF_CHUNCK_SIZE * DUBLE_BLOCK_SIZE),
                                   playerPos.z - (HALF_CHUNCK_SIZE * DUBLE_BLOCK_SIZE));

    Vector3 maxChunckPos = Vector3(playerPos.x + (HALF_CHUNCK_SIZE * DUBLE_BLOCK_SIZE),
                                   playerPos.y + (HALF_CHUNCK_SIZE * DUBLE_BLOCK_SIZE),
                                   playerPos.z + (HALF_CHUNCK_SIZE * DUBLE_BLOCK_SIZE));

    for (u16 blockIndex = 0; blockIndex < this->blocks.size(); blockIndex++)
    {
        if (!this->blocks[blockIndex]->position.collidesBox(minChunckPos, maxChunckPos))
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
            this->blocks.erase(this->blocks.begin()+blockIndex);
            printf("Size: %d\n", this->blocks.size());
            counter++;
        }
    }

    this->blocks.shrink_to_fit();
}
