#include "menu_manager.hpp"
#include <utils/debug.hpp>

MainMenu::MainMenu(TextureRepository *t_texRepo, ScreenSettings *t_screen)
{
    /**
     * TODO: ->
     * - Load rotating skybox
     * - Load tyracraft sprite
     * - Load menu option
     *      - Play Game
     *      - How to play
     *      - About
     * */
    this->t_texRepo = t_texRepo;

    const float halfWidth = t_screen->width / 2;
    const float halfHeight = t_screen->height / 2;

    // Background
    background[0].setMode(MODE_STRETCH);
    background[0].size.set(halfWidth, halfHeight);
    background[0].position.set(0.0F, 0.0F);
    background[1].setMode(MODE_STRETCH);
    background[1].size.set(halfWidth, halfHeight);
    background[1].position.set(halfWidth, 0.0F);
    background[2].setMode(MODE_STRETCH);
    background[2].size.set(halfWidth, halfHeight);
    background[2].position.set(0.0F, halfHeight);
    background[3].setMode(MODE_STRETCH);
    background[3].size.set(halfWidth, halfHeight);
    background[3].position.set(halfWidth, halfHeight);

    t_texRepo->add("assets/textures/menu/", "background_tl", PNG)->addLink(background[0].getId());
    t_texRepo->add("assets/textures/menu/", "background_tr", PNG)->addLink(background[1].getId());
    t_texRepo->add("assets/textures/menu/", "background_bl", PNG)->addLink(background[2].getId());
    t_texRepo->add("assets/textures/menu/", "background_br", PNG)->addLink(background[3].getId());

    // Load title
    // Title
    title[0].setMode(MODE_STRETCH);
    title[0].size.set(128, 96);
    title[0].position.set((halfWidth)-256, 64);

    title[1].setMode(MODE_STRETCH);
    title[1].size.set(128, 96);
    title[1].position.set(halfWidth - 128, 64);

    title[2].setMode(MODE_STRETCH);
    title[2].size.set(128, 96);
    title[2].position.set(halfWidth, 64);

    title[3].setMode(MODE_STRETCH);
    title[3].size.set(128, 96);
    title[3].position.set(halfWidth + 128, 64);

    t_texRepo->add("assets/textures/menu/", "title_1", PNG)->addLink(title[0].getId());
    t_texRepo->add("assets/textures/menu/", "title_2", PNG)->addLink(title[1].getId());
    t_texRepo->add("assets/textures/menu/", "title_3", PNG)->addLink(title[2].getId());
    t_texRepo->add("assets/textures/menu/", "title_4", PNG)->addLink(title[3].getId());

    // Alpha Version
    subtitle.setMode(MODE_STRETCH);
    subtitle.size.set(130, 16);
    subtitle.position.set((halfWidth) - (130 / 2), 164);

    t_texRepo->add("assets/textures/menu/", "sub_title", PNG)->addLink(subtitle.getId());

    // Buttons
    btnCross.setMode(MODE_STRETCH);
    btnCross.size.set(45, 45);
    btnCross.position.set(30, t_screen->height - 60);

    t_texRepo->add("assets/textures/ui/", "btn_cross", PNG)->addLink(btnCross.getId());

    // Load slots
    slot[0].setMode(MODE_STRETCH);
    slot[0].size.set(200, 25);
    slot[0].position.set((halfWidth)-100, 265);
    slot[1].setMode(MODE_STRETCH);
    slot[1].size.set(200, 25);
    slot[1].position.set((halfWidth)-100, 265 + 30);
    slot[2].setMode(MODE_STRETCH);
    slot[2].size.set(200, 25);
    slot[2].position.set((halfWidth)-100, 265 + 60);

    t_texRepo->add("assets/textures/menu/", "slot", PNG)->addLink(slot[0].getId());
    t_texRepo->add("assets/textures/menu/", "hovered_slot", PNG)->addLink(slot[1].getId());
    t_texRepo->add("assets/textures/menu/", "selected_slot", PNG)->addLink(slot[2].getId());

    // Texts
    textPlayGame.setMode(MODE_STRETCH);
    textPlayGame.size.set(80, 15);
    textPlayGame.position.set((halfWidth)-40, 265 + 35);

    if (this->activeOption == PLAY_GAME)
    {
        textPlayGame.color.r = 255;
        textPlayGame.color.g = 255;
        textPlayGame.color.b = 0;
    }

    t_texRepo->add("assets/textures/menu/", "play_game", PNG)->addLink(textPlayGame.getId());

    textSelect.setMode(MODE_STRETCH);
    textSelect.size.set(64, 16);
    textSelect.position.set(30 + 40, t_screen->height - 47);
    t_texRepo->add("assets/textures/menu/", "select", PNG)->addLink(textSelect.getId());

    // TODO: Load menuSkybox;
    menuSkybox.loadObj("meshes/menu-skybox/", "skybox", 100.0F, false);
    menuSkybox.shouldBeFrustumCulled = false;
    t_texRepo->addByMesh("meshes/menu-skybox/", menuSkybox, BMP);
}

MainMenu::~MainMenu()
{
}

void MainMenu::update(Pad &t_pad)
{
    if (t_pad.isCrossClicked)
        this->selectedOption = this->activeOption;
}

void MainMenu::render(Renderer *t_renderer)
{
    // Meshes
    menuSkybox.rotation.x += 1;
    t_renderer->draw(menuSkybox);

    // Sprites

    // Background
    t_renderer->draw(background[0]);
    t_renderer->draw(background[1]);
    t_renderer->draw(background[2]);
    t_renderer->draw(background[3]);

    // Title & Subtitle
    t_renderer->draw(title[0]);
    t_renderer->draw(title[1]);
    t_renderer->draw(title[2]);
    t_renderer->draw(title[3]);
    t_renderer->draw(subtitle);

    // Slots
    t_renderer->draw(slot[1]);
    // t_renderer->draw(slot[1]);
    // t_renderer->draw(slot[2]);

    // Texts
    t_renderer->draw(textPlayGame);
    t_renderer->draw(textSelect);

    // Buttons
    t_renderer->draw(btnCross);
}

u8 MainMenu::shouldInitGame()
{
    return this->selectedOption == PLAY_GAME;
}