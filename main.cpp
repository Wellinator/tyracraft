#include "start.hpp"

int main()
{
    Engine engine = Engine();
    Start game = Start(&engine);
    game.engine->init(&game, 128);
    SleepThread();
    return 0;
}