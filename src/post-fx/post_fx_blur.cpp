#include "managers/post-fx/post_fx_blur.hpp"

void postFxBlur(PostFxHelper& hlp, uint32_t bp0_words, uint32_t bp1_words,
                int bw0_pixels, int w, int h, int depth, float gain,
                float clampBlack) {
  // Port direto do sps2demo blur.cpp (lines 4-58)

  // Calcula half-width para buffer ping-pong
  int bw1_pixels = bw0_pixels / 2;
  if (bw0_pixels & 1) bw1_pixels = (bw0_pixels + 1) / 2;

  // Setup: clamp to edge, frame=bp0, texture=bp0
  hlp.setClamp(2, 2, 0, w - 1, 0, h - 1);
  hlp.setFrame(bp0_words, bw0_pixels);
  hlp.setTex0(bp0_words, bw0_pixels);

  // ClampBlack pass: subtrai pixels escuros (threshold)
  if (clampBlack > 0) {
    // ALPHA(2,1,2,1,fix): (0 - Cd) * fix/128 + Cd = Cd * (1 - fix/128)
    // Com fix = clampBlack*128, pixels abaixo do threshold são darkened
    hlp.setAlpha(2, 1, 2, 1, (int)(clampBlack * 128));
    hlp.drawBox(0, 0, w - 1, h - 1, true);
    hlp.setTexflush();
  }

  // Blur pass 1: horizontal blur com UV offset 1.8333
  // ALPHA(1,0,2,0,32): Cs + 0 * fix/128 + 0 = Cs, mas com fix=32 → blend 25%
  // Na verdade a fórmula é ((A-B)*C)>>7 + D, então (1-0)*fix>>7 + 0 = fix/128
  // O blur funciona porque o blit com alpha acumula múltiplas samples
  hlp.setAlpha(1, 0, 2, 0, 32);
  hlp.blit(0, 0, w, h, 1.8333f, 0.5f, w + 1.8333f, h + 0.5f, true);
  hlp.setTexflush();

  // Blur pass 2: vertical blur com UV offset 1.8333
  hlp.blit(0, 0, w, h, 0.5f, 1.8333f, w + 0.5f, h + 1.8333f, true);
  hlp.setTexflush();

  // Downsample para bp1 com gain
  hlp.setFrame(bp1_words, bw1_pixels);
  hlp.setRgba((int)(gain * 128), (int)(gain * 128), (int)(gain * 128), 128);
  hlp.blit(0, 0, w / 2, h / 2, 1, 1, w + 1, h + 1, false, true,
           true);  // flip=true
  hlp.setTexflush();
  hlp.setRgba(128, 128, 128, 128);  // Reset para cor neutra

  // Recursão: blur na half-resolution
  if (depth > 1) {
    postFxBlur(hlp, bp1_words, bp0_words, bw1_pixels, w / 2, h / 2, depth - 1,
               gain, clampBlack);
  }

  // Blur novamente no buffer half-res (bp1)
  hlp.setClamp(2, 2, 0, w / 2 - 1, 0, h / 2 - 1);
  hlp.setFrame(bp1_words, bw1_pixels);
  hlp.setTex0(bp1_words, bw1_pixels);

  if (clampBlack > 0) {
    hlp.setAlpha(2, 1, 2, 1, (int)(clampBlack * 128));
    hlp.drawBox(0, 0, w / 2 - 1, h / 2 - 1, true);
    hlp.setAlpha(1, 0, 2, 0, 32);
    hlp.setTexflush();
  }

  hlp.blit(0, 0, w / 2, h / 2, 1.8333f, 0.5f, w / 2 + 1.8333f, h / 2 + 0.5f,
           true);
  hlp.setTexflush();
  hlp.blit(0, 0, w / 2, h / 2, 0.5f, 1.8333f, w / 2 + 0.5f, h / 2 + 1.8333f,
           true);
  hlp.setTexflush();

  // Upsample de volta para bp0
  hlp.setFrame(bp0_words, bw0_pixels);
  hlp.setTex0(bp1_words, bw1_pixels);
  hlp.setRgba((int)(gain * 128), (int)(gain * 128), (int)(gain * 128), 128);
  hlp.blit(0, 0, w, h, 0.25f, 0.25f, w / 2 + 0.25f, h / 2 + 0.25f, false, true,
           true);  // flip=true, alpha=false
  hlp.setTexflush();
  hlp.setRgba(128, 128, 128, 128);
}
