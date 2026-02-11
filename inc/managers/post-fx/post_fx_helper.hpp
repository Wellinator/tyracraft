#pragma once
#include "tyra"
#include <stdint.h>
#include <cstdint>

/**
 * PostFxHelper - Adaptação do Posteffecthelper do sps2demo para ps2sdk.
 *
 * Gerencia registradores GS do Context 2 com dirty-flag tracking e oferece
 * operações de blit/downsample/upsample para post-processing effects.
 * Usar Context 2 evita corromper o estado de rendering normal (Context 1).
 *
 * Diferenças do sps2demo original:
 * - Usa dma_channel_send_normal() ao invés de dynDma
 * - Endereços em word units (não pages)
 * - DMA síncrono (send+wait) ao invés de chain building
 */
class PostFxHelper {
 public:
  /**
   * Construtor - inicializa estado GS para o framebuffer atual.
   * @param fbAddr_words Endereço do framebuffer em word units
   * @param screenW Largura da tela em pixels
   * @param screenH Altura da tela em pixels
   */
  PostFxHelper(uint32_t fbAddr_words, uint32_t screenW, uint32_t screenH);
  ~PostFxHelper();

  // ===== SETTERS DE REGISTRADORES GS =====

  /**
   * ALPHA register - controla alpha blending.
   * Formula: ((A - B) * C) >> 7 + D
   * @param a Source A (0=Cs, 1=Cd, 2=0)
   * @param b Source B (0=Cs, 1=Cd, 2=0)
   * @param c Alpha source (0=As, 1=Ad, 2=FIX)
   * @param d Destination (0=Cs, 1=Cd, 2=0)
   * @param fix Fixed alpha value (0-255)
   */
  void setAlpha(int a, int b, int c, int d, int fix);

  /**
   * CLAMP register - controla texture wrapping/clamping.
   */
  void setClamp(int wms, int wmt, int minu, int maxu, int minv, int maxv);

  /**
   * CLAMP shortcut - clamp to edge mode.
   */
  void setClamp(int w, int h);

  /**
   * FRAME register - configura framebuffer de destino.
   * @param fbp_words Endereço em word units
   * @param fbw_pixels Largura do buffer em pixels
   * @param psm Pixel Storage Mode (0=PSM_32)
   * @param fbmsk Write mask (0=write all channels)
   */
  void setFrame(uint32_t fbp_words, int fbw_pixels, int psm = 0,
                uint32_t fbmsk = 0);

  /**
   * RGBAQ register - define cor constante.
   */
  void setRgba(int r, int g, int b, int a);

  /**
   * SCISSOR register - define área de clipping.
   */
  void setScissor(int x0, int y0, int x1, int y1);

  /**
   * TEST register - configura alpha/depth/destination tests.
   */
  void setTest(int ate, int atst, int aref, int afail, int date, int datm,
               int zte, int ztst);

  /**
   * TEX0 register - configura textura source.
   * @param tbp_words Endereço da textura em word units
   * @param tbw_pixels Largura da textura em pixels
   * @param psm Pixel Storage Mode
   * @param tw log2(width)
   * @param th log2(height)
   * @param tcc Color component (0=RGB, 1=RGBA)
   * @param tfx Texture function (0=MODULATE, 1=DECAL)
   */
  void setTex0(uint32_t tbp_words, int tbw_pixels, int psm = 0, int tw = 9,
               int th = 9, int tcc = 1, int tfx = 0);

  /**
   * TEX1 register - configura filtro de textura.
   * @param filtering 0=nearest, 1=linear
   */
  void setFiltering(int filtering);

  /**
   * TEXFLUSH - invalida cache de textura (obrigatório após write→read).
   */
  void setTexflush();

  /**
   * XYOFFSET register - offset de coordenadas de primitivas.
   */
  void setXyoffset(int ofx, int ofy);

  // ===== OPERAÇÕES =====

  /**
   * Envia registradores modificados para o GS via DMA.
   * Apenas registradores com dirty flag são enviados.
   */
  void flushRegisters();

  /**
   * Blit - desenha sprite textured/untextured com alpha opcional.
   * @param x0,y0 Canto superior esquerdo (pixels)
   * @param x1,y1 Canto inferior direito (pixels)
   * @param alpha Habilita alpha blending
   * @param texture Habilita textura
   * @param flip Inverte UVs (para downsample)
   */
  void blit(int x0, int y0, int x1, int y1, bool alpha = false,
            bool texture = false, bool flip = false);

  /**
   * Blit com UVs customizados.
   * @param u0,v0,u1,v1 Coordenadas UV (pixels, não normalizados)
   */
  void blit(int x0, int y0, int x1, int y1, float u0, float v0, float u1,
            float v1, bool alpha = false, bool texture = true,
            bool flip = false);

  /**
   * Downsample 2x - UV sampling com offset 2x+1.
   */
  void downSample(int x0, int y0, int x1, int y1, bool alpha = false);

  /**
   * Upsample 2x - UV sampling com scale 0.5.
   */
  void upSample(int x0, int y0, int x1, int y1, bool alpha = false);

  /**
   * Desenha box (sem textura) com a cor RGBAQ atual.
   */
  void drawBox(int x0, int y0, int x1, int y1, bool alpha = false);

  /**
   * Salva o estado completo do Context 1 do GS (8 registradores).
   * Deve ser chamado ANTES de iniciar post-FX.
   */
  void saveContext1();

  /**
   * Restaura o estado completo do Context 1 do GS.
   * Deve ser chamado APÓS completar post-FX.
   */
  void restoreContext1();

  /**
   * Limpa todos os dirty flags após override manual de registradores via DmaGifBuilder.
   * Evita que flushRegisters() reenvie valores desatualizados.
   * CRÍTICO: chamar após qualquer override de TEX0/FRAME/etc fora do PostFxHelper.
   */
  void clearDirtyFlags();

 private:
  enum REGS {
    REG_ALPHA,
    REG_CLAMP,
    REG_FRAME,
    REG_RGBAQ,
    REG_SCISSOR,
    REG_TEST,
    REG_TEX0,
    REG_TEXFLUSH,
    REG_XYOFFSET,
    REG_TEX1,
    NUM_REGS
  };

  static const int regAddress[NUM_REGS];
  uint64_t registers[NUM_REGS];
  bool registerChanged[NUM_REGS];
  int textureWidth, textureHeight;
  uint32_t screenW, screenH;

  // ===== Context 1 Save/Restore =====
  struct GsContext1State {
    uint64_t frame;    // FRAME_1
    uint64_t zbuf;     // ZBUF_1
    uint64_t test;     // TEST_1
    uint64_t scissor;  // SCISSOR_1
    uint64_t tex0;     // TEX0_1
    uint64_t alpha;    // ALPHA_1
    uint64_t clamp;    // CLAMP_1
    uint64_t fba;      // FBA_1
  };

  static GsContext1State savedContext;
  static bool contextSaved;
};
