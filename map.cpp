#include "map.hpp"

Map::Map(Engine *t_engine)
{
    chunck = new Chunck(t_engine, 100.0F, 0.0F, 0.0F);
}

Map::~Map()
{
}

void Map::update(Pad &t_pad, Camera &camera){
    chunck->update(t_pad, camera);
};
