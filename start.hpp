#ifndef _START_
#define _START_

#include <utils/debug.hpp>
#include <tamtypes.h>
#include <game.hpp>
#include <engine.hpp>
#include "managers/terrain_manager.hpp"
#include "managers/state_manager.hpp"
#include "objects/chunck.hpp"
#include "objects/Block.hpp"
#include "camera.hpp"
#include <models/mesh.hpp>
class Start : public Game, AudioListener
{
private:
    Mesh skybox;
    Camera camera;
    TextureRepository *texRepo;
    u32 audioTicks;
    u8 skip1Beat;
    StateManager stateManager;

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
