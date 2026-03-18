#include "managers/world_generation/feature.hpp"

Feature::Feature(bool t_doUpdate) {
    this->doUpdate = t_doUpdate;
}

void Feature::placeBlock(Level* level, int x, int y, int z, uint8_t blockType) {
    if (level->BoundCheckMap(x, y, z)) {
        level->SetBlockInMap(x, y, z, blockType);
        // Note: doUpdate might be used in the future if TyraCraft supports a "no update" setter
        // or light propagation triggers on set block. Currently SetBlockInMap exists natively.
    }
}
