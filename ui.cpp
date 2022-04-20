#include "ui.hpp"
#include <utils/debug.hpp>

Ui::Ui(TextureRepository *t_texRepo)
{
    crosshair.size.set(18.0F, 18.0F);
    crosshair.position.set(320.0F, 240.0F);
    t_texRepo->add("assets/textures/ui/", "crosshair", PNG)->addLink(crosshair.getId());
}

Ui::~Ui() {}

void Ui::update(const Player &player)
{
    return;
}

void Ui::render(Renderer *t_renderer)
{
    t_renderer->draw(crosshair);
}