#include <models/mesh.hpp>
#include <loaders/obj_loader.hpp>
#include <utils/debug.hpp>
#include <modules/texture_repository.hpp>

#include "Block.hpp"

Block::Block()
{
    this->mesh.position.set(0, 0, 0);
};

Block::~Block(){};

void Block::init(Mesh &mother, float X, float Y, float Z)
{
    this->mesh.position.set(X, Y, Z);
    this->mesh.loadFrom(mother);
    // this->mesh.shouldBeLighted = false;
    // this->mesh.shouldBeFrustumCulled = false;
}

bool Block::shouldBeDrawn()
{
    //Clip if its hidden or if its an Air Block
    return !this->isHidden;
}