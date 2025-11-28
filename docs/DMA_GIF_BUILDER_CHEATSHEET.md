🎯 DmaGifBuilder - Quick Reference Cheat Sheet

## 📌 Quick Reference

### Include Header
```cpp
#include "managers/post-fx/dma_gif_builder.hpp"
```

---

## 🚀 Common Usage Patterns

### 1. Basic Setup (Auto-Count)
```cpp
DmaGifBuilder builder;
builder.begin();
builder.addGifTag(0, GIF_REG_AD);  // 0 = auto-count
builder.addAd(GS_SET_TEST(...), GS_REG_TEST_1);
builder.addAd(GS_SET_ZBUF(...), GS_REG_ZBUF_1);
builder.send();
```

### 2. Setup with Manual Count
```cpp
DmaGifBuilder builder;
builder.begin();
builder.addGifTag(3, GIF_REG_AD);  // Manual count
builder.addAd(GS_SET_TEST(...), GS_REG_TEST_1);
builder.addAd(GS_SET_ZBUF(...), GS_REG_ZBUF_1);
builder.addAd(GS_SET_FRAME(...), GS_REG_FRAME_1);
builder.send();
```

### 3. Multiple GIF Tags
```cpp
DmaGifBuilder builder;
builder.begin();

// Tag 1
builder.addGifTag(0, GIF_REG_AD);
builder.addAd(...);
builder.addAd(...);

// Tag 2 (Tag 1 auto-finalized)
builder.addGifTag(0, GIF_REG_AD);
builder.addAd(...);

builder.send();
```

### 4. Multiple Passes
```cpp
DmaGifBuilder builder;

// Pass 1
builder.begin();
builder.addGifTag(0, GIF_REG_AD);
builder.addAd(...);
builder.send();

// Pass 2
builder.begin();  // Auto-reset
builder.addGifTag(0, GIF_REG_AD);
builder.addAd(...);
builder.send();
```

### 5. Sprite Loop
```cpp
DmaGifBuilder builder;
builder.begin();

// Setup
builder.addGifTag(0, GIF_REG_AD);
builder.addAd(GS_SET_PRIM(...), GS_REG_PRIM);

// Loop tag
builder.addGifLoopTag(
    96,  // nloop
    1,   // eop
    1,   // pre
    GS_SET_PRIM(...),
    GIF_FLG_PACKED,
    4,   // nreg
    (GIF_REG_UV) | (GIF_REG_XYZ2 << 4) | ...
);

// Sprite data
for (int i = 0; i < 96; i++) {
    builder.addRaw(uv, 0);
    builder.addRaw(xyz, 1);
}

builder.send();
```

### 6. Async Send
```cpp
DmaGifBuilder builder;
builder.begin();
builder.addGifTag(0, GIF_REG_AD);
builder.addAd(...);
builder.sendAsync();  // Don't wait
```

### 7. With Macros
```cpp
DmaGifBuilder builder;

DMA_BEGIN(builder);
builder.addGifTag(0, GIF_REG_AD);
DMA_ADD(builder, GS_SET_TEST(...), GS_REG_TEST_1);
DMA_ADD(builder, GS_SET_ZBUF(...), GS_REG_ZBUF_1);
DMA_SEND(builder);
```

### 8. Reuse (Reset)
```cpp
DmaGifBuilder builder;

// First use
builder.begin();
builder.addGifTag(0, GIF_REG_AD);
builder.addAd(...);
builder.send();

// Reuse (optional, begin() already resets)
builder.reset();

// Second use
builder.begin();
builder.addGifTag(0, GIF_REG_AD);
builder.addAd(...);
builder.send();
```

---

## 📋 Complete API

### Methods

| Method | Parameters | Description |
|--------|-----------|-------------|
| `begin()` | - | Initialize/reset builder |
| `addGifTag(count, regs)` | `u32 count`, `u64 regs` | Add GIF tag (count=0 for auto) |
| `addAd(value, reg)` | `u64 value`, `u64 reg` | Add reg/value (A+D mode) |
| `addRaw(low, high)` | `u64 low`, `u64 high` | Add raw qword |
| `addGifLoopTag(...)` | See example 5 | Add loop tag |
| `send(waitCycles)` | `u32 waitCycles=500` | Send and wait |
| `sendAsync()` | - | Send without waiting |
| `reset()` | - | Reset (optional) |
| `getPacketCount()` | - | Return current count |
| `getSize()` | - | Return size in qwords |

