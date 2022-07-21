#pragma once

#include <tamtypes.h>
#include <pad/pad.hpp>
#include "entities/player.hpp"
#include "entities/World.hpp"
#include "contants.hpp"
#include "ui.hpp"
#include <renderer/3d/mesh/mesh.hpp>
#include "splash_screen.hpp"
#include "menu_manager.hpp"
#include "items_repository.hpp"
#include <chrono>
#include <renderer/renderer.hpp>
#include "renderer/renderer_settings.hpp"

using Tyra::RendererSettings;
using Tyra::Renderer;
using Tyra::Audio;
using Tyra::Pad;

class StateManager
{
public:
    StateManager();
    ~StateManager();

    void init(Renderer* t_renderer, Audio* t_audio);
    void update(float deltaTime, Pad &t_pad, Camera &camera);

    u8 currentState();
    void loadSplashScreen();
    void loadMenu();
    void loadInGameMenu();
    void play();

    void setBgColorAndAmbientColor();

    World *world;
    Ui *ui;
    Player *player;
    ItemRepository *itemRepository;

private:
    void loadGame();

    GAME_MODE gameMode = SURVIVAL;

    u8 _state = SPLASH_SCREEN;
    Renderer* t_renderer;
    Audio* t_audio;

    SplashScreen *splashScreen;
    MainMenu *mainMenu;

    //Control game mode change
    //Control Cross click debounce for changing game mode
    std::chrono::steady_clock::time_point lastTimeCrossWasClicked;
    void controlGameMode(Pad &t_pad);
};
