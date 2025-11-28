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

  static inline uint32_t lzw(uint32_t val) {
    uint32_t res;
    __asm__ __volatile__("   plzcw   %0, %1    " : "=r"(res) : "r"(val));
    return (res);
  }
};
