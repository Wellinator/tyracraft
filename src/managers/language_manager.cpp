#include "managers/language_manager.hpp"

/**
 * @brief Definition of global json language
 *
 */
nlohmann::json g_language_repository(nlohmann::json::value_t::object);

void LanguageManager_SetLanguage(nlohmann::json language) {
  g_language_repository = nlohmann::json(language);
  TYRA_LOG("Language defined successfully!");
}

void LanguageManager_UnsetLanguage() {
  g_language_repository.clear();
  g_language_repository = nlohmann::json(nullptr);
}
