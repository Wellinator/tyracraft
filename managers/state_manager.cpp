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
    mainMenu = new MainMenu(t_texRepo, t_screen);
}

void StateManager::update(Pad &t_pad, Camera camera)
{

    // Splash Screen
    if (_state == SPLASH_SCREEN)
    {
        splashScreen->render(t_renderer);
    }
    else if (splashScreen->shouldBeDestroyed())
    {
        delete splashScreen;
    }

    if (_state == MAIN_MENU && mainMenu)
    {
        camera.update(t_pad, mainMenu->menuSkybox);
        mainMenu->render(t_renderer);
    }

    if (_state == IN_GAME)
    {
        player->update(t_pad, camera);
        camera.update(t_pad, player->mesh);
        world->update(player, &camera);
        ui->update(*player);

        world->render(t_renderer);
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
    world = new World(t_texRepo);
    ui = new Ui(t_texRepo);
    player = new Player(t_audio, t_texRepo);

    world->init();
}

void StateManager::play()
{
    loadGame();
}