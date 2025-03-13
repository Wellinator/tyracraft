#include "managers/post-fx/post_fx_manager.hpp"
#include <gs_gp.h>
#include <gs_psm.h>
#include <dma_tags.h>

using Tyra::Color;
using Tyra::RendererCoreTextureBuffers;
using Tyra::Texture;
using Tyra::TextureBuilderData;

PostFxManager::PostFxManager(Renderer* renderer)
    : Singleton<PostFxManager>(), settings(renderer->core.getSettings()) {
  pRenderer = renderer;
  init();
};

PostFxManager::~PostFxManager(){};

void PostFxManager::render(Color fogColor) {
  // Set GS settings
  qword_t packets[20] ALIGNED(64);
  qword_t* q = packets;

  q = draw_disable_tests(q, 0, &pRenderer->core.gs.zBuffer);

  dma_channel_send_normal(DMA_CHANNEL_GIF, packets, q - packets, 0, 0);
  dma_wait_fast();

  // Apply the post effects
  renderFog(fogColor);

  // Reset GS old settings
  q = packets;

  PACK_GIFTAG(q, GIF_SET_TAG(2, 1, 0, 0, GIF_FLG_PACKED, 1), GIF_REG_AD);
  q++;

  // Alpha Blending
  PACK_GIFTAG(q,
              GS_SET_ALPHA(BLEND_COLOR_SOURCE, BLEND_COLOR_DEST,
                           BLEND_ALPHA_SOURCE, BLEND_COLOR_DEST, 0x80),
              GS_REG_ALPHA_1);
  q++;

  PACK_GIFTAG(q, GS_SET_CLAMP(WRAP_CLAMP, WRAP_CLAMP, 0, 0, 0, 0),
              GS_REG_CLAMP_1);
  q++;

  q = draw_enable_tests(q, 0, &pRenderer->core.gs.zBuffer);

  q = draw_texture_expand_alpha(q, 0x80, ALPHA_EXPAND_NORMAL, 0x80);

  dma_channel_send_normal(DMA_CHANNEL_GIF, packets, q - packets, 0, 0);
  dma_wait_fast();
}

uint32_t getPatternValue(uint32_t N) {
  // uint32_t mod = N % 3;
  // if (mod == 1) {
  //   return N + 1;
  // } else if (mod == 2) {
  //   return N - 1;
  // } else {
  //   return N;
  // }
  const int r = N % 3;
  return N + (r == 1 ? 1 : (r == 2 ? -1 : 0));
}

double easeInOut(double x, double maxInput, double offset = 0.1,
                 double steepness = 15.0) {
  // Optional: clamp x to the range [0, maxInput]
  x = std::max(std::min(x, maxInput), 0.0);

  // Normalize input to [0,1]
  double t = x / maxInput;

  // Compute the logistic (sigmoid) function value
  double logistic = 1.0 / (1.0 + std::exp(-steepness * (t - offset)));

  // Determine the logistic values at the endpoints for normalization:
  // at t = 0:
  double logistic0 = 1.0 / (1.0 + std::exp(steepness * offset));
  // at t = 1:
  double logistic1 = 1.0 / (1.0 + std::exp(-steepness * (1.0 - offset)));

  // Normalize so that logistic0 maps to 0 and logistic1 maps to 1
  double normalized = (logistic - logistic0) / (logistic1 - logistic0);

  return normalized * 128.0;
}

