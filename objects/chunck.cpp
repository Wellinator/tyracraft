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
    //TODO: draw with mesh array as Tyra Engine recommends;
    for (size_t i = 0; i < this->nodes.size(); i++)
    {
        engine->renderer->draw(this->nodes[i]->mesh);
    }
};

void Chunck::add(Node *t_node)
{
    this->nodes.push_back(t_node);
};

void Chunck::clear()
{
    this->nodes.clear();
}
