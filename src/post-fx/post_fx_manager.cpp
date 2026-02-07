#include "managers/post-fx/post_fx_manager.hpp"
#include "screen_settings.hpp"
#include "debug.hpp"
#include <gs_gp.h>
#include <gs_psm.h>
#include <dma_tags.h>
#include <screenshot.h>

#include <stdlib.h>
#include <malloc.h>
#include <math.h>
#include <kernel.h>
#include <draw.h>
#include <graph.h>
#include <gs_gp.h>
#include <gs_psm.h>
#include <dma.h>

using Tyra::Color;
using Tyra::RendererCoreTextureBuffers;
using Tyra::Texture;
using Tyra::TextureBuilderData;

PostFxManager::PostFxManager(Renderer* renderer)
    : Singleton<PostFxManager>(), settings(renderer->core.getSettings()) {
  pRenderer = renderer;

  // Upload fog CLUT (inversão + clipping + curva não-linear)
  uploadFogCLUT();
}

PostFxManager::~PostFxManager() {
  // CLUT é alocada em posição fixa da VRAM, não requer cleanup manual
  // (VRAM é gerenciada pelo GS e reinicializada a cada boot)
}

void PostFxManager::setTwTh(int w, int h, int* tw, int* th) {
  *tw = 31 - (lzw(w) + 1);
  if (w > (1 << *tw)) (*tw)++;

  *th = 31 - (lzw(h) + 1);
  if (h > (1 << *th)) (*th)++;
}

void PostFxManager::setFogNearPercent(float percent) {
  if (percent < 0.0f) percent = 0.0f;
  if (percent > 1.0f) percent = 1.0f;
  if (fogNearPercent != percent) {
    fogNearPercent = percent;
    clutDirty = true;
  }
}

void PostFxManager::setFogIntensity(float intensity) {
  if (intensity < 0.01f) intensity = 0.01f;
  if (fogIntensity != intensity) {
    fogIntensity = intensity;
    clutDirty = true;
  }
}

