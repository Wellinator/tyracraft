#pragma once

#include <tamtypes.h>

// Define debug mode
extern u8 g_debug_mode;

#ifdef DEBUG_MODE
#include "tyra"
using Tyra::Pad;

// Implement a debug menu to toggle features, print debug info and turn on/off
// some systems
struct DebugMenu {
  u8 showDebugMenu = false;
  u8 showCollisionBoxes = false;
  u8 showChunkBorders = false;
  u8 showPlayerBoundingBox = false;
  u8 showTargetedBlockBoundingBox = false;
  u8 showPlayerInfo = false;
  u8 showWorldInfo = false;
  u8 showBlockUpdateInfo = false;
  u8 showChunkUpdateInfo = false;
  u8 showLightUpdateInfo = false;
  u8 showLiquidPropagationInfo = false;
  u8 showMobPathfinding = false;
  u8 enableDayNightCycle = true;
  u8 enableClouds = true;
  u8 enableRenderOpaque = true;
  u8 enableRenderTranslucent = true;
  u8 enableRenderParticles = true;
  u8 enableRenderMobs = true;
  u8 enableRenderPlayers = true;
  u8 enableRenderBlockDamage = true;
  u8 enableRenderUI = true;
  u8 enablePostFx = false;
  u8 godMode = false;
  u8 noclipMode = false;
  u8 logChunkMemoryUsage = false;
};

extern DebugMenu g_debug_menu;

void renderDebugMenu();
void handleDebugInput(Pad* pPad);
#endif  // end if DEBUG_MODE