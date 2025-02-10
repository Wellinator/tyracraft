#include "managers/post-fx/post_fx_manager.hpp"
#include <gs_gp.h>
#include <gs_psm.h>
#include <dma_tags.h>

using Tyra::Color;
using Tyra::RendererCoreTextureBuffers;
using Tyra::Texture;
using Tyra::TextureBuilderData;

PostFxManager::PostFxManager(Engine* t_engine)
    : Singleton<PostFxManager>(),
      settings(t_engine->renderer.core.getSettings()) {
  pEngine = t_engine;
  init();
};

PostFxManager::~PostFxManager(){};

void PostFxManager::render() {
  // Set GS settings
  qword_t packets[5] ALIGNED(64);
  qword_t* q = packets;

  q = draw_disable_tests(q, 0, &pEngine->renderer.core.gs.zBuffer);

  q = draw_pixel_alpha_control(q, DRAW_DISABLE);

  dma_channel_send_normal(DMA_CHANNEL_GIF, packets, q - packets, 0, 0);
  dma_wait_fast();

  // Apply the post effects
  renderFog();

  // Reset GS old settings
  q = packets;

  PACK_GIFTAG(q, GIF_SET_TAG(5, 1, 0, 0, GIF_FLG_PACKED, 1), GIF_REG_AD);
  q++;

  // Alpha Blending
  PACK_GIFTAG(q,
              GS_SET_ALPHA(BLEND_COLOR_SOURCE, BLEND_COLOR_DEST,
                           BLEND_ALPHA_SOURCE, BLEND_COLOR_DEST, 0x80),
              GS_REG_ALPHA_1);
  q++;

  PACK_GIFTAG(q, GS_SET_PABE(DRAW_ENABLE), GS_REG_PABE);
  q++;

  // Reenable ztest
  PACK_GIFTAG(
      q,
      GS_SET_TEST(DRAW_ENABLE, ATEST_METHOD_NOTEQUAL, 0x00,
                  ATEST_KEEP_FRAMEBUFFER, DRAW_DISABLE, DRAW_DISABLE,
                  DRAW_ENABLE, pEngine->renderer.core.gs.zBuffer.method),
      GS_REG_TEST);
  q++;

  // Setup whole texture clamping
  // texwrap_t wrap;
  // wrap.horizontal = WRAP_CLAMP;
  // wrap.vertical = WRAP_CLAMP;
  // wrap.minu = wrap.maxu = 0;
  // wrap.minv = wrap.maxv = 0;

  // Texture wrapping/clamping
  PACK_GIFTAG(q, GS_SET_CLAMP(WRAP_CLAMP, WRAP_CLAMP, 0, 0, 0, 0),
              GS_REG_CLAMP);
  q++;
  PACK_GIFTAG(q, GS_SET_TEXA(0x80, ALPHA_EXPAND_NORMAL, 0x80), GS_REG_TEXA);
  q++;

  dma_channel_send_normal(DMA_CHANNEL_GIF, packets, q - packets, 0, 0);
  dma_wait_fast();
}

void PostFxManager::init() {
  // Create temp depth buffer texture
  TextureBuilderData pDepthTempBuffer;
  pDepthTempBuffer.width = 64;
  pDepthTempBuffer.height = 32;
  pDepthTempBuffer.bpp = Tyra::TextureBpp::bpp32;
  pDepthTempBuffer.data = new u8[64 * 32 * 4];
  pDepthTempBuffer.gsComponents = TEXTURE_COMPONENTS_RGBA;

  pDepthBufferTexture = new Texture(&pDepthTempBuffer);
  pEngine->renderer.core.texture.repository.add(pDepthBufferTexture);
  pEngine->renderer.core.texture.useTexture(pDepthBufferTexture);

  // Create FOG color pallet
  TextureBuilderData pColorPaletteRaster;
  pColorPaletteRaster.width = 16;
  pColorPaletteRaster.height = 16;
  pColorPaletteRaster.bpp = Tyra::TextureBpp::bpp32;
  pColorPaletteRaster.gsComponents = TEXTURE_COMPONENTS_RGBA;
  pColorPaletteRaster.data = new u8[16 * 16 * 4];

  int length = 1024;
  float maxValue = 128;
  int minOffset = 0;
  int maxOffset = 32;
  bool asInteger = true;

  // Clamp offsets within bounds
  minOffset = std::max(0, minOffset);
  maxOffset = std::min(length - 1, maxOffset);

  // Calculate step size for linear increment
  float step = maxValue / (maxOffset - minOffset);

  // Fill linear values within the offset range
  for (int i = minOffset; i <= maxOffset; ++i) {
    float value = (i - minOffset) * step;
    pColorPaletteRaster.data[i] =
        asInteger ? std::round(value) : std::round(value * 10000.0) / 10000.0;
  }

  // // Fill exponential values within the offset range
  // for (int i = minOffset; i <= maxOffset; ++i) {
  //   float normalizedIndex =
  //       static_cast<float>(i - minOffset) / (maxOffset - minOffset);
  //   float value =
  //       maxValue * std::pow(normalizedIndex, 2);  // Exponential growth
  //   pColorPaletteRaster.data[i] =
  //       asInteger ? std::round(value) : std::round(value * 10000.0) /
  //       10000.0;
  // }

  // Set maxValue for indices after maxOffset
  for (int i = maxOffset + 1; i < length; ++i) {
    pColorPaletteRaster.data[i] =
        asInteger ? maxValue : std::round(maxValue * 10000.0) / 10000.0;
  }

  pFogTexture = new Texture(&pColorPaletteRaster);
  pEngine->renderer.core.texture.repository.add(pFogTexture);
  pEngine->renderer.core.texture.useTexture(pFogTexture);
};