// ===================================================================
// Upload da Fog CLUT (STRATEGY B)
// ===================================================================
//
// A CLUT não-linear substitui os antigos passes 1 (invert Z) e 2 (clip Z).
// Cada entrada da paleta codifica três operações numa única lookup:
//
//   1. INVERSÃO: O Z-buffer do PS2 tem near=ALTO, far=BAIXO.
//      Para fog, queremos far=alpha alto, near=alpha baixo.
//      A CLUT inverte: CLUT[alto] = 0 (near, sem fog),
//                      CLUT[baixo] = 127 (far, fog máximo).
//
//   2. CLIPPING: Valores acima de um threshold (objetos próximos)
//      são forçados a alpha=0 (sem fog). Controlado por fogNearPercent.
//
//   3. CURVA NÃO-LINEAR: pow(t, fogIntensity) controla a distribuição
//      do fog. Valores maiores concentram o fog mais longe.
//
// CSM1 SWIZZLE: O PS2 GS armazena CLUT de 256 entradas com blocos de 8
// reordenados (dentro de cada grupo de 32, blocos 8-15 e 16-23 são trocados).
// Precisamos pre-swizzle os dados para que o hardware faça lookup correto.
//
// FORMATO: Cada pixel RGBA da paleta é (V, V, V, V) onde V é o fog alpha.
// O channel copy usa FRAME mask para escrever apenas no canal alpha do FB.
//
void PostFxManager::uploadFogCLUT() {
  constexpr int CLUT_WIDTH = 16;
  constexpr int CLUT_HEIGHT = 16;
  constexpr int CLUT_SIZE = CLUT_WIDTH * CLUT_HEIGHT;

  uint32_t* clutData =
      (uint32_t*)memalign(64, CLUT_SIZE * sizeof(uint32_t));
  if (!clutData) {
#ifdef DEBUG_MODE
    TYRA_LOG("[FOG] ERROR: Failed to allocate CLUT data buffer!");
#endif
    return;
  }

  // Calcular threshold de clipping em termos do green byte (0-255).
  // Green byte do Z: 255 = near plane, 0 = far plane.
  // fogNearPercent = 0.15 significa "15% mais perto não tem fog".
  // Threshold = 255 * (1.0 - fogNearPercent) = ~217 para 0.15
  // Valores de green byte ACIMA do threshold → alpha = 0 (sem fog)
  float clipThreshold = 255.0f * (1.0f - fogNearPercent);

  // Preencher a paleta com curva de fog não-linear.
  // Usar array temporário linear, depois aplicar CSM1 swizzle.
  uint8_t fogValues[CLUT_SIZE];

  for (int i = 0; i < CLUT_SIZE; i++) {
    if ((float)i >= clipThreshold) {
      // Objetos próximos (green byte alto) → sem fog
      fogValues[i] = 0;
    } else {
      // Calcular fog normalizado: 0.0 (no threshold) → 1.0 (far plane, i=0)
      float t = 1.0f - ((float)i / clipThreshold);
      // Aplicar curva não-linear: pow(t, intensity)
      // intensity=1.0 → linear, intensity=2.0 → quadrático (mais fog longe)
      float curved = powf(t, fogIntensity);
      // Escalar para range GS alpha (0-127, onde 128 = 1.0)
      int alpha = (int)(curved * 127.0f + 0.5f);
      if (alpha > 127) alpha = 127;
      if (alpha < 0) alpha = 0;
      fogValues[i] = (uint8_t)alpha;
    }
  }

  // Aplicar CSM1 swizzle: para que CLUT[i] retorne fogValues[i],
  // precisamos armazenar fogValues[i] na posição csm1Reorder(i) em VRAM.
  for (int i = 0; i < CLUT_SIZE; i++) {
    int swizzled = csm1Reorder(i);
    uint8_t v = fogValues[i];
    clutData[swizzled] = ((uint32_t)v << 24) | ((uint32_t)v << 16) |
                         ((uint32_t)v << 8) | (uint32_t)v;
  }

  // Alocar CLUT no final da VRAM (4MB - 8KB)
  // Endereço em WORD units (4 bytes) para compatibilidade com ps2sdk.
  // 4MB = 4194304 bytes. 4MB - 8KB = 4186112 bytes.
  // 4186112 / 4 = 1046528 (em word units)
  constexpr uint32_t CLUT_VRAM_ADDR_WORDS = (4 * 1024 * 1024 - 8 * 1024) / 4;
  clutVramAddress = CLUT_VRAM_ADDR_WORDS;

  // Upload via draw_texture_transfer (DMA chain com REF para clutData)
  qword_t packets[30] ALIGNED(64);
  qword_t* q = packets;

  q = draw_texture_transfer(q, clutData, CLUT_WIDTH, CLUT_HEIGHT, GS_PSM_32,
                            clutVramAddress, 256);
  q = draw_texture_flush(q);

  // CRITICAL: FlushCache APÓS construir os packets E preencher clutData.
  // O DMA controller lê da memória física — tanto os packets (DMA chain tags)
  // quanto clutData (via DMA REF). Ambos precisam estar flushed do cache.
  FlushCache(0);

  dma_channel_send_chain(DMA_CHANNEL_GIF, packets, q - packets, 0, 0);
  dma_channel_wait(DMA_CHANNEL_GIF, 500);

  free(clutData);
  clutDirty = false;

#ifdef DEBUG_MODE
  TYRA_LOG("[FOG] Fog CLUT uploaded (nearPercent=", fogNearPercent,
           ", intensity=", fogIntensity, ")");
  TYRA_LOG("[FOG] CLUT VRAM address (words): ", clutVramAddress);
  TYRA_LOG("[FOG] CLUT VRAM address (256B blocks / TEX0 CBP): ",
           clutVramAddress >> 6);
#endif
}

#ifdef DEBUG_MODE
void PostFxManager::saveDebugScreenshot(const char* filename, uint32_t address,
                                        uint32_t width, uint32_t height,
                                        uint32_t psm) {
  if (!g_debug_menu.fogTriggerScreenshot) return;

  TYRA_LOG("Saving fog debug screenshot: ", filename);
  ps2_screenshot_file(filename, address >> 6, width, height, psm);
}
#endif

