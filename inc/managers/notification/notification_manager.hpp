#pragma once

#include "tyra"
#include <tamtypes.h>
#include <vector>
#include <string>
#include <locale>
#include "singleton.hpp"
#include "managers/font/font_manager.hpp"
#include "managers/notification/notification.hpp"

using Tyra::Color;
using Tyra::FileUtils;
using Tyra::Renderer;
using Tyra::Sprite;
using Tyra::Texture;
using Tyra::Vec2;

class NotificationManager : public Singleton<NotificationManager> {
 public:
  NotificationManager(Renderer* t_renderer);
  ~NotificationManager();

  void update(const float deltaTime);
  void render();

  Notification* notify(const char* title, const char* message);
  Notification* notify(const std::string& title, const std::string& message);

 private:
  std::vector<Notification*> notifications;

  Renderer* pRenderer;
  Texture* backgroundDefault = nullptr;
  Texture* popupDark = nullptr;
  Texture* popupLight = nullptr;

  void loadTextures();
  void destroyAllNotifications();
  void destroyExpiredNotifications();
};
