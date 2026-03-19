#pragma once

#include "post_fx_helper.hpp"
#include <stdint.h>

/**
 * postFxDusk - Orange tint effect when looking at setting sun at dusk.
 * 
 * @param hlp PostFxHelper instance
 * @param screenW Screen width em pixels
 * @param screenH Screen height em pixels
 * @param intensity 0.0 to 1.0 intensity of the effect
 */
void postFxDusk(PostFxHelper& hlp, int screenW, int screenH, float intensity);
