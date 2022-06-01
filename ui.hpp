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

    void loadlHud();
    // void renderHud(Renderer *t_renderer, GAME_MODE gameMode, Player* t_player);

private:
    TextureRepository *t_texRepo;
    
    Sprite crosshair;
    Sprite empty_slots;
    Sprite selected_slot;
    Sprite xp_bar_full;
    Sprite health[10];
    Sprite hungry[10];
    Sprite armor[10];
    Sprite breath[10];
};

#endif