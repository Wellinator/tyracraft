#pragma once
#include <tamtypes.h>
#include "tyra"
#include "constants.hpp"

// Values in ticks
#define TICK 1.0F / 20.0F
#define REAL_TIME_TO_TICK 36.0F
#define DAY_INIT 0
#define DAY_MID 6000
#define DAY_SUNSET 12000
#define NIGHT_INIT 13000
#define NIGHT_MID 18000
#define DAY_SUNRISE 23000
#define DAY_END 240000
#define DAY_DURATION_IN_TICKS 24000.0F

/**
 * @brief Declaration of global tick variables
 *
 */
extern double elapsedRealTime;
extern float g_ticksCounter;
extern u16 ticksDayCounter;

class TickManager {
 public:
  TickManager();
  ~TickManager();

  void update(const float& deltaTime);
  void updateTicks(const float& deltaTime);
};
