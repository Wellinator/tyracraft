#include "managers/post-fx/post_fx_manager.hpp"
#include "managers/dma_gif_builder.hpp"
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

  // fogStartPercent define onde o fog COMEÇA (objetos mais próximos ficam nítidos)
  // fogStartPercent=0.0 -> fog começa no near (tudo tem fog, nada é clippado)
  // fogStartPercent=0.5 -> fog começa no meio (metade próxima é clippada/nítida)
  // fogStartPercent=1.0 -> fog começa no far (quase tudo é clippado/nítido)
  float fogDistance = nearPlane + (farPlane - nearPlane) * fogStartPercent;

  // Evitar divisão por zero
  if (fogDistance <= nearPlane) fogDistance = nearPlane + 0.001f;
  if (fogDistance >= farPlane) fogDistance = farPlane - 0.001f;

  // Calcular Z original usando a fórmula do Z-Buffer perspectivo do PS2
  // Z-Buffer do PS2: objetos PRÓXIMOS têm valores ALTOS, objetos DISTANTES têm valores BAIXOS
  const float maxZ = 16777215.0f;  // 2^24 - 1 = 0xFFFFFF

  // Fórmula do Z-Buffer perspectivo:
  // Z_normalized = (far / distance - 1) / (far / near - 1)
  float zNormalized =
      (farPlane / fogDistance - 1.0f) / (farPlane / nearPlane - 1.0f);
  uint32_t zOriginal = (uint32_t)(zNormalized * maxZ);

  // APÓS PASS 1 (inversão): Z_invertido = 0xFFFFFF - Z_original
  // Resultado: objetos PRÓXIMOS têm valores BAIXOS, objetos DISTANTES têm valores ALTOS
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

#ifdef DEBUG_MODE
void PostFxManager::saveDebugScreenshot(const char* filename, uint32_t address,
                                        uint32_t width, uint32_t height,
                                        uint32_t psm) {
  if (!g_debug_menu.fogTriggerScreenshot) return;

  TYRA_LOG("Saving fog debug screenshot: ", filename);
  ps2_screenshot_file(filename, address >> 6, width, height, psm);
}
#endif

PostFxManager::PostFxManager(Renderer* renderer)
    : Singleton<PostFxManager>(), settings(renderer->core.getSettings()) {
  pRenderer = renderer;

  // Inicializar valores de CLIP_ZVALUE
  // fogStartPercent=0.5 = fog começa na metade da distância visível
  // Objetos mais próximos que 50% do far plane ficam nítidos (sem fog)
  DEFAULT_CLIP_ZVALUE = calculateClipZValue(0.1f, 128.0f, 0.5f);
  CLIP_ZVALUE = DEFAULT_CLIP_ZVALUE;
  
#ifdef DEBUG_MODE
  TYRA_LOG("[FOG INIT] DEFAULT_CLIP_ZVALUE = 0x", std::hex, DEFAULT_CLIP_ZVALUE, std::dec);
#endif
};

PostFxManager::~PostFxManager() {};

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
  //    - Usa 16-bit mode (PSMCT16S) para channel shuffle
  //    - Copia em strips de 8 pixels com offset +8 para ler canal G
  // 4. Downsample framebuffer para workbuffer (width/2 × height/2)
  // 5. Draw workbuffer com alpha blending (profundidade como máscara)
  // 6. Aplicar cor fog e saturação

  const uint32_t width = settings.getWidth();
  const uint32_t height = settings.getHeight();
  const uint32_t halfWidth = width / 2;
  const uint32_t halfHeight = height / 2;
  int tw, th;
  setTwTh(width, height, &tw, &th);

  const zbuffer_t zbuffer = pRenderer->core.gs.zBuffer;
  const framebuffer_t buf_frame = pRenderer->core.gs.getCurrentFrameData();

  const uint32_t zbufferAddr = zbuffer.address;
  const uint32_t zbufferPsm = zbuffer.zsm;

  qword_t packets[200] ALIGNED(64);
  qword_t* q = packets;

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
    saveDebugScreenshot("host:debug/fog_pass_setup.tga", buf_frame.address,
                        width, height, buf_frame.psm);
  }
#endif

  // 1st pass: invert z-buffer values
