/*
 * DmaGifBuilder - Helper class for building and sending GIF packets via DMA
 * 
 * This class eliminates the need for manual packet counting by automatically
 * tracking the number of qwords added to the packet buffer. Inspired by the
 * RWDMA_LOCAL_BLOCK_BEGIN/ADDTOPKT/LOAD_GIFTAG_QWC pattern.
 * 
 * Usage Example:
 * 
 *   DmaGifBuilder builder;
 *   builder.begin();
 *   
 *   builder.add(GS_SET_TEST(...), GS_REG_TEST_1);
 *   builder.add(GS_SET_ZBUF(...), GS_REG_ZBUF_1);
 *   builder.add(GS_SET_FRAME(...), GS_REG_FRAME_1);
 *   
 *   builder.send();  // Automatically counts packets and sends
 */

#pragma once

#include <tamtypes.h>
#include <gs_gp.h>
#include <dma.h>
#include <dma_tags.h>
#include <stdio.h>

#define DMA_GIF_BUILDER_MAX_PACKETS 500

class DmaGifBuilder {
 public:
  DmaGifBuilder();
  ~DmaGifBuilder();

  /**
   * Begin building a new packet sequence
   * Initializes the packet buffer and resets counters
   */
  void begin();

  /**
   * Add a GIF packed mode tag with A+D descriptor
   * This is typically the first call after begin() to set up the GIF tag.
   * The NLOOP count will be automatically calculated if regCount is 0.
   * 
   * @param regs Register specification (e.g., GIF_REG_AD)
   * @param regCount Number of loop iterations / A+D pairs (default: 0 for auto-count)
   * @param eop End Of Packet flag (default: 1 = last GIF tag in this transfer)
   */
  void addGifTag(u64 regs, u32 regCount = 0, u32 eop = 1);

  /**
   * Add a register/value pair in A+D mode
   * Automatically updates the GIF tag count if addGifTag was called with regCount=0
   * 
   * @param value Register value
   * @param reg Register address (e.g., GS_REG_TEST_1)
   */
  void addAd(u64 value, u64 reg);

  /**
   * Add a raw qword to the packet buffer
   * Use this for custom packet data that doesn't follow A+D mode
   * 
   * @param low Low 64 bits
   * @param high High 64 bits
   */
  void addRaw(u64 low, u64 high);

  /**
   * Add a GIF loop tag (for sprite/primitive loops)
   * 
   * @param nloop Number of loop iterations
   * @param eop End of packet flag
   * @param pre PRE flag
   * @param prim PRIM value
   * @param flg GIF flag (PACKED, REGLIST, IMAGE)
   * @param nreg Number of registers
   * @param regs Register specification
   */
  void addGifLoopTag(u32 nloop, u32 eop, u32 pre, u64 prim, u32 flg, u32 nreg, u64 regs);

  /**
   * Send the accumulated packets via DMA and wait for completion
   * Automatically finalizes the GIF tag count if needed
   * 
   * @param waitCycles Number of cycles to wait (default: 500)
   */
  void send(u32 waitCycles = 500);

  /**
   * Send the accumulated packets via DMA without waiting
   * Useful for background operations
   */
  void sendAsync();

  /**
   * Get the current packet count (for debugging)
   */
  u32 getPacketCount() const { return m_packetCount; }

  /**
   * Get the total size in qwords (for debugging)
   */
  u32 getSize() const { return m_current - m_packets; }

  /**
   * Reset the builder without allocating new memory
   * Useful for reusing the same builder multiple times
   */
  void reset();

 private:
  qword_t* m_packets;        // Packet buffer
  qword_t* m_current;        // Current write position
  qword_t* m_gifTagPos;      // Position of the last GIF tag (for auto-counting)
  u32 m_packetCount;         // Number of packets added since last GIF tag
  bool m_autoCount;          // Whether to auto-count packets
  bool m_begun;              // Whether begin() was called
  bool m_aligned;            // Whether the buffer is properly aligned

  void finalizeGifTag();     // Finalize the GIF tag with actual packet count
  void ensureBegun();        // Check if begin() was called
};

// Convenience macros for cleaner syntax (optional)
#define DMA_BEGIN(builder) builder.begin()
#define DMA_ADD(builder, value, reg) builder.addAd(value, reg)
#define DMA_SEND(builder) builder.send()
#define DMA_SEND_ASYNC(builder) builder.sendAsync()
