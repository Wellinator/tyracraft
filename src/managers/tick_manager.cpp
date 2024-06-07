#include "managers/tick_manager.hpp"
#include "debug.hpp"

/**
 * @brief Definition of global tick variables
 *
 */
double elapsedRealTime = 0;
uint32_t g_ticksCounter = DAY_MID;
float g_ticksFraftion = 0;
u16 ticksDayCounter = 0;

std::deque<float> tickAverageQueue = {};
double lastTickTimeOccurence = 0;

TickManager::TickManager() {}

TickManager::~TickManager() {}

void TickManager::update(const float& deltaTime) {
  elapsedRealTime += deltaTime;

  if (g_ticksFraftion > TICKS_IN_SECONDS) {
    while (g_ticksFraftion > TICKS_IN_SECONDS) {
      g_ticksCounter++;
      g_ticksFraftion -= TICKS_IN_SECONDS;
      onTick();

      if (g_debug_mode) {
        if (tickAverageQueue.size() > 10) {
          tickAverageQueue.pop_front();
        }
        tickAverageQueue.push_back(elapsedRealTime - lastTickTimeOccurence);
        lastTickTimeOccurence = elapsedRealTime;
      }
    }

  } else {
    g_ticksFraftion += deltaTime;
  }

  if (g_ticksCounter > DAY_DURATION_IN_TICKS) {
    ticksDayCounter++;
    g_ticksCounter -= DAY_DURATION_IN_TICKS;
  }
}

float TickManager::getTickTimeAverage() {
  if (tickAverageQueue.size() > 0) {
    float sum = 0;
    for (float i : tickAverageQueue) {
      sum += i;
    }

    float var = sum / tickAverageQueue.size();
    float value = (int)(var * 100 + .5);
    return (float)value / 100;
  } else {
    return 0.00f;
  }
}

u8 isTicksCounterAt(const uint32_t ticks) {
  return (g_ticksCounter % ticks) == 0;
}
