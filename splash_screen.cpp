#include "splash_screen.hpp"
#include <utils/debug.hpp>

SplashScreen::SplashScreen(TextureRepository *t_texRepo, ScreenSettings *t_screen)
{
    this->t_texRepo = t_texRepo;
    u8 index = 0;
    for (u8 row = 0; row < 4; row++)
    {
        for (u8 col = 0; col < 4; col++)
        {
            std::string tmp = std::to_string(index + 1);
            char const *image_index = tmp.c_str();

            tyracraft_grid[index].setMode(MODE_STRETCH);
            tyracraft_grid[index].size.set(t_screen->width / 4, t_screen->height / 4);
            tyracraft_grid[index].position.set(
                floor(t_screen->width / 4 * col),
                floor(t_screen->height / 4 * row));

            tyra_grid[index].setMode(MODE_STRETCH);
            tyra_grid[index].size.set(t_screen->width / 4, t_screen->height / 4);
            tyra_grid[index].position.set(
                floor(t_screen->width / 4 * col),
                floor(t_screen->height / 4 * row));

            t_texRepo->add("assets/splash_screen/tyracraft/", image_index, PNG)->addLink(tyracraft_grid[index].getId());
            t_texRepo->add("assets/splash_screen/tyra/", image_index, PNG)->addLink(tyra_grid[index].getId());
            index++;
        }
    }
}

SplashScreen::~SplashScreen()
{
}

void SplashScreen::render(Renderer *t_renderer)
{
    setBgColorBlack(t_renderer);

    alpha = isFading ? alpha - 1 : alpha + 1;

    if (alpha == 128)
    {
        sleep(2);
        isFading = 1;
    }
    else if (alpha == 0)
    {
        isFading = 0;
    }

    if (!hasShowedTyraCraft)
    {
        renderTyraCraftSplash(t_renderer);
        return;
    }
    if (!hasShowedTyra)
    {
        renderTyraSplash(t_renderer);
        return;
    }
}

void SplashScreen::renderTyraSplash(Renderer *t_renderer)
{
    for (u8 index = 0; index < 16; index++)
    {
        tyra_grid[index].color.a = alpha;
        t_renderer->draw(tyra_grid[index]);
        if (alpha == 0)
            hasShowedTyra = 1;
    }
}

void SplashScreen::renderTyraCraftSplash(Renderer *t_renderer)
{
    for (u8 index = 0; index < 16; index++)
    {
        tyracraft_grid[index].color.a = alpha;
        t_renderer->draw(tyracraft_grid[index]);
        if (alpha == 0)
            hasShowedTyraCraft = 1;
    }
}

void SplashScreen::setBgColorBlack(Renderer *t_renderer)
{
    color_t bgColor;
    bgColor.r = 0x00;
    bgColor.g = 0x00;
    bgColor.b = 0x00;
    t_renderer->setWorldColor(bgColor);
}

u8 SplashScreen::hasFinished()
{
    return hasShowedTyraCraft && hasShowedTyra;
}

u8 SplashScreen::shouldBeDestroyed()
{
    return this->hasFinished();
}