#ifdef DEBUG_MODE
  if (g_debug_menu.fogPass1) {
    saveDebugScreenshot("host:debug/fog_pass1_before_invert_z.tga", zbufferAddr,
                        width, height, zbufferPsm);
#endif
    q = packets;
    PACK_GIFTAG(q, GIF_SET_TAG(6, 1, 0, 0, GIF_FLG_PACKED, 1), GIF_REG_AD);
    q++;

    // Escrever no Z-buffer, proteger apenas o canal alpha (0xFF000000)
    PACK_GIFTAG(
        q, GS_SET_FRAME(zbufferAddr >> 11, width >> 6, zbufferPsm, 0xFF000000),
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
    PACK_GIFTAG(q, GS_SET_XYZ(ftoi4(width), ftoi4(height), 0), GS_REG_XYZ2);
    q++;
    FlushCache(0);
    dma_channel_send_normal(DMA_CHANNEL_GIF, packets, q - packets, 0, 0);
    dma_channel_wait(DMA_CHANNEL_GIF, 500);

#ifdef DEBUG_MODE
    saveDebugScreenshot("host:debug/fog_pass1_invert_z.tga", zbufferAddr, width,
                        height, zbufferPsm);
  }
#endif

  // PASS 2: Clipar objetos próximos
#ifdef DEBUG_MODE
  if (g_debug_menu.fogPass2) {
    TYRA_LOG("[FOG PASS 2] CLIP_ZVALUE = 0x", std::hex, CLIP_ZVALUE, std::dec);
#endif
    q = packets;
    // Aumentado para 9 pacotes para incluir GS_SET_FRAME
    PACK_GIFTAG(q, GIF_SET_TAG(9, 1, 0, 0, GIF_FLG_PACKED, 1), GIF_REG_AD);
    q++;

    // Mascarar escrita de cor (0xFFFFFFFF) para garantir que apenas Z seja
    // alterado Isso substitui o uso de AFAIL=2 que pode ser instável
    PACK_GIFTAG(
        q, GS_SET_FRAME(zbufferAddr >> 11, width >> 6, zbufferPsm, 0xFFFFFFFF),
        GS_REG_FRAME_1);
    q++;

    // Z-Test GREATER: Se CLIP_ZVALUE > Z_Buffer, atualiza Z
    // Alpha Test desabilitado
    PACK_GIFTAG(q, GS_SET_TEST(0, 0, 0, 0, 0, 0, 1, ZTEST_METHOD_GREATER),
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
    saveDebugScreenshot("host:debug/fog_pass2_clip_z.tga", zbufferAddr, width,
                        height, zbufferPsm);
  }
#endif

  // ===== PASS 3: Copiar canal G do Z-buffer para canal Alpha do framebuffer
  // ===== usando channel shuffle (16-bit mode) conforme técnica da Sony
#ifdef DEBUG_MODE
  if (g_debug_menu.fogPass3) {
#endif

    // IMPORTANTE: Restaurar estado consistente do GS antes do channel shuffle
    // Isso garante funcionamento correto mesmo se PASS 1/2 forem desabilitados
    q = packets;
    PACK_GIFTAG(q, GIF_SET_TAG(3, 1, 0, 0, GIF_FLG_PACKED, 1), GIF_REG_AD);
    q++;

    // Restaurar ZBUF para read-only (mask=1)
    PACK_GIFTAG(q, GS_SET_ZBUF(zbufferAddr >> 11, zbufferPsm, 1),
                GS_REG_ZBUF_1);
    q++;

    // Restaurar TEST para all-pass
    PACK_GIFTAG(q, GS_SET_TEST(0, 0, 0, 0, 0, 0, 1, ZTEST_METHOD_ALLPASS),
                GS_REG_TEST_1);
    q++;

    // Restaurar TEXA padrão antes do channel shuffle
    PACK_GIFTAG(q, GS_SET_TEXA(0, 0, 128), GS_REG_TEXA);
    q++;

    FlushCache(0);
    dma_channel_send_normal(DMA_CHANNEL_GIF, packets, q - packets, 0, 0);
    dma_channel_wait(DMA_CHANNEL_GIF, 500);

    q = packets;
    PACK_GIFTAG(q, GIF_SET_TAG(8, 1, 0, 0, GIF_FLG_PACKED, 1), GIF_REG_AD);
    q++;

    // Configurar Frame como 16-bit para o channel shuffle
    // Máscara 0x00003fff conforme código original Sony (protege 14 bits
    // inferiores) Width >> 5 para 1024 pixels (correto para 16-bit alias de 512
    // 32-bit)
    PACK_GIFTAG(q,
                GS_SET_FRAME(buf_frame.address >> 11, width >> 5, GS_PSM_16S,
                             0x00003fff),
                GS_REG_FRAME_1);
    q++;

    PACK_GIFTAG(q, GS_SET_TEXFLUSH(0), GS_REG_TEXFLUSH);
    q++;

    // Textura de origem (Z-Buffer) lida como 16-bit
    // TW=10, TH=10 (1024x1024) fixos para channel shuffle, como no código
    // original Width >> 5 para 1024 pixels
    PACK_GIFTAG(q,
                GS_SET_TEX0(zbufferAddr >> 6, width >> 5, GS_PSM_16S, 10, 10, 1,
                            1, 0, 0, 0, 0, 0),
                GS_REG_TEX0_1);
    q++;

    // TEX1 com NEAREST/NEAREST para evitar qualquer filtragem (equivalente ao
    // fog.txt)
    PACK_GIFTAG(q, GS_SET_TEX1(0, 0, 0, 0, 0, 0, 0), GS_REG_TEX1_1);
    q++;

    // TEXA conforme original Sony
    PACK_GIFTAG(q, GS_SET_TEXA(0, 0, 0), GS_REG_TEXA);
    q++;

    // Alpha blend: (Cs - 0) * FIX + 0
    // FIX=64 significa multiplicar por 64/128 = 0.5
    // Isso controla a intensidade da cópia do canal
    PACK_GIFTAG(q, GS_SET_ALPHA(0, 2, 2, 2, 64), GS_REG_ALPHA_1);
    q++;

    // Scissor: largura DOBRADA, altura normal
    PACK_GIFTAG(q, GS_SET_SCISSOR(0, (width * 2) - 1, 0, height - 1),
                GS_REG_SCISSOR_1);
    q++;

    PACK_GIFTAG(q, GS_SET_PRIM(GS_PRIM_SPRITE, 0, 1, 0, 1, 0, 1, 0, 0),
                GS_REG_PRIM);
    q++;

    FlushCache(0);
    dma_channel_send_normal(DMA_CHANNEL_GIF, packets, q - packets, 0, 0);
    dma_channel_wait(DMA_CHANNEL_GIF, 500);

    // Desenhar strips conforme código original
    // Para 512px (32-bit) -> 1024px (16-bit)
    // Strips de 8 pixels
    int strips = (width * 2) >> 3;  // 1024 / 8 = 128 strips

    for (int i = 0; i < strips; i++) {
      q = packets;

      PACK_GIFTAG(
          q,
          GIF_SET_TAG(1, 1, GS_SET_PRIM(GS_PRIM_SPRITE, 0, 1, 0, 1, 0, 1, 0, 0),
                      0, GIF_FLG_REGLIST, 4),
          (GIF_REG_UV) | (GIF_REG_XYZ2 << 4) | (GIF_REG_UV << 8) |
              (GIF_REG_XYZ2 << 12));
      q++;

      // Channel Shuffle G->A (Shift de 8 pixels)
      // X: Posição atual do strip (i * 8)
      // U: Posição atual + 8 pixels (Shift) + 0.5 (Half-texel)

      // Vértice 1 (Top-Left)
      // U = (i * 8) + 8 + 0.5 -> (i << 7) + (8 << 4) + 8
      // X = (i * 8)           -> (i << 7)
      q->dw[0] = GS_SET_UV((8 << 4) + (i << 7) + 8, 8);
      q->dw[1] = GS_SET_XYZ((i << 7), (0 << 4), 0);
      q++;

      // Vértice 2 (Bottom-Right)
      // U = U_start + 8 -> (i << 7) + (8 << 4) + 8 + (8 << 4)
      // X = X_start + 8 -> (i << 7) + (8 << 4)
      q->dw[0] =
          GS_SET_UV((8 << 4) + (i << 7) + 8 + (8 << 4), 8 + (height << 4));
      q->dw[1] = GS_SET_XYZ((i << 7) + (8 << 4), (height << 4), 0);
      q++;

      FlushCache(0);
      dma_channel_send_normal(DMA_CHANNEL_GIF, packets, q - packets, 0, 0);
      dma_channel_wait(DMA_CHANNEL_GIF, 500);
    }

    // Restaurar TEXA e Scissor
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
    saveDebugScreenshot("host:debug/fog_pass3_channel_copy.tga",
                        buf_frame.address, width, height, buf_frame.psm);
  }
#endif

  // 4th pass: downsample framebuffer to workbuffer
#ifdef DEBUG_MODE
  if (g_debug_menu.fogPass4) {
#endif
    // Restaurar estado antes do downsample
    q = packets;
    PACK_GIFTAG(q, GIF_SET_TAG(2, 1, 0, 0, GIF_FLG_PACKED, 1), GIF_REG_AD);
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

    q = packets;
    PACK_GIFTAG(q, GIF_SET_TAG(9, 1, 0, 0, GIF_FLG_PACKED, 1), GIF_REG_AD);
    q++;

    PACK_GIFTAG(
        q, GS_SET_FRAME(zbufferAddr >> 11, width >> 6, zbufferPsm, 0xFF000000),
        GS_REG_FRAME_1);
    q++;

    PACK_GIFTAG(q, GS_SET_TEXFLUSH(0), GS_REG_TEXFLUSH);
    q++;

    // TEX0: TW=10, TH=10 (1024x1024) conforme original, não calculado
    // dinamicamente
    PACK_GIFTAG(q,
                GS_SET_TEX0(buf_frame.address >> 6, width >> 6, buf_frame.psm,
                            10, 10, 0, 1, 0, 0, 0, 0, 0),
                GS_REG_TEX0_1);
    q++;
    PACK_GIFTAG(q, GS_SET_TEX1(1, 0, 1, 1, 0, 0, 0), GS_REG_TEX1_1);
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
    saveDebugScreenshot("host:debug/fog_pass4_downsample.tga", zbufferAddr,
                        halfWidth, halfHeight, zbufferPsm);
  }
#endif

  // 5th pass x2: draw workbuffer onto framebuffer with dest alpha (z-buf
  // channel)
#ifdef DEBUG_MODE
  if (g_debug_menu.fogPass5) {
#endif
    // Restaurar estado antes do blend
    q = packets;
    PACK_GIFTAG(q, GIF_SET_TAG(3, 1, 0, 0, GIF_FLG_PACKED, 1), GIF_REG_AD);
    q++;

    PACK_GIFTAG(q, GS_SET_TEST(0, 0, 0, 0, 0, 0, 1, ZTEST_METHOD_ALLPASS),
                GS_REG_TEST_1);
    q++;

    PACK_GIFTAG(q, GS_SET_ZBUF(zbufferAddr >> 11, zbufferPsm, 1),
                GS_REG_ZBUF_1);
    q++;

    PACK_GIFTAG(q, GS_SET_SCISSOR(0, width - 1, 0, height - 1),
                GS_REG_SCISSOR_1);
    q++;

    FlushCache(0);
    dma_channel_send_normal(DMA_CHANNEL_GIF, packets, q - packets, 0, 0);
    dma_channel_wait(DMA_CHANNEL_GIF, 500);

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

    // Textura do workbuffer: tw=10, th=10 conforme original (não
    // halfWidth/halfHeight!)
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

    const int32_t offset = 16;  // Offset conforme código original (não 8!)

    // UV: Mapear do início (1,1) até o fim do buffer de trabalho (halfWidth,
    // halfHeight) Primeiro sprite: leve offset para blend suave
    PACK_GIFTAG(q, GS_SET_UV(ftoi4(1), ftoi4(1)), GS_REG_UV);
    q++;

    // XYZ: Desenhar na tela inteira (1,1 até width+1,height+1) com offset
    PACK_GIFTAG(q, GS_SET_XYZ(ftoi4(1) + offset, ftoi4(1) + offset, 0),
                GS_REG_XYZ2);
    q++;

    PACK_GIFTAG(q, GS_SET_UV(ftoi4(halfWidth), ftoi4(halfHeight)), GS_REG_UV);
    q++;

    PACK_GIFTAG(
        q, GS_SET_XYZ(ftoi4(width + 1) + offset, ftoi4(height + 1) + offset, 0),
        GS_REG_XYZ2);
    q++;

    // Segundo sprite para blend adicional
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
    saveDebugScreenshot("host:debug/fog_pass5_blend_alpha.tga",
                        buf_frame.address, width, height, buf_frame.psm);
  }
#endif

#ifdef DEBUG_MODE
  if (g_debug_menu.fogPass6) {
#endif
    if (fogColor.a > 0) {
      q = packets;
      PACK_GIFTAG(q, GIF_SET_TAG(11, 1, 0, 0, GIF_FLG_PACKED, 1), GIF_REG_AD);
      q++;

      // Ensure pixel test is all-pass before saturation/fog
      PACK_GIFTAG(q, GS_SET_TEST(0, 0, 0, 0, 0, 0, 1, ZTEST_METHOD_ALLPASS),
                  GS_REG_TEST_1);
      q++;

      PACK_GIFTAG(q,
                  GS_SET_FRAME(buf_frame.address >> 11, width >> 6,
                               buf_frame.psm, 0x00000000),
                  GS_REG_FRAME_1);
      q++;

      // 6th pass: add saturation
      PACK_GIFTAG(q, GS_SET_TEXFLUSH(0), GS_REG_TEXFLUSH);
      q++;

      // TEX0: tw=10, th=10 conforme original
      PACK_GIFTAG(q,
                  GS_SET_TEX0(buf_frame.address >> 6, width >> 6, buf_frame.psm,
                              10, 10, 1, 1, 0, 0, 0, 0, 0),
                  GS_REG_TEX0_1);
      q++;

      // Use linear filtering to reduce feedback artifacts during saturation
      PACK_GIFTAG(q, GS_SET_TEX1(1, 0, 1, 1, 0, 0, 0), GS_REG_TEX1_1);
      q++;

      PACK_GIFTAG(q, GS_SET_PRIM(GS_PRIM_SPRITE, 0, 1, 0, 1, 0, 1, 0, 0),
                  GS_REG_PRIM);
      q++;

      // Blend: (Cs - 0) * Ad + Cd (fog.alpha controla intensidade)
      PACK_GIFTAG(q, GS_SET_ALPHA(0, 2, 2, 1, (int)fogColor.a), GS_REG_ALPHA_1);
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
      PACK_GIFTAG(q, GIF_SET_TAG(5, 1, 0, 0, GIF_FLG_PACKED, 1), GIF_REG_AD);
      q++;

      // Blend: (Cs - 0) * Ad + Cd
      // Cs = cor fog, Ad = alpha (máscara profundidade), Cd = cor tela
      PACK_GIFTAG(q, GS_SET_ALPHA(0, 2, 1, 1, 0), GS_REG_ALPHA_1);
      q++;

      PACK_GIFTAG(q, GS_SET_PRIM(GS_PRIM_SPRITE, 0, 0, 0, 1, 0, 1, 0, 0),
                  GS_REG_PRIM);
      q++;

      // Definir cor do fog novamente (necessário após saturation pass)
      PACK_GIFTAG(
          q,
          GS_SET_RGBAQ((int)fogColor.r, (int)fogColor.g, (int)fogColor.b, 0, 0),
          GS_REG_RGBAQ);
      q++;

      PACK_GIFTAG(q, GS_SET_XYZ(ftoi4(0), ftoi4(0), 0), GS_REG_XYZ2);
      q++;

      PACK_GIFTAG(q, GS_SET_XYZ(ftoi4(width), ftoi4(height), 0), GS_REG_XYZ2);
      q++;

      FlushCache(0);
      dma_channel_send_normal(DMA_CHANNEL_GIF, packets, q - packets, 0, 0);
      dma_channel_wait(DMA_CHANNEL_GIF, 500);
    }

#ifdef DEBUG_MODE
    saveDebugScreenshot("host:debug/fog_pass6_final.tga", buf_frame.address,
                        width, height, buf_frame.psm);

    // Reset trigger após capturar todos os passos
    if (g_debug_menu.fogTriggerScreenshot) {
      g_debug_menu.fogTriggerScreenshot = false;
      TYRA_LOG("Fog screenshots capture completed. Trigger reset.");
    }
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
