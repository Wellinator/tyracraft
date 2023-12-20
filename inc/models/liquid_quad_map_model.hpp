#pragma once

struct LiquidQuadMapModel {
  float NW;
  float NE;
  float SE;
  float SW;

  void print(const char* name) const {
    printf("----  QuadMap %s ----\n", name);
    printf("| NW: %f  NE: %f |\n", NW, NE);
    printf("|                |\n");
    printf("| SW: %f  SE: %f |\n", SW, SE);
    printf("---------------------\n\n");
  }
};
