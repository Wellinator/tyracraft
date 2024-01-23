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
  this->t_renderer->getTextureRepository().freeBySprite(gravel.sprite);
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
  this->t_renderer->getTextureRepository().freeBySprite(poppy_flower.sprite);
  this->t_renderer->getTextureRepository().freeBySprite(
      dandelion_flower.sprite);

  this->t_renderer->getTextureRepository().freeBySprite(glowstone.sprite);
  this->t_renderer->getTextureRepository().freeBySprite(jack_o_lantern.sprite);
  this->t_renderer->getTextureRepository().freeBySprite(pumpkin.sprite);

  this->t_renderer->getTextureRepository().freeBySprite(oak_planks.sprite);
  this->t_renderer->getTextureRepository().freeBySprite(spruce_planks.sprite);
  this->t_renderer->getTextureRepository().freeBySprite(birch_planks.sprite);
  this->t_renderer->getTextureRepository().freeBySprite(acacia_planks.sprite);
  this->t_renderer->getTextureRepository().freeBySprite(oak_log.sprite);
  this->t_renderer->getTextureRepository().freeBySprite(birch_log.sprite);
  this->t_renderer->getTextureRepository().freeBySprite(stone_brick.sprite);
  this->t_renderer->getTextureRepository().freeBySprite(
      cracked_stone_bricks.sprite);
  this->t_renderer->getTextureRepository().freeBySprite(
      mossy_stone_bricks.sprite);
  this->t_renderer->getTextureRepository().freeBySprite(
      chiseled_stone_bricks.sprite);

  this->t_renderer->getTextureRepository().freeBySprite(yellow_concrete.sprite);
  this->t_renderer->getTextureRepository().freeBySprite(blue_concrete.sprite);
  this->t_renderer->getTextureRepository().freeBySprite(green_concrete.sprite);
  this->t_renderer->getTextureRepository().freeBySprite(orange_concrete.sprite);
  this->t_renderer->getTextureRepository().freeBySprite(purple_concrete.sprite);
  this->t_renderer->getTextureRepository().freeBySprite(red_concrete.sprite);
  this->t_renderer->getTextureRepository().freeBySprite(white_concrete.sprite);
  this->t_renderer->getTextureRepository().freeBySprite(black_concrete.sprite);

  this->t_renderer->getTextureRepository().freeBySprite(water_bucket.sprite);
  this->t_renderer->getTextureRepository().freeBySprite(lava_bucket.sprite);
  this->t_renderer->getTextureRepository().freeBySprite(torch.sprite);

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

  gravel.id = ItemId::gravel;
  gravel.blockId = Blocks::GRAVEL_BLOCK;
  this->items.push_back(&gravel);
  this->t_renderer->getTextureRepository()
      .add(FileUtils::fromCwd(dir + "/items/gravel.png"))
      ->addLink(gravel.sprite.id);

  glass.id = ItemId::glass;
  glass.blockId = Blocks::GLASS_BLOCK;
  this->items.push_back(&glass);
  this->t_renderer->getTextureRepository()
      .add(FileUtils::fromCwd(dir + "/items/glass.png"))
      ->addLink(glass.sprite.id);

  pumpkin.id = ItemId::pumpkin;
  pumpkin.blockId = Blocks::PUMPKIN_BLOCK;
  this->items.push_back(&pumpkin);
  this->t_renderer->getTextureRepository()
      .add(FileUtils::fromCwd(dir + "/items/pumpkin.png"))
      ->addLink(pumpkin.sprite.id);

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

  // -------------- Flowers ------------------------
  poppy_flower.id = ItemId::poppy_flower;
  poppy_flower.blockId = Blocks::POPPY_FLOWER;
  this->items.push_back(&poppy_flower);
  this->t_renderer->getTextureRepository()
      .add(FileUtils::fromCwd(dir + "/items/poppy_flower.png"))
      ->addLink(poppy_flower.sprite.id);

  dandelion_flower.id = ItemId::dandelion_flower;
  dandelion_flower.blockId = Blocks::DANDELION_FLOWER;
  this->items.push_back(&dandelion_flower);
  this->t_renderer->getTextureRepository()
      .add(FileUtils::fromCwd(dir + "/items/dandelion_flower.png"))
      ->addLink(dandelion_flower.sprite.id);

  // -------------- Light Emissors ------------------------
  glowstone.id = ItemId::glowstone;
  glowstone.blockId = Blocks::GLOWSTONE_BLOCK;
  this->items.push_back(&glowstone);
  this->t_renderer->getTextureRepository()
      .add(FileUtils::fromCwd(dir + "/items/glowstone.png"))
      ->addLink(glowstone.sprite.id);

  jack_o_lantern.id = ItemId::jack_o_lantern;
  jack_o_lantern.blockId = Blocks::JACK_O_LANTERN_BLOCK;
  this->items.push_back(&jack_o_lantern);
  this->t_renderer->getTextureRepository()
      .add(FileUtils::fromCwd(dir + "/items/jack_o_lantern.png"))
      ->addLink(jack_o_lantern.sprite.id);

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

  // -------------- Wood Log ------------------------
  oak_log.id = ItemId::oak_log;
  oak_log.blockId = Blocks::OAK_LOG_BLOCK;
  this->items.push_back(&oak_log);
  this->t_renderer->getTextureRepository()
      .add(FileUtils::fromCwd(dir + "/items/oak_log.png"))
      ->addLink(oak_log.sprite.id);

  birch_log.id = ItemId::birch_log;
  birch_log.blockId = Blocks::BIRCH_LOG_BLOCK;
  this->items.push_back(&birch_log);
  this->t_renderer->getTextureRepository()
      .add(FileUtils::fromCwd(dir + "/items/birch_log.png"))
      ->addLink(birch_log.sprite.id);

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

  // --------------------------Concretes-------------------------
  yellow_concrete.id = ItemId::yellow_concrete;
  yellow_concrete.blockId = Blocks::YELLOW_CONCRETE;
  this->items.push_back(&yellow_concrete);
  this->t_renderer->getTextureRepository()
      .add(FileUtils::fromCwd(dir + "/items/yellow_concrete.png"))
      ->addLink(yellow_concrete.sprite.id);

  blue_concrete.id = ItemId::blue_concrete;
  blue_concrete.blockId = Blocks::BLUE_CONCRETE;
  this->items.push_back(&blue_concrete);
  this->t_renderer->getTextureRepository()
      .add(FileUtils::fromCwd(dir + "/items/blue_concrete.png"))
      ->addLink(blue_concrete.sprite.id);

  green_concrete.id = ItemId::green_concrete;
  green_concrete.blockId = Blocks::GREEN_CONCRETE;
  this->items.push_back(&green_concrete);
  this->t_renderer->getTextureRepository()
      .add(FileUtils::fromCwd(dir + "/items/green_concrete.png"))
      ->addLink(green_concrete.sprite.id);

  orange_concrete.id = ItemId::orange_concrete;
  orange_concrete.blockId = Blocks::ORANGE_CONCRETE;
  this->items.push_back(&orange_concrete);
  this->t_renderer->getTextureRepository()
      .add(FileUtils::fromCwd(dir + "/items/orange_concrete.png"))
      ->addLink(orange_concrete.sprite.id);

  purple_concrete.id = ItemId::purple_concrete;
  purple_concrete.blockId = Blocks::PURPLE_CONCRETE;
  this->items.push_back(&purple_concrete);
  this->t_renderer->getTextureRepository()
      .add(FileUtils::fromCwd(dir + "/items/purple_concrete.png"))
      ->addLink(purple_concrete.sprite.id);

  red_concrete.id = ItemId::red_concrete;
  red_concrete.blockId = Blocks::RED_CONCRETE;
  this->items.push_back(&red_concrete);
  this->t_renderer->getTextureRepository()
      .add(FileUtils::fromCwd(dir + "/items/red_concrete.png"))
      ->addLink(red_concrete.sprite.id);

  white_concrete.id = ItemId::white_concrete;
  white_concrete.blockId = Blocks::WHITE_CONCRETE;
  this->items.push_back(&white_concrete);
  this->t_renderer->getTextureRepository()
      .add(FileUtils::fromCwd(dir + "/items/white_concrete.png"))
      ->addLink(white_concrete.sprite.id);

  black_concrete.id = ItemId::black_concrete;
  black_concrete.blockId = Blocks::BLACK_CONCRETE;
  this->items.push_back(&black_concrete);
  this->t_renderer->getTextureRepository()
      .add(FileUtils::fromCwd(dir + "/items/black_concrete.png"))
      ->addLink(black_concrete.sprite.id);

  // --------------------------Items-------------------------
  water_bucket.id = ItemId::water_bucket;
  water_bucket.blockId = Blocks::WATER_BLOCK;
  this->items.push_back(&water_bucket);
  this->t_renderer->getTextureRepository()
      .add(FileUtils::fromCwd(dir + "/items/water_bucket.png"))
      ->addLink(water_bucket.sprite.id);

  lava_bucket.id = ItemId::lava_bucket;
  lava_bucket.blockId = Blocks::LAVA_BLOCK;
  this->items.push_back(&lava_bucket);
  this->t_renderer->getTextureRepository()
      .add(FileUtils::fromCwd(dir + "/items/lava_bucket.png"))
      ->addLink(lava_bucket.sprite.id);

  torch.id = ItemId::torch;
  torch.blockId = Blocks::TORCH;
  this->items.push_back(&torch);
  this->t_renderer->getTextureRepository()
      .add(FileUtils::fromCwd(dir + "/items/torch.png"))
      ->addLink(torch.sprite.id);

  // --------------------------Slabs-------------------------
  stone_slab.id = ItemId::stone_slab;
  stone_slab.blockId = Blocks::STONE_SLAB;
  this->items.push_back(&stone_slab);
  this->t_renderer->getTextureRepository()
      .add(FileUtils::fromCwd(dir + "/items/stone_slab.png"))
      ->addLink(stone_slab.sprite.id);

  bricks_slab.id = ItemId::bricks_slab;
  bricks_slab.blockId = Blocks::BRICKS_SLAB;
  this->items.push_back(&bricks_slab);
  this->t_renderer->getTextureRepository()
      .add(FileUtils::fromCwd(dir + "/items/bricks_slab.png"))
      ->addLink(bricks_slab.sprite.id);

  oak_slab.id = ItemId::oak_slab;
  oak_slab.blockId = Blocks::OAK_PLANKS_SLAB;
  this->items.push_back(&oak_slab);
  this->t_renderer->getTextureRepository()
      .add(FileUtils::fromCwd(dir + "/items/oak_slab.png"))
      ->addLink(oak_slab.sprite.id);

  spruce_slab.id = ItemId::spruce_slab;
  spruce_slab.blockId = Blocks::SPRUCE_PLANKS_SLAB;
  this->items.push_back(&spruce_slab);
  this->t_renderer->getTextureRepository()
      .add(FileUtils::fromCwd(dir + "/items/spruce_slab.png"))
      ->addLink(spruce_slab.sprite.id);

  birch_slab.id = ItemId::birch_slab;
  birch_slab.blockId = Blocks::BIRCH_PLANKS_SLAB;
  this->items.push_back(&birch_slab);
  this->t_renderer->getTextureRepository()
      .add(FileUtils::fromCwd(dir + "/items/birch_slab.png"))
      ->addLink(birch_slab.sprite.id);

  acacia_slab.id = ItemId::acacia_slab;
  acacia_slab.blockId = Blocks::ACACIA_PLANKS_SLAB;
  this->items.push_back(&acacia_slab);
  this->t_renderer->getTextureRepository()
      .add(FileUtils::fromCwd(dir + "/items/acacia_slab.png"))
      ->addLink(acacia_slab.sprite.id);

  stone_brick_slab.id = ItemId::stone_brick_slab;
  stone_brick_slab.blockId = Blocks::STONE_BRICK_SLAB;
  this->items.push_back(&stone_brick_slab);
  this->t_renderer->getTextureRepository()
      .add(FileUtils::fromCwd(dir + "/items/stone_brick_slab.png"))
      ->addLink(stone_brick_slab.sprite.id);

  cracked_stone_bricks_slab.id = ItemId::cracked_stone_bricks_slab;
  cracked_stone_bricks_slab.blockId = Blocks::CRACKED_STONE_BRICKS_SLAB;
  this->items.push_back(&cracked_stone_bricks_slab);
  this->t_renderer->getTextureRepository()
      .add(FileUtils::fromCwd(dir + "/items/cracked_stone_bricks_slab.png"))
      ->addLink(cracked_stone_bricks_slab.sprite.id);

  mossy_stone_bricks_slab.id = ItemId::mossy_stone_bricks_slab;
  mossy_stone_bricks_slab.blockId = Blocks::MOSSY_STONE_BRICKS_SLAB;
  this->items.push_back(&mossy_stone_bricks_slab);
  this->t_renderer->getTextureRepository()
      .add(FileUtils::fromCwd(dir + "/items/mossy_stone_bricks_slab.png"))
      ->addLink(mossy_stone_bricks_slab.sprite.id);

  chiseled_stone_bricks_slab.id = ItemId::chiseled_stone_bricks_slab;
  chiseled_stone_bricks_slab.blockId = Blocks::CHISELED_STONE_BRICKS_SLAB;
  this->items.push_back(&chiseled_stone_bricks_slab);
  this->t_renderer->getTextureRepository()
      .add(FileUtils::fromCwd(dir + "/items/chiseled_stone_bricks_slab.png"))
      ->addLink(chiseled_stone_bricks_slab.sprite.id);

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
  return nullptr;
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
