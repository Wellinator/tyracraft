#include "map.hpp"

Map::Map(TextureRepository *t_texRepo)
{
    chunck = new Chunck(t_texRepo, 0.0F, 0.0F, 0.0F);
}

Map::~Map()
{
}

void Map::update(Engine *t_engine)
{
    chunck->update(t_engine);
};
