#include "debug.hpp"
#include "managers/font/font_options.hpp"
#include "managers/font/font_manager.hpp"
#include "managers/post-fx/post_fx_manager.hpp"
#include <string>
#include <cstdio>

u8 g_debug_mode = false;

#ifdef DEBUG_MODE

DebugMenu g_debug_menu;
int g_selected_debug_menu_item = 0;

// Helper struct to organize menu items by tab
struct MenuItem {
  const char* label;
  u8* value;
  DebugMenuTab tab;
};

// Define all menu items with their respective tabs
static MenuItem g_all_menu_items[] = {
    // GENERAL tab
    {"Render Opaque", &g_debug_menu.enableRenderOpaque, DebugMenuTab::GENERAL},
    {"Render Translucent", &g_debug_menu.enableRenderTranslucent, DebugMenuTab::GENERAL},
    {"Render Particles", &g_debug_menu.enableRenderParticles, DebugMenuTab::GENERAL},
    {"Render Mobs", &g_debug_menu.enableRenderMobs, DebugMenuTab::GENERAL},
    {"Render Players", &g_debug_menu.enableRenderPlayers, DebugMenuTab::GENERAL},
    {"Render Block Damage", &g_debug_menu.enableRenderBlockDamage, DebugMenuTab::GENERAL},
    {"Render UI", &g_debug_menu.enableRenderUI, DebugMenuTab::GENERAL},
    {"Day/Night Cycle", &g_debug_menu.enableDayNightCycle, DebugMenuTab::GENERAL},
    {"Clouds", &g_debug_menu.enableClouds, DebugMenuTab::GENERAL},
    {"Collision Boxes", &g_debug_menu.showCollisionBoxes, DebugMenuTab::GENERAL},
    {"Chunk Borders", &g_debug_menu.showChunkBorders, DebugMenuTab::GENERAL},
    {"Player Bounding Box", &g_debug_menu.showPlayerBoundingBox, DebugMenuTab::GENERAL},
    {"Targeted Block Box", &g_debug_menu.showTargetedBlockBoundingBox, DebugMenuTab::GENERAL},
    {"Player Info", &g_debug_menu.showPlayerInfo, DebugMenuTab::GENERAL},
    {"World Info", &g_debug_menu.showWorldInfo, DebugMenuTab::GENERAL},
    {"Block Update Info", &g_debug_menu.showBlockUpdateInfo, DebugMenuTab::GENERAL},
    {"Chunk Update Info", &g_debug_menu.showChunkUpdateInfo, DebugMenuTab::GENERAL},
    {"Light Update Info", &g_debug_menu.showLightUpdateInfo, DebugMenuTab::GENERAL},
    {"Liquid Propagation", &g_debug_menu.showLiquidPropagationInfo, DebugMenuTab::GENERAL},
    {"Render Mob Pathfinding", &g_debug_menu.showMobPathfinding, DebugMenuTab::GENERAL},
    {"God Mode", &g_debug_menu.godMode, DebugMenuTab::GENERAL},
    {"Noclip Mode", &g_debug_menu.noclipMode, DebugMenuTab::GENERAL},
    
    // POST_FX tab
    {"Post FX", &g_debug_menu.enablePostFx, DebugMenuTab::POST_FX},
    {"Fog", &g_debug_menu.fogEnabled, DebugMenuTab::POST_FX},
    {"Bloom", &g_debug_menu.enableBloom, DebugMenuTab::POST_FX},
    {"Bloom: Adjust Params", &g_debug_menu.bloomAdjustParams, DebugMenuTab::POST_FX},
    
    // WORLD_INFO tab
    
    // GAMEPLAY tab
    
    // PERFORMANCE tab
    {"Log Pathfinding", &g_debug_menu.logPathfinding, DebugMenuTab::PERFORMANCE},
    {"Log Chunk Memory Usage", &g_debug_menu.logChunkMemoryUsage, DebugMenuTab::PERFORMANCE},
};

static const int g_total_menu_items = sizeof(g_all_menu_items) / sizeof(MenuItem);

