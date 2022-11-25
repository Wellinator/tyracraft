#include "entities/player/player_third_person_render_pip.hpp"

PlayerThirdPersonRenderPip::PlayerThirdPersonRenderPip(Player* t_player)
    : PlayerRenderPip(t_player){};

PlayerThirdPersonRenderPip::~PlayerThirdPersonRenderPip(){};

void PlayerThirdPersonRenderPip::render(Renderer* t_render) {
  t_render->renderer3D.usePipeline(&this->t_player->dynpip);
  this->t_player->dynpip.render(this->t_player->mesh.get(),
                                &this->t_player->modelDynpipOptions);
}
