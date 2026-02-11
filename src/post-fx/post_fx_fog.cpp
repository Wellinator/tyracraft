#include "managers/post-fx/post_fx_fog.hpp"

#include <gs_gp.h>
#include <gs_psm.h>
#include <dma.h>
#include <dma_tags.h>
#include <draw.h>
#include <kernel.h>
#include <malloc.h>
#include <string.h>

#include "managers/dma_gif_builder.hpp"

// ---------------------------------------------------------------------------
//  File-scope storage
// ---------------------------------------------------------------------------
namespace {

/** CLUT VRAM address (word units) — set once by postFxFogInit(). */
uint32_t clutVramAddr = 0;

/** CLUT VRAM address in 256-byte blocks (clutVramAddr >> 6). */
uint32_t clutBlock = 0;

/** Reinterpret float as uint32_t for GIF ST register packing. */
inline uint32_t f2u(float f) {
  uint32_t r;
  memcpy(&r, &f, 4);
  return r;
}

}  // namespace

// ---------------------------------------------------------------------------
//  Stage 1 — Build CSM1-swizzled CLUT and upload to VRAM (IMAGE mode)
// ---------------------------------------------------------------------------
static void buildAndUploadCLUT(uint32_t clutAddr_words) {
  // Linear fog palette: pal[i] = ((i/2) << 24) | 0x808080
  //   RGB = 0x80 (neutral gray — MODULATE preserves it)
  //   A   = i/2  (maps 0-255 → 0-127, i.e. 0.0–~1.0 in GS alpha)
  uint32_t pal[256];
  for (int i = 0; i < 256; i++) {
    pal[i] = (((uint32_t)(i / 2)) << 24) | 0x00808080;
  }

  // Apply CSM1 swizzle (GS CLUT storage order)
  uint32_t swizzled[256];
  for (int p = 0; p < 256; p++) {
    swizzled[(p & 231) + ((p & 8) << 1) + ((p & 16) >> 1)] = pal[p];
  }

  // Upload via draw_texture_transfer (chain-mode DMA, IMAGE GIFtag)
  qword_t packets[30] ALIGNED(64);
  qword_t* q = packets;

  q = draw_texture_transfer(q, swizzled, 16, 16, GS_PSM_32, clutAddr_words,
                            16);
  q = draw_texture_flush(q);

  FlushCache(0);
  dma_channel_send_chain(DMA_CHANNEL_GIF, packets, q - packets, 0, 0);
  dma_channel_wait(DMA_CHANNEL_GIF, 500);
}

// ===========================================================================
//  Public API
// ===========================================================================

void postFxFogInit(uint32_t clutVramAddr_words) {
  clutVramAddr = clutVramAddr_words;
  clutBlock = clutVramAddr >> 6;  // words → 256-byte blocks

  buildAndUploadCLUT(clutVramAddr);
}

