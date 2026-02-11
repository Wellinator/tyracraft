#pragma once

#include "post_fx_helper.hpp"
#include <stdint.h>

/**
 * postFxFog — Fog post-processing effect (based on SCEE POSTFX_FOG_INSTRUCTIONS)
 *
 * Three-pass approach:
 *
 * Pass 1a — PSMCT16 strip copy: Z Green channel → FB Alpha channel
 *   Reinterprets Z-buffer and framebuffer as PSMCT16. Copies 8-pixel-wide
 *   strips from even columns (lower 16 bits) to odd columns (upper 16 bits),
 *   moving Z[15:8] (Green byte) into FB[31:24] (Alpha byte).
 *   FBMSK=0x00FF masks the lower byte of each 16-bit pixel.
 *
 * Pass 1b — PSMT8H + CLUT: scale alpha from 0-255 to 0-127
 *   Reads FB alpha as PSMT8H CLUT index. CLUT maps alpha_in → alpha_in/2,
 *   scaling to the GS alpha range (0-127 = 0.0-1.0).
 *
 * Pass 2 — Draw fog colour using destination alpha
 *   Non-textured alpha-blended 32-pixel-wide sprites; blend formula
 *   (Cd − Cs) × Ad + Cs makes distant pixels (low alpha) fog-colored.
 *
 * Uses Context 2 registers via PostFxHelper. CLUT upload uses
 * draw_texture_transfer (IMAGE mode).
 */

/**
 * Initialise the fog subsystem: upload linear CLUT to VRAM.
 * Call once (e.g. in PostFxManager constructor).
 *
 * @param clutVramAddr_words  CLUT destination in VRAM (word units)
 */
void postFxFogInit(uint32_t clutVramAddr_words);

/**
 * Execute the fog post-processing passes.
 *
 * @param hlp             PostFxHelper bound to the current framebuffer
 * @param fbAddr_words    Framebuffer address (word units)
 * @param zbufAddr_words  Z-buffer address (word units)
 * @param screenW         Screen width in pixels (e.g. 512)
 * @param screenH         Screen height in pixels (e.g. 448)
 * @param fogR/G/B        Fog colour (0-255 each)
 */
void postFxFog(PostFxHelper& hlp, uint32_t fbAddr_words,
               uint32_t zbufAddr_words, uint32_t tempAddr_words, int screenW,
               int screenH, int fogR, int fogG, int fogB);

/** Free any heap memory allocated by the fog subsystem. */
void postFxFogCleanup();
