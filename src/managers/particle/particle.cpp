#include "managers/particle/particle.hpp"

const u8 Particle::DRAW_DATA_COUNT = 6;
const Vec4* Particle::rawData = new Vec4[DRAW_DATA_COUNT]{
    Vec4(1.0F, -1.0F, -1.0), Vec4(-1.0F, 1.0F, -1.0), Vec4(-1.0F, -1.0F, -1.0),
    Vec4(1.0F, -1.0F, -1.0), Vec4(1.0F, 1.0F, -1.0),  Vec4(-1.0F, 1.0F, -1.0)};