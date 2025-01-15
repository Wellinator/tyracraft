#pragma once
#include <tamtypes.h>
#include <string>
#include "tyra"
#include "singleton.hpp"
#include "constants.hpp"
#include "3libs/nlohmann/json.hpp"

class LanguageManager : public Singleton<LanguageManager> {
 public:
  LanguageManager();
  ~LanguageManager();

  void loadLanguage(nlohmann::json* pLanguage);
  void unloadLanguage();

  /**
   * Return a std::string
   * @param key const char*: JSON Pointer format
   * @example "/nested/one"
   */
  std::string translate(const char* key);

  static std::string Translate(const char* key) {
    return LanguageManager::getInstance()->translate(key);
  };

 private:
  //  flattened json
  nlohmann::json _langDictionary;
};
