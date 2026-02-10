---
name: dma
description: 'Guide to PS2 DMA/GIF packet building using the DmaGifBuilder class. Use when: sending data to the Graphics Synthesizer (GS), writing GS registers (TEST, FRAME, ZBUF, ALPHA, SCISSOR, TEX0, etc.), drawing sprites/primitives, building GIF packets in A+D PACKED mode or REGLIST mode, implementing post-processing effects, managing qword_t buffers, using dma_channel_send_normal, calling FlushCache, working with GIFtags, batching vertex data (UV+XYZ pairs), troubleshooting GS lockups, or replacing manual packet construction with auto-counting GIF tags.'
---

# DMA/GIF Packet Building with `DmaGifBuilder`

> **Purpose**: This skill teaches AI agents how to build and send Graphics Interface (GIF)
> packets to the PS2's Graphics Synthesizer (GS) using the `DmaGifBuilder` helper class.
> It covers both A+D PACKED mode (for GS register writes) and REGLIST mode (for efficient
> sprite/primitive batching), with real-world usage patterns from TyraCraft's post-FX system.

---

## Table of Contents

- [Overview](#overview)
- [When to Use DmaGifBuilder](#when-to-use-dmagifbuilder)
- [API Quick Reference](#api-quick-reference)
- [Lifecycle: begin → add → send](#lifecycle-begin--add--send)
- [Pattern A: A+D Auto-Count (GS Register Writes)](#pattern-a-ad-auto-count-gs-register-writes)
- [Pattern B: A+D Manual Drawing (Sprite XYZ2 Kicks)](#pattern-b-ad-manual-drawing-sprite-xyz2-kicks)
- [Pattern C: REGLIST Batched Sprites](#pattern-c-reglist-batched-sprites)
- [GS Register Writes Cheat Sheet](#gs-register-writes-cheat-sheet)
- [Edge Cases & Gotchas](#edge-cases--gotchas)
- [When NOT to Use DmaGifBuilder](#when-not-to-use-dmagifbuilder)
- [File Quick-Reference](#file-quick-reference)
- [Debugging Checklist](#debugging-checklist)

---

## Overview

### What is DmaGifBuilder?

`DmaGifBuilder` is a C++ helper class in [inc/managers/dma_gif_builder.hpp](inc/managers/dma_gif_builder.hpp)
that **eliminates manual packet counting** when building GIF (Graphics Interface) packets
for the PlayStation 2's Graphics Synthesizer (GS). It provides a simple `begin() → add → send()`
API that automatically:

1. Allocates a 64-byte aligned `qword_t` buffer (500 qwords = 8,000 bytes)
2. Tracks the number of register writes / data qwords added
3. Auto-fills the GIFtag `NLOOP` field (loop count) on send
4. Calls `FlushCache(0)` and `dma_channel_send_normal(DMA_CHANNEL_GIF, ...)`
5. Optionally waits for DMA completion

### Why use it?

**Without `DmaGifBuilder`** (legacy manual approach):
```cpp
qword_t packets[20] ALIGNED(64);
qword_t* q = packets;

PACK_GIFTAG(q, GIF_SET_TAG(3, 1, 0, 0, GIF_FLG_PACKED, 1), GIF_REG_AD);
q++;
q->dw[0] = GS_SET_TEST(...); q->dw[1] = GS_REG_TEST_1; q++;
q->dw[0] = GS_SET_FRAME(...); q->dw[1] = GS_REG_FRAME_1; q++;
q->dw[0] = GS_SET_ZBUF(...); q->dw[1] = GS_REG_ZBUF_1; q++;

FlushCache(0);
dma_channel_send_normal(DMA_CHANNEL_GIF, packets, q - packets, 0, 0);
dma_channel_wait(DMA_CHANNEL_GIF, 500);
```
❌ Manual `NLOOP=3` count — error-prone if you add/remove registers  
❌ Manual pointer arithmetic  
❌ Manual `FlushCache` / DMA send boilerplate  

**With `DmaGifBuilder`**:
```cpp
DmaGifBuilder builder;
builder.begin();
builder.addGifTag(GIF_REG_AD);  // NLOOP auto-counted
builder.addAd(GS_SET_TEST(...), GS_REG_TEST_1);
builder.addAd(GS_SET_FRAME(...), GS_REG_FRAME_1);
builder.addAd(GS_SET_ZBUF(...), GS_REG_ZBUF_1);
builder.send();  // Auto-finalizes NLOOP=3, flushes, sends, waits
```
✅ Automatic packet counting  
✅ Clean API — no pointer math  
✅ Built-in `FlushCache` + DMA send  

### Related Documentation

- **Theory & deep reference**: [docs/agents/dma-gif-reference.md](docs/agents/dma-gif-reference.md) — DMAtag format, GIFtag bit layout, complete GS register catalog, PACKED vs REGLIST vs IMAGE modes
- **PS2 DMA system**: [docs/agents/skill-ee-system.md](docs/agents/skill-ee-system.md) — DMA channels, tags, GIF path architecture
- **GS rendering**: [docs/agents/skill-gs-rendering.md](docs/agents/skill-gs-rendering.md) — VRAM layout, pixel tests, alpha blending, texture upload
- **Real-world example**: [src/post-fx/post_fx_manager.cpp](src/post-fx/post_fx_manager.cpp) — All 4 fog pipeline passes use `DmaGifBuilder`

---

## When to Use DmaGifBuilder

| Task | Use DmaGifBuilder? |
|---|---|
| **Writing GS registers** (TEST, FRAME, ZBUF, ALPHA, SCISSOR, etc.) | ✅ Yes — Pattern A (A+D auto-count) |
| **Drawing sprites** via A+D XYZ2 kicks | ✅ Yes — Pattern B (A+D manual) |
| **Batched sprite rendering** (UV+XYZ pairs) | ✅ Yes — Pattern C (REGLIST mode) |
| **Uploading textures/CLUT** (IMAGE mode) | ❌ No — Use legacy `draw_texture_transfer` + chain mode |
| **VU1 microprogram uploads** | ❌ No — Use VIF/DMA chain mode |
| **Complex DMA chains** (scatter-gather) | ❌ No — Use `dma_channel_send_chain` manually |

**Current usage in TyraCraft:** Only `PostFxManager` uses `DmaGifBuilder` (as of v0.86.140).
All other rendering (block meshes, entities, UI) goes through Tyra Engine's abstraction.

---

## API Quick Reference

| Method | Purpose | Key Notes |
|---|---|---|
| `begin()` | Start a new packet sequence | Resets buffer + counters. **Must** call before adding data |
| `addGifTag(regs, regCount=0, eop=1)` | Add A+D PACKED GIFtag | `regCount=0` → auto-count NLOOP. `regs` typically `GIF_REG_AD`. `eop=1` = last tag in transfer |
| `addAd(value, reg)` | Add A+D register write | `value` = `GS_SET_*` macro, `reg` = `GS_REG_*`. Increments auto-count |
| `addRaw(low, high)` | Add raw qword | Use for custom data. Increments auto-count |
| `addGifLoopTag(nloop, eop, pre, prim, flg, nreg, regs)` | Add fully custom GIFtag | For REGLIST/IMAGE modes. **No auto-count** |
| `send(waitCycles=500)` | Finalize, flush, DMA send, wait | Patches NLOOP if auto-count enabled. Default 500-cycle wait |
| `sendAsync()` | Finalize, flush, DMA send, no wait | For background operations |
| `reset()` | Reset without deallocation | Reuse the same builder for another packet sequence |
| `getPacketCount()` | Get current packet count | For debugging — returns # of qwords since last `addGifTag` |
| `getSize()` | Get total buffer usage in qwords | For debugging |

**Convenience macros** (optional — defined in header):
```cpp
DMA_BEGIN(builder);         // → builder.begin()
DMA_ADD(builder, val, reg); // → builder.addAd(val, reg)
DMA_SEND(builder);          // → builder.send()
DMA_SEND_ASYNC(builder);    // → builder.sendAsync()
```

---

## Lifecycle: begin → add → send

### 1. **Instantiate** (usually as a class member)

```cpp
class MyRenderer {
 private:
  DmaGifBuilder m_dmaBuilder;  // Allocates 64-byte aligned buffer in constructor
};
```

Or stack-allocated for one-off usage:
```cpp
void myFunction() {
  DmaGifBuilder builder;  // Heap-allocates 8 KB buffer
  // ... use it ...
}  // Destructor calls free() on buffer
```

### 2. **Begin** a packet sequence

```cpp
builder.begin();  // Resets write pointer + counters
```
❗ Always call `begin()` before adding data. Multiple `begin()` calls are safe (resets state).

### 3. **Add** GIFtag + data

**Three patterns** — see detailed sections below.

### 4. **Send** to GS

```cpp
builder.send();        // Synchronous — waits for DMA completion
// OR
builder.sendAsync();   // Asynchronous — returns immediately
```

Internally, `send()`:
1. Calls `finalizeGifTag()` — patches the last GIFtag's NLOOP field with actual packet count (if auto-count was enabled)
2. Calls `FlushCache(0)` — ensures CPU writes are visible to DMA
3. Calls `dma_channel_send_normal(DMA_CHANNEL_GIF, m_packets, getSize(), 0, 0)`
4. Calls `dma_channel_wait(DMA_CHANNEL_GIF, waitCycles)` (only if not async)

### 5. **Reuse** (optional)

```cpp
builder.reset();  // Reset for another sequence without deallocating memory
// ... builder.begin() → add → send() again ...
```

---

## Pattern A: A+D Auto-Count (GS Register Writes)

**Use case:** Setting up GS state (TEST, FRAME, ZBUF, ALPHA, SCISSOR, etc.)

### Code Example

```cpp
builder.begin();

// GIFtag: PACKED mode, A+D descriptor, auto-count NLOOP
builder.addGifTag(GIF_REG_AD);  // regCount=0 → auto-count

// Add register writes — each increments NLOOP by 1
builder.addAd(GS_SET_TEST(0, ATEST_KEEP_ALL, 0x00, ATEST_METHOD_NOTEQUAL, 0, 0, 1, ZTEST_METHOD_GREATER_EQUAL), GS_REG_TEST_1);
builder.addAd(GS_SET_ZBUF(g_Manager.gs_context[0].zbuf, PSMZ_16, 1), GS_REG_ZBUF_1);
builder.addAd(GS_SET_XYOFFSET(0, 0), GS_REG_XYOFFSET_1);
builder.addAd(GS_SET_SCISSOR(0, SCREEN_WIDTH - 1, 0, SCREEN_HEIGHT - 1), GS_REG_SCISSOR_1);
builder.addAd(GS_SET_COLCLAMP(1), GS_REG_COLCLAMP);

builder.send();  // Auto-finalizes NLOOP=5
```

### What Happens

1. `addGifTag(GIF_REG_AD)` writes a **partial GIFtag** with `NLOOP=0` (placeholder)
2. Each `addAd()` call:
   - Writes a qword: `low 64 bits = value`, `high 64 bits = register address`
   - Increments internal `m_packetCount` counter
3. `send()` calls `finalizeGifTag()`:
   - Patches the GIFtag's NLOOP field: `NLOOP = m_packetCount` (5 in this example)
   - Flushes cache, sends via DMA

### GIF Packet Layout (Sent to GS)

```
Qword 0: [GIFtag: NLOOP=5, FLG=PACKED, NREG=1, REGS=GIF_REG_AD, EOP=1]
Qword 1: [low=TEST_value,     high=GS_REG_TEST_1]
Qword 2: [low=ZBUF_value,     high=GS_REG_ZBUF_1]
Qword 3: [low=XYOFFSET_value, high=GS_REG_XYOFFSET_1]
Qword 4: [low=SCISSOR_value,  high=GS_REG_SCISSOR_1]
Qword 5: [low=COLCLAMP_value, high=GS_REG_COLCLAMP]
```

### Real-World Example

See [`PostFxManager::fogSetupGS()`](src/post-fx/post_fx_manager.cpp#L173-L202) — sets up 11 GS registers using this exact pattern.

---

## Pattern B: A+D Manual Drawing (Sprite XYZ2 Kicks)

**Use case:** Drawing sprites by writing XYZ2 register (triggers drawing kick)

### Code Example

```cpp
builder.begin();
builder.addGifTag(GIF_REG_AD);  // Auto-count NLOOP

// Set fog color once
builder.addAd(GS_SET_RGBAQ(r, g, b, 0x80, 0), GS_REG_RGBAQ);

// Draw multiple 32-pixel-wide vertical strips
for (u16 x = 0; x < SCREEN_WIDTH; x += STRIP_WIDTH) {
  u16 x1 = x;
  u16 x2 = x + STRIP_WIDTH;
  
  // Write two XYZ2 vertices → triggers sprite draw
  builder.addAd(GS_SET_XYZ(ftoi4(x1), ftoi4(0), 0x00000000), GS_REG_XYZ2);
  builder.addAd(GS_SET_XYZ(ftoi4(x2), ftoi4(SCREEN_HEIGHT), 0x00000000), GS_REG_XYZ2);
  // GS draws a sprite from (x1,0) to (x2,SCREEN_HEIGHT)
}

builder.send();
```

### What Happens

- Each pair of `XYZ2` writes = 1 sprite drawn
- NLOOP auto-counts: `1 (RGBAQ) + 2 × num_strips (XYZ2 pairs)`
- **Drawing kick**: Writing `GS_REG_XYZ2/3` causes the GS to immediately rasterize using the currently set PRIM/color/texture state

### Real-World Example

See [`PostFxManager::fogApplyColor()`](src/post-fx/post_fx_manager.cpp#L402-L425) — draws 16 vertical fog strips using this pattern.

---

## Pattern C: REGLIST Batched Sprites

**Use case:** High-performance sprite rendering with UV+XYZ vertex pairs (e.g., copying Z-buffer to alpha channel)

### Code Example

```cpp
builder.begin();

// Manual REGLIST GIFtag — NO auto-count (we know exactly how many sprites)
u32 numSprites = 100;
builder.addGifLoopTag(
  numSprites,                     // NLOOP = number of sprites
  1,                              // EOP = 1 (end of packet)
  0,                              // PRE = 0 (no PRIM kick in tag)
  0,                              // PRIM (ignored when PRE=0)
  GIF_FLG_REGLIST,                // FLG = REGLIST mode
  2,                              // NREG = 2 (UV + XYZ2)
  GIF_REG_UV | (GIF_REG_XYZ2 << 4) // REGS = {UV, XYZ2}
);

// Add UV+XYZ pairs — each qword = 1 loop iteration
for (int i = 0; i < numSprites; i++) {
  u64 uv = GS_SET_UV(ftoi4(u), ftoi4(v));          // 64-bit UV value
  u64 xyz = GS_SET_XYZ(ftoi4(x), ftoi4(y), z);     // 64-bit XYZ value
  builder.addRaw(uv, xyz);  // Pack both into 1 qword
}

builder.send();
```

### Critical Notes

- **REGLIST mode packs 2 register values per qword** (UV in low 64 bits, XYZ in high 64 bits)
- `GS_SET_UV` / `GS_SET_XYZ` produce **64-bit REGLIST-format values** — they do **NOT** work in PACKED mode (different bit layouts!)
- **No auto-count** — you must set `NLOOP` manually in `addGifLoopTag` because `addRaw` doesn't know how many registers per qword
- **Drawing kick**: Writing XYZ2 via REGLIST still triggers sprite rasterization

### REGLIST vs PACKED Differences

| Aspect | PACKED Mode | REGLIST Mode |
|---|---|---|
| Data per qword | 1 register (128 bits: value+address) | NREG registers (each 64 bits) |
| Typical use | GS register writes (A+D) | Batched primitives (UV+XYZ) |
| `GS_SET_UV` / `GS_SET_XYZ` | ❌ **Wrong format!** | ✅ Correct 64-bit REGLIST format |
| Auto-count support | ✅ Yes (`addAd` increments) | ❌ No — use `addGifLoopTag` with explicit NLOOP |

### Real-World Example

See [`PostFxManager::fogCopyZGreenToAlpha()`](src/post-fx/post_fx_manager.cpp#L263-L340) — draws 2,688 8-pixel-wide sprites in REGLIST mode to copy Z-buffer green channel to alpha.

---

## GS Register Writes Cheat Sheet

### Drawing Context

| Macro | Register | Purpose | Example |
|---|---|---|---|
| `GS_SET_FRAME(fbp, fbw, psm, fbmsk)` | `GS_REG_FRAME_1/2` | Framebuffer config | `GS_SET_FRAME(ctx.frame, ctx.width/64, PSM_32, 0x00FF0000)` |
| `GS_SET_ZBUF(zbp, psm, zmsk)` | `GS_REG_ZBUF_1/2` | Z-buffer config | `GS_SET_ZBUF(ctx.zbuf, PSMZ_16, 1)` ← ZMSK=1 disables Z writes |
| `GS_SET_XYOFFSET(ofx, ofy)` | `GS_REG_XYOFFSET_1/2` | Coordinate offset | `GS_SET_XYOFFSET(0, 0)` ← raw pixel coords (no offset) |
| `GS_SET_SCISSOR(x0, x1, y0, y1)` | `GS_REG_SCISSOR_1/2` | Clip rectangle | `GS_SET_SCISSOR(0, 511, 0, 447)` |

### Texture

| Macro | Register | Purpose | Example |
|---|---|---|---|
| `GS_SET_TEX0(...)` | `GS_REG_TEX0_1/2` | Texture config (14 params!) | See [dma-gif-reference.md](docs/agents/dma-gif-reference.md#complete-gs-register-catalog) |
| `GS_SET_TEX1(lcm, mxl, mmag, mmin, ...)` | `GS_REG_TEX1_1/2` | Texture filter | `GS_SET_TEX1(1, 0, 0, 0, 0, 0, 0)` ← NEAREST filter |
| `GS_SET_TEXFLUSH(...)` | `GS_REG_TEXFLUSH` | **Flush texture cache** | `GS_SET_TEXFLUSH(0)` ← **Mandatory** after CLUT/texture upload |
| `GS_SET_CLAMP(wms, wmt, ...)` | `GS_REG_CLAMP_1/2` | Texture wrap mode | `GS_SET_CLAMP(1, 1, 0, 511, 0, 447)` |
| `GS_SET_TEXA(ta0, aem, ta1)` | `GS_REG_TEXA` | 16-bit alpha expansion | `GS_SET_TEXA(0, 1, 0x80)` |

### Pixel Tests & Blending

| Macro | Register | Purpose | Example |
|---|---|---|---|
| `GS_SET_TEST(ate, atst, aref, afail, date, datm, zte, ztst)` | `GS_REG_TEST_1/2` | Alpha/depth tests | `GS_SET_TEST(0, 0, 0, 0, 0, 0, 1, ZTEST_METHOD_ALWAYS)` |
| `GS_SET_ALPHA(a, b, c, d, fix)` | `GS_REG_ALPHA_1/2` | Blend formula `(A-B)×C>>7+D` | `GS_SET_ALPHA(0, 1, 2, 1, 0x80)` ← alpha blend |
| `GS_SET_PABE(pabe)` | `GS_REG_PABE` | Per-pixel alpha blend | `GS_SET_PABE(1)` |
| `GS_SET_COLCLAMP(clamp)` | `GS_REG_COLCLAMP` | Color clamping | `GS_SET_COLCLAMP(1)` ← clamp to [0,255] |

### Primitive Drawing

| Macro | Register | Purpose | Example |
|---|---|---|---|
| `GS_SET_PRIM(prim, iip, tme, fge, abe, aa1, fst, ctxt, fix)` | `GS_REG_PRIM` | Primitive type + attrs | `GS_SET_PRIM(6, 0, 1, 0, 1, 0, 1, 0, 0)` ← textured sprite |
| `GS_SET_XYZ(x, y, z)` | `GS_REG_XYZ2/3` | **Vertex position (triggers kick!)** | `GS_SET_XYZ(ftoi4(x), ftoi4(y), z)` ← **MUST BE LAST!** |
| `GS_SET_UV(u, v)` | `GS_REG_UV` | Texture coords (REGLIST only!) | `GS_SET_UV(ftoi4(u), ftoi4(v))` |
| `GS_SET_RGBAQ(r, g, b, a, q)` | `GS_REG_RGBAQ` | Vertex color | `GS_SET_RGBAQ(128, 128, 128, 0x80, 0)` |

**Full catalog**: [docs/agents/dma-gif-reference.md](docs/agents/dma-gif-reference.md#complete-gs-register-catalog)

---

## Edge Cases & Gotchas

### 1. Buffer Overflow (500 Qword Limit)

**Problem:** `DMA_GIF_BUILDER_MAX_PACKETS = 500` — adding more than 500 qwords causes silent buffer overflow or crash.

**Detection:** Each `add*` method checks and prints a warning:
```cpp
if (getSize() >= DMA_GIF_BUILDER_MAX_PACKETS) {
  printf("DmaGifBuilder: Buffer overflow! Max %d packets exceeded.\n", DMA_GIF_BUILDER_MAX_PACKETS);
  return;
}
```

**Solutions:**
- **Batch sends**: Call `send()` → `reset()` → continue building
- **Example**: `PostFxManager::fogCopyZGreenToAlpha()` batches sprites in groups of 320 to avoid overflow (see [line 285](src/post-fx/post_fx_manager.cpp#L285))
- **Increase limit**: Change `DMA_GIF_BUILDER_MAX_PACKETS` in [dma_gif_builder.hpp](inc/managers/dma_gif_builder.hpp#L12) (costs 8 KB heap per 500 qwords)

### 2. FlushCache(0) is Mandatory

**Problem:** DMA reads directly from RAM, bypassing CPU cache. If CPU writes packets to cache but doesn't flush, DMA transfers stale/garbage data.

**Automatic:** `DmaGifBuilder::send()` / `sendAsync()` always call `FlushCache(0)` before DMA send — you don't need to call it manually.

**Manual mode**: If you extract the buffer pointer (`m_packets`) and send via custom DMA code, **you must call `FlushCache(0)` yourself**.

### 3. NLOOP Mismatch → GS Lockup

**Problem:** If GIFtag `NLOOP` doesn't match the actual number of data qwords, the GS hangs waiting for missing data (or processes garbage).

**Auto-count protection:** Pattern A (A+D auto-count) eliminates this risk — `finalizeGifTag()` always writes the correct count.

**Manual NLOOP risk:** Pattern C (REGLIST mode via `addGifLoopTag`) requires you to set NLOOP correctly:
```cpp
// ✅ Correct — NLOOP matches loop iterations
builder.addGifLoopTag(100, 1, 0, 0, GIF_FLG_REGLIST, 2, GIF_REG_UV | (GIF_REG_XYZ2 << 4));
for (int i = 0; i < 100; i++) {  // ← Exactly 100 iterations
  builder.addRaw(uv, xyz);
}

// ❌ Wrong — NLOOP=100 but only 99 qwords → GS hangs!
builder.addGifLoopTag(100, ...);
for (int i = 0; i < 99; i++) {  // ← Mismatch!
  builder.addRaw(uv, xyz);
}
```

### 4. GS_SET_UV / GS_SET_XYZ Are REGLIST-Only!

**Problem:** `GS_SET_UV(u, v)` and `GS_SET_XYZ(x, y, z)` produce **64-bit REGLIST-format values**, not the 128-bit A+D PACKED format expected by `addAd()`.

**Symptom:** Using them in Pattern A causes rendering corruption — UV/XYZ values appear in wrong register fields.

**Rule:**
- ✅ **Pattern C (REGLIST)**: `builder.addRaw(GS_SET_UV(...), GS_SET_XYZ(...))`
- ❌ **Pattern A/B (PACKED)**: `builder.addAd(GS_SET_UV(...), GS_REG_UV)` ← **WRONG!**

**For PACKED mode sprite drawing:** Write XYZ via A+D descriptor (Pattern B) or use PRIM with `GS_SET_XYZ` in REGLIST mode (Pattern C).

### 5. TEXFLUSH After Texture/CLUT Upload

**Problem:** GS texture cache is not coherent with VRAM writes. After uploading a texture or CLUT, the cache contains stale data.

**Rule:** **Always** write `GS_REG_TEXFLUSH` after texture/CLUT upload:
```cpp
// Upload CLUT via legacy draw_texture_transfer (IMAGE mode)
uploadFogCLUT();

// Then configure texture + FLUSH before drawing
builder.begin();
builder.addGifTag(GIF_REG_AD);
builder.addAd(GS_SET_TEX0(...), GS_REG_TEX0_1);
builder.addAd(GS_SET_TEXFLUSH(0), GS_REG_TEXFLUSH);  // ← Mandatory!
builder.send();
```

**Symptom if forgotten:** Wrong colors, black textures, or rendering previous frame's texture.

### 6. ZTE=0 is Prohibited

**Problem:** PS2 hardware has a silicon bug — setting `ZTE=0` (depth test disabled) in the `TEST` register causes undefined behavior.

**Workaround:** Use `ZTE=1` + `ZTST=ALWAYS` + `ZMSK=1` to effectively disable depth testing:
```cpp
// ❌ Wrong — ZTE=0 can cause GS instability
builder.addAd(GS_SET_TEST(0, 0, 0, 0, 0, 0, 0, 0), GS_REG_TEST_1);

// ✅ Correct — ZTE=1, ZTST=ALWAYS, ZMSK=1
builder.addAd(GS_SET_TEST(
  0, ATEST_KEEP_ALL, 0, ATEST_METHOD_NOTEQUAL,  // Alpha test off
  0, 0,                                          // Dest alpha test off
  1, ZTEST_METHOD_ALWAYS                         // Depth test: always pass
), GS_REG_TEST_1);
builder.addAd(GS_SET_ZBUF(ctx.zbuf, PSMZ_16, 1), GS_REG_ZBUF_1);  // ZMSK=1 → no Z writes
```

### 7. Alpha 0x80 = 1.0 (Not 0xFF)

**Problem:** PS2 blend math uses `(value × alpha) >> 7`, so **0x80 = 1.0** (fully opaque), not 0xFF.

**Rule:**
- `0x00` = 0.0 (fully transparent)
- `0x80` = 1.0 (fully opaque)
- `0xFF` = 1.99... (over-bright — usually clamped)

**Example:**
```cpp
// ✅ Correct — opaque fog color
builder.addAd(GS_SET_RGBAQ(r, g, b, 0x80, 0), GS_REG_RGBAQ);

// ❌ Wrong — 0xFF is not 1.0!
builder.addAd(GS_SET_RGBAQ(r, g, b, 0xFF, 0), GS_REG_RGBAQ);
```

### 8. XYZ2 Triggers Drawing Kick — Must Be Last

**Problem:** Writing `GS_REG_XYZ2/3` immediately triggers the GS to rasterize a primitive using the **current** register state. If you set XYZ before other registers (UV, color, texture), those registers will have stale/default values.

**Rule:** **Always write XYZ2 last** in a drawing sequence:
```cpp
// ✅ Correct order
builder.addAd(GS_SET_UV(...), GS_REG_UV);          // 1. Texture coords
builder.addAd(GS_SET_RGBAQ(...), GS_REG_RGBAQ);    // 2. Color
builder.addAd(GS_SET_XYZ(...), GS_REG_XYZ2);       // 3. Vertex (triggers kick)

// ❌ Wrong — XYZ2 triggers draw before UV/color are set
builder.addAd(GS_SET_XYZ(...), GS_REG_XYZ2);       // ← Draws with default UV/color!
builder.addAd(GS_SET_UV(...), GS_REG_UV);          // ← Too late
```

### 9. memalign(64) + free() — Never delete

**Problem:** `DmaGifBuilder` allocates its buffer via `memalign(64, ...)` in the constructor. Using `delete`/`delete[]` on `memalign`'d memory causes heap corruption.

**Automatic:** The destructor correctly calls `free(m_packets)` — you don't need to manage this manually.

**If extending DmaGifBuilder:** Always pair `memalign` with `free`, never `new`/`delete`:
```cpp
// ✅ Correct
qword_t* buffer = (qword_t*)memalign(64, size);
// ... use buffer ...
free(buffer);

// ❌ Wrong — memalign + delete = crash
qword_t* buffer = (qword_t*)memalign(64, size);
delete[] buffer;  // ← HEAP CORRUPTION!
```

### 10. 32-Pixel Strip Width for Page Cache Optimization

**Problem:** GS VRAM is organized in 2048-byte pages. Rendering very narrow vertical strips (e.g., 1 pixel wide) across the screen causes page buffer thrashing — each strip touches many pages, flushing the write combine buffer repeatedly.

**Rule:** Use **32-pixel-wide strips** for full-screen effects (fog, post-FX):
```cpp
#define STRIP_WIDTH 32  // Optimized for page buffer

for (u16 x = 0; x < SCREEN_WIDTH; x += STRIP_WIDTH) {
  // Draw strip from x to x+STRIP_WIDTH
}
```

**Exception:** Z→Alpha copy uses **8-pixel strips** because the operation reinterprets pixel formats (different page alignment). See `POST_FX_SPRITE_WIDTH_IN_PX = 8` in [post_fx_manager.cpp](src/post-fx/post_fx_manager.cpp#L49).

**Theory:** [docs/agents/skill-postfx.md](docs/agents/skill-postfx.md#32-pixel-strip-width), [docs/agents/skill-gs-rendering.md](docs/agents/skill-gs-rendering.md#vram-layout)

---

## When NOT to Use DmaGifBuilder

### Texture/CLUT Uploads (IMAGE Mode)

**Reason:** `DmaGifBuilder` only supports **PACKED** (A+D) and **REGLIST** modes via its API.
Texture/CLUT uploads require **IMAGE mode** (FLG=2), which transfers raw pixel data without
register descriptors.

**Solution:** Use the legacy PS2SDK `draw_*` helpers + chain mode DMA:

```cpp
// Allocate CLUT data (256 × RGBA32 = 1024 bytes)
u32* clut = (u32*)memalign(64, 256 * sizeof(u32));
// ... fill CLUT with color data ...

// Build chain-mode packet for IMAGE transfer
qword_t packets[30] ALIGNED(64);
qword_t* q = packets;

q = draw_texture_transfer(q, clut, 256, 1, GS_PSM_32, clutBufferBase, 64);
q = draw_texture_flush(q);

FlushCache(0);
dma_channel_send_chain(DMA_CHANNEL_GIF, packets, q - packets, 0, 0);
dma_channel_wait(DMA_CHANNEL_GIF, 500);

free(clut);
```

**Real-world example:** [`PostFxManager::uploadFogCLUT()`](src/post-fx/post_fx_manager.cpp#L136-L170)

**Why chain mode?** The `draw_texture_transfer` function builds DMAtags (refe/cnt/end) that
specify scatter-gather addresses. Normal mode DMA can't interpret these tags.

### VU1 Microprogram Uploads

Use VIF/DMA chain mode — outside `DmaGifBuilder`'s scope. Tyra Engine handles this internally.

### Complex Scatter-Gather Transfers

If you need non-contiguous memory sources or conditional DMA branching (call/ret tags),
manually build a DMAtag chain and use `dma_channel_send_chain`.

---

## File Quick-Reference

| File | Role | Key Contents |
|---|---|---|
| [inc/managers/dma_gif_builder.hpp](inc/managers/dma_gif_builder.hpp) | **Header** | Class definition, API docs, convenience macros, `DMA_GIF_BUILDER_MAX_PACKETS` |
| [src/managers/dma_gif_builder.cpp](src/managers/dma_gif_builder.cpp) | **Implementation** | Constructor (`memalign`), destructor (`free`), all methods, NLOOP patching logic |
| [inc/managers/post-fx/post_fx_manager.hpp](inc/managers/post-fx/post_fx_manager.hpp) | **Consumer** | `DmaGifBuilder dmaBuilder;` member, fog pipeline method declarations |
| [src/post-fx/post_fx_manager.cpp](src/post-fx/post_fx_manager.cpp) | **Usage examples** | All 3 patterns: A+D auto-count (4 methods), REGLIST batched sprites (`fogCopyZGreenToAlpha`), legacy IMAGE upload (`uploadFogCLUT`) |
| [docs/agents/dma-gif-reference.md](docs/agents/dma-gif-reference.md) | **Theory reference** | DMAtag format, GIFtag bit layout, complete GS register catalog, PACKED vs REGLIST vs IMAGE modes |
| [docs/agents/skill-ee-system.md](docs/agents/skill-ee-system.md) | **PS2 DMA system** | DMA channels, tags, GIF packet format, VIF decompression |
| [docs/agents/skill-gs-rendering.md](docs/agents/skill-gs-rendering.md) | **GS rendering** | VRAM layout, primitives, pixel tests, alpha blending formula |

---

## Debugging Checklist

When encountering DMA/GS issues, check the following:

### ✅ Pre-Flight Checks

- [ ] Called `builder.begin()` before adding data?
- [ ] Buffer size under 500 qwords? (Check console for "Buffer overflow!" warnings)
- [ ] Using correct pattern for your use case? (A=GS registers, B=A+D drawing, C=REGLIST batching)

### ✅ GS Register Issues

- [ ] `GS_SET_TEXFLUSH` written after texture/CLUT upload?
- [ ] `ZTE=1` in TEST register? (Never use `ZTE=0`)
- [ ] Alpha values use `0x80` for 1.0? (Not `0xFF`)
- [ ] XYZ2 written **last** in drawing sequence? (After UV, RGBAQ, etc.)

### ✅ REGLIST Mode Issues

- [ ] Using `addGifLoopTag` (not `addGifTag`) for REGLIST?
- [ ] NLOOP count matches actual loop iterations?
- [ ] Using `GS_SET_UV`/`GS_SET_XYZ` **only** in REGLIST mode? (Never in PACKED A+D)
- [ ] Packing correct number of registers per qword? (2 for UV+XYZ)

### ✅ DMA Transfer Issues

- [ ] `FlushCache(0)` called? (Automatic in `send()`/`sendAsync()`)
- [ ] Buffer 64-byte aligned? (Automatic in constructor via `memalign(64, ...)`)
- [ ] Waiting for DMA completion? (Use `send(waitCycles)` not `sendAsync()` if next code depends on transfer)

### ✅ Memory Issues

- [ ] Freeing `memalign`'d memory with `free()` (not `delete`)?
- [ ] No double-free on `DmaGifBuilder` destruction?
- [ ] Stack buffers using `ALIGNED(64)` attribute if passed to DMA?

### ✅ GS Lockup Recovery

If the GS is frozen (black screen, no response):

1. **Check NLOOP mismatch** — most common cause. Verify loop counts in REGLIST mode.
2. **Check incomplete packet** — did you call `send()` after adding data?
3. **Check texture cache** — missing `TEXFLUSH` after upload?
4. **Reset GS** — power cycle the PS2/PCSX2 emulator.

---

*Last updated: 2026-02-08 — v0.86.140-pre-alpha*