#pragma once

#include <renderer/renderer.hpp>
#include <renderer/renderer.hpp>
#include <tamtypes.h>
#include "entities/player.hpp"
#include "managers/items_repository.hpp"
#include "constants.hpp"

using Tyra::Renderer;
using Tyra::Sprite;

class Ui {
 public:
  Ui();
  ~Ui();

  void init(Renderer* t_renderer, ItemRepository* itemRepository,
            Player* t_player);
  void update();

  void renderCrosshair();
  void renderInventory();
  void renderExperienceBar();
  void renderArmorBar();
  void renderHealthBar();
  void renderHungerBar();
  void renderBreathBar();

  void loadlHud();

 private:
  Renderer* t_renderer;
  ItemRepository* t_itemRepository;
  Player* t_player;

  Sprite crosshair;
  Sprite empty_slots;
  Sprite selected_slot;
  Sprite xp_bar_full;
  Sprite health[10];
  Sprite hungry[10];
  Sprite armor[10];
  Sprite breath[10];

  // Player inventory ui
  Sprite* playerInventory[INVENTORY_SIZE] = {NULL, NULL, NULL, NULL, NULL,
                                             NULL, NULL, NULL, NULL};

  float BASE_X_POS;
  float BASE_Y_POS;
  const float HUD_WIDTH = 192.0F;


  void updateHud();
  void updatePlayerInventory();
  void updateSelectedSlot();
};