void PostFxManager::fogPassSetup() {
  // ===== CONFIGURAÇÃO INICIAL =====
  // Define os parâmetros do GS para os passes subsequentes
  // - TEST: ALLPASS (sem testes, aceita tudo)
  // - ZBUF: Z-buffer writes DESABILITADAS
  // - XYOFFSET: (0,0) para coordenadas de pixel brutas
  // - PABE: Desabilitado
  // - SCISSOR: Resolução completa (512x448)
  // - COLCLAMP: Habilitado

#ifdef DEBUG_MODE
  if (g_debug_menu.fogPassSetup == false) return;
#endif

  qword_t packets[20] ALIGNED(64);
  qword_t* q = packets;

  // GIF tag: 6 registradores A+D
  PACK_GIFTAG(q, GIF_SET_TAG(6, 1, 0, 0, GIF_FLG_PACKED, 1), GIF_REG_AD);
  q++;

  // TEST_1: ALLPASS (aceita todos os pixels)
  PACK_GIFTAG(q, GS_SET_TEST(0, 0, 0, 0, 0, 0, 1, ZTEST_METHOD_ALLPASS),
              GS_REG_TEST_1);
  q++;

  // ZBUF_1: Z-buffer writes DESABILITADAS (mask=1)
  PACK_GIFTAG(q, GS_SET_ZBUF(fog_zbufferAddr >> 11, fog_zbufferPsm, 1),
              GS_REG_ZBUF_1);
  q++;

  // XYOFFSET_1: Offset (0,0)
  PACK_GIFTAG(q, GS_SET_XYOFFSET(0, 0), GS_REG_XYOFFSET_1);
  q++;

  // PABE: Per-pixel alpha blending desabilitado
  PACK_GIFTAG(q, GS_SET_PABE(0), GS_REG_PABE);
  q++;

  // SCISSOR_1: Clipping area (0,511) x (0,447)
  PACK_GIFTAG(q, GS_SET_SCISSOR(0, fog_width - 1, 0, fog_height - 1),
              GS_REG_SCISSOR_1);
  q++;

  // COLCLAMP: Color clamp habilitado
  PACK_GIFTAG(q, GS_SET_COLCLAMP(1), GS_REG_COLCLAMP);
  q++;

  FlushCache(0);
  dma_channel_send_normal(DMA_CHANNEL_GIF, packets, q - packets, 0, 0);
  dma_channel_wait(DMA_CHANNEL_GIF, 500);

#ifdef DEBUG_MODE
  saveDebugScreenshot("host:debug/fog_pass_setup.tga", fog_buf_frame.address,
                      fog_width, fog_height, fog_buf_frame.psm);
#endif
}

void PostFxManager::copyZBufferBlockToTemp(uint32_t zbufPage,
                                           uint32_t tempAddr) {
  // ===== LOCAL-TO-LOCAL TRANSFER =====
  // Copia um bloco 64x32 do Z-buffer (PSMZ_32) para um buffer temporário
  // (PSM_32), usando BITBLTBUF. Isso resolve o problema de swizzle entre
  // PSMZ_32 e PSM_8 ao ler o Z-buffer como textura.
  //
  // Baseado no código GTA SA PostFX (postfx.c →
  // CSkyEdgePostEffects_CopyDepthBuffer)

  qword_t packets[20] ALIGNED(64);
  qword_t* q = packets;

  // 4 registradores: BITBLTBUF, TRXPOS, TRXREG, TRXDIR
  PACK_GIFTAG(q, GIF_SET_TAG(4, 1, 0, 0, GIF_FLG_PACKED, 1), GIF_REG_AD);
  q++;

  // BITBLTBUF: Source=zbuffer page (PSMZ_32, BW=1), Dest=temp (PSM_32, BW=1)
  // BW=1 significa 64 pixels de largura (1 * 64)
  // zbufPage e tempAddr já estão em unidades de 256 bytes (64 words)
  PACK_GIFTAG(q,
              GS_SET_BITBLTBUF(zbufPage, 1, GS_PSMZ_32, tempAddr, 1, GS_PSM_32),
              GS_REG_BITBLTBUF);
  q++;

  // TRXPOS: Source (0,0) → Dest (0,0), upper-left to lower-right
  PACK_GIFTAG(q, GS_SET_TRXPOS(0, 0, 0, 0, 0), GS_REG_TRXPOS);
  q++;

  // TRXREG: 64x32 pixels
  PACK_GIFTAG(q, GS_SET_TRXREG(64, 32), GS_REG_TRXREG);
  q++;

  // TRXDIR: 2 = local-to-local
  PACK_GIFTAG(q, GS_SET_TRXDIR(2), GS_REG_TRXDIR);
  q++;

  FlushCache(0);
  dma_channel_send_normal(DMA_CHANNEL_GIF, packets, q - packets, 0, 0);
  dma_channel_wait(DMA_CHANNEL_GIF, 500);
}

