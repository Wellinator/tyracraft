#include "managers/tick_scheduler.hpp"

// ===== TickTaskHandle Implementation =====
TickTaskHandle::~TickTaskHandle() { cancel(); }

TickTaskHandle::TickTaskHandle(TickTaskHandle&& other)
    : id(other.id), scheduler(other.scheduler) {
  other.id = INVALID_TICK_TASK;
  other.scheduler = nullptr;
}

TickTaskHandle& TickTaskHandle::operator=(TickTaskHandle&& other) {
  if (this != &other) {
    cancel();  // Cancel current task
    id = other.id;
    scheduler = other.scheduler;
    other.id = INVALID_TICK_TASK;
    other.scheduler = nullptr;
  }
  return *this;
}

void TickTaskHandle::cancel() {
  if (isValid()) {
    scheduler->cancel(id);
    id = INVALID_TICK_TASK;
    scheduler = nullptr;
  }
}

// ===== TickScheduler Implementation =====
TickScheduler::TickScheduler() : nextId(1), isIterating(0), hasCancelled(0) {
  tasks.reserve(32);  // Pre-allocate space for typical usage
}

TickScheduler::~TickScheduler() {
  cancelAll();
}

TickTaskId TickScheduler::every(u16 interval, std::function<void()> callback) {
  if (interval == 0) return INVALID_TICK_TASK;

  TickTask task;
  task.callback = callback;
  task.id = generateId();
  task.interval = interval;
  task.counter = 0;
  task.active = 1;
  task.oneShot = 0;

  tasks.push_back(task);
  return task.id;
}

TickTaskId TickScheduler::after(u32 delay, std::function<void()> callback) {
  if (delay == 0) return INVALID_TICK_TASK;

  TickTask task;
  task.callback = callback;
  task.id = generateId();
  task.interval = static_cast<u16>(delay);
  task.counter = 0;
  task.active = 1;
  task.oneShot = 1;

  tasks.push_back(task);
  return task.id;
}

TickTaskHandle TickScheduler::everyHandle(u16 interval,
                                          std::function<void()> callback) {
  TickTaskId id = every(interval, callback);
  return TickTaskHandle(id, this);
}

TickTaskHandle TickScheduler::afterHandle(u32 delay,
                                          std::function<void()> callback) {
  TickTaskId id = after(delay, callback);
  return TickTaskHandle(id, this);
}

void TickScheduler::cancel(TickTaskId id) {
  if (id == INVALID_TICK_TASK) return;

  for (size_t i = 0; i < tasks.size(); i++) {
    if (tasks[i].id == id) {
      tasks[i].active = 0;
      hasCancelled = 1;
      break;
    }
  }
}

void TickScheduler::cancelAll() {
  tasks.clear();
  nextId = 1;
  hasCancelled = 0;
}

void TickScheduler::tick() {
  isIterating = 1;
  
  // Criar snapshot das tasks ativas para evitar problemas se callbacks
  // modificarem o vetor ou cancelarem outras tasks
  std::vector<TickTask*> activeSnapshot;
  activeSnapshot.reserve(tasks.size());
  
  for (size_t i = 0; i < tasks.size(); i++) {
    if (tasks[i].active) {
      activeSnapshot.push_back(&tasks[i]);
    }
  }

  // Processar o snapshot
  for (size_t i = 0; i < activeSnapshot.size(); i++) {
    TickTask* task = activeSnapshot[i];
    
    // Verificar se ainda está ativa (pode ter sido cancelada por callback anterior)
    if (!task->active) continue;

    task->counter++;
    if (task->counter >= task->interval) {
      // Execute callback - ATENÇÃO: pode modificar tasks!
      try {
        task->callback();
      } catch (...) {
        // Proteção contra exceções - no PS2 isso é crítico
        task->active = 0;
        hasCancelled = 1;
        continue;
      }

      // Verificar novamente se ainda está ativa (callback pode ter cancelado)
      if (task->active) {
        // Reset or deactivate
        if (task->oneShot) {
          task->active = 0;
          hasCancelled = 1;
        } else {
          task->counter = 0;
        }
      }
    }
  }

  isIterating = 0;
  compactIfNeeded();
}

void TickScheduler::handleCounterWrap() {
  // Neste design, os contadores são relativos, então não há necessidade de ajuste
  // Este método está disponível para extensão futura se necessário
}

TickTaskId TickScheduler::generateId() {
  if (nextId == 0) nextId = 1;  // Skip INVALID_TICK_TASK
  return nextId++;
}

void TickScheduler::compactIfNeeded() {
  if (!hasCancelled) return;

  // Remove inactive tasks
  size_t writeIndex = 0;
  for (size_t readIndex = 0; readIndex < tasks.size(); readIndex++) {
    if (tasks[readIndex].active) {
      if (writeIndex != readIndex) {
        tasks[writeIndex] = std::move(tasks[readIndex]);
      }
      writeIndex++;
    }
  }

  tasks.resize(writeIndex);
  hasCancelled = 0;
}

int TickScheduler::getTaskCount() const {
  return static_cast<int>(tasks.size());
}

int TickScheduler::getActiveTaskCount() const {
  int count = 0;
  for (size_t i = 0; i < tasks.size(); i++) {
    if (tasks[i].active) count++;
  }
  return count;
}

bool TickScheduler::getTaskInfo(int index, TaskInfo& out) const {
  if (index < 0 || index >= static_cast<int>(tasks.size())) return false;
  out.id = tasks[index].id;
  out.interval = tasks[index].interval;
  out.counter = tasks[index].counter;
  out.active = tasks[index].active;
  out.oneShot = tasks[index].oneShot;
  return true;
}
