#pragma once
#include <tamtypes.h>
#include <functional>
#include <vector>

using TickTaskId = u16;
constexpr TickTaskId INVALID_TICK_TASK = 0;

// RAII wrapper para auto-cancelamento de callbacks
// Uso: armazenar em membro da classe que registra callbacks
class TickTaskHandle {
 public:
  TickTaskHandle() : id(INVALID_TICK_TASK), scheduler(nullptr) {}
  TickTaskHandle(TickTaskId taskId, class TickScheduler* sched)
      : id(taskId), scheduler(sched) {}
  
  ~TickTaskHandle();

  // Move-only (não copiável para evitar double-free)
  TickTaskHandle(const TickTaskHandle&) = delete;
  TickTaskHandle& operator=(const TickTaskHandle&) = delete;
  TickTaskHandle(TickTaskHandle&& other);
  TickTaskHandle& operator=(TickTaskHandle&& other);

  // Cancelar manualmente
  void cancel();
  
  // Verificar se ainda é válido
  bool isValid() const { return id != INVALID_TICK_TASK && scheduler != nullptr; }
  
  TickTaskId getId() const { return id; }

 private:
  TickTaskId id;
  class TickScheduler* scheduler;
};

// Container RAII para múltiplos handles
class TickTaskHandles {
 public:
  TickTaskHandles() = default;
  ~TickTaskHandles() { cancelAll(); }
  
  // Não copiável
  TickTaskHandles(const TickTaskHandles&) = delete;
  TickTaskHandles& operator=(const TickTaskHandles&) = delete;
  
  void add(TickTaskHandle&& handle) {
    handles.push_back(std::move(handle));
  }
  
  void cancelAll() {
    for (auto& h : handles) h.cancel();
    handles.clear();
  }
  
 private:
  std::vector<TickTaskHandle> handles;
};

class TickScheduler {
 public:
  TickScheduler();
  ~TickScheduler();

  // Registrar callback recorrente a cada N ticks
  // IMPORTANTE: Use TickTaskHandle para garantir cancelamento automático!
  TickTaskId every(u16 interval, std::function<void()> callback);

  // Registrar callback one-shot após N ticks
  TickTaskId after(u32 delay, std::function<void()> callback);

  // Versões que retornam handles RAII (PREFERIR ESTAS)
  TickTaskHandle everyHandle(u16 interval, std::function<void()> callback);
  TickTaskHandle afterHandle(u32 delay, std::function<void()> callback);

  // Cancelar um callback agendado
  void cancel(TickTaskId id);

  // Limpar todos os callbacks
  void cancelAll();

  // Processar um tick — chamado pelo TickManager
  void tick();

  // Ajustar contadores após wrap do dia
  void handleCounterWrap();

  // Read-only introspection for debug display
  struct TaskInfo {
    TickTaskId id;
    u16 interval;
    u16 counter;
    u8 active;
    u8 oneShot;
  };

  int getTaskCount() const;
  int getActiveTaskCount() const;
  bool getTaskInfo(int index, TaskInfo& out) const;

 private:
  struct TickTask {
    std::function<void()> callback;
    TickTaskId id;
    u16 interval;    // 0 = one-shot
    u16 counter;     // conta de 0 até interval
    u8 active;
    u8 oneShot;
  };

  std::vector<TickTask> tasks;
  TickTaskId nextId;
  u8 isIterating;
  u8 hasCancelled;

  TickTaskId generateId();
  void compactIfNeeded();
};
