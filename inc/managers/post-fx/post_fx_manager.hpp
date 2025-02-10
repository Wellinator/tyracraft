#pragma once
#include "tyra"
#include "singleton.hpp"

using Tyra::Engine;
using Tyra::RendererSettings;
using Tyra::Texture;
using Tyra::TextureBuilderData;

typedef enum {
  CHANNEL_RED,
  CHANNEL_GREEN,
  CHANNEL_BLUE,
  CHANNEL_ALPHA,
} ColourChannels;

class PostFxManager : public Singleton<PostFxManager> {
 public:
  PostFxManager(Engine* t_engine);
  ~PostFxManager();

  void render();

 private:
  constexpr static float gsCenter = 4096.0F;
  constexpr static float screenCenter = gsCenter / 2.0F;

  Engine* pEngine = nullptr;
  const RendererSettings& settings;

  Texture* pFogTexture;
  Texture* pDepthBufferTexture;

  void init();
  void renderFog();
  void copyDepthBuffer(ColourChannels channelIn, Texture* palette);
  void performChannelCopy(ColourChannels channelIn, ColourChannels channelOut,
                          uint32_t blockX, uint32_t blockY, uint32_t source,
                          uint32_t width, uint32_t height,
                          uint32_t paletteAddress);
  void setTwTh(int w, int h, int* tw, int* th);

  static inline uint32_t lzw(uint32_t val) {
    uint32_t res;
    __asm__ __volatile__("   plzcw   %0, %1    " : "=r"(res) : "r"(val));
    return (res);
  }
};
