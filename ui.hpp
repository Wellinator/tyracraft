#ifndef _UI_
#define _UI_

#include <modules/texture_repository.hpp>
#include <modules/renderer.hpp>
#include "objects/player.hpp"
#include <tamtypes.h>

class Ui
{

public:
    Ui(TextureRepository *t_texRepo);
    ~Ui();

    void update(const Player &player);
    void render(Renderer *t_renderer);

private:
    Sprite crosshair;
};

#endif