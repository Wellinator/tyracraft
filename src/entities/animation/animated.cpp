#include "entities/animation/animated.hpp"
#include <algorithm>

using Tyra::Mesh;
using Tyra::MeshMaterial;
using Tyra::Vec4;

Animated::Animated(Tyra::Mesh** framesArray, const int size) {
  setFrames(framesArray, size);
}

Animated::~Animated() {
  clearFrames();
  clearAnimations();
}

void Animated::update(const float& deltaTime) {
  if (currentAnimation == -1 || _currentAnimationFramesIndices.empty()) return;

  animationTimer += deltaTime * 1000.0f;  // Convert to milliseconds

  const int animationFrameCount = _currentAnimationFramesIndices.size();
  const float timePerFrame = animationDurationInMs / animationFrameCount;

  if (animationTimer >= animationDurationInMs) {
    if (isLooping) {
      animationTimer = fmod(animationTimer, animationDurationInMs);
      // Calculate the correct frame based on the wrapped timer
      int newAnimationFrameIndex =
          static_cast<int>(animationTimer / timePerFrame);
      newAnimationFrameIndex =
          std::min(newAnimationFrameIndex, animationFrameCount - 1);

      setAnimationFrameIndex(newAnimationFrameIndex);
      onAnimationComplete();
    } else {
      animationTimer = animationDurationInMs;
      // Set to last frame in the animation sequence
      int lastFrameIndex = animationFrameCount - 1;
      setAnimationFrameIndex(lastFrameIndex);
      currentAnimation = -1;  // Stop the animation
      onAnimationEnd();
    }
  } else {
    // Calculate which frame in the animation sequence we should be at
    // based on time per frame distribution
    int newAnimationFrameIndex =
        static_cast<int>(animationTimer / timePerFrame);

    // Clamp to valid range
    newAnimationFrameIndex =
        std::min(newAnimationFrameIndex, animationFrameCount - 1);

    // printf("New Animation Frame Index: %d (time: %.2f, per frame: %.2f)\n",
    //        newAnimationFrameIndex, animationTimer, timePerFrame);

    if (newAnimationFrameIndex != _currentAnimationFrameIndex) {
      if (wrapFrames && newAnimationFrameIndex < _currentAnimationFrameIndex) {
        // If wrapping is enabled and we've looped back, call
        // onAnimationComplete
        onAnimationComplete();
      }
      setAnimationFrameIndex(newAnimationFrameIndex);
    }
  }
}

void Animated::setFrames(Tyra::Mesh** framesArray, const int size) {
  for (int i = 0; i < size; i++) {
    frames.push_back(framesArray[i]);
  }
  _currentAnimationFrameIndex = 0;
}

void Animated::clearFrames() {
  frames.clear();
  _currentAnimationFrameIndex = -1;
}

void Animated::setAnimation(const int animationId) {
  if (animations.find(animationId) == animations.end()) return;

  const AnimationOptions& animationOptions = animations[animationId];
  currentAnimation = animationOptions.animationId;
  _currentAnimationFramesIndices = animationOptions.framesIndices;
  animationDurationInMs = animationOptions.durationInMs;

  // If we zero the timer, we always start from the first frame
  // In this case, we want to keep the current timer position for a smooth
  // transition
  animationTimer = fmod(animationTimer, animationDurationInMs);

  isLooping = animationOptions.loop;
  wrapFrames = animationOptions.wrapFrames;

  // The _lastAnimationFrameIndex will be the current frame before switching
  // to the new animation
  _lastAnimationFrameIndex = _currentAnimationFrameIndex;

  // The _lastAnimationFrameIndex now becomes the first frame of the new
  // animation
  _lastAnimationFrameIndex = 0;

  onAnimationStart();
}

void Animated::addAnimation(const AnimationOptions& animationOptions) {
  animations[animationOptions.animationId] = animationOptions;
}

void Animated::clearAnimations() {
  animations.clear();
  currentAnimation = -1;
  animationDurationInMs = 0.0f;
  animationTimer = 0.0f;
  isLooping = false;
  wrapFrames = false;
}

int Animated::getNextAnimationFrameIndex() {
  const int animationFrameCount = _currentAnimationFramesIndices.size();
  const int newIndex = (_currentAnimationFrameIndex + 1) % animationFrameCount;
  return newIndex;
}

int Animated::getPrevAnimationFrameIndex() {
  const int animationFrameCount = _currentAnimationFramesIndices.size();
  const int newIndex = (_currentAnimationFrameIndex - 1 + animationFrameCount) %
                       animationFrameCount;
  return newIndex;
}

void Animated::nextFrame() {
  _lastAnimationFrameIndex = _currentAnimationFrameIndex;
  _currentAnimationFrameIndex = getNextAnimationFrameIndex();
}

void Animated::prevFrame() {
  _lastAnimationFrameIndex = _currentAnimationFrameIndex;
  _currentAnimationFrameIndex = getPrevAnimationFrameIndex();
}

void Animated::setAnimationFrameIndex(int animationFrameIndex) {
  const int animationFrameCount = _currentAnimationFramesIndices.size();
  if (animationFrameIndex >= 0 && animationFrameIndex < animationFrameCount) {
    _lastAnimationFrameIndex = _currentAnimationFrameIndex;
    _currentAnimationFrameIndex = animationFrameIndex;
  }
}

int Animated::getCurrentFrame() const { return _currentAnimationFrameIndex; }

void Animated::removeAnimation(const int animationId) {
  animations.erase(animationId);
}

