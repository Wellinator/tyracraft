#pragma once
#include "tyra"
#include "singleton.hpp"

using Tyra::Color;
using Tyra::Engine;
using Tyra::Renderer;
using Tyra::RendererSettings;
using Tyra::Texture;
using Tyra::TextureBuilderData;

typedef enum {
  CHANNEL_RED,
  CHANNEL_GREEN,
  CHANNEL_BLUE,
  CHANNEL_ALPHA,
} ColourChannels;

typedef struct {
  unsigned char enable;
  unsigned char method;
  unsigned char compval;
  unsigned char keep;
} ALPHATEST;

typedef struct {
  unsigned char enable;
  unsigned char pass;
} DESTTEST;

typedef struct {
  unsigned char enable;
  unsigned char method;
} DEPTHTEST;

typedef struct {
  char color1;
  char color2;
  char alpha;
  char color3;
  unsigned char fixed_alpha;
} BLEND;

class PostFxManager : public Singleton<PostFxManager> {
 public:
  PostFxManager(Renderer* renderer);
  ~PostFxManager();

  /**
   * Renderiza fog pós-processamento usando channel copy do Z-buffer.
   *
   * Pipeline simplificada (Strategy B):
   *   1. fogPassSetup()       — configura GS state para pós-processamento
   *   2. fogPass3ChannelCopy() — BITBLTBUF + PSM_8 + fog CLUT: Z green → FB
   * alpha
   *   3. fogPass6Apply()      — lerp(framebuffer, fogColor, alpha)
   *   4. fogPassRestore()     — restaura GS state original
   *
   * A CLUT não-linear controla inversão de Z, clipping e curva do fog.
   */
  void renderFog(Color fogColor);

  /**
   * Define a porcentagem da distância onde o fog começa (0.0 = near, 1.0 =
   * far). Valores menores fazem o fog começar mais perto da câmera.
   * @param percent Valor entre 0.0 e 1.0 (padrão: 0.15)
   */
  void setFogNearPercent(float percent);

  /**
   * Define a intensidade/densidade do fog.
   * Controla o expoente da curva: valores maiores = fog mais concentrado
   * perto do far plane. Valores menores = fog mais espalhado.
   * @param intensity Valor > 0.0 (padrão: 2.0)
   */
  void setFogIntensity(float intensity);

  /** Retorna a porcentagem de near atual. */
  float getFogNearPercent() const { return fogNearPercent; }

  /** Retorna a intensidade/densidade do fog atual. */
  float getFogIntensity() const { return fogIntensity; }

 private:
  constexpr static float gsCenter = 4096.0F;
  constexpr static float screenCenter = gsCenter / 2.0F;

  Renderer* pRenderer = nullptr;
  const RendererSettings& settings;

  void setTwTh(int w, int h, int* tw, int* th);

  /**
   * Copia o Z-buffer inteiro para o framebuffer alpha usando channel shuffle.
   * Processa em blocos de 64x32 com BITBLTBUF local-to-local (PSMZ_32→PSM_32)
   * seguido de PSM_8 + fog CLUT para extrair o canal e aplicar a curva de fog.
   */
  void copyDepthBuffer(ColourChannels channelIn, ColourChannels channelOut);

  /**
   * Copia um bloco 64x32 do Z-buffer (PSMZ_32) para buffer temporário (PSM_32)
   * usando BITBLTBUF local-to-local transfer. Resolve o problema de swizzle
   * entre PSMZ_32 e PSM_8 ao ler o Z-buffer como textura.
   */
  void copyZBufferBlockToTemp(uint32_t zbufPage, uint32_t tempAddr);

  /**
   * Faz channel copy de um bloco 64x32 usando PSM_8 + fog CLUT.
   * A CLUT aplica inversão + clipping + curva de fog automaticamente.
   */
  void performChannelCopyBlock(ColourChannels channelIn,
                               ColourChannels channelOut, uint32_t blockX,
                               uint32_t blockY, uint32_t source_addr);

  /**
   * Faz upload da CLUT de fog para a VRAM.
   * A paleta codifica: inversão de Z (near→0, far→high), clipping por
   * nearPercent, e curva não-linear de fog controlada por intensity.
   * Aplica CSM1 swizzle para compatibilidade com o hardware do GS.
   */
  void uploadFogCLUT();

  // Fog rendering passes
  void fogPassSetup();
  void fogPass3ChannelCopy();
  void fogPass6Apply(const Color& fogColor);
  void fogPassRestore();

  // Propriedades reutilizáveis para fog rendering
  uint32_t fog_width;
  uint32_t fog_height;
  int fog_tw;
  int fog_th;
  uint32_t fog_zbufferAddr;
  uint32_t fog_zbufferPsm;
  zbuffer_t fog_zbuffer;
  framebuffer_t fog_buf_frame;

  /**
   * Endereço da CLUT na VRAM após alocação.
   */
  uint32_t clutVramAddress = 0;

  /**
   * Parâmetros de configuração do fog.
   * fogNearPercent: porcentagem da distância sem fog (0.0-1.0)
   * fogIntensity: expoente da curva de fog (> 0.0)
   * clutDirty: flag para re-upload da CLUT quando parâmetros mudam
   */
  float fogNearPercent = 0.15f;
  float fogIntensity = 2.0f;
  bool clutDirty = true;

#ifdef DEBUG_MODE
  void saveDebugScreenshot(const char* filename, uint32_t address,
                           uint32_t width, uint32_t height, uint32_t psm);
#endif

  /**
   * Aplica reordenamento CSM1 para índice de CLUT de 256 entradas.
   * O PS2 GS acessa a CLUT com blocos de 8 entradas reordenados:
   * dentro de cada grupo de 32, os blocos 8-15 e 16-23 são trocados.
   */
  static inline int csm1Reorder(int i) {
    int group = i & ~31;
    int sub = i & 31;
    if (sub >= 8 && sub < 16)
      return group + sub + 8;
    else if (sub >= 16 && sub < 24)
      return group + sub - 8;
    return i;
  }

  static inline uint32_t lzw(uint32_t val) {
    uint32_t res;
    __asm__ __volatile__("   plzcw   %0, %1    " : "=r"(res) : "r"(val));
    return (res);
  }
};
