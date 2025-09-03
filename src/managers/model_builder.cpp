#include "managers/model_builder.hpp"

M4x4 ModelBuilder_BuildModel(Vec4* offset) {
  Level* pLevel = Level::getInstance();
  const Blocks block_type = static_cast<Blocks>(
      pLevel->GetBlockFromMap(offset->x, offset->y, offset->z));

  switch (block_type) {
    case Blocks::TORCH:
      return ModelBuilder_TorchModel(offset);
      break;

    case Blocks::WATER_BLOCK:
    case Blocks::LAVA_BLOCK:
      return ModelBuilder_NoRotationModel(offset);
      break;

    // TODO: add model for slabs

    default:
      return ModelBuilder_DefaultModel(offset);
      break;
  }
}

M4x4 ModelBuilder_DefaultModel(Vec4* offset) {
  Level* pLevel = Level::getInstance();
  Vec4 position = pLevel->offsetToWorldPos(offset);

  M4x4 model;
  model.identity();

  const auto orientation =
      pLevel->GetBlockOrientationDataFromMap(offset->x, offset->y, offset->z);

  switch (orientation) {
    case BlockOrientation::North:
      model.rotateY(_90DEGINRAD);
      break;
    case BlockOrientation::South:
      model.rotateY(_270DEGINRAD);
      break;
    case BlockOrientation::West:
      model.rotateY(_180DEGINRAD);
      break;
    case BlockOrientation::East:
    case BlockOrientation::Top:
    default:
      break;
  }

  model.scale(BLOCK_SIZE);
  model.translate(position);
  return model;
}

M4x4 ModelBuilder_NoRotationModel(Vec4* offset) {
  Vec4 position = Level::getInstance()->offsetToWorldPos(offset);

  M4x4 model;
  model.identity();

  model.scale(BLOCK_SIZE);
  model.translate(position);
  return model;
}

M4x4 ModelBuilder_TorchModel(Vec4* offset) {
  Level* pLevel = Level::getInstance();
  Vec4 position = pLevel->offsetToWorldPos(offset);

  Vec4 offsetCorrection = Vec4(0, 0, 0);
  const float offsetH = BLOCK_SIZE * 0.70F;
  const float offsetV = BLOCK_SIZE * 0.30F;

  M4x4 model;
  model.identity();

  const auto orientation =
      pLevel->GetTorchOrientationDataFromMap(offset->x, offset->y, offset->z);
  if (BlockOrientation::Top != orientation) {
    const float _20DEGINRAD = 20 * Tyra::Math::ANG2RAD;
    model.rotateZ(_20DEGINRAD);

    switch (orientation) {
      case BlockOrientation::North:
        model.rotateY(_90DEGINRAD);
        offsetCorrection.set(0, offsetV, -offsetH);
        break;
      case BlockOrientation::South:
        model.rotateY(_270DEGINRAD);
        offsetCorrection.set(0, offsetV, offsetH);
        break;
      case BlockOrientation::West:
        model.rotateY(_180DEGINRAD);
        offsetCorrection.set(-offsetH, offsetV, 0);
        break;
      case BlockOrientation::East:
        offsetCorrection.set(offsetH, offsetV, 0);
        break;
      default:
        break;
    }
  }

  model.scale(BLOCK_SIZE);
  model.translate(position + offsetCorrection);
  return model;
}
