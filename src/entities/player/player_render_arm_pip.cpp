#include "entities/player/player_render_arm_pip.hpp"

PlayerRenderArmPip::PlayerRenderArmPip(Player* t_player)
    : PlayerRenderPip(t_player){};

PlayerRenderArmPip::~PlayerRenderArmPip(){};

void PlayerRenderArmPip::render(Renderer* t_render) {
  if (this->t_player->getSelectedInventoryItemType() == ItemId::empty) {
    t_render->renderer3D.usePipeline(&this->t_player->dynpip);
    this->t_player->dynpip.render(this->t_player->armMesh.get(),
                                  &this->t_player->armDynpipOptions);
  }
}
