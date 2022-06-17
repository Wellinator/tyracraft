#ifndef _STATE_MANAGER_
#define _STATE_MANAGER_

#include <tamtypes.h>
#include <modules/pad.hpp>

#include "../objects/player.hpp"
#include "../objects/World.hpp"
#include "../include/contants.hpp"
#include "../ui.hpp"
#include <models/mesh.hpp>
#include "../splash_screen.hpp"
#include "menu_manager.hpp"
#include "items_repository.hpp"
#include <chrono>

class StateManager
{
public:
    StateManager();
    ~StateManager();

    void init(TextureRepository *t_texRepo, Renderer *t_renderer, Audio *t_audio, ScreenSettings *t_screen);
    void update(Pad &t_pad, Camera &camera);

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
    TextureRepository *t_texRepo;
    Renderer *t_renderer;
    Audio *t_audio;
    ScreenSettings *t_screen;

    SplashScreen *splashScreen;
    MainMenu *mainMenu;

    //Control game mode change
    //Control Cross click debounce for changing game mode
    std::chrono::steady_clock::time_point lastTimeCrossWasClicked;
    void controlGameMode(Pad &t_pad);
};

#endif