void PostFxManager::copyDepthBuffer(ColourChannels channelIn,
                                    Texture* palette) {
  uint32_t width = settings.getWidth(), height = settings.getHeight();
  uint32_t zbufferAddr = pEngine->renderer.core.gs.zBuffer.address;
  RendererCoreTextureBuffers texBuffer =
      pEngine->renderer.core.texture.useTexture(palette);

  uint32_t pal_addr = texBuffer.core->address;

  uint32_t page = 0;
  uint32_t x, y;

  for (y = 0; y < height; y += 32) {
    for (x = 0; x < width; x += 64) {
      uint32_t buf_addr =
          pEngine->renderer.core.texture.useTexture(pDepthBufferTexture)
              .core->address;

      qword_t packets[5] ALIGNED(64);
      qword_t* q = packets;

      PACK_GIFTAG(q, GIF_SET_TAG(4, 1, 0, 0, GIF_FLG_PACKED, 1), GIF_REG_AD);
      q++;

      PACK_GIFTAG(q,
                  GS_SET_BITBLTBUF((zbufferAddr >> 6) + page, 1, GS_PSMZ_32,
                                   buf_addr >> 6, 1, GS_PSM_32),
                  GS_REG_BITBLTBUF);
      q++;

      PACK_GIFTAG(q, GS_SET_TRXPOS(0, 0, 0, 0, 0),
                  GS_REG_TRXPOS);  // ...then set the offset in the buffer, and
                                   // pixel transmission order...
      q++;

      PACK_GIFTAG(q, (uint64_t)(64) | ((uint64_t)(32) << 32),
                  GS_REG_TRXREG);  // ...the width and height of the
                                   // transmission for the dest buffer...
      q++;

      PACK_GIFTAG(q, 2L, GS_REG_TRXDIR);  // ...and finally the direction of
                                          // transmission, local-to-local...
      q++;

      pal_addr = pEngine->renderer.core.texture.useTexture(palette)
                     .core->address;

      dma_channel_send_normal(DMA_CHANNEL_GIF, packets, q - packets, 0, 0);
      dma_channel_fast_waits(DMA_CHANNEL_GIF);
      dma_wait_fast();

      performChannelCopy(channelIn, CHANNEL_ALPHA, x, y, buf_addr, width,
                         height, pal_addr);

      page += 32;
    }
  }
};

void PostFxManager::performChannelCopy(ColourChannels channelIn,
                                       ColourChannels channelOut,
                                       uint32_t blockX, uint32_t blockY,
                                       uint32_t source_addr, uint32_t width,
                                       uint32_t height, uint32_t pal_addr) {
  const u8 ctx = pEngine->renderer.core.gs.getDrawContext();
  const framebuffer_t buf_frame =
      pEngine->renderer.core.gs.getCurrentFrameData();

  uint64_t frameAddress = buf_frame.address >> 6;

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

  PACK_GIFTAG(q, GS_SET_XYOFFSET(0, 0), GS_REG_XYOFFSET_1);
  q++;

  int tw, th;
  setTwTh(width, height, &tw, &th);
  PACK_GIFTAG(q,
              GS_SET_TEX0(source_addr >> 6, 1, GS_PSM_8, tw, th, 1, 1,
                          pal_addr >> 6, GS_PSM_32, 0, 0, 1),
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
              GS_SET_FRAME(frameAddress, buf_frame.width >> 6, buf_frame.psm,
                           frame_mask),
              GS_REG_FRAME_1 + ctx);
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
  dma_wait_fast();

  q = packets;

  PACK_GIFTAG(q, GIF_SET_TAG(3, 1, 0, 0, GIF_FLG_PACKED, 1), GIF_REG_AD);
  q++;

  PACK_GIFTAG(q, GS_SET_CLAMP(WRAP_CLAMP, WRAP_CLAMP, 0, 0, 0, 0),
              GS_REG_CLAMP_1);
  q++;

  PACK_GIFTAG(q,
              GS_SET_FRAME(frameAddress, buf_frame.width >> 6, buf_frame.psm,
                           buf_frame.mask),
              GS_REG_FRAME_1 + ctx);
  q++;

  PACK_GIFTAG(q,
              GS_SET_XYOFFSET(
                  (int)(screenCenter - (settings.getWidth() / 2.0F) * 16.0f),
                  (int)(screenCenter - (settings.getHeight() / 2.0F) * 16.0f)),
              GS_REG_XYOFFSET_1);
  q++;

  dma_channel_send_normal(DMA_CHANNEL_GIF, packets, q - packets, 0, 0);
  dma_wait_fast();
};

