#include "managers/model_builder.hpp"

void ModelBuilder_BuildModel(Block* t_block, LevelMap* t_terrain) {
  switch (t_block->type) {
    case Blocks::TORCH:
      ModelBuilder_TorchModel(t_block, t_terrain);
      break;

    case Blocks::WATER_BLOCK:
    case Blocks::LAVA_BLOCK:
      ModelBuilder_NoRotationModel(t_block);
      break;

    default:
      ModelBuilder_DefaultModel(t_block, t_terrain);
      break;
  }
}

void ModelBuilder_DefaultModel(Block* t_block, LevelMap* t_terrain) {
  Vec4 pos;
  GetXYZFromPos(&t_block->offset, &pos);

  const auto orientation =
      GetBlockOrientationDataFromMap(t_terrain, pos.x, pos.y, pos.z);

  t_block->model.identity();

  switch (orientation) {
    case BlockOrientation::North:
      t_block->model.rotateY(_90DEGINRAD);
      break;
    case BlockOrientation::South:
      t_block->model.rotateY(_270DEGINRAD);
      break;
    case BlockOrientation::West:
      t_block->model.rotateY(_180DEGINRAD);
      break;
    case BlockOrientation::Top:
    default:
      break;
  }

  t_block->model.scale(BLOCK_SIZE);
  t_block->model.translate(t_block->position);
}

void ModelBuilder_NoRotationModel(Block* t_block) {
  t_block->model.identity();
  t_block->model.scale(BLOCK_SIZE);
  t_block->model.translate(t_block->position);
}

void ModelBuilder_TorchModel(Block* t_block, LevelMap* t_terrain) {
  Vec4 pos;
  GetXYZFromPos(&t_block->offset, &pos);

  const auto orientation =
      GetTorchOrientationDataFromMap(t_terrain, pos.x, pos.y, pos.z);

  Vec4 offsetCorrection = Vec4(0, 0, 0);
  const float offsetH = BLOCK_SIZE * 0.70F;
  const float offsetV = BLOCK_SIZE * 0.30F;

  t_block->model.identity();

  if (BlockOrientation::Top != orientation) {
    const float _20DEGINRAD = 20 * Tyra::Math::ANG2RAD;
    t_block->model.rotateZ(_20DEGINRAD);

    switch (orientation) {
      case BlockOrientation::North:
        t_block->model.rotateY(_90DEGINRAD);
        offsetCorrection.set(0, offsetV, -offsetH);
        break;
      case BlockOrientation::South:
        t_block->model.rotateY(_270DEGINRAD);
        offsetCorrection.set(0, offsetV, offsetH);
        break;
      case BlockOrientation::West:
        t_block->model.rotateY(_180DEGINRAD);
        offsetCorrection.set(-offsetH, offsetV, 0);
        break;
      case BlockOrientation::East:
        offsetCorrection.set(offsetH, offsetV, 0);
        break;
      default:
        break;
    }
  }

  t_block->model.scale(BLOCK_SIZE);
  t_block->model.translate(t_block->position + offsetCorrection);
}
