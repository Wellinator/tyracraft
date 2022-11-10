#include "ui.hpp"
#include <debug/debug.hpp>
#include "file/file_utils.hpp"

using Tyra::FileUtils;

Ui::Ui() {}

Ui::~Ui() {
  TextureRepository* textureRepository =
      &this->t_renderer->getTextureRepository();

  textureRepository->freeBySprite(crosshair);
  textureRepository->freeBySprite(empty_slots);
  textureRepository->freeBySprite(selected_slot);
  textureRepository->freeBySprite(xp_bar_full);

  for (u8 i = 0; i < 10; i++) textureRepository->freeBySprite(health[i]);
  for (u8 i = 0; i < 10; i++) textureRepository->freeBySprite(hungry[i]);
  for (u8 i = 0; i < 10; i++) textureRepository->freeBySprite(armor[i]);
  for (u8 i = 0; i < 10; i++) textureRepository->freeBySprite(breath[i]);

  for (u8 i = 0; i < INVENTORY_SIZE; i++)
    if (playerInventory[i] != NULL)
      textureRepository->freeBySprite(*playerInventory[i]);
}

void Ui::init(Renderer* t_renderer, ItemRepository* itemRepository,
              Player* t_player) {
  TYRA_ASSERT(t_renderer, "Param 't_renderer' not initialized");
  TYRA_ASSERT(itemRepository, "Param 'itemRepository' not initialized");
  TYRA_ASSERT(t_player, "Param 't_player' not initialized");

  this->t_renderer = t_renderer;
  this->t_itemRepository = itemRepository;
  this->t_player = t_player;

  this->BASE_X_POS = t_renderer->core.getSettings().getWidth() / 2 - 96;
  this->BASE_Y_POS = t_renderer->core.getSettings().getHeight() - 60;

  this->loadlHud();
}

void Ui::update() { this->updateHud(); }

void Ui::renderInventory() {
  this->t_renderer->renderer2D.render(&empty_slots);

  // Draw itens from player inventory
  for (u8 i = 0; i < INVENTORY_SIZE; i++)
    if (playerInventory[i] != NULL)
      this->t_renderer->renderer2D.render(playerInventory[i]);

  this->t_renderer->renderer2D.render(&selected_slot);
}

void Ui::renderCrosshair() { this->t_renderer->renderer2D.render(&crosshair); }

void Ui::renderExperienceBar() {
  this->t_renderer->renderer2D.render(&xp_bar_full);
}

void Ui::renderArmorBar() {
  for (u8 i = 0; i < 10; i++) this->t_renderer->renderer2D.render(&armor[i]);
}

void Ui::renderHealthBar() {
  for (u8 i = 0; i < 10; i++) this->t_renderer->renderer2D.render(&health[i]);
}

void Ui::renderHungerBar() {
  for (u8 i = 0; i < 10; i++) this->t_renderer->renderer2D.render(&hungry[i]);
}

void Ui::renderBreathBar() {
  for (u8 i = 0; i < 10; i++) this->t_renderer->renderer2D.render(&breath[i]);
}

