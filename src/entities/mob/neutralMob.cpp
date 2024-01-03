#include "entities/mob/neutralMob.hpp"

NeutralMob::NeutralMob(const MobType mobType)
    : Mob(MobCategory::Neutral, mobType){};

NeutralMob::~NeutralMob(){};
