#ifndef _CHUNCK_
#define _CHUNCK_

#define CHUNCK_SIZE 2

#include <engine.hpp>
#include <tamtypes.h>
#include <modules/timer.hpp>
#include <modules/pad.hpp>
#include "../camera.hpp"
#include "modules/texture_repository.hpp"
#include "./Block.hpp"

class Chunck
{
public:
    Chunck(Engine *t_engine, float offsetX, float offsetY, float offsetZ);
    ~Chunck();
    
    Mesh **getMeshes() { return meshes; }
    inline u8 getMeshesCount() { return CHUNCK_SIZE * 3; }
    void update(Pad &t_pad, Camera &camera);

private:
    Mesh **meshes;
    Block *blocks[CHUNCK_SIZE * 3];
    TextureRepository *texRepo;
    Engine *engine;
};

#endif
