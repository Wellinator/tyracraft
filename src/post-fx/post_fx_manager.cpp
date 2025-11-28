#include "managers/post-fx/post_fx_manager.hpp"
#include "managers/dma_gif_builder.hpp"
#include "debug.hpp"
#include <gs_gp.h>
#include <gs_psm.h>
#include <dma_tags.h>

#include <stdlib.h>
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

// Channel copy a block
void PostFxManager::performChannelCopy(ColourChannels channelIn,
                                       ColourChannels channelOut,
                                       uint32_t blockX, uint32_t blockY,
                                       uint32_t source_addr, uint32_t width,
                                       uint32_t height, uint32_t dest) {
  const framebuffer_t buf_frame = pRenderer->core.gs.getCurrentFrameData();

  // For the BLUE and ALPHA channels, we need to offset our 'U's by 8 texels
  const uint32_t horz_block_offset =
      (channelIn == CHANNEL_BLUE || channelIn == CHANNEL_ALPHA);
  // For the GREEN and ALPHA channels, we need to offset our 'T's by 2 texels
  const uint32_t vert_block_offset =
      (channelIn == CHANNEL_GREEN || channelIn == CHANNEL_ALPHA);

  const uint32_t clamp_horz = horz_block_offset ? 8 : 0;
  const uint32_t clamp_vert = vert_block_offset ? 2 : 0;

  qword_t packets[500] ALIGNED(64);
  qword_t* q = packets;

  PACK_GIFTAG(q, GIF_SET_TAG(4, 1, 0, 0, GIF_FLG_PACKED, 1), GIF_REG_AD);
  q++;

  //   PACK_GIFTAG(q, GS_SET_XYOFFSET(0, 0), GS_REG_XYOFFSET_1);
  //   q++;

  int tw, th;
  setTwTh(width, height, &tw, &th);
  PACK_GIFTAG(q,
              GS_SET_TEX0(source_addr >> 6, 1, GS_PSM_8, tw, th, 1, 1,
                          dest >> 6, GS_PSM_32, 0, 0, 1),
              GS_REG_TEX0_1);
  q++;

  PACK_GIFTAG(q,
              GS_SET_CLAMP(WRAP_REGION_REPEAT, WRAP_REGION_REPEAT, 0xF7,
                           clamp_horz, 0xFD, clamp_vert),
              GIF_REG_CLAMP_1);
  q++;

  PACK_GIFTAG(q, GS_SET_TEXFLUSH(1), GS_REG_TEXFLUSH);
  q++;

  uint32_t frame_mask;
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
    default:
      frame_mask = ~0x00FF0000;
      break;
  }

  PACK_GIFTAG(q,
              GS_SET_FRAME(buf_frame.address >> 11, buf_frame.width >> 6,
                           buf_frame.psm, frame_mask),
              GS_REG_FRAME_1);
  q++;

  PACK_GIFTAG(
      q,
      GIF_SET_TAG(96, 1, 1, GS_SET_PRIM(GS_PRIM_SPRITE, 0, 1, 0, 0, 0, 1, 0, 0),
                  GIF_FLG_PACKED, 4),
      (GIF_REG_UV) | (GIF_REG_XYZ2 << 4) | (GIF_REG_UV << 8) |
          (GIF_REG_XYZ2 << 12));
  q++;

  int y;
  for (y = 0; y < 32; y += 2) {
    if (((y % 4) == 0) ^ (vert_block_offset == 1))  // Even (4 16x2 sprites)
    {
      int x;
      for (x = 0; x < 64; x += 16) {
        // UV
        PACK_GIFTAG(q, GIF_SET_ST(8 + ((8 + x * 2) << 4), 8 + ((y * 2) << 4)),
                    0);
        q++;

        // XYZ2
        PACK_GIFTAG(q,
                    (uint64_t)((x + blockX) << 4) |
                        ((uint64_t)((y + blockY) << 4) << 32),
                    1);
        q++;

        // UV
        PACK_GIFTAG(
            q, GIF_SET_ST(8 + ((24 + x * 2) << 4), 8 + ((2 + y * 2) << 4)), 0);
        q++;

        // XYZ2
        PACK_GIFTAG(q,
                    (uint64_t)((x + 16 + blockX) << 4) |
                        ((uint64_t)((y + 2 + blockY) << 4) << 32),
                    1);
        q++;
      }
    } else  // Odd (Eight 8x2 sprites)
    {
      int x;
      for (x = 0; x < 64; x += 8) {
        // UV
        PACK_GIFTAG(q, GIF_SET_ST(8 + ((4 + x * 2) << 4), 8 + ((y * 2) << 4)),
                    0);
        q++;

        // XYZ2
        PACK_GIFTAG(q,
                    (uint64_t)((x + blockX) << 4) |
                        ((uint64_t)((y + blockY) << 4) << 32),
                    1);
        q++;

        // UV
        PACK_GIFTAG(
            q, GIF_SET_ST(8 + ((12 + x * 2) << 4), 8 + ((2 + y * 2) << 4)), 0);
        q++;

        // XYZ2
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
  q = packets;

  PACK_GIFTAG(q, GIF_SET_TAG(1, 1, 0, 0, GIF_FLG_PACKED, 1), GIF_REG_AD);
  q++;

  PACK_GIFTAG(q, GS_SET_CLAMP(WRAP_CLAMP, WRAP_CLAMP, 0, 0, 0, 0),
              GS_REG_CLAMP_1);
  q++;

  //   PACK_GIFTAG(q,
  //               GS_SET_FRAME(buf_frame.address >> 11, buf_frame.width >> 6,
  //                            buf_frame.psm, buf_frame.mask),
  //               GS_REG_FRAME_1);
  //   q++;

  //   PACK_GIFTAG(q,
  //               GS_SET_XYOFFSET(
  //                   (int)(screenCenter - (settings.getWidth() / 2.0F)
  //                   * 16.0f), (int)(screenCenter - (settings.getHeight()
  //                   / 2.0F) * 16.0f)),
  //               GS_REG_XYOFFSET_1);
  //   q++;

  dma_channel_send_normal(DMA_CHANNEL_GIF, packets, q - packets, 0, 0);
  dma_channel_wait(DMA_CHANNEL_GIF, 500);
};

void PostFxManager::setTwTh(int w, int h, int* tw, int* th) {
  *tw = 31 - (lzw(w) + 1);
  if (w > (1 << *tw)) (*tw)++;

  *th = 31 - (lzw(h) + 1);
  if (h > (1 << *th)) (*th)++;
}

PostFxManager::PostFxManager(Renderer* renderer)
    : Singleton<PostFxManager>(), settings(renderer->core.getSettings()) {
  pRenderer = renderer;
};

PostFxManager::~PostFxManager() {};

void PostFxManager::renderFog(Color fogColor) {
#ifdef DEBUG_MODE
  if (g_debug_menu.enablePostFx == false) return;
#endif  // DEBUG_MODE

  const uint32_t width = settings.getWidth();
  const uint32_t height = settings.getHeight();
  const uint32_t halfWidth = width / 2;
  const uint32_t halfHeight = height / 2;

  const zbuffer_t zbuffer = pRenderer->core.gs.zBuffer;
  const framebuffer_t buf_frame = pRenderer->core.gs.getCurrentFrameData();

  const uint32_t zbufferAddr = zbuffer.address;
  const uint32_t zbufferPsm = zbuffer.zsm;

  qword_t packets[200] ALIGNED(64);
  qword_t* q = packets;
  //   const uint32_t CLIP_ZVALUE = 0xff8000;
  const uint32_t CLIP_ZVALUE = 0xFFF000;

  // ===== PASS 1 & 2: Clipar Z-Buffer =====
  // Remove objetos próximos do zbuffer para que não recebam fog

  // Setup inicial
#ifdef DEBUG_MODE
  if (g_debug_menu.fogPassSetup) {
#endif
    PACK_GIFTAG(q, GIF_SET_TAG(6, 1, 0, 0, GIF_FLG_PACKED, 1), GIF_REG_AD);
    q++;
    PACK_GIFTAG(q, GS_SET_XYOFFSET(0, 0), GS_REG_XYOFFSET_1);
    q++;
    PACK_GIFTAG(q, GS_SET_TEST(0, 0, 0, 0, 0, 0, 1, ZTEST_METHOD_ALLPASS),
                GS_REG_TEST_1);
    q++;
    PACK_GIFTAG(q, GS_SET_ZBUF(zbufferAddr >> 11, zbufferPsm, 1),
                GS_REG_ZBUF_1);
    q++;
    PACK_GIFTAG(q, GS_SET_PABE(0), GS_REG_PABE);
    q++;
    PACK_GIFTAG(q, GS_SET_SCISSOR(0, width - 1, 0, height - 1),
                GS_REG_SCISSOR_1);
    q++;
    PACK_GIFTAG(q, GS_SET_COLCLAMP(1), GS_REG_COLCLAMP);
    q++;

    FlushCache(0);
    dma_channel_send_normal(DMA_CHANNEL_GIF, packets, q - packets, 0, 0);
    dma_channel_wait(DMA_CHANNEL_GIF, 500);
#ifdef DEBUG_MODE
  }
#endif

  // 1st pass: invert z-buffer values
#ifdef DEBUG_MODE
  if (g_debug_menu.fogPass1) {
#endif
    q = packets;
    PACK_GIFTAG(q, GIF_SET_TAG(6, 1, 0, 0, GIF_FLG_PACKED, 1), GIF_REG_AD);
    q++;

    PACK_GIFTAG(
        q, GS_SET_FRAME(zbufferAddr >> 11, width >> 6, zbufferPsm, 0xff000000),
        GS_REG_FRAME_1);
    q++;

    PACK_GIFTAG(q, GS_SET_ALPHA(0, 1, 2, 2, 128), GS_REG_ALPHA_1);
    q++;

    PACK_GIFTAG(q, GS_SET_PRIM(GS_PRIM_SPRITE, 0, 0, 0, 1, 0, 0, 0, 0),
                GS_REG_PRIM);
    q++;
    PACK_GIFTAG(q, GS_SET_RGBAQ(255, 255, 255, 128, 0), GS_REG_RGBAQ);
    q++;
    PACK_GIFTAG(q, GS_SET_XYZ(ftoi4(0), ftoi4(0), 0), GS_REG_XYZ2);
    q++;
    PACK_GIFTAG(q, GS_SET_XYZ(ftoi4(width), ftoi4(height), 0), GS_REG_XYZ2);
    q++;
    FlushCache(0);
    dma_channel_send_normal(DMA_CHANNEL_GIF, packets, q - packets, 0, 0);
    dma_channel_wait(DMA_CHANNEL_GIF, 500);
#ifdef DEBUG_MODE
  }
#endif

  // PASS 2: Clipar objetos próximos
#ifdef DEBUG_MODE
  if (g_debug_menu.fogPass2) {
#endif
    q = packets;
    PACK_GIFTAG(q, GIF_SET_TAG(8, 1, 0, 0, GIF_FLG_PACKED, 1), GIF_REG_AD);
    q++;
    PACK_GIFTAG(q, GS_SET_TEST(1, 0, 0, 2, 0, 0, 1, ZTEST_METHOD_GREATER),
                GS_REG_TEST_1);
    q++;
    PACK_GIFTAG(q, GS_SET_ZBUF(zbufferAddr >> 11, zbufferPsm, 0),
                GS_REG_ZBUF_1);
    q++;
    PACK_GIFTAG(q, GS_SET_PRIM(GS_PRIM_SPRITE, 0, 0, 0, 0, 0, 0, 0, 0),
                GS_REG_PRIM);
    q++;
    PACK_GIFTAG(q, GS_SET_RGBAQ(0, 0, 0, 0, 0), GS_REG_RGBAQ);
    q++;

    PACK_GIFTAG(q, GS_SET_XYZ(ftoi4(0), ftoi4(0), CLIP_ZVALUE), GS_REG_XYZ2);
    q++;
    PACK_GIFTAG(q, GS_SET_XYZ(ftoi4(width), ftoi4(height), CLIP_ZVALUE),
                GS_REG_XYZ2);
    q++;
    PACK_GIFTAG(q, GS_SET_TEST(0, 0, 0, 0, 0, 0, 1, ZTEST_METHOD_ALLPASS),
                GS_REG_TEST_1);
    q++;
    PACK_GIFTAG(q, GS_SET_ZBUF(zbufferAddr >> 11, zbufferPsm, 1),
                GS_REG_ZBUF_1);
    q++;

    FlushCache(0);
    dma_channel_send_normal(DMA_CHANNEL_GIF, packets, q - packets, 0, 0);
    dma_channel_wait(DMA_CHANNEL_GIF, 500);
#ifdef DEBUG_MODE
  }
#endif

  // ===== PASS 3: Copiar canal G do Z-buffer para canal Alpha do framebuffer
  // ===== Isso cria uma máscara de profundidade no alpha do framebuffer
#ifdef DEBUG_MODE
  if (g_debug_menu.fogPass3) {
#endif

    // performChannelCopy(CHANNEL_GREEN, CHANNEL_ALPHA, width, height,
    // zbufferAddr, width, height, 0);

    /*
     */
    q = packets;
    PACK_GIFTAG(q, GIF_SET_TAG(8, 1, 0, 0, GIF_FLG_PACKED, 1), GIF_REG_AD);
    q++;

    PACK_GIFTAG(q,
                GS_SET_FRAME(buf_frame.address >> 11, width >> 6, GS_PSM_16S,
                             0x00003fff),
                GS_REG_FRAME);
    q++;

    PACK_GIFTAG(q, GS_SET_TEXFLUSH(0), GS_REG_TEXFLUSH);
    q++;

    int tw, th;
    setTwTh(width, height, &tw, &th);
    PACK_GIFTAG(q,
                GS_SET_TEX0(zbufferAddr >> 6, width >> 6, GS_PSM_16S, tw, th, 1,
                            1, 0, GS_PSM_32, 0, 0, 0),
                GS_REG_TEX0_1);
    q++;
    // PACK_GIFTAG(q,
    //             GS_SET_TEX0(zbufferAddr >> 6, width >> 6, GS_PSM_16S, 10, 10,
    //             1,
    //                         1, 0, 0, 0, 0, 0),
    //             GS_REG_TEX0_1);
    // q++;

    PACK_GIFTAG(q, GS_SET_TEX1(0, 0, 0, 0, 0, 0, 0), GS_REG_TEX1_1);
    q++;
    PACK_GIFTAG(q, GS_SET_TEXA(0, 0, 0), GS_REG_TEXA);
    q++;
    PACK_GIFTAG(q, GS_SET_ALPHA(0, 2, 2, 2, 64), GS_REG_ALPHA_1);
    q++;
    PACK_GIFTAG(q, GS_SET_SCISSOR(0, width - 1, 0, (height * 2) - 1),
                GS_REG_SCISSOR_1);
    q++;
    PACK_GIFTAG(q, GS_SET_PRIM(GS_PRIM_SPRITE, 0, 1, 0, 1, 0, 1, 0, 0),
                GS_REG_PRIM);
    q++;

    FlushCache(0);
    dma_channel_send_normal(DMA_CHANNEL_GIF, packets, q - packets, 0, 0);
    dma_channel_wait(DMA_CHANNEL_GIF, 500);

    // Desenhar strips para realizar o channel shuffle
    // Isso é necessário por causa da forma como o PS2 armazena pixels em blocos

    int strips = width >> 4;
    for (int i = 0; i < strips; i++) {
      q = packets;
      // GIF tag em modo REGLIST: NLOOP=1 significa processar NREG=4
      // registradores uma vez Ordem: UV, XYZ2, UV, XYZ2 (primeiro e segundo
      // vértice do sprite)
      PACK_GIFTAG(
          q,
          GIF_SET_TAG(1, 1, GS_SET_PRIM(GS_PRIM_SPRITE, 0, 1, 0, 1, 0, 1, 0, 0),
                      0, GIF_FLG_REGLIST, 4),
          (GIF_REG_UV) | (GIF_REG_XYZ2 << 4) | (GIF_REG_UV << 8) |
              (GIF_REG_XYZ2 << 12));
      q++;

      // Em modo REGLIST, cada qword contém dados para 2 registradores (64 bits
      // cada) Primeiro qword: UV (lower 64 bits) + XYZ2 (upper 64 bits)
      q->dw[0] = GS_SET_UV(8 + (i << 8), 8);
      q->dw[1] = GS_SET_XYZ((8 << 4) + (i << 8), (0 << 4), 0);
      q++;

      // Segundo qword: UV (lower 64 bits) + XYZ2 (upper 64 bits)
      q->dw[0] = GS_SET_UV(8 + (8 << 4) + (i << 8), 8 + ((height * 2) << 4));
      q->dw[1] =
          GS_SET_XYZ((8 << 4) + (8 << 4) + (i << 8), ((height * 2) << 4), 0);
      q++;

      FlushCache(0);
      dma_channel_send_normal(DMA_CHANNEL_GIF, packets, q - packets, 0, 0);
      dma_channel_wait(DMA_CHANNEL_GIF, 500);
    }

    // Restaurar scissor e texa
    q = packets;
    PACK_GIFTAG(q, GIF_SET_TAG(2, 1, 0, 0, GIF_FLG_PACKED, 1), GIF_REG_AD);
    q++;
    PACK_GIFTAG(q, GS_SET_TEXA(0, 0, 128), GS_REG_TEXA);
    q++;
    PACK_GIFTAG(q, GS_SET_SCISSOR(0, width - 1, 0, height - 1),
                GS_REG_SCISSOR_1);
    q++;

    FlushCache(0);
    dma_channel_send_normal(DMA_CHANNEL_GIF, packets, q - packets, 0, 0);
    dma_channel_wait(DMA_CHANNEL_GIF, 500);

#ifdef DEBUG_MODE
  }
#endif

  // 4th pass: downsample framebuffer to workbuffer
#ifdef DEBUG_MODE
  if (g_debug_menu.fogPass4) {
#endif
    q = packets;
    PACK_GIFTAG(q, GIF_SET_TAG(9, 1, 0, 0, GIF_FLG_PACKED, 1), GIF_REG_AD);
    q++;

    PACK_GIFTAG(
        q, GS_SET_FRAME(zbufferAddr >> 11, width >> 6, zbufferPsm, 0xFF000000),
        GS_REG_FRAME_1);
    q++;

    PACK_GIFTAG(q, GS_SET_TEXFLUSH(0), GS_REG_TEXFLUSH);
    q++;

    PACK_GIFTAG(q,
                GS_SET_TEX0(buf_frame.address >> 6, width >> 6, buf_frame.psm,
                            10, 10, 0, 1, 0, 0, 0, 0, 0),
                GS_REG_TEX0_1);
    q++;
    PACK_GIFTAG(q, GS_SET_TEX1(0, 0, 0, 0, 0, 0, 0), GS_REG_TEX1_1);
    q++;

    PACK_GIFTAG(q, GS_SET_PRIM(GS_PRIM_SPRITE, 0, 1, 0, 0, 0, 1, 0, 0),
                GS_REG_PRIM);
    q++;
    PACK_GIFTAG(q, GS_SET_UV(ftoi4(0), ftoi4(0)), GS_REG_UV);
    q++;
    PACK_GIFTAG(q, GS_SET_XYZ(ftoi4(0), ftoi4(0), 0), GS_REG_XYZ2);
    q++;
    PACK_GIFTAG(q, GS_SET_UV(ftoi4(width), ftoi4(height)), GS_REG_UV);
    q++;
    PACK_GIFTAG(q, GS_SET_XYZ(ftoi4(halfWidth), ftoi4(halfHeight), 0),
                GS_REG_XYZ2);
    q++;

    FlushCache(0);
    dma_channel_send_normal(DMA_CHANNEL_GIF, packets, q - packets, 0, 0);
    dma_channel_wait(DMA_CHANNEL_GIF, 500);
#ifdef DEBUG_MODE
  }
#endif

  // 5th pass x2: draw workbuffer onto framebuffer with dest alpha (z-buf
  // channel)
#ifdef DEBUG_MODE
  if (g_debug_menu.fogPass5) {
#endif
    q = packets;
    PACK_GIFTAG(q, GIF_SET_TAG(13, 1, 0, 0, GIF_FLG_PACKED, 1), GIF_REG_AD);
    q++;

    PACK_GIFTAG(q,
                GS_SET_FRAME(buf_frame.address >> 11, width >> 6, buf_frame.psm,
                             0xFF000000),
                GS_REG_FRAME_1);
    q++;

    PACK_GIFTAG(q, GS_SET_TEXFLUSH(0), GS_REG_TEXFLUSH);
    q++;

    PACK_GIFTAG(q,
                GS_SET_TEX0(zbufferAddr >> 6, width >> 6, buf_frame.psm, 10, 10,
                            1, 1, 0, 0, 0, 0, 0),
                GS_REG_TEX0_1);
    q++;

    PACK_GIFTAG(q, GS_SET_ALPHA(1, 0, 1, 0, 0), GS_REG_ALPHA_1);
    q++;

    PACK_GIFTAG(q, GS_SET_PRIM(GS_PRIM_SPRITE, 0, 1, 0, 1, 0, 1, 0, 0),
                GS_REG_PRIM);
    q++;

    const int32_t offset = 16;

    PACK_GIFTAG(q, GS_SET_UV(ftoi4(1), ftoi4(1)), GS_REG_UV);
    q++;

    PACK_GIFTAG(q, GS_SET_XYZ(ftoi4(1) + offset, ftoi4(1) + offset, 0),
                GS_REG_XYZ2);
    q++;

    PACK_GIFTAG(q, GS_SET_UV(ftoi4(width), ftoi4(height)), GS_REG_UV);
    q++;

    PACK_GIFTAG(
        q, GS_SET_XYZ(ftoi4(width + 1) + offset, ftoi4(height + 1) + offset, 0),
        GS_REG_XYZ2);
    q++;

    PACK_GIFTAG(q, GS_SET_UV(ftoi4(0) + offset, ftoi4(0) + offset), GS_REG_UV);
    q++;

    PACK_GIFTAG(q, GS_SET_XYZ(ftoi4(0), ftoi4(0), 0), GS_REG_XYZ2);
    q++;

    PACK_GIFTAG(q, GS_SET_UV(ftoi4(halfWidth), ftoi4(halfHeight)), GS_REG_UV);
    q++;

    PACK_GIFTAG(q, GS_SET_XYZ(ftoi4(width) - offset, ftoi4(height) - offset, 0),
                GS_REG_XYZ2);
    q++;

    FlushCache(0);
    dma_channel_send_normal(DMA_CHANNEL_GIF, packets, q - packets, 0, 0);
    dma_channel_wait(DMA_CHANNEL_GIF, 500);
#ifdef DEBUG_MODE
  }
#endif

#ifdef DEBUG_MODE
  if (g_debug_menu.fogPass6) {
#endif
    if (fogColor.a > 0) {
      q = packets;
      PACK_GIFTAG(q, GIF_SET_TAG(9, 1, 0, 0, GIF_FLG_PACKED, 1), GIF_REG_AD);
      q++;

      PACK_GIFTAG(
          q,
          GS_SET_RGBAQ((int)fogColor.r, (int)fogColor.g, (int)fogColor.b, 0, 0),
          GS_REG_RGBAQ);
      q++;

      // 6th pass: add saturation
      PACK_GIFTAG(q, GS_SET_TEXFLUSH(0), GS_REG_TEXFLUSH);
      q++;

      PACK_GIFTAG(q,
                  GS_SET_TEX0(buf_frame.address >> 6, width >> 6, buf_frame.psm,
                              10, 10, 1, 0, 0, 0, 0, 0, 0),
                  GS_REG_TEX0);
      q++;

      PACK_GIFTAG(q, GS_SET_PRIM(GS_PRIM_SPRITE, 0, 1, 0, 1, 0, 1, 0, 0),
                  GS_REG_PRIM);
      q++;

      PACK_GIFTAG(q, GS_SET_ALPHA(0, 2, 1, 1, 0), GS_REG_ALPHA_1);
      q++;

      PACK_GIFTAG(q, GS_SET_UV(ftoi4(0), ftoi4(0)), GS_REG_UV);
      q++;
      PACK_GIFTAG(q, GS_SET_XYZ(ftoi4(0), ftoi4(0), 0), GS_REG_XYZ2);
      q++;
      PACK_GIFTAG(q, GS_SET_UV(ftoi4(width), ftoi4(height)), GS_REG_UV);
      q++;
      PACK_GIFTAG(q, GS_SET_XYZ(ftoi4(width), ftoi4(height), 0), GS_REG_XYZ2);
      q++;

      FlushCache(0);
      dma_channel_send_normal(DMA_CHANNEL_GIF, packets, q - packets, 0, 0);
      dma_channel_wait(DMA_CHANNEL_GIF, 500);

      // 6th pass: add fog
      q = packets;
      PACK_GIFTAG(q, GIF_SET_TAG(4, 1, 0, 0, GIF_FLG_PACKED, 1), GIF_REG_AD);
      q++;

      PACK_GIFTAG(q, GS_SET_ALPHA(0, 2, 1, 1, 0), GS_REG_ALPHA_1);
      q++;

      PACK_GIFTAG(q, GS_SET_PRIM(GS_PRIM_SPRITE, 0, 0, 0, 1, 0, 1, 0, 0),
                  GS_REG_PRIM);
      q++;

      PACK_GIFTAG(q, GS_SET_XYZ(0, 0, 0), GS_REG_XYZ2);
      q++;

      PACK_GIFTAG(q, GS_SET_XYZ(ftoi4(width), ftoi4(height), 0), GS_REG_XYZ2);
      q++;

      FlushCache(0);
      dma_channel_send_normal(DMA_CHANNEL_GIF, packets, q - packets, 0, 0);
      dma_channel_wait(DMA_CHANNEL_GIF, 500);
    }
#ifdef DEBUG_MODE
  }
#endif

  //   Restore GS working frame buffer
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
    ztest.method = zbuffer.method;

    q = packets;
    PACK_GIFTAG(q, GIF_SET_TAG(8, 1, 0, 0, GIF_FLG_PACKED, 1), GIF_REG_AD);
    q++;

    PACK_GIFTAG(q,
                GS_SET_FRAME(buf_frame.address >> 11, buf_frame.width >> 6,
                             buf_frame.psm, buf_frame.mask),
                GS_REG_FRAME_1);
    q++;

    // ZBuffer setting
    PACK_GIFTAG(q,
                GS_SET_ZBUF(zbuffer.address >> 11, zbuffer.zsm, zbuffer.mask),
                GS_REG_ZBUF);
    q++;

    // Primitive coordinate offsets
    PACK_GIFTAG(q, GS_SET_XYOFFSET(0, 0), GS_REG_XYOFFSET_1);
    q++;

    // Scissoring area
    PACK_GIFTAG(q,
                GS_SET_SCISSOR(0, buf_frame.width - 1, 0, buf_frame.height - 1),
                GS_REG_SCISSOR);
    q++;

    // Pixel testing
    PACK_GIFTAG(
        q,
        GS_SET_TEST(atest.enable, atest.method, atest.compval, atest.keep,
                    dtest.enable, dtest.pass, ztest.enable, ztest.method),
        GS_REG_TEST);
    q++;

    // Per-pixel Alpha Blending (Blends if MSB of ALPHA is true)
    PACK_GIFTAG(q, GS_SET_PABE(DRAW_DISABLE), GS_REG_PABE);
    q++;

    // Alpha Blending
    PACK_GIFTAG(q,
                GS_SET_ALPHA(blend.color1, blend.color2, blend.alpha,
                             blend.color3, blend.fixed_alpha),
                GS_REG_ALPHA);
    q++;

    // Color Clamp
    PACK_GIFTAG(q, GS_SET_COLCLAMP(GS_ENABLE), GS_REG_COLCLAMP);
    q++;

    FlushCache(0);
    dma_channel_send_normal(DMA_CHANNEL_GIF, packets, q - packets, 0, 0);
    dma_channel_wait(DMA_CHANNEL_GIF, 500);
#ifdef DEBUG_MODE
  }
#endif
}
