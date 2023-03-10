#include "managers/items_repository.hpp"
#include <renderer/core/texture/models/texture.hpp>
#include "file/file_utils.hpp"
#include <string.h>

using Tyra::FileUtils;
using Tyra::Sprite;
using Tyra::Texture;
using Tyra::Vec4;

ItemRepository::ItemRepository() {}

ItemRepository::~ItemRepository() {
  this->t_renderer->getTextureRepository().freeBySprite(dirt.sprite);
  this->t_renderer->getTextureRepository().freeBySprite(sand.sprite);
  this->t_renderer->getTextureRepository().freeBySprite(stone.sprite);
  this->t_renderer->getTextureRepository().freeBySprite(bricks.sprite);
  this->t_renderer->getTextureRepository().freeBySprite(glass.sprite);
  this->t_renderer->getTextureRepository().freeBySprite(coal_ore_block.sprite);
  this->t_renderer->getTextureRepository().freeBySprite(
      diamond_ore_block.sprite);
  this->t_renderer->getTextureRepository().freeBySprite(iron_ore_block.sprite);
  this->t_renderer->getTextureRepository().freeBySprite(gold_ore_block.sprite);
  this->t_renderer->getTextureRepository().freeBySprite(
      redstone_ore_block.sprite);
  this->t_renderer->getTextureRepository().freeBySprite(
      emerald_ore_block.sprite);
  this->t_renderer->getTextureRepository().freeBySprite(oak_planks.sprite);
  this->t_renderer->getTextureRepository().freeBySprite(spruce_planks.sprite);
  this->t_renderer->getTextureRepository().freeBySprite(birch_planks.sprite);
  this->t_renderer->getTextureRepository().freeBySprite(acacia_planks.sprite);
  this->t_renderer->getTextureRepository().freeBySprite(stone_brick.sprite);
  this->t_renderer->getTextureRepository().freeBySprite(
      cracked_stone_bricks.sprite);
  this->t_renderer->getTextureRepository().freeBySprite(
      mossy_stone_bricks.sprite);
  this->t_renderer->getTextureRepository().freeBySprite(
      chiseled_stone_bricks.sprite);
  //   this->t_renderer->getTextureRepository().freeBySprite(wooden_axe.sprite);
}

void ItemRepository::init(Renderer* t_renderer,
                          const std::string& texturePack) {
  TYRA_ASSERT(t_renderer, "Param 't_renderer' not initialized!");
  this->t_renderer = t_renderer;
  this->loadItems(texturePack);
}

