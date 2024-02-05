#pragma once
#include "entities/player/player_render_pip.hpp"
#include "entities/player/player.hpp"
#include "constants.hpp"
#include "math.h"
#include <tyra>

using Tyra::M4x4;
using Tyra::Renderer;
using Tyra::StaPipBag;
using Tyra::StaPipColorBag;
using Tyra::StaPipInfoBag;
using Tyra::StaPipTextureBag;
using Tyra::StaticPipeline;

class PlayerRenderArmPip : public PlayerRenderPip {
 public:
  PlayerRenderArmPip(Player* t_player);
  ~PlayerRenderArmPip();

  void update(const float& deltaTime, Camera* t_camera);
  void render(Renderer* t_render);
  void renderArm(Renderer* t_render);
  void renderItem(Renderer* t_render);

  void loadItemDrawData();
  void unloadItemDrawData();

 private:
  std::vector<Vec4> vertices;
  std::vector<Vec4> uvMap;

  std::array<Vec4, 36> armVertices = {Vec4(-0.119292f, 0.002216f, 0.39074f),
                                      Vec4(-0.198348f, 0.213004f, 0.286706f),
                                      Vec4(-0.114057f, -0.085817f, -0.382805f),
                                      Vec4(-0.114057f, -0.085817f, -0.382805f),
                                      Vec4(-0.198348f, 0.213004f, 0.286706f),
                                      Vec4(0.035002f, 0.296605f, 0.278771f),
                                      Vec4(-0.119292f, 0.002216f, 0.39074f),
                                      Vec4(-0.114057f, -0.085817f, -0.382805f),
                                      Vec4(-0.035002f, -0.296605f, -0.278771f),
                                      Vec4(-0.198348f, 0.213004f, 0.286706f),
                                      Vec4(-0.119292f, 0.002216f, 0.39074f),
                                      Vec4(0.035002f, 0.296605f, 0.278771f),
                                      Vec4(0.119292f, -0.002216f, -0.39074f),
                                      Vec4(-0.114057f, -0.085817f, -0.382805f),
                                      Vec4(0.035002f, 0.296605f, 0.278771f),
                                      Vec4(-0.035002f, -0.296605f, -0.278771f),
                                      Vec4(0.114057f, 0.085817f, 0.382805f),
                                      Vec4(-0.119292f, 0.002216f, 0.39074f),
                                      Vec4(0.198348f, -0.213004f, -0.286706f),
                                      Vec4(0.035002f, 0.296605f, 0.278771f),
                                      Vec4(0.114057f, 0.085817f, 0.382805f),
                                      Vec4(0.035002f, 0.296605f, 0.278771f),
                                      Vec4(-0.119292f, 0.002216f, 0.39074f),
                                      Vec4(0.114057f, 0.085817f, 0.382805f),
                                      Vec4(-0.114057f, -0.085817f, -0.382805f),
                                      Vec4(0.198348f, -0.213004f, -0.286706f),
                                      Vec4(-0.035002f, -0.296605f, -0.278771f),
                                      Vec4(0.119292f, -0.002216f, -0.39074f),
                                      Vec4(0.198348f, -0.213004f, -0.286706f),
                                      Vec4(-0.114057f, -0.085817f, -0.382805f),
                                      Vec4(-0.035002f, -0.296605f, -0.278771f),
                                      Vec4(0.198348f, -0.213004f, -0.286706f),
                                      Vec4(0.114057f, 0.085817f, 0.382805f),
                                      Vec4(0.198348f, -0.213004f, -0.286706f),
                                      Vec4(0.119292f, -0.002216f, -0.39074f),
                                      Vec4(0.035002f, 0.296605f, 0.278771f)};

