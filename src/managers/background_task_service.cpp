#include "managers/background_task_service.hpp"
#include <tyra>
#include <cstring>

static u8 worker_stack[8 * 1024] ALIGNED(16);
static u8 dispatcher_stack[8 * 1024] ALIGNED(16);

// --- Dispatcher thread: sleeps and gets woken by alarm for scheduling ---
void BackgroundTaskService::dispatcherEntry(void* arg) {
  while (1) {
    SleepThread();
  }
}

// --- Worker thread: waits for work, executes it, pushes completion ---
void BackgroundTaskService::workerEntry(void* arg) {
  auto* svc = BackgroundTaskService::getInstance();

  while (1) {
    WaitSema(svc->workAvailable);

    if (svc->shouldExit) {
      ExitThread();
      return;
    }

    // Dequeue work item
    WaitSema(svc->queueMutex);
    int head = svc->queueHead;
    BgWorkItem item = svc->queue[head];
    svc->queue[head].state = static_cast<u8>(BgTaskState::Running);
    svc->queueHead = (head + 1) % QUEUE_CAPACITY;
    SignalSema(svc->queueMutex);

    // Execute the work
    if (item.work) {
      item.work();
    }

    // Push to completion queue
    WaitSema(svc->doneMutex);
    svc->doneQueue[svc->doneTail] = item.id;
    svc->doneTail = (svc->doneTail + 1) % DONE_CAPACITY;
    SignalSema(svc->doneMutex);

    // Mark completed in the original queue slot
    WaitSema(svc->queueMutex);
    // Find the item by id to update state (it may have moved)
    for (int i = 0; i < QUEUE_CAPACITY; i++) {
      if (svc->queue[i].id == item.id &&
          svc->queue[i].state == static_cast<u8>(BgTaskState::Running)) {
        svc->queue[i].state = static_cast<u8>(BgTaskState::Completed);
        break;
      }
    }
    SignalSema(svc->queueMutex);
  }
}

// --- Alarm callback: preemptive scheduling via thread rotation ---
void BackgroundTaskService::alarmCallback(s32 id, u16 time, void* arg) {
  auto* svc = BackgroundTaskService::getInstance();
  iWakeupThread(svc->dispatcherThreadId);
  iRotateThreadReadyQueue(WORKER_PRIORITY);
  iSetAlarm(ALARM_CYCLE, BackgroundTaskService::alarmCallback, NULL);
}

// --- Constructor ---
BackgroundTaskService::BackgroundTaskService()
    : Singleton<BackgroundTaskService>() {
  // Init ring buffers
  queueHead = 0;
  queueTail = 0;
  doneHead = 0;
  doneTail = 0;
  shouldExit = false;
  nextId = 1;
  alarmId = -1;
  memset(queue, 0, sizeof(queue));
  memset(doneQueue, 0, sizeof(doneQueue));

  // Create semaphores
  ee_sema_t sema;

  // Queue mutex (binary semaphore)
  sema.init_count = 1;
  sema.max_count = 1;
  sema.attr = 0;
  sema.option = 0;
  queueMutex = CreateSema(&sema);
  TYRA_ASSERT(queueMutex >= 0, "Failed to create queue mutex semaphore!");

  // Done mutex (binary semaphore)
  sema.init_count = 1;
  sema.max_count = 1;
  doneMutex = CreateSema(&sema);
  TYRA_ASSERT(doneMutex >= 0, "Failed to create done mutex semaphore!");

  // Work available (counting semaphore)
  sema.init_count = 0;
  sema.max_count = QUEUE_CAPACITY;
  workAvailable = CreateSema(&sema);
  TYRA_ASSERT(workAvailable >= 0,
              "Failed to create work available semaphore!");

  // Create dispatcher thread
  dispatcherThread.attr = 0;
  dispatcherThread.option = 0;
  dispatcherThread.func = (void*)dispatcherEntry;
  dispatcherThread.stack = dispatcher_stack;
  dispatcherThread.stack_size = 8 * 1024;
  dispatcherThread.gp_reg = &_gp;
  dispatcherThread.initial_priority = DISPATCHER_PRIORITY;

  dispatcherThreadId = CreateThread(&dispatcherThread);
  TYRA_ASSERT(dispatcherThreadId >= 0,
              "Failed to create dispatcher thread!");
  TYRA_ASSERT(StartThread(dispatcherThreadId, NULL) >= 0,
              "Failed to start dispatcher thread!");

  // Create worker thread
  workerThread.attr = 0;
  workerThread.option = 0;
  workerThread.func = (void*)workerEntry;
  workerThread.stack = worker_stack;
  workerThread.stack_size = 8 * 1024;
  workerThread.gp_reg = &_gp;
  workerThread.initial_priority = WORKER_PRIORITY;

  workerThreadId = CreateThread(&workerThread);
  TYRA_ASSERT(workerThreadId >= 0, "Failed to create worker thread!");
  TYRA_ASSERT(StartThread(workerThreadId, NULL) >= 0,
              "Failed to start worker thread!");

  // Start alarm for preemptive scheduling
  alarmId = SetAlarm(ALARM_CYCLE, BackgroundTaskService::alarmCallback, NULL);
  TYRA_ASSERT(alarmId >= 0, "Failed to create scheduling alarm!");

  TYRA_LOG("BackgroundTaskService initialized");
}

