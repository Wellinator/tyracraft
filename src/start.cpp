#include "start.hpp"

Start::Start(Engine *t_engine) : engine(t_engine), camera(&t_engine->screen)
{
}

Start::~Start()
{
    return;
}

void Start::onInit()
{
    texRepo = engine->renderer->getTextureRepository();

    stateManager.init(texRepo, engine->renderer, &engine->audio, &engine->screen);

    engine->renderer->disableVSync();

    // Set camera definitions
    engine->renderer->setCameraDefinitions(&camera.view, &camera.unitCirclePosition, camera.planes);
    engine->audio.addSongListener(this);
}

void Start::onUpdate()
{
    // printf("%f\n", engine->fps);
    stateManager.update(1 / engine->fps, engine->pad, camera);
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
