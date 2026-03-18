#include "managers/save/save_migration.hpp"
#include "managers/save/save_format.hpp"

namespace TyraCraft {

// ============================================================================
// HELPER: Legacy draw distance mapping
// ============================================================================

// ============================================================================
// IDENTITY MIGRATION V1
// ============================================================================

SaveResult MigrationV1::Apply(SaveSerializer& serializer,
                              NewGameOptions* outOptions,
                              LevelMap* outMap,
                              Tyra::Vec4* outPlayerPos,
                              float* outCameraPitch,
                              float* outCameraYaw,
                              uint64_t* outTicksCounter,
                              uint64_t* outElapsedRealTime,
                              uint64_t* outTicksDayCounter) {
  // V1 (New Folder-Based Format): Direct read using standard format
  SaveResult result = SaveFormat::ReadWorldOptions(serializer, outOptions);
  if (!result) return result;

  // Player state
  result = SaveFormat::ReadPlayerState(serializer, outPlayerPos, outCameraPitch,
                                        outCameraYaw);
  if (!result) return result;

  // Tick state
  return SaveFormat::ReadTickState(serializer, outTicksCounter, outElapsedRealTime,
                                      outTicksDayCounter);
}

}  // namespace TyraCraft
