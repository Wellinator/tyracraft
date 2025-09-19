#include "debug.hpp"
#include "managers/font/font_options.hpp"
#include "managers/font/font_manager.hpp"
#include <string>
#include <cstdio>

u8 g_debug_mode = false;

#ifdef DEBUG_MODE

DebugMenu g_debug_menu;
int g_selected_debug_menu_item = 0;

void renderDebugMenu() {
  const float screenWidth = 512.0F;
  const float screenHeight = 448.0F;
  const float defaultFontSize = 0.60F;
  float startX = 5.0F;
  float startY = 50.0F;
  float lineHeight = 12.0F;
  float currentY = startY;
  float currentX = startX;

  FontOptions turnedOnOptions(Vec2(0, 0), Color(50.0F, 200.0F, 50.0F),
                              defaultFontSize, TextAlignment::Left);
  FontOptions turnedOffOptions(Vec2(0, 0), Color(255.0F, 0.0F, 0.0F),
                               defaultFontSize, TextAlignment::Left);
  FontOptions labelOptions(Vec2(0, 0), Color(255.0F, 255.0F, 0.0F),
                           defaultFontSize, TextAlignment::Left);
  FontOptions selectedOptions(Vec2(0, 0), Color(255.0F, 255.0F, 255.0F),
                              defaultFontSize, TextAlignment::Left);

  FontManager& fm = FontManager::getInstanceRef();

  // Define menu items
  struct MenuItem {
    const char* label;
    u8* value;
    bool isToggleable;
  };

  MenuItem menuItems[] = {
      {"Collision Boxes", &g_debug_menu.showCollisionBoxes, true},
      {"Chunk Borders", &g_debug_menu.showChunkBorders, true},
      {"Player Bounding Box", &g_debug_menu.showPlayerBoundingBox, true},
      {"Targeted Block Box", &g_debug_menu.showTargetedBlockBoundingBox, true},
      {"Player Info", &g_debug_menu.showPlayerInfo, true},
      {"World Info", &g_debug_menu.showWorldInfo, true},
      {"Liquid Propagation", &g_debug_menu.showLiquidPropagationInfo, true},
      {"Render Mob Pathfinding", &g_debug_menu.showMobPathfinding, true},
      {"Day/Night Cycle", &g_debug_menu.enableDayNightCycle, true},
      {"Clouds", &g_debug_menu.enableClouds, true},
      {"Render Opaque", &g_debug_menu.enableRenderOpaque, true},
      {"Render Translucent", &g_debug_menu.enableRenderTranslucent, true},
      {"Render Particles", &g_debug_menu.enableRenderParticles, true},
      {"Render Mobs", &g_debug_menu.enableRenderMobs, true},
      {"Render Players", &g_debug_menu.enableRenderPlayers, true},
      {"Render Block Damage", &g_debug_menu.enableRenderBlockDamage, true},
      {"Render UI", &g_debug_menu.enableRenderUI, true},
      {"Post FX", &g_debug_menu.enablePostFx, true},
      {"Block Update Info", &g_debug_menu.showBlockUpdateInfo, true},
      {"Chunk Update Info", &g_debug_menu.showChunkUpdateInfo, true},
      {"Light Update Info", &g_debug_menu.showLightUpdateInfo, true},
      {"God Mode", &g_debug_menu.godMode, true},
      {"Noclip Mode", &g_debug_menu.noclipMode, true},
      {"Log Chunk Memory Usage", &g_debug_menu.logChunkMemoryUsage, true},
  };

  const int numItems = sizeof(menuItems) / sizeof(MenuItem);

  // Render title
  labelOptions.position = Vec2(currentX, currentY);
  fm.printText("=== DEBUG MENU ===", labelOptions);
  currentY += lineHeight * 1.5F;

  // Render menu items
  for (int i = 0; i < numItems; i++) {
    bool isSelected = (i == g_selected_debug_menu_item);

    // Choose color based on selection and state
    FontOptions* options;
    if (isSelected) {
      options = &selectedOptions;
    } else if (*(menuItems[i].value)) {
      options = &turnedOnOptions;
    } else {
      options = &turnedOffOptions;
    }

    // Add selection indicator
    std::string text = isSelected ? "> " : "  ";
    text += menuItems[i].label;
    text += ": ";
    text += (*(menuItems[i].value)) ? "ON" : "OFF";

    options->position = Vec2(currentX, currentY);
    fm.printText(text, *options);
    currentY += lineHeight;
  }

  // Render instructions
  const float instructionOffset = 10.0F;
  currentY += instructionOffset;

  labelOptions.position = Vec2(currentX, currentY);
  fm.printText("D-Pad Up/Down: Navigate", labelOptions);
  currentY += lineHeight;

  labelOptions.position = Vec2(currentX, currentY);
  fm.printText("X: Toggle Option", labelOptions);
  currentY += lineHeight;

  labelOptions.position = Vec2(currentX, currentY);
  fm.printText("Triangle: Close Menu", labelOptions);
}

