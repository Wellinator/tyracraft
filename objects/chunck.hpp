#ifndef _CHUNCK_
#define _CHUNCK_

#include <engine.hpp>
#include <tamtypes.h>
#include <modules/timer.hpp>
#include <modules/pad.hpp>
#include "../camera.hpp"
#include "modules/texture_repository.hpp"
#include "Block.hpp"

class Chunck
{
public:
    Chunck(Engine *t_engine, float offsetX, float offsetY, float offsetZ);
    ~Chunck();
    Mesh **getMeshes() { return meshes; }
    void update(const Pad &t_pad, Camera &camera);
    
private:
    Mesh **meshes;
    Block* blocks[CHUNCK_SIZE*CHUNCK_SIZE*CHUNCK_SIZE];
    TextureRepository *texRepo;
    Engine *engine;
};

#endif
