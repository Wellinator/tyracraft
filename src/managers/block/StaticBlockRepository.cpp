#include "managers/block/StaticBlockRepository.hpp"
#include "entities/blocks/BasicBlocks.hpp"
#include "entities/blocks/OreBlocks.hpp"
#include "entities/blocks/PlantBlocks.hpp"
#include "entities/blocks/ExtendedBlocks.hpp"
#include "entities/blocks/SpecialBlocks.hpp"
#include "entities/blocks/SlabBlocks.hpp"
#include <cstring>

std::array<std::unique_ptr<Block>, static_cast<size_t>(Blocks::TOTAL_OF_BLOCKS)>
    blockTemplates{nullptr};

// Definition of the static transparency look-up table.
u8 StaticBlockRepository::s_transparencyTable[256];

StaticBlockRepository::StaticBlockRepository()
    : Singleton<StaticBlockRepository>() {
  initializeBlocks();
}

void StaticBlockRepository::initializeBlocks() {
  TYRA_LOG("StaticBlockRepository::initializeBlocks() - Initializing blocks");
  // Initialize all block templates in the EXACT same order as the original
  // repository This order is CRITICAL for maintaining compatibility

  // VOID and AIR_BLOCK (indices 0 and 1) are left as nullptr
  blockTemplates[static_cast<size_t>(Blocks::VOID)] = nullptr;
  blockTemplates[static_cast<size_t>(Blocks::AIR_BLOCK)] = nullptr;

  // Base Blocks - EXACT order from original loadBlocksInfo()
  blockTemplates[static_cast<size_t>(Blocks::STONE_BLOCK)] =
      std::make_unique<StoneBlock>();
  blockTemplates[static_cast<size_t>(Blocks::GRASS_BLOCK)] =
      std::make_unique<GrassBlock>();
  blockTemplates[static_cast<size_t>(Blocks::DIRTY_BLOCK)] =
      std::make_unique<DirtyBlock>();
  blockTemplates[static_cast<size_t>(Blocks::WATER_BLOCK)] =
      std::make_unique<WaterBlock>();
  blockTemplates[static_cast<size_t>(Blocks::BEDROCK_BLOCK)] =
      std::make_unique<BedrockBlock>();
  blockTemplates[static_cast<size_t>(Blocks::SAND_BLOCK)] =
      std::make_unique<SandBlock>();
  blockTemplates[static_cast<size_t>(Blocks::GLASS_BLOCK)] =
      std::make_unique<GlassBlock>();
  blockTemplates[static_cast<size_t>(Blocks::BRICKS_BLOCK)] =
      std::make_unique<BricksBlock>();
  blockTemplates[static_cast<size_t>(Blocks::GRAVEL_BLOCK)] =
      std::make_unique<GravelBlock>();
  blockTemplates[static_cast<size_t>(Blocks::SANDSTONE_BLOCK)] =
      std::make_unique<SandstoneBlock>();
  blockTemplates[static_cast<size_t>(Blocks::CACTUS_BLOCK)] =
      std::make_unique<CactusBlock>();

  // Face oriented blocks
  blockTemplates[static_cast<size_t>(Blocks::PUMPKIN_BLOCK)] =
      std::make_unique<PumpkinBlock>();

  // Ores and Minerals
  blockTemplates[static_cast<size_t>(Blocks::GOLD_ORE_BLOCK)] =
      std::make_unique<GoldOreBlock>();
  blockTemplates[static_cast<size_t>(Blocks::REDSTONE_ORE_BLOCK)] =
      std::make_unique<RedstoneOreBlock>();
  blockTemplates[static_cast<size_t>(Blocks::IRON_ORE_BLOCK)] =
      std::make_unique<IronOreBlock>();
  blockTemplates[static_cast<size_t>(Blocks::EMERALD_ORE_BLOCK)] =
      std::make_unique<EmeraldOreBlock>();
  blockTemplates[static_cast<size_t>(Blocks::DIAMOND_ORE_BLOCK)] =
      std::make_unique<DiamondOreBlock>();
  blockTemplates[static_cast<size_t>(Blocks::COAL_ORE_BLOCK)] =
      std::make_unique<CoalOreBlock>();

  // Flowers
  blockTemplates[static_cast<size_t>(Blocks::GRASS)] =
      std::make_unique<GrassPlant>();
  blockTemplates[static_cast<size_t>(Blocks::POPPY_FLOWER)] =
      std::make_unique<PoppyFlower>();
  blockTemplates[static_cast<size_t>(Blocks::DANDELION_FLOWER)] =
      std::make_unique<DandelionFlower>();
  blockTemplates[static_cast<size_t>(Blocks::DEAD_BUSH)] =
      std::make_unique<DeadBushBlock>();
  blockTemplates[static_cast<size_t>(Blocks::REEDS_BLOCK)] =
      std::make_unique<ReedsPlant>();
  blockTemplates[static_cast<size_t>(Blocks::TALL_GRASS_BLOCK)] =
      std::make_unique<TallGrassPlant>();

  // Wood Planks
  blockTemplates[static_cast<size_t>(Blocks::OAK_PLANKS_BLOCK)] =
      std::make_unique<OakPlanksBlock>();
  blockTemplates[static_cast<size_t>(Blocks::SPRUCE_PLANKS_BLOCK)] =
      std::make_unique<SprucePlanksBlock>();
  blockTemplates[static_cast<size_t>(Blocks::ACACIA_PLANKS_BLOCK)] =
      std::make_unique<AcaciaPlanksBlock>();
  blockTemplates[static_cast<size_t>(Blocks::BIRCH_PLANKS_BLOCK)] =
      std::make_unique<BirchPlanksBlock>();

  // Light Emissor
  blockTemplates[static_cast<size_t>(Blocks::GLOWSTONE_BLOCK)] =
      std::make_unique<GlowstoneBlock>();
  blockTemplates[static_cast<size_t>(Blocks::JACK_O_LANTERN_BLOCK)] =
      std::make_unique<JackOLanternBlock>();
  blockTemplates[static_cast<size_t>(Blocks::LAVA_BLOCK)] =
      std::make_unique<LavaBlock>();

  // Stone bricks
  blockTemplates[static_cast<size_t>(Blocks::CRACKED_STONE_BRICKS_BLOCK)] =
      std::make_unique<CrackedStoneBricksBlock>();
  blockTemplates[static_cast<size_t>(Blocks::STONE_BRICK_BLOCK)] =
      std::make_unique<StoneBrickBlock>();
  blockTemplates[static_cast<size_t>(Blocks::MOSSY_STONE_BRICKS_BLOCK)] =
      std::make_unique<MossyStoneBricksBlock>();
  blockTemplates[static_cast<size_t>(Blocks::CHISELED_STONE_BRICKS_BLOCK)] =
      std::make_unique<ChiseledStoneBricksBlock>();

  // Wools
  blockTemplates[static_cast<size_t>(Blocks::YELLOW_WOOL)] =
      std::make_unique<YellowWool>();
  blockTemplates[static_cast<size_t>(Blocks::BLUE_WOOL)] =
      std::make_unique<BlueWool>();
  blockTemplates[static_cast<size_t>(Blocks::GREEN_WOOL)] =
      std::make_unique<GreenWool>();
  blockTemplates[static_cast<size_t>(Blocks::ORANGE_WOOL)] =
      std::make_unique<OrangeWool>();
  blockTemplates[static_cast<size_t>(Blocks::PURPLE_WOOL)] =
      std::make_unique<PurpleWool>();
  blockTemplates[static_cast<size_t>(Blocks::RED_WOOL)] =
      std::make_unique<RedWool>();
  blockTemplates[static_cast<size_t>(Blocks::WHITE_WOOL)] =
      std::make_unique<WhiteWool>();
  blockTemplates[static_cast<size_t>(Blocks::BLACK_WOOL)] =
      std::make_unique<BlackWool>();

  // Logs
  blockTemplates[static_cast<size_t>(Blocks::OAK_LOG_BLOCK)] =
      std::make_unique<OakLogBlock>();
  blockTemplates[static_cast<size_t>(Blocks::BIRCH_LOG_BLOCK)] =
      std::make_unique<BirchLogBlock>();

  // Leaves
  blockTemplates[static_cast<size_t>(Blocks::BIRCH_LEAVES_BLOCK)] =
      std::make_unique<BirchLeavesBlock>();
  blockTemplates[static_cast<size_t>(Blocks::OAK_LEAVES_BLOCK)] =
      std::make_unique<OakLeavesBlock>();

  // Items
  blockTemplates[static_cast<size_t>(Blocks::TORCH)] =
      std::make_unique<Torch>();

  // Slabs
  blockTemplates[static_cast<size_t>(Blocks::STONE_SLAB)] =
      std::make_unique<StoneSlab>();
  blockTemplates[static_cast<size_t>(Blocks::BRICKS_SLAB)] =
      std::make_unique<BricksSlab>();
  blockTemplates[static_cast<size_t>(Blocks::OAK_PLANKS_SLAB)] =
      std::make_unique<OakPlanksSlab>();
  blockTemplates[static_cast<size_t>(Blocks::SPRUCE_PLANKS_SLAB)] =
      std::make_unique<SprucePlanksSlab>();
  blockTemplates[static_cast<size_t>(Blocks::ACACIA_PLANKS_SLAB)] =
      std::make_unique<AcaciaPlanksSlab>();
  blockTemplates[static_cast<size_t>(Blocks::BIRCH_PLANKS_SLAB)] =
      std::make_unique<BirchPlanksSlab>();
  blockTemplates[static_cast<size_t>(Blocks::CRACKED_STONE_BRICKS_SLAB)] =
      std::make_unique<CrackedStoneBricksSlab>();
  blockTemplates[static_cast<size_t>(Blocks::STONE_BRICK_SLAB)] =
      std::make_unique<StoneBrickSlab>();
  blockTemplates[static_cast<size_t>(Blocks::MOSSY_STONE_BRICKS_SLAB)] =
      std::make_unique<MossyStoneBricksSlab>();

  // Build the flat transparency table now that all templates are ready.
  buildTransparencyTable();
}

