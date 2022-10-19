#include "entities/sfx_library.hpp"

SfxLibrary::SfxLibrary(Audio* t_audio) {
  this->t_audio = t_audio;
  this->buildSoundFXLibraries(t_audio);
}

SfxLibrary::~SfxLibrary() {
  for (size_t i = 0; i < categories.size(); i++) {
    delete categories[i];
  }
  categories.clear();
}

void SfxLibrary::buildSoundFXLibraries(Audio* t_audio) {
  categories.push_back(new SfxRandomCategory(t_audio));
  categories.push_back(new SfxDigCategory(t_audio));
}

SfxLibraryCategory* SfxLibrary::getCategory(SoundFxCategory idCategory) {
  for (size_t i = 0; i < categories.size(); i++)
    if (categories[i]->id == idCategory) return categories[i];

  TYRA_ASSERT(false, "Category not found in sound fx library");
  return nullptr;
}

SfxLibrarySound* SfxLibrary::getSound(SoundFxCategory idCategory,
                                      SoundFX idSound) {
  SfxLibraryCategory* t_category = this->getCategory(idCategory);
  if (t_category != nullptr) return t_category->getSound(idSound);

  return nullptr;
}
