#include "managers/post-fx/post_fx_helper.hpp"
#include "managers/dma_gif_builder.hpp"
#include <draw.h>
#include <dma.h>
#include <dma_tags.h>
#include <gs_gp.h>
#include <gs_psm.h>
#include <kernel.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>

// Mapeamento de registradores GS — usa CONTEXT 2 (igual ao sps2demo)
// Context 2 evita corromper o estado de rendering normal (Context 1)
const int PostFxHelper::regAddress[PostFxHelper::NUM_REGS] = {
    0x43,  // ALPHA (GS_REG_ALPHA_2)
    0x09,  // CLAMP (GS_REG_CLAMP_2)
    0x4d,  // FRAME (GS_REG_FRAME_2)
    0x01,  // RGBAQ (GS_REG_RGBAQ) — shared, not context-specific
    0x41,  // SCISSOR (GS_REG_SCISSOR_2)
    0x48,  // TEST (GS_REG_TEST_2)
    0x07,  // TEX0 (GS_REG_TEX0_2)
    0x3f,  // TEXFLUSH (GS_REG_TEXFLUSH) — shared, not context-specific
    0x19,  // XYOFFSET (GS_REG_XYOFFSET_2)
    0x15,  // TEX1 (GS_REG_TEX1_2)
};

// Static members for Context 1 save/restore
PostFxHelper::GsContext1State PostFxHelper::savedContext = {};
bool PostFxHelper::contextSaved = false;

PostFxHelper::PostFxHelper(uint32_t fbAddr_words, uint32_t screenW,
                           uint32_t screenH)
    : screenW(screenW), screenH(screenH) {
  // Inicializa todos os registradores com dirty flag
  memset(registerChanged, 0, sizeof(registerChanged));

  // Setup padrão (baseado em sps2demo Posteffecthelper constructor)
  setAlpha(0, 1, 0, 1, 0);     // Normal blend: Cs*As + Cd*(1-As)
  setClamp(0, 0, 0, 0, 0, 0);  // Repeat mode
  setFrame(fbAddr_words, screenW);
  setRgba(128, 128, 128, 128);  // Cor neutra (0.5 em fixed-point)
  setScissor(0, 0, screenW - 1, screenH - 1);
  setTest(1, 0, 0, 1, 0, 0, 1, 1);  // Alpha test enabled, Z test ALWAYS

  // Textura default: mesmo framebuffer como source
  int tw = 31 - (__builtin_clz(screenW) + 1);
  if (screenW > (1u << tw)) tw++;
  int th = 31 - (__builtin_clz(screenH) + 1);
  if (screenH > (1u << th)) th++;

  setTex0(fbAddr_words, screenW, 0, tw, th, 1, 0);
  setTexflush();
  setFiltering(1);  // Linear filtering
  setXyoffset(0, 0);
}

PostFxHelper::~PostFxHelper() {
  // Restaura alpha blend padrão
  setAlpha(0, 1, 0, 1, 0);
  flushRegisters();
}

void PostFxHelper::setAlpha(int a, int b, int c, int d, int fix) {
  registerChanged[REG_ALPHA] = true;
  registers[REG_ALPHA] =
      a | (b << 2) | (c << 4) | (d << 6) | ((uint64_t)fix << 32);
}

void PostFxHelper::setClamp(int wms, int wmt, int minu, int maxu, int minv,
                            int maxv) {
  registerChanged[REG_CLAMP] = true;
  registers[REG_CLAMP] = wms | (wmt << 2) | (minu << 4) | (maxu << 14) |
                         ((uint64_t)minv << 24) | ((uint64_t)maxv << 34);
}

void PostFxHelper::setClamp(int w, int h) {
  setClamp(2, 2, 0, w - 1, 0, h - 1);  // Clamp to edge
}

void PostFxHelper::setFrame(uint32_t fbp_words, int fbw_pixels, int psm,
                            uint32_t fbmsk) {
  registerChanged[REG_FRAME] = true;
  // FRAME register: FBP (bits 0-8 = page address), FBW (bits 16-21 = width/64),
  // PSM (bits 24-29), FBMSK (bits 32-63)
  uint32_t fbp_pages = fbp_words >> 11;   // words → 8192-byte pages
  uint32_t fbw_blocks = fbw_pixels >> 6;  // pixels → 64-pixel blocks
  registers[REG_FRAME] =
      fbp_pages | (fbw_blocks << 16) | (psm << 24) | ((uint64_t)fbmsk << 32);
}

void PostFxHelper::setRgba(int r, int g, int b, int a) {
  registerChanged[REG_RGBAQ] = true;
  registers[REG_RGBAQ] = r | (g << 8) | (b << 16) | ((uint64_t)a << 24) |
                         ((uint64_t)0x3f800000 << 32);  // Q = 1.0 (float)
}