void PostFxManager::init() {
  // Create temp depth buffer texture
  TextureBuilderData pDepthTempBuffer;
  pDepthTempBuffer.width = 64;
  pDepthTempBuffer.height = 32;
  pDepthTempBuffer.bpp = Tyra::TextureBpp::bpp32;
  pDepthTempBuffer.data = new u8[64 * 32 * 4]{0};
  pDepthTempBuffer.gsComponents = TEXTURE_COMPONENTS_RGBA;

  pDepthBufferTexture = new Texture(&pDepthTempBuffer);
  pRenderer->core.texture.repository.add(pDepthBufferTexture);
  pRenderer->core.texture.useTexture(pDepthBufferTexture);

  // Create FOG color pallet
  TextureBuilderData pColorPaletteRaster;
  pColorPaletteRaster.width = 16;
  pColorPaletteRaster.height = 16;
  pColorPaletteRaster.bpp = Tyra::TextureBpp::bpp32;
  pColorPaletteRaster.gsComponents = TEXTURE_COMPONENTS_RGBA;
  pColorPaletteRaster.data = new u8[16 * 16 * 4]{0};

  for (int i = 0; i < 1024; i++) {
    pColorPaletteRaster.data[i] = 128;
  }

  // u8* pallet = pColorPaletteRaster.data;
  // for (int i = 0; i < 256; i++) {
  //   const int targetIndex = getPatternValue(i) * 4;
  //   const u8 value = std::min(128, int((i * 3) + 1));

  //   pallet[targetIndex + 0] = value;
  //   pallet[targetIndex + 1] = value;
  //   pallet[targetIndex + 2] = value;
  //   pallet[targetIndex + 3] = value;

  //   printf("[%lu] = %i\n", getPatternValue(i), value);
  // }

  // const u8 blockLength = 8;
  // const uint32_t maxValue = 128;
  // uint32_t* pallet = reinterpret_cast<uint32_t*>(pColorPaletteRaster.data);

  // for (uint32_t i = 0; i <= 5; i++) {
  //   const uint32_t offset = getPatternValue(i) * blockLength;
  //   TYRA_LOG("offset: ", offset);

  //   for (u8 j = 0; j < 8; j++) {
  //     const uint32_t index = offset + j;

  //     TYRA_LOG("index: ", index);

  //     // const uint32_t value = std::round(easeInOut(i, 255, 0.08f, 40));
  //     // const uint32_t value = std::floor(std::exp2(std::pow(i, 0.5f)
  //     * 1.5f));
  //     // const uint32_t value = std::floor(std::exp2(index * 0.35f));

  //     const uint32_t value = index * 2;

  //     if (index < 256) pallet[index] = std::min(value, maxValue) << 24;
  //   }
  // }

  pFogTexture = new Texture(&pColorPaletteRaster);
  pRenderer->core.texture.repository.add(pFogTexture);
  pRenderer->core.texture.useTexture(pFogTexture);

  uint8_t fog_scale[18] = {0, 1, 2, 3, 10, 7, 3, 2, 1,
                           1, 0, 0, 0, 0,  0, 0, 0, 0};
  scaleDepthMask(pFogTexture, 1, fog_scale);
};

void PostFxManager::updateDebugPallet() {
  uint32_t* pallet = reinterpret_cast<uint32_t*>(pFogTexture->core->data);
  uint32_t i = 1;

  pallet[0] = (i) << 24;
  pallet[1] = (i += 4) << 24;
  pallet[2] = (i += 4) << 24;
  pallet[3] = (i += 4) << 24;
  pallet[4] = (i += 4) << 24;
  pallet[5] = (i += 4) << 24;
  pallet[6] = (i += 4) << 24;
  pallet[7] = (i += 4) << 24;

  pallet[16] = (i += 3) << 24;
  pallet[17] = (i += 3) << 24;
  pallet[18] = (i += 3) << 24;
  pallet[19] = (i += 3) << 24;
  pallet[20] = (i += 3) << 24;
  pallet[21] = (i += 3) << 24;
  pallet[22] = (i += 3) << 24;
  pallet[23] = (i += 3) << 24;

  pallet[8] = (i += 2) << 24;
  pallet[9] = (i += 2) << 24;
  pallet[10] = (i += 2) << 24;
  pallet[11] = (i += 2) << 24;
  pallet[12] = (i += 2) << 24;
  pallet[13] = (i += 2) << 24;
  pallet[14] = (i += 2) << 24;
  pallet[15] = (i += 2) << 24;

  pallet[24] = (i += 1) << 24;
  pallet[25] = (i += 1) << 24;
  pallet[26] = (i += 1) << 24;
  pallet[27] = (i += 1) << 24;
  pallet[28] = (i += 1) << 24;
  pallet[29] = (i += 1) << 24;
  pallet[30] = (i += 1) << 24;
  pallet[31] = (i += 1) << 24;

  printf("debugPalletIndex: %i, old value: %li\n", debugPalletIndex,
         pallet[debugPalletIndex] >> 24);
  pallet[debugPalletIndex] = 1 << 24;

  pRenderer->core.texture.updateTextureInfo(pFogTexture);

  debugPalletIndex++;
  debugPalletIndex %= 24;
}

