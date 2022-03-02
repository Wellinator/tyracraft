#include <models/mesh.hpp>
#include <loaders/obj_loader.hpp>
#include <utils/debug.hpp>

#include "chunck.hpp"

Chunck::Chunck(Engine *t_engine)
{
    this->engine = t_engine;
};

Chunck::~Chunck()
{
}

void Chunck::renderer()
{
    for (size_t i = 0; i < this->blocks.size(); i++)
    {
        if (this->blocks[i]->shouldBeDrawn())
        {
            engine->renderer->draw(this->blocks[i]->mesh);
        }
    }
};

void Chunck::add(Block *block)
{
    this->blocks.push_back(block);
};

void Chunck::clear()
{
    this->blocks.clear();
}
