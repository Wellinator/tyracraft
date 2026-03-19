#include "managers/post-fx/post_fx_dusk.hpp"

void postFxDusk(PostFxHelper& hlp, int screenW, int screenH, float intensity) {
  if (intensity <= 0.001f) return;

  // Color: Warm orange/peach (RGB: 255, 140, 40)
  // We use additive blending to make it look like a glow/haze
  hlp.setRgba(255, 140, 40, 128);

  // ALPHA formula: (A - B) * C + D
  // A=Cs (Source, orange), B=0, C=FIX (intensity), D=Cd (Dest, framebuffer)
  // Result: Cs * FIX + Cd
  int fix = (int)(intensity * 128); // 0-128 range for Tyra's hex shift
  if (fix > 128) fix = 128;

  hlp.setAlpha(0, 2, 2, 1, fix);

  // Draw full screen box
  hlp.drawBox(0, 0, screenW, screenH, true);
}
