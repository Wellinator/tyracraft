#include "ui.hpp"
#include <utils/debug.hpp>

Ui::Ui(TextureRepository *t_texRepo)
{
    this->t_texRepo = t_texRepo;
    this->loadlHud();
}

Ui::~Ui() {}

void Ui::update(const Player &player)
{
    return;
}

void Ui::render(Renderer *t_renderer)
{
    t_renderer->draw(crosshair);
    t_renderer->draw(empty_slots);
    t_renderer->draw(selected_slot);
    t_renderer->draw(xp_bar_full);
    for (u8 i = 0; i < 10; i++)
    {
        t_renderer->draw(armor[i]); //TODO: Daw only when wearing an armor
        t_renderer->draw(health[i]);
        t_renderer->draw(hungry[i]);
        t_renderer->draw(breath[i]);
    }
}

void Ui::loadlHud()
{
    crosshair.setMode(MODE_STRETCH);
    crosshair.size.set(8.0f, 8.0f);
    crosshair.position.set(320.0f, 240.0f);
    t_texRepo->add("assets/textures/ui/", "crosshair", PNG)->addLink(crosshair.getId());

    //Items slots
    empty_slots.setMode(MODE_STRETCH);
    empty_slots.size.set(192.0f, 32.0f);
    empty_slots.position.set(224.0f, 446.0f);
    t_texRepo->add("assets/hud/", "empty_slots", PNG)->addLink(empty_slots.getId());

    selected_slot.setMode(MODE_STRETCH);
    selected_slot.size.set(22.0f, 32.0f);
    selected_slot.position.set(224.0f, 446.0f);
    t_texRepo->add("assets/hud/", "selected_slot", PNG)->addLink(selected_slot.getId());

    xp_bar_full.setMode(MODE_STRETCH);
    xp_bar_full.size.set(192.0f, 8.0f);
    xp_bar_full.position.set(224.0f, 436.0f);
    t_texRepo->add("assets/hud/", "xp_bar_full", PNG)->addLink(xp_bar_full.getId());

    for (u8 i = 0; i < 10; i++)
    {
        health[i].setMode(MODE_STRETCH);
        health[i].size.set(8.0f, 8.0f);
        health[i].position.set(224.0f + (i * 8.0f), 446.0f - 20.0f);
        t_texRepo->add("assets/hud/", "health_full", PNG)->addLink(health[i].getId());
        
        armor[i].setMode(MODE_STRETCH);
        armor[i].size.set(8.0f, 8.0f);
        armor[i].position.set(224.0f + (i * 8.0f), 446.0f - 28.0f);
        t_texRepo->add("assets/hud/", "armor_full", PNG)->addLink(armor[i].getId());

        hungry[i].setMode(MODE_STRETCH);
        hungry[i].size.set(8.0f, 8.0f);
        hungry[i].position.set(330.66f + (i * 8.0f), 446.0f - 20.0f);
        t_texRepo->add("assets/hud/", "hungry_full", PNG)->addLink(hungry[i].getId());

        breath[i].setMode(MODE_STRETCH);
        breath[i].size.set(8.0f, 8.0f);
        breath[i].position.set(330.66f + (i * 8.0f), 446.0f - 29.0f);
        t_texRepo->add("assets/hud/", "breath_full", PNG)->addLink(breath[i].getId());

    }
    
}