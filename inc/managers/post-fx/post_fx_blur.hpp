#pragma once

#include "post_fx_helper.hpp"
#include <stdint.h>

/**
 * postFxBlur - Gaussian pyramid blur (port do sps2demo blur.cpp)
 *
 * Implementa blur multi-level com ping-pong buffers e offset UV sampling.
 * O algoritmo aplica blur horizontal+vertical usando bilinear interpolation
 * com offset de 1.8333 pixels, depois recursivamente blur em half-resolution.
 *
 * @param hlp PostFxHelper instance para operações GS
 * @param bp0_words Buffer principal (word address)
 * @param bp1_words Buffer secundário para ping-pong (word address)
 * @param bw0_pixels Largura do buffer principal em pixels
 * @param w Largura da região a blur em pixels
 * @param h Altura da região a blur em pixels
 * @param depth Profundidade da pirâmide (1-3 recomendado)
 * @param gain Multiplicador de brilho (default 1.0)
 * @param clampBlack Threshold para remover pixels escuros (0.0-1.0, default 0)
 *
 * Baseado em sps2demo/posteffects/blur.cpp
 */
void postFxBlur(PostFxHelper& hlp, uint32_t bp0_words, uint32_t bp1_words,
                int bw0_pixels, int w, int h, int depth, float gain = 1.0f,
                float clampBlack = 0.0f);
