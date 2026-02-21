#pragma once

#include <tamtypes.h>
#include <functional>
#include "singleton.hpp"
#include <kernel.h>

using BgTaskId = u16;
constexpr BgTaskId INVALID_BG_TASK = 0;

enum class BgTaskState : u8 {
  Pending,
  Running,
  Completed,
  NotFound
};

struct BgWorkItem {
  std::function<void()> work;
  std::function<void()> onComplete;
  BgTaskId id;
  volatile u8 state;  // BgTaskState cast to u8
};

class BackgroundTaskService : public Singleton<BackgroundTaskService> {
 public:
  BackgroundTaskService();
  ~BackgroundTaskService();

  /**
   * Submit work to be executed on the worker thread.
   * Non-blocking. Returns INVALID_BG_TASK if the queue is full.
   * @param work Function to execute on worker thread
   * @param onComplete Optional callback fired on main thread via pollCompletions()
   */
  BgTaskId submit(std::function<void()> work,
                  std::function<void()> onComplete = nullptr);

  /** Check the state of a submitted task */
  BgTaskState getTaskState(BgTaskId id);

  /**
   * Process completed tasks and fire onComplete callbacks.
   * Must be called from the main thread (e.g., once per frame).
   */
  void pollCompletions();

  /** Number of tasks pending + running */
  int getPendingCount();

  inline s32 getDispatcherThreadId() { return dispatcherThreadId; }

  static constexpr int QUEUE_CAPACITY = 16;
  static constexpr int DONE_CAPACITY = 16;
  static constexpr s32 WORKER_PRIORITY = 0x1E;
  static constexpr s32 DISPATCHER_PRIORITY = 0x10;
  static constexpr int ALARM_CYCLE = 150;

 private:
  // Work queue (ring buffer)
  BgWorkItem queue[QUEUE_CAPACITY];
  volatile int queueHead;
  volatile int queueTail;

  // Completion queue (ring buffer)
  BgTaskId doneQueue[DONE_CAPACITY];
  volatile int doneHead;
  volatile int doneTail;

  // Semaphores
  s32 queueMutex;
  s32 doneMutex;
  s32 workAvailable;

  // Worker thread
  ee_thread_t workerThread;
  s32 workerThreadId;
  volatile bool shouldExit;

  // Dispatcher thread (alarm-based scheduling)
  ee_thread_t dispatcherThread;
  s32 dispatcherThreadId;
  s32 alarmId;

  // Task ID generator
  BgTaskId nextId;
  BgTaskId generateId();

  // Thread entry points
  static void workerEntry(void* arg);
  static void dispatcherEntry(void* arg);
  static void alarmCallback(s32 id, u16 time, void* arg);

  // Internal helpers
  BgWorkItem* findItemById(BgTaskId id);
  bool isQueueFull() const;
  bool isQueueEmpty() const;
};
