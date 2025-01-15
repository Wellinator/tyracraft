#pragma once
#include <tamtypes.h>
#include "tyra"

using Tyra::Color;
using Tyra::FileUtils;
using Tyra::Renderer;
using Tyra::Sprite;
using Tyra::Texture;
using Tyra::Vec2;

enum class NotificationAlignment {
  TopLeft,
  TopRight,
  BottomLeft,
  BottomRight,
  Center
};

enum class NotificationType {
  Default,
  Success,
  Warning,
  Error,
};

class Notification {
 public:
  Notification(Renderer* t_renderer, Texture* t_Texture);

  Notification(Renderer* t_renderer, Texture* t_Texture, const char* c_title,
               const char* c_message);

  Notification(
      Renderer* t_renderer, Texture* t_Texture, const std::string& title,
      const std::string& message, const float& width, const float& height,
      const float& timeout,
      const NotificationType& type = NotificationType::Default,
      const NotificationAlignment& alignment = NotificationAlignment::TopRight);

  ~Notification();

  void update(const float deltaTime);
  void render();
  void dismiss();

  NotificationAlignment alignment = NotificationAlignment::TopRight;
  NotificationType type = NotificationType::Default;

  // Time to fadeout in milliseconds
  float timeout = 3.0F;

  std::string title;
  std::string message;

  u8 destroy = false;
  u8 expired = false;

 private:
  void init();
  void calculatePositionByAlignment();

  Vec2 position = Vec2(0.0f, 0.0f);
  Vec2 offset = Vec2(0.0f, 0.0f);

  float elapsedTime = 0.0F;
  float alpha = 1.0F;
  float width = 240.0F;
  float height = 50.0F;
  const float padding = 2.0F;
  const float margin = 10.0F;
  const float iconOffset = 25.0F;

  Renderer* pRenderer;
  Texture* pTexture;
  Sprite bgSprite;
};
