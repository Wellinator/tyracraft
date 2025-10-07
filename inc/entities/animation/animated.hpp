#pragma once

#include <tyra>
#include <vector>
#include <unordered_map>

using Tyra::Color;
using Tyra::Vec4;

struct AnimationOptions {
  int animationId;
  std::vector<u8> framesIndices;

  // Total duration of the animation in milliseconds. Primary
  // frame duration is calculated based on this value
  float durationInMs = 0.0f;

  // Optional, used if durationInMs is not set. Apply per frame duration
  std::vector<float> framesDurationsInMs = {};

  bool loop;
  bool wrapFrames;

  AnimationOptions()
      : animationId(-1), durationInMs(1000.0f), loop(true), wrapFrames(false) {
        };
};

class Animated {
 private:
  std::vector<Tyra::Mesh*> frames;  // All available mesh frames
  std::unordered_map<int, AnimationOptions> animations;

  int currentAnimation;
  // Indices into _currentAnimationFramesIndices (not direct frame indices)
  int _lastAnimationFrameIndex = 0;     // Previous index in animation sequence
  int _currentAnimationFrameIndex = 0;  // Current index in animation sequence

  // Total duration of the animation in milliseconds. Primary
  // frame duration is calculated based on this value
  float animationDurationInMs = 0.0f;

  // Optional, used if animationDurationInMs is not set. Apply per frame duration
  std::vector<float> framesDurationsInMs = {};

  float animationTimer;
  bool isLooping;
  bool wrapFrames;
  // Contains indices that reference frames in the 'frames' vector
  std::vector<u8> _currentAnimationFramesIndices;

  int getNextAnimationFrameIndex();
  int getPrevAnimationFrameIndex();
  void nextFrame();
  void prevFrame();
  void setAnimationFrameIndex(int animationFrameIndex);

  // Helper methods to get actual frame indices
  int getCurrentActualFrameIndex() const;
  int getLastActualFrameIndex() const;

 public:
  Animated();
  Animated(Tyra::Mesh** framesArray, const int size);
  virtual ~Animated();

  virtual void update(const float& deltaTime);
  virtual void render() { TYRA_TRAP("Animated::render() not implemented"); };

  void setFrames(Tyra::Mesh** framesArray, const int size);
  void setAnimation(const int animationId);
  void addAnimation(const AnimationOptions& animationOptions);
  void clearAnimations();
  void clearFrames();
  int getCurrentFrame() const;
  AnimationOptions getCurrentAnimation() const;
  void removeAnimation(const int animationId);
  bool isAnimationPlaying() const;
  bool isAnimationLooping() const;
  float getAnimationDuration() const;
  void fillDrawDataByFrame(std::vector<Vec4>* pVertices,
                           std::vector<Color>* pVerticesColors,
                           std::vector<Vec4>* pUvMap);
  void fillDrawDataByLerp(std::vector<Vec4>* pVertices,
                          std::vector<Color>* pVerticesColors,
                          std::vector<Vec4>* pUvMap);

 protected:
  virtual void onAnimationStart() {};
  virtual void onAnimationEnd() {};
  virtual void onAnimationComplete() {};
};
