#pragma once

struct NewGameOptions {
  NewGameOptions(){};
  ~NewGameOptions(){};

 public:
  bool makeFlat = false;
  bool enableWater = false;
  bool enableTrees = true;
};
