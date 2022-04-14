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

class MainMenu
{

public:
    MainMenu(TextureRepository *t_texRepo, ScreenSettings *t_screen);
    ~MainMenu();

    void render(Renderer *t_renderer);
    u8 shouldBeDestroyed();

private:
    u8 hasFinished();

    TextureRepository *t_texRepo;
    // Mesh menuSkybox;

    //Array of options [original, hovered, selected]
    Sprite slot[3];

    //Array of options [original, hovered]
    Sprite optionPlay[2];
    Sprite optionAbout[2];
};

#endif
