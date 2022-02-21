#include <models/mesh.hpp>
#include <loaders/obj_loader.hpp>
#include <utils/debug.hpp>

#include "chunck.hpp"

Chunck::Chunck(){};

Chunck::~Chunck()
{
}

void Chunck::renderer()
{
    for (size_t i = 0; i < this->blocks.size(); i++)
    {
        if (this->blocks[i].isDataLoaded() && engine->renderer->getTextureRepository()->getBySpriteOrMesh(this->blocks[i].getMaterial(0).getId()))
        {
            engine->renderer->draw(this->blocks[i]);
        }
    }
};

void Chunck::pushBlock(Block block)
{
    this->blocks.push_back(block);
};
