#include "Block.hpp"

Block::Block(){};

Block::~Block(){};

void Block::init(int block_type, float X, float Y, float Z)
{
    this->position.set(X, Y, Z);
    this->block_type = block_type;
}

bool Block::shouldBeDrawn()
{
    //Clip if its hidden or if its an Air Block
    return !this->isHidden;
}
