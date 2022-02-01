#ifndef _CHUNCK_
#define _CHUNCK_

#define CHUNCK_SIZE 16 

#include <engine.hpp>
#include <tamtypes.h>
#include <modules/timer.hpp>
#include <modules/pad.hpp>
#include <modules/texture_repository.hpp>
#include "../camera.hpp"

#include "Block.hpp"

class Chunck
{
public:
    Chunck(TextureRepository *t_texRepo, float offsetX, float offsetY, float offsetZ);
    ~Chunck();

    Mesh **meshes;
    TextureRepository *texRepo;
    int chunckSize;
    int blockIndex;
    Block blocks[CHUNCK_SIZE * CHUNCK_SIZE * CHUNCK_SIZE];
    Block *baseBlobk;
    void update(Engine *t_engine);
    inline Mesh **getMeshes() const { return meshes; }
    inline int getMeshesLength() const { return CHUNCK_SIZE * CHUNCK_SIZE; }

private:
    float initialPosition;
    void initChunck();
};

#endif
