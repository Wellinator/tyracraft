#include "managers/tick_manager.hpp"

/**
 * @brief Definition of global tick variables
 *
 */
double elapsedRealTime = 0;
float ticksCounter = DAY_MID;
u16 ticksDayCounter = 0;

TickManager::TickManager() {}

TickManager::~TickManager() {}

void TickManager::update(const float& deltaTime) {
  updateTicks(std::min(deltaTime, MAX_FRAME_MS));
}

void TickManager::updateTicks(const float& deltaTime) {
  elapsedRealTime += deltaTime;
  ticksCounter += deltaTime * REAL_TIME_TO_TICK;

  if (ticksCounter >= DAY_DURATION_IN_TICKS) {
    ticksDayCounter++;
    ticksCounter -= DAY_DURATION_IN_TICKS;
  }
}
