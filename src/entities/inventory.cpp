
#include "entities/inventory.hpp"

Inventory::Inventory(Renderer* renderer, ItemRepository* itemRepository) {
  t_renderer = renderer;
  t_itemRepository = itemRepository;

  init();
}

Inventory::~Inventory() {
  TextureRepository* textureRepository =
      &this->t_renderer->getTextureRepository();

  textureRepository->freeBySprite(selected_slot);
  textureRepository->freeBySprite(itemframe);

  for (u8 i = 0; i < SIZE; i++)
    if (active_slots_prites[i] != NULL)
      textureRepository->freeBySprite(*active_slots_prites[i]);
}

void Inventory::load_active_slots() {
  for (int i = indexOffset; i < (SIZE + indexOffset); i++) {
    if (i < (u8)ItemId::total_of_items) {
      active_slots[i] = static_cast<ItemId>(i + 1);
    } else {
      active_slots[i] = ItemId::empty;
    }
  }
}

void Inventory::load_sprites() {
  BASE_X_POS = (t_renderer->core.getSettings().getWidth() / 2) - 160;
  BASE_Y_POS = (t_renderer->core.getSettings().getHeight() / 2) - 96;

  std::string itemframeTexPath = FileUtils::fromCwd("assets/hud/itemframe.png");
  itemframe.mode = Tyra::MODE_STRETCH;
  itemframe.size.set(320.0F, 192.0f);
  itemframe.position.set(BASE_X_POS, BASE_Y_POS);
  this->t_renderer->core.texture.repository.add(itemframeTexPath)
      ->addLink(itemframe.id);

  std::string selectedSlotTexPath =
      FileUtils::fromCwd("assets/hud/selected_slot.png");
  selected_slot.mode = Tyra::MODE_STRETCH;
  selected_slot.size.set(31.0f, 31.0f);
  selected_slot.position.set(BASE_X_POS + 5, BASE_Y_POS + 5);
  t_renderer->core.texture.repository.add(selectedSlotTexPath)
      ->addLink(selected_slot.id);
}

void Inventory::init() {
  load_sprites();
  load_active_slots();
}

void Inventory::update() {}

void Inventory::render() {
  this->t_renderer->renderer2D.render(&itemframe);
  this->t_renderer->renderer2D.render(&selected_slot);
}
