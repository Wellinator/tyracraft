#include "managers/items_repository.hpp"
#include <renderer/core/texture/models/texture.hpp>
#include "file/file_utils.hpp"
#include <string.h>

using Tyra::FileUtils;
using Tyra::Sprite;
using Tyra::Texture;
using Tyra::Vec4;

ItemRepository::ItemRepository() {}
ItemRepository::~ItemRepository() {}

void ItemRepository::init(Renderer* t_renderer) {
  this->t_renderer = t_renderer;
  this->loadItems();
}

void ItemRepository::loadItems() {
  //----------------Blocks------------------------------
  dirt.id = ITEM_TYPES::dirt;
  dirt.blockId = DIRTY_BLOCK;
  this->items.push_back(&dirt);
  this->t_renderer->core.texture.repository
      .add(FileUtils::fromCwd("assets/textures/items/dirt.png"))
      ->addLink(dirt.sprite.id);

  sand.id = ITEM_TYPES::sand;
  sand.blockId = SAND_BLOCK;
  this->items.push_back(&sand);
  this->t_renderer->core.texture.repository
      .add(FileUtils::fromCwd("assets/textures/items/sand.png"))
      ->addLink(sand.sprite.id);

  stone.id = ITEM_TYPES::stone;
  stone.blockId = STONE_BLOCK;
  this->items.push_back(&stone);
  this->t_renderer->core.texture.repository
      .add(FileUtils::fromCwd("assets/textures/items/stone.png"))
      ->addLink(stone.sprite.id);

  bricks.id = ITEM_TYPES::bricks;
  bricks.blockId = BRICKS_BLOCK;
  this->items.push_back(&bricks);
  this->t_renderer->core.texture.repository
      .add(FileUtils::fromCwd("assets/textures/items/bricks.png"))
      ->addLink(bricks.sprite.id);

  glass.id = ITEM_TYPES::glass;
  glass.blockId = GLASS_BLOCK;
  this->items.push_back(&glass);
  this->t_renderer->core.texture.repository
      .add(FileUtils::fromCwd("assets/textures/items/glass.png"))
      ->addLink(glass.sprite.id);

  //------------------------------Ores and Minerals blocks----------------------
  coal_ore_block.id = ITEM_TYPES::coal_ore_block;
  coal_ore_block.blockId = COAL_ORE_BLOCK;
  this->items.push_back(&coal_ore_block);
  this->t_renderer->core.texture.repository
      .add(FileUtils::fromCwd("assets/textures/items/coal_ore_block.png"))
      ->addLink(coal_ore_block.sprite.id);

  diamond_ore_block.id = ITEM_TYPES::diamond_ore_block;
  diamond_ore_block.blockId = DIAMOND_ORE_BLOCK;
  this->items.push_back(&diamond_ore_block);
  this->t_renderer->core.texture.repository
      .add(FileUtils::fromCwd("assets/textures/items/diamond_ore_block.png"))
      ->addLink(diamond_ore_block.sprite.id);

  iron_ore_block.id = ITEM_TYPES::iron_ore_block;
  iron_ore_block.blockId = IRON_ORE_BLOCK;
  this->items.push_back(&iron_ore_block);
  this->t_renderer->core.texture.repository
      .add(FileUtils::fromCwd("assets/textures/items/iron_ore_block.png"))
      ->addLink(iron_ore_block.sprite.id);

  gold_ore_block.id = ITEM_TYPES::gold_ore_block;
  gold_ore_block.blockId = GOLD_ORE_BLOCK;
  this->items.push_back(&gold_ore_block);
  this->t_renderer->core.texture.repository
      .add(FileUtils::fromCwd("assets/textures/items/gold_ore_block.png"))
      ->addLink(gold_ore_block.sprite.id);

  redstone_ore_block.id = ITEM_TYPES::redstone_ore_block;
  redstone_ore_block.blockId = REDSTONE_ORE_BLOCK;
  this->items.push_back(&redstone_ore_block);
  this->t_renderer->core.texture.repository
      .add(FileUtils::fromCwd("assets/textures/items/redstone_ore_block.png"))
      ->addLink(redstone_ore_block.sprite.id);

  emerald_ore_block.id = ITEM_TYPES::emerald_ore_block;
  emerald_ore_block.blockId = EMERALD_ORE_BLOCK;
  this->items.push_back(&emerald_ore_block);
  this->t_renderer->core.texture.repository
      .add(FileUtils::fromCwd("assets/textures/items/emerald_ore_block.png"))
      ->addLink(emerald_ore_block.sprite.id);

  // -------------- Wood Planks ------------------------
  oak_planks.id = ITEM_TYPES::oak_planks;
  oak_planks.blockId = OAK_PLANKS_BLOCK;
  this->items.push_back(&oak_planks);
  this->t_renderer->core.texture.repository
      .add(FileUtils::fromCwd("assets/textures/items/oak_planks.png"))
      ->addLink(oak_planks.sprite.id);

  spruce_planks.id = ITEM_TYPES::spruce_planks;
  spruce_planks.blockId = SPRUCE_PLANKS_BLOCK;
  this->items.push_back(&spruce_planks);
  this->t_renderer->core.texture.repository
      .add(FileUtils::fromCwd("assets/textures/items/spruce_planks.png"))
      ->addLink(spruce_planks.sprite.id);

  birch_planks.id = ITEM_TYPES::birch_planks;
  birch_planks.blockId = BIRCH_PLANKS_BLOCK;
  this->items.push_back(&birch_planks);
  this->t_renderer->core.texture.repository
      .add(FileUtils::fromCwd("assets/textures/items/birch_planks.png"))
      ->addLink(birch_planks.sprite.id);

  acacia_planks.id = ITEM_TYPES::acacia_planks;
  acacia_planks.blockId = ACACIA_PLANKS_BLOCK;
  this->items.push_back(&acacia_planks);
  this->t_renderer->core.texture.repository
      .add(FileUtils::fromCwd("assets/textures/items/acacia_planks.png"))
      ->addLink(acacia_planks.sprite.id);

  // --------------------------Stone Bricks-------------------------
  stone_brick.id = ITEM_TYPES::stone_brick;
  stone_brick.blockId = STONE_BRICK_BLOCK;
  this->items.push_back(&stone_brick);
  this->t_renderer->core.texture.repository
      .add(FileUtils::fromCwd("assets/textures/items/stone_brick.png"))
      ->addLink(stone_brick.sprite.id);

  cracked_stone_bricks.id = ITEM_TYPES::cracked_stone_bricks;
  cracked_stone_bricks.blockId = CRACKED_STONE_BRICKS_BLOCK;
  this->items.push_back(&cracked_stone_bricks);
  this->t_renderer->core.texture.repository
      .add(FileUtils::fromCwd("assets/textures/items/cracked_stone_bricks.png"))
      ->addLink(cracked_stone_bricks.sprite.id);

  mossy_stone_bricks.id = ITEM_TYPES::mossy_stone_bricks;
  mossy_stone_bricks.blockId = MOSSY_STONE_BRICKS_BLOCK;
  this->items.push_back(&mossy_stone_bricks);
  this->t_renderer->core.texture.repository
      .add(FileUtils::fromCwd("assets/textures/items/mossy_stone_bricks.png"))
      ->addLink(mossy_stone_bricks.sprite.id);

  chiseled_stone_bricks.id = ITEM_TYPES::chiseled_stone_bricks;
  chiseled_stone_bricks.blockId = CHISELED_STONE_BRICKS_BLOCK;
  this->items.push_back(&chiseled_stone_bricks);
  this->t_renderer->core.texture.repository
      .add(
          FileUtils::fromCwd("assets/textures/items/chiseled_stone_bricks.png"))
      ->addLink(chiseled_stone_bricks.sprite.id);

  // --------------------Stripped Oak Wood----------------
  stripped_oak_wood.id = ITEM_TYPES::stripped_oak_wood;
  stripped_oak_wood.blockId = STRIPPED_OAK_WOOD_BLOCK;
  this->items.push_back(&stripped_oak_wood);
  this->t_renderer->core.texture.repository
      .add(FileUtils::fromCwd("assets/textures/items/stripped_oak_wood.png"))
      ->addLink(stripped_oak_wood.sprite.id);
}

Item* ItemRepository::getItemById(ITEM_TYPES& itemId) {
  for (int i = 0; i < items.size(); i++)
    if (items[i]->id == itemId) return items[i];
  return NULL;
}

void ItemRepository::linkTextureByItemType(ITEM_TYPES itemType,
                                           const u32 t_spriteId) {
  this->t_renderer->core.texture.repository
      .getBySpriteId(getSpriteByItemType(itemType).id)
      ->addLink(t_spriteId);
}

void ItemRepository::removeTextureLinkByBlockType(ITEM_TYPES itemType,
                                                  const u32 t_spriteId) {
  Texture* tex = this->t_renderer->core.texture.repository.getBySpriteId(
      getSpriteByItemType(itemType).id);
  s32 index = tex->getIndexOfLink(t_spriteId);
  if (index != -1) tex->removeLinkByIndex(index);
}

Sprite& ItemRepository::getSpriteByItemType(ITEM_TYPES itemType) {
  for (int i = 0; i < items.size(); i++)
    if (items[i]->id == itemType) return items[i]->sprite;
}