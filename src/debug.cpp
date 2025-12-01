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
    {"Fog: Setup", &g_debug_menu.fogPassSetup, DebugMenuTab::POST_FX},
    {"Fog: First Pass", &g_debug_menu.fogPass1, DebugMenuTab::POST_FX},
    {"Fog: Second Pass", &g_debug_menu.fogPass2, DebugMenuTab::POST_FX},
    {"Fog: Third Pass", &g_debug_menu.fogPass3, DebugMenuTab::POST_FX},
    {"Fog: Fourth Pass", &g_debug_menu.fogPass4, DebugMenuTab::POST_FX},
    {"Fog: Fifth Pass", &g_debug_menu.fogPass5, DebugMenuTab::POST_FX},
    {"Fog: Sixth Pass", &g_debug_menu.fogPass6, DebugMenuTab::POST_FX},
    {"Fog: Restore GS State", &g_debug_menu.fogPassRestore, DebugMenuTab::POST_FX},
    {"Fog: Use Custom CLIP_Z", &g_debug_menu.fogUseCustomClipZ, DebugMenuTab::POST_FX},
    
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

  // Render CLIP_ZVALUE adjustment when enabled
  if (g_debug_menu.currentTab == DebugMenuTab::POST_FX && 
      g_debug_menu.fogUseCustomClipZ) {
    currentY += lineHeight * 0.5F;
    
    labelOptions.position = Vec2(currentX, currentY);
    fm.printText("--- CLIP_Z Adjustment ---", labelOptions);
    currentY += lineHeight;
    
    char valueStr[64];
    snprintf(valueStr, sizeof(valueStr), "  Value: 0x%06X (%d)", 
             g_debug_menu.fogClipZValue, g_debug_menu.fogClipZValue);
    labelOptions.position = Vec2(currentX, currentY);
    fm.printText(valueStr, labelOptions);
    currentY += lineHeight;
    
    labelOptions.position = Vec2(currentX, currentY);
    fm.printText("  L2/R2: +/- 0x1000", labelOptions);
    currentY += lineHeight;
    
    labelOptions.position = Vec2(currentX, currentY);
    fm.printText("  Left/Right: +/- 0x100", labelOptions);
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

  // Adjust CLIP_ZVALUE when custom mode is enabled
  if (g_debug_menu.fogUseCustomClipZ) {
    auto& postFxManager = PostFxManager::getInstance();
    
    if (pressed.L2) {
      postFxManager.adjustClipZValue(-0x1000);
      g_debug_menu.fogClipZValue = postFxManager.getClipZValue();
    }
    if (pressed.R2) {
      postFxManager.adjustClipZValue(0x1000);
      g_debug_menu.fogClipZValue = postFxManager.getClipZValue();
    }
    if (pressed.DpadLeft) {
      postFxManager.adjustClipZValue(-0x100);
      g_debug_menu.fogClipZValue = postFxManager.getClipZValue();
    }
    if (pressed.DpadRight) {
      postFxManager.adjustClipZValue(0x100);
      g_debug_menu.fogClipZValue = postFxManager.getClipZValue();
    }
  }

  // Get number of items in current tab
  int itemsInCurrentTab = getItemCountForTab(g_debug_menu.currentTab);

  // Navigate up (skip when adjusting CLIP_Z with DpadLeft/Right)
  if (clicked.DpadUp && !g_debug_menu.fogUseCustomClipZ) {
    g_selected_debug_menu_item--;
    if (g_selected_debug_menu_item < 0) {
      g_selected_debug_menu_item = itemsInCurrentTab - 1;
    }
  }

  // Navigate down (skip when adjusting CLIP_Z with DpadLeft/Right)
  if (clicked.DpadDown && !g_debug_menu.fogUseCustomClipZ) {
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
        
        // Sincronizar com PostFxManager quando togglear "Fog: Use Custom CLIP_Z"
        if (g_all_menu_items[i].value == &g_debug_menu.fogUseCustomClipZ) {
          auto& postFxManager = PostFxManager::getInstance();
          
          if (g_debug_menu.fogUseCustomClipZ) {
            // Ao ativar, sincronizar valor atual do PostFxManager
            g_debug_menu.fogClipZValue = postFxManager.getClipZValue();
          } else {
            // Ao desativar, resetar para valor padrão
            postFxManager.resetClipZValueToDefault();
            g_debug_menu.fogClipZValue = postFxManager.getClipZValue();
          }
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