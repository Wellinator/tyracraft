#pragma once

#include "managers/save/save_serializer.hpp"
#include "models/new_game_model.hpp"
#include "entities/level.hpp"
#include "tyra"

namespace TyraCraft {

/**
 * @brief Schema definition for save file format (version 3)
 *
 * Defines the structure of data written to/read from save files.
 * Encapsulates all primitive read/write operations to prevent duplicating
 * field access logic across multiple LoadSavedGameVx() functions.
 *
 * This makes it easy to:
 * - Add new optional fields (via bit flags)
 * - Create migrations between versions
 * - Test serialization independently
 *
 * Save Format (V3):
 *   [Magic Number]           u32    "TCWS" identifier
 *   [Version]                i32    3
 *   [World Seed]             u32
 *   [Game Mode]              u8
 *   [World Name]             u16 len + chars
 *   [Draw Distance Mode]     u8
 *   [Initial Time]           f32
 *   [World Type]             u8
 *   [Texture Pack Name]      u16 len + chars
 *   [Player Position]        f32 x,y,z,w
 *   [Camera Pitch]           f32
 *   [Camera Yaw]             f32
 *   [Tick Counter]           u64
 *   [Elapsed Real Time]      u64
 *   [Ticks Day Counter]      u64
 *   [World Dimension Data]   u16 width, length, height
 *   [Spawn Position]         u16 x,y,z (3 × u16)
 *   [World Size]             u32
 *   [Block Data]             1MB
 *   [Light Data]             1MB
 *   [Metadata]               1MB
 *   [Optional Fields Flags]  u8  (bit 0: has_inventory, bit 1: reserved...)
 *   [Hot Inventory]          (optional, if flag set)
 *
 * Total: ~3MB compressed with gzip
 */
class SaveFormat {
 public:
  // Version tracking
  static constexpr int32_t SAVE_FORMAT_VERSION = 3;

  // ========================================================================
  // METADATA I/O (everything before world data)
  // ========================================================================

  /**
   * Writes save file header: magic number and version
   */
  static SaveResult WriteHeader(SaveSerializer& serializer) {
    SaveResult result = serializer.WriteMagicNumber();
    if (!result) return result;
    return serializer.WriteInt32(SAVE_FORMAT_VERSION);
  }

  /**
   * Reads and validates save file header
   */
  static SaveResult ReadHeader(SaveSerializer& serializer, int32_t& outVersion) {
    SaveResult result = serializer.ValidateMagicNumber();
    if (!result) return result;
    return serializer.ReadInt32(&outVersion);
  }

  /**
   * Writes world options (seed, game mode, name, texture pack, etc.)
   * @note Does NOT write drawDistanceMode - handled separately per version
   */
  static SaveResult WriteWorldOptions(SaveSerializer& serializer,
                                       const NewGameOptions* options) {
    SaveResult result = serializer.WriteUInt32(options->seed);
    if (!result) return result;

    result = serializer.WriteUInt8((uint8_t)options->gameMode);
    if (!result) return result;

    result = serializer.WriteString(options->name, SaveSerializer::MAX_SAVE_NAME_LENGTH);
    if (!result) return result;

    // V3: Write draw distance mode in correct enum format
    result = serializer.WriteUInt8((uint8_t)options->drawDistanceMode);
    if (!result) return result;

    result = serializer.WriteFloat(options->initialTime);
    if (!result) return result;

    result = serializer.WriteUInt8((uint8_t)options->type);
    if (!result) return result;

    result = serializer.WriteString(options->texturePack,
                                    SaveSerializer::MAX_TEXTURE_PACK_NAME);
    if (!result) return result;

    return SaveResult::Success();
  }