// Get tab name
const char* getTabName(DebugMenuTab tab) {
  switch (tab) {
    case DebugMenuTab::GENERAL: return "GENERAL";
    case DebugMenuTab::POST_FX: return "POST FX";
    case DebugMenuTab::WORLD_INFO: return "WORLD INFO";
    case DebugMenuTab::GAMEPLAY: return "GAMEPLAY";
    case DebugMenuTab::PERFORMANCE: return "PERFORMANCE";
    default: return "UNKNOWN";
  }
}

// Count items in current tab
int getItemCountForTab(DebugMenuTab tab) {
  int count = 0;
  for (int i = 0; i < g_total_menu_items; i++) {
    if (g_all_menu_items[i].tab == tab) {
      count++;
    }
  }
  return count;
}

void renderDebugMenu() {
  const float defaultFontSize = 0.60F;
  float startX = 5.0F;
  float startY = 30.0F;
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
  FontOptions tabOptions(Vec2(0, 0), Color(100.0F, 150.0F, 255.0F),
                         defaultFontSize, TextAlignment::Left);
  FontOptions activeTabOptions(Vec2(0, 0), Color(255.0F, 200.0F, 100.0F),
                               defaultFontSize + 0.1F, TextAlignment::Left);

  FontManager& fm = FontManager::getInstanceRef();

  // Render title
  labelOptions.position = Vec2(currentX, currentY);
  fm.printText("=== DEBUG MENU ===", labelOptions);
  currentY += lineHeight * 1.5F;

  // Render tabs
  float tabX = currentX;
  for (int t = 0; t < static_cast<int>(DebugMenuTab::COUNT); t++) {
    DebugMenuTab tab = static_cast<DebugMenuTab>(t);
    bool isCurrentTab = (tab == g_debug_menu.currentTab);
    
    FontOptions* tabOpt = isCurrentTab ? &activeTabOptions : &tabOptions;
    std::string tabText = isCurrentTab ? "[" : " ";
    tabText += getTabName(tab);
    tabText += isCurrentTab ? "]" : " ";
    
    tabOpt->position = Vec2(tabX, currentY);
    fm.printText(tabText, *tabOpt);
    tabX += 80.0F; // Spacing between tabs
  }
  currentY += lineHeight * 2.0F;

  // Render menu items for current tab
  int itemIndexInTab = 0;
  for (int i = 0; i < g_total_menu_items; i++) {
    if (g_all_menu_items[i].tab != g_debug_menu.currentTab) {
      continue;
    }

    bool isSelected = (itemIndexInTab == g_selected_debug_menu_item);

    // Choose color based on selection and state
    FontOptions* options;
    if (isSelected) {
      options = &selectedOptions;
    } else if (*(g_all_menu_items[i].value)) {
      options = &turnedOnOptions;
    } else {
      options = &turnedOffOptions;
    }

    // Add selection indicator
    std::string text = isSelected ? "> " : "  ";
    text += g_all_menu_items[i].label;
    text += ": ";
    text += (*(g_all_menu_items[i].value)) ? "ON" : "OFF";

    options->position = Vec2(currentX, currentY);
    fm.printText(text, *options);
    currentY += lineHeight;
    itemIndexInTab++;
  }

  // Render bloom parameter adjustment when enabled
  if (g_debug_menu.currentTab == DebugMenuTab::POST_FX &&
      g_debug_menu.bloomAdjustParams) {
    currentY += lineHeight * 0.5F;

    labelOptions.position = Vec2(currentX, currentY);
    fm.printText("--- Bloom Parameters ---", labelOptions);
    currentY += lineHeight;

    char valueStr[64];
    snprintf(valueStr, sizeof(valueStr), "  Cutoff: %.2f",
             (float)g_debug_menu.bloomCutoffX100 / 100.0f);
    labelOptions.position = Vec2(currentX, currentY);
    fm.printText(valueStr, labelOptions);
    currentY += lineHeight;

    snprintf(valueStr, sizeof(valueStr), "  Depth: %d",
             g_debug_menu.bloomDepth);
    labelOptions.position = Vec2(currentX, currentY);
    fm.printText(valueStr, labelOptions);
    currentY += lineHeight;

    snprintf(valueStr, sizeof(valueStr), "  Scale: %.1f",
             (float)g_debug_menu.bloomSourceScaleX10 / 10.0f);
    labelOptions.position = Vec2(currentX, currentY);
    fm.printText(valueStr, labelOptions);
    currentY += lineHeight;

    snprintf(valueStr, sizeof(valueStr), "  Gain: %.1f",
             (float)g_debug_menu.bloomGainX10 / 10.0f);
    labelOptions.position = Vec2(currentX, currentY);
    fm.printText(valueStr, labelOptions);
    currentY += lineHeight;

    labelOptions.position = Vec2(currentX, currentY);
    fm.printText("  L2/R2: Cutoff +/- 0.05", labelOptions);
    currentY += lineHeight;

    labelOptions.position = Vec2(currentX, currentY);
    fm.printText("  Left/Right: Depth +/- 1", labelOptions);
    currentY += lineHeight;

    labelOptions.position = Vec2(currentX, currentY);
    fm.printText("  Up/Down: Scale +/- 0.1", labelOptions);
    currentY += lineHeight;

    labelOptions.position = Vec2(currentX, currentY);
    fm.printText("  L1/R1: Gain +/- 0.1", labelOptions);
    currentY += lineHeight;

    labelOptions.position = Vec2(currentX, currentY);
    fm.printText("  Square: Reset defaults", labelOptions);
  }

  // Render instructions
  const float instructionOffset = 10.0F;
  currentY += instructionOffset;

  labelOptions.position = Vec2(currentX, currentY);
  fm.printText("L1/R1: Switch Tab", labelOptions);
  currentY += lineHeight;

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
  const auto& pressed = pPad->getPressed();

  // Switch tabs with L1/R1
  if (clicked.L1) {
    int currentTabIndex = static_cast<int>(g_debug_menu.currentTab);
    currentTabIndex--;
    if (currentTabIndex < 0) {
      currentTabIndex = static_cast<int>(DebugMenuTab::COUNT) - 1;
    }
    g_debug_menu.currentTab = static_cast<DebugMenuTab>(currentTabIndex);
    g_selected_debug_menu_item = 0; // Reset selection when changing tabs
  }

  if (clicked.R1) {
    int currentTabIndex = static_cast<int>(g_debug_menu.currentTab);
    currentTabIndex++;
    if (currentTabIndex >= static_cast<int>(DebugMenuTab::COUNT)) {
      currentTabIndex = 0;
    }
    g_debug_menu.currentTab = static_cast<DebugMenuTab>(currentTabIndex);
    g_selected_debug_menu_item = 0; // Reset selection when changing tabs
  }

  // Adjust bloom parameters when adjustment mode is enabled
  if (g_debug_menu.bloomAdjustParams) {
    auto* postFxManager = PostFxManager::getInstance();

    if (pressed.L2) {
      g_debug_menu.bloomCutoffX100 -= 5;
      if (g_debug_menu.bloomCutoffX100 < 0) g_debug_menu.bloomCutoffX100 = 0;
      postFxManager->setBloomCutoff((float)g_debug_menu.bloomCutoffX100 / 100.0f);
    }
    if (pressed.R2) {
      g_debug_menu.bloomCutoffX100 += 5;
      if (g_debug_menu.bloomCutoffX100 > 100) g_debug_menu.bloomCutoffX100 = 100;
      postFxManager->setBloomCutoff((float)g_debug_menu.bloomCutoffX100 / 100.0f);
    }
    if (pressed.DpadLeft) {
      g_debug_menu.bloomDepth -= 1;
      if (g_debug_menu.bloomDepth < 1) g_debug_menu.bloomDepth = 1;
      postFxManager->setBloomDepth(g_debug_menu.bloomDepth);
    }
    if (pressed.DpadRight) {
      g_debug_menu.bloomDepth += 1;
      if (g_debug_menu.bloomDepth > 4) g_debug_menu.bloomDepth = 4;
      postFxManager->setBloomDepth(g_debug_menu.bloomDepth);
    }
    if (pressed.DpadUp) {
      g_debug_menu.bloomSourceScaleX10 += 1;
      if (g_debug_menu.bloomSourceScaleX10 > 50) g_debug_menu.bloomSourceScaleX10 = 50;
      postFxManager->setBloomSourceScale((float)g_debug_menu.bloomSourceScaleX10 / 10.0f);
    }
    if (pressed.DpadDown) {
      g_debug_menu.bloomSourceScaleX10 -= 1;
      if (g_debug_menu.bloomSourceScaleX10 < 0) g_debug_menu.bloomSourceScaleX10 = 0;
      postFxManager->setBloomSourceScale((float)g_debug_menu.bloomSourceScaleX10 / 10.0f);
    }
    if (pressed.L1) {
      g_debug_menu.bloomGainX10 -= 1;
      if (g_debug_menu.bloomGainX10 < 1) g_debug_menu.bloomGainX10 = 1;
      postFxManager->setBloomGain((float)g_debug_menu.bloomGainX10 / 10.0f);
    }
    if (pressed.R1) {
      g_debug_menu.bloomGainX10 += 1;
      if (g_debug_menu.bloomGainX10 > 30) g_debug_menu.bloomGainX10 = 30;
      postFxManager->setBloomGain((float)g_debug_menu.bloomGainX10 / 10.0f);
    }
    if (clicked.Square) {
      g_debug_menu.bloomCutoffX100 = 30;
      g_debug_menu.bloomDepth = 3;
      g_debug_menu.bloomSourceScaleX10 = 15;
      g_debug_menu.bloomGainX10 = 12;
      postFxManager->setBloomCutoff(0.3f);
      postFxManager->setBloomDepth(3);
      postFxManager->setBloomSourceScale(1.5f);
      postFxManager->setBloomGain(1.2f);
    }
  }

  // Get number of items in current tab
  int itemsInCurrentTab = getItemCountForTab(g_debug_menu.currentTab);

  // Navigate up (sempre permitido)
  if (clicked.DpadUp) {
    g_selected_debug_menu_item--;
    if (g_selected_debug_menu_item < 0) {
      g_selected_debug_menu_item = itemsInCurrentTab - 1;
    }
  }

  // Navigate down (sempre permitido)
  if (clicked.DpadDown) {
    g_selected_debug_menu_item++;
    if (g_selected_debug_menu_item >= itemsInCurrentTab) {
      g_selected_debug_menu_item = 0;
    }
  }

  // Toggle option
  if (clicked.Cross) {
    // Find the selected item in the current tab
    int itemIndexInTab = 0;
    for (int i = 0; i < g_total_menu_items; i++) {
      if (g_all_menu_items[i].tab != g_debug_menu.currentTab) {
        continue;
      }
      
      if (itemIndexInTab == g_selected_debug_menu_item) {
        *(g_all_menu_items[i].value) = !(*(g_all_menu_items[i].value));
        
        // Sincronizar com PostFxManager quando togglear "Bloom: Adjust Params"
        if (g_all_menu_items[i].value == &g_debug_menu.bloomAdjustParams) {
          auto* postFxManager = PostFxManager::getInstance();

          if (g_debug_menu.bloomAdjustParams) {
            // Ao ativar, sincronizar valores atuais do PostFxManager
            g_debug_menu.bloomCutoffX100 =
                (int)(postFxManager->getBloomCutoff() * 100.0f + 0.5f);
            g_debug_menu.bloomDepth = postFxManager->getBloomDepth();
            g_debug_menu.bloomSourceScaleX10 =
                (int)(postFxManager->getBloomSourceScale() * 10.0f + 0.5f);
            g_debug_menu.bloomGainX10 =
                (int)(postFxManager->getBloomGain() * 10.0f + 0.5f);
          }
        }

        // Sincronizar enableBloom com PostFxManager
        if (g_all_menu_items[i].value == &g_debug_menu.enableBloom) {
          auto* postFxManager = PostFxManager::getInstance();
          postFxManager->setBloomEnabled(g_debug_menu.enableBloom);
        }
        
        break;
      }
      itemIndexInTab++;
    }
  }

  // Close debug menu
  if (clicked.Triangle) {
    g_debug_menu.showDebugMenu = false;
    g_selected_debug_menu_item = 0;
  }
}

#endif  // end if DEBUG_MODE