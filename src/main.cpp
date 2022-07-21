#include "start.hpp"
#include "engine.hpp"

using Tyra::Engine;
using Tyra::Mesh;

int main()
{
    Engine engine = Engine();
    Start game = Start(&engine);
    engine.run(&game);
    SleepThread();
    return 0;
}