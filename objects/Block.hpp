#ifndef _BLOCK_
#define _BLOCK_

#define CHUNCK_SIZE 5

#include "../camera.hpp"
#include <tamtypes.h>
#include <modules/pad.hpp>
#include <modules/timer.hpp>
#include "modules/texture_repository.hpp"

/** Block 3D object class  */
class Block
{
public:
    Mesh mesh;
    Block(TextureRepository *t_texRepo, float X, float Y, float Z);
    ~Block();

    void update(const Pad &t_pad, const Camera &t_camera);
    void move(Pad &t_pad);

private:
    TextureRepository *texRepo;
    Vector3 *getNextPosition(const Pad &t_pad, const Camera &t_camera);
    void rotate(const Pad &t_pad);
    float speed;
};

#endif
