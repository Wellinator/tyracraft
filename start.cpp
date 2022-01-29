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
    engine->audio.addSongListener(this);
    engine->audio.loadSong("sounds/menu.wav");
    engine->audio.playSong();
    engine->audio.setSongVolume(100);
}

void Start::onUpdate()
{
    map->update(engine->pad, camera);
    player->update(engine->pad, camera);
    camera.update(engine->pad, player->mesh);
    engine->renderer->draw(player->mesh);
    engine->renderer->draw(map->chunck->getMeshes(), 2);
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

void Start::onAudioTick()
{
    if ((++audioTicks + 20) % 21 == 0)
    {
        skip1Beat = !skip1Beat;
    }
    if (audioTicks > 10000)
        audioTicks = 0;
}

void Start::onAudioFinish()
{
    audioTicks = 0;
}
