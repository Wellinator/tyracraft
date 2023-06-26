#include "entities/player/player_render_body_pip.hpp"

PlayerRenderBodyPip::PlayerRenderBodyPip(Player* t_player)
    : PlayerRenderPip(t_player){};

PlayerRenderBodyPip::~PlayerRenderBodyPip(){};

void PlayerRenderBodyPip::render(Renderer* t_render) {
  t_render->renderer3D.usePipeline(&this->t_player->dynpip);
  this->t_player->dynpip.render(this->t_player->mesh.get(),
                                &this->t_player->modelDynpipOptions);
}
