#include <models/mesh.hpp>
#include <loaders/obj_loader.hpp>
#include "chunck.hpp"

Chunck::Chunck(Engine *t_engine)
{
    this->engine = t_engine;
};

Chunck::~Chunck()
{
}

void Chunck::renderer(Player *t_player)
{
    // Vector3 from;
    // from.set(t_player->getPosition());
    // TODO: draw with mesh array as Tyra Engine recommends;
    for (u16 i = 0; i < this->blocks.size(); i++)
    {
        float visibility = 255 * this->getVisibityByPosition(
                                     t_player->getPosition().distanceTo(this->blocks[i]->mesh.position));

        if (visibility <= 3)
            continue;

        this->blocks[i]->mesh.getMaterial(0).color.a = visibility;

        // Draw mesh
        engine->renderer->draw(this->blocks[i]->mesh);
    }
};

void Chunck::add(Block *t_block)
{
    this->blocks.push_back(t_block);
};

void Chunck::clear()
{
    this->blocks.clear();
    this->blocks.shrink_to_fit();
}

/**
 * Calculate the FOG by distance;
 */
float Chunck::getVisibityByPosition(float d)
{
    // float const offset = 4.0F * BLOCK_SIZE;
    // return Utils::FOG_LINEAR(
    //     d,
    //     0,
    //     CHUNCK_SIZE * BLOCK_SIZE,
    //     offset);

    return Utils::FOG_EXP_GRAD(d, 0.007F, 3.0F);
}
