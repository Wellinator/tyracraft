#include "managers/save/save_migration.hpp"
#include "managers/save/save_format.hpp"

namespace TyraCraft {

// ============================================================================
// HELPER: Legacy draw distance mapping
// ============================================================================

static DrawDistanceMode MapLegacyDrawDistance(u8 legacyDistance) {
  if (legacyDistance <= 10)
    return DrawDistanceMode::Low;
  else if (legacyDistance <= 16)
    return DrawDistanceMode::Medium;
  else if (legacyDistance <= 24)
    return DrawDistanceMode::High;
  else
    return DrawDistanceMode::Auto;
}

// ============================================================================
// MIGRATION V1 → V3
// ============================================================================

SaveResult MigrationV1ToV2::Apply(SaveSerializer& serializer,
                                   NewGameOptions* outOptions,
                                   LevelMap* outMap,
                                   Tyra::Vec4* outPlayerPos,
                                   float* outCameraPitch,
                                   float* outCameraYaw,
                                   uint64_t* outTicksCounter,
                                   uint64_t* outElapsedRealTime,
                                   uint64_t* outTicksDayCounter) {
  // V1 Format: seed, gameMode, name, legacyDrawDistance, initialTime, type, texturePack

  SaveResult result = serializer.ReadUInt32(&outOptions->seed);
  if (!result) return result;

  uint8_t gameMode;
  result = serializer.ReadUInt8(&gameMode);
  if (!result) return result;
  outOptions->gameMode = (GameMode)gameMode;

  result = serializer.ReadString(&outOptions->name,
                                 SaveSerializer::MAX_SAVE_NAME_LENGTH);
  if (!result) return result;

  // V1-specific: Draw distance is raw u8, must map to enum
  u8 legacyDrawDistance;
  result = serializer.ReadUInt8(&legacyDrawDistance);
  if (!result) return result;
  outOptions->drawDistanceMode = MapLegacyDrawDistance(legacyDrawDistance);

  // Rest of options
  result = serializer.ReadFloat(&outOptions->initialTime);
  if (!result) return result;

  uint8_t worldType;
  result = serializer.ReadUInt8(&worldType);
  if (!result) return result;
  outOptions->type = (WorldType)worldType;

  result = serializer.ReadString(&outOptions->texturePack,
                                 SaveSerializer::MAX_TEXTURE_PACK_NAME);
  if (!result) return result;

  // V1-specific: Player position read as Vec4 struct
  // Also has the w component fix patch
  result = serializer.ReadVec4(outPlayerPos);
  if (!result) return result;
  outPlayerPos->w = 1.0f;  // PATCH: Fix for v1 saves that missed w component

  // Camera direction (pitch, yaw)
  result = serializer.ReadFloat(outCameraPitch);
  if (!result) return result;
  
  result = serializer.ReadFloat(outCameraYaw);
  if (!result) return result;

  // Tick state
  result = SaveFormat::ReadTickState(serializer, outTicksCounter, outElapsedRealTime,
                                      outTicksDayCounter);
  if (!result) return result;

  // World structure and data
  result = SaveFormat::ReadWorldStructure(serializer, outMap);
  if (!result) return result;

  return SaveFormat::ReadWorldData(serializer, outMap);
}

DrawDistanceMode MigrationV1ToV2::MapLegacyDrawDistance(u8 legacyDistance) {
  return ::TyraCraft::MapLegacyDrawDistance(legacyDistance);
}

// ============================================================================
// MIGRATION V2 → V3
// ============================================================================

SaveResult MigrationV2ToV3::Apply(SaveSerializer& serializer,
                                   NewGameOptions* outOptions,
                                   LevelMap* outMap,
                                   Tyra::Vec4* outPlayerPos,
                                   float* outCameraPitch,
                                   float* outCameraYaw,
                                   uint64_t* outTicksCounter,
                                   uint64_t* outElapsedRealTime,
                                   uint64_t* outTicksDayCounter) {
  // V2 Format: seed, gameMode, name, legacyDrawDistance, initialTime, type, texturePack
  // (same as V1, but player position method differs)

  SaveResult result = serializer.ReadUInt32(&outOptions->seed);
  if (!result) return result;

  uint8_t gameMode;
  result = serializer.ReadUInt8(&gameMode);
  if (!result) return result;
  outOptions->gameMode = (GameMode)gameMode;

  result = serializer.ReadString(&outOptions->name,
                                 SaveSerializer::MAX_SAVE_NAME_LENGTH);
  if (!result) return result;

  // V2-specific: Draw distance is raw u8 (same as V1)
  u8 legacyDrawDistance;
  result = serializer.ReadUInt8(&legacyDrawDistance);
  if (!result) return result;
  outOptions->drawDistanceMode = MapLegacyDrawDistance(legacyDrawDistance);

  // Rest of options
  result = serializer.ReadFloat(&outOptions->initialTime);
  if (!result) return result;

  uint8_t worldType;
  result = serializer.ReadUInt8(&worldType);
  if (!result) return result;
  outOptions->type = (WorldType)worldType;

  result = serializer.ReadString(&outOptions->texturePack,
                                 SaveSerializer::MAX_TEXTURE_PACK_NAME);
  if (!result) return result;

  // V2-specific: Player position read as 4×f32 (correct way)
  result = serializer.ReadVec4(outPlayerPos);
  if (!result) return result;

  // Camera direction (pitch, yaw)
  result = serializer.ReadFloat(outCameraPitch);
  if (!result) return result;
  
  result = serializer.ReadFloat(outCameraYaw);
  if (!result) return result;

  // Tick state
  result = SaveFormat::ReadTickState(serializer, outTicksCounter, outElapsedRealTime,
                                      outTicksDayCounter);
  if (!result) return result;

  // World structure and data
  result = SaveFormat::ReadWorldStructure(serializer, outMap);
  if (!result) return result;

  return SaveFormat::ReadWorldData(serializer, outMap);
}

DrawDistanceMode MigrationV2ToV3::MapLegacyDrawDistance(u8 legacyDistance) {
  return ::TyraCraft::MapLegacyDrawDistance(legacyDistance);
}

// ============================================================================
// IDENTITY MIGRATION V3 → V3
// ============================================================================

SaveResult MigrationV3::Apply(SaveSerializer& serializer,
                              NewGameOptions* outOptions,
                              LevelMap* outMap,
                              Tyra::Vec4* outPlayerPos,
                              float* outCameraPitch,
                              float* outCameraYaw,
                              uint64_t* outTicksCounter,
                              uint64_t* outElapsedRealTime,
                              uint64_t* outTicksDayCounter) {
  // V3: Direct read using standard format (no transformations)

  SaveResult result = SaveFormat::ReadWorldOptions(serializer, outOptions);
  if (!result) return result;

  // Player state
  result = SaveFormat::ReadPlayerState(serializer, outPlayerPos, outCameraPitch,
                                        outCameraYaw);
  if (!result) return result;

  // Tick state
  result = SaveFormat::ReadTickState(serializer, outTicksCounter, outElapsedRealTime,
                                      outTicksDayCounter);
  if (!result) return result;

  // World structure and data
  result = SaveFormat::ReadWorldStructure(serializer, outMap);
  if (!result) return result;

  return SaveFormat::ReadWorldData(serializer, outMap);
}

}  // namespace TyraCraft