### Macros

| Macro | Equivalent | Description |
|-------|------------|-------------|
| `DMA_BEGIN(builder)` | `builder.begin()` | Initialize |
| `DMA_ADD(builder, val, reg)` | `builder.addAd(val, reg)` | Add A+D |
| `DMA_SEND(builder)` | `builder.send()` | Send |
| `DMA_SEND_ASYNC(builder)` | `builder.sendAsync()` | Send async |

---

## 🔄 Quick Conversion

### From Manual to DmaGifBuilder

| Manual | DmaGifBuilder |
|--------|--------------|
| `qword_t packets[N] ALIGNED(64);` | `DmaGifBuilder builder;` |
| `qword_t* q = packets;` | `builder.begin();` |
| `PACK_GIFTAG(q, GIF_SET_TAG(N, ...), ...); q++;` | `builder.addGifTag(0, ...);` |
| `PACK_GIFTAG(q, value, reg); q++;` | `builder.addAd(value, reg);` |
| `q = packets;` (reset) | `builder.begin();` |
| `FlushCache(0);` | (automatic) |
| `dma_channel_send_normal(...);` | `builder.send();` |
| `dma_channel_wait(...);` | (included in send) |

---

## ⚠️ Common Errors

| Error | Cause | Solution |
|------|-------|---------|
| `begin() not called` | Forgot `begin()` | Always call `begin()` first |
| `Buffer overflow!` | Too many packets | Increase `DMA_GIF_BUILDER_MAX_PACKETS` |
| Wrong visual count | Using wrong manual count | Use auto-count (`0`) |
| GS hang | Bug in original code | Verify registers |
| Link error | .cpp not compiled | Check Makefile |

---

## 🎯 Tips & Tricks

### ✅ Do
- ✅ Use auto-count (`0`) whenever possible
- ✅ Call `begin()` before using
- ✅ Reuse one instance for multiple passes
- ✅ Use `getPacketCount()` for debugging
- ✅ Test after refactoring

### ❌ Avoid
- ❌ Forgetting `begin()`
- ❌ Using manual count when unnecessary
- ❌ Creating multiple instances unnecessarily
- ❌ Ignoring overflow warnings

---

## 🐛 Debug

### Printf Debug
```cpp
printf("Packets: %u, Size: %u\n", 
             builder.getPacketCount(), 
             builder.getSize());
```

### Check Overflow
```cpp
builder.addAd(...);
if (builder.getSize() > DMA_GIF_BUILDER_MAX_PACKETS - 10) {
    printf("WARNING: Near buffer limit!\n");
}
```

### Track Passes
```cpp
builder.begin();
printf("[Pass 1] Starting...\n");
// ...
builder.send();
printf("[Pass 1] Sent %u packets\n", builder.getPacketCount());
```

---

## 📊 Visual Comparison

```
BEFORE:                         AFTER:
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
qword_t packets[20] ALIGNED(64); DmaGifBuilder builder;
qword_t* q = packets;            
                                                                builder.begin();

PACK_GIFTAG(q, GIF_SET_TAG(     builder.addGifTag(0, 
    3, 1, 0, 0, GIF_FLG_PACKED,     GIF_REG_AD);
    1), GIF_REG_AD);               
q++;                             

PACK_GIFTAG(q,                   builder.addAd(
    GS_SET_TEST(...),                GS_SET_TEST(...),
    GS_REG_TEST_1);                  GS_REG_TEST_1);
q++;                             

PACK_GIFTAG(q,                   builder.addAd(
    GS_SET_ZBUF(...),                GS_SET_ZBUF(...),
    GS_REG_ZBUF_1);                  GS_REG_ZBUF_1);
q++;                             

PACK_GIFTAG(q,                   builder.addAd(
    GS_SET_FRAME(...),               GS_SET_FRAME(...),
    GS_REG_FRAME_1);                 GS_REG_FRAME_1);
q++;                             

FlushCache(0);                   
dma_channel_send_normal(         builder.send();
    DMA_CHANNEL_GIF, packets,      
    q - packets, 0, 0);            
dma_channel_wait(                
    DMA_CHANNEL_GIF, 500);         
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
16 lines, 8 q++, 1 manual count   10 lines, 0 q++, auto-count
```

