#include "start.hpp"

Start::Start(Engine *t_engine)
    : engine(t_engine), camera(&t_engine->screen)
{
    PRINT_LOG("Initing game");
}

Start::~Start()
{
    return;
}

void Start::onInit()
{
    map = new Map(engine);
    player = new Player(&engine->audio, engine->renderer->getTextureRepository());
    setBgColorAndAmbientColor();
    engine->renderer->setCameraDefinitions(&camera.view, &camera.unitCirclePosition, camera.planes);
}

void Start::onUpdate()
{
    map->update(engine->pad, camera);
    player->update(engine->pad, camera);
}

void Start::setBgColorAndAmbientColor()
{
    color_t bgColor;
    bgColor.r = 0x00;
    bgColor.g = 0x00;
    bgColor.b = 0x00;
    engine->renderer->setWorldColor(bgColor);
    Vector3 ambient = Vector3(0.004F, 0.004F, 0.004F);
    engine->renderer->setAmbientLight(ambient);
}
