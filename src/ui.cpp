#include "ui.hpp"
#include <debug/debug.hpp>
#include "file/file_utils.hpp"

using Tyra::FileUtils;

Ui::Ui() {}

Ui::~Ui() {}

void Ui::init(Renderer* t_renderer, ItemRepository* itemRepository,
              Player* t_player) {
  this->t_renderer = t_renderer;
  this->t_itemRepository = itemRepository;
  this->t_player = t_player;
  this->loadlHud();
}

void Ui::update() { this->updateHud(); }

void Ui::render() {
  this->t_renderer->renderer2D.render(&crosshair);
  this->t_renderer->renderer2D.render(&empty_slots);
  this->t_renderer->renderer2D.render(&selected_slot);
  this->t_renderer->renderer2D.render(&xp_bar_full);
  for (u8 i = 0; i < 10; i++) {
    // TODO: Daw only when wearing an armor
    this->t_renderer->renderer2D.render(&armor[i]);
    this->t_renderer->renderer2D.render(&health[i]);
    this->t_renderer->renderer2D.render(&hungry[i]);
    this->t_renderer->renderer2D.render(&breath[i]);
  }

  // Draw itens from player inventory
  for (u8 i = 0; i < INVENTORY_SIZE; i++)
    if (playerInventory[i] != NULL)
      this->t_renderer->renderer2D.render(playerInventory[i]);
}

void Ui::loadlHud() {
  const float width = t_renderer->core.getSettings().getWidth();
  const float height = t_renderer->core.getSettings().getHeight();

  std::string crosshairTexPath =
      FileUtils::fromCwd("assets/textures/ui/crosshair.png");
  crosshair.setMode(Tyra::MODE_STRETCH);
  crosshair.size.set(8.0f, 8.0f);
  crosshair.position.set(width / 2, height / 2);
  this->t_renderer->core.texture.repository.add(crosshairTexPath)
      ->addLink(crosshair.getId());

  // Items slots
  std::string emptySlotsTexPath =
      FileUtils::fromCwd("assets/hud/empty_slots.png");
  empty_slots.setMode(Tyra::MODE_STRETCH);
  empty_slots.size.set(192.0f, 28.0f);
  empty_slots.position.set(width / 2 - 96.0f, height - 37);
  this->t_renderer->core.texture.repository.add(emptySlotsTexPath)
      ->addLink(empty_slots.getId());

  std::string selectedSlotTexPath =
      FileUtils::fromCwd("assets/hud/selected_slot.png");
  selected_slot.setMode(Tyra::MODE_STRETCH);
  selected_slot.size.set(22.0f, 28.0f);
  selected_slot.position.set(width / 2 - 96.0f, height - 37);
  this->t_renderer->core.texture.repository.add(selectedSlotTexPath)
      ->addLink(selected_slot.getId());

  std::string xp_bar_fullTexPath =
      FileUtils::fromCwd("assets/hud/xp_bar_full.png");
  xp_bar_full.setMode(Tyra::MODE_STRETCH);
  xp_bar_full.size.set(192.0f, 7.0f);
  xp_bar_full.position.set(width / 2 - 96.0f, height - 47);
  this->t_renderer->core.texture.repository.add(xp_bar_fullTexPath)
      ->addLink(xp_bar_full.getId());

  for (u8 i = 0; i < 10; i++) {
    std::string health_fullTexPath =
        FileUtils::fromCwd("assets/hud/health_full.png");
    health[i].setMode(Tyra::MODE_STRETCH);
    health[i].size.set(8.0f, 8.0f);
    health[i].position.set(width / 2 - 96.0f + (i * 8.0f), height - 57.0f);
    this->t_renderer->core.texture.repository.add(health_fullTexPath)
        ->addLink(health[i].getId());

    std::string armor_fullTexPath =
        FileUtils::fromCwd("assets/hud/armor_full.png");
    armor[i].setMode(Tyra::MODE_STRETCH);
    armor[i].size.set(8.0f, 8.0f);
    armor[i].position.set(width / 2 - 96.0f + (i * 8.0f), height - 67.0f);
    this->t_renderer->core.texture.repository.add(armor_fullTexPath)
        ->addLink(armor[i].getId());

    std::string hungry_fullTexPath =
        FileUtils::fromCwd("assets/hud/hungry_full.png");
    hungry[i].setMode(Tyra::MODE_STRETCH);
    hungry[i].size.set(8.0f, 8.0f);
    hungry[i].position.set(272.0f + (i * 8.0f), height - 57.0f);
    this->t_renderer->core.texture.repository.add(hungry_fullTexPath)
        ->addLink(hungry[i].getId());

    std::string breath_fullTexPath =
        FileUtils::fromCwd("assets/hud/breath_full.png");
    breath[i].setMode(Tyra::MODE_STRETCH);
    breath[i].size.set(8.0f, 8.0f);
    breath[i].position.set(272.0f + (i * 8.0f), height - 67.0f);
    this->t_renderer->core.texture.repository.add(breath_fullTexPath)
        ->addLink(breath[i].getId());
  }
}

void Ui::updateHud() {
  if (this->t_player->selectedSlotHasChanged) this->updateSelectedSlot();

  if (this->t_player->inventoryHasChanged) this->updatePlayerInventory();
}

void Ui::updateSelectedSlot() {
  u8 slotIndex = t_player->getSelectedInventorySlot() - 1;
  selected_slot.position.set(
      FIRST_SLOT_X_POS + (192.0f / INVENTORY_SIZE * slotIndex),
      FIRST_SLOT_Y_POS);
  t_player->selectedSlotHasChanged = 0;
}

void Ui::updatePlayerInventory() {
  const float BASE_WIDTH = 14.0f;
  const float BASE_HEIGHT = 14.0f;
  const float BASE_X = FIRST_SLOT_X_POS + 4;
  const float BASE_Y = FIRST_SLOT_Y_POS + 7;

  ITEM_TYPES* inventoryData = this->t_player->getInventoryData();
  for (u8 i = 0; i < INVENTORY_SIZE; i++) {
    if (inventoryData[i] != ITEM_TYPES::empty) {
      if (this->playerInventory[i] != NULL) {
        printf("Item on inv at %i, clearing...\n", i);
        this->t_itemRepository->removeTextureLinkByBlockType(
            inventoryData[i], this->playerInventory[i]->getId());
        delete this->playerInventory[i];
        this->playerInventory[i] = NULL;
      }

      Sprite* tempItemSprite = new Sprite();
      tempItemSprite->setMode(Tyra::MODE_STRETCH);
      tempItemSprite->size.set(BASE_WIDTH, BASE_HEIGHT);
      tempItemSprite->position.set(BASE_X + (empty_slots.size.x / 9 * i),
                                   BASE_Y);
      this->t_itemRepository->linkTextureByItemType(inventoryData[i],
                                                    tempItemSprite->getId());

      this->playerInventory[i] = tempItemSprite;
    }
  }

  t_player->inventoryHasChanged = 0;
}