  /**
   * Reads world options from save file (V3 format)
   * Expects to read seed, gameMode, name, drawDistanceMode, initialTime, type, texturePack
   * in that order.
   *
   * @note For V1/V2 migration, use individual Read* calls as draw distance format differs
   */
  static SaveResult ReadWorldOptions(SaveSerializer& serializer,
                                      NewGameOptions* outOptions) {
    SaveResult result = serializer.ReadUInt32(&outOptions->seed);
    if (!result) return result;

    uint8_t gameMode;
    result = serializer.ReadUInt8(&gameMode);
    if (!result) return result;
    outOptions->gameMode = (GameMode)gameMode;

    result = serializer.ReadString(&outOptions->name,
                                   SaveSerializer::MAX_SAVE_NAME_LENGTH);
    if (!result) return result;

    uint8_t drawMode;
    result = serializer.ReadUInt8(&drawMode);
    if (!result) return result;
    outOptions->drawDistanceMode = (DrawDistanceMode)drawMode;

    result = serializer.ReadFloat(&outOptions->initialTime);
    if (!result) return result;

    uint8_t worldType;
    result = serializer.ReadUInt8(&worldType);
    if (!result) return result;
    outOptions->type = (WorldType)worldType;

    result = serializer.ReadString(&outOptions->texturePack,
                                   SaveSerializer::MAX_TEXTURE_PACK_NAME);
    if (!result) return result;

    return SaveResult::Success();
  }

  // ========================================================================
  // WORLD OPTIONS PARTIAL READ (for menu/metadata only)
  // ========================================================================

  /**
   * Reads partial world options (seed, gameMode, name) used for UI lists
   * Does NOT read draw distance mode, initialTime, type, or texture pack
   */
  static SaveResult ReadWorldOptionsPartial(SaveSerializer& serializer,
                                             NewGameOptions* outOptions) {
    SaveResult result = serializer.ReadUInt32(&outOptions->seed);
    if (!result) return result;

    uint8_t gameMode;
    result = serializer.ReadUInt8(&gameMode);
    if (!result) return result;
    outOptions->gameMode = (GameMode)gameMode;

    result = serializer.ReadString(&outOptions->name,
                                   SaveSerializer::MAX_SAVE_NAME_LENGTH);
    if (!result) return result;

    return SaveResult::Success();
  }

  /**
   * Reads world name only, skipping seed and gameMode
   * Used for metadata where only the name is needed
   */
  static SaveResult ReadWorldNameOnly(SaveSerializer& serializer,
                                       std::string* outName) {
    uint32_t seed;  // Skip
    SaveResult result = serializer.ReadUInt32(&seed);
    if (!result) return result;

    uint8_t gameMode;  // Skip
    result = serializer.ReadUInt8(&gameMode);
    if (!result) return result;

    return serializer.ReadString(outName,
                                SaveSerializer::MAX_SAVE_NAME_LENGTH);
  }

  // ========================================================================
  // PLAYER & CAMERA STATE I/O
  // ========================================================================

  /**
   * Writes player position and camera direction
   */
  static SaveResult WritePlayerState(SaveSerializer& serializer,
                                      const Tyra::Vec4& playerPos,
                                      float cameraPitch, float cameraYaw) {
    SaveResult result = serializer.WriteVec4(playerPos);
    if (!result) return result;

    result = serializer.WriteFloat(cameraPitch);
    if (!result) return result;

    return serializer.WriteFloat(cameraYaw);
  }

  /**
   * Reads player position and camera direction
   */
  static SaveResult ReadPlayerState(SaveSerializer& serializer,
                                     Tyra::Vec4* outPlayerPos,
                                     float* outPitch, float* outYaw) {
    SaveResult result = serializer.ReadVec4(outPlayerPos);
    if (!result) return result;

    result = serializer.ReadFloat(outPitch);
    if (!result) return result;

    return serializer.ReadFloat(outYaw);
  }

  // ========================================================================
  // TIME & TICK STATE I/O
  // ========================================================================

  /**
   * Writes tick counter and time state
   */
  static SaveResult WriteTickState(SaveSerializer& serializer,
                                    uint64_t ticksCounter,
                                    uint64_t elapsedRealTime,
                                    uint64_t ticksDayCounter) {
    SaveResult result = serializer.WriteBuffer(&ticksCounter, sizeof(ticksCounter));
    if (!result) return result;

    result = serializer.WriteBuffer(&elapsedRealTime, sizeof(elapsedRealTime));
    if (!result) return result;

    return serializer.WriteBuffer(&ticksDayCounter, sizeof(ticksDayCounter));
  }