---

## 🔍 When to Use

### ✅ Use DmaGifBuilder
- New code
- Multiple GIF tags
- Buggy code
- Sprite loops
- Post-FX passes
- Complex code

### ⚠️ Consider Manual
- Stable legacy code
- Extreme hot path (< 0.01%)
- Trivial setup (< 3 registers)

---

## 📚 Useful Links

- **[Integration Guide](./DMA_GIF_BUILDER_GUIA_INTEGRACAO.md)** - Complete step-by-step
- **[Complete Documentation](./DMA_GIF_BUILDER.md)** - API reference
- **[Technical Comparison](./DMA_GIF_BUILDER_COMPARACAO.md)** - Detailed analysis
- **[Examples](./dma_gif_builder_examples.cpp)** - Example code
- **[Tests](./dma_gif_builder_test.cpp)** - Test suite

---

## 💾 Template to Copy

```cpp
// ===== DmaGifBuilder Template =====

#include "managers/post-fx/dma_gif_builder.hpp"

void MyClass::myMethod() {
    DmaGifBuilder builder;
    
    // Pass 1
    builder.begin();
    builder.addGifTag(0, GIF_REG_AD);
    builder.addAd(GS_SET_TEST(...), GS_REG_TEST_1);
    builder.addAd(GS_SET_ZBUF(...), GS_REG_ZBUF_1);
    builder.send();
    
    // Pass 2
    builder.begin();
    builder.addGifTag(0, GIF_REG_AD);
    builder.addAd(GS_SET_FRAME(...), GS_REG_FRAME_1);
    builder.send();
}
```

---

## 🎓 One-Page Summary

```
┌────────────────────────────────────────────────────────────┐
│            DMAGIFBUILDER - CHEAT SHEET                     │
├────────────────────────────────────────────────────────────┤
│                                                            │
│  #include "managers/post-fx/dma_gif_builder.hpp"          │
│                                                            │
│  DmaGifBuilder builder;                                    │
│                                                            │
│  builder.begin();                 // Initialize           │
│  builder.addGifTag(0, GIF_REG_AD); // Auto-count          │
│  builder.addAd(val, reg);         // Add A+D              │
│  builder.send();                  // Send                 │
│                                                            │
│  ────────────────────────────────────────────────────     │
│                                                            │
│  QUICK CONVERSION:                                         │
│                                                            │
│  ❌ qword_t* q = packets;                                 │
│  ✅ builder.begin();                                       │
│                                                            │
│  ❌ PACK_GIFTAG(q, GIF_SET_TAG(N, ...), ...); q++;        │
│  ✅ builder.addGifTag(0, ...);                             │
│                                                            │
│  ❌ PACK_GIFTAG(q, val, reg); q++;                        │
│  ✅ builder.addAd(val, reg);                               │
│                                                            │
│  ❌ dma_channel_send_normal(...);                         │
│  ❌ dma_channel_wait(...);                                │
│  ✅ builder.send();                                        │
│                                                            │
│  ────────────────────────────────────────────────────     │
│                                                            │
│  BENEFITS:                                                 │
│  • Zero counting bugs                                     │
│  • 35% less code                                          │
│  • More readable                                          │
│  • Automatic cache flush                                  │
│  • Overflow checking                                      │
│                                                            │
└────────────────────────────────────────────────────────────┘
```

---

**💡 Tip:** Print this page and keep it nearby while coding!

---

📅 **Last updated:** 2025-01-18  
👤 **Author:** Wellinator (TyraCraft)  
📚 **Complete documentation:** [docs/README.md](./README.md)
 🎯 DmaGifBuilder - Folha de Referência Rápida (Cheat Sheet)

## 📌 Quick Reference

