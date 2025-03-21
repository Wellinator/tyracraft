#pragma once

class Mob;

class MobState {
 public:
  virtual ~MobState() = default;
  virtual void update(Mob* pMob, const float& fixedDeltaTime) = 0;
};