  /**
   * Reads tick counter and time state
   */
  static SaveResult ReadTickState(SaveSerializer& serializer,
                                   uint64_t* outTicksCounter,
                                   uint64_t* outElapsedRealTime,
                                   uint64_t* outTicksDayCounter) {
    SaveResult result = serializer.ReadBuffer(outTicksCounter, sizeof(*outTicksCounter));
    if (!result) return result;

    result = serializer.ReadBuffer(outElapsedRealTime, sizeof(*outElapsedRealTime));
    if (!result) return result;

    return serializer.ReadBuffer(outTicksDayCounter, sizeof(*outTicksDayCounter));
  }

  // ========================================================================
  // WORLD MAP STRUCTURE I/O
  // ========================================================================

  /**
   * Writes world map dimensions and spawn location
   */
  static SaveResult WriteWorldStructure(SaveSerializer& serializer,
                                         const LevelMap* map) {
    SaveResult result = serializer.WriteUInt16(map->width);
    if (!result) return result;

    result = serializer.WriteUInt16(map->length);
    if (!result) return result;

    result = serializer.WriteUInt16(map->height);
    if (!result) return result;

    result = serializer.WriteUInt16(map->spawnX);
    if (!result) return result;

    result = serializer.WriteUInt16(map->spawnY);
    if (!result) return result;

    result = serializer.WriteUInt16(map->spawnZ);
    if (!result) return result;

    uint32_t worldSize = OVERWORLD_SIZE;
    return serializer.WriteUInt32(worldSize);
  }

  /**
   * Reads world map dimensions and spawn location
   */
  static SaveResult ReadWorldStructure(SaveSerializer& serializer,
                                        LevelMap* outMap) {
    SaveResult result = serializer.ReadUInt16(&outMap->width);
    if (!result) return result;

    result = serializer.ReadUInt16(&outMap->length);
    if (!result) return result;

    result = serializer.ReadUInt16(&outMap->height);
    if (!result) return result;

    result = serializer.ReadUInt16(&outMap->spawnX);
    if (!result) return result;

    result = serializer.ReadUInt16(&outMap->spawnY);
    if (!result) return result;

    result = serializer.ReadUInt16(&outMap->spawnZ);
    if (!result) return result;

    uint32_t worldSize;
    result = serializer.ReadUInt32(&worldSize);
    if (!result) return result;

    // Validate world size matches expected
    if (worldSize != OVERWORLD_SIZE) {
      return SaveResult::Failure("World size mismatch - save file may be corrupted");
    }

    return SaveResult::Success();
  }

  // ========================================================================
  // RAW WORLD DATA I/O (~3MB)
  // ========================================================================

  /**
   * Writes all world block, light, and metadata arrays
   */
  static SaveResult WriteWorldData(SaveSerializer& serializer,
                                    const LevelMap* map) {
    SaveResult result = serializer.WriteBuffer(map->blocks, sizeof(map->blocks));
    if (!result) return result;

    result = serializer.WriteBuffer(map->lightData, sizeof(map->lightData));
    if (!result) return result;

    return serializer.WriteBuffer(map->metaData, sizeof(map->metaData));
  }

  /**
   * Reads all world block, light, and metadata arrays
   */
  static SaveResult ReadWorldData(SaveSerializer& serializer, LevelMap* outMap) {
    SaveResult result = serializer.ReadBuffer(outMap->blocks, sizeof(outMap->blocks));
    if (!result) return result;

    result = serializer.ReadBuffer(outMap->lightData, sizeof(outMap->lightData));
    if (!result) return result;

    return serializer.ReadBuffer(outMap->metaData, sizeof(outMap->metaData));
  }
};

}  // namespace TyraCraft
