#include "entities/Block.hpp"
#include "renderer/3d/pipeline/minecraft/mcpip_block.hpp"

using Tyra::McpipBlock;

Block::Block(u8 type) : McpipBlock() { this->type = type; }

Block::~Block(){}