### Incluir Header
```cpp
#include "managers/post-fx/dma_gif_builder.hpp"
```

---

## 🚀 Padrões de Uso Comuns

### 1. Setup Básico (Auto-Count)
```cpp
DmaGifBuilder builder;
builder.begin();
builder.addGifTag(0, GIF_REG_AD);  // 0 = auto-count
builder.addAd(GS_SET_TEST(...), GS_REG_TEST_1);
builder.addAd(GS_SET_ZBUF(...), GS_REG_ZBUF_1);
builder.send();
```

### 2. Setup com Count Manual
```cpp
DmaGifBuilder builder;
builder.begin();
builder.addGifTag(3, GIF_REG_AD);  // Manual count
builder.addAd(GS_SET_TEST(...), GS_REG_TEST_1);
builder.addAd(GS_SET_ZBUF(...), GS_REG_ZBUF_1);
builder.addAd(GS_SET_FRAME(...), GS_REG_FRAME_1);
builder.send();
```

### 3. Múltiplos GIF Tags
```cpp
DmaGifBuilder builder;
builder.begin();

// Tag 1
builder.addGifTag(0, GIF_REG_AD);
builder.addAd(...);
builder.addAd(...);

// Tag 2 (Tag 1 auto-finalizado)
builder.addGifTag(0, GIF_REG_AD);
builder.addAd(...);

builder.send();
```

### 4. Múltiplos Passes
```cpp
DmaGifBuilder builder;

// Pass 1
builder.begin();
builder.addGifTag(0, GIF_REG_AD);
builder.addAd(...);
builder.send();

// Pass 2
builder.begin();  // Auto-reset
builder.addGifTag(0, GIF_REG_AD);
builder.addAd(...);
builder.send();
```

### 5. Sprite Loop
```cpp
DmaGifBuilder builder;
builder.begin();

// Setup
builder.addGifTag(0, GIF_REG_AD);
builder.addAd(GS_SET_PRIM(...), GS_REG_PRIM);

// Loop tag
builder.addGifLoopTag(
  96,  // nloop
  1,   // eop
  1,   // pre
  GS_SET_PRIM(...),
  GIF_FLG_PACKED,
  4,   // nreg
  (GIF_REG_UV) | (GIF_REG_XYZ2 << 4) | ...
);

// Sprite data
for (int i = 0; i < 96; i++) {
  builder.addRaw(uv, 0);
  builder.addRaw(xyz, 1);
}

builder.send();
```

### 6. Envio Assíncrono
```cpp
DmaGifBuilder builder;
builder.begin();
builder.addGifTag(0, GIF_REG_AD);
builder.addAd(...);
builder.sendAsync();  // Não aguarda
```

### 7. Com Macros
```cpp
DmaGifBuilder builder;

DMA_BEGIN(builder);
builder.addGifTag(0, GIF_REG_AD);
DMA_ADD(builder, GS_SET_TEST(...), GS_REG_TEST_1);
DMA_ADD(builder, GS_SET_ZBUF(...), GS_REG_ZBUF_1);
DMA_SEND(builder);
```

### 8. Reutilização (Reset)
```cpp
DmaGifBuilder builder;

// Primeiro uso
builder.begin();
builder.addGifTag(0, GIF_REG_AD);
builder.addAd(...);
builder.send();

// Reutilizar (opcional, begin() já faz reset)
builder.reset();

// Segundo uso
builder.begin();
builder.addGifTag(0, GIF_REG_AD);
builder.addAd(...);
builder.send();
```

---

## 📋 API Completa

### Métodos

| Método | Parâmetros | Descrição |
|--------|-----------|-----------|
| `begin()` | - | Inicia/reseta builder |
| `addGifTag(count, regs)` | `u32 count`, `u64 regs` | Adiciona GIF tag (count=0 para auto) |
| `addAd(value, reg)` | `u64 value`, `u64 reg` | Adiciona reg/value (A+D mode) |
| `addRaw(low, high)` | `u64 low`, `u64 high` | Adiciona qword raw |
| `addGifLoopTag(...)` | Ver exemplo 5 | Adiciona loop tag |
| `send(waitCycles)` | `u32 waitCycles=500` | Envia e aguarda |
| `sendAsync()` | - | Envia sem aguardar |
| `reset()` | - | Reseta (opcional) |
| `getPacketCount()` | - | Retorna count atual |
| `getSize()` | - | Retorna size em qwords |

