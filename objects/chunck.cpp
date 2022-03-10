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
    // TODO: draw with mesh array as Tyra Engine recommends;
    for (size_t i = 0; i < this->blocks.size(); i++)
    {
        engine->renderer->draw(this->blocks[i]->mesh);
    }
};

void Chunck::add(Block *t_block)
{
    this->blocks.push_back(t_block);
};

void Chunck::clear()
{
    for (size_t i = 0; i < this->blocks.size(); i++)
    {
        delete this->blocks[i];
    }
    this->blocks.clear();
    this->blocks.shrink_to_fit();
}
