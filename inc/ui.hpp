#pragma once

#include <renderer/renderer.hpp>
#include <renderer/renderer.hpp>
#include <tamtypes.h>
#include "entities/inventory.hpp"
#include "entities/player/player.hpp"
#include "managers/items_repository.hpp"
#include "managers/font/font_manager.hpp"
#include "constants.hpp"

using Tyra::Renderer;
using Tyra::Sprite;
using Tyra::Texture;

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
  void renderInventoryMenu();
  void renderUnderWaterOverlay();

  void loadInventory();
  void unloadInventory();
  inline const u8 isInventoryOpened() { return _isInventoryOpened; };

  void loadlHud();
  inline Inventory* getInvetory() { return creativeInventory; };

 private:
  Renderer* t_renderer;
  ItemRepository* t_itemRepository;
  Player* t_player;

  Sprite crosshair;
  Sprite empty_slots;
  Sprite selected_slot;
  Sprite xp_bar_full;
  Sprite underWaterOverlay;
  Sprite health[10];
  Sprite hungry[10];
  Sprite armor[10];
  Sprite breath[10];
  Texture* healthTexture;
  Texture* hungryTexture;
  Texture* armorTexture;
  Texture* breathTexture;

  // Player inventory ui
  Sprite* playerInventory[HOT_INVENTORY_SIZE] = {NULL, NULL, NULL, NULL, NULL,
                                             NULL, NULL, NULL, NULL};

  float BASE_X_POS;
  float BASE_Y_POS;
  const float HUD_HEIGHT = 64.0F;
  const float HUD_WIDTH = 496.0F;
  const float COL_WIDTH = HUD_WIDTH / 12.4F;

  u8 _isInventoryOpened = false;
  Inventory* creativeInventory = nullptr;

  void updateHud();
  void updatePlayerInventory();
  void updateSelectedSlot();
};