### Macros

| Macro | Equivalente | Descrição |
|-------|------------|-----------|
| `DMA_BEGIN(builder)` | `builder.begin()` | Inicia |
| `DMA_ADD(builder, val, reg)` | `builder.addAd(val, reg)` | Adiciona A+D |
| `DMA_SEND(builder)` | `builder.send()` | Envia |
| `DMA_SEND_ASYNC(builder)` | `builder.sendAsync()` | Envia async |

---

## 🔄 Conversão Rápida

### De Manual para DmaGifBuilder

| Manual | DmaGifBuilder |
|--------|--------------|
| `qword_t packets[N] ALIGNED(64);` | `DmaGifBuilder builder;` |
| `qword_t* q = packets;` | `builder.begin();` |
| `PACK_GIFTAG(q, GIF_SET_TAG(N, ...), ...); q++;` | `builder.addGifTag(0, ...);` |
| `PACK_GIFTAG(q, value, reg); q++;` | `builder.addAd(value, reg);` |
| `q = packets;` (reset) | `builder.begin();` |
| `FlushCache(0);` | (automático) |
| `dma_channel_send_normal(...);` | `builder.send();` |
| `dma_channel_wait(...);` | (incluído em send) |

---

## ⚠️ Erros Comuns

| Erro | Causa | Solução |
|------|-------|---------|
| `begin() not called` | Esquecer `begin()` | Sempre chame `begin()` primeiro |
| `Buffer overflow!` | Muitos pacotes | Aumente `DMA_GIF_BUILDER_MAX_PACKETS` |
| Count errado visual | Usar count manual errado | Use auto-count (`0`) |
| GS hang | Bug no código original | Verifique registradores |
| Link error | .cpp não compilado | Verifique Makefile |

---

## 🎯 Dicas & Truques

### ✅ Faça
- ✅ Use auto-count (`0`) sempre que possível
- ✅ Chame `begin()` antes de usar
- ✅ Reutilize uma instância para múltiplos passes
- ✅ Use `getPacketCount()` para debug
- ✅ Teste após refatorar

### ❌ Evite
- ❌ Esquecer `begin()`
- ❌ Usar count manual quando não precisa
- ❌ Criar múltiplas instâncias desnecessariamente
- ❌ Ignorar warnings de overflow

---

## 🐛 Debug

### Printf Debug
```cpp
printf("Packets: %u, Size: %u\n", 
       builder.getPacketCount(), 
       builder.getSize());
```

### Verificar Overflow
```cpp
builder.addAd(...);
if (builder.getSize() > DMA_GIF_BUILDER_MAX_PACKETS - 10) {
  printf("WARNING: Near buffer limit!\n");
}
```

### Rastrear Passes
```cpp
builder.begin();
printf("[Pass 1] Starting...\n");
// ...
builder.send();
printf("[Pass 1] Sent %u packets\n", builder.getPacketCount());
```

---

## 📊 Comparação Visual

```
ANTES:                          DEPOIS:
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
qword_t packets[20] ALIGNED(64); DmaGifBuilder builder;
qword_t* q = packets;            
                                builder.begin();

PACK_GIFTAG(q, GIF_SET_TAG(     builder.addGifTag(0, 
  3, 1, 0, 0, GIF_FLG_PACKED,     GIF_REG_AD);
  1), GIF_REG_AD);               
q++;                             

PACK_GIFTAG(q,                   builder.addAd(
  GS_SET_TEST(...),                GS_SET_TEST(...),
  GS_REG_TEST_1);                  GS_REG_TEST_1);
q++;                             

PACK_GIFTAG(q,                   builder.addAd(
  GS_SET_ZBUF(...),                GS_SET_ZBUF(...),
  GS_REG_ZBUF_1);                  GS_REG_ZBUF_1);
q++;                             

PACK_GIFTAG(q,                   builder.addAd(
  GS_SET_FRAME(...),               GS_SET_FRAME(...),
  GS_REG_FRAME_1);                 GS_REG_FRAME_1);
q++;                             

FlushCache(0);                   
dma_channel_send_normal(         builder.send();
  DMA_CHANNEL_GIF, packets,      
  q - packets, 0, 0);            
dma_channel_wait(                
  DMA_CHANNEL_GIF, 500);         
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
16 linhas, 8 q++, 1 count manual  10 linhas, 0 q++, auto-count
```

