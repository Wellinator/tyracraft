#include <engine.hpp>
#include "objects/chunck.hpp"

class Map
{
private:
    /* data */
public:
    Map(Engine *t_engine);
    ~Map();

    Chunck *chunck;

    void update(const Pad &t_pad, Camera &camera);
};