AnimationOptions Animated::getCurrentAnimation() const {
  return animations.at(currentAnimation);
}

bool Animated::isAnimationPlaying() const { return currentAnimation != -1; }

bool Animated::isAnimationLooping() const { return isLooping; }

float Animated::getAnimationDuration() const { return animationDurationInMs; }

int Animated::getCurrentActualFrameIndex() const {
  TYRA_ASSERT(!_currentAnimationFramesIndices.empty(),
              "Animation not setted or has no frames");

  return _currentAnimationFramesIndices[_currentAnimationFrameIndex];
}

int Animated::getLastActualFrameIndex() const {
  TYRA_ASSERT(!_currentAnimationFramesIndices.empty(),
              "Animation not setted or has no frames");
  return _currentAnimationFramesIndices[_lastAnimationFrameIndex];
}

void Animated::fillDrawDataByFrame(std::vector<Vec4>* pVertices,
                                   std::vector<Color>* pVerticesColors,
                                   std::vector<Vec4>* pUvMap) {
  pVertices->clear();
  pVerticesColors->clear();
  pUvMap->clear();

  // TODO: calc total of vertices
  pVertices->reserve(100);
  pVerticesColors->reserve(100);
  pUvMap->reserve(100);

  // Get actual frame index from the animation sequence
  const int actualFrameIndex = getCurrentActualFrameIndex();
  if (actualFrameIndex < 0 ||
      actualFrameIndex >= static_cast<int>(frames.size())) {
    return TYRA_TRAP("Animated::fillDrawDataByFrame\nInvalid frame index: ",
                     actualFrameIndex);  // Invalid frame index
  }
  const Tyra::Mesh* frame = frames[actualFrameIndex];

  const size_t materialCount = frame->materials.size();
  for (size_t i = 0; i < materialCount; i++) {
    auto material = frame->materials[i];

    const auto& vertices = material->frames[0]->vertices;
    const size_t count = material->frames[0]->count;
    for (size_t j = 0; j < count; j++) {
      pVertices->emplace_back(vertices[j]);
    }

    const auto& uvs = material->frames[0]->textureCoords;
    for (size_t j = 0; j < count; j++) {
      pUvMap->emplace_back(uvs[j]);
    }
  }
}

void Animated::fillDrawDataByLerp(std::vector<Vec4>* pVertices,
                                  std::vector<Color>* pVerticesColors,
                                  std::vector<Vec4>* pUvMap) {
  pVertices->clear();
  pVerticesColors->clear();
  pUvMap->clear();

  // TODO: calc total of vertices
  pVertices->reserve(100);
  pVerticesColors->reserve(100);
  pUvMap->reserve(100);

  // Get actual frame indices from the animation sequence
  int actualLastFrameIndex = getLastActualFrameIndex();
  int actualCurrentFrameIndex = getCurrentActualFrameIndex();

  if (actualLastFrameIndex < 0 || actualCurrentFrameIndex < 0 ||
      actualLastFrameIndex >= static_cast<int>(frames.size()) ||
      actualCurrentFrameIndex >= static_cast<int>(frames.size())) {
    return TYRA_TRAP("Animated::fillDrawDataByLerp\nInvalid frame indices: ",
                     actualLastFrameIndex,
                     actualCurrentFrameIndex);  // Invalid frame indices
  }

  const Tyra::Mesh* frameFrom = frames[actualLastFrameIndex];
  const Tyra::Mesh* frameTo = frames[actualCurrentFrameIndex];

  // Calculate interpolation factor within the current frame
  const int animationFrameCount = _currentAnimationFramesIndices.size();
  const float timePerFrame = animationDurationInMs / animationFrameCount;
  const float currentFrameStartTime =
      _currentAnimationFrameIndex * timePerFrame;
  const float timeInCurrentFrame = animationTimer - currentFrameStartTime;
  const float lerp = timeInCurrentFrame / timePerFrame;

  const size_t materialCount = frameFrom->materials.size();
  for (size_t i = 0; i < materialCount; i++) {
    auto materialFrom = frameFrom->materials[i];
    auto materialTo = frameTo->materials[i];

    const size_t count = materialFrom->frames[0]->count;

    const auto& verticesFrom = materialFrom->frames[0]->vertices;
    const auto& verticesTo = materialTo->frames[0]->vertices;

    const auto& uvsFrom = materialFrom->frames[0]->textureCoords;
    const auto& uvsTo = materialTo->frames[0]->textureCoords;

    materialFrom->frames[0]->colors;

    for (size_t j = 0; j < count; j++) {
      Vec4 vertexFrom = verticesFrom[j];
      Vec4 vertexTo = verticesTo[j];
      Vec4 vertex = Vec4::getByLerp(vertexFrom, vertexTo, lerp);
      pVertices->emplace_back(vertex);

      Vec4 uvFrom = uvsFrom[j];
      Vec4 uvTo = uvsTo[j];
      Vec4 uv = Vec4::getByLerp(uvFrom, uvTo, lerp);
      pUvMap->emplace_back(uv);
    }

    // Check if colors were loaded
    // const auto& colorsFrom = materialFrom->frames[0]->colors;
    // const auto& colorsTo = materialTo->frames[0]->colors;
    // for (size_t j = 0; j < count; j++) {
    //   Color colorFrom = colorsFrom[j];
    //   Color colorTo = colorsTo[j];
    //   Color color = lerp(colorFrom, colorTo, lerp);
    //   pVerticesColors->push_back(color);
    // }
  }
}
