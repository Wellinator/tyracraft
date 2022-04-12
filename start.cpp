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

    stateManager.init(texRepo, engine->renderer, &engine->audio);
    splashScreen = new SplashScreen(texRepo, &engine->screen);
    
    setBgColorAndAmbientColor();

    engine->renderer->disableVSync();

    // Load models and textures;

    // Load skybox (temp);
    //  skybox.loadObj("meshes/skybox/", "skybox", 800.0F, false);
    //  texRepo->addByMesh("meshes/skybox/", skybox, BMP);
    //  skybox.shouldBeFrustumCulled = true;
    //  skybox.shouldBeBackfaceCulled = false;

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
    // stateManager.update(engine->pad, camera);

    
    // engine->renderer->draw(skybox);

    // TODO: Should render only if is third person Cam;
    //  engine->renderer->draw(player->mesh);

    // Render UI
    splashScreen->render(engine->renderer);
}

void Start::setBgColorAndAmbientColor()
{
    color_t bgColor;
    bgColor.r = 0xC0;
    bgColor.g = 0xD8;
    bgColor.b = 0xFF;
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