void StaticBlockRepository::buildTransparencyTable() {
  // Zero-fill the whole table (covers IDs >= TOTAL_OF_BLOCKS too).
  memset(s_transparencyTable, 0, sizeof(s_transparencyTable));

  // VOID (0) -> 0: out-of-bounds boundary, treat as solid.
  // AIR_BLOCK (1) -> 1: always transparent.
  s_transparencyTable[static_cast<u8>(Blocks::VOID)] = 0;
  s_transparencyTable[static_cast<u8>(Blocks::AIR_BLOCK)] = 1;

  // Populate from block templates (single hasTransparency() vtable call
  // per block type at startup, not once per face during every chunk build).
  for (size_t i = static_cast<size_t>(Blocks::STONE_BLOCK);
       i < static_cast<size_t>(Blocks::TOTAL_OF_BLOCKS); ++i) {
    if (blockTemplates[i]) {
      s_transparencyTable[i] = blockTemplates[i]->hasTransparency() ? 1u : 0u;
    }
  }

  // LAVA and WATER are handled as transparent for neighbour visibility so that
  // faces between two different liquid types are shown. The liquid-specific
  // face function already deals with same-type adjacency independently.
  s_transparencyTable[static_cast<u8>(Blocks::LAVA_BLOCK)] = 1;
  s_transparencyTable[static_cast<u8>(Blocks::WATER_BLOCK)] = 1;
}