// --- Destructor ---
BackgroundTaskService::~BackgroundTaskService() {
  // Signal worker to exit
  shouldExit = true;
  SignalSema(workAvailable);

  // Give worker time to exit gracefully
  ee_thread_status_t status;
  for (int i = 0; i < 1000; i++) {
    ReferThreadStatus(workerThreadId, &status);
    if (status.status == THS_DORMANT) break;
  }

  // Clean up threads
  TerminateThread(workerThreadId);
  DeleteThread(workerThreadId);
  TerminateThread(dispatcherThreadId);
  DeleteThread(dispatcherThreadId);

  // Release alarm
  if (alarmId >= 0) {
    ReleaseAlarm(alarmId);
    alarmId = -1;
  }

  // Delete semaphores
  DeleteSema(queueMutex);
  DeleteSema(doneMutex);
  DeleteSema(workAvailable);

  TYRA_LOG("BackgroundTaskService shutdown");
}

// --- Submit work (main thread, non-blocking) ---
BgTaskId BackgroundTaskService::submit(std::function<void()> work,
                                       std::function<void()> onComplete) {
  if (isQueueFull()) {
    TYRA_WARN("BackgroundTaskService: queue is full, task rejected!");
    return INVALID_BG_TASK;
  }

  BgTaskId id = generateId();

  WaitSema(queueMutex);
  BgWorkItem& item = queue[queueTail];
  item.work = work;
  item.onComplete = onComplete;
  item.id = id;
  item.state = static_cast<u8>(BgTaskState::Pending);
  queueTail = (queueTail + 1) % QUEUE_CAPACITY;
  SignalSema(queueMutex);

  // Wake up worker thread
  SignalSema(workAvailable);

  return id;
}

// --- Poll completions (main thread) ---
void BackgroundTaskService::pollCompletions() {
  WaitSema(doneMutex);

  while (doneHead != doneTail) {
    BgTaskId id = doneQueue[doneHead];
    doneHead = (doneHead + 1) % DONE_CAPACITY;

    // Find and extract onComplete callback
    std::function<void()> callback = nullptr;
    WaitSema(queueMutex);
    BgWorkItem* item = findItemById(id);
    if (item) {
      callback = item->onComplete;
      // Clear the slot
      item->work = nullptr;
      item->onComplete = nullptr;
      item->id = INVALID_BG_TASK;
      item->state = 0;
    }
    SignalSema(queueMutex);

    // Fire callback outside of locks
    if (callback) {
      SignalSema(doneMutex);
      callback();
      WaitSema(doneMutex);
    }
  }

  SignalSema(doneMutex);
}

// --- Get task state ---
BgTaskState BackgroundTaskService::getTaskState(BgTaskId id) {
  WaitSema(queueMutex);
  BgWorkItem* item = findItemById(id);
  BgTaskState state =
      item ? static_cast<BgTaskState>(item->state) : BgTaskState::NotFound;
  SignalSema(queueMutex);
  return state;
}

// --- Get pending count ---
int BackgroundTaskService::getPendingCount() {
  WaitSema(queueMutex);
  int count = (queueTail - queueHead + QUEUE_CAPACITY) % QUEUE_CAPACITY;
  SignalSema(queueMutex);
  return count;
}

// --- Internal helpers ---
BgTaskId BackgroundTaskService::generateId() {
  BgTaskId id = nextId++;
  if (nextId == INVALID_BG_TASK) nextId = 1;
  return id;
}

BgWorkItem* BackgroundTaskService::findItemById(BgTaskId id) {
  for (int i = 0; i < QUEUE_CAPACITY; i++) {
    if (queue[i].id == id) return &queue[i];
  }
  return nullptr;
}

bool BackgroundTaskService::isQueueFull() const {
  return ((queueTail + 1) % QUEUE_CAPACITY) == queueHead;
}

bool BackgroundTaskService::isQueueEmpty() const {
  return queueHead == queueTail;
}
