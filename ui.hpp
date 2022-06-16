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

    void update(Player *t_player);
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

    const float FIRST_SLOT_X_POS = 224.0f;
    const float FIRST_SLOT_Y_POS = 446.0f;

    void updateHud(Player *t_player);
};

#endif