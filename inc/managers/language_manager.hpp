#pragma once
#include <tamtypes.h>
#include <string>
#include "tyra"
#include "constants.hpp"
#include "3libs/nlohmann/json.hpp"

/**
 * @brief Declaration of global json language
 *
 */
extern nlohmann::json g_language_repository;

void LanguageManager_SetLanguage(nlohmann::json language);
void LanguageManager_UnsetLanguage();