void PostFxManager::copyDepthBuffer(ColourChannels channelIn,
                                    Texture* palette) {
  uint32_t width = settings.getWidth(), height = settings.getHeight();
  uint32_t zbufferAddr = pRenderer->core.gs.zBuffer.address;
  RendererCoreTextureBuffers texBuffer =
      pRenderer->core.texture.useTexture(palette);

  uint32_t pal_addr = texBuffer.core->address;

  uint32_t page = 0;
  uint32_t x, y;

  for (y = 0; y < height; y += 32) {
    for (x = 0; x < width; x += 64) {
      uint32_t buf_addr =
          pRenderer->core.texture.useTexture(pDepthBufferTexture).core->address;

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

      pal_addr = pRenderer->core.texture.useTexture(palette).core->address;

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

  PACK_GIFTAG(q, GIF_SET_TAG(5, 1, 0, 0, GIF_FLG_PACKED, 1), GIF_REG_AD);
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
  dma_wait_fast();
  q = packets;

  PACK_GIFTAG(q, GIF_SET_TAG(3, 1, 0, 0, GIF_FLG_PACKED, 1), GIF_REG_AD);
  q++;

  PACK_GIFTAG(q, GS_SET_CLAMP(WRAP_CLAMP, WRAP_CLAMP, 0, 0, 0, 0),
              GS_REG_CLAMP_1);
  q++;

  PACK_GIFTAG(q,
              GS_SET_FRAME(buf_frame.address >> 11, buf_frame.width >> 6,
                           buf_frame.psm, buf_frame.mask),
              GS_REG_FRAME_1);
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

void PostFxManager::scaleDepthMask(Texture* palette, uint8_t initial_value,
                                   uint8_t factors[16]) {
  int i, j, k = initial_value;

  static const uint8_t factor_order[18][2] = {
      {0, 3},   {4, 7},   {16, 19},   {20, 23},   {8, 15},    {24, 31},
      {40, 47}, {32, 39}, {48, 55},   {34, 71},   {56, 63},   {72, 79},
      {88, 95}, {80, 87}, {112, 119}, {104, 111}, {120, 127}, {136, 143}};

  uint32_t* pal_rgba = reinterpret_cast<uint32_t*>(palette->core->data);

  printf("pallet: \n");

  for (j = 0; j < 16; j++) {
    // k = initial_value;

    for (i = factor_order[j][0]; i <= factor_order[j][1]; i++) {
      pal_rgba[i] = (k << 24) | (k << 16) | (k << 8) | k;  // RGBA

      printf("%i,", k);

      if (k < 128 || factors[j] >= 128)
      // if (k <= 128)
      {
        k += factors[j];
      } else if (k != 128) {
        k = 128;
      }
    }
  }

  pRenderer->core.texture.updateTextureInfo(palette);
}

void PostFxManager::renderFog(Color fogColor) {
  Color tlc = fogColor;
  Color trc = fogColor;
  Color blc = fogColor;
  Color brc = fogColor;

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
    PACK_GIFTAG(q, GIF_SET_XYZ(0, 0, 0), 1);
    q++;

    // TRC
    // RGBAQ
    PACK_GIFTAG(q, (uint64_t)(trc.r) | (uint64_t)(trc.g) << 32,
                (uint64_t)(trc.b) | (uint64_t)(trc.a) << 32);
    q++;

    // XYZ2
    PACK_GIFTAG(q, GIF_SET_XYZ(ftoi4(width), 0, 0), 1);
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
    PACK_GIFTAG(q, GIF_SET_XYZ(0, 0, ftoi4(height)), 1);
    q++;

    // BRC
    // RGBAQ
    PACK_GIFTAG(q, (uint64_t)(brc.r) | (uint64_t)(brc.g) << 32,
                (uint64_t)(brc.b) | (uint64_t)(brc.a) << 32);
    q++;

    // XYZ2
    PACK_GIFTAG(q, GIF_SET_XYZ(ftoi4(width), 0, ftoi4(height)), 1);
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