void PostFxManager::performChannelCopyBlock(ColourChannels channelIn,
                                            ColourChannels channelOut,
                                            uint32_t blockX, uint32_t blockY,
                                            uint32_t source_addr) {
  // ===== CHANNEL COPY de um bloco 64x32 =====
  // Baseado em channel_copy.cpp e secore.c (GTA SA PostFX)
  //
  // Lê o buffer temporário (PSM_32 visto como PSM_8 + CLUT identidade)
  // e escreve o canal selecionado no framebuffer.

  // Offsets baseados no canal de entrada
  const uint32_t horz_block_offset =
      (channelIn == CHANNEL_BLUE || channelIn == CHANNEL_ALPHA) ? 1 : 0;
  const uint32_t vert_block_offset =
      (channelIn == CHANNEL_GREEN || channelIn == CHANNEL_ALPHA) ? 1 : 0;

  const uint32_t clamp_horz = horz_block_offset ? 8 : 0;
  const uint32_t clamp_vert = vert_block_offset ? 2 : 0;

  // TW/TH — usar dimensões da tela completa como no GTA SA
  // (set_tw_th(640, 448) → tw=10, th=9 no GTA SA)
  // Os UVs e CLAMP region repeat restringem os acessos ao bloco 64x32
  int tw, th;
  setTwTh(fog_width, fog_height, &tw, &th);

  // Frame mask baseado no canal de saída
  uint32_t frame_mask = 0xFFFFFFFF;
  switch (channelOut) {
    case CHANNEL_RED:
      frame_mask = ~0x000000FF;
      break;
    case CHANNEL_GREEN:
      frame_mask = ~0x0000FF00;
      break;
    case CHANNEL_BLUE:
      frame_mask = ~0x00FF0000;
      break;
    case CHANNEL_ALPHA:
      frame_mask = ~0xFF000000;
      break;
  }

  static qword_t packets[500] ALIGNED(64);
  qword_t* q = packets;

  // 5 registradores setup: XYOFFSET, TEX0, CLAMP, TEXFLUSH, FRAME
  PACK_GIFTAG(q, GIF_SET_TAG(5, 1, 0, 0, GIF_FLG_PACKED, 1), GIF_REG_AD);
  q++;

  // XYOFFSET: (0,0)
  PACK_GIFTAG(q, GS_SET_XYOFFSET(0, 0), GS_REG_XYOFFSET_1);
  q++;

  // TEX0: temp buffer como PSM_8, TBW=1, CLUT identidade
  // source_addr: em 64-word (256-byte) units — ok para TEX0 TBP0
  // clutVramAddress: em word units — >> 6 converte para 256-byte units (CBP)
  // TCC=1, TFX=1(DECAL), CSM=0(CSM1), CLD=1(load CLUT)
  PACK_GIFTAG(q,
              GS_SET_TEX0(source_addr, 1, GS_PSM_8, tw, th, 1, 1,
                          clutVramAddress >> 6, GS_PSM_32, 0, 0, 1),
              GS_REG_TEX0_1);
  q++;

  // CLAMP: Region repeat com offsets para selecionar canal
  PACK_GIFTAG(q, GS_SET_CLAMP(3, 3, 0xF7, clamp_horz, 0xFD, clamp_vert),
              GS_REG_CLAMP_1);
  q++;

  // TEXFLUSH
  PACK_GIFTAG(q, GS_SET_TEXFLUSH(1), GS_REG_TEXFLUSH);
  q++;

  // FRAME: Framebuffer como destino com mask para canal
  PACK_GIFTAG(q,
              GS_SET_FRAME(fog_buf_frame.address >> 11, fog_width >> 6,
                           GS_PSM_32, frame_mask),
              GS_REG_FRAME_1);
  q++;

  // GIF tag para 96 sprites com PRIM embutido via PRE
  // NLOOP=96: 4 EVEN rows × 4 sprites + 4×2 ODD rows × 8 sprites
  //         = 16 + 32×2... actually 4*4 + 4*8 = 16+32=48? No:
  // 32/2 = 16 row-pairs, each produces 4 or 8 sprites × 4 regs = variável
  // O total é sempre 96 sprites (96 NLOOP de 4 regs cada)
  PACK_GIFTAG(q,
              GIF_SET_TAG(96, 1, GIF_PRE_ENABLE,
                          GIF_SET_PRIM(GS_PRIM_SPRITE, 0, 1, 0, 0, 0, 1, 0, 0),
                          GIF_FLG_PACKED, 4),
              (GIF_REG_UV) | (GIF_REG_XYZ2 << 4) | (GIF_REG_UV << 8) |
                  (GIF_REG_XYZ2 << 12));
  q++;

  // Desenhar sprites com padrão de blocos alternados para channel shuffle
  for (int y = 0; y < 32; y += 2) {
    if (((y % 4) == 0) ^ (vert_block_offset == 1)) {
      // EVEN: 4 sprites de 16x2
      for (int x = 0; x < 64; x += 16) {
        // UV start
        PACK_GIFTAG(q, GS_SET_ST(8 + ((8 + x * 2) << 4), 8 + ((y * 2) << 4)),
                    0);
        q++;
        // XYZ2 start
        PACK_GIFTAG(q,
                    (uint64_t)((x + blockX) << 4) |
                        ((uint64_t)((y + blockY) << 4) << 32),
                    1);
        q++;
        // UV end
        PACK_GIFTAG(
            q, GS_SET_ST(8 + ((24 + x * 2) << 4), 8 + ((2 + y * 2) << 4)), 0);
        q++;
        // XYZ2 end
        PACK_GIFTAG(q,
                    (uint64_t)((x + 16 + blockX) << 4) |
                        ((uint64_t)((y + 2 + blockY) << 4) << 32),
                    1);
        q++;
      }
    } else {
      // ODD: 8 sprites de 8x2
      for (int x = 0; x < 64; x += 8) {
        // UV start
        PACK_GIFTAG(q, GS_SET_ST(8 + ((4 + x * 2) << 4), 8 + ((y * 2) << 4)),
                    0);
        q++;
        // XYZ2 start
        PACK_GIFTAG(q,
                    (uint64_t)((x + blockX) << 4) |
                        ((uint64_t)((y + blockY) << 4) << 32),
                    1);
        q++;
        // UV end
        PACK_GIFTAG(
            q, GS_SET_ST(8 + ((12 + x * 2) << 4), 8 + ((2 + y * 2) << 4)), 0);
        q++;
        // XYZ2 end
        PACK_GIFTAG(q,
                    (uint64_t)((x + 8 + blockX) << 4) |
                        ((uint64_t)((y + 2 + blockY) << 4) << 32),
                    1);
        q++;
      }
    }
  }

  FlushCache(0);
  dma_channel_send_normal(DMA_CHANNEL_GIF, packets, q - packets, 0, 0);
  dma_channel_wait(DMA_CHANNEL_GIF, 500);
}

