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

  // Inicializar CLIP_ZVALUE com valor padrão
  CLIP_ZVALUE = DEFAULT_CLIP_ZVALUE;

  // Upload CLUT de identidade para uso no channel copy
  uploadIdentityCLUT();
}

PostFxManager::~PostFxManager() {
  // CLUT é alocada em posição fixa da VRAM, não requer cleanup manual
  // (VRAM é gerenciada pelo GS e reinicializada a cada boot)
}

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

// Upload CLUT de identidade para VRAM usando draw_texture_transfer do PS2SDK.
// A paleta de identidade é uma textura 16x16 (256 pixels) em formato 32-bit,
// onde cada pixel N tem a cor (N, N, N, N) em RGBA. Isso permite que a
// textura 8-bit funcione como lookup direto de bytes.
//
// Baseado em channel_copy.cpp - usa abordagem simples que funciona porque
// draw_texture_transfer lida com o swizzle CSM1 internamente.
void PostFxManager::uploadIdentityCLUT() {
  // Dimensões da paleta: 16x16 = 256 cores
  constexpr int CLUT_WIDTH = 16;
  constexpr int CLUT_HEIGHT = 16;
  constexpr int CLUT_SIZE = CLUT_WIDTH * CLUT_HEIGHT;

  // Alocar dados da paleta de identidade (alinhado a 64 bytes para DMA)
  // Formato PS2 RGBA: R=bits 0-7, G=bits 8-15, B=bits 16-23, A=bits 24-31
  uint32_t* clutData =
      (uint32_t*)aligned_alloc(64, CLUT_SIZE * sizeof(uint32_t));

  // Criar paleta de identidade ESCALADA para range 0-128 do PS2 GS.
  // O PS2 GS trata alpha 128 como 1.0 (opaco) nos blend formulas.
  // Valores > 128 causam overflow em fórmulas como ((Cd-Cs)*Ad)/128 + Cs.
  // Portanto, escalamos cada byte: out = min(i/2, 128)
  // Isso mapeia Z-buffer green byte [0..255] → alpha [0..127]
  // (128 = fully opaque fog, 0 = no fog)
  for (int i = 0; i < CLUT_SIZE; i++) {
    uint8_t scaled = (uint8_t)(i >> 1);  // i/2, max 127
    clutData[i] = ((uint32_t)scaled << 24) | ((uint32_t)scaled << 16) |
                  ((uint32_t)scaled << 8) | (uint32_t)scaled;
  }

  FlushCache(0);

  // Calcular endereço da CLUT após o zbuffer e temp buffer
  // zbuffer: width × height × 4 bytes
  // temp buffer: 64 × 32 × 4 = 8192 bytes
  // Total em words: (512*448*4 + 8192) / 4 = 231936 words após início do
  // zbuffer
  //
  // Vamos usar um endereço fixo no final da VRAM para a CLUT (4MB - 4KB)
  // Endereço em words: (4*1024*1024 - 4096) / 4 = 1047552 words
  // Em 256-byte units: 1047552 >> 6 = 16368
  //
  // Alternativa mais segura: usar o espaço após o framebuffer duplo e zbuffer
  // PS2 VRAM = 4MB = 4194304 bytes
  // Framebuffer 512x448x4x2 (duplo) = 1835008 bytes @ endereço 0
  // Zbuffer 512x448x4 = 917504 bytes @ endereço ~1835008
  // Total usado ≈ 2752512 bytes
  // CLUT 16x16x4 = 1024 bytes
  //
  // Vamos alocar no final da VRAM menos alguns KB para margem de segurança
  // Endereço em WORD units (4 bytes) — compatível com ps2sdk:
  // - draw_texture_transfer espera words (faz dest>>6 internamente para DBP)
  // - graph_vram_allocate retorna words
  // - framebuffer_t.address e zbuffer_t.address são em words
  // Para TEX0 CBP: clutVramAddress >> 6 converte para 256-byte units
  //
  // 4MB = 4194304 bytes. 4MB - 8KB = 4186112 bytes.
  // 4186112 / 4 = 1046528 (em word units)
  constexpr uint32_t CLUT_VRAM_ADDR_WORDS = (4 * 1024 * 1024 - 8 * 1024) / 4;
  clutVramAddress = CLUT_VRAM_ADDR_WORDS;

  // Preparar pacote DMA para upload da CLUT
  // Usar draw_texture_transfer que lida com swizzle CSM1 automaticamente
  // Nota: dest_width = 256 como em channel_copy.cpp (buffer width de 256
  // pixels para 16x16 CLUT em PSM_32, equivalente a row alignment)
  qword_t packets[30] ALIGNED(64);
  qword_t* q = packets;

  q = draw_texture_transfer(q, clutData, CLUT_WIDTH, CLUT_HEIGHT, GS_PSM_32,
                            clutVramAddress, 256);
  q = draw_texture_flush(q);

  // Enviar via DMA
  dma_channel_send_chain(DMA_CHANNEL_GIF, packets, q - packets, 0, 0);
  dma_channel_wait(DMA_CHANNEL_GIF, 500);

  // Liberar memória temporária
  free(clutData);

#ifdef DEBUG_MODE
  TYRA_LOG("[FOG] Identity CLUT created and uploaded to VRAM");
  TYRA_LOG("[FOG] CLUT VRAM address (words): ", clutVramAddress);
  TYRA_LOG("[FOG] CLUT VRAM address (bytes): ", clutVramAddress * 4);
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

  // ZBUF_1: Z-buffer writes DESABILITADAS (mask=1 = não escreve)
  // Passes subsequentes controlam escrita via FRAME target
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
  // Técnica baseada no original (DepthOfFieldWithZFog.cpp):
  // - FRAME target = Z-buffer, com mask 0xFF000000 para proteger byte alto
  // - ALPHA blend (0,1,2,2,128) = ((Cs - Cd) * FIX) >> 7 + D
  //   Cs=255 (RGBAQ), Cd=Z_original, FIX=128 → resultado ≈ 255 - Z_original
  // - Sprite full-screen com Z=0, alpha blending habilitado

#ifdef DEBUG_MODE
  if (g_debug_menu.fogPass1) {
    saveDebugScreenshot("host:debug/fog_pass1_before_invert_z.tga",
                        fog_zbufferAddr, fog_width, fog_height, fog_zbufferPsm);
#endif

    {
      qword_t packets[100] ALIGNED(64);
      qword_t* q = packets;

      // 6 registradores A+D: FRAME, ALPHA, PRIM, RGBAQ, XYZ2, XYZ2
      PACK_GIFTAG(q, GIF_SET_TAG(6, 1, 0, 0, GIF_FLG_PACKED, 1), GIF_REG_AD);
      q++;

      // FRAME_1: Z-buffer como destino de renderização
      // PSM do zbuffer, mask 0xFF000000 protege o byte mais alto (alpha)
      // FBW = largura em unidades de 64 pixels
      PACK_GIFTAG(q,
                  GS_SET_FRAME(fog_zbufferAddr >> 11, fog_width >> 6,
                               fog_zbufferPsm, 0xFF000000),
                  GS_REG_FRAME_1);
      q++;

      // ALPHA_1: A=0(Cs), B=1(Cd), C=2(FIX), D=2(0), FIX=128
      // Formula: ((Cs - Cd) * 128) >> 7 + 0 = Cs - Cd
      // Com Cs=0xFF: resultado = 0xFF - Z_original (inversão)
      PACK_GIFTAG(q, GS_SET_ALPHA(0, 1, 2, 2, 128), GS_REG_ALPHA_1);
      q++;

      // PRIM: Sprite, ABE=1 (alpha blend enabled)
      PACK_GIFTAG(q, GS_SET_PRIM(GS_PRIM_SPRITE, 0, 0, 0, 1, 0, 0, 0, 0),
                  GS_REG_PRIM);
      q++;

      // RGBAQ: R=255, G=255, B=255, A=128 (Cs = 0xFF por canal)
      PACK_GIFTAG(q, GS_SET_RGBAQ(255, 255, 255, 128, 0), GS_REG_RGBAQ);
      q++;

      // XYZ2 inicio: (0, 0, Z=0)
      PACK_GIFTAG(q, GS_SET_XYZ(ftoi4(0), ftoi4(0), 0), GIF_REG_XYZ2);
      q++;

      // XYZ2 fim: (width, height, Z=0) — drawing kick
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
  // Baseado no original (DepthOfFieldWithZFog.cpp):
  // - TEST: ZTEST_GREATER + ATE=1 (alpha test habilitado para funcionar)
  // - ZBUF: mask=0 (writes habilitadas) — escrita real no zbuffer
  // - Sprite full-screen com Z=CLIP_ZVALUE e RGBA=0
  // - Onde CLIP_ZVALUE > Z_invertido → sobrescreve com 0 (clippa)
  // - Depois restaura TEST=ALLPASS e ZBUF mask=1

#ifdef DEBUG_MODE
  if (g_debug_menu.fogPass2) {
#endif

    qword_t packets[100] ALIGNED(64);
    qword_t* q = packets;

    // 8 registradores: TEST, ZBUF, PRIM, RGBAQ, XYZ2, XYZ2, TEST, ZBUF
    PACK_GIFTAG(q, GIF_SET_TAG(8, 1, 0, 0, GIF_FLG_PACKED, 1), GIF_REG_AD);
    q++;

    // TEST_1: ATE=1, ATST=0(NEVER), AREF=0, AFAIL=2(ZBONLY),
    //         DATE=0, DATM=0, ZTE=1, ZTST=GREATER
    // Alpha test NEVER para não escrever no frame, AFAIL=ZBONLY para
    // só escrever no zbuffer. Isso replica o comportamento do original
    // onde PRIM não tem ABE e cor é irrelevante.
    PACK_GIFTAG(q, GS_SET_TEST(1, 0, 0, 2, 0, 0, 1, ZTEST_METHOD_GREATER),
                GS_REG_TEST_1);
    q++;

    // ZBUF_1: Z-buffer writes habilitadas (mask=0)
    PACK_GIFTAG(q, GS_SET_ZBUF(fog_zbufferAddr >> 11, fog_zbufferPsm, 0),
                GS_REG_ZBUF_1);
    q++;

    // PRIM: Sprite simples, sem textura, sem blend
    PACK_GIFTAG(q, GS_SET_PRIM(GS_PRIM_SPRITE, 0, 0, 0, 0, 0, 0, 0, 0),
                GS_REG_PRIM);
    q++;

    // RGBAQ: Cor preta (irrelevante, AFAIL=ZBONLY)
    PACK_GIFTAG(q, GS_SET_RGBAQ(0, 0, 0, 0, 0), GS_REG_RGBAQ);
    q++;

    // XYZ2 start: (0, 0, CLIP_ZVALUE)
    PACK_GIFTAG(q, GS_SET_XYZ(ftoi4(0), ftoi4(0), CLIP_ZVALUE), GIF_REG_XYZ2);
    q++;

    // XYZ2 end: (width, height, CLIP_ZVALUE) — drawing kick
    PACK_GIFTAG(q, GS_SET_XYZ(ftoi4(fog_width), ftoi4(fog_height), CLIP_ZVALUE),
                GIF_REG_XYZ2);
    q++;

    // Restaurar: TEST=ALLPASS
    PACK_GIFTAG(q, GS_SET_TEST(0, 0, 0, 0, 0, 0, 1, ZTEST_METHOD_ALLPASS),
                GS_REG_TEST_1);
    q++;

    // Restaurar: ZBUF mask=1 (writes desabilitadas)
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

  qword_t packets[500] ALIGNED(64);
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
  // - The PSMCT16S trick CANNOT be used with PSMZ_32 zbuffer because
  //   PSMZ_32 has reversed column order within blocks compared to PSMCT
  //   formats. Reading PSMZ_32 VRAM as a PSMCT16S texture produces garbled
  //   output.
  //
  // Solution: Use BITBLTBUF local-to-local transfer (PSMZ_32 → PSM_32)
  // in 64x32 blocks to resolve the column swizzle, then channel shuffle
  // with PSM_8 + identity CLUT to extract the Green channel byte and
  // write it into the framebuffer Alpha channel.
  //
  // This matches the GTA SA PostFX approach:
  //   CSkyEdgePostEffects_CopyDepthBuffer(CHANNEL_BLUE, table)
  // But we use CHANNEL_GREEN for 32-bit zbuffer as per Sony doc
  // (bits 15-8 = Green, best mapping for fog distance).

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

void PostFxManager::fogPass4Downsample() {
  // ===== PASS 4: DOWNSAMPLE =====
  // Técnica original de DepthOfFieldWithZFog.cpp:
  // - FRAME: zbuffer como destino, PSM do framebuffer, mask=0xFF000000
  // - TEX0: framebuffer como textura, TCC=0, TFX=DECAL
  // - TEX1: LINEAR filter
  // - PRIM: sprite com textura, SEM ABE
  // - UV: (0,0) to (width, height)
  // - XYZ: (0,0) to (width/2, height/2)

#ifdef DEBUG_MODE
  if (g_debug_menu.fogPass4) {
#endif
    qword_t packets[200] ALIGNED(64);
    qword_t* q = packets;

    // Configuração: 6 registradores (EOP=0, continua com sprite data)
    // Inclui XYOFFSET(0,0) para garantir coordenadas corretas após
    // copyDepthBuffer que pode ter alterado o offset
    PACK_GIFTAG(q, GIF_SET_TAG(6, 0, 0, 0, GIF_FLG_PACKED, 1), GIF_REG_AD);
    q++;

    // XYOFFSET: (0,0) — ESSENCIAL: resetar após copyDepthBuffer
    PACK_GIFTAG(q, GS_SET_XYOFFSET(0, 0), GS_REG_XYOFFSET_1);
    q++;

    // FRAME: Z-buffer como destino (workbuffer)
    // mask=0xFF000000 protege o byte alpha (bits 24-31)
    PACK_GIFTAG(q,
                GS_SET_FRAME(fog_zbufferAddr >> 11, fog_width >> 6,
                             fog_buf_frame.psm, 0xFF000000),
                GS_REG_FRAME_1);
    q++;

    // TEXFLUSH
    PACK_GIFTAG(q, GS_SET_TEXFLUSH(1), GS_REG_TEXFLUSH);
    q++;

    // TEX0: Framebuffer como textura source
    // TCC=0 (ignora alpha da textura), TFX=DECAL
    PACK_GIFTAG(q,
                GS_SET_TEX0(fog_buf_frame.address >> 6, fog_width >> 6,
                            fog_buf_frame.psm, 10, 10, 0, 1, 0, 0, 0, 0, 0),
                GS_REG_TEX0_1);
    q++;

    // TEX1: LINEAR filter (MMAG=1, MMIN=1)
    PACK_GIFTAG(q, GS_SET_TEX1(0, 0, 1, 1, 0, 0, 0), GS_REG_TEX1_1);
    q++;

    // PRIM: Sprite com textura, SEM ABE, FST=1
    PACK_GIFTAG(q, GS_SET_PRIM(GS_PRIM_SPRITE, 0, 1, 0, 0, 0, 1, 0, 0),
                GS_REG_PRIM);
    q++;

    // Sprite data em modo A+D: UV + XYZ2 × 2 = 4 regs
    PACK_GIFTAG(q, GIF_SET_TAG(4, 1, 0, 0, GIF_FLG_PACKED, 1), GIF_REG_AD);
    q++;

    // UV start: (0, 0) com half-texel offset
    PACK_GIFTAG(q, GS_SET_UV(8, 8), GS_REG_UV);
    q++;
    // XYZ2 start: (0, 0)
    PACK_GIFTAG(q, GS_SET_XYZ(ftoi4(0), ftoi4(0), 0), GS_REG_XYZ2);
    q++;

    // UV end: (width, height)
    PACK_GIFTAG(q, GS_SET_UV(8 + (fog_width << 4), 8 + (fog_height << 4)),
                GS_REG_UV);
    q++;
    // XYZ2 end: (halfWidth, halfHeight) → downsample 2x
    PACK_GIFTAG(q, GS_SET_XYZ(ftoi4(fog_halfWidth), ftoi4(fog_halfHeight), 0),
                GS_REG_XYZ2);
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

    // Pass 5 original faz dois sprites com offset de sub-pixel para
    // criar blur. Basicamente desenha o workbuffer (metade) escalado
    // de volta ao framebuffer com alpha blending (depth mask).
    //
    // Original: ALPHA(1,0,1,0,0) = ((Cd - Cs) * Ad) >> 7 + Cs
    // = Cs + (Cd - Cs) * Ad = lerp entre Cs (workbuffer) e Cd (framebuffer)
    // controlado por Ad (alpha do destino = depth mask)

    // Configuração: FRAME + TEXFLUSH + TEX0 + ALPHA + PRIM (5 regs, EOP=0)
    PACK_GIFTAG(q, GIF_SET_TAG(5, 0, 0, 0, GIF_FLG_PACKED, 1), GIF_REG_AD);
    q++;

    // FRAME: Framebuffer original como destino
    // mask=0xFF000000 para proteger alpha (depth mask)
    PACK_GIFTAG(
        q,
        GS_SET_FRAME(fog_buf_frame.address >> 11, fog_buf_frame.width >> 6,
                     fog_buf_frame.psm, 0xFF000000),
        GS_REG_FRAME_1);
    q++;

    // TEXFLUSH
    PACK_GIFTAG(q, GS_SET_TEXFLUSH(1), GS_REG_TEXFLUSH);
    q++;

    // TEX0: Workbuffer (Z-buffer) como textura source
    // TCC=1 (usa alpha da textura), TFNCT=DECAL
    PACK_GIFTAG(q,
                GS_SET_TEX0(fog_zbufferAddr >> 6, fog_width >> 6,
                            fog_buf_frame.psm, 10, 10, 1, 1, 0, 0, 0, 0, 0),
                GS_REG_TEX0_1);
    q++;

    // ALPHA: (1,0,1,0,0) = ((Cd - Cs) * Ad) >> 7 + Cs
    // Blend baseado no alpha do destino (framebuffer)
    PACK_GIFTAG(q, GS_SET_ALPHA(1, 0, 1, 0, 0), GS_REG_ALPHA_1);
    q++;

    // PRIM: Sprite com textura, ABE=1 (alpha blend), FST=1 (UV)
    PACK_GIFTAG(q, GS_SET_PRIM(GS_PRIM_SPRITE, 0, 1, 0, 1, 0, 1, 0, 0),
                GS_REG_PRIM);
    q++;

    // Offset de 16 sub-pixels (1 pixel) para blur, como no original
    int32_t offset = 16;

    // Sprite 1: Com offset positivo — modo A+D (4 regs, EOP=0)
    PACK_GIFTAG(q, GIF_SET_TAG(4, 0, 0, 0, GIF_FLG_PACKED, 1), GIF_REG_AD);
    q++;

    // UV start: (1, 1) com half-texel offset
    PACK_GIFTAG(q, GS_SET_UV(8 + (1 << 4), 8 + (1 << 4)), GS_REG_UV);
    q++;
    // XYZ2 start: (1+offset, 1+offset)
    PACK_GIFTAG(q, GS_SET_XYZ(ftoi4(1) + offset, ftoi4(1) + offset, 0),
                GS_REG_XYZ2);
    q++;

    // UV end: (halfWidth, halfHeight)
    PACK_GIFTAG(q,
                GS_SET_UV(8 + (fog_halfWidth << 4), 8 + (fog_halfHeight << 4)),
                GS_REG_UV);
    q++;
    // XYZ2 end: (width+1+offset, height+1+offset)
    PACK_GIFTAG(
        q,
        GS_SET_XYZ(ftoi4(fog_width + 1) + offset,
                   ftoi4(fog_height + 1) + offset, 0),
        GS_REG_XYZ2);
    q++;

    // Sprite 2: Com offset negativo (blur oposto) — modo A+D (4 regs, EOP=1)
    PACK_GIFTAG(q, GIF_SET_TAG(4, 1, 0, 0, GIF_FLG_PACKED, 1), GIF_REG_AD);
    q++;

    // UV start: (0+offset, 0+offset)
    PACK_GIFTAG(q, GS_SET_UV(8 + offset, 8 + offset), GS_REG_UV);
    q++;
    // XYZ2 start: (0, 0)
    PACK_GIFTAG(q, GS_SET_XYZ(ftoi4(0), ftoi4(0), 0), GS_REG_XYZ2);
    q++;

    // UV end: (halfWidth, halfHeight)
    PACK_GIFTAG(q,
                GS_SET_UV(8 + (fog_halfWidth << 4), 8 + (fog_halfHeight << 4)),
                GS_REG_UV);
    q++;
    // XYZ2 end: (width-offset, height-offset)
    PACK_GIFTAG(q,
                GS_SET_XYZ(ftoi4(fog_width) - offset,
                           ftoi4(fog_height) - offset, 0),
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
  // ===== PASS 6: APLICAR SATURAÇÃO E COR DE FOG =====
  // Técnica original de DepthOfFieldWithZFog.cpp:
  // 1. Primeiro sprite: saturação com MODULATE + ALPHA(0,2,2,1,fog.alpha)
  // 2. Segundo sprite: fog color com ALPHA(0,2,1,1,0) sem textura

  // Só executa se fog.alpha > 0
  if (fogColor.a == 0) return;

#ifdef DEBUG_MODE
  if (g_debug_menu.fogPass6) {
#endif

    qword_t packets[100] ALIGNED(64);
    qword_t* q = packets;

    // === PARTE 1: SATURAÇÃO ===
    // Configuração: RGBAQ, TEXFLUSH, TEX0, PRIM, ALPHA (5 regs, EOP=0)
    PACK_GIFTAG(q, GIF_SET_TAG(5, 0, 0, 0, GIF_FLG_PACKED, 1), GIF_REG_AD);
    q++;

    // RGBAQ: Cor do fog (usada como multiplicador com MODULATE)
    PACK_GIFTAG(
        q,
        GS_SET_RGBAQ((int)fogColor.r, (int)fogColor.g, (int)fogColor.b, 0, 0),
        GS_REG_RGBAQ);
    q++;

    // TEXFLUSH
    PACK_GIFTAG(q, GS_SET_TEXFLUSH(1), GS_REG_TEXFLUSH);
    q++;

    // TEX0: Framebuffer como textura, TFX=MODULATE (0)
    // TCC=1 (usa alpha da textura), TFX=MODULATE
    PACK_GIFTAG(q,
                GS_SET_TEX0(fog_buf_frame.address >> 6, fog_width >> 6,
                            fog_buf_frame.psm, 10, 10, 1, 0, 0, 0, 0, 0, 0),
                GS_REG_TEX0_1);
    q++;

    // PRIM: Sprite com textura, ABE=1, FST=1
    PACK_GIFTAG(q, GS_SET_PRIM(GS_PRIM_SPRITE, 0, 1, 0, 1, 0, 1, 0, 0),
                GS_REG_PRIM);
    q++;

    // ALPHA: (0,2,2,1,fog.alpha) = ((Cs-0)*fog.alpha)/128 + Cd
    // Resultado: Cd + Cs*fog.alpha/128 (adiciona saturação)
    PACK_GIFTAG(q, GS_SET_ALPHA(0, 2, 2, 1, (int)fogColor.a), GS_REG_ALPHA_1);
    q++;

    // Sprite para saturação em modo A+D: UV + XYZ2 × 2 = 4 regs (EOP=0)
    PACK_GIFTAG(q, GIF_SET_TAG(4, 0, 0, 0, GIF_FLG_PACKED, 1), GIF_REG_AD);
    q++;

    // UV start: (0, 0) com half-texel offset
    PACK_GIFTAG(q, GS_SET_UV(8, 8), GS_REG_UV);
    q++;
    // XYZ2 start: (0, 0)
    PACK_GIFTAG(q, GS_SET_XYZ(ftoi4(0), ftoi4(0), 0), GS_REG_XYZ2);
    q++;

    // UV end: (width, height)
    PACK_GIFTAG(q, GS_SET_UV(8 + (fog_width << 4), 8 + (fog_height << 4)),
                GS_REG_UV);
    q++;
    // XYZ2 end: (width, height)
    PACK_GIFTAG(q, GS_SET_XYZ(ftoi4(fog_width), ftoi4(fog_height), 0),
                GS_REG_XYZ2);
    q++;

    // === PARTE 2: FOG COLOR ===
    // ALPHA: (0,2,1,1,0) = ((Cs-0)*Ad)/128 + Cd
    // Resultado: Cd + Cs*Ad/128 (adiciona cor fog baseado no alpha do destino)
    PACK_GIFTAG(q, GIF_SET_TAG(2, 0, 0, 0, GIF_FLG_PACKED, 1), GIF_REG_AD);
    q++;

    PACK_GIFTAG(q, GS_SET_ALPHA(0, 2, 1, 1, 0), GS_REG_ALPHA_1);
    q++;

    // PRIM: Sprite sem textura, ABE=1, FST=1
    PACK_GIFTAG(q, GS_SET_PRIM(GS_PRIM_SPRITE, 0, 0, 0, 1, 0, 1, 0, 0),
                GS_REG_PRIM);
    q++;

    // Sprite para fog color em modo A+D: XYZ2 × 2 = 2 regs (EOP=1, último)
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

    qword_t packets[200] ALIGNED(64);
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
