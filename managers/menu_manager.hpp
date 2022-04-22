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
#define PLAY_GAME 0
#define ABOUT 1

class MainMenu
{

public:
    MainMenu(TextureRepository *t_texRepo, ScreenSettings *t_screen);
    ~MainMenu();

    void render(Renderer *t_renderer);
    u8 shouldBeDestroyed();

    //Rotating skybox
    Mesh menuSkybox;
private:
    u8 hasFinished();
    u8 selectedOption = PLAY_GAME;

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