void PostFxManager::copyDepthBuffer(ColourChannels channelIn,
                                    ColourChannels channelOut) {
  // ===== CHANNEL COPY FULL SCREEN =====
  // Processa a tela inteira em blocos de 64x32 pixels.
  //
  // Para cada bloco:
  // 1. BITBLTBUF local-to-local: copia 64x32 do Z-buffer (PSMZ_32)
  //    para buffer temporário (PSM_32) — resolve problema de swizzle
  // 2. Channel copy: lê temp buffer como PSM_8 + CLUT identidade,
  //    escreve canal selecionado no framebuffer
  //
  // Baseado em: GTA SA PostFX (CSkyEdgePostEffects_CopyDepthBuffer)
  // e channel_copy.cpp

  // Buffer temporário para BITBLTBUF local-to-local.
  // Precisa de 64×32×4 = 8192 bytes = 32 unidades de 256 bytes.
  //
  // Endereços para BITBLTBUF em unidades de 64 words (256 bytes).
  // GTA SA usa: zbuffer = ((ZBP) << 5) e incrementa page += 32
  // ZBP está em unidades de 2048 words (8192 bytes = 1 page PSMZ32)
  // Então (ZBP << 5) converte para unidades de 64 words (256 bytes)
  //
  // fog_zbufferAddr está em words (4 bytes), então:
  // fog_zbufferAddr >> 6 = unidades de 64 words (256 bytes)
  uint32_t zbufBase = fog_zbufferAddr >> 6;  // Converter words → 256-byte units

  // Temp buffer: alocado logo após o zbuffer na VRAM
  // zbuffer size(PSMZ_32) = width * height pixels = width * height words
  // Em 256-byte units (64-word blocks): width * height / 64
  uint32_t zbufSizeIn256ByteUnits = ((uint32_t)fog_width * (uint32_t)fog_height) / 64;
  uint32_t tempAddr = zbufBase + zbufSizeIn256ByteUnits;

  uint32_t widthInBlocks = (uint32_t)fog_width / 64;
  uint32_t heightInBlocks = (uint32_t)fog_height / 32;

  // Percorrer todos os blocos 64x32 da tela
  // page incrementa de 32 unidades por bloco (como no GTA SA: page += 32)
  uint32_t page = zbufBase;
  for (uint32_t by = 0; by < heightInBlocks; by++) {
    for (uint32_t bx = 0; bx < widthInBlocks; bx++) {
      uint32_t blockX = bx * 64;
      uint32_t blockY = by * 32;

      // 1. Copiar bloco do zbuffer (PSMZ_32) → temp (PSM_32) via BITBLTBUF
      copyZBufferBlockToTemp(page, tempAddr);

      // 2. Channel copy: ler temp como PSM_8, escrever no framebuffer
      performChannelCopyBlock(channelIn, channelOut, blockX, blockY,
                              tempAddr);  // tempAddr já em 256-byte units

      page += 32;  // Próximo page (32 × 256 = 8192 bytes = 1 page PSMZ_32)
    }
  }

  // Restaurar estado do GS após channel copy
  // NOTA: NÃO restaurar XYOFFSET aqui! Os passes 4/5/6 usam XYOFFSET=(0,0)
  // e coordenadas de pixel brutas. Restaurar XYOFFSET é responsabilidade
  // do fogPassRestore() no final de toda a pipeline.
  qword_t packets[20] ALIGNED(64);
  qword_t* q = packets;

  PACK_GIFTAG(q, GIF_SET_TAG(2, 1, 0, 0, GIF_FLG_PACKED, 1), GIF_REG_AD);
  q++;

  PACK_GIFTAG(q, GS_SET_CLAMP(1, 1, 0, 0, 0, 0), GS_REG_CLAMP_1);
  q++;

  PACK_GIFTAG(
      q,
      GS_SET_FRAME(fog_buf_frame.address >> 11, fog_buf_frame.width >> 6,
                   fog_buf_frame.psm, fog_buf_frame.mask),
      GS_REG_FRAME_1);
  q++;

  FlushCache(0);
  dma_channel_send_normal(DMA_CHANNEL_GIF, packets, q - packets, 0, 0);
  dma_channel_wait(DMA_CHANNEL_GIF, 500);
}