void handleDebugInput(Pad* pPad) {
  if (!g_debug_menu.showDebugMenu) return;

  const auto& clicked = pPad->getClicked();

  // Calculate total number of menu items (25 toggleable items)
  const int totalItems = 25;

  // Navigate up
  if (clicked.DpadUp) {
    g_selected_debug_menu_item--;
    if (g_selected_debug_menu_item < 0) {
      g_selected_debug_menu_item = totalItems - 1;
    }
  }

  // Navigate down
  if (clicked.DpadDown) {
    g_selected_debug_menu_item++;
    if (g_selected_debug_menu_item >= totalItems) {
      g_selected_debug_menu_item = 0;
    }
  }

  // Toggle option or adjust camera speed
  if (clicked.Cross) {
    // Toggle boolean options
    u8* optionToToggle = nullptr;
    switch (g_selected_debug_menu_item) {
      case 0:
        optionToToggle = &g_debug_menu.showCollisionBoxes;
        break;
      case 1:
        optionToToggle = &g_debug_menu.showChunkBorders;
        break;
      case 2:
        optionToToggle = &g_debug_menu.showPlayerBoundingBox;
        break;
      case 3:
        optionToToggle = &g_debug_menu.showTargetedBlockBoundingBox;
        break;
      case 4:
        optionToToggle = &g_debug_menu.showPlayerInfo;
        break;
      case 5:
        optionToToggle = &g_debug_menu.showWorldInfo;
        break;
      case 6:
        optionToToggle = &g_debug_menu.showLiquidPropagationInfo;
        break;
      case 7:
        optionToToggle = &g_debug_menu.showMobPathfinding;
        break;
      case 8:
        optionToToggle = &g_debug_menu.enableDayNightCycle;
        break;
      case 9:
        optionToToggle = &g_debug_menu.enableClouds;
        break;
      case 10:
        optionToToggle = &g_debug_menu.enableRenderOpaque;
        break;
      case 11:
        optionToToggle = &g_debug_menu.enableRenderTranslucent;
        break;
      case 12:
        optionToToggle = &g_debug_menu.enableRenderParticles;
        break;
      case 13:
        optionToToggle = &g_debug_menu.enableRenderMobs;
        break;
      case 14:
        optionToToggle = &g_debug_menu.enableRenderPlayers;
        break;
      case 15:
        optionToToggle = &g_debug_menu.enableRenderBlockDamage;
        break;
      case 16:
        optionToToggle = &g_debug_menu.enableRenderUI;
        break;
      case 17:
        optionToToggle = &g_debug_menu.enablePostFx;
        break;
      case 18:
        optionToToggle = &g_debug_menu.showBlockUpdateInfo;
        break;
      case 19:
        optionToToggle = &g_debug_menu.showChunkUpdateInfo;
        break;
      case 20:
        optionToToggle = &g_debug_menu.showLightUpdateInfo;
        break;
      case 21:
        optionToToggle = &g_debug_menu.godMode;
        break;
      case 22:
        optionToToggle = &g_debug_menu.noclipMode;
        break;
      case 23:
        optionToToggle = &g_debug_menu.logChunkMemoryUsage;
        break;
    }

    if (optionToToggle != nullptr) {
      *optionToToggle = !(*optionToToggle);
    }
  }

  // Close debug menu
  if (clicked.Triangle) {
    g_debug_menu.showDebugMenu = false;
    g_selected_debug_menu_item = 0;
  }
}

#endif  // end if DEBUG_MODE