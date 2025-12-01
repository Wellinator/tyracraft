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

  void renderFog(Color fogColor);

  /**
   * Define o valor de CLIP_ZVALUE para o fog.
   * @param value Valor de CLIP_ZVALUE (0x000000 a 0xFFFFFF)
   */
  void setClipZValue(uint32_t value);

  /**
   * Ajusta o valor de CLIP_ZVALUE por um delta.
   * @param delta Valor a ser adicionado (pode ser negativo)
   */
  void adjustClipZValue(int delta);

  /**
   * Retorna o valor atual de CLIP_ZVALUE.
   */
  uint32_t getClipZValue() const { return CLIP_ZVALUE; }

  /**
   * Reseta CLIP_ZVALUE para o valor padrão calculado.
   */
  void resetClipZValueToDefault();

 private:
  constexpr static float gsCenter = 4096.0F;
  constexpr static float screenCenter = gsCenter / 2.0F;

  Renderer* pRenderer = nullptr;
  const RendererSettings& settings;

  void setTwTh(int w, int h, int* tw, int* th);
  void performChannelCopy(ColourChannels channelIn, ColourChannels channelOut,
                          uint32_t blockX, uint32_t blockY, uint32_t source,
                          uint32_t width, uint32_t height,
                          uint32_t paletteAddress);

  /**
   * Calcula o valor CLIP_ZVALUE para o efeito de fog.
   * @param nearPlane Distância do plano near (ex: 0.01f)
   * @param farPlane Distância do plano far (ex: 1000.0f)
   * @param fogStartPercent Porcentagem da distância onde o fog começa (0.0f =
   * near, 1.0f = far)
   * @return Valor CLIP_ZVALUE para uso no passo 2 do fog
   */
  static uint32_t calculateClipZValue(float nearPlane, float farPlane,
                                      float fogStartPercent);

  // TODO: mover numeros mágicos para constantes.
  // Reaplicar no RendererSettings.
  uint32_t CLIP_ZVALUE;
  uint32_t DEFAULT_CLIP_ZVALUE;

  static inline uint32_t lzw(uint32_t val) {
    uint32_t res;
    __asm__ __volatile__("   plzcw   %0, %1    " : "=r"(res) : "r"(val));
    return (res);
  }
};
