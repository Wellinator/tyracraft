#include "./state_manager.hpp"

StateManager::StateManager()
{
}

StateManager::~StateManager() {}

void StateManager::init(TextureRepository *t_texRepo, Renderer *t_renderer, Audio *t_audio, ScreenSettings *t_screen)
{
    this->t_texRepo = t_texRepo;
    this->t_renderer = t_renderer;
    this->t_audio = t_audio;
    this->t_screen = t_screen;

    splashScreen = new SplashScreen(t_texRepo, t_screen);
    mainMenu = new MainMenu();

    world = new World();
    ui = new Ui(t_texRepo);
    player = new Player(t_audio, t_texRepo);
}

void StateManager::update(Pad &t_pad, Camera &camera)
{
    // Splash Screen
    if (_state == SPLASH_SCREEN)
    {
        splashScreen->render(t_renderer);
        if (splashScreen->shouldBeDestroyed())
        {
            delete splashScreen;
            this->mainMenu->init(t_texRepo, t_screen, t_audio);
            _state = MAIN_MENU;
        }
        return;
    }

    // Main menu
    if (_state == MAIN_MENU)
    {
        camera.update(t_pad, mainMenu->menuSkybox);
        mainMenu->update(t_pad);
        mainMenu->render(t_renderer);
        if (mainMenu->shouldInitGame())
        {
            delete mainMenu;
            this->loadGame(); // TODO: implement loading screen
            _state = IN_GAME;
        }
        return;
    }

    // In game
    if (_state == IN_GAME)
    {
        //Updates
        world->update(player, &camera, t_pad);
        player->update(t_pad,
                       camera,
                       world->terrainManager->getChunck()->blocks.data(),
                       world->terrainManager->getChunck()->blocks.size());
        ui->update(*player);
        camera.update(t_pad, player->mesh);

        //Render
        world->render(t_renderer);
        // TODO: Should render only if is third person Cam;
        // t_renderer->draw(player->mesh);

        //Lest spet is 2D drawing
        ui->render(t_renderer);
    }
}

u8 StateManager::currentState()
{
    return this->_state;
}

void StateManager::loadSplashScreen()
{
}

void StateManager::loadMenu()
{
}

void StateManager::loadInGameMenu()
{
}

void StateManager::loadGame()
{
    setBgColorAndAmbientColor();
    world->init(t_texRepo);
}

void StateManager::play()
{
    loadGame();
}

void StateManager::setBgColorAndAmbientColor()
{
    color_t bgColor;
    bgColor.r = 0xC0;
    bgColor.g = 0xD8;
    bgColor.b = 0xFF;
    t_renderer->setWorldColor(bgColor);
    Vector3 ambient = Vector3(0.004F, 0.004F, 0.004F);
    t_renderer->setAmbientLight(ambient);
}