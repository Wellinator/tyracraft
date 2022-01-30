#include <engine.hpp>
#include "objects/chunck.hpp"

class Map
{
private:
    /* data */
public:
    Map( TextureRepository *t_texRepo);
    ~Map();
    
    Chunck *chunck;

    void update(Engine *t_engine);
};
