#pragma once

#include <tyra>
#include <array>
#include "constants.hpp"
#include "managers/font/font_manager.hpp"
#include "managers/items_repository.hpp"
#include <renderer/core/2d/sprite/sprite.hpp>

#define COLS 9
#define ROWS 6
#define SIZE COLS* ROWS

using Tyra::FileUtils;
using Tyra::Renderer;
using Tyra::Renderer2D;
using Tyra::Sprite;
using Tyra::TextureRepository;

class Inventory {
 public:
  Inventory(Renderer* renderer, ItemRepository* itemRepository);
  ~Inventory();

  void update();
  void render(FontManager* t_fontManager);

  void moveSelectorUp();
  void moveSelectorDown();
  void moveSelectorLeft();
  void moveSelectorRight();

 private:
  float BASE_X_POS;
  float BASE_Y_POS;
  const float HUD_WIDTH = 195.0F * 2;
  const float HUD_HEIGHT = 132.0F * 2;
  const float COL_WIDTH = HUD_WIDTH / 10.55;
  const float ROW_HEIGHT = HUD_HEIGHT / 7.1F;

  u8 selector_overlay_opacity = 0;
  s8 selector_col = 0;
  s8 selector_row = 0;

  u8 indexOffset = 0;
  std::array<ItemId, (u8)ItemId::total_of_items> slots_data;
  std::array<ItemId, SIZE> active_slots_data;

  Sprite btnCross;
  Sprite btnCircle;

  Sprite selector_overlay;
  Sprite itemframe;
  Sprite scroller;
  Sprite* slots_prites[SIZE];

  Renderer* t_renderer;
  ItemRepository* t_itemRepository;

  void init();
  void load_sprites();
  void load_slots_data();
  void load_active_slots_data();
  void load_slots_sprites();
  void updateSelectorSpritePosition();
};