void PostFxManager::fogPass3ChannelCopy() {
  // ===== PASS 3: COPIAR CANAL G DO Z-BUFFER PARA ALPHA DO FRAMEBUFFER =====
  //
  // Per Sony doc "Using the Z Buffer for Visual and Special Effects":
  // - For 32-bit Z-Buffer, bits 15-8 (Green channel) map best for fog
  // - PSMZ_32 has reversed column order within blocks compared to PSM formats,
  //   so direct texture reads produce garbled output.
  //
  // Solution: BITBLTBUF local-to-local (PSMZ_32 → PSM_32) per 64x32 block
  // resolves swizzle. Then channel copy with PSM_8 + fog CLUT extracts the
  // Green byte and writes the fog alpha into framebuffer Alpha channel.
  //
  // A CLUT não-linear controla inversão (near→0, far→high), clipping
  // (near objects → alpha 0) e curva de fog (pow(t, intensity))
  // — substituindo os antigos passes 1 (invert) e 2 (clip).

#ifdef DEBUG_MODE
  if (g_debug_menu.fogPass3) {
#endif

    copyDepthBuffer(CHANNEL_GREEN, CHANNEL_ALPHA);

#ifdef DEBUG_MODE
    saveDebugScreenshot("host:debug/fog_pass3_channel_copy.tga",
                        fog_buf_frame.address, fog_width, fog_height,
                        fog_buf_frame.psm);
  }
#endif
}