  std::array<Vec4, 36> armUVMap = {
      Vec4(0.751502f, 0.500326f, 1.0f), Vec4(0.812635f, 0.500363f, 1.0f),
      Vec4(0.812536f, 0.312046f, 1.0f), Vec4(0.689446f, 0.311992f, 1.0f),
      Vec4(0.689443f, 0.50031f, 1.0f),  Vec4(0.750576f, 0.50038f, 1.0f),
      Vec4(0.751502f, 0.500326f, 1.0f), Vec4(0.812536f, 0.312046f, 1.0f),
      Vec4(0.751403f, 0.312009f, 1.0f), Vec4(0.750124f, 0.250073f, 1.0f),
      Vec4(0.750233f, 0.312316f, 1.0f), Vec4(0.812333f, 0.250195f, 1.0f),
      Vec4(0.751403f, 0.312009f, 1.0f), Vec4(0.689446f, 0.311992f, 1.0f),
      Vec4(0.750576f, 0.50038f, 1.0f),  Vec4(0.751403f, 0.312009f, 1.0f),
      Vec4(0.688417f, 0.499619f, 1.0f), Vec4(0.751502f, 0.500326f, 1.0f),
      Vec4(0.812536f, 0.312046f, 1.0f), Vec4(0.750576f, 0.50038f, 1.0f),
      Vec4(0.812635f, 0.500363f, 1.0f), Vec4(0.812333f, 0.250195f, 1.0f),
      Vec4(0.750233f, 0.312316f, 1.0f), Vec4(0.812392f, 0.312311f, 1.0f),
      Vec4(0.689446f, 0.311992f, 1.0f), Vec4(0.687571f, 0.249915f, 1.0f),
      Vec4(0.68769f, 0.249971f, 1.0f),  Vec4(0.751403f, 0.312009f, 1.0f),
      Vec4(0.687571f, 0.249915f, 1.0f), Vec4(0.689446f, 0.311992f, 1.0f),
      Vec4(0.751403f, 0.312009f, 1.0f), Vec4(0.688596f, 0.312624f, 1.0f),
      Vec4(0.688417f, 0.499619f, 1.0f), Vec4(0.812536f, 0.312046f, 1.0f),
      Vec4(0.751403f, 0.312009f, 1.0f), Vec4(0.750576f, 0.50038f, 1.0f),
  };

  StaticPipeline stapip;
  StaPipTextureBag textureBag;
  StaPipInfoBag infoBag;
  StaPipColorBag colorBag;
  StaPipBag bag;

  M4x4 rawMatrix = M4x4::Identity;

  /**
   * - TODO: refactore to a animation service
   * - Possible improvements:
   *   - Calcuate Sin and Cos of animation time just once per frame;
   */

  // Base nimation settings
  float interpolation = 0.0f;
  float animantion_speed = 27.0f;
  float animantion_time = 0.02f;
  const float default_animantion_time = 0.02f;
  const float animantion_time_limit = 6.26f;

  // Walking animation
  float scale = 0.0f;
  Vec4 rotation = Vec4(0, 0, 0);
  Vec4 translation = Vec4(0, 0, 0);

  // Break animation;
  u8 is_playing_break_animation = false;
  const Vec4 breaking_animation_rot_offset = Vec4(-0.3f, 0.34f, 0.0f);
  const Vec4 breaking_animation_pos_offset = Vec4(-1.5F, -1.5F, -3.0F);
  Vec4 breaking_start_rot, breaking_end_rot;
  Vec4 breaking_start_pos, breaking_end_pos;

  // Puting animation;
  u8 is_playing_put_animation = false;
  const Vec4 puting_animation_rot_offset = Vec4(-0.3f, 0.34f, 0.0f);
  const Vec4 puting_animation_pos_offset = Vec4(-1.5F, -1.5F, -3.0F);
  Vec4 puting_start_rot, puting_end_rot;
  Vec4 puting_start_pos, puting_end_pos;

  // Base arm settings
  const Vec4 armRot = Vec4(0.35f, -3.24f, 0.0f);
  const Vec4 armPos = Vec4(4.5F, -5.0F, -8.5F);
  const float armScale = BLOCK_SIZE;

  // Base block settings
  const Vec4 blockRot =
      Vec4(Tyra::Math::ANG2RAD * 10, Tyra::Math::ANG2RAD * -25, 0.0f);
  const Vec4 blockPos = Vec4(4.5F, -5.0F, -10.0F);
  const float blockScale = BLOCK_SIZE / 3.7F;

  // Base torch settings
  const Vec4 torchRot =
      Vec4(Tyra::Math::ANG2RAD * -16, Tyra::Math::ANG2RAD * -50, 0.0f);
  const Vec4 torchPos = Vec4(5.7F, -4.6F, -10.0F);
  const float torchScale = BLOCK_SIZE / 2.5F;

  void updateWalkAnimation(Camera* t_camera);
  void updateBreakAnimation(const float& deltaTime);
  void updatePutAnimation(const float& deltaTime);
};
