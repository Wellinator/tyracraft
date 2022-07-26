#pragma once

#include <renderer/renderer.hpp>
#include <renderer/renderer.hpp>
#include <tamtypes.h>
#include "entities/player.hpp"
#include "managers/items_repository.hpp"
#include "contants.hpp"

using Tyra::Renderer;
using Tyra::Sprite;

class Ui {
 public:
  Ui();
  ~Ui();

  void init(Renderer* t_renderer, ItemRepository* itemRepository,
            Player* t_player);
  void update();
  void render();

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

  const float FIRST_SLOT_X_POS = 224.0f;
  const float FIRST_SLOT_Y_POS = 446.0f;

  void updateHud();
  void updatePlayerInventory();
  void updateSelectedSlot();
};