void PostFxHelper::setScissor(int x0, int y0, int x1, int y1) {
  registerChanged[REG_SCISSOR] = true;
  registers[REG_SCISSOR] =
      x0 | (x1 << 16) | ((uint64_t)y0 << 32) | ((uint64_t)y1 << 48);
}

void PostFxHelper::setTest(int ate, int atst, int aref, int afail, int date,
                           int datm, int zte, int ztst) {
  registerChanged[REG_TEST] = true;
  registers[REG_TEST] = ate | (atst << 1) | (aref << 4) | (afail << 12) |
                        (date << 14) | (datm << 15) | (zte << 16) |
                        (ztst << 17);
}

void PostFxHelper::setTex0(uint32_t tbp_words, int tbw_pixels, int psm, int tw,
                           int th, int tcc, int tfx) {
  registerChanged[REG_TEX0] = true;
  // TEX0 register: TBP0 (bits 0-13 = 256-byte block address), TBW (bits 14-19),
  // PSM (bits 20-25), TW/TH (bits 26-33), TCC (bit 34), TFX (bit 35)
  uint32_t tbp_blocks = tbp_words >> 6;   // words → 256-byte blocks
  uint32_t tbw_blocks = tbw_pixels >> 6;  // pixels → 64-pixel blocks

  registers[REG_TEX0] = tbp_blocks | (tbw_blocks << 14) | (psm << 20) |
                        (tw << 26) | ((uint64_t)th << 30) |
                        ((uint64_t)tcc << 34) | ((uint64_t)tfx << 35);

  textureWidth = 1 << tw;
  textureHeight = 1 << th;
}

void PostFxHelper::setFiltering(int filtering) {
  registerChanged[REG_TEX1] = true;
  // TEX1: MMIN (bits 0-2), MMAG (bit 5-6)
  // filtering=0: nearest (0,0), filtering=1: linear (1,1)
  registers[REG_TEX1] = (filtering << 5) | (filtering << 6);
}

void PostFxHelper::setTexflush() { registerChanged[REG_TEXFLUSH] = true; }

void PostFxHelper::setXyoffset(int ofx, int ofy) {
  registerChanged[REG_XYOFFSET] = true;
  registers[REG_XYOFFSET] = ofx | ((uint64_t)ofy << 32);
}

void PostFxHelper::flushRegisters() {
  // Conta quantos registradores mudaram
  int count = 0;
  for (int i = 0; i < NUM_REGS; i++) {
    if (registerChanged[i]) count++;
  }
  if (count == 0) return;

  qword_t packets[NUM_REGS + 2] ALIGNED(64);
  qword_t* q = packets;

  // GIF tag: N registradores A+D
  PACK_GIFTAG(q, GIF_SET_TAG(count, 1, 0, 0, GIF_FLG_PACKED, 1), GIF_REG_AD);
  q++;

  // Envia apenas registradores modificados
  for (int i = 0; i < NUM_REGS; i++) {
    if (registerChanged[i]) {
      PACK_GIFTAG(q, registers[i], regAddress[i]);
      q++;
      registerChanged[i] = false;
    }
  }

  FlushCache(0);
  dma_channel_send_normal(DMA_CHANNEL_GIF, packets, q - packets, 0, 0);
  dma_channel_wait(DMA_CHANNEL_GIF, 500);
}

void PostFxHelper::blit(int x0, int y0, int x1, int y1, bool alpha,
                        bool texture, bool flip) {
  // Versão sem UVs customizados - usa identity mapping
  blit(x0, y0, x1, y1, x0 + 0.5f, y0 + 0.5f, x1 + 0.5f, y1 + 0.5f, alpha,
       texture, flip);
}