void ItemRepository::loadItems(const std::string& texturePack) {
  std::string pathPrefix = "textures/texture_packs/";
  std::string dir = pathPrefix.append(texturePack);

  //----------------Blocks------------------------------
  dirt.id = ItemId::dirt;
  dirt.blockId = Blocks::DIRTY_BLOCK;
  this->items.push_back(&dirt);
  this->t_renderer->getTextureRepository()
      .add(FileUtils::fromCwd(dir + "/items/dirt.png"))
      ->addLink(dirt.sprite.id);

  sand.id = ItemId::sand;
  sand.blockId = Blocks::SAND_BLOCK;
  this->items.push_back(&sand);
  this->t_renderer->getTextureRepository()
      .add(FileUtils::fromCwd(dir + "/items/sand.png"))
      ->addLink(sand.sprite.id);

  stone.id = ItemId::stone;
  stone.blockId = Blocks::STONE_BLOCK;
  this->items.push_back(&stone);
  this->t_renderer->getTextureRepository()
      .add(FileUtils::fromCwd(dir + "/items/stone.png"))
      ->addLink(stone.sprite.id);

  bricks.id = ItemId::bricks;
  bricks.blockId = Blocks::BRICKS_BLOCK;
  this->items.push_back(&bricks);
  this->t_renderer->getTextureRepository()
      .add(FileUtils::fromCwd(dir + "/items/bricks.png"))
      ->addLink(bricks.sprite.id);

  glass.id = ItemId::glass;
  glass.blockId = Blocks::GLASS_BLOCK;
  this->items.push_back(&glass);
  this->t_renderer->getTextureRepository()
      .add(FileUtils::fromCwd(dir + "/items/glass.png"))
      ->addLink(glass.sprite.id);

  //------------------------------Ores and Minerals blocks----------------------
  coal_ore_block.id = ItemId::coal_ore_block;
  coal_ore_block.blockId = Blocks::COAL_ORE_BLOCK;
  this->items.push_back(&coal_ore_block);
  this->t_renderer->getTextureRepository()
      .add(FileUtils::fromCwd(dir + "/items/coal_ore_block.png"))
      ->addLink(coal_ore_block.sprite.id);

  diamond_ore_block.id = ItemId::diamond_ore_block;
  diamond_ore_block.blockId = Blocks::DIAMOND_ORE_BLOCK;
  this->items.push_back(&diamond_ore_block);
  this->t_renderer->getTextureRepository()
      .add(FileUtils::fromCwd(dir + "/items/diamond_ore_block.png"))
      ->addLink(diamond_ore_block.sprite.id);

  iron_ore_block.id = ItemId::iron_ore_block;
  iron_ore_block.blockId = Blocks::IRON_ORE_BLOCK;
  this->items.push_back(&iron_ore_block);
  this->t_renderer->getTextureRepository()
      .add(FileUtils::fromCwd(dir + "/items/iron_ore_block.png"))
      ->addLink(iron_ore_block.sprite.id);

  gold_ore_block.id = ItemId::gold_ore_block;
  gold_ore_block.blockId = Blocks::GOLD_ORE_BLOCK;
  this->items.push_back(&gold_ore_block);
  this->t_renderer->getTextureRepository()
      .add(FileUtils::fromCwd(dir + "/items/gold_ore_block.png"))
      ->addLink(gold_ore_block.sprite.id);

  redstone_ore_block.id = ItemId::redstone_ore_block;
  redstone_ore_block.blockId = Blocks::REDSTONE_ORE_BLOCK;
  this->items.push_back(&redstone_ore_block);
  this->t_renderer->getTextureRepository()
      .add(FileUtils::fromCwd(dir + "/items/redstone_ore_block.png"))
      ->addLink(redstone_ore_block.sprite.id);

  emerald_ore_block.id = ItemId::emerald_ore_block;
  emerald_ore_block.blockId = Blocks::EMERALD_ORE_BLOCK;
  this->items.push_back(&emerald_ore_block);
  this->t_renderer->getTextureRepository()
      .add(FileUtils::fromCwd(dir + "/items/emerald_ore_block.png"))
      ->addLink(emerald_ore_block.sprite.id);

  // -------------- Wood Planks ------------------------
  oak_planks.id = ItemId::oak_planks;
  oak_planks.blockId = Blocks::OAK_PLANKS_BLOCK;
  this->items.push_back(&oak_planks);
  this->t_renderer->getTextureRepository()
      .add(FileUtils::fromCwd(dir + "/items/oak_planks.png"))
      ->addLink(oak_planks.sprite.id);

  spruce_planks.id = ItemId::spruce_planks;
  spruce_planks.blockId = Blocks::SPRUCE_PLANKS_BLOCK;
  this->items.push_back(&spruce_planks);
  this->t_renderer->getTextureRepository()
      .add(FileUtils::fromCwd(dir + "/items/spruce_planks.png"))
      ->addLink(spruce_planks.sprite.id);

  birch_planks.id = ItemId::birch_planks;
  birch_planks.blockId = Blocks::BIRCH_PLANKS_BLOCK;
  this->items.push_back(&birch_planks);
  this->t_renderer->getTextureRepository()
      .add(FileUtils::fromCwd(dir + "/items/birch_planks.png"))
      ->addLink(birch_planks.sprite.id);

  acacia_planks.id = ItemId::acacia_planks;
  acacia_planks.blockId = Blocks::ACACIA_PLANKS_BLOCK;
  this->items.push_back(&acacia_planks);
  this->t_renderer->getTextureRepository()
      .add(FileUtils::fromCwd(dir + "/items/acacia_planks.png"))
      ->addLink(acacia_planks.sprite.id);

  // --------------------------Stone Bricks-------------------------
  stone_brick.id = ItemId::stone_brick;
  stone_brick.blockId = Blocks::STONE_BRICK_BLOCK;
  this->items.push_back(&stone_brick);
  this->t_renderer->getTextureRepository()
      .add(FileUtils::fromCwd(dir + "/items/stone_brick.png"))
      ->addLink(stone_brick.sprite.id);

  cracked_stone_bricks.id = ItemId::cracked_stone_bricks;
  cracked_stone_bricks.blockId = Blocks::CRACKED_STONE_BRICKS_BLOCK;
  this->items.push_back(&cracked_stone_bricks);
  this->t_renderer->getTextureRepository()
      .add(FileUtils::fromCwd(dir + "/items/cracked_stone_bricks.png"))
      ->addLink(cracked_stone_bricks.sprite.id);

  mossy_stone_bricks.id = ItemId::mossy_stone_bricks;
  mossy_stone_bricks.blockId = Blocks::MOSSY_STONE_BRICKS_BLOCK;
  this->items.push_back(&mossy_stone_bricks);
  this->t_renderer->getTextureRepository()
      .add(FileUtils::fromCwd(dir + "/items/mossy_stone_bricks.png"))
      ->addLink(mossy_stone_bricks.sprite.id);

  chiseled_stone_bricks.id = ItemId::chiseled_stone_bricks;
  chiseled_stone_bricks.blockId = Blocks::CHISELED_STONE_BRICKS_BLOCK;
  this->items.push_back(&chiseled_stone_bricks);
  this->t_renderer->getTextureRepository()
      .add(FileUtils::fromCwd(dir + "/items/chiseled_stone_bricks.png"))
      ->addLink(chiseled_stone_bricks.sprite.id);

  // -------------------- Tools ----------------
  //   wooden_axe.id = ItemId::wooden_axe;
  //   wooden_axe.blockId = Blocks::VOID;
  //   this->items.push_back(&wooden_axe);
  //   this->t_renderer->getTextureRepository()
  //       .add(FileUtils::fromCwd(dir.append("textures/texture_pack//items/wooden_axe.png")))
  //       ->addLink(wooden_axe.sprite.id);
}

Item* ItemRepository::getItemById(ItemId& itemId) {
  for (size_t i = 0; i < items.size(); i++)
    if (items[i]->id == itemId) return items[i];
  return NULL;
}

u8 ItemRepository::linkTextureByItemType(const ItemId& itemType,
                                         const u32& t_spriteId) {
  Sprite* baseSprite = getSpriteByItemType(itemType);
  if (baseSprite) {
    this->t_renderer->getTextureRepository()
        .getBySpriteId(baseSprite->id)
        ->addLink(t_spriteId);
    return true;
  }
  return false;
}

void ItemRepository::removeTextureLinkByBlockType(const ItemId& itemType,
                                                  const u32& t_spriteId) {
  Texture* tex = this->t_renderer->getTextureRepository().getBySpriteId(
      getSpriteByItemType(itemType)->id);
  if (tex) {
    const s32 ind = tex->getIndexOfLink(t_spriteId);
    if (ind > -1) tex->removeLinkByIndex(ind);
  }
}

Sprite* ItemRepository::getSpriteByItemType(const ItemId& itemType) {
  for (size_t i = 0; i < items.size(); i++)
    if (items[i]->id == itemType) return &items[i]->sprite;
  return nullptr;
}
