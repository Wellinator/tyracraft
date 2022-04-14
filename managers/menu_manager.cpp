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


    //Load slots
    slot[0].setMode(MODE_STRETCH);
    slot[0].size.set(150, 25);
    slot[0].position.set(150, 25);
    slot[1].setMode(MODE_STRETCH);
    slot[1].size.set(150, 25);
    slot[1].position.set(150, 25 + 30);
    slot[2].setMode(MODE_STRETCH);
    slot[2].size.set(150, 25);
    slot[2].position.set(150, 25 + 60);

    t_texRepo->add("assets/textures/menu/", "slot", PNG)->addLink(slot[0].getId());
    t_texRepo->add("assets/textures/menu/", "hovered_slot", PNG)->addLink(slot[1].getId());
    t_texRepo->add("assets/textures/menu/", "selected_slot", PNG)->addLink(slot[2].getId());




    // TODO: Load menuSkybox;
    // menuSkybox.loadObj("meshes/menu-skybox/", "skybox", 600.0F, true);
    // t_texRepo->addByMesh("meshes/menu-skybox/", menuSkybox, BMP);
    // menuSkybox.shouldBeFrustumCulled = false;
    // menuSkybox.shouldBeBackfaceCulled = false;
}

MainMenu::~MainMenu()
{
}

void MainMenu::render(Renderer *t_renderer)
{
    t_renderer->draw(slot[0]);
    t_renderer->draw(slot[1]);
    t_renderer->draw(slot[2]);
}

u8 MainMenu::shouldBeDestroyed()
{
    return 0;
}