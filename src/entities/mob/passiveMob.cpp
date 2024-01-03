#include "entities/mob/passiveMob.hpp"

PassiveMob::PassiveMob(const MobType mobType)
    : Mob(MobCategory::Passive, mobType){};

PassiveMob::~PassiveMob(){};
