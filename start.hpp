#ifndef _START_
#define _START_

#include <utils/debug.hpp>
#include <tamtypes.h>
#include <game.hpp>
#include <engine.hpp>
#include "managers/terrain_manager.hpp"
#include "objects/chunck.hpp"
#include "objects/Block.hpp"
#include "objects/player.hpp"
#include "objects/World.hpp"
#include "camera.hpp"

class Start : public Game, AudioListener
{
private:
    void setBgColorAndAmbientColor();
    World *world;
    Mesh skybox;
    Player *player;
    Camera camera;
    TextureRepository *texRepo;
    u32 audioTicks;
    u8 skip1Beat;

public:
    Start(Engine *t_engine);
    ~Start();

    void onInit();
    void onUpdate();
    void onAudioTick();
    void onAudioFinish();

    Engine *engine;
};

#endif
