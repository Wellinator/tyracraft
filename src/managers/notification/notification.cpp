#include "managers/notification/notification.hpp"
#include "managers/font/font_manager.hpp"

Notification::Notification(Renderer* t_renderer, Texture* t_Texture) {
  pRenderer = t_renderer;
  pTexture = t_Texture;

  init();
};

Notification::Notification(Renderer* t_renderer, Texture* t_Texture,
                           const char* c_title, const char* c_message) {
  pRenderer = t_renderer;
  pTexture = t_Texture;
  title = c_title;
  message = c_message;

  init();
};

Notification::Notification(Renderer* t_renderer, Texture* t_Texture,
                           const std::string& title, const std::string& message,
                           const float& width, const float& height,
                           const float& timeout, const NotificationType& typ,
                           const NotificationAlignment& alignment) {
  pRenderer = t_renderer;
  pTexture = t_Texture;

  this->title = title;
  this->message = message;
  this->timeout = timeout;
  this->width = width;
  this->height = height;

  this->type = type;
  this->alignment = alignment;

  init();
};

Notification::~Notification() { pTexture->removeLinkById(bgSprite.id); };

void Notification::init() {
  pTexture->addLink(bgSprite.id);
  calculatePositionByAlignment();

  // Configure sprite
  bgSprite.size.x = width;
  bgSprite.size.y = height;

  bgSprite.color.r = 222;
  bgSprite.color.g = 222;
  bgSprite.color.b = 222;

  bgSprite.mode = Tyra::SpriteMode::MODE_STRETCH;
}

void Notification::dismiss() {
  elapsedTime = timeout;
  expired = true;
}

void Notification::calculatePositionByAlignment() {
  auto& settings = pRenderer->core.getSettings();

  switch (alignment) {
    case NotificationAlignment::TopLeft:
      position.x = margin;
      position.y = margin;
      break;

    case NotificationAlignment::TopRight:
      position.x = settings.getWidth() - width - margin;
      position.y = margin;
      break;

    case NotificationAlignment::BottomRight:
      position.x = settings.getWidth() - width - margin;
      position.y = settings.getHeight() - height - margin;
      break;

    case NotificationAlignment::BottomLeft:
      position.x = margin;
      position.y = settings.getHeight() - height - margin;
      break;

    default:
      position.x = settings.getWidth() / 2.0F - width / 2.0F;
      position.y = settings.getHeight() / 2.0F - height / 2.0F;
      break;
  }
}

// TODO: implement a fadeout after expire and before destroy
void Notification::update(const float deltaTime) {
  elapsedTime += deltaTime;
  expired = elapsedTime >= timeout;
  destroy = expired;

  alpha = 1.0F - elapsedTime / timeout;

  bgSprite.position.set(position);
}

void Notification::render() {
  // Backgound
  pRenderer->renderer2D.render(bgSprite);

  // Texts
  Vec2 titlePos = Vec2(position.x + padding, position.y + padding);
  FontManager::getInstance()->printText(
      title, FontOptions(titlePos, Color(0x5A, 0x2B, 0x5A), 0.8F,
                         TextAlignment::Left));

  const float boddyOffset = 22.0f;
  Vec2 messagePos = titlePos;
  messagePos.y += boddyOffset;

  FontManager::getInstance()->printText(
      message,
      FontOptions(messagePos, Color(0, 0, 0), 0.8F, TextAlignment::Left));
}