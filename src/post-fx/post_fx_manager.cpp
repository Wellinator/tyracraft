#include "managers/post-fx/post_fx_manager.hpp"
#include "managers/dma_gif_builder.hpp"
#include "screen_settings.hpp"
#include "debug.hpp"
#include <gs_gp.h>
#include <gs_psm.h>
#include <dma_tags.h>
#include <screenshot.h>

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

PostFxManager::PostFxManager(Renderer* renderer)
    : Singleton<PostFxManager>(), settings(renderer->core.getSettings()) {
  pRenderer = renderer;

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

  // Fazer upload do CLUT de identidade para channel shuffle
  uploadIdentityCLUT();

  // Inicializar valores de CLIP_ZVALUE
  // IMPORTANTE: Para 512x448 (vs 640x448 original), ajustar fogStartPercent
  // fogStartPercent=0.65 para 512x448 (vs 0.5 no original 640x448)
  // Isso compensa a diferença de aspect ratio e densidade de pixels
  DEFAULT_CLIP_ZVALUE =
      calculateClipZValue(SCREEN_NEAR_PLANE, SCREEN_FAR_PLANE, 0.65f);
  CLIP_ZVALUE = DEFAULT_CLIP_ZVALUE;

#ifdef DEBUG_MODE
  TYRA_LOG("[FOG INIT] DEFAULT_CLIP_ZVALUE = 0x", std::hex, DEFAULT_CLIP_ZVALUE,
           std::dec);
  TYRA_LOG("[FOG INIT] Screen resolution: ", (int)SCREEN_WIDTH, "x",
           (int)SCREEN_HEIGHT);
#endif
};

PostFxManager::~PostFxManager() {};

