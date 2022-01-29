#ifndef _START_
#define _START_

#include <utils/debug.hpp>
#include <tamtypes.h>
#include <game.hpp>
#include <engine.hpp>
#include "managers/terrain_generator.hpp"
#include "objects/chunck.hpp"
#include "objects/Block.hpp"
#include "objects/player.hpp"
#include "camera.hpp"
#include "map.hpp"

class Start: public Game
{
    private:
        void setBgColorAndAmbientColor();
        Map *map;
        Player *player;
        Camera camera;
        TextureRepository *texRepo;

    public:
        Start(Engine *t_engine);
        ~Start();

        void onInit();
        void onUpdate();

        Engine *engine;
};

#endif
