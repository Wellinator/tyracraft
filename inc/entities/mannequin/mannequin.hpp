
#include "constants.hpp"
#include "entities/animation/animated.hpp"
#include <cstdint>
#include <vector>
#include <tyra>

using Tyra::M4x4;
using Tyra::StaticPipeline;
using Tyra::Texture;
using Tyra::Vec4;

class Mannequin : public Animated {
 private:
  M4x4 model;

  std::vector<Vec4> vertices = {};
  std::vector<Color> verticesColors = {};
  std::vector<Vec4> uvMap = {};

 public:
  M4x4 scale = M4x4::Identity;
  M4x4 rotation = M4x4::Identity;
  M4x4 translation = M4x4::Identity;

  Mannequin();
  ~Mannequin();

  void update(const float deltaTime);
  void render(StaticPipeline* pipeline, Texture* skinTexture);
};