void PostFxManager::fogPass6Apply(const Color& fogColor) {
  // ===== PASS 6: APLICAR COR DE FOG (LERP) =====
  //
  // Fórmula: Output = lerp(Cd, fogColor, Ad/128)
  //        = Cd + (fogColor - Cd) * Ad/128
  //
  // Usando GS alpha blend: ALPHA(0, 1, 1, 1, 0)
  //   A=0(Cs), B=1(Cd), C=1(Ad), D=1(Cd)
  //   Output = ((Cs - Cd) * Ad) >> 7 + Cd
  //          = Cd + (Cs - Cd) * Ad/128
  //
  // Onde:
  //   Cs = fogColor (RGBAQ, sem textura)
  //   Cd = framebuffer (cena original)
  //   Ad = alpha do framebuffer = fog mask do channel copy
  //
  // Resultado:
  //   Ad=0 (near, sem fog)  → Output = Cd (cena original)
  //   Ad=127 (far, fog max) → Output ≈ fogColor (fog completo)
  //
  // NOTA: O GS faz aritmética interna em 9 bits com sinal, então
  // (Cs - Cd) pode ser negativo sem problemas. COLCLAMP faz clamp
  // apenas no resultado final para [0, 255].

  if (fogColor.a == 0) return;

#ifdef DEBUG_MODE
  if (g_debug_menu.fogPass6) {
#endif

    qword_t packets[20] ALIGNED(64);
    qword_t* q = packets;

    // 5 registradores + 2 XYZ2 para sprite fullscreen
    PACK_GIFTAG(q, GIF_SET_TAG(5, 0, 0, 0, GIF_FLG_PACKED, 1), GIF_REG_AD);
    q++;

    // FRAME: Framebuffer como destino, proteger alpha (mask 0xFF000000)
    // para que a cor do fog não sobrescreva a depth mask
    PACK_GIFTAG(
        q,
        GS_SET_FRAME(fog_buf_frame.address >> 11, fog_buf_frame.width >> 6,
                     fog_buf_frame.psm, 0xFF000000),
        GS_REG_FRAME_1);
    q++;

    // RGBAQ: Cor do fog (Cs na fórmula de blend)
    PACK_GIFTAG(
        q,
        GS_SET_RGBAQ((int)fogColor.r, (int)fogColor.g, (int)fogColor.b, 0, 0),
        GS_REG_RGBAQ);
    q++;

    // ALPHA: (0,1,1,1,0) = ((Cs-Cd)*Ad)>>7 + Cd = lerp(Cd, Cs, Ad/128)
    PACK_GIFTAG(q, GS_SET_ALPHA(0, 1, 1, 1, 0), GS_REG_ALPHA_1);
    q++;

    // PRIM: Sprite sem textura, ABE=1 (alpha blend), FST=1
    PACK_GIFTAG(q, GS_SET_PRIM(GS_PRIM_SPRITE, 0, 0, 0, 1, 0, 1, 0, 0),
                GS_REG_PRIM);
    q++;

    // XYOFFSET: (0,0) garantir coordenadas corretas
    PACK_GIFTAG(q, GS_SET_XYOFFSET(0, 0), GS_REG_XYOFFSET_1);
    q++;

    // Sprite full-screen: 2 XYZ2 (EOP=1)
    PACK_GIFTAG(q, GIF_SET_TAG(2, 1, 0, 0, GIF_FLG_PACKED, 1), GIF_REG_AD);
    q++;

    // XYZ2 start: (0, 0)
    PACK_GIFTAG(q, GS_SET_XYZ(ftoi4(0), ftoi4(0), 0), GS_REG_XYZ2);
    q++;
    // XYZ2 end: (width, height)
    PACK_GIFTAG(q, GS_SET_XYZ(ftoi4(fog_width), ftoi4(fog_height), 0),
                GS_REG_XYZ2);
    q++;

    FlushCache(0);
    dma_channel_send_normal(DMA_CHANNEL_GIF, packets, q - packets, 0, 0);
    dma_channel_wait(DMA_CHANNEL_GIF, 500);

#ifdef DEBUG_MODE
    saveDebugScreenshot("host:debug/fog_pass6_final.tga", fog_buf_frame.address,
                        fog_width, fog_height, fog_buf_frame.psm);
  }
#endif
}

