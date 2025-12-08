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
  (void)pRenderer->core.texture.useTexture(palette);
  RendererCoreTextureBuffers texBuffer =
      pRenderer->core.texture.useTexture(palette);
  clutVramAddress = texBuffer.core->address;

  uint32_t page = 0;
  uint32_t x, y;

  const uint32_t src_tbw = 1;  // width >> 6;  // TBW for full-screen zbuffer
  const uint32_t dst_tbw = 1;  // Temp buffer is 64px wide

  for (y = 0; y < height; y += 32) {
    for (x = 0; x < width; x += 64) {
      uint32_t buf_addr =
          pRenderer->core.texture.useTexture(pDepthBufferTexture).core->address;

      qword_t packets[10] ALIGNED(64);
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

  PACK_GIFTAG(q, GIF_SET_TAG(6, 1, 0, 0, GIF_FLG_PACKED, 1), GIF_REG_AD);
  q++;

  PACK_GIFTAG(q, GS_SET_XYOFFSET(0, 0), GS_REG_XYOFFSET_1);
  q++;

  // TEX0: Configuração da textura 8-bit
  // Para Z-buffer em PSMZ32, usar TBW baseado na largura da tela
  // TBW deve corresponder ao layout do Z-buffer: width >> 6
  // source_addr já aponta para o Z-buffer completo (não para blocos
  // individuais)
  int tw, th;
  setTwTh(fog_width, fog_height, &tw, &th);
  PACK_GIFTAG(q,
              GS_SET_TEX0(source_addr >> 6, fog_width >> 6, GS_PSM_8, tw, th, 1,
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
  // ===== CONFIGURAÇÃO INICIAL =====
  // Define os parâmetros do GS para os passes subsequentes
  // - TEST: ALLPASS (sem testes, aceita tudo)
  // - ZBUF: Habilita escrita no Z-buffer (necessário para Pass 1)
  // - XYOFFSET: (0,0) (offset de coordenadas)
  // - PABE: Desabilitado (não usa alpha per-pixel)
  // - SCISSOR: Clipping area = resolução completa (512x448)
  // - COLCLAMP: Habilitado (evita overflow de cores)

#ifdef DEBUG_MODE
  if (g_debug_menu.fogPassSetup == false) return;
#endif

  qword_t packets[200] ALIGNED(64);
  qword_t* q = packets;

  // GIF tag: 6 registradores A+D
  PACK_GIFTAG(q, GIF_SET_TAG(6, 1, 0, 0, GIF_FLG_PACKED, 1), GIF_REG_AD);
  q++;

  // TEST_1: ALLPASS (aceita todos os pixels)
  PACK_GIFTAG(q, GS_SET_TEST(0, 0, 0, 0, 0, 0, 1, ZTEST_METHOD_ALLPASS),
              GS_REG_TEST_1);
  q++;

  // ZBUF_1: Z-buffer writes habilitadas (mask=0 = escreve)
  PACK_GIFTAG(q, GS_SET_ZBUF(fog_zbufferAddr >> 11, fog_zbufferPsm, 1),
              GS_REG_ZBUF_1);
  q++;

  // XYOFFSET_1: Offset (0,0) para coordenadas de primitivas
  PACK_GIFTAG(q, GS_SET_XYOFFSET(0, 0), GS_REG_XYOFFSET_1);
  q++;

  // PABE: Per-pixel alpha blending desabilitado
  PACK_GIFTAG(q, GS_SET_PABE(0), GS_REG_PABE);
  q++;

  // SCISSOR_1: Clipping area (0,511) x (0,447) para 512x448
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

void PostFxManager::fogPass1InvertZ() {
  // ===== PASS 1: INVERTER Z-BUFFER =====
  // Objetivo: Transformar Z-buffer para usar ZTEST_GREATER no Pass 2
  //
  // Lógica:
  // - PS2 Z-buffer: objetos PRÓXIMOS = valores ALTOS, DISTANTES = valores
  // BAIXOS
  // - Após inversão: objetos PRÓXIMOS = valores BAIXOS, DISTANTES = valores
  // ALTOS
  // - Isso permite usar ZTEST_GREATER para clipar objetos próximos
  //
  // Técnica: Desenhar sprite full-screen com valor Z=0
  // - Com ZTEST_ALLPASS: escreve Z=0 em cada pixel
  // - Com alpha blend (0,1,2,2,128): result = 0xFF - Z_original (inverte)
  // - Resultado: Z_invertido = 0xFFFFFF - Z_original

#ifdef DEBUG_MODE
  if (g_debug_menu.fogPass1) {
    saveDebugScreenshot("host:debug/fog_pass1_before_invert_z.tga",
                        fog_zbufferAddr, fog_width, fog_height, fog_zbufferPsm);
#endif

    {
      qword_t packets[100] ALIGNED(64);
      qword_t* q = packets;

      // GIF tag: 4 registradores A+D (FRAME, ALPHA, PRIM, RGBAQ)
      PACK_GIFTAG(q, GIF_SET_TAG(6, 1, 0, 0, GIF_FLG_PACKED, 1), GIF_REG_AD);
      q++;

      // FRAME_1: Z-buffer como destino
      PACK_GIFTAG(q,
                  GS_SET_FRAME(fog_zbufferAddr >> 11, fog_width >> 6,
                               fog_zbufferPsm, 0xFF000000),
                  GS_REG_FRAME_1);
      q++;

      // ALPHA_1: (0,1,2,2,128) = Cd - (As * Cs)
      // Resultado: 0xFF - Z_original
      PACK_GIFTAG(q, GS_SET_ALPHA(0, 1, 2, 2, 128), GS_REG_ALPHA_1);
      q++;

      // PRIM: Sprite, sem textura, sem anti-aliasing
      PACK_GIFTAG(q, GS_SET_PRIM(GS_PRIM_SPRITE, 0, 0, 0, 1, 0, 0, 0, 0),
                  GS_REG_PRIM);
      q++;

      // RGBAQ: R=255, G=255, B=255, Alpha=128, Q=0
      PACK_GIFTAG(q, GS_SET_RGBAQ(255, 255, 255, 128, 0), GS_REG_RGBAQ);
      q++;

      // Dois XYZ2
      PACK_GIFTAG(q, GS_SET_XYZ(ftoi4(0), ftoi4(0), 0), GIF_REG_XYZ2);
      q++;

      PACK_GIFTAG(q, GS_SET_XYZ(ftoi4(fog_width), ftoi4(fog_height), 0),
                  GIF_REG_XYZ2);
      q++;

      FlushCache(0);
      dma_channel_send_normal(DMA_CHANNEL_GIF, packets, q - packets, 0, 0);
      dma_channel_wait(DMA_CHANNEL_GIF, 500);
    }

#ifdef DEBUG_MODE
    saveDebugScreenshot("host:debug/fog_pass1_invert_z.tga", fog_zbufferAddr,
                        fog_width, fog_height, fog_zbufferPsm);
  }
#endif
}

void PostFxManager::fogPass2ClipZ() {
  // ===== PASS 2: CLIPAR OBJETOS PRÓXIMOS =====
  // Objetivo: Remover objetos próximos do Z-buffer
  //
  // Lógica:
  // - ZTEST_GREATER: se CLIP_ZVALUE > Z_Buffer, sobrescreve pixel
  // - Após Pass 1: objetos PRÓXIMOS têm Z BAIXO
  // - Logo: pixels com Z < CLIP_ZVALUE são clippados
  // - Resultado: apenas objetos DISTANTES permanecem no Z-buffer
  //
  // Técnica: Desenhar sprite full-screen com Z=CLIP_ZVALUE e RGBA=0
  // - Com ZTEST_GREATER: sobrescreve pixels onde Z < CLIP_ZVALUE
  // - Com RGBA=0: limpa os pixels clippados

#ifdef DEBUG_MODE
  if (g_debug_menu.fogPass2) {
#endif

    qword_t packets[100] ALIGNED(64);
    qword_t* q = packets;

    // GIF tag: 3 registradores A+D
    PACK_GIFTAG(q, GIF_SET_TAG(8, 1, 0, 0, GIF_FLG_PACKED, 1), GIF_REG_AD);
    q++;

    // TEST_1: ZTEST_GREATER
    // - Sobrescreve se CLIP_ZVALUE > Z_Buffer
    // - Alpha test desabilitado, depth test habilitado
    PACK_GIFTAG(q, GS_SET_TEST(1, 0, 0, 2, 0, 0, 1, ZTEST_METHOD_GREATER),
                GS_REG_TEST_1);
    q++;

    // ZBUF_1: Z-buffer writes habilitadas (mask=0 = escreve)
    PACK_GIFTAG(q, GS_SET_ZBUF(fog_zbufferAddr >> 11, fog_zbufferPsm, 0),
                GS_REG_ZBUF_1);
    q++;

    // PRIM: Sprite, sem textura, sem anti-aliasing
    // Importante: COLOR_OUTPUT = 0 (não modifica framebuffer)
    PACK_GIFTAG(q, GS_SET_PRIM(GS_PRIM_SPRITE, 0, 0, 0, 0, 0, 0, 0, 0),
                GS_REG_PRIM);
    q++;

    PACK_GIFTAG(q, GS_SET_RGBAQ(0, 0, 0, 0, 0), GS_REG_RGBAQ);
    q++;

    // Dois XYZ2
    PACK_GIFTAG(q, GS_SET_XYZ(ftoi4(0), ftoi4(0), CLIP_ZVALUE), GIF_REG_XYZ2);
    q++;

    // XYZ2 end: (512, 448, CLIP_ZVALUE)
    PACK_GIFTAG(q, GS_SET_XYZ(ftoi4(fog_width), ftoi4(fog_height), CLIP_ZVALUE),
                GIF_REG_XYZ2);
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
  // ===== PASS 3: CHANNEL COPY =====
  // Objetivo: Copiar canal GREEN do Z-buffer para canal ALPHA do framebuffer
  //
  // Técnica baseada em channel_copy.cpp:
  // - Z-buffer em PSM_32: 4 bytes por pixel (R,G,B,A)
  // - Canal GREEN (bits 8-15) contém informação de profundidade
  // - Usa textura 8-bit + CLUT identidade para fazer channel shuffle
  // - Processa em blocos de 64x32 pixels
  //
  // Padrão de sprites:
  // - Linhas pares (y%4==0): 4 sprites de 16x2 cada (EVENs)
  // - Linhas ímpares: 8 sprites de 8x2 cada (ODDs)
  // - Usa offsets horizontais/verticais para selecionar canal G
  //
  // Lógica de offsets:
  // - BLUE/ALPHA: offset U = 8 texels (horiz_block_offset=1)
  // - GREEN/ALPHA: offset V = 2 texels (vert_block_offset=1)
  // - GREEN channel: vert_block_offset=1, horz_block_offset=0

#ifdef DEBUG_MODE
  if (g_debug_menu.fogPass3) {
#endif

    copyDepthBuffer(CHANNEL_GREEN, identityCLUT);

#ifdef DEBUG_MODE
    return saveDebugScreenshot("host:debug/fog_pass3_channel_copy.tga",
                               fog_buf_frame.address, fog_width, fog_height,
                               fog_buf_frame.psm);
  }
#endif
}

void PostFxManager::fogPass4Downsample() {
  // ===== PASS 4: DOWNSAMPLE =====
  // Objetivo: Reduzir framebuffer para metade (512x448 → 256x224)
  // Destino: Z-buffer (usado como buffer temporário)
  //
  // Técnica:
  // - Source: Framebuffer (512x448)
  // - Dest: Z-buffer como image buffer
  // - Filter: LINEAR para suavização
  // - UV: (0,0) to (width, height)
  // - XYZ: (0,0) to (width/2, height/2)

#ifdef DEBUG_MODE
  if (g_debug_menu.fogPass4) {
#endif
    qword_t packets[200] ALIGNED(64);
    qword_t* q = packets;

    // TAG 1: FRAME + TEXFLUSH + TEX0 + TEX1 (4 regs)
    PACK_GIFTAG(q, GIF_SET_TAG(4, 1, 0, 0, GIF_FLG_PACKED, 1), GIF_REG_AD);
    q++;
    PACK_GIFTAG(q,
                GS_SET_FRAME(fog_zbufferAddr >> 11, fog_width >> 6,
                             fog_buf_frame.psm, 0),
                GS_REG_FRAME_1);
    q++;
    PACK_GIFTAG(q, GS_SET_TEXFLUSH(1), GS_REG_TEXFLUSH);
    q++;
    PACK_GIFTAG(q,
                GS_SET_TEX0(fog_buf_frame.address >> 6, fog_width >> 6,
                            fog_buf_frame.psm, 10, 10, 0, 0, 0, 0, 0, 0, 0),
                GS_REG_TEX0_1);
    q++;
    PACK_GIFTAG(q, GS_SET_TEX1(0, 0, 1, 1, 0, 0, 0), GS_REG_TEX1_1);
    q++;

    // TAG 2: PRIM (1 reg)
    PACK_GIFTAG(q, GIF_SET_TAG(1, 1, 0, 0, GIF_FLG_PACKED, 1), GIF_REG_AD);
    q++;
    PACK_GIFTAG(q, GS_SET_PRIM(GS_PRIM_SPRITE, 0, 1, 0, 0, 0, 1, 0, 0),
                GS_REG_PRIM);
    q++;

    // TAG 3: UV/XYZ2 start (2 regs)
    PACK_GIFTAG(q, GIF_SET_TAG(2, 1, 0, 0, GIF_FLG_PACKED, 2),
                (GIF_REG_UV) | (GIF_REG_XYZ2 << 4));
    q++;
    PACK_GIFTAG(q, GS_SET_UV(0, 0), GIF_REG_UV);
    q++;
    PACK_GIFTAG(q, GS_SET_XYZ(ftoi4(0), ftoi4(0), 0), GIF_REG_XYZ2);
    q++;

    // TAG 4: UV/XYZ2 end (2 regs)
    PACK_GIFTAG(q, GIF_SET_TAG(2, 1, 0, 0, GIF_FLG_PACKED, 2),
                (GIF_REG_UV) | (GIF_REG_XYZ2 << 4));
    q++;
    PACK_GIFTAG(q, GS_SET_UV((fog_width << 4), (fog_height << 4)), GIF_REG_UV);
    q++;
    PACK_GIFTAG(q, GS_SET_XYZ(ftoi4(fog_halfWidth), ftoi4(fog_halfHeight), 0),
                GIF_REG_XYZ2);
    q++;

    FlushCache(0);
    dma_channel_send_normal(DMA_CHANNEL_GIF, packets, q - packets, 0, 0);
    dma_channel_wait(DMA_CHANNEL_GIF, 500);

#ifdef DEBUG_MODE
    saveDebugScreenshot("host:debug/fog_pass4_downsample.tga", fog_zbufferAddr,
                        fog_halfWidth, fog_halfHeight, fog_buf_frame.psm);
  }
#endif
}

void PostFxManager::fogPass5Blend() {
  // ===== PASS 5: BLEND ALPHA =====
  // Objetivo: Desenhar workbuffer no framebuffer usando alpha como máscara
  //
  // Técnica:
  // - Source: Workbuffer (Z-buffer com downsampled framebuffer + alpha channel)
  // - Dest: Framebuffer original
  // - Blend: (1,0,1,0,0) = Cs * As
  // - Resultado: blending baseado no alpha extraído do Z-buffer

#ifdef DEBUG_MODE
  if (g_debug_menu.fogPass5) {
#endif
    qword_t packets[200] ALIGNED(64);
    qword_t* q = packets;

    // TAG 1: FRAME + TEXFLUSH + TEX0 + ALPHA (4 regs)
    PACK_GIFTAG(q, GIF_SET_TAG(4, 1, 0, 0, GIF_FLG_PACKED, 1), GIF_REG_AD);
    q++;
    PACK_GIFTAG(q,
                GS_SET_FRAME(fog_buf_frame.address >> 11,
                             fog_buf_frame.width >> 6, fog_buf_frame.psm, 0),
                GS_REG_FRAME_1);
    q++;
    PACK_GIFTAG(q, GS_SET_TEXFLUSH(1), GS_REG_TEXFLUSH);
    q++;
    PACK_GIFTAG(q,
                GS_SET_TEX0(fog_zbufferAddr >> 6, fog_width >> 6,
                            fog_buf_frame.psm, 10, 10, 1, 0, 0, 0, 0, 0, 0),
                GS_REG_TEX0_1);
    q++;
    PACK_GIFTAG(q, GS_SET_ALPHA(1, 0, 1, 0, 0), GS_REG_ALPHA_1);
    q++;

    // TAG 2: PRIM (1 reg)
    PACK_GIFTAG(q, GIF_SET_TAG(1, 1, 0, 0, GIF_FLG_PACKED, 1), GIF_REG_AD);
    q++;
    PACK_GIFTAG(q, GS_SET_PRIM(GS_PRIM_SPRITE, 0, 1, 0, 1, 0, 1, 0, 0),
                GS_REG_PRIM);
    q++;

    // TAG 3: UV/XYZ2 start (2 regs)
    PACK_GIFTAG(q, GIF_SET_TAG(2, 1, 0, 0, GIF_FLG_PACKED, 2),
                (GIF_REG_UV) | (GIF_REG_XYZ2 << 4));
    q++;
    PACK_GIFTAG(q, GS_SET_UV(0, 0), GIF_REG_UV);
    q++;
    PACK_GIFTAG(q, GS_SET_XYZ(ftoi4(0), ftoi4(0), 0), GIF_REG_XYZ2);
    q++;

    // TAG 4: UV/XYZ2 end (2 regs)
    PACK_GIFTAG(q, GIF_SET_TAG(2, 1, 0, 0, GIF_FLG_PACKED, 2),
                (GIF_REG_UV) | (GIF_REG_XYZ2 << 4));
    q++;
    PACK_GIFTAG(q, GS_SET_UV((fog_halfWidth << 4), (fog_halfHeight << 4)),
                GIF_REG_UV);
    q++;
    PACK_GIFTAG(q, GS_SET_XYZ(ftoi4(fog_width), ftoi4(fog_height), 0),
                GIF_REG_XYZ2);
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
  // ===== PASS 6: APLICAR COR DE FOG =====
  // Objetivo: Sobrepor cor de fog no framebuffer
  //
  // NOTA: DESABILITADO
  // Pass 5 (Blend Alpha) já aplica o efeito de profundidade no framebuffer.
  // Pass 6 não é necessário para o funcionamento básico e estava causando
  // artefatos visuais. A cor do fog pode ser controlada ajustando a
  // renderização dos objetos na cena em vez de um pós-efeito separado.

#ifdef DEBUG_MODE
  if (g_debug_menu.fogPass6) {
#endif

    // ADDTOPKT(SCE_GS_RGBAQ, SCE_GS_SET_RGBAQ(fog.red, fog.green, fog.blue, 0,
    // 0) ); ADDTOPKT(SCE_GS_ALPHA_1, SCE_GS_SET_ALPHA(0,2,1,1,0) );
    // ADDTOPKT(SCE_GS_PRIM, SCE_GS_SET_PRIM(SCE_GS_PRIM_SPRITE,0,0,0,1,0,1,0,0)
    // ); ADDTOPKT(SCE_GS_XYZ2, SCE_GS_SET_XYZ(GETPRIMCOORD(0), GETPRIMCOORD(0),
    // 0) ); ADDTOPKT(SCE_GS_XYZ2, SCE_GS_SET_XYZ(GETPRIMCOORD(width),
    // GETPRIMCOORD(SCREEN_DRAW_HEIGHT), 0) );

#ifdef DEBUG_MODE
    saveDebugScreenshot("host:debug/fog_pass6_final.tga", fog_buf_frame.address,
                        fog_width, fog_height, fog_buf_frame.psm);
  }
#endif
}

void PostFxManager::fogPass6CustomApply(const Color& fogColor) {
  // ===== PASS 6: APLICAR COR DE FOG =====
  // Objetivo: Sobrepor cor de fog no framebuffer

#ifdef DEBUG_MODE
  if (g_debug_menu.fogPass6) {
#endif

    qword_t packets[50] ALIGNED(64);
    qword_t* q = packets;

    PACK_GIFTAG(q, GIF_SET_TAG(3, 1, 0, 0, GIF_FLG_PACKED, 1), GIF_REG_AD);
    q++;

    PACK_GIFTAG(q, GS_SET_TEST(0, 0, 0, 0, 0, 0, 1, 1), GS_REG_TEST_1);
    q++;

    // The GS's alpha blending formula is fixed but it contains four variables
    // that can be reconfigured: Output = (((A - B) * C) >> 7) + D A, B, and D
    // are colors and C is an alpha value. Their specific values come from the
    // ALPHA register:
    //       A                B                C                   D
    //   0   Source RGB       Source RGB       Source alpha        Source RGB
    //   1   Framebuffer RGB  Framebuffer RGB  Framebuffer alpha   Framebuffer
    //   RGB 2   0                0                FIX                 0 3
    //   Reserved         Reserved         Reserved            Reserved
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
      PACK_GIFTAG(q, (uint64_t)(fogColor.r) | (uint64_t)(fogColor.g) << 32,
                  (uint64_t)(fogColor.b) | (uint64_t)(fogColor.a) << 32);
      q++;

      // XYZ2
      PACK_GIFTAG(q, GIF_SET_XYZ(0, 0, 0), 1);
      q++;

      // TRC
      // RGBAQ
      PACK_GIFTAG(q, (uint64_t)(fogColor.r) | (uint64_t)(fogColor.g) << 32,
                  (uint64_t)(fogColor.b) | (uint64_t)(fogColor.a) << 32);
      q++;

      // XYZ2
      PACK_GIFTAG(q, GIF_SET_XYZ(ftoi4(fog_width), 0, 0), 1);
      q++;
    }

    // LOOP 2
    {
      // BLC
      // RGBAQ
      PACK_GIFTAG(q, (uint64_t)(fogColor.r) | (uint64_t)(fogColor.g) << 32,
                  (uint64_t)(fogColor.b) | (uint64_t)(fogColor.a) << 32);
      q++;

      // XYZ2
      PACK_GIFTAG(q, GIF_SET_XYZ(0, 0, ftoi4(fog_height)), 1);
      q++;

      // BRC
      // RGBAQ
      PACK_GIFTAG(q, (uint64_t)(fogColor.r) | (uint64_t)(fogColor.g) << 32,
                  (uint64_t)(fogColor.b) | (uint64_t)(fogColor.a) << 32);
      q++;

      // XYZ2
      PACK_GIFTAG(q, GIF_SET_XYZ(ftoi4(fog_width), 0, ftoi4(fog_height)), 1);
      q++;
    }

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

    qword_t packets[200] ALIGNED(64);
    qword_t* q = packets;

    // GIF tag: 8 registradores A+D
    PACK_GIFTAG(q, GIF_SET_TAG(8, 1, 0, 0, GIF_FLG_PACKED, 1), GIF_REG_AD);
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
  fogPass1InvertZ();
  fogPass3ChannelCopy();
  fogPass4Downsample();
  fogPass5Blend();
  // fogPass6Apply(fogColor);
  fogPass6CustomApply(fogColor);
  fogPassRestore();
}
