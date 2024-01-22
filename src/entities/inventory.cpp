
#include "entities/inventory.hpp"
#include "utils.hpp"

Inventory::Inventory(Renderer* renderer, ItemRepository* itemRepository) {
  t_renderer = renderer;
  t_itemRepository = itemRepository;

  init();
}

Inventory::~Inventory() {
  TextureRepository* textureRepository =
      &this->t_renderer->getTextureRepository();

  textureRepository->freeBySprite(selector_overlay);
  textureRepository->freeBySprite(itemframe);
  textureRepository->freeBySprite(scroller);
  textureRepository->freeBySprite(btnCross);
  textureRepository->freeBySprite(btnCircle);

  for (size_t i = 0; i < SIZE; i++) {
    if (slots_prites[i]) delete slots_prites[i];
  }
}

void Inventory::init() {
  BASE_X_POS = (t_renderer->core.getSettings().getWidth() / 2) - 196;
  BASE_Y_POS = (t_renderer->core.getSettings().getHeight() / 2) - 196;

  load_sprites();
  load_slots_data();
  load_active_slots_data();
  load_slots_sprites();
}

void Inventory::load_slots_data() {
  const u8 firstItem = (u8)ItemId::dirt;
  const u8 lastItem = (u8)ItemId::total_of_items;
  u8 index = 0;

  for (size_t i = firstItem; i < lastItem; i++) {
    slots_data[index] = static_cast<ItemId>(i);
    index++;
  }
}

void Inventory::load_active_slots_data() {
  const u8 lastItem = (u8)ItemId::total_of_items;
  u8 index = 0;

  for (int i = 0; i < ROWS; i++) {
    for (int j = 0; j < COLS; j++) {
      active_slots_data[index] =
          index < lastItem - 1 ? slots_data[index] : ItemId::empty;
      index++;
    }
  }
}

void Inventory::load_slots_sprites() {
  const float BASE_WIDTH = 32.0f;
  const float BASE_HEIGHT = 32.0f;
  const float BASE_X = BASE_X_POS + 18;
  const float BASE_Y = BASE_Y_POS + 36;
  u8 index = 0;

  for (size_t i = 0; i < ROWS; i++) {
    for (size_t j = 0; j < COLS; j++) {
      if ((u8)active_slots_data[index] > (u8)ItemId::empty &&
          (u8)active_slots_data[index] < (u8)ItemId::total_of_items) {
        if (slots_prites[index]) {
          this->t_itemRepository->removeTextureLinkByBlockType(
              active_slots_data[index], slots_prites[index]->id);
          delete slots_prites[index];
          slots_prites[index] = nullptr;
        }

        Sprite* tempItemSprite = new Sprite();
        tempItemSprite->mode = Tyra::MODE_STRETCH;
        tempItemSprite->size.set(BASE_WIDTH, BASE_HEIGHT);
        tempItemSprite->position.set(BASE_X - j + (COL_WIDTH * j),
                                     BASE_Y - i + (ROW_HEIGHT * i));
        const u8 linkStatus = this->t_itemRepository->linkTextureByItemType(
            active_slots_data[index], tempItemSprite->id);

        if (linkStatus) {
          slots_prites[index] = tempItemSprite;
        } else {
          slots_prites[index] = nullptr;
          delete tempItemSprite;
        }
      } else {
        delete slots_prites[index];
        slots_prites[index] = nullptr;
      }

      index++;
    }
  }
}

void Inventory::load_sprites() {
  itemframe.mode = Tyra::MODE_STRETCH;
  itemframe.size.set(512.0F, 512.0F);
  itemframe.position.set(BASE_X_POS, BASE_Y_POS);
  t_renderer->core.texture.repository
      .add(FileUtils::fromCwd("textures/gui/tab_items.png"))
      ->addLink(itemframe.id);

  selector_overlay.mode = Tyra::MODE_STRETCH;
  selector_overlay.size.set(32.0f, 32.0f);
  selector_overlay.position.set(BASE_X_POS + 18, BASE_Y_POS + 36);
  t_renderer->core.texture.repository
      .add(FileUtils::fromCwd("textures/gui/selector_overlay.png"))
      ->addLink(selector_overlay.id);

  scroller.mode = Tyra::MODE_STRETCH;
  scroller.size.set(32.0f, 32.0f);
  scroller.position.set(BASE_X_POS + 350, BASE_Y_POS + 36);
  t_renderer->core.texture.repository
      .add(FileUtils::fromCwd("textures/gui/scroller.png"))
      ->addLink(scroller.id);

  // Buttons
  btnCross.mode = Tyra::MODE_STRETCH;
  btnCross.size.set(25, 25);
  btnCross.position.set(15, t_renderer->core.getSettings().getHeight() - 40);
  t_renderer->getTextureRepository()
      .add(FileUtils::fromCwd("textures/gui/btn_cross.png"))
      ->addLink(btnCross.id);

  btnCircle.mode = Tyra::MODE_STRETCH;
  btnCircle.size.set(25, 25);
  btnCircle.position.set(190, t_renderer->core.getSettings().getHeight() - 40);
  t_renderer->getTextureRepository()
      .add(FileUtils::fromCwd("textures/gui/btn_circle.png"))
      ->addLink(btnCircle.id);
}

void Inventory::update() {
  // TODO: implements scroller percentage
  selector_overlay_opacity += 3;
  const float percent = Utils::Abs(selector_overlay_opacity - 128.0F) / 128.0F;
  selector_overlay.color.a = percent * 128;
}

void Inventory::render() {
  t_renderer->renderer2D.render(itemframe);

  // Draw itens at each slot
  for (u8 i = 0; i < SIZE; i++)
    if (slots_prites[i]) t_renderer->renderer2D.render(slots_prites[i]);

  t_renderer->renderer2D.render(scroller);
  t_renderer->renderer2D.render(selector_overlay);

  // Texts
  t_renderer->renderer2D.render(btnCross);
  FontManager_printText("Select Item", 35, 407);
  t_renderer->renderer2D.render(btnCircle);
  FontManager_printText("Exit", 210, 407);
}

void Inventory::moveSelectorUp() {
  selector_row--;
  if (selector_row < 0) selector_row = ROWS - 1;
  selector_row %= ROWS;
  selector_overlay_opacity = 0;
  updateSelectorSpritePosition();
}

void Inventory::moveSelectorDown() {
  selector_row++;
  selector_row %= ROWS;
  selector_overlay_opacity = 0;
  updateSelectorSpritePosition();
}

void Inventory::moveSelectorLeft() {
  selector_col--;
  if (selector_col < 0) selector_col = COLS - 1;
  selector_overlay_opacity = 0;
  updateSelectorSpritePosition();
}

void Inventory::moveSelectorRight() {
  selector_col++;
  selector_col %= COLS;
  selector_overlay_opacity = 0;
  updateSelectorSpritePosition();
}

void Inventory::updateSelectorSpritePosition() {
  const float BASE_X = BASE_X_POS + 18;
  const float BASE_Y = BASE_Y_POS + 36;

  selector_overlay.position.set(
      BASE_X - selector_col + (COL_WIDTH * selector_col),
      BASE_Y - selector_row + (ROW_HEIGHT * selector_row));
}

const ItemId Inventory::getSelectedItem() {
  const u8 index = (selector_row * COLS) + selector_col;
  const ItemId result = active_slots_data[index];
  return result;
}
