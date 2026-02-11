#include "managers/post-fx/post_fx_glare.hpp"
#include "managers/post-fx/post_fx_blur.hpp"

void postFxGlare(PostFxHelper& hlp, uint32_t fbAddr_words, uint32_t tempA_words,
                 uint32_t tempB_words, int screenW, int screenH, float cutoff,
                 int depth, float sourceScale, float gain, float clampBlack) {
  // Port direto do sps2demo glare.cpp (lines 6-32)

  int halfW = screenW / 2;
  int halfH = screenH / 2;
  int halfBW = screenW / 2;  // BW em pixels (metade da largura da tela)

  // Step 1: Downsample para half-resolution em tempA
  hlp.setFrame(tempA_words, halfBW);
  hlp.downSample(0, 0, halfW, halfH);

  // Step 2: Threshold (brightness cutoff)
  if (cutoff > 0) {
    // ALPHA(2,0,2,1,fix): (0 - Cs) * fix/128 + Cd
    // Com Cs=Cd (mesmo buffer), fica: Cd - Cd*fix/128 = Cd*(1-fix/128)
    // Isso subtrai uma fração do pixel, removendo pixels escuros
    int fix = (int)(cutoff * 256);
    if (fix > 255) fix = 255;  // Clamp para range válido
    hlp.setAlpha(2, 0, 2, 1, fix);
    hlp.blit(0, 0, halfW, halfH, true);
  }

  // Step 3: Blur pyramid na half-resolution
  // Usa blur com clampBlack=0.5 para suprimir ruído
  postFxBlur(hlp, tempA_words, tempB_words, halfBW, halfW, halfH, depth, gain,
             clampBlack);

  hlp.setTexflush();

  // Step 4: Composite de volta ao framebuffer full-res
  hlp.setFrame(fbAddr_words, screenW);
  hlp.setTex0(tempA_words, halfBW);
  hlp.setClamp(2, 2, 0, halfW - 1, 0, halfH - 1);

  // Blend mode: additive ou subtractive baseado no sinal de sourceScale
  if (sourceScale > 0) {
    // Additive: ALPHA(1,2,2,0,fix) = Cs + Cd
    hlp.setAlpha(1, 2, 2, 0, (int)(sourceScale * 128));
  } else {
    // Subtractive: ALPHA(2,1,2,0,fix) = 0 - Cd
    hlp.setAlpha(2, 1, 2, 0, (int)(-sourceScale * 128));
  }

  // Offset para centralizar a half-res texture (sps2demo usa offset baseado em
  // depth) O offset corrige small pixel shifts do blur pyramid
  int offset = (depth & 1) ? (1 << (depth - 2)) : -(1 << (depth - 2));
  if (depth <= 1) offset = 0;  // No offset para depth=1

  hlp.blit(0, 0, screenW, screenH, offset, offset, halfW + offset,
           halfH + offset, true);
}