---

## 🔍 Quando Usar

### ✅ Use DmaGifBuilder
- Código novo
- Múltiplos GIF tags
- Código com bugs
- Sprite loops
- Post-FX passes
- Código complexo

### ⚠️ Considere Manual
- Código legado estável
- Hot path extremo (< 0.01%)
- Setup trivial (< 3 registradores)

---

## 📚 Links Úteis

- **[Guia de Integração](./DMA_GIF_BUILDER_GUIA_INTEGRACAO.md)** - Passo a passo completo
- **[Documentação Completa](./DMA_GIF_BUILDER.md)** - API reference
- **[Comparação Técnica](./DMA_GIF_BUILDER_COMPARACAO.md)** - Análise detalhada
- **[Exemplos](./dma_gif_builder_examples.cpp)** - Código de exemplo
- **[Testes](./dma_gif_builder_test.cpp)** - Suite de testes

---

## 💾 Template para Copiar

```cpp
// ===== DmaGifBuilder Template =====

#include "managers/post-fx/dma_gif_builder.hpp"

void MyClass::myMethod() {
  DmaGifBuilder builder;
  
  // Pass 1
  builder.begin();
  builder.addGifTag(0, GIF_REG_AD);
  builder.addAd(GS_SET_TEST(...), GS_REG_TEST_1);
  builder.addAd(GS_SET_ZBUF(...), GS_REG_ZBUF_1);
  builder.send();
  
  // Pass 2
  builder.begin();
  builder.addGifTag(0, GIF_REG_AD);
  builder.addAd(GS_SET_FRAME(...), GS_REG_FRAME_1);
  builder.send();
}
```

---

## 🎓 Resumo em Uma Página

```
┌────────────────────────────────────────────────────────────┐
│            DMAGIFBUILDER - CHEAT SHEET                     │
├────────────────────────────────────────────────────────────┤
│                                                            │
│  #include "managers/post-fx/dma_gif_builder.hpp"          │
│                                                            │
│  DmaGifBuilder builder;                                    │
│                                                            │
│  builder.begin();                 // Iniciar              │
│  builder.addGifTag(0, GIF_REG_AD); // Auto-count          │
│  builder.addAd(val, reg);         // Adicionar A+D        │
│  builder.send();                  // Enviar               │
│                                                            │
│  ────────────────────────────────────────────────────     │
│                                                            │
│  CONVERSÃO RÁPIDA:                                         │
│                                                            │
│  ❌ qword_t* q = packets;                                 │
│  ✅ builder.begin();                                       │
│                                                            │
│  ❌ PACK_GIFTAG(q, GIF_SET_TAG(N, ...), ...); q++;        │
│  ✅ builder.addGifTag(0, ...);                             │
│                                                            │
│  ❌ PACK_GIFTAG(q, val, reg); q++;                        │
│  ✅ builder.addAd(val, reg);                               │
│                                                            │
│  ❌ dma_channel_send_normal(...);                         │
│  ❌ dma_channel_wait(...);                                │
│  ✅ builder.send();                                        │
│                                                            │
│  ────────────────────────────────────────────────────     │
│                                                            │
│  BENEFÍCIOS:                                               │
│  • Zero bugs de contagem                                  │
│  • 35% menos código                                       │
│  • Mais legível                                           │
│  • Cache flush automático                                 │
│  • Verificação de overflow                                │
│                                                            │
└────────────────────────────────────────────────────────────┘
```

---

**💡 Dica:** Imprima esta página e mantenha ao lado enquanto programa!

---

📅 **Última atualização:** 2025-01-18  
👤 **Autor:** Wellinator (TyraCraft)  
📚 **Documentação completa:** [docs/README.md](./README.md)
