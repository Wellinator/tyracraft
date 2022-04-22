/*
# ______       ____   ___
#   |     \/   ____| |___|
#   |     |   |   \  |   |
#-----------------------------------------------------------------------
# Copyright 2020, tyra - https://github.com/h4570/tyra
# Licenced under Apache License 2.0
# Sandro Sobczyński <sandro.sobczynski@gmail.com>
*/

#ifndef _MAIN_MENU_
#define _MAIN_MENU_

#include <modules/texture_repository.hpp>
#include <modules/renderer.hpp>
#include <modules/pad.hpp>
#include <models/mesh.hpp>
#include <models/sprite.hpp>
#include <models/screen_settings.hpp>
#include <tamtypes.h>
#include <sifrpc.h>
#include <debug.h>
#include <unistd.h>
#include <math.h>
#include <string>
#include "./include/contants.hpp"

//MENU_OPTIONS
#define PLAY_GAME 1
#define ABOUT 2

class MainMenu
{

public:
    MainMenu(TextureRepository *t_texRepo, ScreenSettings *t_screen);
    ~MainMenu();

    void update(Pad &t_pad);
    void render(Renderer *t_renderer);
    u8 shouldInitGame();

    //Rotating skybox
    Mesh menuSkybox;
private:
    u8 hasFinished();
    u8 activeOption = PLAY_GAME;
    u8 selectedOption = 0;

    TextureRepository *t_texRepo;

    Sprite title[4];
    Sprite background[4];
    Sprite subtitle;
  
    //Array of options [original, active, selected]
    Sprite slot[3];

    //Texts
    Sprite textPlayGame;
    Sprite textSelect;

    //Buttons
    Sprite btnCross;
};

#endif
