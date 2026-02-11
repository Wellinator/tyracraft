#pragma once
#include "tyra"
#include "singleton.hpp"

using Tyra::Color;
using Tyra::Engine;
using Tyra::Renderer;
using Tyra::RendererSettings;
using Tyra::Texture;
using Tyra::TextureBuilderData;

class PostFxManager : public Singleton<PostFxManager> {
 public:
  PostFxManager(Renderer* renderer);
  ~PostFxManager();

  /**
   * Renderiza fog pós-processamento usando channel copy do Z-buffer.
   *
   * Usa Context 2 do GS (nenhum save/restore de Context 1 necessário).
   * Porta do sps2demo fadetofog + rgbaindexer com DMA chain + ref tags.
   */
  void renderFog(Color fogColor);

  /**
   * Renderiza bloom/glare effect.
   * NOTA: Deve ser chamado APÓS renderFog() para que o Z-buffer VRAM esteja
   * livre para uso como temp buffers.
   */
  void renderBloom();

  /**
   * Renderiza todos os efeitos de pós-processamento na ordem correta:
   * fog → bloom → restore.
   * @param fogColor Cor do fog
   */
  void renderAll(Color fogColor);

  // ===== Bloom controls =====
  void setBloomCutoff(float cutoff) { bloomCutoff = cutoff; }
  float getBloomCutoff() const { return bloomCutoff; }

  void setBloomDepth(int depth) { bloomDepth = depth; }
  int getBloomDepth() const { return bloomDepth; }

  void setBloomSourceScale(float scale) { bloomSourceScale = scale; }
  float getBloomSourceScale() const { return bloomSourceScale; }

  void setBloomGain(float gain) { bloomGain = gain; }
  float getBloomGain() const { return bloomGain; }

 private:
  Renderer* pRenderer = nullptr;
  const RendererSettings& settings;

  // Fixed VRAM position for fog CLUT (end of 4MB - 8KB)
  uint32_t fogClutVramAddr = 0;

  // ===== Bloom parameters =====
  float bloomCutoff = 0.3f;      // Brightness threshold (0.0-1.0)
  int bloomDepth = 3;            // Blur pyramid depth (1-4)
  float bloomSourceScale = 1.5f; // Additive blend strength
  float bloomGain = 1.2f;        // Brightness multiplier

  // VRAM addresses para temp buffers (computed from Z-buffer)
  uint32_t tempBufA_words = 0;
  uint32_t tempBufB_words = 0;

  /**
   * Calcula endereços dos buffers temporários a partir do Z-buffer.
   * Após o fog completar, o Z-buffer fica livre para reutilização.
   */
  void computeTempBufferAddresses();
};
