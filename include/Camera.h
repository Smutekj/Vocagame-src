#pragma once

#include <View.h>

#include "Utils/Vector2.h"
#include <queue>

class GameObject;
class PostOffice;

struct Camera
{
  enum class MoveState
  {
    FollowingPlayer,
    Fixed,
    MovingToPosition,
    FollowingPath,
  };
  enum class SizeState
  {
    FollowingPlayer,
    Fixed,
    Resizing,
  };

  Camera(utils::Vector2f center, utils::Vector2f size, PostOffice &messanger);

  void update(float dt);
  
  void startMovingTo(utils::Vector2f target, float duration, std::function<void(Camera &)> callback = [](Camera &) {});
  void setPostition(utils::Vector2f pos)
  {
    m_view.setCenter(pos);
  }
  void zoom(float factor);
  void setSize(const utils::Vector2f& size);
  void setSpeed(float speed);
  void startFollowingPath(std::deque<utils::Vector2f> path, float duration, std::function<void(Camera&)> callback = [](Camera&){});
  void followPath(float dt);
  void startChangingSize(utils::Vector2f size, float duration, std::function<void(Camera &)> callback = [](Camera &) {});
  View getView() const;

  void setFolowee(GameObject* followee);

private:
  bool moveToTarget(float dt);
  void resizeToTarget(float dt);
  void followPlayer(float dt);

public:
MoveState m_move_state = MoveState::FollowingPlayer;
SizeState m_view_size_state = SizeState::FollowingPlayer;

bool m_bounded = false;
Rectf m_world_bounds;
Rectf threshold_rect = Rectf{0.5f, 0.5f, 0.f, 0.f};
bool m_can_move_x = true;
bool m_can_move_y = true;

private:
GameObject* m_followee = nullptr;

  float m_move_view_time = 0.;
  float m_move_view_duration = 5.;
  float m_max_view_speed = 150.;
  float m_size_change_speed = 1.;
  float m_resize_view_duration;
  float m_resize_view_time = 0.f;

  utils::Vector2f m_view_target_size;
  utils::Vector2f m_view_target;
  utils::Vector2f m_view_velocity = {0};

  View m_default_view;
  View m_view;

  bool m_player_near_edge = false;

  PostOffice& m_messanger;

  std::function<void(Camera &)> m_on_reaching_target_callback = [](Camera &) {};


  std::deque<utils::Vector2f> m_path;
};
