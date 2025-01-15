#include "managers/notification/notification_manager.hpp"

NotificationManager::NotificationManager(Renderer* t_renderer)
    : Singleton<NotificationManager>() {
  pRenderer = t_renderer;
  loadTextures();
}

NotificationManager::~NotificationManager() { destroyAllNotifications(); }

void NotificationManager::update(const float deltaTime) {
  size_t count = notifications.size();
  u8 hasDestroyedNotifications = false;

  for (size_t i = 0; i < count; i++) {
    if (notifications[i]->destroy) {
      hasDestroyedNotifications = true;
      continue;
    }

    notifications[i]->update(deltaTime);
  }

  if (hasDestroyedNotifications) destroyExpiredNotifications();
};

void NotificationManager::render() {
  size_t count = notifications.size();
  for (size_t i = 0; i < count; i++) {
    if (notifications[i]->destroy) continue;
    notifications[i]->render();
  }
};

Notification* NotificationManager::notify(const std::string& title,
                                          const std::string& message) {
  return notify(title.c_str(), message.c_str());
};

Notification* NotificationManager::notify(const char* title,
                                          const char* message) {
  Notification* pNotification =
      new Notification(pRenderer, popupLight, title, message);
  notifications.emplace_back(pNotification);

  return pNotification;
};

void NotificationManager::loadTextures() {
  backgroundDefault = pRenderer->core.texture.repository.add(
      FileUtils::fromCwd("textures/gui/notification/background_default.png"));
  popupDark = pRenderer->core.texture.repository.add(
      FileUtils::fromCwd("textures/gui/notification/popup_dark.png"));
  popupLight = pRenderer->core.texture.repository.add(
      FileUtils::fromCwd("textures/gui/notification/popup_light.png"));
}

void NotificationManager::destroyExpiredNotifications() {
  notifications.erase(std::remove_if(notifications.begin(), notifications.end(),
                                     [](Notification* n) {
                                       u8 shouldDestroy = n->destroy;
                                       if (shouldDestroy) delete n;
                                       return shouldDestroy;
                                     }),
                      notifications.end());

  notifications.clear();
  notifications.shrink_to_fit();
}

void NotificationManager::destroyAllNotifications() {
  size_t size = notifications.size();
  for (size_t i = 0; i < size; i++) {
    delete notifications[i];
  }

  notifications.clear();
  notifications.shrink_to_fit();
}