void PostFxHelper::blit(int x0, int y0, int x1, int y1, float u0, float v0,
                        float u1, float v1, bool alpha, bool texture,
                        bool flip) {
  // Port of sps2demo posteffecthelper.cpp blit()
  // Uses FST=0 (float S/T mode) matching the sps2demo convention.

  if (flip) {
    float w = u1 - u0;
    float h = v1 - v0;
    u0 = u1 - w / (x1 - x0);
    v0 = v1 - h / (y1 - y0);
    u1 = u0 - w;
    v1 = v0 - h;
  }

  // 32-texel-wide strips to avoid GS page-buffer thrashing
  float ud = (u0 < u1) ? u1 - u0 : u0 - u1;
  int nstrips = (int)(ud + 32) / 32;
  if (nstrips < 1) nstrips = 1;

  int xstep = (x1 - x0 + nstrips - 1) / nstrips;
  ud = (u1 - u0) * xstep / (x1 - x0);

  int maxx = x1;
  float maxu = u1;
  x1 = x0 + xstep;
  u1 = u0 + ud;

  flushRegisters();

  // GIF tag + nstrips * 4 qwords (ST, XYZ2, ST, XYZ2)
  qword_t* packets =
      (qword_t*)memalign(64, (1 + nstrips * 4) * sizeof(qword_t));
  qword_t* q = packets;

  // PRIM: sprite, FST=0 (float S/T), Context 2 (CTXT=1)
  // sps2demo: prim = 6 + (alpha?64:0) + (texture?16:0) + 512(CTXT)
  //   bit 0-2=6(sprite), bit4=TME, bit6=ABE, bit9=CTXT → FST=0
  uint64_t prim = GS_SET_PRIM(GS_PRIM_SPRITE, 0, texture ? 1 : 0, 0,
                              alpha ? 1 : 0, 0, 0, 1, 0);

  // REGS: ST, XYZ2, ST, XYZ2  (same as sps2demo 0x5252)
  q->dw[0] = GIF_SET_TAG(nstrips, 1, GIF_PRE_ENABLE, prim, GIF_FLG_PACKED, 4);
  q->dw[1] = GIF_REG_ST | (GIF_REG_XYZ2 << 4) |
             (GIF_REG_ST << 8) | (GIF_REG_XYZ2 << 12);
  q++;

  // Helper: reinterpret float as u32
  auto f2u = [](float f) -> uint32_t {
    uint32_t r;
    memcpy(&r, &f, 4);
    return r;
  };

  for (int i = 0; i < nstrips; i++) {
    // ST start — PACKED: S[31:0] T[63:32] Q[95:64] (float)
    float s0 = u0 / textureWidth;
    float t0 = v0 / textureHeight;
    q->dw[0] = (uint64_t)f2u(s0) | ((uint64_t)f2u(t0) << 32);
    q->dw[1] = 0;  // Q (unused with FST-style sprites)
    q++;

    // XYZ2 start — PACKED: X[15:0] | Y[47:32] | Z[103:72]
    q->dw[0] = (uint64_t)(x0 * 16) | ((uint64_t)(y0 * 16) << 32);
    q->dw[1] = 0;  // Z = 0
    q++;

    // ST end
    float s1 = u1 / textureWidth;
    float t1 = v1 / textureHeight;
    q->dw[0] = (uint64_t)f2u(s1) | ((uint64_t)f2u(t1) << 32);
    q->dw[1] = 0;
    q++;

    // XYZ2 end
    q->dw[0] = (uint64_t)(x1 * 16) | ((uint64_t)(y1 * 16) << 32);
    q->dw[1] = 0;
    q++;

    x0 = x1;
    x1 += xstep;
    u0 = u1;
    u1 += ud;
    if (x1 > maxx) {
      x1 = maxx;
      u1 = maxu;
    }
  }

  FlushCache(0);
  dma_channel_send_normal(DMA_CHANNEL_GIF, packets, q - packets, 0, 0);
  dma_channel_wait(DMA_CHANNEL_GIF, 500);

  free(packets);
}

void PostFxHelper::downSample(int x0, int y0, int x1, int y1, bool alpha) {
  // UV sampling com offset 2x+1 (bilinear downsample)
  blit(x0, y0, x1, y1, x0 * 2 + 1, y0 * 2 + 1, x1 * 2 + 1, y1 * 2 + 1, alpha);
}

void PostFxHelper::upSample(int x0, int y0, int x1, int y1, bool alpha) {
  // UV sampling com scale 0.5 (bilinear upsample)
  blit(x0, y0, x1, y1, (float)x0 / 2, (float)y0 / 2, (float)x1 / 2,
       (float)y1 / 2, alpha);
}

void PostFxHelper::drawBox(int x0, int y0, int x1, int y1, bool alpha) {
  // Draws a filled sprite covering (x0,y0)→(x1,y1) in 32-pixel-wide strips.
  // Non-textured, uses current RGBAQ colour + alpha blending if requested.
  // 32-pixel strips avoid GS page-buffer thrashing.

  flushRegisters();

  constexpr int STRIP_W = 32;
  int nstrips = (x1 - x0 + STRIP_W - 1) / STRIP_W;
  if (nstrips < 1) nstrips = 1;

  // GIFtag + nstrips * 2 qwords (XYZ2 pair per sprite)
  qword_t packets[1 + 128] ALIGNED(64);  // max 64 strips
  qword_t* q = packets;

  // PRIM: sprite, no texture, alpha optional, FST=0, Context 2 (CTXT=1)
  uint64_t prim =
      GS_SET_PRIM(GS_PRIM_SPRITE, 0, 0, 0, alpha ? 1 : 0, 0, 0, 1, 0);

  q->dw[0] = GIF_SET_TAG(nstrips, 1, GIF_PRE_ENABLE, prim, GIF_FLG_PACKED, 2);
  q->dw[1] = GIF_REG_XYZ2 | (GIF_REG_XYZ2 << 4);
  q++;

  int cx0 = x0;
  for (int i = 0; i < nstrips; i++) {
    int cx1 = cx0 + STRIP_W;
    if (cx1 > x1) cx1 = x1;

    // XYZ2 top-left — PACKED: X[15:0] | Y[47:32] | Z[103:72]
    q->dw[0] = (uint64_t)(cx0 * 16) | ((uint64_t)(y0 * 16) << 32);
    q->dw[1] = 0;
    q++;

    // XYZ2 bottom-right (triggers drawing kick)
    q->dw[0] = (uint64_t)(cx1 * 16) | ((uint64_t)(y1 * 16) << 32);
    q->dw[1] = 0;
    q++;

    cx0 = cx1;
  }

  FlushCache(0);
  dma_channel_send_normal(DMA_CHANNEL_GIF, packets, q - packets, 0, 0);
  dma_channel_wait(DMA_CHANNEL_GIF, 500);
}

