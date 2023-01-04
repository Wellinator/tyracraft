#pragma once

class NewGameOptions {
 public:
  NewGameOptions(){};
  ~NewGameOptions(){};

  bool makeFlat = false;
  bool enableWater = false;
  bool enableCaves = false;
  bool enableTrees = true;
  float initialTime = 6000;
};
