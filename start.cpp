#include "start.hpp"

Start::Start(Engine *t_engine)
    : engine(t_engine), camera(&t_engine->screen)
{
    consoleLog("Initing game");
    world = new World(engine);
    // Load World
}

Start::~Start()
{
    return;
}

void Start::onInit()
{
    texRepo = engine->renderer->getTextureRepository();
    player = new Player(&engine->audio, texRepo);
    
    setBgColorAndAmbientColor();
    engine->renderer->disableVSync();
    
    // Load models and textures;
    world->init();
    skybox.loadObj("meshes/skybox/", "skybox", 800.0F, false);
    texRepo->addByMesh("meshes/skybox/", skybox, BMP);
    skybox.shouldBeFrustumCulled = false;
    skybox.shouldBeBackfaceCulled = false;
    

    // Set camera definitions
    engine->renderer->setCameraDefinitions(&camera.view, &camera.unitCirclePosition, camera.planes);
    
    // Load MENU song
    engine->audio.addSongListener(this);
    engine->audio.loadSong("sounds/menu.wav");
    engine->audio.playSong();
    engine->audio.setSongVolume(100);
}

void Start::onUpdate()
{
    player->update(engine->pad, camera);
    camera.update(engine->pad, player->mesh);
    
    engine->renderer->draw(skybox);
    //engine->renderer->draw(player->mesh);
    world->update(player);
}

void Start::setBgColorAndAmbientColor()
{
    color_t bgColor;
    bgColor.r = 0x87;
    bgColor.g = 0xCE;
    bgColor.b = 0xFA;
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