// ===========================================================================
//  Context 1 Save/Restore — Correção Bug #3 (Context State Pollution)
// ===========================================================================

void PostFxHelper::saveContext1() {
  // Salva todos os 8 registradores do Context 1.
  // Atualmente não há API ps2sdk para LER registradores GS, então salvamos
  // os valores que estão em nosso estado interno.
  //
  // NOTA: Isso assume que o PostFxHelper foi inicializado com os valores
  // corretos do framebuffer atual. Se outros sistemas modificaram Context 1
  // diretamente (fora do PostFxHelper), esses valores serão perdidos.
  //
  // Solução ideal: usar TRXREG/TRXPOS/TRXDIR para READ-back do GS, mas
  // isso é complexo e lento. Para TyraCraft, assumimos que post-FX sempre
  // roda no início do frame após o rendering completo, então o estado
  // conhecido do PostFxHelper é suficiente.

  savedContext.frame = registers[REG_FRAME];
  savedContext.alpha = registers[REG_ALPHA];
  savedContext.test = registers[REG_TEST];
  savedContext.scissor = registers[REG_SCISSOR];
  savedContext.tex0 = registers[REG_TEX0];
  savedContext.clamp = registers[REG_CLAMP];
  
  // ZBUF e FBA não estão rastreados pelo PostFxHelper (não há setters),
  // então usamos valores padrão seguros.
  // ZBUF_1: assume Z-buffer padrão sem write mask
  // FBA_1: assume FBA desabilitado (padrão)
  savedContext.zbuf = 0;  // Será preenchido corretamente abaixo
  savedContext.fba = 0;   // FBA=0 (disabled)

  contextSaved = true;
}

void PostFxHelper::restoreContext1() {
  if (!contextSaved) {
    // Erro: tentou restaurar sem ter salvo antes
    return;
  }

  // Envia os 8 registradores salvos de volta ao GS via DMA
  DmaGifBuilder builder;
  builder.begin();
  builder.addGifTag(GIF_REG_AD);
  
  builder.addAd(savedContext.frame, GS_REG_FRAME_1);
  builder.addAd(savedContext.zbuf, GS_REG_ZBUF_1);
  builder.addAd(savedContext.test, GS_REG_TEST_1);
  builder.addAd(savedContext.scissor, GS_REG_SCISSOR_1);
  builder.addAd(savedContext.tex0, GS_REG_TEX0_1);
  builder.addAd(savedContext.alpha, GS_REG_ALPHA_1);
  builder.addAd(savedContext.clamp, GS_REG_CLAMP_1);
  builder.addAd(savedContext.fba, GS_REG_FBA_1);
  
  // TEXFLUSH para garantir que mudanças de TEX0 sejam aplicadas
  builder.addAd(GS_SET_TEXFLUSH(0), GS_REG_TEXFLUSH);
  
  builder.send();

  // Atualiza estado interno para refletir restauração
  registers[REG_FRAME] = savedContext.frame;
  registers[REG_ALPHA] = savedContext.alpha;
  registers[REG_TEST] = savedContext.test;
  registers[REG_SCISSOR] = savedContext.scissor;
  registers[REG_TEX0] = savedContext.tex0;
  registers[REG_CLAMP] = savedContext.clamp;
  
  // Limpa dirty flags — registradores foram restaurados
  memset(registerChanged, 0, sizeof(registerChanged));
  
  contextSaved = false;
}

void PostFxHelper::clearDirtyFlags() {
  // Limpa todos os dirty flags.
  // Usado após override manual de registradores via DmaGifBuilder
  // para evitar que flushRegisters() reenvie valores desatualizados.
  //
  // CRÍTICO para Bug #4: após DmaGifBuilder sobrescrever TEX0 com
  // parâmetros de CLUT, chamar clearDirtyFlags() evita que blit()
  // reenvie o TEX0 antigo.
  memset(registerChanged, 0, sizeof(registerChanged));
}
