#include "managers/tick_manager.hpp"

/**
 * @brief Definition of global tick variables
 *
 */
double elapsedRealTime = 0;
uint32_t g_ticksCounter = DAY_MID;
float g_ticksFraftion = 0;
u16 ticksDayCounter = 0;

TickManager::TickManager() {}

TickManager::~TickManager() {}

void TickManager::update(const float& deltaTime) {
  elapsedRealTime += deltaTime;

  if (g_ticksFraftion > TICKS_IN_SECONDS) {
    while (g_ticksFraftion > TICKS_IN_SECONDS) {
      g_ticksCounter++;
      g_ticksFraftion -= TICKS_IN_SECONDS;
      onTick();
    }
  } else {
    g_ticksFraftion += deltaTime;
  }

  if (g_ticksCounter > DAY_DURATION_IN_TICKS) {
    ticksDayCounter++;
    g_ticksCounter -= DAY_DURATION_IN_TICKS;
  }
}

u8 isTicksCounterAt(const uint32_t ticks) {
  return (g_ticksCounter % ticks) == 0;
}