void PostFxManager::copyDepthBuffer(ColourChannels channelIn,
                                    Texture* palette) {
  uint32_t width = SCREEN_WIDTH, height = SCREEN_HEIGHT;
  uint32_t zbufferAddr = pRenderer->core.gs.zBuffer.address;
  RendererCoreTextureBuffers texBuffer =
      pRenderer->core.texture.useTexture(palette);

  uint32_t pal_addr = texBuffer.core->address;

  uint32_t page = 0;
  uint32_t x, y;

  const uint32_t src_tbw = width >> 6;  // TBW for full-screen zbuffer
  const uint32_t dst_tbw = 1;           // Temp buffer is 64px wide

  for (y = 0; y < height; y += 32) {
    for (x = 0; x < width; x += 64) {
      uint32_t buf_addr =
          pRenderer->core.texture.useTexture(pDepthBufferTexture).core->address;

      qword_t packets[5] ALIGNED(64);
      qword_t* q = packets;

      PACK_GIFTAG(q, GIF_SET_TAG(4, 1, 0, 0, GIF_FLG_PACKED, 1), GIF_REG_AD);
      q++;

      PACK_GIFTAG(
          q,
          GS_SET_BITBLTBUF((zbufferAddr >> 6) + page, src_tbw, GS_PSMZ_32,
                           buf_addr >> 6, dst_tbw, GS_PSM_32),
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
      dma_channel_wait(DMA_CHANNEL_GIF, 500);

      performChannelCopy(channelIn, CHANNEL_ALPHA, x, y, buf_addr);

      page += 32;
    }
  }
};

// Channel copy a block
// Baseado na implementação original de channel_copy.cpp
// Usa textura 8-bit com CLUT de identidade para fazer channel shuffle
void PostFxManager::performChannelCopy(ColourChannels channelIn,
                                       ColourChannels channelOut,
                                       uint32_t blockX, uint32_t blockY,
                                       uint32_t source_addr) {
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

  // TEX0: Configuração da textura 8-bit
  // Para Z-buffer em PSMZ32, usar TBW baseado na largura da tela
  // TBW deve corresponder ao layout do Z-buffer: width >> 6
  // source_addr já aponta para o Z-buffer completo (não para blocos
  // individuais)
  PACK_GIFTAG(q,
              GS_SET_TEX0(source_addr >> 6, fog_width >> 6, GS_PSM_8, 10, 10, 1,
                          1, clutVramAddress >> 6, GS_PSM_32, 0, 0, 1),
              GS_REG_TEX0_1);
  q++;

  // CLAMP: Region repeat com offsets para selecionar o canal correto
  PACK_GIFTAG(q,
              GS_SET_CLAMP(WRAP_REGION_REPEAT, WRAP_REGION_REPEAT, 0xF7,
                           clamp_horz, 0xFD, clamp_vert),
              GS_REG_CLAMP_1);
  q++;

  PACK_GIFTAG(q, GS_SET_TEXFLUSH(1), GS_REG_TEXFLUSH);
  q++;

  // PRIM: Sprite com textura habilitada, alpha blending habilitado
  PACK_GIFTAG(q, GS_SET_PRIM(GS_PRIM_SPRITE, 0, 1, 0, 1, 0, 1, 0, 0),
              GS_REG_PRIM);
  q++;

  // GIF tag para 96 primitivas (sprites) - UV/XYZ2 pairs
  PACK_GIFTAG(q, GIF_SET_TAG(96, 1, 0, 0, GIF_FLG_PACKED, 4),
              (GIF_REG_UV) | (GIF_REG_XYZ2 << 4) | (GIF_REG_UV << 8) |
                  (GIF_REG_XYZ2 << 12));
  q++;

  // Desenhar blocos 64x32 com padrões alternados para channel shuffle
  int y;
  for (y = 0; y < 32; y += 2) {
    if (((y % 4) == 0) ^ (vert_block_offset == 1))  // Even (4 16x2 sprites)
    {
      int x;
      for (x = 0; x < 64; x += 16) {
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
    } else  // Odd (Eight 8x2 sprites)
    {
      int x;
      for (x = 0; x < 64; x += 8) {
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

  // Restaurar CLAMP para modo normal
  q = packets;
  PACK_GIFTAG(q, GIF_SET_TAG(2, 1, 0, 0, GIF_FLG_PACKED, 1), GIF_REG_AD);
  q++;

  PACK_GIFTAG(q, GS_SET_CLAMP(WRAP_CLAMP, WRAP_CLAMP, 0, 0, 0, 0),
              GS_REG_CLAMP_1);
  q++;

  PACK_GIFTAG(
      q,
      GS_SET_XYOFFSET((int)(screenCenter - (SCREEN_WIDTH / 2.0F) * 16.0f),
                      (int)(screenCenter - (SCREEN_HEIGHT / 2.0F) * 16.0f)),
      GS_REG_XYOFFSET_1);
  q++;

  dma_channel_send_normal(DMA_CHANNEL_GIF, packets, q - packets, 0, 0);
  dma_channel_wait(DMA_CHANNEL_GIF, 500);
};

/**
 * Calcula o CLIP_ZVALUE baseado nos planos near/far e porcentagem de início do
 * fog.
 *
 * O Z-Buffer do PS2 usa mapeamento hiperbólico (não linear).
 * Após o PASS 1, os valores são invertidos: Z_invertido = 0xFFFFFF - Z_original
 *
 * Lógica do CLIP_ZVALUE com ZTEST_GREATER:
 * - Pass 1 inverte Z-Buffer: objetos próximos ficam com valores BAIXOS
 * - Pass 2 usa ZTEST_GREATER: se CLIP_ZVALUE > Z_Buffer, sobrescreve
 * - Para clipar apenas objetos PRÓXIMOS (próximo ao fogDistance):
 *   * CLIP_ZVALUE deve ser um valor INTERMEDIÁRIO
 *   * Pixels com Z < CLIP_ZVALUE (objetos próximos) são clippados
 *   * Pixels com Z > CLIP_ZVALUE (objetos distantes) mantêm profundidade
 *
 * Comportamento do fogStartPercent:
 * - 0.0 = fog começa no near (clippa quase nada, fog em tudo)
 * - 0.5 = fog começa no meio (clippa metade próxima)
 * - 1.0 = fog começa no far (clippa quase tudo, fog só no horizonte)
 *
 * @param nearPlane Distância do plano near (ex: 0.01f)
 * @param farPlane Distância do plano far (ex: 1000.0f)
 * @param fogStartPercent Porcentagem onde fog começa (0.0 = near, 1.0 = far)
 * @return Valor CLIP_ZVALUE para uso no Passo 2 com ZTEST_GREATER
 */
uint32_t PostFxManager::calculateClipZValue(float nearPlane, float farPlane,
                                            float fogStartPercent) {
  // Clampar porcentagem entre 0 e 1
  if (fogStartPercent < 0.0f) fogStartPercent = 0.0f;
  if (fogStartPercent > 1.0f) fogStartPercent = 1.0f;

  // fogStartPercent define onde o fog COMEÇA (objetos mais próximos ficam
  // nítidos) fogStartPercent=0.0 -> fog começa no near (tudo tem fog, nada é
  // clippado) fogStartPercent=0.5 -> fog começa no meio (metade próxima é
  // clippada/nítida) fogStartPercent=1.0 -> fog começa no far (quase tudo é
  // clippado/nítido)
  float fogDistance = nearPlane + (farPlane - nearPlane) * fogStartPercent;

  // Evitar divisão por zero
  if (fogDistance <= nearPlane) fogDistance = nearPlane + 0.001f;
  if (fogDistance >= farPlane) fogDistance = farPlane - 0.001f;

  // Calcular Z original usando a fórmula do Z-Buffer perspectivo do PS2
  // Z-Buffer do PS2: objetos PRÓXIMOS têm valores ALTOS, objetos DISTANTES têm
  // valores BAIXOS
  const float maxZ = 16777215.0f;  // 2^24 - 1 = 0xFFFFFF

  // Fórmula do Z-Buffer perspectivo:
  // Z_normalized = (far / distance - 1) / (far / near - 1)
  float zNormalized =
      (farPlane / fogDistance - 1.0f) / (farPlane / nearPlane - 1.0f);
  uint32_t zOriginal = (uint32_t)(zNormalized * maxZ);

  // APÓS PASS 1 (inversão): Z_invertido = 0xFFFFFF - Z_original
  // Resultado: objetos PRÓXIMOS têm valores BAIXOS, objetos DISTANTES têm
  // valores ALTOS
  uint32_t zInvertido = 0xFFFFFF - zOriginal;

  // Para ZTEST_GREATER no Pass 2:
  // - Sobrescreve se CLIP_ZVALUE > Z_Buffer
  // - Queremos clippar objetos MAIS PRÓXIMOS que fogDistance
  // - Objetos próximos têm Z BAIXO após inversão
  // - Logo, CLIP_ZVALUE deve ser o threshold: valores MENORES serão clippados
  //
  // CLIP_ZVALUE = zInvertido (threshold direto)
  // Pixels com Z < zInvertido (mais próximos) → CLIP_ZVALUE > Z → SOBRESCREVE ✓
  // Pixels com Z > zInvertido (mais distantes) → CLIP_ZVALUE < Z → MANTÉM ✓

  // Retornar apenas os 24 bits úteis
  return zInvertido & 0x00FFFFFF;
}

void PostFxManager::setTwTh(int w, int h, int* tw, int* th) {
  *tw = 31 - (lzw(w) + 1);
  if (w > (1 << *tw)) (*tw)++;

  *th = 31 - (lzw(h) + 1);
  if (h > (1 << *th)) (*th)++;
}

// Upload CLUT de identidade para VRAM usando o sistema de texturas da Tyra
// Engine. A paleta de identidade é uma textura 16x16 (256 pixels) em formato
// 32-bit, onde cada pixel N tem a cor (N, N, N, N) em RGBA. Isso permite que a
// textura 8-bit funcione como lookup direto de bytes.
void PostFxManager::uploadIdentityCLUT() {
  // Dimensões da paleta: 16x16 = 256 cores
  constexpr int CLUT_WIDTH = 16;
  constexpr int CLUT_HEIGHT = 16;
  constexpr int CLUT_SIZE = CLUT_WIDTH * CLUT_HEIGHT;

  // Criar dados da paleta de identidade
  // Formato PS2 RGBA: R=bits 0-7, G=bits 8-15, B=bits 16-23, A=bits 24-31
  unsigned char* clutData =
      new unsigned char[CLUT_SIZE * 4];  // 4 bytes por pixel (RGBA)

  for (int i = 0; i < CLUT_SIZE; i++) {
    unsigned char value = static_cast<unsigned char>(i);
    clutData[i * 4 + 0] = value;  // R
    clutData[i * 4 + 1] = value;  // G
    clutData[i * 4 + 2] = value;  // B
    clutData[i * 4 + 3] = value;  // A
  }

  // Criar TextureBuilderData para a Tyra Engine
  TextureBuilderData* builderData = new TextureBuilderData();
  builderData->name = "identity_clut";
  builderData->width = CLUT_WIDTH;
  builderData->height = CLUT_HEIGHT;
  builderData->data = clutData;  // A Tyra assume ownership dos dados
  builderData->bpp = Tyra::bpp32;
  builderData->gsComponents = TEXTURE_COMPONENTS_RGBA;

  // Sem CLUT própria (esta textura É a CLUT)
  builderData->clut = nullptr;
  builderData->clutWidth = 0;
  builderData->clutHeight = 0;

  // Criar textura usando o sistema da Tyra
  identityCLUT = new Texture(builderData);
  delete builderData;  // TextureBuilderData pode ser deletado após criar
                       // Texture

  // Registrar no repositório de texturas
  auto& textureRepo = pRenderer->getTextureRepository();
  textureRepo.add(identityCLUT);

  // Alocar e fazer upload para VRAM usando o sistema da Tyra
  auto& textureSender = pRenderer->core.texture;
  auto buffers = textureSender.useTexture(identityCLUT);

  // Guardar o endereço da CLUT na VRAM para uso no performChannelCopy
  if (buffers.core != nullptr) {
    clutVramAddress = buffers.core->address;
  }

#ifdef DEBUG_MODE
  TYRA_LOG("[FOG] Identity CLUT created and uploaded to VRAM");
  TYRA_LOG("[FOG] CLUT VRAM address: 0x", std::hex, clutVramAddress, std::dec);
  TYRA_LOG("[FOG] CLUT texture ID: ", identityCLUT->id);
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

void PostFxManager::setClipZValue(uint32_t value) {
  // Clampar valor entre 0x000000 e 0xFFFFFF
  CLIP_ZVALUE = value & 0x00FFFFFF;
}

void PostFxManager::adjustClipZValue(int delta) {
  int64_t newValue = static_cast<int64_t>(CLIP_ZVALUE) + delta;

  // Clampar entre 0 e 0xFFFFFF
  if (newValue < 0) {
    newValue = 0;
  } else if (newValue > 0xFFFFFF) {
    newValue = 0xFFFFFF;
  }

  CLIP_ZVALUE = static_cast<uint32_t>(newValue);
}

void PostFxManager::resetClipZValueToDefault() {
  CLIP_ZVALUE = DEFAULT_CLIP_ZVALUE;
}

void PostFxManager::fogPassSetup() {
  // Setup inicial
#ifdef DEBUG_MODE
  if (g_debug_menu.fogPassSetup) {
#endif

    qword_t packets[200] ALIGNED(64);
    qword_t* q = packets;
    PACK_GIFTAG(q, GIF_SET_TAG(6, 1, 0, 0, GIF_FLG_PACKED, 1), GIF_REG_AD);
    q++;

    PACK_GIFTAG(q, GS_SET_TEST(0, 0, 0, 0, 0, 0, 1, ZTEST_METHOD_ALLPASS),
                GS_REG_TEST_1);
    q++;
    // Allow Z writes for the upcoming passes (Pass 1 needs to modify zbuffer)
    PACK_GIFTAG(q, GS_SET_ZBUF(fog_zbufferAddr >> 11, fog_zbufferPsm, 1),
                GS_REG_ZBUF_1);
    q++;
    PACK_GIFTAG(q, GS_SET_XYOFFSET(0, 0), GS_REG_XYOFFSET_1);
    q++;
    PACK_GIFTAG(q, GS_SET_PABE(0), GS_REG_PABE);
    q++;
    PACK_GIFTAG(q, GS_SET_SCISSOR(0, fog_width - 1, 0, fog_height - 1),
                GS_REG_SCISSOR_1);
    q++;
    PACK_GIFTAG(q, GS_SET_COLCLAMP(1), GS_REG_COLCLAMP);
    q++;

    FlushCache(0);
    dma_channel_send_normal(DMA_CHANNEL_GIF, packets, q - packets, 0, 0);
    dma_channel_wait(DMA_CHANNEL_GIF, 500);

#ifdef DEBUG_MODE
    saveDebugScreenshot("host:debug/fog_pass_setup.tga", fog_buf_frame.address,
                        fog_width, fog_height, fog_buf_frame.psm);
  }
#endif
}

void PostFxManager::fogPass1InvertZ() {
  // 1st pass: invert z-buffer values
#ifdef DEBUG_MODE
  if (g_debug_menu.fogPass1) {
    saveDebugScreenshot("host:debug/fog_pass1_before_invert_z.tga",
                        fog_zbufferAddr, fog_width, fog_height, fog_zbufferPsm);
#endif
    qword_t packets[200] ALIGNED(64);
    qword_t* q = packets;
    PACK_GIFTAG(q, GIF_SET_TAG(6, 1, 0, 0, GIF_FLG_PACKED, 1), GIF_REG_AD);
    q++;

    // Escrever no Z-buffer, proteger apenas o canal alpha (0xFF000000)
    PACK_GIFTAG(q,
                GS_SET_FRAME(fog_zbufferAddr >> 11, fog_width >> 6,
                             fog_zbufferPsm, 0xFF000000),
                GS_REG_FRAME_1);
    q++;

    // Alpha blend para inversão do Z-buffer
    PACK_GIFTAG(q, GS_SET_ALPHA(0, 1, 2, 2, 128), GS_REG_ALPHA_1);
    q++;

    PACK_GIFTAG(q, GS_SET_PRIM(GS_PRIM_SPRITE, 0, 0, 0, 1, 0, 0, 0, 0),
                GS_REG_PRIM);
    q++;
    PACK_GIFTAG(q, GS_SET_RGBAQ(255, 255, 255, 128, 0), GS_REG_RGBAQ);
    q++;
    PACK_GIFTAG(q, GS_SET_XYZ(ftoi4(0), ftoi4(0), 0), GS_REG_XYZ2);
    q++;
    PACK_GIFTAG(q, GS_SET_XYZ(ftoi4(fog_width), ftoi4(fog_height), 0),
                GS_REG_XYZ2);
    q++;
    FlushCache(0);
    dma_channel_send_normal(DMA_CHANNEL_GIF, packets, q - packets, 0, 0);
    dma_channel_wait(DMA_CHANNEL_GIF, 500);

#ifdef DEBUG_MODE
    saveDebugScreenshot("host:debug/fog_pass1_invert_z.tga", fog_zbufferAddr,
                        fog_width, fog_height, fog_zbufferPsm);
  }
#endif
}

void PostFxManager::fogPass2ClipZ() {
  // PASS 2: Clipar objetos próximos
#ifdef DEBUG_MODE
  if (g_debug_menu.fogPass2) {
#endif
    qword_t packets[200] ALIGNED(64);
    qword_t* q = packets;
    PACK_GIFTAG(q, GIF_SET_TAG(6, 1, 0, 0, GIF_FLG_PACKED, 1), GIF_REG_AD);
    q++;

    PACK_GIFTAG(q, GS_SET_TEST(1, 0, 0, 2, 0, 0, 1, ZTEST_METHOD_GREATER),
                GS_REG_TEST_1);
    q++;
    PACK_GIFTAG(q, GS_SET_ZBUF(fog_zbufferAddr >> 11, fog_zbufferPsm, 0),
                GS_REG_ZBUF_1);
    q++;
    PACK_GIFTAG(q, GS_SET_PRIM(GS_PRIM_SPRITE, 0, 0, 0, 0, 0, 0, 0, 0),
                GS_REG_PRIM);
    q++;
    PACK_GIFTAG(q, GS_SET_RGBAQ(0, 0, 0, 0, 0), GS_REG_RGBAQ);
    q++;
    PACK_GIFTAG(q, GS_SET_XYZ(ftoi4(0), ftoi4(0), CLIP_ZVALUE), GS_REG_XYZ2);
    q++;
    PACK_GIFTAG(q, GS_SET_XYZ(ftoi4(fog_width), ftoi4(fog_height), CLIP_ZVALUE),
                GS_REG_XYZ2);
    q++;

    FlushCache(0);
    dma_channel_send_normal(DMA_CHANNEL_GIF, packets, q - packets, 0, 0);
    dma_channel_wait(DMA_CHANNEL_GIF, 500);

    q = packets;
    PACK_GIFTAG(q, GIF_SET_TAG(2, 1, 0, 0, GIF_FLG_PACKED, 1), GIF_REG_AD);
    q++;
    PACK_GIFTAG(q, GS_SET_TEST(0, 0, 0, 0, 0, 0, 1, ZTEST_METHOD_ALLPASS),
                GS_REG_TEST_1);
    q++;
    PACK_GIFTAG(q, GS_SET_ZBUF(fog_zbufferAddr >> 11, fog_zbufferPsm, 1),
                GS_REG_ZBUF_1);
    q++;

    FlushCache(0);
    dma_channel_send_normal(DMA_CHANNEL_GIF, packets, q - packets, 0, 0);
    dma_channel_wait(DMA_CHANNEL_GIF, 500);

#ifdef DEBUG_MODE
    saveDebugScreenshot("host:debug/fog_pass2_clip_z.tga", fog_zbufferAddr,
                        fog_width, fog_height, fog_zbufferPsm);
  }
#endif
}

void PostFxManager::fogPass3ChannelCopy() {
  // ===== PASS 3: Channel Shuffle - Copiar canal G do Z-buffer para Alpha do
  // framebuffer =====
  //
  // TÉCNICA: Implementação correta baseada em channel_copy.cpp
  // - Processa Z-buffer em blocos de 64x32 pixels
  // - Usa textura 8-bit com CLUT de identidade para channel shuffle
  // - Copia LOCAL-TO-LOCAL para temp buffer, depois performChannelCopy
  // - Canal GREEN do Z-buffer → Canal ALPHA do framebuffer
#ifdef DEBUG_MODE
  if (g_debug_menu.fogPass3) {
#endif

    // Preparar GS para escrever apenas no canal alpha do framebuffer
    {
      qword_t packets[16] ALIGNED(64);
      qword_t* q = packets;

      PACK_GIFTAG(q, GIF_SET_TAG(5, 1, 0, 0, GIF_FLG_PACKED, 1), GIF_REG_AD);
      q++;

      // FRAME: framebuffer, máscara RGB (0x00FFFFFF) para escrever só alpha
      PACK_GIFTAG(q,
                  GS_SET_FRAME(fog_buf_frame.address >> 11, fog_width >> 6,
                               fog_buf_frame.psm, 0x00FFFFFF),
                  GS_REG_FRAME_1);
      q++;

      PACK_GIFTAG(q, GS_SET_TEST(0, 0, 0, 0, 0, 0, 1, ZTEST_METHOD_ALLPASS),
                  GS_REG_TEST_1);
      q++;

      PACK_GIFTAG(q, GS_SET_ZBUF(fog_zbufferAddr >> 11, fog_zbufferPsm, 1),
                  GS_REG_ZBUF_1);
      q++;

      PACK_GIFTAG(q, GS_SET_XYOFFSET(0, 0), GS_REG_XYOFFSET_1);
      q++;

      PACK_GIFTAG(q, GS_SET_SCISSOR(0, fog_width - 1, 0, fog_height - 1),
                  GS_REG_SCISSOR_1);
      q++;

      FlushCache(0);
      dma_channel_send_normal(DMA_CHANNEL_GIF, packets, q - packets, 0, 0);
      dma_channel_wait(DMA_CHANNEL_GIF, 500);
    }

    // Usar copyDepthBuffer que implementa a técnica correta:
    // - Loop através de blocos 64x32
    // - LOCAL-TO-LOCAL copy do Z-buffer para temp buffer
    // - performChannelCopy(GREEN → ALPHA) para cada bloco
    copyDepthBuffer(CHANNEL_GREEN, identityCLUT);

#ifdef DEBUG_MODE
    saveDebugScreenshot("host:debug/fog_pass3_channel_copy.tga",
                        fog_buf_frame.address, fog_width, fog_height,
                        fog_buf_frame.psm);
  }
#endif
}

void PostFxManager::fogPass4Downsample() {
  // 4th pass: downsample framebuffer to workbuffer
  // Usa LINEAR filtering para downsample suave
#ifdef DEBUG_MODE
  if (g_debug_menu.fogPass4) {
#endif
    qword_t packets[200] ALIGNED(64);
    qword_t* q = packets;

    // Restaurar FRAME para framebuffer antes de configurar workbuffer
    PACK_GIFTAG(q, GIF_SET_TAG(2, 1, 0, 0, GIF_FLG_PACKED, 1), GIF_REG_AD);
    q++;

    PACK_GIFTAG(q,
                GS_SET_FRAME(fog_buf_frame.address >> 11, fog_width >> 6,
                             fog_buf_frame.psm, fog_buf_frame.mask),
                GS_REG_FRAME_1);
    q++;

    PACK_GIFTAG(q, GS_SET_TEST(0, 0, 0, 0, 0, 0, 1, ZTEST_METHOD_ALLPASS),
                GS_REG_TEST_1);
    q++;

    FlushCache(0);
    dma_channel_send_normal(DMA_CHANNEL_GIF, packets, q - packets, 0, 0);
    dma_channel_wait(DMA_CHANNEL_GIF, 500);

    // Setup: FRAME = workbuffer (Z-buffer), TEX0 = framebuffer, LINEAR
    // filtering
    q = packets;
    PACK_GIFTAG(q, GIF_SET_TAG(5, 1, 0, 0, GIF_FLG_PACKED, 1), GIF_REG_AD);
    q++;

    PACK_GIFTAG(q,
                GS_SET_FRAME(fog_zbufferAddr >> 11, fog_width >> 6,
                             fog_zbufferPsm, 0xFF000000),
                GS_REG_FRAME_1);
    q++;

    PACK_GIFTAG(q, GS_SET_TEXFLUSH(0), GS_REG_TEXFLUSH);
    q++;

    // TEX0: Framebuffer como textura, TW=10 TH=10 (1024x1024), DECAL
    PACK_GIFTAG(q,
                GS_SET_TEX0(fog_buf_frame.address >> 6, fog_width >> 6,
                            fog_buf_frame.psm, 10, 10, 0, 1, 0, 0, 0, 0, 0),
                GS_REG_TEX0_1);
    q++;

    // TEX1: LINEAR filtering (importante para downsample suave)
    PACK_GIFTAG(q, GS_SET_TEX1(1, 0, 1, 1, 0, 0, 0), GS_REG_TEX1_1);
    q++;

    PACK_GIFTAG(q, GS_SET_PRIM(GS_PRIM_SPRITE, 0, 1, 0, 0, 0, 1, 0, 0),
                GS_REG_PRIM);
    q++;

    FlushCache(0);
    dma_channel_send_normal(DMA_CHANNEL_GIF, packets, q - packets, 0, 0);
    dma_channel_wait(DMA_CHANNEL_GIF, 500);

    // 1 sprite: mapeia (0,0)→(width,height) para (0,0)→(width/2,height/2)
    q = packets;
    PACK_GIFTAG(q, GIF_SET_TAG(4, 1, 0, 0, GIF_FLG_PACKED, 2),
                GIF_REG_UV | (GIF_REG_XYZ2 << 4));
    q++;

    PACK_GIFTAG(q, GS_SET_UV(ftoi4(0), ftoi4(0)), GS_REG_UV);
    q++;
    PACK_GIFTAG(q, GS_SET_XYZ(ftoi4(0), ftoi4(0), 0), GS_REG_XYZ2);
    q++;

    PACK_GIFTAG(q, GS_SET_UV(ftoi4(fog_width), ftoi4(fog_height)), GS_REG_UV);
    q++;
    PACK_GIFTAG(q, GS_SET_XYZ(ftoi4(fog_halfWidth), ftoi4(fog_halfHeight), 0),
                GS_REG_XYZ2);
    q++;

    FlushCache(0);
    dma_channel_send_normal(DMA_CHANNEL_GIF, packets, q - packets, 0, 0);
    dma_channel_wait(DMA_CHANNEL_GIF, 500);

#ifdef DEBUG_MODE
    saveDebugScreenshot("host:debug/fog_pass4_downsample.tga", fog_zbufferAddr,
                        fog_halfWidth, fog_halfHeight, fog_zbufferPsm);
  }
#endif
}

void PostFxManager::fogPass5Blend() {
  // 5th pass x2: draw workbuffer onto framebuffer with dest alpha (z-buf
  // channel)
  // Desenha o workbuffer downsampled 2x sobre o framebuffer com blur
  // Usa dest alpha (canal de profundidade) como máscara de blend
#ifdef DEBUG_MODE
  if (g_debug_menu.fogPass5) {
#endif
    qword_t packets[200] ALIGNED(64);
    qword_t* q = packets;

    // Setup: Restaurar estados e configurar blend com dest alpha
    PACK_GIFTAG(q, GIF_SET_TAG(5, 1, 0, 0, GIF_FLG_PACKED, 1), GIF_REG_AD);
    q++;

    PACK_GIFTAG(q, GS_SET_TEST(0, 0, 0, 0, 0, 0, 1, ZTEST_METHOD_ALLPASS),
                GS_REG_TEST_1);
    q++;

    PACK_GIFTAG(q, GS_SET_ZBUF(fog_zbufferAddr >> 11, fog_zbufferPsm, 1),
                GS_REG_ZBUF_1);
    q++;

    PACK_GIFTAG(q, GS_SET_XYOFFSET(0, 0), GS_REG_XYOFFSET_1);
    q++;

    PACK_GIFTAG(q, GS_SET_SCISSOR(0, fog_width - 1, 0, fog_height - 1),
                GS_REG_SCISSOR_1);
    q++;

    PACK_GIFTAG(q,
                GS_SET_FRAME(fog_buf_frame.address >> 11, fog_width >> 6,
                             fog_buf_frame.psm, 0xFF000000),
                GS_REG_FRAME_1);
    q++;

    FlushCache(0);
    dma_channel_send_normal(DMA_CHANNEL_GIF, packets, q - packets, 0, 0);
    dma_channel_wait(DMA_CHANNEL_GIF, 500);

    q = packets;
    PACK_GIFTAG(q, GIF_SET_TAG(3, 1, 0, 0, GIF_FLG_PACKED, 1), GIF_REG_AD);
    q++;

    PACK_GIFTAG(q, GS_SET_TEXFLUSH(0), GS_REG_TEXFLUSH);
    q++;

    // Textura do workbuffer (downsampled): tw=10, th=10, DECAL
    PACK_GIFTAG(q,
                GS_SET_TEX0(fog_zbufferAddr >> 6, fog_width >> 6,
                            fog_buf_frame.psm, 10, 10, 1, 1, 0, 0, 0, 0, 0),
                GS_REG_TEX0_1);
    q++;

    // ALPHA: (Cd - Cs) * Ad + Cs (blend usando dest alpha como máscara)
    PACK_GIFTAG(q, GS_SET_ALPHA(1, 0, 1, 0, 0), GS_REG_ALPHA_1);
    q++;

    FlushCache(0);
    dma_channel_send_normal(DMA_CHANNEL_GIF, packets, q - packets, 0, 0);
    dma_channel_wait(DMA_CHANNEL_GIF, 500);

    q = packets;
    PACK_GIFTAG(q, GIF_SET_TAG(1, 1, 0, 0, GIF_FLG_PACKED, 1), GIF_REG_AD);
    q++;

    PACK_GIFTAG(q, GS_SET_PRIM(GS_PRIM_SPRITE, 0, 1, 0, 1, 0, 1, 0, 0),
                GS_REG_PRIM);
    q++;

    FlushCache(0);
    dma_channel_send_normal(DMA_CHANNEL_GIF, packets, q - packets, 0, 0);
    dma_channel_wait(DMA_CHANNEL_GIF, 500);

    // Offset para criar efeito de blur cruzado (conforme GTA SA)
    constexpr int32_t FOG_BLEND_OFFSET = 16;

    // Primeiro sprite: offset positivo para criar blur
    q = packets;
    PACK_GIFTAG(q, GIF_SET_TAG(4, 1, 0, 0, GIF_FLG_PACKED, 2),
                GIF_REG_UV | (GIF_REG_XYZ2 << 4));
    q++;
    PACK_GIFTAG(q, GS_SET_UV(ftoi4(1), ftoi4(1)), GS_REG_UV);
    q++;
    PACK_GIFTAG(
        q,
        GS_SET_XYZ(ftoi4(1) + FOG_BLEND_OFFSET, ftoi4(1) + FOG_BLEND_OFFSET, 0),
        GS_REG_XYZ2);
    q++;
    PACK_GIFTAG(q, GS_SET_UV(ftoi4(fog_halfWidth), ftoi4(fog_halfHeight)),
                GS_REG_UV);
    q++;
    PACK_GIFTAG(q,
                GS_SET_XYZ(ftoi4(fog_width + 1) + FOG_BLEND_OFFSET,
                           ftoi4(fog_height + 1) + FOG_BLEND_OFFSET, 0),
                GS_REG_XYZ2);
    q++;

    FlushCache(0);
    dma_channel_send_normal(DMA_CHANNEL_GIF, packets, q - packets, 0, 0);
    dma_channel_wait(DMA_CHANNEL_GIF, 500);

    // Segundo sprite: offset negativo (complementar) para blur cruzado
    q = packets;
    PACK_GIFTAG(q, GIF_SET_TAG(4, 1, 0, 0, GIF_FLG_PACKED, 2),
                GIF_REG_UV | (GIF_REG_XYZ2 << 4));
    q++;
    PACK_GIFTAG(
        q, GS_SET_UV(ftoi4(0) + FOG_BLEND_OFFSET, ftoi4(0) + FOG_BLEND_OFFSET),
        GS_REG_UV);
    q++;
    PACK_GIFTAG(q, GS_SET_XYZ(ftoi4(0), ftoi4(0), 0), GS_REG_XYZ2);
    q++;
    PACK_GIFTAG(q, GS_SET_UV(ftoi4(fog_halfWidth), ftoi4(fog_halfHeight)),
                GS_REG_UV);
    q++;
    PACK_GIFTAG(q,
                GS_SET_XYZ(ftoi4(fog_width) - FOG_BLEND_OFFSET,
                           ftoi4(fog_height) - FOG_BLEND_OFFSET, 0),
                GS_REG_XYZ2);
    q++;

    FlushCache(0);
    dma_channel_send_normal(DMA_CHANNEL_GIF, packets, q - packets, 0, 0);
    dma_channel_wait(DMA_CHANNEL_GIF, 500);

#ifdef DEBUG_MODE
    saveDebugScreenshot("host:debug/fog_pass5_blend_alpha.tga",
                        fog_buf_frame.address, fog_width, fog_height,
                        fog_buf_frame.psm);
  }
#endif
}

void PostFxManager::fogPass6Apply(const Color& fogColor) {
#ifdef DEBUG_MODE
  if (g_debug_menu.fogPass6) {
#endif
    qword_t packets[200] ALIGNED(64);
    qword_t* q = packets;

    // Apenas aplicar fog se a cor tiver alpha > 0
    if (fogColor.a > 0) {
      // ===== 6th pass part 1: add saturation =====
      // Setup registros GS
      PACK_GIFTAG(q, GIF_SET_TAG(5, 1, 0, 0, GIF_FLG_PACKED, 1), GIF_REG_AD);
      q++;

      PACK_GIFTAG(
          q,
          GS_SET_RGBAQ((int)fogColor.r, (int)fogColor.g, (int)fogColor.b, 0, 0),
          GS_REG_RGBAQ);
      q++;

      PACK_GIFTAG(q, GS_SET_TEXFLUSH(0), GS_REG_TEXFLUSH);
      q++;

      // TEX0: tw=10, th=10 conforme original, MODULATE para saturation
      PACK_GIFTAG(q,
                  GS_SET_TEX0(fog_buf_frame.address >> 6, fog_width >> 6,
                              fog_buf_frame.psm, 10, 10, 1, 2, 0, 0, 0, 0, 0),
                  GS_REG_TEX0_1);
      q++;

      PACK_GIFTAG(q, GS_SET_PRIM(GS_PRIM_SPRITE, 0, 1, 0, 1, 0, 1, 0, 0),
                  GS_REG_PRIM);
      q++;

      // Blend saturation: (Cs - 0) * FIX + Cd, onde FIX = fog.alpha
      PACK_GIFTAG(q, GS_SET_ALPHA(0, 2, 2, 1, (int)fogColor.a), GS_REG_ALPHA_1);
      q++;

      FlushCache(0);
      dma_channel_send_normal(DMA_CHANNEL_GIF, packets, q - packets, 0, 0);
      dma_channel_wait(DMA_CHANNEL_GIF, 500);

      // Primitivas do sprite
      q = packets;
      PACK_GIFTAG(q, GIF_SET_TAG(4, 1, 0, 0, GIF_FLG_PACKED, 2),
                  GIF_REG_UV | (GIF_REG_XYZ2 << 4));
      q++;
      PACK_GIFTAG(q, GS_SET_UV(ftoi4(0), ftoi4(0)), GS_REG_UV);
      q++;
      PACK_GIFTAG(q, GS_SET_XYZ(ftoi4(0), ftoi4(0), 0), GS_REG_XYZ2);
      q++;
      PACK_GIFTAG(q, GS_SET_UV(ftoi4(fog_width), ftoi4(fog_height)), GS_REG_UV);
      q++;
      PACK_GIFTAG(q, GS_SET_XYZ(ftoi4(fog_width), ftoi4(fog_height), 0),
                  GS_REG_XYZ2);
      q++;

      FlushCache(0);
      dma_channel_send_normal(DMA_CHANNEL_GIF, packets, q - packets, 0, 0);
      dma_channel_wait(DMA_CHANNEL_GIF, 500);

      // ===== 6th pass part 2: add fog color (SÓLIDO) =====
      // Setup registros GS
      q = packets;
      PACK_GIFTAG(q, GIF_SET_TAG(3, 1, 0, 0, GIF_FLG_PACKED, 1), GIF_REG_AD);
      q++;

      // Blend fog usando dest alpha: (Cs - Cd) * Ad + Cd
      PACK_GIFTAG(q, GS_SET_ALPHA(0, 1, 1, 1, 0), GS_REG_ALPHA_1);
      q++;

      // Sprite sem textura
      PACK_GIFTAG(q, GS_SET_PRIM(GS_PRIM_SPRITE, 0, 0, 0, 1, 0, 1, 0, 0),
                  GS_REG_PRIM);
      q++;

      // Cor do fog
      PACK_GIFTAG(q,
                  GS_SET_RGBAQ((int)fogColor.r, (int)fogColor.g,
                               (int)fogColor.b, (int)fogColor.a, 0),
                  GS_REG_RGBAQ);
      q++;

      FlushCache(0);
      dma_channel_send_normal(DMA_CHANNEL_GIF, packets, q - packets, 0, 0);
      dma_channel_wait(DMA_CHANNEL_GIF, 500);

      // Primitivas do sprite
      q = packets;
      PACK_GIFTAG(q, GIF_SET_TAG(2, 1, 0, 0, GIF_FLG_PACKED, 1), GIF_REG_AD);
      q++;
      PACK_GIFTAG(q, GS_SET_XYZ(ftoi4(0), ftoi4(0), 0), GS_REG_XYZ2);
      q++;
      PACK_GIFTAG(q, GS_SET_XYZ(ftoi4(fog_width), ftoi4(fog_height), 0),
                  GS_REG_XYZ2);
      q++;
    } else {
      // Se fog.alpha == 0, pula o efeito
      q = packets;
      PACK_GIFTAG(q, GIF_SET_TAG(1, 1, 0, 0, GIF_FLG_PACKED, 1), GIF_REG_AD);
      q++;

      // Restaurar blend padrão
      PACK_GIFTAG(q, GS_SET_ALPHA(0, 1, 0, 1, 0), GS_REG_ALPHA_1);
      q++;
    }

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
    ztest.method = fog_zbuffer.method;

    qword_t packets[200] ALIGNED(64);
    qword_t* q = packets;
    PACK_GIFTAG(q, GIF_SET_TAG(8, 1, 0, 0, GIF_FLG_PACKED, 1), GIF_REG_AD);
    q++;

    PACK_GIFTAG(
        q,
        GS_SET_FRAME(fog_buf_frame.address >> 11, fog_buf_frame.width >> 6,
                     fog_buf_frame.psm, fog_buf_frame.mask),
        GS_REG_FRAME_1);
    q++;

    // ZBuffer setting
    PACK_GIFTAG(q,
                GS_SET_ZBUF(fog_zbuffer.address >> 11, fog_zbuffer.zsm,
                            fog_zbuffer.mask),
                GS_REG_ZBUF);
    q++;

    // Primitive coordinate offsets - RESTAURAR OFFSET ORIGINAL
    PACK_GIFTAG(
        q,
        GS_SET_XYOFFSET((int)(2048.0f - (fog_buf_frame.width / 2.0F) * 16.0f),
                        (int)(2048.0f - (fog_buf_frame.height / 2.0F) * 16.0f)),
        GS_REG_XYOFFSET_1);
    q++;

    // Scissoring area
    PACK_GIFTAG(
        q,
        GS_SET_SCISSOR(0, fog_buf_frame.width - 1, 0, fog_buf_frame.height - 1),
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

  // ===== DOCUMENTAÇÃO TÉCNICA: Depth of Field com Z-Buffer Fog =====
  // Baseado em técnica original Sony (fog.txt - 640x448)
  // Adaptado para 512x448 com channel shuffle G→A (16-bit mode)
  //
  // Passes:
  // 1. Invert Z-buffer values (para usar ZGREATER em Pass 2)
  // 2. Clip objetos próximos (remove da máscara de profundidade)
  // 3. Channel copy: G-channel do Z-buffer → Alpha-channel do framebuffer
  //    - Usa textura 8-bit + CLUT identidade em blocos 64x32 (channel_copy)
  //    - Copia local-to-local para temp buffer e faz shuffle por bloco
  // 4. Downsample framebuffer para workbuffer (width/2 × height/2)
  // 5. Draw workbuffer com alpha blending (profundidade como máscara)
  // 6. Aplicar cor fog e saturação

  // Configurar propriedades reutilizáveis
  fog_width = SCREEN_WIDTH;
  fog_height = SCREEN_HEIGHT;
  fog_halfWidth = fog_width / 2;
  fog_halfHeight = fog_height / 2;
  setTwTh(fog_width, fog_height, &fog_tw, &fog_th);

  fog_zbuffer = pRenderer->core.gs.zBuffer;
  fog_buf_frame = pRenderer->core.gs.getCurrentFrameData();
  fog_zbufferAddr = fog_zbuffer.address;
  fog_zbufferPsm = fog_zbuffer.zsm;

  // ===== PASS 1 & 2: Clipar Z-Buffer =====
  // Remove objetos próximos do zbuffer para que não recebam fog

  fogPassSetup();
  fogPass1InvertZ();
  fogPass2ClipZ();
  fogPass3ChannelCopy();
  fogPass4Downsample();
  fogPass5Blend();
  fogPass6Apply(fogColor);
  fogPassRestore();
}
