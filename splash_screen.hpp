/*
# ______       ____   ___
#   |     \/   ____| |___|
#   |     |   |   \  |   |
#-----------------------------------------------------------------------
# Copyright 2020, tyra - https://github.com/h4570/tyra
# Licenced under Apache License 2.0
# Sandro Sobczyński <sandro.sobczynski@gmail.com>
*/

#ifndef _SPLASH_SCREEN_
#define _SPLASH_SCREEN_

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

/** 3D camera which follow by 3D object. Can be rotated via pad */
class SplashScreen
{

public:
    SplashScreen(TextureRepository *t_texRepo, ScreenSettings *t_screen);
    ~SplashScreen();

    void render(Renderer *t_renderer);
    u8 shouldBeDestroyed();

private:
    void setBgColorBlack(Renderer *t_renderer);
    void renderTyraSplash(Renderer *t_renderer);
    void renderTyraCraftSplash(Renderer *t_renderer);
    u8 hasFinished();

    TextureRepository *t_texRepo;
    Sprite tyracraft_grid[16];
    Sprite tyra_grid[16];
    u8 alpha = 1;
    u8 isFading = 0;
    u8 hasShowedTyraCraft = 0;
    u8 hasShowedTyra = 0;
};

#endif
