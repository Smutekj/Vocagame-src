#pragma once

#include "GameObject.h"
#include "EffectsFactory.h"
#include <deque>

using SDL_Keycode = int;
struct SnakeClientEvent
{
    utils::Vector2f pos;
    float angle;
    double time;
    int dir;
    float turn_radius;
    std::string type;
};

class Snake : public GameObject
{

public:
    struct Spec : public GameObjectSpec
    {
        double time = 0.;
        std::size_t tail_size = 5;
        float turn_radius = 30.;
    };

    //!< determines that at moment \p time, the head was at angle and po
    struct PastPoint
    {
        float angle = 0.;
        utils::Vector2f pos;
        float radius;
        float speed;
        double time;
        int dir = 0; //! to know if and where the head was turning
    };
    struct Link
    {
        float angle;
        utils::Vector2f pos;
    };

public:
    Snake(GameWorld &world, const Spec &spec, int ent_id = -1);
    virtual void onCreation() override;
    virtual void update(float dt) override;
    virtual void draw(LayersHolder &target, Assets &assets) override;
    virtual void onCollisionWith(GameObject &obj, CollisionData &c_data) override;

    //! should i refactor these?
    void onEvent(SnakeClientEvent event);
    void onKeyPress(SDL_Keycode keysym);
    void onKeyRelease(SDL_Keycode key);
    void onSprint();
    void onSprintStop();
    void onSpeedUpAdd(float factor);
    void onSpeedUpMult(float factor);
    void onSlowDownAdd(float factor);
    void onSlowDownMult(float factor);
    void onShoot();

    void synchronize(utils::Vector2f pos, float angle, double server_time);

private:
    void loseTail(std::size_t first_lost_id);
    void bounceOff(utils::Vector2f wall_normal);
    
private:
    bool m_sprinting = false;

public:
    double time_x = 0.;

    float m_sprint_speedup = 1.69; //nice

    std::deque<PastPoint> past_points;
    int dir = 0;
    utils::Vector2f head_pos = {200, 200};
    float head_angle = 0.f;
    float delta_angle = 160.f;
    float turn_radius = 20.f;
    double time_factor = 1.f;
    float link_delay = 25.f / 150.f;
    float speed = 150.f;
    struct EatInfo
    {
        double poop_time;
        double time;
        utils::Vector2f pos;
        float angle;
    };
    float link_dist = 25.f;
    std::deque<Link> tail;
    std::deque<EatInfo> eat_times;

private:
    EffectsFactory m_effects_maker;
};

Snake::PastPoint extrapolate(const Snake::PastPoint &point, float speed, double time_factor, double time);