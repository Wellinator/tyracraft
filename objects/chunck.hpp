#ifndef _CHUNCK_
#define _CHUNCK_

#include <engine.hpp>
#include <vector>
#include "../include/contants.hpp"
#include "Node.hpp"

class Chunck
{
public:
    Chunck(Engine *t_engine);
    ~Chunck();

    Engine *engine;
    TextureRepository *texRepo;
    void renderer();
    inline int getChunckSize() const { return CHUNCK_SIZE * CHUNCK_SIZE * CHUNCK_SIZE; };
    void add(Node *t_node);
    void clear();
    std::vector<Node *> nodes;

private:
};

#endif