void Ui::loadlHud() {
  const float width = t_renderer->core.getSettings().getWidth();
  const float height = t_renderer->core.getSettings().getHeight();

  std::string crosshairTexPath =
      FileUtils::fromCwd("assets/textures/ui/crosshair.png");
  crosshair.mode = Tyra::MODE_STRETCH;
  crosshair.size.set(8.0f, 8.0f);
  crosshair.position.set(width / 2, height / 2);
  this->t_renderer->core.texture.repository.add(crosshairTexPath)
      ->addLink(crosshair.id);

  for (u8 i = 0; i < 10; i++) {
    std::string armor_fullTexPath =
        FileUtils::fromCwd("assets/hud/armor_full.png");
    armor[i].mode = Tyra::MODE_STRETCH;
    armor[i].size.set(15.0f, 15.0f);
    armor[i].position.set(BASE_X_POS + (i * 8.0f), BASE_Y_POS);
    this->t_renderer->core.texture.repository.add(armor_fullTexPath)
        ->addLink(armor[i].id);

    std::string health_fullTexPath =
        FileUtils::fromCwd("assets/hud/health_full.png");
    health[i].mode = Tyra::MODE_STRETCH;
    health[i].size.set(15.0f, 15.0f);
    health[i].position.set(BASE_X_POS + (i * 8.0f), BASE_Y_POS + 10);
    this->t_renderer->core.texture.repository.add(health_fullTexPath)
        ->addLink(health[i].id);

    std::string breath_fullTexPath =
        FileUtils::fromCwd("assets/hud/breath_full.png");
    breath[i].mode = Tyra::MODE_STRETCH;
    breath[i].size.set(15.0f, 15.0f);
    breath[i].position.set(BASE_X_POS + (HUD_WIDTH - 10.0F * 9.0F) + (i * 8.0f),
                           BASE_Y_POS);
    this->t_renderer->core.texture.repository.add(breath_fullTexPath)
        ->addLink(breath[i].id);

    std::string hungry_fullTexPath =
        FileUtils::fromCwd("assets/hud/hungry_full.png");
    hungry[i].mode = Tyra::MODE_STRETCH;
    hungry[i].size.set(15.0f, 15.0f);
    hungry[i].position.set(BASE_X_POS + (HUD_WIDTH - 10.0F * 9.0F) + (i * 8.0f),
                           BASE_Y_POS + 10);
    this->t_renderer->core.texture.repository.add(hungry_fullTexPath)
        ->addLink(hungry[i].id);
  }

  std::string xp_bar_fullTexPath =
      FileUtils::fromCwd("assets/hud/xp_bar_full.png");
  xp_bar_full.mode = Tyra::MODE_STRETCH;
  xp_bar_full.size.set(256.0F, 8.0f);
  xp_bar_full.position.set(BASE_X_POS, BASE_Y_POS + 21);
  this->t_renderer->core.texture.repository.add(xp_bar_fullTexPath)
      ->addLink(xp_bar_full.id);

  std::string emptySlotsTexPath =
      FileUtils::fromCwd("assets/hud/empty_slots.png");
  empty_slots.mode = Tyra::MODE_STRETCH;
  empty_slots.size.set(256.0F, 32.0f);
  empty_slots.position.set(BASE_X_POS, BASE_Y_POS + 29);
  this->t_renderer->core.texture.repository.add(emptySlotsTexPath)
      ->addLink(empty_slots.id);

  std::string selectedSlotTexPath =
      FileUtils::fromCwd("assets/hud/selected_slot.png");
  selected_slot.mode = Tyra::MODE_STRETCH;
  selected_slot.size.set(31.0f, 31.0f);
  selected_slot.position.set(BASE_X_POS, BASE_Y_POS + 29);
  this->t_renderer->core.texture.repository.add(selectedSlotTexPath)
      ->addLink(selected_slot.id);
}

void Ui::updateHud() {
  if (this->t_player->selectedSlotHasChanged) this->updateSelectedSlot();

  if (this->t_player->inventoryHasChanged) this->updatePlayerInventory();
}

void Ui::updateSelectedSlot() {
  u8 slotIndex = t_player->getSelectedInventorySlot() - 1;
  selected_slot.position.set(
      BASE_X_POS + ((HUD_WIDTH) / 9.1 * slotIndex) - slotIndex,
      BASE_Y_POS + 29);
  t_player->selectedSlotHasChanged = 0;
}

void Ui::updatePlayerInventory() {
  const float BASE_WIDTH = 17.0f;
  const float BASE_HEIGHT = 17.0f;
  const float BASE_X = BASE_X_POS + 2;
  const float BASE_Y = BASE_Y_POS + 29 + 2;

  ItemId* inventoryData = this->t_player->getInventoryData();
  for (u8 i = 0; i < INVENTORY_SIZE; i++) {
    if (inventoryData[i] != ItemId::empty) {
      if (this->playerInventory[i] != NULL) {
        this->t_itemRepository->removeTextureLinkByBlockType(
            inventoryData[i], this->playerInventory[i]->id);
        delete this->playerInventory[i];
        this->playerInventory[i] = NULL;
      }

      Sprite* tempItemSprite = new Sprite();
      tempItemSprite->mode = Tyra::MODE_STRETCH;
      tempItemSprite->size.set(BASE_WIDTH, BASE_HEIGHT);
      tempItemSprite->position.set(BASE_X - i + (HUD_WIDTH / 9.1 * i), BASE_Y);
      this->t_itemRepository->linkTextureByItemType(inventoryData[i],
                                                    tempItemSprite->id);

      this->playerInventory[i] = tempItemSprite;
    } else {
      delete this->playerInventory[i];
      this->playerInventory[i] = NULL;
    }
  }

  t_player->inventoryHasChanged = 0;
}