// ---------------------------------------------------------------------------
//  postFxFog — main entry point, called every frame
//
//  Three-pass approach based on SCEE POSTFX_FOG_INSTRUCTIONS document:
//
//  Pass 1a — PSMCT16 strip copy: Z Green channel → FB Alpha channel
//    Reinterpret both Z-buffer and framebuffer as PSMCT16. Copy 8-pixel-
//    wide strips from even columns (lower 16 bits = R+G) to odd columns
//    (upper 16 bits = B+A). FBMSK=0x00FF masks lower byte of each 16-bit
//    pixel, so only upper byte (bits 8-15) is written. This moves Z[15:8]
//    (Green byte) into FB[31:24] (Alpha byte).
//
//  Pass 1b — PSMT8H + CLUT: scale alpha from 0-255 to 0-127
//    Read FB alpha (byte 3 of 32-bit pixel) as PSMT8H CLUT index.
//    CLUT maps alpha_out = alpha_in / 2 (GS alpha range 0-127 = 0.0-1.0).
//    Write only alpha back (FBMSK=0x00FFFFFF).
//
//  Pass 2 — Fog colour blend using destination alpha
//    Blend: (Cd - Cs) × Ad + Cs. Near (high Ad) keeps original, far
//    (low Ad) gets fog colour. Non-textured 32-pixel-wide strips.
// ---------------------------------------------------------------------------
void postFxFog(PostFxHelper& hlp, uint32_t fbAddr_words,
               uint32_t zbufAddr_words, uint32_t tempAddr_words, int screenW,
               int screenH, int fogR, int fogG, int fogB) {
  // =====================================================================
  //  Pre-pass — Re-swizzle Z-buffer from PSMZ24 to PSMCT32 page layout
  //
  //  PSMZ24 and PSMCT32 have different block orderings within each 8KB
  //  page. The PSMCT16 strip-copy trick (Pass 1a) requires data in the
  //  PSMCT32 layout family. A GS local-to-local VRAM transfer reads
  //  using PSMZ24 addressing and writes using PSMCT32 addressing,
  //  preserving the raw 32-bit Z values while re-ordering the blocks.
  //
  //  The destination (tempAddr) must be a separate VRAM region because
  //  in-place conversion corrupts data (the different block orderings
  //  cause write-before-read hazards within each page).
  // =====================================================================
  {
    uint32_t zbufBlock = zbufAddr_words >> 6;  // words → 256-byte blocks
    uint32_t tempBlock = tempAddr_words >> 6;
    uint32_t bufW = screenW >> 6;  // pixels → 64-pixel units

    DmaGifBuilder builder;
    builder.begin();
    builder.addGifTag(GIF_REG_AD);

    builder.addAd(
        GS_SET_BITBLTBUF(zbufBlock, bufW, GS_PSMZ_24, tempBlock, bufW,
                          GS_PSM_32),
        GS_REG_BITBLTBUF);
    builder.addAd(GS_SET_TRXPOS(0, 0, 0, 0, 0), GS_REG_TRXPOS);
    builder.addAd(GS_SET_TRXREG(screenW, screenH), GS_REG_TRXREG);
    builder.addAd(GS_SET_TRXDIR(2), GS_REG_TRXDIR);  // 2 = local-to-local
    builder.send();
  }

  // Sync: the next DMA send serializes after the local-to-local completes
  {
    DmaGifBuilder builder;
    builder.begin();
    builder.addGifTag(GIF_REG_AD);
    builder.addAd(0, GS_REG_TEXFLUSH);
    builder.send();
  }

  // =====================================================================
  //  Pass 1a — PSMCT16 strip copy: Z Green → FB Alpha
  //
  //  Now reads from tempAddr (PSMCT32 layout) instead of zbufAddr.
  //  Treat 32-bit buffers as 16-bit. Within each page column (8 pixels
  //  wide), alternating 8-pixel groups in the PSMCT16 view contain the
  //  lower/upper 16 bits of each 32-bit pixel.
  //  Copying from even columns (source) to odd columns (dest+8 offset)
  //  moves the Green byte (Z bits 15-8) to the Alpha byte (FB bits 31-24).
  //
  //  Scissor height is doubled because PSMCT16 pages are 64 rows tall
  //  vs PSMCT32's 32 rows, so the same VRAM maps to 2x the height.
  // =====================================================================

  int copyH = screenH * 2;

  // Frame buffer as PSMCT16, FBMSK=0x00FF masks lower byte of 16-bit pixel
  // Only upper byte (bits 8-15) is written → maps to Alpha byte in FB
  hlp.setFrame(fbAddr_words, screenW, GS_PSM_16, 0x00FF);

  // Z writes OFF, depth test always pass
  hlp.setTest(0, 0, 0, 0, 0, 0, 1, 1);

  // Scissor: height doubled for PSMCT16 reinterpretation
  hlp.setScissor(0, 0, screenW - 1, copyH - 1);

  // No offset (screen-space coordinates)
  hlp.setXyoffset(0, 0);

  // Clamp to edge (prevent wrap-around at texture boundaries)
  hlp.setClamp(screenW, copyH);

  // Nearest filtering (exact texel copy, no interpolation)
  hlp.setFiltering(0);

  // Invalidate texture cache
  hlp.setTexflush();

  // Alpha blend not needed for raw copy (ABE=0 in PRIM)
  hlp.setAlpha(0, 1, 0, 1, 0);

  // Neutral colour
  hlp.setRgba(0x80, 0x80, 0x80, 0x80);

  // TEX0: Re-swizzled Z data (now in PSMCT32 layout) as PSMCT16 texture
  // tw=9 (512), th=10 (1024) covers doubled height
  // TCC=1, TFX=DECAL (direct copy of texture to output)
  hlp.setTex0(tempAddr_words, screenW, GS_PSM_16, 9, 10, 1, 1);

  hlp.flushRegisters();

  // ZBUF_2: Z writes OFF (sent separately via DmaGifBuilder)
  {
    DmaGifBuilder builder;
    builder.begin();
    builder.addGifTag(GIF_REG_AD);
    uint32_t zbufPage = zbufAddr_words >> 11;
    builder.addAd(GS_SET_ZBUF(zbufPage, GS_PSMZ_24, 1), GS_REG_ZBUF_2);
    builder.send();
  }

  hlp.clearDirtyFlags();

  // --- Draw 8-pixel-wide strip sprites ---
  // Each strip copies from even column (lower 16 bits of Z) to odd column
  // (upper 16 bits of FB), moving Z Green byte → FB Alpha byte.
  //
  // For 512-pixel screen: 32 strips of 8 pixels, spaced every 16 pixels.
  //   Strip i: src UV (i*16, 0)→(i*16+8, copyH)
  //            dst XY (i*16+8, 0)→(i*16+16, copyH)
  {
    int numStrips = screenW / 16;
    float texW = (float)(1 << 9);   // 512.0
    float texH = (float)(1 << 10);  // 1024.0

    // PRIM: sprite, textured, NO alpha blend, FST=0 (float S/T), Context 2
    uint64_t prim =
        GS_SET_PRIM(GS_PRIM_SPRITE, 0, 1, 0, 0, 0, 0, 1, 0);

    qword_t* packets =
        (qword_t*)memalign(64, (1 + numStrips * 4) * sizeof(qword_t));
    qword_t* q = packets;

    // GIF tag: numStrips loops × 4 regs (ST, XYZ2, ST, XYZ2)
    q->dw[0] = GIF_SET_TAG(numStrips, 1, GIF_PRE_ENABLE, prim,
                            GIF_FLG_PACKED, 4);
    q->dw[1] = GIF_REG_ST | (GIF_REG_XYZ2 << 4) | (GIF_REG_ST << 8) |
               (GIF_REG_XYZ2 << 12);
    q++;

    for (int i = 0; i < numStrips; i++) {
      int srcX = i * 16;       // Source: even column (lower 16 bits of Z)
      int dstX = i * 16 + 8;   // Dest: odd column (upper 16 bits of FB)

      // ST start (top-left)
      float s0 = (srcX + 0.5f) / texW;
      float t0 = 0.5f / texH;
      q->dw[0] = (uint64_t)f2u(s0) | ((uint64_t)f2u(t0) << 32);
      q->dw[1] = 0;
      q++;

      // XYZ2 start
      q->dw[0] = (uint64_t)(dstX * 16) | ((uint64_t)0 << 32);
      q->dw[1] = 0;
      q++;

      // ST end (bottom-right)
      float s1 = (srcX + 8 + 0.5f) / texW;
      float t1 = (copyH + 0.5f) / texH;
      q->dw[0] = (uint64_t)f2u(s1) | ((uint64_t)f2u(t1) << 32);
      q->dw[1] = 0;
      q++;

      // XYZ2 end
      q->dw[0] =
          (uint64_t)((dstX + 8) * 16) |
          ((uint64_t)((uint64_t)copyH * 16) << 32);
      q->dw[1] = 0;
      q++;
    }

    FlushCache(0);
    dma_channel_send_normal(DMA_CHANNEL_GIF, packets, q - packets, 0, 0);
    dma_channel_wait(DMA_CHANNEL_GIF, 500);
    free(packets);
  }

  // =====================================================================
  //  Pass 1b — PSMT8H + CLUT: Scale alpha 0-255 → 0-127
  //
  //  PSMT8H (0x1B) reads byte 3 (bits 24-31) of each 32-bit pixel as a
  //  CLUT index. It shares the PSMCT32 page swizzle, so reading the FB
  //  works correctly. The CLUT maps index i → alpha = i/2, scaling the
  //  raw Green byte range (0-255) to GS alpha range (0-127 = 0.0-1.0).
  //
  //  Self-read: FB is both source texture and destination. Using 32-pixel
  //  strips (via hlp.blit) ensures no page-cache coherency issues.
  // =====================================================================

  // Frame buffer as PSMCT32, write only alpha (mask RGB)
  hlp.setFrame(fbAddr_words, screenW, GS_PSM_32, 0x00FFFFFF);

  // Normal scissor
  hlp.setScissor(0, 0, screenW - 1, screenH - 1);

  // Z test always pass
  hlp.setTest(0, 0, 0, 0, 0, 0, 1, 1);

  // Clamp to edge
  hlp.setClamp(screenW, screenH);

  // Nearest filtering
  hlp.setFiltering(0);

  // Invalidate texture cache (critical: FB was just written in Pass 1a)
  hlp.setTexflush();

  // No alpha blending needed
  hlp.setAlpha(0, 1, 0, 1, 0);

  // Neutral colour: MODULATE with 0x80 preserves CLUT values
  hlp.setRgba(0x80, 0x80, 0x80, 0x80);

  // Set TEX0 so PostFxHelper tracks textureWidth/Height for blit() UVs
  // PSM=0x1B (PSMT8H), tw=9 (512), th=9 (512), TCC=1, TFX=MODULATE
  hlp.setTex0(fbAddr_words, screenW, 0x1B, 9, 9, 1, 0);

  hlp.flushRegisters();

  // Override TEX0 with full CLUT parameters via DmaGifBuilder
  {
    uint32_t texBlock = fbAddr_words >> 6;
    uint32_t texBW = screenW >> 6;
    int tw = 9;
    int th = 9;

    // Use GS_SET_TEX0 macro to avoid bit-position errors
    // Parameters: TBA, TBW, PSM, TW, TH, TCC, TFNCT, CBA, CPSM, CSM, CSA, CLD
    uint64_t tex0 = GS_SET_TEX0(texBlock, texBW, 0x1B, tw, th, 1, 0,
                                 clutBlock, 0, 0, 0, 1);

    DmaGifBuilder builder;
    builder.begin();
    builder.addGifTag(GIF_REG_AD);
    builder.addAd(tex0, GS_REG_TEX0_2);
    builder.addAd(GS_SET_TEXFLUSH(0), GS_REG_TEXFLUSH);

    uint32_t zbufPage = zbufAddr_words >> 11;
    builder.addAd(GS_SET_ZBUF(zbufPage, GS_PSMZ_24, 1), GS_REG_ZBUF_2);
    builder.send();
  }

  hlp.clearDirtyFlags();

  // Blit full screen: PSMT8H reads FB alpha, CLUT scales it, writes back
  hlp.blit(0, 0, screenW, screenH, false, true, false);

  // =====================================================================
  //  Pass 2 — Draw fog colour over the framebuffer using destination alpha
  //
  //  After passes 1a+1b, FB alpha contains CLUT-scaled fog density:
  //    Near objects (reversed Z = high) → high Ad ≈ 1.0
  //    Far objects  (reversed Z = low)  → low Ad  ≈ 0.0
  //
  //  Blend formula: (Cd − Cs) × Ad + Cs  →  ALPHA(1, 0, 1, 0, 0)
  //    Near (Ad ≈ 1.0): (Cd - Fog)×1.0 + Fog = Cd    → keeps original
  //    Far  (Ad ≈ 0.0): (Cd - Fog)×0.0 + Fog = Fog   → full fog
  //
  //  Non-textured sprite, 32-pixel-wide strips.
  // =====================================================================

  // Restore framebuffer to 32-bit, no mask
  hlp.setFrame(fbAddr_words, screenW, GS_PSM_32, 0);

  // Restore scissor to normal
  hlp.setScissor(0, 0, screenW - 1, screenH - 1);

  // TEST: Z always pass
  hlp.setTest(0, 0, 0, 0, 0, 0, 1, 1);

  // ALPHA: (Cd - Cs) × Ad + Cs  →  ALPHA(1, 0, 1, 0, 0)
  hlp.setAlpha(1, 0, 1, 0, 0);

  // Fog colour
  hlp.setRgba(fogR, fogG, fogB, 0x80);

  // ZBUF: writes OFF
  {
    DmaGifBuilder builder;
    builder.begin();
    builder.addGifTag(GIF_REG_AD);
    uint32_t zbufPage = zbufAddr_words >> 11;
    builder.addAd(GS_SET_ZBUF(zbufPage, GS_PSMZ_24, 1), GS_REG_ZBUF_2);
    builder.send();
  }

  // Draw fog as non-textured alpha-blended 32-pixel strips
  hlp.drawBox(0, 0, screenW, screenH, true);
}

// ===========================================================================

void postFxFogCleanup() {
  // Nothing heap-allocated any more.
}
