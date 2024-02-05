#include "entities/player/player_render_body_pip.hpp"

PlayerRenderBodyPip::PlayerRenderBodyPip(Player* t_player)
    : PlayerRenderPip(t_player){};

PlayerRenderBodyPip::~PlayerRenderBodyPip(){};

void PlayerRenderBodyPip::update(const float& deltaTime, Camera* t_camera) {
  auto& materials = t_player->mesh->materials;
  for (size_t i = 0; i < materials.size(); i++) {
    materials[i]->ambient.set(*t_player->getBaseColorAtPlayerPos());
  }
};

void PlayerRenderBodyPip::render(Renderer* t_render) {
  t_render->renderer3D.usePipeline(&this->t_player->dynpip);
  this->t_player->dynpip.render(this->t_player->mesh.get(),
                                &this->t_player->modelDynpipOptions);
}

void PlayerRenderBodyPip::loadItemDrawData(){};

void PlayerRenderBodyPip::unloadItemDrawData(){};
