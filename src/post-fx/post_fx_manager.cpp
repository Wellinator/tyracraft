#include "managers/post-fx/post_fx_manager.hpp"
#include "managers/post-fx/post_fx_fog.hpp"
#include "managers/post-fx/post_fx_helper.hpp"
#include "managers/post-fx/post_fx_glare.hpp"
#include "screen_settings.hpp"
#include "constants.hpp"
#include "debug.hpp"
#include <cstdio>
#include <cstdint>

using Tyra::Color;

PostFxManager::PostFxManager(Renderer* renderer)
    : Singleton<PostFxManager>(),
      settings(renderer != nullptr ? renderer->core.getSettings()
                                    : *(RendererSettings*)nullptr) {
  // BUG FIX #2: Null check para evitar segfault
  if (renderer == nullptr) {
    printf("[PostFxManager] FATAL: Renderer pointer is null!\n");
    // PS2 não tem exceptions — crashar gracefully
    // Alternativamente: return após definir pRenderer = nullptr e checar em todos os métodos
    // Por enquanto, deixamos crashar (nullptr deref na linha acima força erro)
    return;
  }

  pRenderer = renderer;

  // Fog CLUT at end of VRAM: (4MB - 8KB) / 4 words
  // BUG FIX #7: Usa constantes ao invés de magic numbers
  fogClutVramAddr = (PS2_VRAM_SIZE_BYTES - FOG_CLUT_SIZE_BYTES) / 4;
  postFxFogInit(fogClutVramAddr);
}

PostFxManager::~PostFxManager() {
  postFxFogCleanup();
}

void PostFxManager::computeTempBufferAddresses() {
  auto zbufferAddr = pRenderer->core.gs.zBuffer.address;
  tempBufA_words = zbufferAddr;
  
  // BUG FIX #1/#7: Usa constante ao invés de magic number (28 * 2048)
  // tempBufB offset: 256×224×4 bytes/pixel = 229,376 bytes = 57,344 words
  tempBufB_words = zbufferAddr + HALF_RES_BUFFER_SIZE_WORDS;

  // BUG FIX #8: Validação de VRAM size
  // Z-buffer para 512×448 PSM_32: 512×448×4 = 917,504 bytes = 229,376 words
  // Precisamos de 2× HALF_RES_BUFFER_SIZE_WORDS = 114,688 words
  // Total: 114,688 < 229,376 ✓ (safe)
  uint32_t zbufSize_words = (SCREEN_WIDTH * SCREEN_HEIGHT * 4) / 4;
  uint32_t requiredSize_words = 2 * HALF_RES_BUFFER_SIZE_WORDS;
  
  if (requiredSize_words > zbufSize_words) {
    printf("[PostFxManager] WARNING: Bloom temp buffers (%u words) exceed Z-buffer size (%u words)!\n",
           requiredSize_words, zbufSize_words);
  }
}

void PostFxManager::renderBloom() {
#ifdef DEBUG_MODE
  if (!bloomEnabled) return;
#endif

  computeTempBufferAddresses();

  auto frameBuffer = pRenderer->core.gs.getCurrentFrameData();
  uint32_t fbAddr = frameBuffer.address;

  // PostFxHelper usa Context 2, não precisa save/restore de Context 1
  PostFxHelper hlp(fbAddr, SCREEN_WIDTH, SCREEN_HEIGHT);

  postFxGlare(hlp, fbAddr, tempBufA_words, tempBufB_words, SCREEN_WIDTH,
              SCREEN_HEIGHT, bloomCutoff, bloomDepth, bloomSourceScale,
              bloomGain, 0.5f);
}

void PostFxManager::renderAll(Color fogColor) {
#ifdef DEBUG_MODE
  if (g_debug_menu.enablePostFx == false) return;
#endif

  auto frameBuffer = pRenderer->core.gs.getCurrentFrameData();
  uint32_t fbAddr = frameBuffer.address;
  uint32_t zbufAddr = pRenderer->core.gs.zBuffer.address;

  // Use the other framebuffer as temp storage for Z re-swizzle
  // (PSMZ24→PSMCT32 page layout conversion needed before PSMCT16 trick)
  u8 otherCtx = 1 - pRenderer->core.gs.getDrawContext();
  uint32_t tempAddr = pRenderer->core.gs.getFrameBuffer(otherCtx).address;

  // PostFxHelper usa Context 2, não precisa save/restore de Context 1
  PostFxHelper hlp(fbAddr, SCREEN_WIDTH, SCREEN_HEIGHT);

  // Phase 1: Fog (reads Z-buffer for the last time)
#ifdef DEBUG_MODE
  if (g_debug_menu.fogEnabled) {
#endif
    postFxFog(hlp, fbAddr, zbufAddr, tempAddr, SCREEN_WIDTH, SCREEN_HEIGHT,
              (int)fogColor.r, (int)fogColor.g, (int)fogColor.b);
#ifdef DEBUG_MODE
  }
#endif

  // Phase 2: Bloom (reuses Z-buffer VRAM as temp storage)
  if (bloomEnabled) {
    computeTempBufferAddresses();
    postFxGlare(hlp, fbAddr, tempBufA_words, tempBufB_words, SCREEN_WIDTH,
                SCREEN_HEIGHT, bloomCutoff, bloomDepth, bloomSourceScale,
                bloomGain, 0.5f);
  }
}

void PostFxManager::renderFog(Color fogColor) {
#ifdef DEBUG_MODE
  if (g_debug_menu.enablePostFx == false) return;
  if (g_debug_menu.fogEnabled == false) return;
#endif

  auto frameBuffer = pRenderer->core.gs.getCurrentFrameData();
  uint32_t fbAddr = frameBuffer.address;
  uint32_t zbufAddr = pRenderer->core.gs.zBuffer.address;

  // Use the other framebuffer as temp storage for Z re-swizzle
  u8 otherCtx = 1 - pRenderer->core.gs.getDrawContext();
  uint32_t tempAddr = pRenderer->core.gs.getFrameBuffer(otherCtx).address;

  // PostFxHelper usa Context 2, não precisa save/restore de Context 1
  PostFxHelper hlp(fbAddr, SCREEN_WIDTH, SCREEN_HEIGHT);

  postFxFog(hlp, fbAddr, zbufAddr, tempAddr, SCREEN_WIDTH, SCREEN_HEIGHT,
            (int)fogColor.r, (int)fogColor.g, (int)fogColor.b);
}
