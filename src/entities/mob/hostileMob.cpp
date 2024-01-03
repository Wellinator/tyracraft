#include "entities/mob/hostileMob.hpp"

HostileMob::HostileMob(const MobType mobType)
    : Mob(MobCategory::Hostile, mobType){};

HostileMob::~HostileMob(){};
