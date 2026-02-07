#pragma once

#include "GameObject.h"

#include <Sprite.h>
#include <filesystem>
#include <unordered_set>
#include "TimedEventManager.h"
#include "Vertex.h"

class GameWorld;
class Renderer;

struct Player : public GameObject
{

    enum class HullAnim{
        Breathing,
        RunningLeft,
        RunningRight,
        Jumping,
        StoppingLeft,
        StoppingRight,
        Jiggling,
    };

    enum class ColorAnim{
        Breathing,
        Success,
        Failure,
        RunningLeft,
        RunningRight,
        Jumping,
        FellDown,
    };
    
    enum class EyeAnim{
        Breathing,
        Blinking,
        Jumping,
        RunningLeft,
        RunningRight,
        StoppingLeft,
        StoppingRight,
    };
public:
    Player(GameWorld *world, int ent_id);

    virtual ~Player() = default;

    virtual void update(float dt) override;
    virtual void draw(LayersHolder &target, Assets &assets) override;
    virtual void onCreation() override;
    virtual void onDestruction() override;
    virtual void onCollisionWith(GameObject &obj, CollisionData &c_data) override;

    float getHp() const;
    float getHpRatio() const;

    void onJump();
    void onRunStart(int dir);
    void onRunEnd(int dir);

private:
    void drawOctagon(utils::Vector2f center, float radius, float rotation, Color color, Renderer &canvas);

    void switchEyeAnim(EyeAnim anim);
    void switchHullAnim(HullAnim anim);
    void switchColorAnim(ColorAnim anim);

public:
    utils::Vector2f m_impulse_vel = {0.f};
    std::unordered_set<int> m_touching_platforms_ids;

    float speed = 0.f;

    float m_gravity = 1300.5f;
    float m_gravity_falling = 2000.f;
    float m_fall_speed_limit = 2000;
    float m_jump_vel = 800.f;
    bool m_is_falling = false;
    bool m_is_jumping = false;
    bool m_first_step = true;
    int m_standing_platform_id = -1;
    GameObject *m_standing_platform = nullptr;
    utils::Vector2f m_platform_vel = {0, 0};

    float m_move_speed = 400.f;

    bool m_moving_left = false;
    bool m_moving_right = false;
    bool m_on_ground = false;
    int m_jump_count = 0;
    float m_jump_cooldown = 1.0f;
    float m_jump_timer = 0.f;

    bool m_passed_speed_gate = false;
    bool m_accelerating = false;
    bool m_deccelerating = false;

    float m_boost_max_speed = 150.f;
    float m_boost_factor = 2.f;
    float m_slow_boost_factor = 0.3f;
    float m_slow_factor = 12.5f;
    float acceleration = 1.5f;

    float health = 3;
    float max_health = 10;

    Sprite m_player_shape;

    std::vector<Color> m_vertex_colors;
    std::vector<Vertex> m_verts;

    TimedEventManager m_timers;
    float m_rotation = 0.f;

    float m_run_anim_disp = 0.f;
    
    float eye_y_pos = m_pos.y + m_size.x / 7.f;
    float eye_x_disp = 0.f;
    float eye_distance = m_size.x / 6.f;
    utils::Vector2f eye_size;
    Color eye_color = {4,4,0,1};
    
    int m_run_anim_id = -1;
    
    int m_active_hull_anim_id = -1;
    HullAnim m_active_hull_anim = HullAnim::Breathing;
    
    int m_active_eye_anim_id = -1;
    EyeAnim m_active_eye_anim = EyeAnim::Breathing;
    
    int m_active_color_anim_id = -1;
    ColorAnim m_active_color_anim = ColorAnim::Breathing;
        
private:
};

using PlayeEntity = Player;