void PostFxManager::setTwTh(int w, int h, int* tw, int* th) {
  *tw = 31 - (lzw(w) + 1);
  if (w > (1 << *tw)) (*tw)++;

  *th = 31 - (lzw(h) + 1);
  if (h > (1 << *th)) (*th)++;
}

void PostFxManager::renderFog() {
  Color tlc = Color(200.0f, 100.0f, 100.0f, 80);
  Color trc = Color(200.0f, 100.0f, 100.0f, 80);
  Color blc = Color(200.0f, 100.0f, 100.0f, 80);
  Color brc = Color(200.0f, 100.0f, 100.0f, 80);

  uint64_t width = settings.getWidth(), height = settings.getHeight();

  copyDepthBuffer(CHANNEL_GREEN, pFogTexture);

  qword_t packets[17] ALIGNED(64);
  qword_t* q = packets;

  PACK_GIFTAG(q, GIF_SET_TAG(3, 1, 0, 0, GIF_FLG_PACKED, 1), GIF_REG_AD);
  q++;

  PACK_GIFTAG(q, GS_SET_TEST(0, 0, 0, 0, 0, 0, 1, 1), GS_REG_TEST_1);
  q++;

  // The GS's alpha blending formula is fixed but it contains four variables
  // that can be reconfigured: Output = (((A - B) * C) >> 7) + D A, B, and D are
  // colors and C is an alpha value. Their specific values come from the ALPHA
  // register:
  //       A                B                C                   D
  //   0   Source RGB       Source RGB       Source alpha        Source RGB
  //   1   Framebuffer RGB  Framebuffer RGB  Framebuffer alpha   Framebuffer RGB
  //   2   0                0                FIX                 0
  //   3   Reserved         Reserved         Reserved            Reserved
  PACK_GIFTAG(q,
              GS_SET_ALPHA(BLEND_COLOR_DEST, BLEND_COLOR_SOURCE,
                           BLEND_COLOR_DEST, BLEND_COLOR_SOURCE, 0x00),
              GS_REG_ALPHA_1);
  q++;

  PACK_GIFTAG(q, GS_SET_XYOFFSET(0, 0), GS_REG_XYOFFSET_1);
  q++;

  PACK_GIFTAG(
      q,
      GIF_SET_TAG(4, 1, 1,
                  GS_SET_PRIM(GS_PRIM_TRIANGLE_STRIP, 1, 0, 0, 1, 0, 0, 0, 0),
                  GIF_FLG_PACKED, 2),
      (GS_REG_RGBAQ) | (GS_REG_XYZ2 << 4));
  q++;

  // LOOP 1
  {
    // TLC
    // RGBAQ
    PACK_GIFTAG(q, (uint64_t)(tlc.r) | (uint64_t)(tlc.g) << 32,
                (uint64_t)(tlc.b) | (uint64_t)(tlc.a) << 32);
    q++;

    // XYZ2
    PACK_GIFTAG(q, (uint64_t)(0 << 4) | (uint64_t)(0 << 4) << 32,
                (uint64_t)(1));
    q++;

    // TRC
    // RGBAQ
    PACK_GIFTAG(q, (uint64_t)(trc.r) | (uint64_t)(trc.g) << 32,
                (uint64_t)(trc.b) | (uint64_t)(trc.a) << 32);
    q++;

    // XYZ2
    PACK_GIFTAG(q, (uint64_t)((((width << 4)) | (((uint64_t)(0 << 4)) << 32))),
                (uint64_t)(1));
    q++;
  }

  // LOOP 2
  {
    // BLC
    // RGBAQ
    PACK_GIFTAG(q, (uint64_t)(blc.r) | (uint64_t)(blc.g) << 32,
                (uint64_t)(blc.b) | (uint64_t)(blc.a) << 32);
    q++;

    // XYZ2
    PACK_GIFTAG(q, (uint64_t)((((0 << 4)) | ((uint64_t)(height << 4) << 32))),
                (uint64_t)(1));
    q++;

    // BRC
    // RGBAQ
    PACK_GIFTAG(q, (uint64_t)(brc.r) | (uint64_t)(brc.g) << 32,
                (uint64_t)(brc.b) | (uint64_t)(brc.a) << 32);
    q++;

    // XYZ2
    PACK_GIFTAG(q,
                (uint64_t)((((width << 4)) | ((uint64_t)(height << 4) << 32))),
                (uint64_t)(1));
    q++;
  }

  PACK_GIFTAG(q, GIF_SET_TAG(1, 1, 0, 0, GIF_FLG_PACKED, 1), GIF_REG_AD);
  q++;

  PACK_GIFTAG(q,
              GS_SET_XYOFFSET(
                  (int)(screenCenter - (settings.getWidth() / 2.0F) * 16.0f),
                  (int)(screenCenter - (settings.getHeight() / 2.0F) * 16.0f)),
              GS_REG_XYOFFSET_1);
  q++;

  dma_channel_send_normal(DMA_CHANNEL_GIF, packets, q - packets, 0, 0);
  dma_wait_fast();
};
