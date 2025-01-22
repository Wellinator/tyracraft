#pragma once

#include <tamtypes.h>
#include "singleton.hpp"
#include "tyra"
#include "map"
#include "kernel.h"

struct Task {
  void* function;
  void* stack;
};

class TaskManager : public Singleton<TaskManager> {
 public:
  TaskManager();
  ~TaskManager();
  int createTask(Task task);
  bool killTask(int taskId);
  inline int getDispatcherThreadId() { return dispatcherThreadId; }

  const int alarmCycle = 150;
  const s32 priority = 0x1E;

 private:
  std::map<int, ee_thread_t> tasks = {};

  ee_thread_t dispatcherThread;
  int dispatcherThreadId;

  int alarmId;
  void setAlarm();
  void releaseAlarm();

 public:
  static void alarmCallback(s32 id, u16 time, void* arg);
};
