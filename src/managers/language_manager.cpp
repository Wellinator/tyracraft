#include "managers/language_manager.hpp"

LanguageManager::LanguageManager() {
  _langDictionary = nlohmann::json::value_t::object;
}

LanguageManager::~LanguageManager() {}

void LanguageManager::loadLanguage(nlohmann::json* pLanguage) {
  _langDictionary = nlohmann::json(*pLanguage);
  _langDictionary.flatten();

  TYRA_LOG("Language defined successfully!");
}

void LanguageManager::unloadLanguage() {
  _langDictionary.clear();
  _langDictionary = nlohmann::json(nullptr);
}

std::string LanguageManager::translate(const char* key) {
  return _langDictionary[nlohmann::json::json_pointer(key)].get<std::string>();
}