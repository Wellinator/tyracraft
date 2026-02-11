#pragma once

#include "post_fx_helper.hpp"
#include <stdint.h>

/**
 * postFxGlare - Bloom/glare effect (port do sps2demo glare.cpp)
 *
 * Pipeline:
 * 1. Downsample framebuffer → halfResBuf (256x224)
 * 2. Threshold: remove pixels abaixo do cutoff
 * 3. Blur pyramid na half-resolution
 * 4. Composite de volta ao framebuffer com additive blend
 *
 * @param hlp PostFxHelper instance
 * @param fbAddr_words Framebuffer address (word units)
 * @param tempA_words Temp buffer A address (half-res, word units)
 * @param tempB_words Temp buffer B address (quarter-res, word units)
 * @param screenW Screen width em pixels
 * @param screenH Screen height em pixels
 * @param cutoff Brightness threshold (0.0-1.0, 0=no threshold)
 * @param depth Blur pyramid depth (1-4)
 * @param sourceScale Blend strength (positive=additive, negative=subtractive)
 * @param gain Brightness multiplier durante blur
 * @param clampBlack Darkening threshold durante blur (default 0.5)
 *
 * Baseado em sps2demo/posteffects/glare.cpp
 */
void postFxGlare(PostFxHelper& hlp, uint32_t fbAddr_words, uint32_t tempA_words,
                 uint32_t tempB_words, int screenW, int screenH, float cutoff,
                 int depth, float sourceScale, float gain,
                 float clampBlack = 0.5f);
