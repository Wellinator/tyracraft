#include "entities/player/player_first_person_render_pip.hpp"

PlayerFirstPersonRenderPip::PlayerFirstPersonRenderPip(Player* t_player)
    : PlayerRenderPip(t_player){};

PlayerFirstPersonRenderPip::~PlayerFirstPersonRenderPip(){};

void PlayerFirstPersonRenderPip::render(Renderer* t_render) {
  if (this->t_player->getSelectedInventoryItemType() == ItemId::empty) {
    t_render->renderer3D.usePipeline(&this->t_player->dynpip);
    this->t_player->dynpip.render(this->t_player->armMesh.get(),
                                  &this->t_player->armDynpipOptions);
  }
}
