#pragma once

#include "entities/level.hpp"
#include "3libs/mazegen/mazegen.hpp"
#include "iomanip"

/**
 * @brief Generates the world
 * @TODO Offer a callback for world percentage
 */
void Mazecraft_GenerateMap(unsigned int seed, const u8 width, const u8 height,
                           mazegen::Config cfg);