Block* StaticBlockRepository::getBlockTemplate(Blocks blockType) {
  size_t index = static_cast<size_t>(blockType);
  TYRA_ASSERT(index > static_cast<size_t>(Blocks::AIR_BLOCK) &&
                  index < static_cast<size_t>(Blocks::TOTAL_OF_BLOCKS),
              "Invalid block type: ", static_cast<int>(index));

  return blockTemplates[index].get();
}

Block* StaticBlockRepository::getBlockTemplate(u8 blockId) {
  size_t index = blockId;
  TYRA_ASSERT(index > static_cast<size_t>(Blocks::AIR_BLOCK) &&
                  index < static_cast<size_t>(Blocks::TOTAL_OF_BLOCKS),
              "Invalid block type: ", static_cast<int>(index));

  return blockTemplates[index].get();
}

Block* StaticBlockRepository::createBlock(Blocks blockType) {
  Block* template_block = getBlockTemplate(blockType);
  if (!template_block) {
    return nullptr;
  }

  // Create a new instance based on the template type
  switch (blockType) {
    case Blocks::STONE_BLOCK:
      return new StoneBlock();
    case Blocks::GRASS_BLOCK:
      return new GrassBlock();
    case Blocks::DIRTY_BLOCK:
      return new DirtyBlock();
    case Blocks::WATER_BLOCK:
      return new WaterBlock();
    case Blocks::BEDROCK_BLOCK:
      return new BedrockBlock();
    case Blocks::SAND_BLOCK:
      return new SandBlock();
    case Blocks::GLASS_BLOCK:
      return new GlassBlock();
    case Blocks::BRICKS_BLOCK:
      return new BricksBlock();
    case Blocks::GRAVEL_BLOCK:
      return new GravelBlock();
    case Blocks::SANDSTONE_BLOCK:
      return new SandstoneBlock();
    case Blocks::CACTUS_BLOCK:
      return new CactusBlock();
    case Blocks::PUMPKIN_BLOCK:
      return new PumpkinBlock();
    case Blocks::GOLD_ORE_BLOCK:
      return new GoldOreBlock();
    case Blocks::REDSTONE_ORE_BLOCK:
      return new RedstoneOreBlock();
    case Blocks::IRON_ORE_BLOCK:
      return new IronOreBlock();
    case Blocks::EMERALD_ORE_BLOCK:
      return new EmeraldOreBlock();
    case Blocks::DIAMOND_ORE_BLOCK:
      return new DiamondOreBlock();
    case Blocks::COAL_ORE_BLOCK:
      return new CoalOreBlock();
    case Blocks::GRASS:
      return new GrassPlant();
    case Blocks::POPPY_FLOWER:
      return new PoppyFlower();
    case Blocks::DANDELION_FLOWER:
      return new DandelionFlower();
    case Blocks::DEAD_BUSH:
      return new DeadBushBlock();
    case Blocks::REEDS_BLOCK:
      return new ReedsPlant();
    case Blocks::TALL_GRASS_BLOCK:
      return new TallGrassPlant();
    case Blocks::OAK_PLANKS_BLOCK:
      return new OakPlanksBlock();
    case Blocks::SPRUCE_PLANKS_BLOCK:
      return new SprucePlanksBlock();
    case Blocks::ACACIA_PLANKS_BLOCK:
      return new AcaciaPlanksBlock();
    case Blocks::BIRCH_PLANKS_BLOCK:
      return new BirchPlanksBlock();
    case Blocks::GLOWSTONE_BLOCK:
      return new GlowstoneBlock();
    case Blocks::JACK_O_LANTERN_BLOCK:
      return new JackOLanternBlock();
    case Blocks::LAVA_BLOCK:
      return new LavaBlock();
    case Blocks::CRACKED_STONE_BRICKS_BLOCK:
      return new CrackedStoneBricksBlock();
    case Blocks::STONE_BRICK_BLOCK:
      return new StoneBrickBlock();
    case Blocks::MOSSY_STONE_BRICKS_BLOCK:
      return new MossyStoneBricksBlock();
    case Blocks::CHISELED_STONE_BRICKS_BLOCK:
      return new ChiseledStoneBricksBlock();
    case Blocks::YELLOW_WOOL:
      return new YellowWool();
    case Blocks::BLUE_WOOL:
      return new BlueWool();
    case Blocks::GREEN_WOOL:
      return new GreenWool();
    case Blocks::ORANGE_WOOL:
      return new OrangeWool();
    case Blocks::PURPLE_WOOL:
      return new PurpleWool();
    case Blocks::RED_WOOL:
      return new RedWool();
    case Blocks::WHITE_WOOL:
      return new WhiteWool();
    case Blocks::BLACK_WOOL:
      return new BlackWool();
    case Blocks::OAK_LOG_BLOCK:
      return new OakLogBlock();
    case Blocks::BIRCH_LOG_BLOCK:
      return new BirchLogBlock();
    case Blocks::BIRCH_LEAVES_BLOCK:
      return new BirchLeavesBlock();
    case Blocks::OAK_LEAVES_BLOCK:
      return new OakLeavesBlock();
    case Blocks::TORCH:
      return new Torch();
    case Blocks::STONE_SLAB:
      return new StoneSlab();
    case Blocks::BRICKS_SLAB:
      return new BricksSlab();
    case Blocks::OAK_PLANKS_SLAB:
      return new OakPlanksSlab();
    case Blocks::SPRUCE_PLANKS_SLAB:
      return new SprucePlanksSlab();
    case Blocks::ACACIA_PLANKS_SLAB:
      return new AcaciaPlanksSlab();
    case Blocks::BIRCH_PLANKS_SLAB:
      return new BirchPlanksSlab();
    case Blocks::CRACKED_STONE_BRICKS_SLAB:
      return new CrackedStoneBricksSlab();
    case Blocks::STONE_BRICK_SLAB:
      return new StoneBrickSlab();
    case Blocks::MOSSY_STONE_BRICKS_SLAB:
      return new MossyStoneBricksSlab();
    default:
      return nullptr;
  }
}

bool StaticBlockRepository::isBlockTransparent(Blocks blockType) {
  Block* template_block = getBlockTemplate(blockType);
  return template_block ? template_block->hasTransparency() : false;
}
