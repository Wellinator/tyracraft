/*
 * DmaGifBuilder - Implementation
 */

#include "managers/dma_gif_builder.hpp"
#include <gs_gp.h>
#include <dma.h>
#include <dma_tags.h>
#include <gif_tags.h>
#include <kernel.h>
#include <malloc.h>
#include <string.h>

DmaGifBuilder::DmaGifBuilder()
    : m_packets(nullptr),
      m_current(nullptr),
      m_gifTagPos(nullptr),
      m_packetCount(0),
      m_autoCount(false),
      m_begun(false),
      m_aligned(false) {
  // Allocate aligned memory for DMA packets (64-byte alignment required)
  m_packets = (qword_t*)memalign(64, DMA_GIF_BUILDER_MAX_PACKETS * sizeof(qword_t));
  
  if (m_packets == nullptr) {
    printf("DmaGifBuilder: Failed to allocate packet buffer!\n");
    return;
  }
  
  m_aligned = true;
  memset(m_packets, 0, DMA_GIF_BUILDER_MAX_PACKETS * sizeof(qword_t));
}

DmaGifBuilder::~DmaGifBuilder() {
  if (m_packets != nullptr) {
    free(m_packets);
    m_packets = nullptr;
  }
}

void DmaGifBuilder::begin() {
  if (!m_aligned) {
    printf("DmaGifBuilder: Cannot begin - buffer not properly aligned!\n");
    return;
  }

  m_current = m_packets;
  m_gifTagPos = nullptr;
  m_packetCount = 0;
  m_autoCount = false;
  m_begun = true;
}

void DmaGifBuilder::addGifTag(u64 regs, u32 regCount, u32 eop) {
  ensureBegun();

  // Check buffer overflow BEFORE writing
  if ((m_current - m_packets) >= DMA_GIF_BUILDER_MAX_PACKETS) {
    printf("DmaGifBuilder: Buffer overflow in addGifTag! Max: %d\n",
           DMA_GIF_BUILDER_MAX_PACKETS);
    return;
  }

  // Finalize previous GIF tag if exists
  if (m_gifTagPos != nullptr && m_autoCount) {
    finalizeGifTag();
  }

  // Store position for later count update if regCount is 0 (auto-count mode)
  m_gifTagPos = m_current;
  m_autoCount = (regCount == 0);

  // Add GIF tag (NREG=1 for A+D mode)
  PACK_GIFTAG(m_current, GIF_SET_TAG(regCount, eop, 0, 0, GIF_FLG_PACKED, 1),
              regs);
  m_current++;
  m_packetCount = 0;
}

void DmaGifBuilder::addAd(u64 value, u64 reg) {
  ensureBegun();

  // Check buffer overflow BEFORE writing
  if ((m_current - m_packets) >= DMA_GIF_BUILDER_MAX_PACKETS) {
    printf("DmaGifBuilder: Buffer overflow in addAd! Max: %d\n",
           DMA_GIF_BUILDER_MAX_PACKETS);
    return;
  }

  // Add register/value pair (A+D format: data in dw[0], addr in dw[1])
  PACK_GIFTAG(m_current, value, reg);
  m_current++;
  m_packetCount++;
}

void DmaGifBuilder::addRaw(u64 low, u64 high) {
  ensureBegun();

  // Check buffer overflow BEFORE writing
  if ((m_current - m_packets) >= DMA_GIF_BUILDER_MAX_PACKETS) {
    printf("DmaGifBuilder: Buffer overflow in addRaw! Max: %d\n",
           DMA_GIF_BUILDER_MAX_PACKETS);
    return;
  }

  m_current->dw[0] = low;
  m_current->dw[1] = high;
  m_current++;
  m_packetCount++;
}

void DmaGifBuilder::addGifLoopTag(u32 nloop, u32 eop, u32 pre, u64 prim, u32 flg, u32 nreg, u64 regs) {
  ensureBegun();

  // Check buffer overflow BEFORE writing
  if ((m_current - m_packets) >= DMA_GIF_BUILDER_MAX_PACKETS) {
    printf("DmaGifBuilder: Buffer overflow in addGifLoopTag! Max: %d\n",
           DMA_GIF_BUILDER_MAX_PACKETS);
    return;
  }

  // Finalize previous GIF tag if exists
  if (m_gifTagPos != nullptr && m_autoCount) {
    finalizeGifTag();
  }

  // Add GIF loop tag with explicit NLOOP (no auto-counting)
  PACK_GIFTAG(m_current, GIF_SET_TAG(nloop, eop, pre, prim, flg, nreg), regs);
  m_current++;
  
  // Reset auto-count since this tag has explicit NLOOP
  m_gifTagPos = nullptr;
  m_autoCount = false;
  m_packetCount = 0;
}

void DmaGifBuilder::send(u32 waitCycles) {
  ensureBegun();

  // Finalize GIF tag if auto-counting
  if (m_gifTagPos != nullptr && m_autoCount) {
    finalizeGifTag();
  }

  u32 size = m_current - m_packets;
  
  if (size == 0) {
    printf("DmaGifBuilder: Warning - sending empty packet!\n");
    return;
  }

  // Flush cache before DMA transfer
  FlushCache(0);

  // Send via DMA
  dma_channel_send_normal(DMA_CHANNEL_GIF, m_packets, size, 0, 0);
  dma_channel_wait(DMA_CHANNEL_GIF, waitCycles);

  // Reset for next use
  m_begun = false;
}

void DmaGifBuilder::sendAsync() {
  ensureBegun();

  // Finalize GIF tag if auto-counting
  if (m_gifTagPos != nullptr && m_autoCount) {
    finalizeGifTag();
  }

  u32 size = m_current - m_packets;
  
  if (size == 0) {
    printf("DmaGifBuilder: Warning - sending empty packet!\n");
    return;
  }

  // Flush cache before DMA transfer
  FlushCache(0);

  // Send via DMA without waiting
  dma_channel_send_normal(DMA_CHANNEL_GIF, m_packets, size, 0, 0);

  // Reset for next use
  m_begun = false;
}

void DmaGifBuilder::reset() {
  m_current = m_packets;
  m_gifTagPos = nullptr;
  m_packetCount = 0;
  m_autoCount = false;
  m_begun = false;
}

void DmaGifBuilder::finalizeGifTag() {
  if (m_gifTagPos == nullptr) return;

  // Update the NLOOP field in the GIF tag with the actual packet count
  // GIF tag structure: bits [14:0] = NLOOP
  u64 gifTag = m_gifTagPos->dw[0];
  gifTag = (gifTag & ~0x7FFFull) | (m_packetCount & 0x7FFF);
  m_gifTagPos->dw[0] = gifTag;

  // Reset for next GIF tag
  m_gifTagPos = nullptr;
  m_autoCount = false;
}

void DmaGifBuilder::ensureBegun() {
  if (!m_begun) {
    printf("DmaGifBuilder: Error - begin() must be called first!\n");
  }
}
