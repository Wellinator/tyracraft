#pragma once
#include <tamtypes.h>
#include "tyra"
#include "constants.hpp"
#include <functional>
#include <deque>

// Values in ticks
#define TICK 1.0F / 20.0F
#define TICKS_PER_SECOND 20
#define TICKS_IN_SECONDS 0.05F
#define DAY_INIT 0
#define DAY_SUNRISE 1000
#define DAY_MID 6000
#define DAY_SUNSET 12000
#define NIGHT_INIT 13000
#define NIGHT_MID 18000
#define DAY_END 240000
#define DAY_DURATION_IN_TICKS 24000.0F

/**
 * @brief Declaration of global tick variables
 *
 */
extern double elapsedRealTime;
extern uint32_t g_ticksCounter;
extern u16 ticksDayCounter;
extern u8 isTicksCounterAt(const uint32_t ticks);
extern std::deque<float> tickAverageQueue;

class TickManager {
 public:
  TickManager();
  ~TickManager();

  void update(const float& deltaTime);

  float getTickTimeAverage();

  std::function<void()> onTick = std::function<void()>{};
};
