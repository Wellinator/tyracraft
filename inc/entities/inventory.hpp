#pragma once

#include <tyra>
#include <array>
#include "constants.hpp"
#include "managers/items_repository.hpp"
#include <renderer/core/2d/sprite/sprite.hpp>

#define COLS 6
#define ROWS 6
#define SIZE COLS* ROWS

using Tyra::FileUtils;
using Tyra::Renderer;
using Tyra::Sprite;
using Tyra::TextureRepository;

class Inventory {
 public:
  Inventory(Renderer* renderer, ItemRepository* itemRepository);
  ~Inventory();

  void update();
  void render();

 private:
  float BASE_X_POS;
  float BASE_Y_POS;
  u8 indexOffset = 0;
  std::array<ItemId, SIZE> active_slots;

  Sprite selected_slot;
  Sprite itemframe;
  Sprite* active_slots_prites[SIZE];

  Renderer* t_renderer;
  ItemRepository* t_itemRepository;

  void init();
  void load_sprites();
  void load_active_slots();
};
