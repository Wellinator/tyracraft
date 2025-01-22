#include "managers/task_manager.hpp"

static u8 dispatcher_stack[8 * 1024] ALIGNED(16);

void dispatcher(void* apParam) {
  while (1) {
    SleepThread();
  }
}

TaskManager::TaskManager() : Singleton<TaskManager>() {
  dispatcherThread.func = (void*)dispatcher;
  dispatcherThread.stack = dispatcher_stack;
  dispatcherThread.stack_size = 8 * 1024;
  dispatcherThread.gp_reg = &_gp;
  dispatcherThread.initial_priority = 0x00;

  dispatcherThreadId = CreateThread(&dispatcherThread);
  TYRA_ASSERT(dispatcherThreadId > -1,
              "Error creating dispatcher in task manager!");

  TYRA_ASSERT(StartThread(dispatcherThreadId, NULL) > -1,
              "Error starting dispatcher thread!");
}

TaskManager::~TaskManager() {}

int TaskManager::createTask(Task task) {
  if (alarmId < 0) setAlarm();

  ee_thread_t tempThread;
  tempThread.attr = 0;
  tempThread.option = 0;
  tempThread.func = task.function;
  tempThread.stack = task.stack;
  tempThread.stack_size = sizeof(task.stack);
  tempThread.initial_priority = priority;
  tempThread.gp_reg = &_gp;

  const int threadId = CreateThread(&tempThread);
  if (threadId < 0) {
    TYRA_ERROR("    Error creating task!");
    return -1;
  }

  if (tasks.count(threadId) > 0) {
    TYRA_ERROR("    There is a thread with the same id!");
    return -1;
  }

  tasks.insert(std::pair<int, ee_thread_t>(threadId, tempThread));

  if (StartThread(threadId, NULL) < 0) {
    TYRA_ERROR("Error starting new task!");
    return -1;
  };

  return threadId;
}

bool TaskManager::killTask(int taskId) {
  if (tasks.count(taskId) == 0) {
    TYRA_ERROR("There is no thread with provided id!");
    return false;
  }

  tasks.erase(taskId);
  if (tasks.size() == 0) releaseAlarm();

  TerminateThread(taskId);
  DeleteThread(taskId);

  return true;
}

void TaskManager::setAlarm() {
  alarmId = SetAlarm(alarmCycle, TaskManager::alarmCallback, 0);
  if (alarmId < 0) {
    TYRA_TRAP("Error creating task manager alarm!");
  }
}

void TaskManager::releaseAlarm() {
  ReleaseAlarm(alarmId);
  alarmId = -1;
}

void TaskManager::alarmCallback(s32 id, u16 time, void* arg) {
  TaskManager* tm = TaskManager::getInstance();
  iWakeupThread(tm->getDispatcherThreadId());
  iRotateThreadReadyQueue(tm->priority);
  iSetAlarm(tm->alarmCycle, TaskManager::alarmCallback, NULL);
}
