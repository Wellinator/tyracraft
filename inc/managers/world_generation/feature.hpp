#pragma once

#include "entities/level.hpp"

class Feature {
protected:
    bool doUpdate;

public:
    Feature(bool t_doUpdate = false);
    virtual ~Feature() = default;

    /**
     * @brief Places the feature starting at the specified coordinates.
     * @return true if the feature was successfully placed, false otherwise.
     */
    virtual bool place(Level* level, int x, int y, int z) = 0;

protected:
    virtual void placeBlock(Level* level, int x, int y, int z, uint8_t blockType);
};
