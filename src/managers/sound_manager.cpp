#include "managers/sound_manager.hpp"
#include "utils.hpp"

SoundManager::SoundManager(Engine* t_engine) {
  this->t_engine = t_engine;
  this->soundLibrary = new SfxLibrary(&t_engine->audio);
}

SoundManager::~SoundManager() { delete soundLibrary; }

void SoundManager::playSfx(const SoundFxCategory& idCategory,
                           const SoundFX& idSound) {
  this->playSfx(idCategory, idSound, getAvailableChannel());
};

void SoundManager::playSfx(const SoundFxCategory& idCategory,
                           const SoundFX& idSound, const s8& t_ch) {
  SfxLibrarySound* t_sound = this->getSound(idCategory, idSound);
  if (t_sound) this->t_engine->audio.adpcm.tryPlay(t_sound->_sound, t_ch);
};

void SoundManager::playSfx(SfxLibrarySound* t_sound, const s8& t_ch) {
  if (t_sound) this->t_engine->audio.adpcm.tryPlay(t_sound->_sound, t_ch);
};

void SoundManager::setSfxVolume(const u8& t_vol, const s8& t_ch) {
  return this->t_engine->audio.adpcm.setVolume(t_vol, t_ch);
};

const int SoundManager::getAvailableChannel() {
  currentAdpcmChannel %= MAX_ADPCM_CH;
  return currentAdpcmChannel++;
};

const std::string SoundManager::GetRandonSongFromPath(
    const std::string& songsDir) {
  std::vector<UtilDirectory> dirFilesList = Utils::listDir(songsDir.c_str());
  std::vector<std::string> availableSongs;

  for (size_t i = 0; i < dirFilesList.size(); i++) {
    const UtilDirectory dir = dirFilesList.at(i);
    const std::string fileExtension =
        FileUtils::getExtensionOfFilename(dir.name);

    if (strncmp(fileExtension.c_str(), "wav", 3) == 0) {
      std::string songPath = std::string(songsDir).append(dir.name);
      availableSongs.push_back(songPath);
    }
  }
  if (availableSongs.size() > 0) {
    int randSongIndex = Math::randomi(0, (availableSongs.size() - 1));
    return availableSongs.at(randSongIndex);
  } else
    return std::string("");
}