void PostFxManager::fogPassRestore() {
  // ===== RESTAURAÇÃO DO GS =====
  // Retorna o GS para estado normal após todos os passes
  // Restaura: FRAME, ZBUF, XYOFFSET, SCISSOR, TEST, PABE, ALPHA, COLCLAMP

#ifdef DEBUG_MODE
  if (g_debug_menu.fogPassRestore) {
#endif

    ALPHATEST atest;
    DESTTEST dtest;
    DEPTHTEST ztest;
    BLEND blend;

    blend.color1 = BLEND_COLOR_SOURCE;
    blend.color2 = BLEND_COLOR_DEST;
    blend.alpha = BLEND_ALPHA_SOURCE;
    blend.color3 = BLEND_COLOR_DEST;
    blend.fixed_alpha = 0x80;

    atest.enable = DRAW_ENABLE;
    atest.method = ATEST_METHOD_NOTEQUAL;
    atest.compval = 0x00;
    atest.keep = ATEST_KEEP_FRAMEBUFFER;

    dtest.enable = DRAW_DISABLE;
    dtest.pass = DRAW_DISABLE;

    ztest.enable = DRAW_ENABLE;
    ztest.method = fog_zbuffer.method;

    qword_t packets[20] ALIGNED(64);
    qword_t* q = packets;

    // GIF tag: 10 registradores A+D
    PACK_GIFTAG(q, GIF_SET_TAG(10, 1, 0, 0, GIF_FLG_PACKED, 1), GIF_REG_AD);
    q++;

    // FRAME_1: Restaurar framebuffer original
    PACK_GIFTAG(
        q,
        GS_SET_FRAME(fog_buf_frame.address >> 11, fog_buf_frame.width >> 6,
                     fog_buf_frame.psm, fog_buf_frame.mask),
        GS_REG_FRAME_1);
    q++;

    // ZBUF_1: Restaurar Z-buffer original
    PACK_GIFTAG(q,
                GS_SET_ZBUF(fog_zbuffer.address >> 11, fog_zbuffer.zsm,
                            fog_zbuffer.mask),
                GS_REG_ZBUF_1);
    q++;

    // XYOFFSET_1: Restaurar offset de primitivas
    // Calcula offset baseado no tamanho do framebuffer
    PACK_GIFTAG(
        q,
        GS_SET_XYOFFSET(
            (int)(screenCenter - (fog_buf_frame.width / 2.0F) * 16.0f),
            (int)(screenCenter - (fog_buf_frame.height / 2.0F) * 16.0f)),
        GS_REG_XYOFFSET_1);
    q++;

    // SCISSOR_1: Restaurar scissoring area (resolução completa)
    PACK_GIFTAG(
        q,
        GS_SET_SCISSOR(0, fog_buf_frame.width - 1, 0, fog_buf_frame.height - 1),
        GS_REG_SCISSOR_1);
    q++;

    // TEST_1: Restaurar testes normais
    PACK_GIFTAG(
        q,
        GS_SET_TEST(atest.enable, atest.method, atest.compval, atest.keep,
                    dtest.enable, dtest.pass, ztest.enable, ztest.method),
        GS_REG_TEST_1);
    q++;

    // PABE: Restaurar per-pixel alpha blending (desabilitado)
    PACK_GIFTAG(q, GS_SET_PABE(DRAW_DISABLE), GS_REG_PABE);
    q++;

    // ALPHA_1: Restaurar blend defaults
    PACK_GIFTAG(q,
                GS_SET_ALPHA(blend.color1, blend.color2, blend.alpha,
                             blend.color3, blend.fixed_alpha),
                GS_REG_ALPHA_1);
    q++;

    // COLCLAMP: Restaurar color clamp (habilitado)
    PACK_GIFTAG(q, GS_SET_COLCLAMP(GS_ENABLE), GS_REG_COLCLAMP);
    q++;

    // CLAMP_1: Restaurar clamp mode padrão (REPEAT para S e T)
    PACK_GIFTAG(q, GS_SET_CLAMP(0, 0, 0, 0, 0, 0), GS_REG_CLAMP_1);
    q++;

    // TEX1_1: Restaurar filtro de textura padrão (NEAREST)
    PACK_GIFTAG(q, GS_SET_TEX1(0, 0, 0, 0, 0, 0, 0), GS_REG_TEX1_1);
    q++;

    FlushCache(0);
    dma_channel_send_normal(DMA_CHANNEL_GIF, packets, q - packets, 0, 0);
    dma_channel_wait(DMA_CHANNEL_GIF, 500);

#ifdef DEBUG_MODE
  }

  // Reset trigger após capturar todos os passos
  if (g_debug_menu.fogTriggerScreenshot) {
    g_debug_menu.fogTriggerScreenshot = false;
    TYRA_LOG("Fog screenshots capture completed. Trigger reset.");
  }
#endif
}

void PostFxManager::renderFog(Color fogColor) {
#ifdef DEBUG_MODE
  if (g_debug_menu.enablePostFx == false) return;
#endif  // DEBUG_MODE

  // ===== STRATEGY B: Pipeline simplificada de fog pós-processamento =====
  //
  // A CLUT não-linear faz todo o trabalho de inversão, clipping e curva
  // numa única lookup durante o channel copy. Isso elimina os antigos
  // passes 1 (invert Z), 2 (clip Z), 4 (downsample) e 5 (blur/blend).
  //
  // Pipeline:
  //   Setup → Channel Copy (G→Alpha via CLUT) → Apply Fog Color → Restore
  //
  // A CLUT converte o green byte do Z-buffer (near=255, far=0) em fog alpha:
  //   - Near objects → alpha 0 (sem fog)
  //   - Far objects → alpha 127 (fog máximo)
  //   - Curva não-linear controlada por fogIntensity (pow)
  //   - Clipping de near objects controlado por fogNearPercent

  // Configurar propriedades reutilizáveis
  fog_width = SCREEN_WIDTH;
  fog_height = SCREEN_HEIGHT;
  setTwTh(fog_width, fog_height, &fog_tw, &fog_th);

  fog_zbuffer = pRenderer->core.gs.zBuffer;
  fog_buf_frame = pRenderer->core.gs.getCurrentFrameData();
  fog_zbufferAddr = fog_zbuffer.address;
  fog_zbufferPsm = fog_zbuffer.zsm;

  // Re-upload CLUT se os parâmetros mudaram
  if (clutDirty) {
    uploadFogCLUT();
  }

  // Pipeline simplificada: 4 etapas em vez de 8
  fogPassSetup();
  fogPass3ChannelCopy();
  fogPass6Apply(fogColor);
  fogPassRestore();
}
